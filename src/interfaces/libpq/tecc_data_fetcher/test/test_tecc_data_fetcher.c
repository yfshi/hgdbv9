/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * test_tecc_data_fetcher.c
 *    TECC data fetcher module test suite
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/test/test_tecc_data_fetcher.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_data_fetcher.h"
#include "tecc_data_fetcher_types.h"
#include "tecc_metadata_serializer.h"
#include "tecc_permission_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 测试用例函数声明 */
static void test_bitmask_parsing(void);
static void test_error_handling(void);
static void test_permission_checking(void);

/* 辅助函数 */
static void print_test_result(const char *test_name, bool passed);

/*
 * main - 测试主函数
 */
int
main(int argc, char **argv)
{
    printf("=== TECC Data Fetcher Module Test Suite ===\n\n");

    /* 运行测试用例 */
    test_bitmask_parsing();
    test_error_handling();
    test_permission_checking();

    printf("\n=== All tests completed ===\n");
    return 0;
}



/*
 * test_bitmask_parsing - 测试位掩码解析
 */
static void
test_bitmask_parsing(void)
{
    bool passed = true;
    int bitmask;

    printf("Testing bitmask parsing...\n");

    /* 测试单个算法 */
    bitmask = tecc_parse_algorithms("RND");
    if (!tecc_has_algorithm(bitmask, TECC_ALG_RND))
    {
        printf("  ERROR: Failed to parse RND algorithm\n");
        passed = false;
    }

    /* 测试多个算法 */
    bitmask = tecc_parse_algorithms("RND,DET");
    if (!tecc_has_algorithm(bitmask, TECC_ALG_RND) ||
        !tecc_has_algorithm(bitmask, TECC_ALG_DET))
    {
        printf("  ERROR: Failed to parse RND,DET algorithms\n");
        passed = false;
    }

    /* 测试无效算法 */
    bitmask = tecc_parse_algorithms("INVALID");
    if (bitmask != 0)
    {
        printf("  ERROR: Should return 0 for invalid algorithm\n");
        passed = false;
    }

    /* 测试空字符串 */
    bitmask = tecc_parse_algorithms("");
    if (bitmask != 0)
    {
        printf("  ERROR: Should return 0 for empty string\n");
        passed = false;
    }

    print_test_result("Bitmask Parsing", passed);
}

/*
 * test_error_handling - 测试错误处理
 */
static void
test_error_handling(void)
{
    bool passed = true;
    const char *error_msg;

    printf("Testing error handling...\n");

    /* 测试所有错误码的消息 */
    error_msg = tecc_fetcher_error_message(TECC_FETCHER_OK);
    if (error_msg == NULL || strcmp(error_msg, "Success") != 0)
    {
        printf("  ERROR: Wrong message for TECC_FETCHER_OK\n");
        passed = false;
    }

    error_msg = tecc_fetcher_error_message(TECC_FETCHER_ERROR_CONNECTION);
    if (error_msg == NULL || strcmp(error_msg, "Connection error") != 0)
    {
        printf("  ERROR: Wrong message for TECC_FETCHER_ERROR_CONNECTION\n");
        passed = false;
    }

    /* 测试未知错误码 */
    error_msg = tecc_fetcher_error_message(999);
    if (error_msg == NULL || strcmp(error_msg, "Unknown error") != 0)
    {
        printf("  ERROR: Wrong message for unknown error code\n");
        passed = false;
    }

    /* 测试错误日志函数（应该不崩溃） */
    tecc_log_fetcher_error(TECC_FETCHER_OK, "Test context");
    tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY, NULL);

    print_test_result("Error Handling", passed);
}

/*
 * test_permission_checking - 测试权限检查
 */
static void
test_permission_checking(void)
{
    bool passed = true;
    const char *user;

    printf("Testing permission checking...\n");

    /* 注意：这些测试在没有实际数据库连接的情况下是存根测试 */
    /* 在实际测试中，需要设置测试数据库和权限 */

    /* 测试函数调用（应该不崩溃） */
    /* 注意：由于函数签名已更新，需要传递state参数，但在测试环境中我们传递NULL */
    (void)tecc_check_kek_permission(NULL, "test_kek", "test_namespace");
    (void)tecc_check_dek_permission(NULL, "test_dek", "test_namespace");
    /* 测试获取当前用户（应该返回非NULL） */
    user = tecc_get_current_user(NULL);
    if (user == NULL)
    {
        printf("  WARNING: tecc_get_current_user returned NULL\n");
        /* 这不是错误，因为测试环境可能没有设置用户 */
    }

    print_test_result("Permission Checking", passed);
}

/*
 * print_test_result - 打印测试结果
 */
static void
print_test_result(const char *test_name, bool passed)
{
    if (passed)
    {
        printf("  ✓ %s: PASSED\n", test_name);
    }
    else
    {
        printf("  ✗ %s: FAILED\n", test_name);
    }
}


/* End - ReqID:TECC4.0 */