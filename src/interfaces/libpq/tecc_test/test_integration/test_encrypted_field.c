/* Begin - ReqID:TECC4.0 */
/*
 * File: test_encrypted_field.c
 * Abstract:
 *   Test program for encrypted field functionality.
 *   Tests CREATE KEK/DEK, CREATE TABLE with encrypted column, and INSERT data.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/test_encrypted_field.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Function to execute shell command */
int execute_command(const char *command) {
    printf("Executing: %s\n", command);
    int status = system(command);
    printf("Command exit status: %d\n", status);
    return status;
}

/* Function to check PostgreSQL server status */
int check_postgres_status(void) {
    printf("\n=== Checking PostgreSQL Server Status ===\n");

    char command[512];
    snprintf(command, sizeof(command),
             "export PATH=\"/home/wxp/pg14/bin:$PATH\" && "
             "pg_ctl status -D /home/wxp/pg14/db0 2>&1 | grep -q 'server is running' && "
             "echo 'Server is running' || echo 'Server is not running'");

    return execute_command(command);
}

/* Function to start PostgreSQL server */
int start_postgres_server(void) {
    printf("\n=== Starting PostgreSQL Server ===\n");

    char command[512];
    snprintf(command, sizeof(command),
             "export PATH=\"/home/wxp/pg14/bin:$PATH\" && "
             "pg_ctl start -D /home/wxp/pg14/db0 -l /home/wxp/pg14/db0/logfile");

    int status = execute_command(command);

    /* Wait a moment for server to start */
    sleep(2);

    return status;
}

/* Function to execute SQL command via psql */
int execute_sql(const char *sql) {
    char command[4096];
    char output_file[256];
    FILE *fp;
    char buffer[1024];

    /* 创建临时文件保存输出 */
    snprintf(output_file, sizeof(output_file), "/tmp/tecc_test_output_%d.txt", getpid());

    /* 将SQL写入临时文件，避免引号冲突，并将输出重定向到文件 */
    snprintf(command, sizeof(command),
             "export LD_LIBRARY_PATH=/home/wxp/pg14/lib:/usr/lib64:$LD_LIBRARY_PATH && "
             "export PATH=\"/home/wxp/pg14/bin:$PATH\" && "
             "echo \"%s\" | psql -h localhost -p 5432 -U postgres 2>&1 > %s",
             sql, output_file);

    printf("\n--- Executing SQL: %s ---\n", sql);
    int status = execute_command(command);

    /* 读取并显示输出文件内容 */
    fp = fopen(output_file, "r");
    if (fp) {
        printf("--- psql output ---\n");
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
        }
        printf("--- end psql output ---\n");
        fclose(fp);
        unlink(output_file); /* 删除临时文件 */
    }

    printf("--- SQL execution completed with status: %d ---\n", status);
    return status;
}

