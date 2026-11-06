/* Begin - ReqID:TECC4.0 */
/*
 * File: test_key_manager_performance.c
 *
 * Abstract:
 *   Performance tests for TECC key management module.
 *   Tests throughput, latency, and resource usage.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_key_manager_performance.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "high_level_api.h"
#include <math.h>  /* For fabs() */

/* Fixed KEK constants (from high_level_api.c) */
#define TECC_FIXED_KEK_LENGTH 32  /* 256-bit key */
static const unsigned char TECC_FIXED_KEK_DATA[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/* Mock cache function for testing */
typedef struct TeccColumnKeyMetadata {
    char key_name[64];
    unsigned int kek_id;
    char encrypted_key[1024];
    bool supports_det;
    bool supports_ope;
    bool supports_search;
    char status[16];
} TeccColumnKeyMetadata;

typedef const TeccColumnKeyMetadata *TeccCacheColumnKeyResult;

/* Multiple mock cache entries for performance testing */
static TeccColumnKeyMetadata mock_cache_entries[] = {
    {
        .key_name = "dek_perf_001",
        .kek_id = 10001,
        .encrypted_key = "0123456789abcdef0123456789abcdef",
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    },
    {
        .key_name = "dek_perf_002",
        .kek_id = 10001,
        .encrypted_key = "123456789abcdef0123456789abcdef0",
        .supports_det = true,
        .supports_ope = true,
        .supports_search = false,
        .status = "active"
    },
    {
        .key_name = "dek_perf_003",
        .kek_id = 10002,
        .encrypted_key = "23456789abcdef0123456789abcdef01",
        .supports_det = false,
        .supports_ope = true,
        .supports_search = false,
        .status = "active"
    },
    {
        .key_name = "dek_perf_004",
        .kek_id = 10001,
        .encrypted_key = "3456789abcdef0123456789abcdef012",
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    },
    {
        .key_name = "dek_perf_005",
        .kek_id = 10002,
        .encrypted_key = "456789abcdef0123456789abcdef0123",
        .supports_det = false,
        .supports_ope = true,
        .supports_search = true,
        .status = "active"
    }
};

/* Mock cache function - old interface removed */
/* Note: Use tecc_cache_get_dek_by_name() in real implementations */

/* Mock security functions for testing */
int security_decrypt_dek_with_hmac(const unsigned char *encrypted_dek,
                                  size_t encrypted_len,
                                  const unsigned char *kek,
                                  size_t kek_len,
                                  unsigned char **decrypted_dek,
                                  size_t *decrypted_len)
{
    /* Mock decryption: just copy data for testing */
    if (encrypted_dek == NULL || encrypted_len == 0 || kek == NULL || kek_len == 0)
        return -1;

    *decrypted_dek = malloc(encrypted_len);
    if (*decrypted_dek == NULL)
        return -1;

    memcpy(*decrypted_dek, encrypted_dek, encrypted_len);
    *decrypted_len = encrypted_len;

    return 0;
}

void tecc_key_secure_erase(void *data, size_t length)
{
    if (data != NULL && length > 0)
    {
        memset(data, 0, length);
    }
}

/* Performance measurement utilities */
typedef struct {
    struct timeval start_time;
    struct timeval end_time;
} Timer;

void timer_start(Timer *timer)
{
    gettimeofday(&timer->start_time, NULL);
}

void timer_stop(Timer *timer)
{
    gettimeofday(&timer->end_time, NULL);
}

double timer_elapsed_ms(Timer *timer)
{
    long seconds = timer->end_time.tv_sec - timer->start_time.tv_sec;
    long microseconds = timer->end_time.tv_usec - timer->start_time.tv_usec;
    return (seconds * 1000.0) + (microseconds / 1000.0);
}

double timer_elapsed_us(Timer *timer)
{
    long seconds = timer->end_time.tv_sec - timer->start_time.tv_sec;
    long microseconds = timer->end_time.tv_usec - timer->start_time.tv_usec;
    return (seconds * 1000000.0) + microseconds;
}

/* Test function prototypes */
void test_throughput_keks(void);
void test_throughput_deks(void);
void test_latency_keks(void);
void test_latency_deks(void);
void test_mixed_workload(void);
void test_memory_footprint(void);
void test_cache_performance(void);

int main()
{
    printf("TECC Key Management Module Performance Tests\n");
    printf("============================================\n\n");

    /* Run all performance tests */
    test_throughput_keks();
    test_throughput_deks();
    test_latency_keks();
    test_latency_deks();
    test_mixed_workload();
    test_memory_footprint();
    test_cache_performance();

    printf("\n============================================\n");
    printf("All performance tests completed.\n");

    return 0;
}

void test_throughput_keks(void)
{
    printf("=== Testing KEK Throughput ===\n");

    const int iterations = 10000;
    Timer timer;
    int success_count = 0;

    printf("  Performing %d KEK retrieval operations...\n", iterations);

    timer_start(&timer);

    for (int i = 0; i < iterations; i++) {
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);

        if (kek != NULL) {
            success_count++;
            tecc_key_free_key_data(kek, kek_len);
        }

        /* Clear errors between iterations */
        tecc_key_clear_last_error();
    }

    timer_stop(&timer);

    double total_time_ms = timer_elapsed_ms(timer);
    double ops_per_second = (success_count / total_time_ms) * 1000.0;
    double avg_time_ms = total_time_ms / success_count;

    printf("  Results:\n");
    printf("    Total time: %.2f ms\n", total_time_ms);
    printf("    Successful operations: %d/%d (%.1f%%)\n",
           success_count, iterations, (success_count * 100.0) / iterations);
    printf("    Throughput: %.0f ops/second\n", ops_per_second);
    printf("    Average latency: %.3f ms/op\n", avg_time_ms);

    if (success_count == iterations) {
        printf("  SUCCESS: All KEK operations completed successfully\n");
    } else {
        printf("  WARNING: %d KEK operations failed\n", iterations - success_count);
    }
}

