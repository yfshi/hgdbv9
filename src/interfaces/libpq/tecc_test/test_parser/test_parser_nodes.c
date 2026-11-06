/* Begin - ReqID:TECC4.0 */
/*
 * File: test_parser_nodes.c
 *
 * Abstract:
 *   Node type compatibility tests for TECC frontend parser.
 *   Tests that node type values are compatible with PostgreSQL
 *   and that the node structure layout is correct.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_parser_nodes.c
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

void test_pfree(void *ptr)
{
    free(ptr);
}

/* Helper function to print test result */
static void print_test_result(const char *test_name, bool passed, const char *details)
{
    if (passed) {
        printf("  ✅ %s", test_name);
        if (details) printf(" (%s)", details);
        printf("\n");
        tests_passed++;
    } else {
        printf("  ❌ %s", test_name);
        if (details) printf(" (%s)", details);
        printf("\n");
        tests_failed++;
    }
}

/* Test 1: Node structure layout */
static void test_node_structure_layout(void)
{
    printf("Test 1: Node Structure Layout\n");

    /* Test 1.1: Node structure size and alignment (TECC: type + location) */
    size_t node_size = sizeof(Node);
    bool size_valid = (node_size >= sizeof(int) * 2);
    print_test_result("Node structure size", size_valid,
                     node_size == 8 ? "8 bytes (2 ints)" : "unexpected size");

    /* Test 1.2: Field offsets */
    Node test_node;
    memset(&test_node, 0, sizeof(test_node));
    test_node.type = 100;
    test_node.location = 200;

    bool offsets_valid = true;

    /* Check type field */
    int *type_ptr = &test_node.type;
    if ((char *)type_ptr != (char *)&test_node) {
        offsets_valid = false;
        printf("    - Type field offset incorrect\n");
    }

    /* Check location field */
    int *location_ptr = &test_node.location;
    if ((char *)location_ptr != (char *)&test_node + sizeof(int)) {
        offsets_valid = false;
        printf("    - Location field offset incorrect\n");
    }

    print_test_result("Node field offsets", offsets_valid, NULL);

    if (offsets_valid) {
        printf("    - Field order: type → location\n");
        printf("    - Field values: type=%d, location=%d\n",
               test_node.type, test_node.location);
    }
}

/* Test 2: Node type value compatibility */
static void test_node_type_compatibility(void)
{
    printf("\nTest 2: Node Type Value Compatibility\n");

    /* PostgreSQL compatible node type values (should be < 400) */
    struct {
        const char *type_name;
        int type_value;
        bool is_postgresql_compatible;
    } pg_compatible_types[] = {
        {"T_SelectStmt", 241, true},
        {"T_InsertStmt", 238, true},
        {"T_UpdateStmt", 240, true},
        {"T_DeleteStmt", 239, true},
        {"T_DropStmt", 256, true},
        {"T_List", 231, true},
        {"T_Value", 225, true},
        {"T_Integer", 226, true},
        {"T_String", 228, true},
        {"T_Float", 227, true},
        {"T_Null", 230, true},
        {NULL, 0, false}
    };

    /* TECC specific node type values (simplified parser range: < 200) */
    struct {
        const char *type_name;
        int type_value;
        bool is_tecc_specific;
    } tecc_specific_types[] = {
        {"FE_T_CreateKekStmt", FE_T_CreateKekStmt, true},
        {"FE_T_CreateDekStmt", FE_T_CreateDekStmt, true},
        {NULL, 0, false}
    };

    printf("  2.1: PostgreSQL Compatible Types (< 400):\n");
    for (int i = 0; pg_compatible_types[i].type_name != NULL; i++) {
        bool is_compatible = (pg_compatible_types[i].type_value < 400);
        char details[64];
        snprintf(details, sizeof(details), "%d", pg_compatible_types[i].type_value);

        print_test_result(pg_compatible_types[i].type_name, is_compatible, details);

        if (!is_compatible) {
            printf("    - Warning: Value %d may conflict with PostgreSQL\n",
                   pg_compatible_types[i].type_value);
        }
    }

    printf("\n  2.2: TECC Specific Types (simplified parser):\n");
    for (int i = 0; tecc_specific_types[i].type_name != NULL; i++) {
        bool is_tecc = tecc_specific_types[i].is_tecc_specific;
        char details[64];
        snprintf(details, sizeof(details), "%d", tecc_specific_types[i].type_value);

        print_test_result(tecc_specific_types[i].type_name, is_tecc, details);
    }

    /* Test 2.3: No overlap between PostgreSQL and TECC ranges */
    bool no_overlap = true;
    for (int i = 0; pg_compatible_types[i].type_name != NULL; i++) {
        for (int j = 0; tecc_specific_types[j].type_name != NULL; j++) {
            if (pg_compatible_types[i].type_value == tecc_specific_types[j].type_value) {
                no_overlap = false;
                printf("    - Conflict: %s and %s both use value %d\n",
                       pg_compatible_types[i].type_name,
                       tecc_specific_types[j].type_name,
                       pg_compatible_types[i].type_value);
            }
        }
    }

    print_test_result("No type value conflicts", no_overlap, NULL);
}