/* Main test function */
int main(void) {
    printf("=== TECC Encrypted Field Test Program ===\n");
    printf("Testing: CREATE KEK/DEK, CREATE TABLE with encrypted column, INSERT data\n\n");

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

    /* Step 2: Test basic connection */
    printf("\n=== Step 2: Testing Basic Connection ===\n");
    printf("Testing basic SELECT to check if database is accessible...\n");
    printf("This will also show lexer debug output for SELECT 1;\n");
    status = execute_sql("SELECT 1;");
    if (status != 0) {
        printf("ERROR: Failed to connect to database\n");
        return 1;
    }

    /* Step 2.5: Check if TECC extension is loaded */
    printf("\n=== Step 2.5: Checking TECC Extension Status ===\n");
    printf("Checking if TECC system tables exist...\n");
    status = execute_sql("SELECT EXISTS (SELECT 1 FROM pg_class WHERE relname = 'hg_kek');");
    if (status != 0) {
        printf("WARNING: Failed to check TECC system tables\n");
    }

    printf("Checking if TECC extension can be loaded...\n");
    status = execute_sql("CREATE EXTENSION IF NOT EXISTS tecc;");
    if (status != 0) {
        printf("WARNING: Failed to create TECC extension\n");
        printf("Trying to check if extension exists...\n");
        status = execute_sql("SELECT extname FROM pg_extension WHERE extname = 'tecc';");
    }

    printf("Checking if TECC commands are recognized...\n");
    status = execute_sql("\\help CREATE KEK");
    if (status != 0) {
        printf("WARNING: CREATE KEK command may not be recognized\n");
    }

    /* Step 3: Clean up any existing test objects */
    printf("\n=== Step 3: Cleaning Up Existing Test Objects ===\n");
    printf("Attempting to drop existing test objects...\n");
    execute_sql("DROP TABLE IF EXISTS test_encrypted_table;");
    execute_sql("DROP DEK IF EXISTS test_dek;");
    execute_sql("DROP KEK IF EXISTS test_kek;");

    /* Step 4: Create KEK (Key Encryption Key) */
    printf("\n=== Step 4: Creating KEK ===\n");
    printf("This is the critical test - CREATE KEK should work if TECC is properly installed\n");
    printf("If this fails, TECC extension is not working correctly\n");
    /* 根据需求文档，正确的语法是: CREATE KEK kek_name WITH (KEY_STORE = kms_method, KEY_PATH = "key_path") */
    status = execute_sql("CREATE KEK test_kek WITH (KEY_STORE = 'localkms', KEY_PATH = '/tmp/test_kek.key');");
    if (status != 0) {
        printf("\n=== CRITICAL ERROR: Failed to create KEK ===\n");
        printf("This indicates TECC extension is not working properly.\n");
        printf("Possible issues:\n");
        printf("1. TECC extension not installed in PostgreSQL\n");
        printf("2. TECC libpq integration not working\n");
        printf("3. TECC processor state not initialized\n");
        printf("\nLet's test a simpler non-TECC command to verify basic functionality...\n");

        /* Test a simple non-TECC command */
        printf("\nTesting simple CREATE TABLE (non-TECC)...\n");
        status = execute_sql("CREATE TABLE test_simple_table (id INT, name VARCHAR(50));");
        if (status == 0) {
            printf("Simple CREATE TABLE works, so basic PostgreSQL is functioning.\n");
            printf("The problem is specifically with TECC commands.\n");
            execute_sql("DROP TABLE test_simple_table;");
        }

        return 1;
    }

    /* Step 5: Create DEK (Data Encryption Key) */
    printf("\n=== Step 5: Creating DEK ===\n");
    /* 根据需求文档，正确的语法是: CREATE DEK dek_name WITH VALUES(KEK = kek_name) */
    status = execute_sql("CREATE DEK test_dek WITH VALUES(KEK = 'test_kek');");
    if (status != 0) {
        printf("ERROR: Failed to create DEK\n");
        return 1;
    }

    /* Step 6: Create table with encrypted column */
    printf("\n=== Step 6: Creating Table with Encrypted Column ===\n");
    status = execute_sql("CREATE TABLE test_encrypted_table (id INT, name VARCHAR(50), secret_data INT4_ENC ENCRYPTED WITH (dek = 'test_dek', support_algo = 'det'));");
    if (status != 0) {
        printf("ERROR: Failed to create table\n");
        return 1;
    }

    /* Step 6.5: Check metadata tables */
    printf("\n=== Step 6.5: Checking Metadata Tables ===\n");
    printf("First, check if TECC system tables exist...\n");
    execute_sql("SELECT EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'hg_encrypted_columns') AS has_hg_encrypted_columns, EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'hg_dek') AS has_hg_dek, EXISTS (SELECT 1 FROM pg_tables WHERE tablename = 'hg_kek') AS has_hg_kek;");

    printf("\nChecking hg_encrypted_columns table structure...\n");
    execute_sql("SELECT column_name, data_type FROM information_schema.columns WHERE table_name = 'hg_encrypted_columns' ORDER BY ordinal_position;");

    printf("\nChecking if hg_encrypted_columns table has any data...\n");
    execute_sql("SELECT COUNT(*) AS total_rows FROM hg_encrypted_columns;");

    printf("\nChecking if hg_encrypted_columns table has data for test_encrypted_table...\n");
    execute_sql("SELECT table_name, column_name, data_type_original_oid, column_key_id FROM hg_encrypted_columns WHERE table_name = 'test_encrypted_table';");

    printf("\nChecking all data in hg_encrypted_columns...\n");
    execute_sql("SELECT table_name, column_name, data_type_original_oid, column_key_id FROM hg_encrypted_columns LIMIT 10;");

    printf("\nChecking hg_dek table structure...\n");
    execute_sql("SELECT column_name, data_type FROM information_schema.columns WHERE table_name = 'hg_dek' ORDER BY ordinal_position;");

    printf("\nChecking if hg_dek table has data...\n");
    execute_sql("SELECT COUNT(*) AS total_rows FROM hg_dek;");
    execute_sql("SELECT dek_name, key_owner, algorithm, key_length FROM hg_dek LIMIT 10;");

    printf("\nChecking hg_kek table structure...\n");
    execute_sql("SELECT column_name, data_type FROM information_schema.columns WHERE table_name = 'hg_kek' ORDER BY ordinal_position;");

    printf("\nChecking if hg_kek table has data...\n");
    execute_sql("SELECT COUNT(*) AS total_rows FROM hg_kek;");
    execute_sql("SELECT kek_name, key_owner, kms_type, key_length FROM hg_kek LIMIT 10;");

    /* Step 7: Insert data into table with encrypted column */
    printf("\n=== Step 7: Inserting Data with Encrypted Column ===\n");
    status = execute_sql("INSERT INTO test_encrypted_table (id, name, secret_data) VALUES (1, 'Test User', 12345);");
    if (status != 0) {
        printf("ERROR: Failed to insert data\n");
        return 1;
    }

    /* Step 8: Query the inserted data */
    printf("\n=== Step 8: Querying Inserted Data ===\n");
    execute_sql("SELECT * FROM test_encrypted_table;");

    /* Step 9: Test multiple INSERT statements */
    printf("\n=== Step 9: Testing Multiple INSERT Statements ===\n");
    status = execute_sql("INSERT INTO test_encrypted_table (id, name, secret_data) VALUES (2, 'User Two', 67890); INSERT INTO test_encrypted_table (id, name, secret_data) VALUES (3, 'User Three', 11111);");
    if (status != 0) {
        printf("ERROR: Failed to execute multiple INSERT statements\n");
        return 1;
    }

    /* Step 10: Verify all data */
    printf("\n=== Step 10: Verifying All Data ===\n");
    execute_sql("SELECT id, name, secret_data FROM test_encrypted_table ORDER BY id;");

    /* Step 11: Clean up */
    printf("\n=== Step 11: Cleaning Up Test Objects ===\n");
    execute_sql("DROP TABLE test_encrypted_table;");
    execute_sql("DROP DEK test_dek;");
    execute_sql("DROP KEK test_kek;");

    printf("\n=== Test Program Completed ===\n");
    return 0;
}
/* End - ReqID:TECC4.0 */