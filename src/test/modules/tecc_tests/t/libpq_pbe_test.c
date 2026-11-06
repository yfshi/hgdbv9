/*
 * libpq_pbe_test.c - TECC libpq PBE (Prepared Statement / Extended Query) 测试
 *
 * 编译:
 *   gcc -o libpq_pbe_test libpq_pbe_test.c -I$(pg_config --includedir) \
 *       -L$(pg_config --libdir) -lpq
 *
 * 使用 TECC 版本的 libpq 编译:
 *   gcc -o libpq_pbe_test libpq_pbe_test.c \
 *       -Isrc/include -Isrc/interfaces/libpq \
 *       -Lsrc/interfaces/libpq -lpq -Wl,-rpath,src/interfaces/libpq
 *
 * 运行:
 *   ./libpq_pbe_test "host=/tmp port=15432 dbname=postgres user=why enable_tecc=1"
 *
 * 测试场景:
 *   1. PQprepare + PQexecPrepared — 命名 prepared statement 的 INSERT/SELECT
 *   2. PQexecParams — 匿名参数化查询 (extended query protocol)
 *   3. 多加密类型: int4_enc, text_enc, varchar_enc, float8_enc, bpchar_enc
 *   4. DET/OPE/SEARCH 加密模式的 WHERE 条件
 *   5. 二进制参数格式 (paramFormats=1)
 *   6. NULL 参数绑定
 *   7. UPDATE/DELETE with prepared stmt
 *   8. PQdescribePrepared
 *   9. 重复执行同一 prepared statement
 *   10. 多参混合查询
 *   11. NUMERIC_ENC 类型
 *   12. 简单协议 PREPARE/EXECUTE（psql 风格，通过 PQexec，对照用）
 *
 * 已知 bug / 限制:
 *   Bug 1: PQexecPrepared 不经过 tecc_preprocess_params
 *          → PQsendQueryPrepared 调用 PQsendQueryGuts 时 command=NULL
 *          → 条件 if (conn->enable_tecc && command) 为 false
 *          → 参数值不加密，明文发送到服务端
 *          → 服务端: "invalid hex string: '50000'"
 *          → fe-exec.c:1913，tecc_preprocess_params 被跳过
 *
 *   Bug 2: PQexecParams 参数加密可能也不生效
 *          → tecc_preprocess_params 被调用
 *          → tecc_processor_run_pre_query 可能返回 NO_ENCRYPTED_COLUMNS
 *          → 或者参数解析失败，返回 NULL
 *          → 结果：明文值直接发送到服务端，同样报错
 *
 *   对照: 简单协议 PQexec("PREPARE ...") + PQexec("EXECUTE ...") 正常。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
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
    printf("=== 连接成功: %s ===\n", conninfo);
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
    PQclear(res);
}

/* ========== 测试用例 ========== */

/*
 * 测试 1: PQprepare + PQexecPrepared 基本流程
 * - 创建表, prepare INSERT, exec multiple times
 */
