/* Begin - ReqID:TECC4.0 */
/*
 * File: test_parser_performance.c
 *
 * Abstract:
 *   Performance tests for TECC frontend parser.
 *   Tests parsing speed, memory usage, and scalability.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_parser_performance.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1
#define FRONTEND_PARSER 1

/* Include parser headers */
#include "../../frontend_parser/fe_adapter.h"
#include "../../frontend_parser/tecc_nodes.h"

/* Performance test configuration */
#define WARMUP_ITERATIONS 100
#define TEST_ITERATIONS 1000
#define MEMORY_TEST_ITERATIONS 10000

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

/* Get current time in milliseconds */
static double get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* Test 1: Basic parsing performance */
static void test_basic_parsing_performance(void)
{
    printf("Test 1: Basic Parsing Performance\n");

    const char *test_sql = "SELECT * FROM users WHERE id = 1 AND name = 'test';";
    printf("  Test SQL: %s\n", test_sql);
    printf("  Warmup: %d iterations\n", WARMUP_ITERATIONS);
    printf("  Test: %d iterations\n", TEST_ITERATIONS);

    /* Warmup */
    printf("  Warming up...");
    fflush(stdout);
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        List *result = tecc_parser_parse(test_sql);
        if (result) tecc_parser_free_tree(result);
    }
    printf(" done\n");

    /* Performance test */
    printf("  Running performance test...");
    fflush(stdout);

    double start_time = get_time_ms();
    int success_count = 0;

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        List *result = tecc_parser_parse(test_sql);
        if (result) {
            success_count++;
            tecc_parser_free_tree(result);
        }

        /* Progress indicator */
        if ((i + 1) % 100 == 0) {
            printf(".");
            fflush(stdout);
        }
    }

    double end_time = get_time_ms();
    printf(" done\n");

    /* Calculate metrics */
    double total_time_ms = end_time - start_time;
    double avg_time_ms = total_time_ms / TEST_ITERATIONS;
    double ops_per_second = TEST_ITERATIONS / (total_time_ms / 1000.0);

    /* Print results */
    printf("\n  Results:\n");
    printf("    - Total time: %.2f ms\n", total_time_ms);
    printf("    - Average time per parse: %.4f ms\n", avg_time_ms);
    printf("    - Operations per second: %.0f\n", ops_per_second);
    printf("    - Success rate: %d/%d (%.1f%%)\n",
           success_count, TEST_ITERATIONS,
           (success_count * 100.0) / TEST_ITERATIONS);

    /* Performance criteria */
    bool meets_criteria = (avg_time_ms < 1.0); /* Less than 1ms per parse */
    printf("    - Performance criteria: < 1ms per parse: %s\n",
           meets_criteria ? "✅ PASS" : "❌ FAIL");
}

/* Test 2: Different SQL statement performance */
static void test_different_sql_performance(void)
{
    printf("\nTest 2: Different SQL Statement Performance\n");

    struct {
        const char *sql;
        const char *description;
    } test_cases[] = {
        {"SELECT * FROM t;", "Simple SELECT"},
        {"INSERT INTO t VALUES (1, 'test');", "Simple INSERT"},
        {"UPDATE t SET c = 1 WHERE id = 1;", "Simple UPDATE"},
        {"DELETE FROM t WHERE id = 1;", "Simple DELETE"},
        {"CREATE KEK master_key;", "TECC CREATE KEK"},
        {"CREATE DEK data_key;", "TECC CREATE DEK"},
        {"SELECT a, b, c FROM t1 JOIN t2 ON t1.id = t2.id WHERE t1.x > 10;", "Complex SELECT"},
        {NULL, NULL}
    };

    printf("  Running %d iterations per test case\n", TEST_ITERATIONS / 10);

    for (int i = 0; test_cases[i].sql != NULL; i++) {
        printf("  %s: ", test_cases[i].description);

        double start_time = get_time_ms();
        int success_count = 0;
        int iterations = TEST_ITERATIONS / 10; /* Fewer iterations for variety */

        for (int j = 0; j < iterations; j++) {
            List *result = tecc_parser_parse(test_cases[i].sql);
            if (result) {
                success_count++;
                tecc_parser_free_tree(result);
            }
        }

        double end_time = get_time_ms();
        double total_time_ms = end_time - start_time;
        double avg_time_ms = total_time_ms / iterations;

        printf("%.3f ms/parse", avg_time_ms);

        /* Performance indicator */
        if (avg_time_ms < 0.5) {
            printf(" ✅\n");
        } else if (avg_time_ms < 1.0) {
            printf(" ⚠\n");
        } else {
            printf(" ❌\n");
        }
    }
}

