/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_logging.c
 *
 * Abstract:
 *   TECC通用日志工具实现
 *
 * Authored by <wangxp@highgo.com>, 20260419.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common/tecc_logging.c
 */

#include "tecc_logging.h"
#ifdef _WIN32
#include <port/win32/pthread.h>
#else
#include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* 全局日志级别缓存和初始化控制 */
static pthread_once_t tecc_log_level_once = PTHREAD_ONCE_INIT;
static int tecc_log_level_cache = -1;  /* -1表示未初始化 */

/* 解析日志级别字符串 */
int
tecc_parse_log_level(const char *level_str)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char lower[32];
    int i;

    if (level_str == NULL)
        return TECC_LOG_DEFAULT_LEVEL;

    /* 转换为小写以便比较 */
    for (i = 0; level_str[i] && i < sizeof(lower) - 1; i++) {
        lower[i] = tolower((unsigned char)level_str[i]);
    }
    lower[i] = '\0';

    /* 数字级别 */
    if (isdigit((unsigned char)level_str[0])) {
        int level = atoi(level_str);
        if (level >= TECC_LOG_DEBUG5 && level <= TECC_LOG_PANIC)
            return level;
        return TECC_LOG_DEFAULT_LEVEL;
    }

    /* 字符串级别 */
    if (strcmp(lower, "panic") == 0) return TECC_LOG_PANIC;
    if (strcmp(lower, "fatal") == 0) return TECC_LOG_FATAL;
    if (strcmp(lower, "error") == 0) return TECC_LOG_ERROR;
    if (strcmp(lower, "warning") == 0) return TECC_LOG_WARNING;
    if (strcmp(lower, "notice") == 0) return TECC_LOG_NOTICE;
    if (strcmp(lower, "info") == 0) return TECC_LOG_INFO;
    if (strcmp(lower, "log") == 0) return TECC_LOG_LOG;
    if (strcmp(lower, "debug1") == 0) return TECC_LOG_DEBUG1;
    if (strcmp(lower, "debug2") == 0) return TECC_LOG_DEBUG2;
    if (strcmp(lower, "debug3") == 0) return TECC_LOG_DEBUG3;
    if (strcmp(lower, "debug4") == 0) return TECC_LOG_DEBUG4;
    if (strcmp(lower, "debug5") == 0) return TECC_LOG_DEBUG5;
    if (strcmp(lower, "debug") == 0) return TECC_LOG_DEBUG1; /* 默认DEBUG1 */

    return TECC_LOG_DEFAULT_LEVEL;
}

/* 日志级别初始化函数（只调用一次） */
static void
tecc_init_log_level(void)
{
    const char *env_level = getenv(TECC_LOG_LEVEL_ENV);
    tecc_log_level_cache = tecc_parse_log_level(env_level);
}

/* 获取当前日志级别（完全线程安全版本） */
int
tecc_get_log_level(void)
{
    /* 使用pthread_once确保只初始化一次
     * 这是完全线程安全的初始化模式
     */
    pthread_once(&tecc_log_level_once, tecc_init_log_level);
    return tecc_log_level_cache;
}

/* 日志级别转字符串 */
const char *
tecc_log_level_to_string(int level)
{
    switch (level) {
        case TECC_LOG_PANIC:   return "PANIC";
        case TECC_LOG_FATAL:   return "FATAL";
        case TECC_LOG_ERROR:   return "ERROR";
        case TECC_LOG_WARNING: return "WARNING";
        case TECC_LOG_NOTICE:  return "NOTICE";
        case TECC_LOG_INFO:    return "INFO";
        case TECC_LOG_LOG:     return "LOG";
        case TECC_LOG_DEBUG1:  return "DEBUG1";
        case TECC_LOG_DEBUG2:  return "DEBUG2";
        case TECC_LOG_DEBUG3:  return "DEBUG3";
        case TECC_LOG_DEBUG4:  return "DEBUG4";
        case TECC_LOG_DEBUG5:  return "DEBUG5";
        default:               return "UNKNOWN";
    }
}

/* 判断是否应该记录日志 */
bool
tecc_should_log(int current_level, int configured_level)
{
    /* ERROR及以上级别总是记录 */
    if (current_level >= TECC_LOG_ERROR)
        return true;

    /* 其他级别：当前级别 >= 配置级别时记录 */
    return current_level >= configured_level;
}

/* 重置日志级别缓存（主要用于测试） */
void
tecc_reset_log_level_cache(void)
{
    /* 注意：这不是线程安全的，仅用于测试
     * 在生产环境中不应调用此函数
     */
    tecc_log_level_cache = -1;
    memset(&tecc_log_level_once, 0, sizeof(tecc_log_level_once));
}
/* End - ReqID:TECC4.0 */