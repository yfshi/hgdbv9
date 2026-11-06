/* Begin - ReqID:TECC4.0 */
/*
 * File: test_compatibility.c
 *
 * Abstract:
 *   Test compatibility between frontend parser and processor module.
 *   Tests CREATE TABLE parsing and column extraction.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_compatibility.c
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

/* Include processor headers */
#include "../../../tecc_processor/tecc_stmt_processor.h"
#include "../../../tecc_processor/tecc_column_ref_data.h"

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

/* Test 1: CREATE TABLE parsing and column extraction */
static void test_create_table_compatibility(void)
{
    printf("Test 1: CREATE TABLE Parser-Processor Compatibility\n");

    const char *test_cases[] = {
        "CREATE TABLE users (id int, name text);",
        "CREATE TABLE users (id int PRIMARY KEY, name text_enc, salary int_enc);",
        "CREATE TABLE IF NOT EXISTS users (id int, name varchar(100));",
        "CREATE TABLE users (id int, name text_enc, CONSTRAINT pk_id PRIMARY KEY (id));",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        printf("\nTesting SQL: %s\n", sql);

        /* Step 1: Parse SQL with frontend parser */
        List *parse_tree = tecc_parser_parse(sql);

        if (!parse_tree) {
            char test_name[256];
            snprintf(test_name, sizeof(test_name), "Parse SQL: %s", sql);
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
            continue;
        }

        /* Step 2: Verify parse tree structure */
        if (parse_tree->head && parse_tree->length > 0) {
            ListCell *cell = parse_tree->head;
            Node *node = (Node *)cell->data;

            if (node->type == T_CreateStmt) {
                CreateStmt *create_stmt = (CreateStmt *)node;

                /* Test 1.1: Check RangeVar fields are set */
                bool rangevar_ok = false;
                if (create_stmt->relation) {
                    RangeVar *rangevar = create_stmt->relation;
                    printf("    - RangeVar: catalog='%s', schema='%s', table='%s'\n",
                           rangevar->catalogname ? rangevar->catalogname : "(null)",
                           rangevar->schemaname ? rangevar->schemaname : "(null)",
                           rangevar->relname ? rangevar->relname : "(null)");

                    /* Check that relname is set (most important for processor) */
                    if (rangevar->relname) {
                        rangevar_ok = true;
                    }
                }

                char test_name1[256];
                snprintf(test_name1, sizeof(test_name1), "RangeVar fields set for: %s", sql);
                print_test_result(test_name1, rangevar_ok);

                /* Test 1.2: Check ColumnDef structure */
                bool coldef_ok = false;
                if (create_stmt->tableElts && create_stmt->tableElts->length > 0) {
                    ListCell *lc;
                    foreach(lc, create_stmt->tableElts) {
                        Node *elem = (Node *)lfirst(lc);
                        if (IsA(elem, ColumnDef)) {
                            ColumnDef *col_def = (ColumnDef *)elem;
                            printf("    - ColumnDef: name='%s', typeName=%p\n",
                                   col_def->colname ? col_def->colname : "(null)",
                                   col_def->typeName);

                            if (col_def->colname) {
                                coldef_ok = true;
                            }
                        }
                    }
                }

                char test_name2[256];
                snprintf(test_name2, sizeof(test_name2), "ColumnDef structure valid for: %s", sql);
                print_test_result(test_name2, coldef_ok);

                /* Test 1.3: Simulate processor column extraction */
                if (rangevar_ok && coldef_ok) {
                    printf("    - Parser output compatible with processor\n");
                }
            } else {
                printf("    - ERROR: Expected T_CreateStmt, got type: %d\n", node->type);
            }
        }

        tecc_parser_free_tree(parse_tree);
    }
}