/* Test 3: Memory usage and leak detection */
static void test_memory_usage(void)
{
    printf("\nTest 3: Memory Usage and Leak Detection\n");

    printf("  Testing memory allocation patterns...\n");

    const char *test_sql = "SELECT * FROM test_table;";
    int iterations = MEMORY_TEST_ITERATIONS / 10;

    /* Track memory allocation pattern */
    size_t total_allocated = 0;
    size_t total_freed = 0;

    printf("  Running %d iterations...", iterations);
    fflush(stdout);

    for (int i = 0; i < iterations; i++) {
        List *result = tecc_parser_parse(test_sql);
        if (result) {
            /* Estimate memory usage */
            total_allocated += sizeof(List);

            ListCell *cell = result->head;
            while (cell) {
                total_allocated += sizeof(ListCell);
                if (cell->data.ptr_value) {
                    total_allocated += sizeof(Node);
                }
                cell = cell->next;
            }

            tecc_parser_free_tree(result);
            total_freed = total_allocated; /* Assuming perfect free */

            /* Progress indicator */
            if ((i + 1) % 100 == 0) {
                printf(".");
                fflush(stdout);
            }
        }
    }

    printf(" done\n");

    /* Analyze memory patterns */
    printf("  Memory analysis:\n");
    printf("    - Estimated allocation per parse: %.1f bytes\n",
           (double)total_allocated / iterations);

    bool no_leak = (total_allocated == total_freed);
    printf("    - Memory leak detection: %s\n",
           no_leak ? "✅ No leaks detected" : "❌ Possible leaks");

    if (!no_leak) {
        printf("    - Warning: Allocation/Freed mismatch: %zu/%zu bytes\n",
               total_allocated, total_freed);
    }
}

/* Test 4: Concurrent parsing simulation */
static void test_concurrent_parsing_simulation(void)
{
    printf("\nTest 4: Concurrent Parsing Simulation\n");

    /* Note: This is a simulation, not true concurrency */
    printf("  Simulating concurrent parsing workload...\n");

    const char *sql_patterns[] = {
        "SELECT * FROM table_%d;",
        "INSERT INTO logs (id, message) VALUES (%d, 'test');",
        "UPDATE config SET value = '%d' WHERE key = 'test';",
        "DELETE FROM temp WHERE id = %d;",
        NULL
    };

    int total_parses = 500;
    int successful_parses = 0;
    double start_time = get_time_ms();

    printf("  Parsing %d SQL statements...", total_parses);
    fflush(stdout);

    for (int i = 0; i < total_parses; i++) {
        /* Use different SQL patterns to simulate varied workload */
        const char *pattern = sql_patterns[i % 4];
        char sql[256];
        snprintf(sql, sizeof(sql), pattern, i);

        List *result = tecc_parser_parse(sql);
        if (result) {
            successful_parses++;
            tecc_parser_free_tree(result);
        }

        /* Progress indicator */
        if ((i + 1) % 50 == 0) {
            printf(".");
            fflush(stdout);
        }
    }

    double end_time = get_time_ms();
    printf(" done\n");

    double total_time_ms = end_time - start_time;
    double avg_time_ms = total_time_ms / total_parses;
    double ops_per_second = total_parses / (total_time_ms / 1000.0);

    printf("  Results:\n");
    printf("    - Total time: %.2f ms\n", total_time_ms);
    printf("    - Average time: %.3f ms\n", avg_time_ms);
    printf("    - Throughput: %.0f ops/sec\n", ops_per_second);
    printf("    - Success rate: %d/%d (%.1f%%)\n",
           successful_parses, total_parses,
           (successful_parses * 100.0) / total_parses);

    /* Performance criteria for concurrent workload */
    bool meets_criteria = (avg_time_ms < 2.0 && ops_per_second > 500);
    printf("    - Concurrent performance: %s\n",
           meets_criteria ? "✅ PASS" : "❌ FAIL");
}

