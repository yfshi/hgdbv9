/* Begin - ReqID:TECC4.0 */
/*
 * File: test_parser_ddl.c
 *
 * Abstract:
 *   DDL statement tests for TECC frontend parser.
 *   Tests CREATE TABLE, ALTER TABLE, DROP TABLE statements.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_parser_ddl.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1
#define FRONTEND_PARSER 1

/* Include parser headers */
#include "../../frontend_parser/fe_adapter.h"
#include "../../frontend_parser/tecc_nodes.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper function to print test result */
static void print_test_result(const char *test_name, bool passed)
{
    if (passed) {
        printf("  ✅ %s\n", test_name);
        tests_passed++;
    } else {
        printf("  ❌ %s\n", test_name);
        tests_failed++;
    }
}

/* Test 1: CREATE TABLE statement parsing */
static void test_create_table_parsing(void)
{
    printf("Test 1: CREATE TABLE Statement Parsing\n");

    const char *test_cases[] = {
        "CREATE TABLE users (id int, name text);",
        "CREATE TABLE users (id int PRIMARY KEY, name text_enc, salary int_enc);",
        "CREATE TABLE IF NOT EXISTS users (id int, name varchar(100));",
        "CREATE TEMP TABLE temp_users (id int, data text);",
        "CREATE TABLE users (id int, name text COLLATE \"C\");",
        "CREATE TABLE users (id int, name text_enc, CONSTRAINT pk_id PRIMARY KEY (id));",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        List *parse_tree = tecc_parser_parse(sql);

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Parse CREATE TABLE: %s", sql);

        if (parse_tree) {
            print_test_result(test_name, true);

            /* Verify parse tree structure */
            if (parse_tree->head && parse_tree->length > 0) {
                ListCell *cell = parse_tree->head;
                Node *node = (Node *)cell->data.ptr_value;

                if (node->type == FE_T_CreateStmt) {
                    CreateStmt *create_stmt = (CreateStmt *)node;
                    printf("    - CREATE TABLE: table=%s, columns=%d\n",
                           create_stmt->relation ? create_stmt->relation->relname : "NULL",
                           create_stmt->tableElts ? create_stmt->tableElts->length : 0);

                    /* Check for encrypted columns */
                    if (create_stmt->tableElts) {
                        ListCell *lc;
                        foreach(lc, create_stmt->tableElts) {
                            Node *elem = (Node *)lfirst(lc);
                            if (IsA(elem, ColumnDef)) {
                                ColumnDef *col_def = (ColumnDef *)elem;
                                printf("    - Column: %s\n", col_def->colname);
                            }
                        }
                    }
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
        }
    }
}

/* Test 2: ALTER TABLE statement parsing */
static void test_alter_table_parsing(void)
{
    printf("\nTest 2: ALTER TABLE Statement Parsing\n");

    const char *test_cases[] = {
        "ALTER TABLE users ADD COLUMN email text;",
        "ALTER TABLE users DROP COLUMN email;",
        "ALTER TABLE users ALTER COLUMN name TYPE varchar(100);",
        "ALTER TABLE users ALTER COLUMN name SET NOT NULL;",
        "ALTER TABLE users ALTER COLUMN name DROP NOT NULL;",
        "ALTER TABLE users ADD CONSTRAINT pk_id PRIMARY KEY (id);",
        "ALTER TABLE users DROP CONSTRAINT pk_id;",
        /* Important: COLLATE change test for encrypted columns */
        "ALTER TABLE users ALTER COLUMN name TYPE text_enc COLLATE \"C\";",
        "ALTER TABLE users ALTER COLUMN name SET DATA TYPE text_enc COLLATE \"en_US\";",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        List *parse_tree = tecc_parser_parse(sql);

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Parse ALTER TABLE: %s", sql);

        if (parse_tree) {
            print_test_result(test_name, true);

            /* Verify parse tree structure */
            if (parse_tree->head && parse_tree->length > 0) {
                ListCell *cell = parse_tree->head;
                Node *node = (Node *)cell->data.ptr_value;

                printf("    - Node type: %d\n", node->type);

                /* Note: ALTER TABLE parsing may not be fully implemented yet */
                if (node->type == FE_T_AlterTableStmt) {
                    printf("    - ALTER TABLE statement parsed successfully\n");
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result(test_name, false);
            printf("    - Note: ALTER TABLE parsing may not be fully implemented yet\n");
        }
    }
}

/* Test 3: DROP TABLE statement parsing */
static void test_drop_table_parsing(void)
{
    printf("\nTest 3: DROP TABLE Statement Parsing\n");

    const char *test_cases[] = {
        "DROP TABLE users;",
        "DROP TABLE users CASCADE;",
        "DROP TABLE users RESTRICT;",
        "DROP TABLE IF EXISTS users;",
        "DROP TABLE IF EXISTS users CASCADE;",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        List *parse_tree = tecc_parser_parse(sql);

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Parse DROP TABLE: %s", sql);

        if (parse_tree) {
            print_test_result(test_name, true);

            /* Verify parse tree structure */
            if (parse_tree->head && parse_tree->length > 0) {
                ListCell *cell = parse_tree->head;
                Node *node = (Node *)cell->data.ptr_value;

                if (node->type == FE_T_DropStmt) {
                    DropStmt *drop_stmt = (DropStmt *)node;
                    printf("    - DROP TABLE: removeType=%d, missing_ok=%d, behavior=%d\n",
                           drop_stmt->removeType, drop_stmt->missing_ok, drop_stmt->behavior);

                    /* Check object type */
                    if (drop_stmt->removeType == OBJECT_TABLE) {
                        printf("    - Object type: TABLE\n");
                    }
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
        }
    }
}

/* Test 4: Encrypted column type recognition */
static void test_encrypted_column_recognition(void)
{
    printf("\nTest 4: Encrypted Column Type Recognition\n");

    const char *test_cases[] = {
        "CREATE TABLE t1 (id int, name text_enc, salary int_enc);",
        "CREATE TABLE t2 (id int, data varchar_enc(100), amount numeric_enc);",
        "CREATE TABLE t3 (id int, content text_enc COLLATE \"C\");",
        "ALTER TABLE t1 ADD COLUMN bonus float8_enc;",
        "ALTER TABLE t1 ALTER COLUMN name TYPE text_enc COLLATE \"en_US\";",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        List *parse_tree = tecc_parser_parse(sql);

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Encrypted column test: %s", sql);

        if (parse_tree) {
            print_test_result(test_name, true);

            /* Simple check - just verify parsing succeeded */
            printf("    - Parsing successful\n");

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL with encrypted columns\n");
        }
    }
}

/* Test 5: DDL statement node type verification */
static void test_ddl_node_type_verification(void)
{
    printf("\nTest 5: DDL Statement Node Type Verification\n");

    struct {
        const char *sql;
        int expected_type;
        const char *type_name;
    } test_cases[] = {
        {"CREATE TABLE t (id int);", 254, "FE_T_CreateStmt"},
        {"ALTER TABLE t ADD COLUMN c int;", 244, "FE_T_AlterTableStmt"},
        {"DROP TABLE t;", 256, "FE_T_DropStmt"},
        {"CREATE KEK k;", 1000, "T_CreateKekStmt"},
        {"CREATE DEK d;", 1001, "T_CreateDekStmt"},
        {"DROP KEK k;", 256, "FE_T_DropStmt"},
        {"DROP DEK d;", 256, "FE_T_DropStmt"},
        {NULL, 0, NULL}
    };

    for (int i = 0; test_cases[i].sql != NULL; i++) {
        List *parse_tree = tecc_parser_parse(test_cases[i].sql);

        if (parse_tree && parse_tree->head) {
            ListCell *cell = parse_tree->head;
            Node *node = (Node *)cell->data.ptr_value;

            char test_name[256];
            snprintf(test_name, sizeof(test_name), "Node type %s = %d",
                    test_cases[i].type_name, test_cases[i].expected_type);

            bool passed = (node->type == test_cases[i].expected_type);
            print_test_result(test_name, passed);

            if (!passed) {
                printf("    - Expected: %d, Got: %d\n",
                       test_cases[i].expected_type, node->type);
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            char test_name[256];
            snprintf(test_name, sizeof(test_name), "Parse %s", test_cases[i].sql);
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
        }
    }
}

int main(void)
{
    printf("=== TECC Frontend Parser DDL Statement Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run all DDL tests */
    test_create_table_parsing();
    test_alter_table_parsing();
    test_drop_table_parsing();
    test_encrypted_column_recognition();
    test_ddl_node_type_verification();

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n✅ All DDL statement tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some DDL tests failed. Please check the errors above.\n");
        return 1;
    }
}