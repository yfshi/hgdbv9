/* Begin - ReqID:TECC4.0 */
/*
 * File: test_parser_basic.c
 *
 * Abstract:
 *   Basic functional tests for TECC frontend parser.
 *   Tests basic SQL parsing, node creation, and memory management.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_parser_basic.c
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

/* Simple memory allocation functions for testing */
void *test_palloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void *test_palloc0(size_t size)
{
    return calloc(1, size);
}

void test_pfree(void *ptr)
{
    free(ptr);
}

char *test_pstrdup(const char *str)
{
    if (!str) return NULL;
    return strdup(str);
}

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

/* Test 1: Basic SQL parsing */
static void test_basic_sql_parsing(void)
{
    printf("Test 1: Basic SQL Parsing\n");

    const char *test_cases[] = {
        "SELECT * FROM users;",
        "INSERT INTO users (id, name) VALUES (1, 'test');",
        "UPDATE users SET name = 'new' WHERE id = 1;",
        "DELETE FROM users WHERE id = 1;",
        "CREATE KEK master_key WITH (KEY_STORE = kms_method, KEY_PATH = 'key_path');",
        "CREATE DEK data_key WITH VALUES (KEK = kek_name);",
        "DROP DEK data_key;",
        "COPY users FROM STDIN;",
        "COPY users (id, name) FROM STDIN;",
        "COPY users TO STDOUT;",
        "COPY (SELECT * FROM users) TO STDOUT;",
        "COPY users TO STDOUT WITH (FORMAT 'csv', HEADER);",
        "PREPARE plan1 AS SELECT * FROM users;",
        "EXECUTE plan1;",
        "DECLARE cur1 CURSOR FOR SELECT * FROM users;",
        NULL
    };

    for (int i = 0; test_cases[i] != NULL; i++) {
        const char *sql = test_cases[i];
        List *parse_tree = tecc_parser_parse(sql);

        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Parse SQL: %s", sql);

        if (parse_tree) {
            print_test_result(test_name, true);

            /* Verify parse tree structure */
            if (parse_tree->head && parse_tree->length > 0) {
                printf("    - Parse tree: length=%d\n",
                       parse_tree->length);

                /* Verify node has location information */
                ListCell *cell = parse_tree->head;
                while (cell) {
                    Node *node = (Node *)cell->data.ptr_value;
                    if (node) {
                        printf("    - Node: type=%d, location=%d\n",
                               node->type, node->location);
                    }
                    cell = cell->next;
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result(test_name, false);
        }
    }
}

/* Test 2: Node type verification */
static void test_node_type_verification(void)
{
    printf("\nTest 2: Node Type Verification\n");

    /* Test standard SQL statement node types */
    struct {
        const char *sql;
        int expected_type;
        const char *type_name;
    } test_cases[] = {
        {"SELECT * FROM t;", FE_T_SelectStmt, "T_SelectStmt"},
        {"INSERT INTO t VALUES (1);", FE_T_InsertStmt, "T_InsertStmt"},
        {"INSERT INTO t DEFAULT VALUES;", FE_T_InsertStmt, "T_InsertStmt"},
        {"UPDATE t SET c = 1;", FE_T_UpdateStmt, "T_UpdateStmt"},
        {"DELETE FROM t;", FE_T_DeleteStmt, "T_DeleteStmt"},
        {"DROP TABLE t;", FE_T_DropStmt, "T_DropStmt"},
        {"CREATE KEK k WITH (KEY_STORE = kms);", FE_T_CreateKekStmt, "T_CreateKekStmt"},
        {"CREATE DEK d WITH VALUES (KEK = k);", FE_T_CreateDekStmt, "T_CreateDekStmt"},
        {"COPY t TO STDOUT;", FE_T_CopyStmt, "T_CopyStmt"},
        {"COPY t FROM STDIN;", FE_T_CopyStmt, "T_CopyStmt"},
        {"COPY (SELECT * FROM t) TO STDOUT;", FE_T_CopyStmt, "T_CopyStmt"},
        {"PREPARE p AS SELECT * FROM t;", FE_T_PrepareStmt, "T_PrepareStmt"},
        {"EXECUTE p;", FE_T_ExecuteStmt, "T_ExecuteStmt"},
        {"DECLARE c CURSOR FOR SELECT * FROM t;", FE_T_DeclareCursorStmt, "T_DeclareCursorStmt"},
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
        }
    }
}

/* Test 3: Position information */
static void test_position_information(void)
{
    printf("\nTest 3: Position Information\n");

    const char *sql = "SELECT id, name FROM users WHERE age > 18;";
    List *parse_tree = tecc_parser_parse(sql);

    if (parse_tree && parse_tree->head) {
        ListCell *cell = parse_tree->head;
        Node *node = (Node *)cell->data.ptr_value;

        /* Test 3.1: Location should be valid */
        bool location_valid = (node->location >= 0);
        print_test_result("Location is valid", location_valid);

        /* Test 3.2: Location should be within SQL bounds */
        bool bounds_valid = (node->location >= 0 && node->location <= strlen(sql));
        print_test_result("Location within bounds", bounds_valid);

        if (bounds_valid && node->location < strlen(sql)) {
            /* Extract SQL fragment from location to end for demonstration */
            char fragment[256];
            int max_len = strlen(sql) - node->location;
            if (max_len > 255) max_len = 255;
            strncpy(fragment, sql + node->location, max_len);
            fragment[max_len] = '\0';
            printf("    - SQL from location: \"%s\"\n", fragment);
        }

        tecc_parser_free_tree(parse_tree);
    } else {
        print_test_result("Parse SQL for position test", false);
    }
}

/* Test 4: Memory management */
static void test_memory_management(void)
{
    printf("\nTest 4: Memory Management\n");

    /* Test 4.1: Parse and free multiple times */
    const char *sql = "SELECT * FROM test;";
    bool all_freed = true;

    for (int i = 0; i < 10; i++) {
        List *parse_tree = tecc_parser_parse(sql);
        if (!parse_tree) {
            all_freed = false;
            break;
        }
        tecc_parser_free_tree(parse_tree);
    }

    print_test_result("Multiple parse/free cycles", all_freed);

    /* Test 4.2: NULL handling */
    List *null_result = tecc_parser_parse(NULL);
    bool null_handled = (null_result == NULL);
    print_test_result("NULL input returns NULL", null_handled);

    /* Test 4.3: Empty string handling */
    List *empty_result = tecc_parser_parse("");
    bool empty_handled = (empty_result == NULL);
    print_test_result("Empty string returns NULL", empty_handled);
}

/* Test 5: List structure */
static void test_list_structure(void)
{
    printf("\nTest 5: List Structure\n");

    const char *sql = "SELECT * FROM table1; SELECT * FROM table2;";
    List *parse_tree = tecc_parser_parse(sql);

    if (parse_tree) {
        /* Test 5.1: List has valid structure */
        bool has_valid_structure = ((parse_tree->type == FE_T_List || parse_tree->type == 0) &&
                                   parse_tree->length >= 0);
        print_test_result("List has valid structure", has_valid_structure);

        /* Test 5.2: List length matches cell count */
        int cell_count = 0;
        ListCell *cell = parse_tree->head;
        while (cell) {
            cell_count++;
            cell = cell->next;
        }

        bool length_matches = (parse_tree->length == cell_count);
        print_test_result("List length matches cell count", length_matches);

        printf("    - List length: %d, Cell count: %d\n",
               parse_tree->length, cell_count);

        tecc_parser_free_tree(parse_tree);
    } else {
        print_test_result("Parse SQL for list test", false);
    }
}

/* Test 6: Prepared statement and cursor node structure */
static void test_prepare_execute_cursor_nodes(void)
{
    printf("\nTest 6: Prepare/Execute/Declare Cursor Node Structure\n");

    /* Test 6.1: PREPARE statement node */
    {
        const char *sql = "PREPARE myplan AS SELECT * FROM users WHERE id = 1;";
        List *parse_tree = tecc_parser_parse(sql);

        if (parse_tree && parse_tree->head) {
            Node *node = (Node *)parse_tree->head->data.ptr_value;
            bool is_prepare = (node->type == FE_T_PrepareStmt);
            print_test_result("PREPARE: correct node type", is_prepare);

            if (is_prepare) {
                PrepareStmt *ps = (PrepareStmt *)node;
                bool has_name = (ps->name != NULL && strcmp(ps->name, "myplan") == 0);
                bool has_query = (ps->query != NULL);
                print_test_result("PREPARE: has correct name", has_name);
                print_test_result("PREPARE: has inner query", has_query);

                if (ps->query) {
                    bool query_is_select = (ps->query->type == FE_T_SelectStmt);
                    print_test_result("PREPARE: inner query is SELECT", query_is_select);
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result("PREPARE: parse SQL", false);
        }
    }

    /* Test 6.2: EXECUTE statement node */
    {
        const char *sql = "EXECUTE myplan;";
        List *parse_tree = tecc_parser_parse(sql);

        if (parse_tree && parse_tree->head) {
            Node *node = (Node *)parse_tree->head->data.ptr_value;
            bool is_execute = (node->type == FE_T_ExecuteStmt);
            print_test_result("EXECUTE: correct node type", is_execute);

            if (is_execute) {
                ExecuteStmt *es = (ExecuteStmt *)node;
                bool has_name = (es->name != NULL && strcmp(es->name, "myplan") == 0);
                print_test_result("EXECUTE: has correct name", has_name);
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result("EXECUTE: parse SQL", false);
        }
    }

    /* Test 6.3: DECLARE CURSOR statement node */
    {
        const char *sql = "DECLARE mycursor CURSOR FOR SELECT * FROM users;";
        List *parse_tree = tecc_parser_parse(sql);

        if (parse_tree && parse_tree->head) {
            Node *node = (Node *)parse_tree->head->data.ptr_value;
            bool is_declare = (node->type == FE_T_DeclareCursorStmt);
            print_test_result("DECLARE CURSOR: correct node type", is_declare);

            if (is_declare) {
                DeclareCursorStmt *dc = (DeclareCursorStmt *)node;
                bool has_name = (dc->portalname != NULL && strcmp(dc->portalname, "mycursor") == 0);
                bool has_query = (dc->query != NULL);
                print_test_result("DECLARE CURSOR: has correct cursor name", has_name);
                print_test_result("DECLARE CURSOR: has inner query", has_query);

                if (dc->query) {
                    bool query_is_select = (dc->query->type == FE_T_SelectStmt);
                    print_test_result("DECLARE CURSOR: inner query is SELECT", query_is_select);
                }
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            print_test_result("DECLARE CURSOR: parse SQL", false);
        }
    }
}

int main(void)
{
    printf("=== TECC Frontend Parser Basic Functional Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run all tests */
    test_basic_sql_parsing();
    test_node_type_verification();
    test_position_information();
    test_memory_management();
    test_list_structure();
    test_prepare_execute_cursor_nodes();

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n✅ All basic functional tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Please check the errors above.\n");
        return 1;
    }
}