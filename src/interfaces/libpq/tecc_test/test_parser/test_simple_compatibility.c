/* Begin - ReqID:TECC4.0 */
/*
 * File: test_simple_compatibility.c
 *
 * Abstract:
 *   Simple test for parser compatibility with processor module.
 *   Tests CREATE TABLE parsing and basic structure.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_simple_compatibility.c
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

/* Simple test: CREATE TABLE parsing */
static void test_simple_create_table(void)
{
    printf("Test: Simple CREATE TABLE Parsing\n");

    const char *test_sql = "CREATE TABLE users (id int, name text_enc);";
    printf("Testing SQL: %s\n", test_sql);

    /* Parse SQL */
    List *parse_tree = tecc_parser_parse(test_sql);

    if (!parse_tree) {
        print_test_result("Parse CREATE TABLE", false);
        printf("    - Failed to parse SQL\n");
        return;
    }

    /* Check parse tree structure */
    if (parse_tree->head && parse_tree->length > 0) {
        ListCell *cell = parse_tree->head;
        Node *node = (Node *)cell->data;

        /* Test 1: Node type */
        bool node_type_ok = (node->type == T_CreateStmt);
        print_test_result("Node type is T_CreateStmt", node_type_ok);

        if (node_type_ok) {
            CreateStmt *create_stmt = (CreateStmt *)node;

            /* Test 2: RangeVar structure */
            bool rangevar_ok = false;
            if (create_stmt->relation) {
                RangeVar *rangevar = create_stmt->relation;
                printf("    - Table name: %s\n", rangevar->relname ? rangevar->relname : "(null)");

                if (rangevar->relname && strcmp(rangevar->relname, "users") == 0) {
                    rangevar_ok = true;
                }
            }
            print_test_result("RangeVar has correct table name", rangevar_ok);

            /* Test 3: Column definitions */
            bool columns_ok = false;
            if (create_stmt->tableElts && create_stmt->tableElts->length > 0) {
                printf("    - Found %d table elements\n", create_stmt->tableElts->length);

                ListCell *lc;
                int col_count = 0;
                foreach(lc, create_stmt->tableElts) {
                    Node *elem = (Node *)lfirst(lc);
                    if (IsA(elem, ColumnDef)) {
                        ColumnDef *col_def = (ColumnDef *)elem;
                        printf("    - Column %d: name='%s'\n", ++col_count,
                               col_def->colname ? col_def->colname : "(null)");
                    }
                }

                if (col_count >= 2) {
                    columns_ok = true;
                }
            }
            print_test_result("Has at least 2 column definitions", columns_ok);

            /* Test 4: Overall compatibility */
            if (node_type_ok && rangevar_ok && columns_ok) {
                printf("\n✅ CREATE TABLE parsing is compatible with processor requirements:\n");
                printf("    - Correct node type (T_CreateStmt)\n");
                printf("    - Table name accessible via relation->relname\n");
                printf("    - Column definitions accessible via tableElts\n");
                printf("    - Column names accessible via ColumnDef->colname\n");
            }
        }

        tecc_parser_free_tree(parse_tree);
    } else {
        print_test_result("Parse tree has content", false);
        printf("    - Parse tree is empty\n");
    }
}

/* Simple test: DROP TABLE parsing */
static void test_simple_drop_table(void)
{
    printf("\nTest: Simple DROP TABLE Parsing\n");

    const char *test_sql = "DROP TABLE users;";
    printf("Testing SQL: %s\n", test_sql);

    /* Parse SQL */
    List *parse_tree = tecc_parser_parse(test_sql);

    if (!parse_tree) {
        print_test_result("Parse DROP TABLE", false);
        printf("    - Failed to parse SQL\n");
        return;
    }

    /* Check parse tree structure */
    if (parse_tree->head && parse_tree->length > 0) {
        ListCell *cell = parse_tree->head;
        Node *node = (Node *)cell->data;

        /* Test 1: Node type */
        bool node_type_ok = (node->type == T_DropStmt);
        print_test_result("Node type is T_DropStmt", node_type_ok);

        if (node_type_ok) {
            DropStmt *drop_stmt = (DropStmt *)node;

            /* Test 2: Object type */
            bool object_type_ok = (drop_stmt->removeType == OBJECT_TABLE);
            print_test_result("Object type is TABLE", object_type_ok);

            /* Test 3: Overall compatibility */
            if (node_type_ok && object_type_ok) {
                printf("\n✅ DROP TABLE parsing is compatible with processor requirements:\n");
                printf("    - Correct node type (T_DropStmt)\n");
                printf("    - Correct object type (OBJECT_TABLE)\n");
                printf("    - Processor can identify this as a DROP TABLE statement\n");
            }
        }

        tecc_parser_free_tree(parse_tree);
    } else {
        print_test_result("Parse tree has content", false);
        printf("    - Parse tree is empty\n");
    }
}

int main(void)
{
    printf("=== TECC Parser-Processor Simple Compatibility Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run simple tests */
    test_simple_create_table();
    test_simple_drop_table();

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n✅ All compatibility tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some compatibility tests failed.\n");
        return 1;
    }
}