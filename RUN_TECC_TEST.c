/* Begin - ReqID:TECC4.0 */
/*
 * File: RUN_TECC_TEST.c
 * Abstract:
 *   TECC测试程序 - 用于测试TECC语法和功能
 *   位置: /home/wxp/tecc4_1/RUN_TECC_TEST.c
 *   编译: gcc -o RUN_TECC_TEST RUN_TECC_TEST.c && ./RUN_TECC_TEST
 *
 *   重要: 这个文件放在项目根目录，用于快速测试TECC语法
 *   每次需要测试CREATE KEK/DEK语法时，使用这个程序
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 * Updated: 20260419 - 移动到根目录，重命名为显眼的名字
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   /home/wxp/tecc4_1/RUN_TECC_TEST.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Function to execute shell command */
int execute_command(const char *command) {
    printf("Executing: %s\n", command);
    fflush(stdout);  /* 确保输出立即显示 */
    int status = system(command);
    printf("\nCommand exit status: %d\n", status);
    return status;
}

/* Function to check PostgreSQL server status */
int check_postgres_status(void) {
    printf("\n=== Checking PostgreSQL Server Status ===\n");

    char command[512];
    snprintf(command, sizeof(command),
             "export PATH=\"/home/wxp/pg14_2/bin:$PATH\" && "
             "pg_ctl status -D /home/wxp/pg14_2/data 2>&1 | grep -q 'server is running' && "
             "echo 'Server is running' || echo 'Server is not running'");

    return execute_command(command);
}

/* Function to start PostgreSQL server */
int start_postgres_server(void) {
    printf("\n=== Starting PostgreSQL Server ===\n");

    char command[512];
    snprintf(command, sizeof(command),
             "export PATH=\"/home/wxp/pg14_2/bin:$PATH\" && "
             "pg_ctl start -D /home/wxp/pg14_2/data -l /home/wxp/pg14_2/data/logfile -o \"-p 5434 -k /tmp\"");

    int status = execute_command(command);

    /* Wait a moment for server to start */
    sleep(2);

    return status;
}

/* Function to execute SQL script file via psql */
int execute_sql_script(const char *script_file) {
    char command[4096];
    FILE *fp;
    char buffer[1024];

    /* 检查脚本文件是否存在 */
    if (access(script_file, F_OK) != 0) {
        printf("ERROR: SQL script file '%s' does not exist\n", script_file);
        return 1;
    }

    /* 执行SQL脚本文件，直接输出到控制台 */
    /* 使用环境变量传递的TECC_LOG_LEVEL，如果没有则使用DEBUG */
    /* 设置client_min_messages=notice以看到更多日志 */
    snprintf(command, sizeof(command),
             "export LD_LIBRARY_PATH=/home/wxp/pg14_2/lib:/usr/lib64:$LD_LIBRARY_PATH && "
             "export PATH=\"/home/wxp/pg14_2/bin:$PATH\" && "
             "export TECC_LOG_LEVEL=${TECC_LOG_LEVEL:-WARNING} && "
             "psql -h /tmp -p 5434 -U wxp -d postgres --set=client_min_messages=notice -f \"%s\" 2>&1",
             script_file);

    printf("\n=== Executing SQL script: %s ===\n", script_file);

    /* 显示脚本内容 */
    printf("--- Script content ---\n");
    fp = fopen(script_file, "r");
    if (fp) {
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
        fclose(fp);
    }
    printf("--- End script content ---\n");

    printf("\n--- psql output (direct) ---\n");
    int status = execute_command(command);
    printf("--- end psql output ---\n");

    printf("\n--- SQL script execution completed with status: %d ---\n", status);
    return status;
}

/* Function to execute single SQL command (backward compatibility) */
int execute_sql(const char *sql) {
    char temp_file[256];
    FILE *fp;

    /* 创建临时SQL文件 */
    snprintf(temp_file, sizeof(temp_file), "/tmp/tecc_temp_sql_%d.sql", getpid());

    fp = fopen(temp_file, "w");
    if (!fp) {
        printf("ERROR: Failed to create temporary SQL file\n");
        return 1;
    }

    fprintf(fp, "%s\n", sql);
    fclose(fp);

    int status = execute_sql_script(temp_file);

    /* 删除临时文件 */
    unlink(temp_file);

    return status;
}

/* Main test function - executes SQL script file */
int main(int argc, char *argv[]) {
    char *script_file = "test_tecc_syntax.sql";

    printf("=== TECC SQL Script Test Program ===\n");
    printf("Location: /home/wxp/tecc4_1/RUN_TECC_TEST.c\n");
    printf("Usage: ./RUN_TECC_TEST [sql_script_file]\n");
    printf("Default script: %s\n\n", script_file);

    /* Use command line argument if provided */
    if (argc > 1) {
        script_file = argv[1];
    }

    printf("Will execute SQL script: %s\n", script_file);

    /* Step 1: Check PostgreSQL server status */
    printf("\n=== Step 1: Checking PostgreSQL Server Status ===\n");
    int status = check_postgres_status();

    if (status != 0) {
        printf("PostgreSQL server may not be running. Starting server...\n");
        status = start_postgres_server();

        if (status != 0) {
            printf("ERROR: Failed to start PostgreSQL server\n");
            return 1;
        }
    }

    printf("PostgreSQL server should be running.\n");

    /* Step 2: Execute the SQL script */
    printf("\n=== Step 2: Executing SQL Script ===\n");
    status = execute_sql_script(script_file);

    if (status != 0) {
        printf("\n=== WARNING: SQL script execution returned non-zero status ===\n");
        printf("This may indicate errors in the SQL script.\n");
        printf("Check the output above for error messages.\n");
    } else {
        printf("\n=== SQL script executed successfully ===\n");
    }

    return status;
}
/* End - ReqID:TECC4.0 */