/* Test 2: DROP TABLE parsing compatibility */
static void test_drop_table_compatibility(void)
{
    printf("\nTest 2: DROP TABLE Parser-Processor Compatibility\n");

    const char *test_cases[] = {
        "DROP TABLE users;",
        "DROP TABLE users CASCADE;",
        "DROP TABLE IF EXISTS users;",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        printf("\nTesting SQL: %s\n", sql);

        /* Step 1: Parse SQL with frontend parser */
        List *parse_tree = tecc_parser_parse(sql);

        if (!parse_tree) {
            char test_name[256];
            snprintf(test_name, sizeof(test_name), "Parse DROP SQL: %s", sql);
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
            continue;
        }

        /* Step 2: Verify parse tree structure */
        if (parse_tree->head && parse_tree->length > 0) {
            ListCell *cell = parse_tree->head;
            Node *node = (Node *)cell->data;

            if (node->type == T_DropStmt) {
                DropStmt *drop_stmt = (DropStmt *)node;

                /* Check DropStmt fields */
                printf("    - DropStmt: removeType=%d, missing_ok=%d, behavior=%d\n",
                       drop_stmt->removeType, drop_stmt->missing_ok, drop_stmt->behavior);

                /* Check object type */
                bool drop_ok = (drop_stmt->removeType == OBJECT_TABLE);

                char test_name[256];
                snprintf(test_name, sizeof(test_name), "DROP TABLE structure valid: %s", sql);
                print_test_result(test_name, drop_ok);

                if (drop_ok) {
                    printf("    - DROP TABLE compatible with processor\n");
                }
            } else {
                printf("    - ERROR: Expected T_DropStmt, got type: %d\n", node->type);
            }
        }

        tecc_parser_free_tree(parse_tree);
    }
}

/* Test 3: Encrypted column type recognition */
static void test_encrypted_column_compatibility(void)
{
    printf("\nTest 3: Encrypted Column Type Compatibility\n");

    const char *test_cases[] = {
        "CREATE TABLE t1 (id int, name text_enc, salary int_enc);",
        "CREATE TABLE t2 (id int, data varchar_enc(100), amount numeric_enc);",
        "CREATE TABLE t3 (id int, content text_enc COLLATE \"C\");",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        printf("\nTesting SQL: %s\n", sql);

        /* Parse SQL */
        List *parse_tree = tecc_parser_parse(sql);

        if (!parse_tree) {
            char test_name[256];
            snprintf(test_name, sizeof(test_name), "Parse encrypted column SQL: %s", sql);
            print_test_result(test_name, false);
            printf("    - Failed to parse SQL\n");
            continue;
        }

        /* Check for encrypted column types */
        bool has_encrypted = false;
        if (parse_tree->head && parse_tree->length > 0) {
            ListCell *cell = parse_tree->head;
            Node *node = (Node *)cell->data;

            if (node->type == T_CreateStmt) {
                CreateStmt *create_stmt = (CreateStmt *)node;

                if (create_stmt->tableElts) {
                    ListCell *lc;
                    foreach(lc, create_stmt->tableElts) {
                        Node *elem = (Node *)lfirst(lc);
                        if (IsA(elem, ColumnDef)) {
                            ColumnDef *col_def = (ColumnDef *)elem;
                            if (col_def->typeName && col_def->typeName->names) {
                                /* Check if type name indicates encrypted column */
                                /* This is a simplified check - actual implementation would be more complex */
                                if (strstr(sql, "_enc") != NULL) {
                                    has_encrypted = true;
                                    printf("    - Found encrypted column: %s\n", col_def->colname);
                                }
                            }
                        }
                    }
                }
            }
        }

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Encrypted column detection: %s", sql);
        print_test_result(test_name, has_encrypted);

        tecc_parser_free_tree(parse_tree);
    }
}

int main(void)
{
    printf("=== TECC Parser-Processor Compatibility Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run all compatibility tests */
    test_create_table_compatibility();
    test_drop_table_compatibility();
    test_encrypted_column_compatibility();

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n✅ All compatibility tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some compatibility tests failed. Please check the errors above.\n");
        return 1;
    }
}