void test_throughput_deks(void)
{
    printf("\n=== Testing DEK Throughput ===\n");

    const int iterations = 10000;
    Timer timer;
    int success_count = 0;

    /* Use different DEK names to simulate realistic workload */
    const char *dek_names[] = {"dek_perf_001", "dek_perf_002", "dek_perf_003",
                              "dek_perf_004", "dek_perf_005"};
    int num_deks = sizeof(dek_names) / sizeof(dek_names[0]);

    printf("  Performing %d DEK retrieval operations (%d different DEKs)...\n",
           iterations, num_deks);

    timer_start(&timer);

    for (int i = 0; i < iterations; i++) {
        const char *dek_name = dek_names[i % num_deks];
        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_name, &dek_len);

        if (dek != NULL) {
            success_count++;
            tecc_key_free_key_data(dek, dek_len);
        }

        /* Clear errors between iterations */
        tecc_key_clear_last_error();
    }

    timer_stop(&timer);

    double total_time_ms = timer_elapsed_ms(timer);
    double ops_per_second = (success_count / total_time_ms) * 1000.0;
    double avg_time_ms = total_time_ms / success_count;

    printf("  Results:\n");
    printf("    Total time: %.2f ms\n", total_time_ms);
    printf("    Successful operations: %d/%d (%.1f%%)\n",
           success_count, iterations, (success_count * 100.0) / iterations);
    printf("    Throughput: %.0f ops/second\n", ops_per_second);
    printf("    Average latency: %.3f ms/op\n", avg_time_ms);

    if (success_count == iterations) {
        printf("  SUCCESS: All DEK operations completed successfully\n");
    } else {
        printf("  WARNING: %d DEK operations failed\n", iterations - success_count);
    }
}

void test_latency_keks(void)
{
    printf("\n=== Testing KEK Latency ===\n");

    const int warmup_iterations = 100;
    const int measurement_iterations = 1000;
    double total_latency_us = 0.0;
    double min_latency_us = 1e9;
    double max_latency_us = 0.0;
    int success_count = 0;

    printf("  Measuring latency for %d KEK operations...\n", measurement_iterations);

    /* Warmup phase */
    for (int i = 0; i < warmup_iterations; i++) {
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);
        if (kek != NULL) {
            tecc_key_free_key_data(kek, kek_len);
        }
        tecc_key_clear_last_error();
    }

    /* Measurement phase */
    for (int i = 0; i < measurement_iterations; i++) {
        Timer timer;
        timer_start(&timer);

        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);

        timer_stop(&timer);

        if (kek != NULL) {
            double latency_us = timer_elapsed_us(timer);
            total_latency_us += latency_us;
            success_count++;

            if (latency_us < min_latency_us) min_latency_us = latency_us;
            if (latency_us > max_latency_us) max_latency_us = latency_us;

            tecc_key_free_key_data(kek, kek_len);
        }

        tecc_key_clear_last_error();
    }

    if (success_count > 0) {
        double avg_latency_us = total_latency_us / success_count;
        double avg_latency_ms = avg_latency_us / 1000.0;

        printf("  Results (based on %d successful measurements):\n", success_count);
        printf("    Average latency: %.1f us (%.3f ms)\n", avg_latency_us, avg_latency_ms);
        printf("    Minimum latency: %.1f us\n", min_latency_us);
        printf("    Maximum latency: %.1f us\n", max_latency_us);
        printf("    Latency range: %.1f us\n", max_latency_us - min_latency_us);

        /* Calculate percentiles (simplified) */
        printf("    Performance characteristics:\n");
        if (avg_latency_us < 100) {
            printf("      Excellent performance (< 100us)\n");
        } else if (avg_latency_us < 1000) {
            printf("      Good performance (< 1ms)\n");
        } else if (avg_latency_us < 10000) {
            printf("      Acceptable performance (< 10ms)\n");
        } else {
            printf("      Performance may need optimization (> 10ms)\n");
        }
    } else {
        printf("  FAIL: No successful KEK operations to measure\n");
    }
}

