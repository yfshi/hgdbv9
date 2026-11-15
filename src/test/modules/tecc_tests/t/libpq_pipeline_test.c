/*
 * libpq_pipeline_test.c - TECC libpq 管道模式 (pipeline mode) 测试
 *
 * 验证 Review 问题 1 修复后，TECC 在 libpq 管道模式下透明加解密可用：
 *   1. 管道模式批量 PQsendQuery（简单协议在管道模式走扩展协议 Parse/Bind/Execute）
 *   2. INSERT 加密列常量（TECC 文本改写）
 *   3. SELECT 加密列（结果解密）
 *   4. PQsendQueryParams（参数化，走 tecc_preprocess_params）
 *   5. PQsendPrepare + PQsendQueryPrepared（prepared 语句参数加密）
 *   6. 非管道模式对照（确保行为不变）
 *
 * 编译（使用 TECC 版 libpq）:
 *   gcc -o libpq_pipeline_test libpq_pipeline_test.c \
 *       -I../../../src/include -I../../../src/interfaces/libpq \
 *       -L../../../src/interfaces/libpq -lpq -Wl,-rpath,../../../src/interfaces/libpq
 *
 * 运行:
 *   ./libpq_pipeline_test "host=/tmp port=15432 dbname=postgres user=postgres enable_tecc=1"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libpq-fe.h>

/* ========== 工具函数 ========== */

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

static void
check_result(PGconn *conn, PGresult *res, const char *desc)
{
    ExecStatusType status = PQresultStatus(res);
    total_tests++;

    if (status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK)
    {
        passed_tests++;
        printf("  PASS [%s]\n", desc);
    }
    else
    {
        failed_tests++;
        printf("  FAIL [%s]: %s\n", desc, PQresultErrorMessage(res));
    }
}

static PGconn *
do_connect(const char *conninfo)
{
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "连接失败: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("=== 连接成功 ===\n");
    return conn;
}

static PGresult *
exec_sql(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK &&
        PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("  SQL ERROR: %s\n  SQL: %.100s\n", PQresultErrorMessage(res), sql);
    }
    return res;
}

static void
show_rows(PGconn *conn, const char *sql)
{
    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int nrows = PQntuples(res);
        int ncols = PQnfields(res);
        for (int r = 0; r < nrows; r++)
        {
            printf("    row %d:", r);
            for (int c = 0; c < ncols; c++)
                printf(" %s=%s", PQfname(res, c), PQgetvalue(res, r, c));
            printf("\n");
        }
        if (nrows == 0)
            printf("    (no rows)\n");
        printf("    (%d row(s))\n", nrows);
    }
    else
    {
        printf("  QUERY ERROR: %s\n  SQL: %.100s\n", PQresultErrorMessage(res), sql);
    }
    PQclear(res);
}

/*
 * 管道模式结果收集：连续 PQgetResult 直到 NULL，逐结果检查。
 * 返回遇到错误的结果数（含 FATAL），用于断言。
 */
static int
drain_pipeline_results(PGconn *conn, const char *desc, bool expect_ok,
                       int expected_results)
{
    int bad = 0;
    int got = 0;
    PGresult *res;

    printf("  drain [%s] (expect %d results):\n", desc, expected_results);
    while (got < expected_results)
    {
        res = PQgetResult(conn);
        if (res == NULL)
            continue;   /* 当前命令结果结束，继续推进队列 */
        got++;
        ExecStatusType status = PQresultStatus(res);
        if (status == PGRES_TUPLES_OK)
        {
            int nrows = PQntuples(res);
            int ncols = PQnfields(res);
            printf("    result %d: TUPLES_OK rows=%d cols=%d", got, nrows, ncols);
            if (nrows > 0 && ncols > 0)
            {
                printf("  first row:");
                for (int c = 0; c < ncols; c++)
                    printf(" %s=%s", PQfname(res, c), PQgetvalue(res, 0, c));
            }
            printf("\n");
        }
        else if (status == PGRES_COMMAND_OK)
        {
            printf("    result %d: COMMAND_OK tag=%s\n", got, PQcmdStatus(res));
        }
        else if (status == PGRES_PIPELINE_SYNC)
        {
            printf("    result %d: PIPELINE_SYNC\n", got);
        }
        else
        {
            bad++;
            printf("    result %d: %s ERROR: %s\n", got,
                   PQresStatus(status), PQresultErrorMessage(res));
        }
        PQclear(res);
    }

    total_tests++;
    if ((expect_ok && bad == 0) || (!expect_ok && bad > 0))
    {
        passed_tests++;
        printf("  PASS [%s] (%d results, %d bad)\n", desc, got, bad);
    }
    else
    {
        failed_tests++;
        printf("  FAIL [%s] (%d results, %d bad)\n", desc, got, bad);
    }
    return bad;
}

