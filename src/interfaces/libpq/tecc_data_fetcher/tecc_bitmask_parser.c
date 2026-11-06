/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_bitmask_parser.c
 *    TECC bitmask parsing implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_bitmask_parser.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_bitmask_parser.h"
#include "tecc_error_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * tecc_has_algorithm - 检查是否包含特定算法
 *
 * bitmask: 算法位掩码
 * alg: 要检查的算法
 * 返回: true如果包含该算法
 */
bool
tecc_has_algorithm(int bitmask, TeccEncryptionAlgorithm alg)
{
    return (bitmask & alg) != 0;
}

/*
 * tecc_parse_algorithms - 解析算法位掩码字符串
 *
 * bitmask_str: 算法字符串，如"RND,DET"
 * 返回: 位掩码值
 */
int
tecc_parse_algorithms(const char *bitmask_str)
{
    int bitmask = 0;
    char *str_copy;
    char *token;
    char *saveptr;

    if (bitmask_str == NULL || bitmask_str[0] == '\0')
    {
        return 0;
    }

    /* 复制字符串以便分割 */
    str_copy = pg_strdup(bitmask_str);
    if (str_copy == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to duplicate string in parse_algorithms");
        return 0;
    }

    /* 分割字符串 */
    token = strtok_r(str_copy, ",", &saveptr);
    while (token != NULL)
    {
        /* 去除空格 */
        while (isspace((unsigned char)*token))
            token++;

        /* 根据算法名称设置位 */
        if (strcasecmp(token, "RND") == 0)
        {
            bitmask |= TECC_ALG_RND;
        }
        else if (strcasecmp(token, "DET") == 0)
        {
            bitmask |= TECC_ALG_DET;
        }
        else if (strcasecmp(token, "OPE") == 0)
        {
            bitmask |= TECC_ALG_OPE;
        }
        else if (strcasecmp(token, "SEARCH") == 0)
        {
            bitmask |= TECC_ALG_SEARCH;
        }
        else
        {
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE,
                                   "Unknown algorithm in parse_algorithms");
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    pg_free(str_copy);
    return bitmask;
}

/*
 * tecc_algorithms_to_string - 将位掩码转换为字符串
 *
 * bitmask: 算法位掩码
 * 返回: 算法字符串，需要调用者释放
 */
char *
tecc_algorithms_to_string(int bitmask)
{
    char *result;
    size_t len = 0;
    bool first = true;

    /* 计算所需长度 */
    if (bitmask & TECC_ALG_RND)
        len += 4; /* "RND" + 逗号或结束符 */
    if (bitmask & TECC_ALG_DET)
        len += 4;
    if (bitmask & TECC_ALG_OPE)
        len += 4;
    if (bitmask & TECC_ALG_SEARCH)
        len += 7; /* "SEARCH"更长 */

    if (len == 0)
    {
        return pg_strdup("");
    }

    /* 分配内存 */
    result = (char *)pg_malloc(len);
    if (result == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate string in algorithms_to_string");
        return NULL;
    }

    result[0] = '\0';

    /* 构建字符串 */
    if (bitmask & TECC_ALG_RND)
    {
        strcat(result, "RND");
        first = false;
    }

    if (bitmask & TECC_ALG_DET)
    {
        if (!first)
            strcat(result, ",");
        strcat(result, "DET");
        first = false;
    }

    if (bitmask & TECC_ALG_OPE)
    {
        if (!first)
            strcat(result, ",");
        strcat(result, "OPE");
        first = false;
    }

    if (bitmask & TECC_ALG_SEARCH)
    {
        if (!first)
            strcat(result, ",");
        strcat(result, "SEARCH");
    }

    return result;
}

/*
 * tecc_get_encryption_algorithm_name - 获取算法名称
 *
 * alg: 算法枚举值
 * 返回: 算法名称字符串
 */
const char *
tecc_get_encryption_algorithm_name(TeccEncryptionAlgorithm alg)
{
    switch (alg)
    {
        case TECC_ALG_RND:
            return "RND";
        case TECC_ALG_DET:
            return "DET";
        case TECC_ALG_OPE:
            return "OPE";
        case TECC_ALG_SEARCH:
            return "SEARCH";
        default:
            return "UNKNOWN";
    }
}

/*
 * tecc_validate_algorithm_combination - 检查算法组合是否有效
 *
 * bitmask: 算法位掩码
 * 返回: true如果组合有效
 */
bool
tecc_validate_algorithm_combination(int bitmask)
{
    /* 基本验证：至少需要一个算法 */
    if (bitmask == 0)
    {
        return false;
    }

    /* TODO: 添加更复杂的验证规则 */
    /* 例如：某些算法不能组合使用 */

    return true;
}

/*
 * tecc_count_algorithms - 获取算法数量
 *
 * bitmask: 算法位掩码
 * 返回: 算法数量
 */
int
tecc_count_algorithms(int bitmask)
{
    int count = 0;

    if (bitmask & TECC_ALG_RND)
        count++;
    if (bitmask & TECC_ALG_DET)
        count++;
    if (bitmask & TECC_ALG_OPE)
        count++;
    if (bitmask & TECC_ALG_SEARCH)
        count++;

    return count;
}

/*
 * tecc_add_algorithm - 添加算法到位掩码
 *
 * bitmask: 原始位掩码
 * alg: 要添加的算法
 * 返回: 新的位掩码
 */
int
tecc_add_algorithm(int bitmask, TeccEncryptionAlgorithm alg)
{
    return bitmask | alg;
}

/*
 * tecc_remove_algorithm - 从位掩码移除算法
 *
 * bitmask: 原始位掩码
 * alg: 要移除的算法
 * 返回: 新的位掩码
 */
int
tecc_remove_algorithm(int bitmask, TeccEncryptionAlgorithm alg)
{
    return bitmask & ~alg;
}

/*
 * tecc_has_multiple_algorithms - 检查是否包含多个算法
 *
 * bitmask: 算法位掩码
 * 返回: true如果包含多个算法
 */
bool
tecc_has_multiple_algorithms(int bitmask)
{
    return tecc_count_algorithms(bitmask) > 1;
}
/* End - ReqID:TECC4.0 */