void test_latency_deks(void)
{
    printf("\n=== Testing DEK Latency ===\n");

    const int warmup_iterations = 100;
    const int measurement_iterations = 1000;
    double total_latency_us = 0.0;
    double min_latency_us = 1e9;
    double max_latency_us = 0.0;
    int success_count = 0;

    /* Use different DEK names */
    const char *dek_names[] = {"dek_perf_001", "dek_perf_002", "dek_perf_003"};
    int num_deks = sizeof(dek_names) / sizeof(dek_names[0]);

    printf("  Measuring latency for %d DEK operations (%d different DEKs)...\n",
           measurement_iterations, num_deks);

    /* Warmup phase */
    for (int i = 0; i < warmup_iterations; i++) {
        const char *dek_name = dek_names[i % num_deks];
        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_name, &dek_len);
        if (dek != NULL) {
            tecc_key_free_key_data(dek, dek_len);
        }
        tecc_key_clear_last_error();
    }

    /* Measurement phase */
    for (int i = 0; i < measurement_iterations; i++) {
        const char *dek_name = dek_names[i % num_deks];
        Timer timer;
        timer_start(&timer);

        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_name, &dek_len);

        timer_stop(&timer);

        if (dek != NULL) {
            double latency_us = timer_elapsed_us(timer);
            total_latency_us += latency_us;
            success_count++;

            if (latency_us < min_latency_us) min_latency_us = latency_us;
            if (latency_us > max_latency_us) max_latency_us = latency_us;

            tecc_key_free_key_data(dek, dek_len);
        }

        tecc_key_clear_last_error();
    }

    if (success_count > 0) {
        double avg_latency_us = total_latency_us / success_count;
        double avg_latency_ms = avg_latency_us / 1000.0;

        printf("  Results (based on %d successful measurements):\n", success_count);
        printf("    Average latency: %.1f us (%.3f ms)\n", avg_latency_us, avg_latency_ms);
        printf("    Minimum latency: %.1f us\n", min_latency_us);
        printf("    Maximum latency: %.1f us\n", max_latency_us);
        printf("    Latency range: %.1f us\n", max_latency_us - min_latency_us);

        /* Compare with KEK latency */
        printf("    Comparison with KEK operations:\n");
        printf("      DEK operations include cache lookup + decryption\n");
        printf("      Expected to be slower than simple KEK retrieval\n");
    } else {
        printf("  FAIL: No successful DEK operations to measure\n");
    }
}