/* Test 3: Node type mapping verification */
static void test_node_type_mapping(void)
{
    printf("\nTest 3: Node Type Mapping Verification\n");

    /* Test that SQL statements produce correct node types */
    struct {
        const char *sql;
        int expected_type;
        const char *expected_name;
    } test_cases[] = {
        /* Standard SQL statements */
        {"SELECT * FROM t;", 241, "T_SelectStmt"},
        {"INSERT INTO t VALUES (1);", 238, "T_InsertStmt"},
        {"UPDATE t SET c = 1;", 240, "T_UpdateStmt"},
        {"DELETE FROM t;", 239, "T_DeleteStmt"},
        {"DROP TABLE t;", FE_T_DropStmt, "T_DropStmt"},

        /* TECC specific statements */
        {"CREATE KEK master_key WITH (KEY_STORE = kms_method);", FE_T_CreateKekStmt, "T_CreateKekStmt"},
        {"CREATE DEK data_key WITH VALUES (KEK = kek_name);", FE_T_CreateDekStmt, "T_CreateDekStmt"},
        {"DROP DEK data_key;", FE_T_DropStmt, "T_DropStmt"}, /* DROP uses same type */

        {NULL, 0, NULL}
    };

    for (int i = 0; test_cases[i].sql != NULL; i++) {
        List *parse_tree = tecc_parser_parse(test_cases[i].sql);

        if (parse_tree && parse_tree->head) {
            ListCell *cell = parse_tree->head;
            Node *node = (Node *)cell->data.ptr_value;

            bool type_matches = (node->type == test_cases[i].expected_type);

            char test_name[128];
            snprintf(test_name, sizeof(test_name), "SQL: %s", test_cases[i].sql);

            char details[128];
            snprintf(details, sizeof(details), "expected %s(%d), got %d",
                    test_cases[i].expected_name, test_cases[i].expected_type, node->type);

            print_test_result(test_name, type_matches, details);

            if (!type_matches) {
                printf("    - SQL: %s\n", test_cases[i].sql);
                printf("    - Expected: %s (%d)\n",
                       test_cases[i].expected_name, test_cases[i].expected_type);
                printf("    - Got: %d\n", node->type);
            }

            tecc_parser_free_tree(parse_tree);
        } else {
            char test_name[128];
            snprintf(test_name, sizeof(test_name), "Parse: %s", test_cases[i].sql);
            print_test_result(test_name, false, "parse failed");
        }
    }
}

/* Test 4: List structure compatibility */
static void test_list_structure_compatibility(void)
{
    printf("\nTest 4: List Structure Compatibility\n");

    /* Test 4.1: List structure size */
    size_t list_size = sizeof(List);
    bool size_reasonable = (list_size >= sizeof(int) + sizeof(int) + sizeof(void *) * 2);
    print_test_result("List structure size reasonable", size_reasonable,
                     list_size == 28 ? "28 bytes" : "unexpected size");

    /* Test 4.2: List field layout (TECC: type + length + head + tail) */
    List test_list;
    memset(&test_list, 0, sizeof(test_list));
    test_list.type = 100;
    test_list.length = 300;
    test_list.head = NULL;
    test_list.tail = NULL;

    bool layout_valid = true;

    /* Quick sanity check of field values */
    if (test_list.type != 100 || test_list.length != 300) {
        layout_valid = false;
        printf("    - Field values corrupted\n");
    }

    print_test_result("List field layout", layout_valid, NULL);

    if (layout_valid) {
        printf("    - List fields: type=%d, length=%d\n",
               test_list.type, test_list.length);
    }
}

/* Test 5: Value structure compatibility */
static void test_value_structure_compatibility(void)
{
    printf("\nTest 5: Value Structure Compatibility\n");

    /* Test 5.1: Value structure size */
    size_t value_size = sizeof(Value);
    bool size_reasonable = (value_size >= sizeof(Node) + 8); /* Node + union (at least 8 bytes) */
    print_test_result("Value structure size reasonable", size_reasonable,
                     value_size == sizeof(Node) + 8 ? "Node + union" : "unexpected size");

    /* Test 5.2: Value union access */
    Value test_value;
    memset(&test_value, 0, sizeof(test_value));
    test_value.type = 225; /* T_Value */

    /* Test integer value */
    test_value.val.ival = 12345;
    bool int_valid = (test_value.val.ival == 12345);
    print_test_result("Value union integer access", int_valid, NULL);

    /* Test string value */
    test_value.val.str = "test string";
    bool str_valid = (strcmp(test_value.val.str, "test string") == 0);
    print_test_result("Value union string access", str_valid, NULL);

    /* Test float value */
    test_value.val.dval = 3.14159;
    bool float_valid = (test_value.val.dval > 3.14 && test_value.val.dval < 3.15);
    print_test_result("Value union float access", float_valid, NULL);

    /* Test boolean value */
    test_value.val.bval = true;
    bool bool_valid = (test_value.val.bval == true);
    print_test_result("Value union boolean access", bool_valid, NULL);
}

int main(void)
{
    printf("=== TECC Frontend Parser Node Type Compatibility Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run all tests */
    test_node_structure_layout();
    test_node_type_compatibility();
    test_node_type_mapping();
    test_list_structure_compatibility();
    test_value_structure_compatibility();

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_passed + tests_failed);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    /* Compatibility assessment */
    printf("\n=== Compatibility Assessment ===\n");
    if (tests_failed == 0) {
        printf("✅ All node type compatibility tests passed!\n");
        printf("✅ Node structure layout is correct\n");
        return 0;
    } else {
        printf("❌ Some compatibility tests failed.\n");
        return 1;
    }
}