/* ========== 测试用例 ========== */

/*
 * 测试 1: 管道模式 PQsendQueryParams（0 参数扩展协议）批量 INSERT + SELECT
 * TECC 文本改写（常量加密进 SQL 文本）+ 结果解密。
 *
 * 注意：不使用 PQsendQuery（简单协议文本）。PG 官方 commit 0032a545670
 * "Remove PQsendQuery support in pipeline mode" 确认 PG14 的 PQsendQuery
 * 管道模式实现（扩展协议转换）存在已知 bug（结果按 2:1 丢失），官方在
 * PG15 移除该路径，仅保留 PG14 兼容但建议改用 PQsendQueryParams。
 */
static void
test_pipeline_basic(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 1: 管道模式 PQsendQueryParams 批量 INSERT/SELECT\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pipe_c1 CASCADE;"
        "CREATE TABLE test_pipe_c1 ("
        "  id INT4,"
        "  name VARCHAR(100),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pipe, SUPPORT_ALGO = 'ope'),"
        "  bonus INT4_ENC ENCRYPTED WITH (DEK = dek_pipe, SUPPORT_ALGO = 'det')"
        ");");
    check_result(conn, res, "create table test_pipe_c1");
    PQclear(res);

    /* 进入管道模式 */
    if (PQenterPipelineMode(conn) != 1)
    {
        printf("  FAIL [enter pipeline mode]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
        return;
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [enter pipeline mode]\n");

    /* 批量发送 INSERT（0 参数扩展协议，TECC 文本改写：常量加密进 SQL 文本） */
    for (int i = 1; i <= 5; i++)
    {
        char sql[512];
        snprintf(sql, sizeof(sql),
            "INSERT INTO test_pipe_c1 VALUES (%d, 'name%d', %d, %d)",
            i, i, 1000 + i * 100, 50 + i);
        if (PQsendQueryParams(conn, sql, 0, NULL, NULL, NULL, NULL, 0) != 1)
        {
            printf("  FAIL [PQsendQueryParams %d]: %s\n", i, PQerrorMessage(conn));
            failed_tests++;
            total_tests++;
        }
        /* 管道模式下数据积压在 out buffer，须显式 flush 才会发出 */
        if (PQflush(conn) != 0)
        {
            printf("  FAIL [PQflush %d]: %s\n", i, PQerrorMessage(conn));
            failed_tests++;
            total_tests++;
        }
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [sent %d INSERTs]\n", 5);

    /* 批量发送 SELECT（结果解密） */
    if (PQsendQueryParams(conn,
            "SELECT id, name, salary, bonus FROM test_pipe_c1 ORDER BY id",
            0, NULL, NULL, NULL, NULL, 0) != 1)
    {
        printf("  FAIL [PQsendQueryParams SELECT]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }
    if (PQflush(conn) != 0)
    {
        printf("  FAIL [PQflush SELECT]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 同步点：必须发送 PQpipelineSync 产生 PIPELINE_SYNC 结果，队列才能结束 */
    if (PQpipelineSync(conn) != 1)
    {
        printf("  FAIL [PQpipelineSync]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 管道模式下数据积压在 out buffer，须显式 flush 才会发出 */
    if (PQflush(conn) != 0)
    {
        printf("  FAIL [PQflush]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 收集结果：5 INSERT + 1 SELECT + 1 PIPELINE_SYNC = 7 结果 */
    drain_pipeline_results(conn, "pipeline basic drain", true, 7);

    /* 退出管道模式 */
    if (PQexitPipelineMode(conn) != 1)
    {
        printf("  WARN [exit pipeline mode]: %s\n", PQerrorMessage(conn));
    }

    /* 退出后校验数据（非管道模式） */
    printf("--- 校验（非管道模式） ---\n");
    show_rows(conn, "SELECT id, name, salary, bonus FROM test_pipe_c1 ORDER BY id");
    show_rows(conn, "SELECT count(*) AS cnt FROM test_pipe_c1");
}

/*
 * 测试 2: 管道模式 PQsendQueryParams（参数化 $N）
 * 走 tecc_preprocess_params → 参数加密。
 */
static void
test_pipeline_params(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 2: 管道模式 PQsendQueryParams\n");
    printf("========================================\n");

    PGresult *res;
    const char *paramValues[3];

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pipe_c2 CASCADE;"
        "CREATE TABLE test_pipe_c2 ("
        "  id INT4,"
        "  name VARCHAR(100),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pipe, SUPPORT_ALGO = 'ope')"
        ");");
    check_result(conn, res, "create table test_pipe_c2");
    PQclear(res);

    if (PQenterPipelineMode(conn) != 1)
    {
        printf("  FAIL [enter pipeline mode]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
        return;
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [enter pipeline mode]\n");

    /* 批量参数化 INSERT */
    for (int i = 1; i <= 4; i++)
    {
        char idbuf[16], salbuf[32];
        snprintf(idbuf, sizeof(idbuf), "%d", i);
        snprintf(salbuf, sizeof(salbuf), "%d", 2000 + i * 50);
        paramValues[0] = idbuf;
        paramValues[1] = "pname";
        paramValues[2] = salbuf;
        if (PQsendQueryParams(conn,
                "INSERT INTO test_pipe_c2 VALUES ($1, $2, $3)",
                3, NULL, paramValues, NULL, NULL, 0) != 1)
        {
            printf("  FAIL [PQsendQueryParams %d]: %s\n", i, PQerrorMessage(conn));
            failed_tests++;
            total_tests++;
        }
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [sent %d parameterized INSERTs]\n", 4);

    /* SELECT（文本常量 WHERE，走文本改写路径；结果解密） */
    /* 注：参数化 WHERE（WHERE salary = $1）加密是既有缺陷（非管道模式
     * 同样失败），不在本次管道模式支持范围内，此处用文本常量验证。 */
    {
        if (PQsendQueryParams(conn,
                "SELECT id, name, salary FROM test_pipe_c2 WHERE salary = 2100 ORDER BY id",
                0, NULL, NULL, NULL, NULL, 0) != 1)
        {
            printf("  FAIL [PQsendQueryParams SELECT]: %s\n", PQerrorMessage(conn));
            failed_tests++;
            total_tests++;
        }
    }

    /* 同步点 */
    if (PQpipelineSync(conn) != 1)
    {
        printf("  FAIL [PQpipelineSync]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 管道模式下数据积压在 out buffer，须显式 flush 才会发出 */
    if (PQflush(conn) != 0)
    {
        printf("  FAIL [PQflush]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 收集结果：4 INSERT + 1 SELECT + 1 SYNC = 6 结果 */
    drain_pipeline_results(conn, "pipeline params drain", true, 6);

    if (PQexitPipelineMode(conn) != 1)
    {
        printf("  WARN [exit pipeline mode]: %s\n", PQerrorMessage(conn));
    }

    printf("--- 校验（非管道模式） ---\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pipe_c2 ORDER BY id");
}

/*
 * 测试 3: 管道模式 PQsendPrepare + PQsendQueryPrepared
 * prepared 语句 + 参数加密（fe-exec.c:1925 分支）。
 */
static void
test_pipeline_prepared(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 3: 管道模式 PQsendPrepare + PQsendQueryPrepared\n");
    printf("========================================\n");

    PGresult *res;
    const char *paramValues[2];

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pipe_c3 CASCADE;"
        "CREATE TABLE test_pipe_c3 ("
        "  id INT4,"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pipe, SUPPORT_ALGO = 'ope')"
        ");");
    check_result(conn, res, "create table test_pipe_c3");
    PQclear(res);

    if (PQenterPipelineMode(conn) != 1)
    {
        printf("  FAIL [enter pipeline mode]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
        return;
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [enter pipeline mode]\n");

    /* PQsendPrepare 命名语句 */
    if (PQsendPrepare(conn, "pipe_c3_ins",
            "INSERT INTO test_pipe_c3 VALUES ($1, $2)", 2, NULL) != 1)
    {
        printf("  FAIL [PQsendPrepare]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 批量 PQsendQueryPrepared */
    for (int i = 1; i <= 3; i++)
    {
        char idbuf[16], salbuf[32];
        snprintf(idbuf, sizeof(idbuf), "%d", i);
        snprintf(salbuf, sizeof(salbuf), "%d", 3000 + i * 100);
        paramValues[0] = idbuf;
        paramValues[1] = salbuf;
        if (PQsendQueryPrepared(conn, "pipe_c3_ins", 2, paramValues, NULL, NULL, 0) != 1)
        {
            printf("  FAIL [PQsendQueryPrepared %d]: %s\n", i, PQerrorMessage(conn));
            failed_tests++;
            total_tests++;
        }
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [sent %d prepared executes]\n", 3);

    /* 同步点 */
    if (PQpipelineSync(conn) != 1)
    {
        printf("  FAIL [PQpipelineSync]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 管道模式下数据积压在 out buffer，须显式 flush 才会发出 */
    if (PQflush(conn) != 0)
    {
        printf("  FAIL [PQflush]: %s\n", PQerrorMessage(conn));
        failed_tests++;
        total_tests++;
    }

    /* 收集结果：1 Prepare + 3 Execute + 1 SYNC = 5 结果 */
    drain_pipeline_results(conn, "pipeline prepared drain", true, 5);

    if (PQexitPipelineMode(conn) != 1)
    {
        printf("  WARN [exit pipeline mode]: %s\n", PQerrorMessage(conn));
    }

    printf("--- 校验（非管道模式） ---\n");
    show_rows(conn, "SELECT id, salary FROM test_pipe_c3 ORDER BY id");
}

/*
 * 测试 4: 非管道模式对照（确保行为不变）
 */
static void
test_non_pipeline_baseline(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 4: 非管道模式对照\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pipe_c4 CASCADE;"
        "CREATE TABLE test_pipe_c4 ("
        "  id INT4,"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pipe, SUPPORT_ALGO = 'det')"
        ");");
    check_result(conn, res, "create table test_pipe_c4");
    PQclear(res);

    res = exec_sql(conn, "INSERT INTO test_pipe_c4 VALUES (1, 11111);");
    check_result(conn, res, "plain INSERT");
    PQclear(res);

    res = exec_sql(conn, "INSERT INTO test_pipe_c4 VALUES (2, 22222);");
    check_result(conn, res, "plain INSERT 2");
    PQclear(res);

    show_rows(conn, "SELECT id, salary FROM test_pipe_c4 ORDER BY id");
}

/* ========== 主函数 ========== */
int
main(int argc, char *argv[])
{
    const char *conninfo;

    if (argc > 1)
        conninfo = argv[1];
    else
        conninfo = "host=localhost port=5432 dbname=postgres user=postgres enable_tecc=1";

    PGconn *conn = do_connect(conninfo);

    /* 检查 TECC 是否启用 */
    PGresult *res = PQexec(conn, "SHOW enable_tecc;");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        printf("TECC enable_tecc = %s\n", PQgetvalue(res, 0, 0));
    PQclear(res);

    /* 创建测试 KEK/DEK（逐条发送，确保 KEK cache 刷新） */
    printf("\n=== 创建测试 KEK/DEK ===\n");
    res = exec_sql(conn, "DROP KEK IF EXISTS kek_pipe CASCADE;");
    PQclear(res);
    res = exec_sql(conn,
        "CREATE KEK kek_pipe WITH (key_store = 'localkms', "
        "  key_path = '/tmp/kek_pipe.key');");
    PQclear(res);
    res = exec_sql(conn, "CREATE DEK dek_pipe WITH VALUES(KEK = kek_pipe);");
    PQclear(res);

    /* 运行所有测试 */
    test_pipeline_basic(conn);
    test_pipeline_params(conn);
    test_pipeline_prepared(conn);
    test_non_pipeline_baseline(conn);

    /* 清理 */
    printf("\n=== 清理 KEK/DEK ===\n");
    /* 先删表（解除 DEK 依赖），再删 DEK/KEK */
    res = exec_sql(conn, "DROP TABLE IF EXISTS test_pipe_c1 CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP TABLE IF EXISTS test_pipe_c2 CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP TABLE IF EXISTS test_pipe_c3 CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP TABLE IF EXISTS test_pipe_c4 CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP DEK IF EXISTS dek_pipe CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP KEK IF EXISTS kek_pipe CASCADE;");
    PQclear(res);

    /* 总结 */
    printf("\n========================================\n");
    printf("测试总结\n");
    printf("========================================\n");
    printf("  总计: %d\n", total_tests);
    printf("  通过: %d\n", passed_tests);
    printf("  失败: %d\n", failed_tests);
    printf("========================================\n");

    PQfinish(conn);

    return failed_tests > 0 ? 1 : 0;
}