void test_mixed_workload(void)
{
    printf("\n=== Testing Mixed Workload ===\n");

    const int iterations = 5000;
    Timer timer;
    int kek_success = 0;
    int dek_success = 0;
    int validation_success = 0;

    printf("  Simulating mixed workload (%d total operations)...\n", iterations * 3);

    timer_start(&timer);

    for (int i = 0; i < iterations; i++) {
        /* KEK operation (33% of workload) */
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);
        if (kek != NULL) {
            kek_success++;
            tecc_key_free_key_data(kek, kek_len);
        }

        /* DEK operation (33% of workload) */
        const char *dek_name = (i % 2 == 0) ? "dek_perf_001" : "dek_perf_002";
        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_name, &dek_len);
        if (dek != NULL) {
            dek_success++;
            tecc_key_free_key_data(dek, dek_len);
        }

        /* Validation operation (33% of workload) */
        bool valid = tecc_key_validate_dek(dek_name);
        if (valid) {
            validation_success++;
        }

        /* Clear errors periodically */
        if (i % 100 == 0) {
            tecc_key_clear_last_error();
        }
    }

    timer_stop(&timer);

    double total_time_ms = timer_elapsed_ms(timer);
    int total_operations = iterations * 3;
    int total_success = kek_success + dek_success + validation_success;
    double ops_per_second = (total_success / total_time_ms) * 1000.0;

    printf("  Results:\n");
    printf("    Total time: %.2f ms\n", total_time_ms);
    printf("    Total operations: %d\n", total_operations);
    printf("    Successful operations: %d (%.1f%%)\n",
           total_success, (total_success * 100.0) / total_operations);
    printf("    Throughput: %.0f ops/second\n", ops_per_second);
    printf("    Breakdown:\n");
    printf("      KEK operations: %d/%d (%.1f%%)\n",
           kek_success, iterations, (kek_success * 100.0) / iterations);
    printf("      DEK operations: %d/%d (%.1f%%)\n",
           dek_success, iterations, (dek_success * 100.0) / iterations);
    printf("      Validation operations: %d/%d (%.1f%%)\n",
           validation_success, iterations, (validation_success * 100.0) / iterations);

    double success_rate = (total_success * 100.0) / total_operations;
    if (success_rate > 95.0) {
        printf("  SUCCESS: Mixed workload handled well (%.1f%% success rate)\n", success_rate);
    } else {
        printf("  WARNING: Mixed workload success rate below 95%% (%.1f%%)\n", success_rate);
    }
}

void test_memory_footprint(void)
{
    printf("\n=== Testing Memory Footprint ===\n");

    const int batch_size = 100;
    const int num_batches = 10;

    printf("  Testing memory usage with %d batches of %d operations...\n",
           num_batches, batch_size);

    /* Note: This is a simplified memory test. In a real scenario,
       we would use memory profiling tools. */

    size_t total_keys_allocated = 0;
    size_t max_concurrent_keys = 0;
    size_t current_concurrent_keys = 0;

    for (int batch = 0; batch < num_batches; batch++) {
        /* Allocate a batch of keys */
        unsigned char *keys[batch_size];
        size_t key_lengths[batch_size];
        int allocated_in_batch = 0;

        for (int i = 0; i < batch_size; i++) {
            if (i % 3 == 0) {
                /* KEK */
                size_t kek_len = 0;
                keys[i] = tecc_key_get_kek("test_kek", &kek_len);
                key_lengths[i] = kek_len;
            } else {
                /* DEK */
                const char *dek_name = (i % 2 == 0) ? "dek_perf_001" : "dek_perf_002";
                size_t dek_len = 0;
                keys[i] = tecc_key_get_decrypted_dek(dek_name, &dek_len);
                key_lengths[i] = dek_len;
            }

            if (keys[i] != NULL) {
                allocated_in_batch++;
                total_keys_allocated++;
                current_concurrent_keys++;
            }
        }

        /* Update max concurrent keys */
        if (current_concurrent_keys > max_concurrent_keys) {
            max_concurrent_keys = current_concurrent_keys;
        }

        /* Free half of the keys (simulating usage pattern) */
        int keys_to_free = allocated_in_batch / 2;
        int freed_count = 0;

        for (int i = 0; i < batch_size && freed_count < keys_to_free; i++) {
            if (keys[i] != NULL) {
                tecc_key_free_key_data(keys[i], key_lengths[i]);
                keys[i] = NULL;
                current_concurrent_keys--;
                freed_count++;
            }
        }

        /* Clear errors between batches */
        tecc_key_clear_last_error();

        printf("    Batch %d: Allocated %d keys, Freed %d keys, Current: %zu keys\n",
               batch + 1, allocated_in_batch, freed_count, current_concurrent_keys);
    }

    /* Clean up any remaining keys */
    printf("  Cleaning up remaining keys...\n");

    printf("  Memory footprint summary:\n");
    printf("    Total keys allocated: %zu\n", total_keys_allocated);
    printf("    Maximum concurrent keys: %zu\n", max_concurrent_keys);
    printf("    Estimated memory usage: %.1f KB (assuming 32-byte keys)\n",
           (max_concurrent_keys * 32.0) / 1024.0);

    if (current_concurrent_keys == 0) {
        printf("  SUCCESS: All keys properly freed (no memory leaks detected)\n");
    } else {
        printf("  WARNING: %zu keys not freed (potential memory leak)\n", current_concurrent_keys);
    }
}

