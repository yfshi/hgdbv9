/* Begin - ReqID:TECC4.0 */
/*
 * File: test_real_processor.c
 * Abstract:
 *   Test program that attempts to link with real processor module.
 *   Minimal implementation to avoid header conflicts.
 *
 * Authored by <wangxp@highgo.com>, 20260416.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/test_real_processor.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple logging */
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

/* Minimal forward declarations to avoid header conflicts */
typedef void* PGconn;
typedef struct Node Node;
typedef struct TeccStatementData TeccStatementData;

/* Declare processor functions with correct signatures */
void tecc_processor_init(PGconn *conn);
int tecc_processor_run_pre_statement(const Node *stmt, TeccStatementData *statement_data);

/* Test SQL */
static const char *test_sql = "INSERT INTO users (id, name, salary_enc) VALUES (1, 'Alice', 50000);";

/* Main test function */
int main(void) {
    printf("=== Real Processor Module Test ===\n\n");

    LOG_INFO("Testing processor module initialization");

    /* Initialize processor */
    tecc_processor_init(NULL);
    LOG_INFO("Processor initialization called");

    LOG_INFO("Testing processor statement processing");

    /* Note: We cannot actually call tecc_processor_run_pre_statement without
     * proper parse tree and statement data structures. This is just to test
     * linking with the real processor module.
     */

    printf("\n=== Test Completed ===\n");
    printf("If this program compiles and links successfully, it means:\n");
    printf("1. Processor module is properly compiled\n");
    printf("2. Test program can link with processor module\n");
    printf("3. No major header conflicts exist\n");

    return 0;
}
/* End - ReqID:TECC4.0 */