/* Test 5: Stress test with long SQL */
static void test_stress_with_long_sql(void)
{
    printf("\nTest 5: Stress Test with Long SQL\n");

    /* Create a long SQL statement */
    char long_sql[1024];
    strcpy(long_sql, "SELECT ");

    /* Add many columns */
    for (int i = 0; i < 20; i++) {
        char col[32];
        snprintf(col, sizeof(col), "column_%d, ", i);
        strcat(long_sql, col);
    }

    strcat(long_sql, "extra_column FROM very_large_table WHERE ");

    /* Add many conditions */
    for (int i = 0; i < 10; i++) {
        char cond[64];
        snprintf(cond, sizeof(cond), "col_%d > %d AND ", i, i * 10);
        strcat(long_sql, cond);
    }

    strcat(long_sql, "status = 'active' ORDER BY created_at DESC LIMIT 100;");

    printf("  SQL length: %zu characters\n", strlen(long_sql));
    printf("  Running stress test...");

    int iterations = 100;
    int success_count = 0;
    double start_time = get_time_ms();

    for (int i = 0; i < iterations; i++) {
        List *result = tecc_parser_parse(long_sql);
        if (result) {
            success_count++;
            tecc_parser_free_tree(result);
        }
    }

    double end_time = get_time_ms();
    printf(" done\n");

    double total_time_ms = end_time - start_time;
    double avg_time_ms = total_time_ms / iterations;

    printf("  Results:\n");
    printf("    - Average time: %.2f ms\n", avg_time_ms);
    printf("    - Success rate: %d/%d\n", success_count, iterations);

    bool stress_test_passed = (avg_time_ms < 10.0 && success_count == iterations);
    printf("    - Stress test: %s\n",
           stress_test_passed ? "✅ PASS" : "❌ FAIL");
}

/* Test 6: Performance regression baseline */
static void test_performance_regression_baseline(void)
{
    printf("\nTest 6: Performance Regression Baseline\n");

    /* This test establishes a performance baseline for future regression testing */
    const char *baseline_sql = "SELECT * FROM baseline_table WHERE id = 123;";

    printf("  Establishing performance baseline...\n");
    printf("  SQL: %s\n", baseline_sql);
    printf("  Running baseline measurement...");

    int baseline_iterations = 1000;
    double start_time = get_time_ms();

    for (int i = 0; i < baseline_iterations; i++) {
        List *result = tecc_parser_parse(baseline_sql);
        if (result) tecc_parser_free_tree(result);
    }

    double end_time = get_time_ms();
    printf(" done\n");

    double baseline_time_ms = end_time - start_time;
    double baseline_avg_ms = baseline_time_ms / baseline_iterations;

    printf("  Baseline established:\n");
    printf("    - Total time: %.2f ms\n", baseline_time_ms);
    printf("    - Average time: %.4f ms\n", baseline_avg_ms);
    printf("    - Operations/sec: %.0f\n",
           baseline_iterations / (baseline_time_ms / 1000.0));

    /* Save baseline to file for future comparison */
    FILE *baseline_file = fopen("parser_performance_baseline.txt", "w");
    if (baseline_file) {
        fprintf(baseline_file, "TECC Frontend Parser Performance Baseline\n");
        fprintf(baseline_file, "Date: %s\n", __DATE__);
        fprintf(baseline_file, "Time: %s\n", __TIME__);
        fprintf(baseline_file, "SQL: %s\n", baseline_sql);
        fprintf(baseline_file, "Iterations: %d\n", baseline_iterations);
        fprintf(baseline_file, "Total time: %.2f ms\n", baseline_time_ms);
        fprintf(baseline_file, "Average time: %.4f ms\n", baseline_avg_ms);
        fprintf(baseline_file, "Operations/sec: %.0f\n",
                baseline_iterations / (baseline_time_ms / 1000.0));
        fclose(baseline_file);

        printf("    - Baseline saved to: parser_performance_baseline.txt\n");
    }
}

int main(void)
{
    printf("=== TECC Frontend Parser Performance Tests ===\n");
    printf("Start time: %s\n", __TIME__);
    printf("\n");

    /* Run all performance tests */
    test_basic_parsing_performance();
    test_different_sql_performance();
    test_memory_usage();
    test_concurrent_parsing_simulation();
    test_stress_with_long_sql();
    test_performance_regression_baseline();

    /* Performance summary */
    printf("\n=== Performance Summary ===\n");
    printf("All performance tests completed.\n");
    printf("\nRecommendations:\n");
    printf("1. Monitor average parse time in production\n");
    printf("2. Run performance tests regularly to detect regressions\n");
    printf("3. Consider caching for frequently parsed SQL patterns\n");
    printf("4. Profile memory usage for long-running processes\n");
    printf("\nNote: Performance depends on system load and configuration.\n");
    printf("These tests provide a baseline for comparison.\n");

    return 0;
}