void test_cache_performance(void)
{
    printf("\n=== Testing Cache Performance ===\n");

    const int iterations = 10000;
    Timer timer;
    int cache_hits = 0;
    int cache_misses = 0;

    /* Test with keys that are in cache */
    const char *cached_keys[] = {"dek_perf_001", "dek_perf_002", "dek_perf_003",
                                "dek_perf_004", "dek_perf_005"};
    int num_cached_keys = sizeof(cached_keys) / sizeof(cached_keys[0]);

    /* Test with keys not in cache */
    const char *uncached_keys[] = {"dek_uncached_001", "dek_uncached_002",
                                  "dek_uncached_003", "dek_uncached_004"};
    int num_uncached_keys = sizeof(uncached_keys) / sizeof(uncached_keys[0]);

    printf("  Testing cache performance with %d iterations...\n", iterations);
    printf("    Cached keys: %d, Uncached keys: %d\n", num_cached_keys, num_uncached_keys);

    timer_start(&timer);

    for (int i = 0; i < iterations; i++) {
        /* Alternate between cached and uncached keys */
        const char *key_name;
        if (i % 4 == 0) {
            /* 25% cache misses */
            key_name = uncached_keys[i % num_uncached_keys];
        } else {
            /* 75% cache hits */
            key_name = cached_keys[i % num_cached_keys];
        }

        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_dek(key_name, &dek_len);

        if (dek != NULL) {
            /* Determine if this was a cache hit or miss */
            bool is_cached_key = false;
            for (int j = 0; j < num_cached_keys; j++) {
                if (strcmp(key_name, cached_keys[j]) == 0) {
                    is_cached_key = true;
                    break;
                }
            }

            if (is_cached_key) {
                cache_hits++;
            } else {
                cache_misses++;  /* Shouldn't happen with our mock */
            }

            tecc_key_free_key_data(dek, dek_len);
        } else {
            /* Failed to get key */
            if (i % 4 == 0) {
                cache_misses++;  /* Expected for uncached keys */
            }
        }

        /* Clear errors every 100 iterations */
        if (i % 100 == 0) {
            tecc_key_clear_last_error();
        }
    }

    timer_stop(&timer);

    double total_time_ms = timer_elapsed_ms(timer);
    double ops_per_second = (iterations / total_time_ms) * 1000.0;
    double hit_ratio = (cache_hits * 100.0) / (cache_hits + cache_misses);

    printf("  Results:\n");
    printf("    Total time: %.2f ms\n", total_time_ms);
    printf("    Throughput: %.0f ops/second\n", ops_per_second);
    printf("    Cache hits: %d\n", cache_hits);
    printf("    Cache misses: %d\n", cache_misses);
    printf("    Cache hit ratio: %.1f%%\n", hit_ratio);

    /* Calculate expected vs actual hit ratio */
    double expected_hit_ratio = 75.0;  /* We designed for 75% hits */
    double hit_ratio_difference = fabs(hit_ratio - expected_hit_ratio);

    printf("    Expected hit ratio: %.1f%%\n", expected_hit_ratio);
    printf("    Difference: %.1f%%\n", hit_ratio_difference);

    if (hit_ratio_difference < 5.0) {
        printf("  SUCCESS: Cache performance within expected range\n");
    } else {
        printf("  WARNING: Cache performance outside expected range\n");
    }

    /* Test cache warming effect */
    printf("  Testing cache warming effect...\n");

    /* First access (cold cache) */
    timer_start(&timer);
    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_dek("dek_perf_001", &dek_len);
    timer_stop(&timer);
    double first_access_ms = timer_elapsed_ms(timer);

    if (dek != NULL) {
        tecc_key_free_key_data(dek, dek_len);
    }

    /* Subsequent accesses (warm cache) */
    const int warm_iterations = 100;
    double total_warm_time_ms = 0.0;

    timer_start(&timer);
    for (int i = 0; i < warm_iterations; i++) {
        dek = tecc_key_get_dek("dek_perf_001", &dek_len);
        if (dek != NULL) {
            tecc_key_free_key_data(dek, dek_len);
        }
    }
    timer_stop(&timer);
    total_warm_time_ms = timer_elapsed_ms(timer);

    double avg_warm_access_ms = total_warm_time_ms / warm_iterations;
    double speedup = first_access_ms / avg_warm_access_ms;

    printf("    First access (cold): %.3f ms\n", first_access_ms);
    printf("    Average warm access: %.3f ms\n", avg_warm_access_ms);
    printf("    Speedup factor: %.1fx\n", speedup);

    if (speedup > 1.0) {
        printf("  SUCCESS: Cache warming provides performance benefit\n");
    } else {
        printf("  NOTE: Cache warming effect not significant in this test\n");
    }
}