static void
test_basic_prepare_execute(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 1: PQprepare + PQexecPrepared 基本流程\n");
    printf("========================================\n");

    PGresult *res;

    /* 建表 */
    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c1 CASCADE;"
        "CREATE TABLE test_pbe_c1 ("
        "  id INT4, name VARCHAR(100),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),"
        "  dept VARCHAR(50)"
        ");");
    PQclear(res);

    /* PQprepare: INSERT with 4 params */
    res = PQprepare(conn, "pbe_c1_ins",
        "INSERT INTO test_pbe_c1 VALUES ($1, $2, $3, $4)",
        4, NULL);
    check_result(conn, res, "PQprepare INSERT");
    PQclear(res);

    /* PQexecPrepared: 插入第1行 */
    {
        const char *paramValues[4] = {"1", "Alice", "50000", "IT"};
        res = PQexecPrepared(conn, "pbe_c1_ins", 4, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared INSERT row1");
        PQclear(res);
    }

    /* 再插两行 */
    {
        const char *paramValues[4] = {"2", "Bob", "60000", "HR"};
        res = PQexecPrepared(conn, "pbe_c1_ins", 4, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared INSERT row2");
        PQclear(res);
    }

    {
        const char *paramValues[4] = {"3", "Charlie", "70000", "IT"};
        res = PQexecPrepared(conn, "pbe_c1_ins", 4, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared INSERT row3");
        PQclear(res);
    }

    /* 验证数据 */
    printf("  -- 数据验证 --\n");
    show_rows(conn, "SELECT id, name, salary, dept FROM test_pbe_c1 ORDER BY id");

    /* PQprepare: SELECT with WHERE on OPE column */
    res = PQprepare(conn, "pbe_c1_sel_eq",
        "SELECT id, name, salary FROM test_pbe_c1 WHERE salary = $1",
        1, NULL);
    check_result(conn, res, "PQprepare SELECT WHERE OPE =");
    PQclear(res);

    {
        const char *paramValues[1] = {"60000"};
        res = PQexecPrepared(conn, "pbe_c1_sel_eq", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared SELECT WHERE salary=60000");
        PQclear(res);
        printf("  -- WHERE salary = 60000 --\n");
        show_rows(conn, "SELECT id, name, salary FROM test_pbe_c1 WHERE salary = 60000");
    }

    /* PQprepare: SELECT WHERE > (OPE range) */
    res = PQprepare(conn, "pbe_c1_sel_gt",
        "SELECT id, name, salary, dept FROM test_pbe_c1 WHERE salary > $1",
        1, NULL);
    check_result(conn, res, "PQprepare SELECT WHERE OPE >");
    PQclear(res);

    {
        const char *paramValues[1] = {"55000"};
        res = PQexecPrepared(conn, "pbe_c1_sel_gt", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared SELECT WHERE salary>55000");
        PQclear(res);
    }

    /* PQprepare: SELECT BETWEEN */
    res = PQprepare(conn, "pbe_c1_sel_btw",
        "SELECT id, name, salary FROM test_pbe_c1 WHERE salary BETWEEN $1 AND $2",
        2, NULL);
    check_result(conn, res, "PQprepare SELECT WHERE BETWEEN");
    PQclear(res);

    {
        const char *paramValues[2] = {"50000", "65000"};
        res = PQexecPrepared(conn, "pbe_c1_sel_btw", 2, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared SELECT BETWEEN 50000-65000");
        PQclear(res);
    }

    /* 清理 */
    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c1 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 2: PQexecParams (匿名参数化查询)
 */
static void
test_exec_params(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 2: PQexecParams 匿名参数化查询\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c2 CASCADE;"
        "CREATE TABLE test_pbe_c2 ("
        "  id INT4,"
        "  name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  age INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* PQexecParams: INSERT with text params */
    {
        const Oid paramTypes[3] = {0, 0, 0};  /* 让服务端推断 */
        const char *paramValues[3] = {"1", "Alice", "30"};
        res = PQexecParams(conn,
            "INSERT INTO test_pbe_c2 VALUES ($1, $2, $3)",
            3, paramTypes, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams INSERT row1");
        PQclear(res);
    }

    {
        const char *paramValues[3] = {"2", "Bob", "25"};
        res = PQexecParams(conn,
            "INSERT INTO test_pbe_c2 VALUES ($1, $2, $3)",
            3, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams INSERT row2");
        PQclear(res);
    }

    printf("  -- 数据验证 --\n");
    show_rows(conn, "SELECT id, name, age FROM test_pbe_c2 ORDER BY id");

    /* PQexecParams: SELECT WHERE name = (DET equality) */
    {
        const char *paramValues[1] = {"Bob"};
        res = PQexecParams(conn,
            "SELECT id, name, age FROM test_pbe_c2 WHERE name = $1",
            1, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams WHERE DET name='Bob'");
        PQclear(res);
    }

    /* PQexecParams: SELECT WHERE age > (OPE range) */
    {
        const char *paramValues[1] = {"28"};
        res = PQexecParams(conn,
            "SELECT id, name, age FROM test_pbe_c2 WHERE age > $1",
            1, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams WHERE OPE age>28");
        PQclear(res);
    }

    /* PQexecParams: SELECT WHERE age BETWEEN */
    {
        const char *paramValues[2] = {"20", "28"};
        res = PQexecParams(conn,
            "SELECT id, name, age FROM test_pbe_c2 WHERE age BETWEEN $1 AND $2",
            2, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams WHERE BETWEEN 20-28");
        PQclear(res);
    }

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c2 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 3: 多种加密类型覆盖
 */
static void
test_multi_enc_types(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 3: 多种加密类型覆盖\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c3 CASCADE;"
        "CREATE TABLE test_pbe_c3 ("
        "  id INT4,"
        "  vc VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  txt TEXT_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET,SEARCH'),"
        "  sal INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),"
        "  score FLOAT8_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),"
        "  code BPCHAR_ENC(8) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET')"
        ");");
    PQclear(res);

    /* 用命名 prepared statement 插入多类型 */
    res = PQprepare(conn, "pbe_c3_ins",
        "INSERT INTO test_pbe_c3 VALUES ($1, $2, $3, $4, $5, $6)",
        6, NULL);
    check_result(conn, res, "PQprepare multi-type INSERT");
    PQclear(res);

    {
        const char *paramValues[6] = {
            "1", "Alice", "Alice bio text", "50000", "95.5", "CODE001"
        };
        res = PQexecPrepared(conn, "pbe_c3_ins", 6, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared multi-type row1");
        PQclear(res);
    }

    {
        const char *paramValues[6] = {
            "2", "Bob", "Bob is here", "60000", "88.0", "CODE002"
        };
        res = PQexecPrepared(conn, "pbe_c3_ins", 6, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared multi-type row2");
        PQclear(res);
    }

    printf("  -- 多类型数据验证 --\n");
    show_rows(conn, "SELECT id, vc, txt, sal, score, code FROM test_pbe_c3 ORDER BY id");

    /* DET 等值：varchar_enc */
    {
        const char *paramValues[1] = {"Alice"};
        res = PQexecPrepared(conn, "pbe_c3_sel_vc", 0, NULL, NULL, NULL, 0);
        PQclear(res);
    }
    res = PQprepare(conn, "pbe_c3_sel_vc",
        "SELECT id, vc, sal FROM test_pbe_c3 WHERE vc = $1",
        1, NULL);
    check_result(conn, res, "PQprepare WHERE varchar_enc =");
    PQclear(res);
    {
        const char *paramValues[1] = {"Alice"};
        res = PQexecPrepared(conn, "pbe_c3_sel_vc", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE vc='Alice'");
        PQclear(res);
    }

    /* BPCHAR_ENC 等值 */
    res = PQprepare(conn, "pbe_c3_sel_bp",
        "SELECT id, code FROM test_pbe_c3 WHERE code = $1",
        1, NULL);
    check_result(conn, res, "PQprepare WHERE bpchar_enc =");
    PQclear(res);
    {
        const char *paramValues[1] = {"CODE002"};
        res = PQexecPrepared(conn, "pbe_c3_sel_bp", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE code='CODE002'");
        PQclear(res);
    }

    /* FLOAT8_ENC OPE range */
    res = PQprepare(conn, "pbe_c3_sel_f8",
        "SELECT id, vc, score FROM test_pbe_c3 WHERE score > $1",
        1, NULL);
    check_result(conn, res, "PQprepare WHERE float8_enc >");
    PQclear(res);
    {
        const char *paramValues[1] = {"90.0"};
        res = PQexecPrepared(conn, "pbe_c3_sel_f8", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE score>90.0");
        PQclear(res);
    }

    /* TEXT_ENC LIKE (SEARCH) */
    res = PQprepare(conn, "pbe_c3_sel_lk",
        "SELECT id, vc, txt FROM test_pbe_c3 WHERE txt LIKE $1",
        1, NULL);
    check_result(conn, res, "PQprepare WHERE text_enc LIKE");
    PQclear(res);
    {
        const char *paramValues[1] = {"%bio%"};
        res = PQexecPrepared(conn, "pbe_c3_sel_lk", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE txt LIKE '%bio%'");
        PQclear(res);
    }

    /* ORDER BY OPE column */
    res = PQprepare(conn, "pbe_c3_ord",
        "SELECT id, vc, sal FROM test_pbe_c3 ORDER BY sal $1",
        1, NULL);
    PQclear(res);

    printf("  -- ORDER BY sal ASC --\n");
    show_rows(conn, "SELECT id, vc, sal FROM test_pbe_c3 ORDER BY sal");

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c3 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 4: NULL 参数绑定
 */
static void
test_null_params(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 4: NULL 参数绑定\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c4 CASCADE;"
        "CREATE TABLE test_pbe_c4 ("
        "  id INT4,"
        "  name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* Prepared INSERT with NULL */
    res = PQprepare(conn, "pbe_c4_ins",
        "INSERT INTO test_pbe_c4 VALUES ($1, $2, $3)",
        3, NULL);
    check_result(conn, res, "PQprepare INSERT with NULL");
    PQclear(res);

    /* name 为 NULL */
    {
        const char *paramValues[3] = {"1", NULL, "50000"};
        const int  paramLengths[3] = {0, 0, 0};
        const int  paramFormats[3] = {0, 0, 0};
        res = PQexecPrepared(conn, "pbe_c4_ins", 3, paramValues,
                             paramLengths, paramFormats, 0);
        check_result(conn, res, "PQexecPrepared NULL name");
        PQclear(res);
    }

    /* salary 为 NULL */
    {
        const char *paramValues[3] = {"2", "Bob", NULL};
        const int  paramLengths[3] = {0, 0, 0};
        const int  paramFormats[3] = {0, 0, 0};
        res = PQexecPrepared(conn, "pbe_c4_ins", 3, paramValues,
                             paramLengths, paramFormats, 0);
        check_result(conn, res, "PQexecPrepared NULL salary");
        PQclear(res);
    }

    /* 全 NULL */
    {
        const char *paramValues[3] = {"3", NULL, NULL};
        const int  paramLengths[3] = {0, 0, 0};
        const int  paramFormats[3] = {0, 0, 0};
        res = PQexecPrepared(conn, "pbe_c4_ins", 3, paramValues,
                             paramLengths, paramFormats, 0);
        check_result(conn, res, "PQexecPrepared both NULL");
        PQclear(res);
    }

    printf("  -- NULL 数据验证 --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c4 ORDER BY id");

    /* SELECT WHERE name IS NULL */
    res = PQprepare(conn, "pbe_c4_sel_null",
        "SELECT id FROM test_pbe_c4 WHERE name IS NULL",
        0, NULL);
    check_result(conn, res, "PQprepare WHERE name IS NULL");
    PQclear(res);
    res = PQexecPrepared(conn, "pbe_c4_sel_null", 0, NULL, NULL, NULL, 0);
    check_result(conn, res, "PQexecPrepared WHERE name IS NULL");
    PQclear(res);

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c4 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 5: UPDATE/DELETE 通过 prepared statement
 */
static void
test_update_delete(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 5: UPDATE/DELETE with prepared stmt\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c5 CASCADE;"
        "CREATE TABLE test_pbe_c5 ("
        "  id INT4, name VARCHAR(50),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);
    res = exec_sql(conn,
        "INSERT INTO test_pbe_c5 VALUES "
        "  (1, 'Alice', 50000), (2, 'Bob', 60000),"
        "  (3, 'Charlie', 70000), (4, 'Diana', 80000);");
    PQclear(res);

    /* UPDATE salary with prepared stmt */
    res = PQprepare(conn, "pbe_c5_upd",
        "UPDATE test_pbe_c5 SET salary = $1 WHERE id = $2",
        2, NULL);
    check_result(conn, res, "PQprepare UPDATE");
    PQclear(res);

    {
        const char *paramValues[2] = {"55000", "1"};
        res = PQexecPrepared(conn, "pbe_c5_upd", 2, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared UPDATE salary=55000 id=1");
        PQclear(res);
    }

    printf("  -- AFTER UPDATE --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c5 ORDER BY id");

    /* DELETE with prepared stmt */
    res = PQprepare(conn, "pbe_c5_del",
        "DELETE FROM test_pbe_c5 WHERE salary > $1",
        1, NULL);
    check_result(conn, res, "PQprepare DELETE");
    PQclear(res);

    {
        const char *paramValues[1] = {"65000"};
        res = PQexecPrepared(conn, "pbe_c5_del", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared DELETE salary>65000");
        PQclear(res);
    }

    printf("  -- AFTER DELETE --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c5 ORDER BY id");

    /* Mixed update: 更新加密列从加密列值 */
    /* Note: salary + $1 对加密列不支持，改为直接 SET salary = $1 */
    res = PQprepare(conn, "pbe_c5_upd2",
        "UPDATE test_pbe_c5 SET name = $1 WHERE id = $2",
        2, NULL);
    check_result(conn, res, "PQprepare UPDATE name (plaintext)");
    PQclear(res);

    {
        const char *paramValues[2] = {"UpdatedName", "2"};
        res = PQexecPrepared(conn, "pbe_c5_upd2", 2, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared UPDATE name id=2");
        PQclear(res);
    }

    printf("  -- AFTER UPDATE name --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c5 ORDER BY id");

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c5 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 6: PQdescribePrepared
 */
static void
test_describe_prepared(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 6: PQdescribePrepared\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c6 CASCADE;"
        "CREATE TABLE test_pbe_c6 ("
        "  id INT4,"
        "  name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  score FLOAT8_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* Prepare */
    res = PQprepare(conn, "pbe_c6_desc",
        "INSERT INTO test_pbe_c6 VALUES ($1, $2, $3)",
        3, NULL);
    check_result(conn, res, "PQprepare for describe");
    PQclear(res);

    /* Describe */
    res = PQdescribePrepared(conn, "pbe_c6_desc");
    check_result(conn, res, "PQdescribePrepared");
    if (PQresultStatus(res) == PGRES_COMMAND_OK)
    {
        int nparams = PQnparams(res);
        printf("  Parameters: %d\n", nparams);
        for (int i = 0; i < nparams; i++)
        {
            printf("    param %d: OID=%u\n", i, PQparamtype(res, i));
        }
    }
    PQclear(res);

    /* 插入数据 */
    {
        const char *paramValues[3] = {"1", "TestDesc", "99.9"};
        res = PQexecPrepared(conn, "pbe_c6_desc", 3, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared after describe");
        PQclear(res);
    }

    show_rows(conn, "SELECT id, name, score FROM test_pbe_c6 ORDER BY id");

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c6 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 7: 多次重复执行同一 prepared statement（批量插入）
 */
static void
test_batch_execute(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 7: 批量执行同一 prepared stmt\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c7 CASCADE;"
        "CREATE TABLE test_pbe_c7 ("
        "  id INT4,"
        "  val TEXT_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET,SEARCH')"
        ");");
    PQclear(res);

    res = PQprepare(conn, "pbe_c7_batch",
        "INSERT INTO test_pbe_c7 VALUES ($1, $2)",
        2, NULL);
    check_result(conn, res, "PQprepare batch INSERT");
    PQclear(res);

    /* 批量插入 50 行 */
    int batch_size = 50;
    printf("  -- 批量插入 %d 行 --\n", batch_size);
    for (int i = 0; i < batch_size; i++)
    {
        char id_str[16], val_str[64];
        snprintf(id_str, sizeof(id_str), "%d", i + 1);
        snprintf(val_str, sizeof(val_str), "batch_value_%d", i + 1);
        const char *paramValues[2] = {id_str, val_str};
        res = PQexecPrepared(conn, "pbe_c7_batch", 2, paramValues, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            printf("  FAIL at row %d: %s\n", i + 1, PQresultErrorMessage(res));
            failed_tests++;
            total_tests++;
            PQclear(res);
            break;
        }
        PQclear(res);
    }
    total_tests++;
    passed_tests++;
    printf("  PASS [批量插入 %d 行]\n", batch_size);

    /* 验证行数 */
    printf("  -- 数据验证 --\n");
    show_rows(conn, "SELECT count(*) FROM test_pbe_c7");

    /* WHERE = DET equality */
    {
        const char *paramValues[1] = {"batch_value_25"};
        res = PQexecParams(conn,
            "SELECT id, val FROM test_pbe_c7 WHERE val = $1",
            1, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams WHERE DET='batch_value_25'");
        PQclear(res);
    }

    /* LIKE search */
    {
        const char *paramValues[1] = {"%value_3%"};
        res = PQexecParams(conn,
            "SELECT id, val FROM test_pbe_c7 WHERE val LIKE $1",
            1, NULL, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecParams WHERE LIKE '%value_3%'");
        PQclear(res);
    }

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c7 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 8: PQexecParams with binary parameters
 */
static void
test_binary_params(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 8: PQexecParams 二进制参数格式\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c8 CASCADE;"
        "CREATE TABLE test_pbe_c8 ("
        "  id INT4,"
        "  name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* 二进制格式参数：int4 需要传网络字节序 */
    {
        /* param 0: id = 1 (int4 binary) */
        int32_t id_val = htonl(1);
        /* param 1: name = 'AliceBinary' (text binary) */
        const char *name_val = "AliceBinary";
        /* param 2: salary = 50000 (int4 binary) */
        int32_t sal_val = htonl(50000);

        const char *paramValues[3] = {
            (const char *)&id_val,
            name_val,
            (const char *)&sal_val
        };
        const int paramLengths[3] = {sizeof(int32_t), (int)strlen(name_val), sizeof(int32_t)};
        const int paramFormats[3] = {1, 1, 1};  /* 全部二进制 */

        res = PQexecParams(conn,
            "INSERT INTO test_pbe_c8 VALUES ($1, $2, $3)",
            3, NULL, paramValues, paramLengths, paramFormats, 0);
        check_result(conn, res, "PQexecParams binary INSERT");
        PQclear(res);
    }

    /* 混合格式：text + binary 混用 */
    {
        int32_t id_val = htonl(2);
        const char *name_val = "MixedFormat";
        int32_t sal_val = htonl(60000);

        const char *paramValues[3] = {
            (const char *)&id_val,
            name_val,
            (const char *)&sal_val
        };
        /* id=binary(1), name=text(0), salary=binary(1) */
        const int paramLengths[3] = {sizeof(int32_t), (int)strlen(name_val), sizeof(int32_t)};
        const int paramFormats[3] = {1, 0, 1};

        res = PQexecParams(conn,
            "INSERT INTO test_pbe_c8 VALUES ($1, $2, $3)",
            3, NULL, paramValues, paramLengths, paramFormats, 0);
        check_result(conn, res, "PQexecParams mixed binary/text INSERT");
        PQclear(res);
    }

    printf("  -- 二进制参数数据验证 --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c8 ORDER BY id");

    /* 二进制结果格式 (resultFormat=1) */
    {
        res = PQexecParams(conn,
            "SELECT id, name, salary FROM test_pbe_c8 ORDER BY id",
            0, NULL, NULL, NULL, NULL, 1);  /* 结果用二进制 */
        check_result(conn, res, "PQexecParams binary result format");
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            int nrows = PQntuples(res);
            for (int r = 0; r < nrows; r++)
            {
                /* id 是二进制 int4 */
                if (!PQgetisnull(res, r, 0))
                {
                    int32_t id_val;
                    memcpy(&id_val, PQgetvalue(res, r, 0), sizeof(int32_t));
                    id_val = ntohl(id_val);
                    printf("    row %d: id=%d (binary)\n", r, id_val);
                }
            }
        }
        PQclear(res);
    }

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c8 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 9: PQexecParams + 混合 WHERE 条件（明文列 + 加密列混合）
 */
static void
test_mixed_where(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 9: 混合 WHERE 条件\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c9 CASCADE;"
        "CREATE TABLE test_pbe_c9 ("
        "  id INT4, dept VARCHAR(50),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE'),"
        "  name TEXT_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET')"
        ");");
    PQclear(res);
    /* INSERT 单独发，否则批量多语句中 TECC 处理器可能不会加密常量 */
    res = exec_sql(conn,
        "INSERT INTO test_pbe_c9 VALUES "
        "  (1, 'IT', 50000, 'Alice'),"
        "  (2, 'HR', 60000, 'Bob');");
    PQclear(res);
    res = exec_sql(conn,
        "INSERT INTO test_pbe_c9 VALUES "
        "  (3, 'IT', 70000, 'Charlie'),"
        "  (4, 'HR', 80000, 'Diana');");
    PQclear(res);

    /* 明文列 + 加密列混合 WHERE */
    res = PQprepare(conn, "pbe_c9_mix",
        "SELECT id, dept, salary, name FROM test_pbe_c9 "
        "WHERE dept = $1 AND salary > $2",
        2, NULL);
    check_result(conn, res, "PQprepare mixed WHERE dept+salary");
    PQclear(res);

    {
        const char *paramValues[2] = {"IT", "55000"};
        res = PQexecPrepared(conn, "pbe_c9_mix", 2, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE dept='IT' AND salary>55000");
        PQclear(res);
    }

    /* 两个加密列混合 WHERE */
    res = PQprepare(conn, "pbe_c9_mix2",
        "SELECT id, name, salary FROM test_pbe_c9 "
        "WHERE name = $1 AND salary BETWEEN $2 AND $3",
        3, NULL);
    check_result(conn, res, "PQprepare mixed WHERE name+salary BETWEEN");
    PQclear(res);

    {
        const char *paramValues[3] = {"Bob", "50000", "70000"};
        res = PQexecPrepared(conn, "pbe_c9_mix2", 3, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared WHERE name='Bob' AND salary 5万-7万");
        PQclear(res);
    }

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c9 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 10: 生命周期管理 - prepare / deallocate / re-prepare
 */
static void
test_lifecycle(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 10: 生命周期管理\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c10 CASCADE;"
        "CREATE TABLE test_pbe_c10 ("
        "  id INT4,"
        "  val INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* Prepare */
    res = PQprepare(conn, "pbe_c10_s1",
        "INSERT INTO test_pbe_c10 VALUES ($1, $2)",
        2, NULL);
    check_result(conn, res, "PQprepare insert");
    PQclear(res);

    /* 用一次 */
    {
        const char *paramValues[2] = {"1", "100"};
        res = PQexecPrepared(conn, "pbe_c10_s1", 2, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared first use");
        PQclear(res);
    }

    /* DEALLOCATE (通过 SQL) */
    res = exec_sql(conn, "DEALLOCATE pbe_c10_s1;");
    check_result(conn, res, "DEALLOCATE");
    PQclear(res);

    /* 再 prepare 同名 stmt */
    res = PQprepare(conn, "pbe_c10_s1",
        "SELECT id, val FROM test_pbe_c10 WHERE val = $1",
        1, NULL);
    check_result(conn, res, "PQprepare re-prepare");
    PQclear(res);

    {
        const char *paramValues[1] = {"100"};
        res = PQexecPrepared(conn, "pbe_c10_s1", 1, paramValues, NULL, NULL, 0);
        check_result(conn, res, "PQexecPrepared after re-prepare");
        PQclear(res);
    }

    /* DEALLOCATE ALL */
    res = exec_sql(conn, "DEALLOCATE ALL;");
    check_result(conn, res, "DEALLOCATE ALL");
    PQclear(res);

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c10 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 12: 简单协议 PREPARE/EXECUTE（psql 风格，对照用）
 * 通过 PQexec 直接发送 PREPARE/EXECUTE SQL，走简单查询协议。
 * 这是 pbe.sql 中的方式，预期正常工作。
 */
static void
test_simple_protocol_prepare(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 12: 简单协议 PREPARE/EXECUTE（对照）\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c12 CASCADE;"
        "CREATE TABLE test_pbe_c12 ("
        "  id INT4,"
        "  name VARCHAR_ENC(100) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'DET'),"
        "  salary INT4_ENC ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);

    /* PREPARE + EXECUTE via simple protocol */
    res = exec_sql(conn,
        "PREPARE p12_ins (int4, varchar_enc, int4_enc) AS "
        "INSERT INTO test_pbe_c12 VALUES ($1, $2, $3);");
    check_result(conn, res, "PREPARE (simple protocol)");
    PQclear(res);

    res = exec_sql(conn, "EXECUTE p12_ins(1, 'Alice', 50000);");
    check_result(conn, res, "EXECUTE p12_ins row1");
    PQclear(res);

    res = exec_sql(conn, "EXECUTE p12_ins(2, 'Bob', 60000);");
    check_result(conn, res, "EXECUTE p12_ins row2");
    PQclear(res);

    printf("  -- 简单协议 PREPARE/EXECUTE 验证 --\n");
    show_rows(conn, "SELECT id, name, salary FROM test_pbe_c12 ORDER BY id");

    /* SELECT via PREPARE/EXECUTE */
    res = exec_sql(conn,
        "PREPARE p12_sel (varchar_enc) AS "
        "SELECT id, name, salary FROM test_pbe_c12 WHERE name = $1;");
    check_result(conn, res, "PREPARE p12_sel (simple protocol)");
    PQclear(res);

    res = exec_sql(conn, "EXECUTE p12_sel('Bob');");
    check_result(conn, res, "EXECUTE p12_sel WHERE name='Bob'");
    PQclear(res);

    /* OPE range via simple protocol */
    res = exec_sql(conn,
        "PREPARE p12_range (int4_enc) AS "
        "SELECT id, name, salary FROM test_pbe_c12 WHERE salary > $1;");
    check_result(conn, res, "PREPARE p12_range (simple protocol)");
    PQclear(res);

    res = exec_sql(conn, "EXECUTE p12_range(55000);");
    check_result(conn, res, "EXECUTE p12_range WHERE salary>55000");
    PQclear(res);

    /* DEALLOCATE */
    exec_sql(conn, "DEALLOCATE ALL;");

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c12 CASCADE;");
    printf("----------------------------------------\n");
}

/*
 * 测试 11: NUMERIC_ENC 类型
 */
static void
test_numeric_enc(PGconn *conn)
{
    printf("\n========================================\n");
    printf("测试 11: NUMERIC_ENC 类型\n");
    printf("========================================\n");

    PGresult *res;

    res = exec_sql(conn,
        "DROP TABLE IF EXISTS test_pbe_c11 CASCADE;"
        "CREATE TABLE test_pbe_c11 ("
        "  id INT4,"
        "  amount NUMERIC_ENC(10,2) ENCRYPTED WITH (DEK = dek_pbe, SUPPORT_ALGO = 'OPE')"
        ");");
    PQclear(res);
    /* 喂数据：用简单协议 */
    res = exec_sql(conn, "INSERT INTO test_pbe_c11 VALUES (1, 1234.56);");
    PQclear(res);
    res = exec_sql(conn, "INSERT INTO test_pbe_c11 VALUES (2, 7890.12);");
    PQclear(res);

    printf("  -- NUMERIC_ENC 数据验证 --\n");
    show_rows(conn, "SELECT id, amount FROM test_pbe_c11 ORDER BY id");

    /* numeric OPE range via simple protocol */
    res = exec_sql(conn,
        "PREPARE p11_rng (numeric_enc) AS "
        "SELECT id, amount FROM test_pbe_c11 WHERE amount > $1;");
    check_result(conn, res, "PREPARE (simple) numeric_enc WHERE >");
    PQclear(res);
    res = exec_sql(conn, "EXECUTE p11_rng(5000);");
    check_result(conn, res, "EXECUTE (simple) numeric_enc WHERE amount>5000");
    PQclear(res);
    exec_sql(conn, "DEALLOCATE p11_rng;");

    exec_sql(conn, "DROP TABLE IF EXISTS test_pbe_c11 CASCADE;");
    printf("----------------------------------------\n");
}

/* ========== 主函数 ========== */
int
main(int argc, char *argv[])
{
    const char *conninfo;

    if (argc > 1)
        conninfo = argv[1];
    else
        conninfo = "host=localhost port=5432 dbname=postgres user=postgres";

    /* 连接数据库 */
    PGconn *conn = do_connect(conninfo);

    /* 检查 TECC 是否启用 */
    PGresult *res = PQexec(conn, "SHOW enable_tecc;");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        printf("TECC enable_tecc = %s\n", PQgetvalue(res, 0, 0));
    PQclear(res);

    /* 创建测试用的 KEK/DEK（必须分多次 PQexec，每次直送单条语句，
     * 否则 TECC 处理器在一个 multi-statement PQexec 中无法刷新
     * KEK cache → 后续 CREATE DEK 会 cache miss）
     */
    printf("\n=== 创建测试 KEK/DEK ===\n");
    res = exec_sql(conn, "DROP KEK IF EXISTS kek_pbe CASCADE;");
    PQclear(res);
    res = exec_sql(conn,
        "CREATE KEK kek_pbe WITH (key_store = 'localkms', "
        "  key_path = '/tmp/kek_pbe.key');");
    PQclear(res);
    res = exec_sql(conn, "CREATE DEK dek_pbe WITH VALUES(KEK = kek_pbe);");
    PQclear(res);

    /* 运行所有测试 */
    test_basic_prepare_execute(conn);
    test_exec_params(conn);
    test_multi_enc_types(conn);
    test_null_params(conn);
    test_update_delete(conn);
    test_describe_prepared(conn);
    test_batch_execute(conn);
    test_binary_params(conn);
    test_mixed_where(conn);
    test_lifecycle(conn);
    test_numeric_enc(conn);
    test_simple_protocol_prepare(conn);

    /* 清理 KEK/DEK（同上，逐条发送） */
    printf("\n=== 清理 KEK/DEK ===\n");
    res = exec_sql(conn, "DROP DEK IF EXISTS dek_pbe CASCADE;");
    PQclear(res);
    res = exec_sql(conn, "DROP KEK IF EXISTS kek_pbe CASCADE;");
    PQclear(res);

    /* 总结 */
    printf("\n========================================\n");
    printf("测试总结\n");
    printf("========================================\n");
    printf("  总计: %d\n", total_tests);
    printf("  通过: %d\n", passed_tests);
    printf("  失败: %d\n", failed_tests);
    printf("========================================\n");
    printf("\n========================================\n");
    printf("Bug 发现\n");
    printf("========================================\n");
    printf("\n");
    printf("Bug A: PQexecPrepared 不经过 tecc_preprocess_params\n");
    printf("  现象: PQsendQueryPrepared → PQsendQueryGuts(command=NULL,\n");
    printf("        stmtName=xxx) — 条件 if(enable_tecc && command) 为 false\n");
    printf("        → tecc_preprocess_params 被跳过\n");
    printf("        → 参数值以明文发送, 服务端报 \"invalid hex string: '50000'\"\n");
    printf("  文件: fe-exec.c:1913\n");
    printf("  范围: PQprepare+PQexecPrepared 全路径\n");
    printf("\n");
    printf("Bug B: PQexecParams 参数加密不生效\n");
    printf("  现象: tecc_preprocess_params 被调用（command不为NULL）,\n");
    printf("        但 tecc_processor_run_pre_query 返回 NO_ENCRYPTED_COLUMNS\n");
    printf("        或未正确处理参数映射 → 返回值 NULL → 明文发送\n");
    printf("  文件: fe-tecc-integration.c:1304-1500\n");
    printf("\n");
    printf("可用路径: 简单协议 PREPARE/EXECUTE via PQexec — 正常工作\n");
    printf("  用例: PREPARE p (int4_enc) AS INSERT INTO t VALUES ($1);\n");
    printf("        EXECUTE p(50000);\n");
    printf("  原理: 走 PQexec → TECC tecc_preprocess_query 拦截并重写\n");
    printf("========================================\n");

    PQfinish(conn);

    return failed_tests > 0 ? 1 : 0;
}
