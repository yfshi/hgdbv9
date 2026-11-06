/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_logging.h
 *
 * Abstract:
 *   TECC通用日志工具，提供与PostgreSQL后端一致的日志级别和环境变量控制
 *   完全线程安全，支持多连接并发访问
 *
 * Authored by <wangxp@highgo.com>, 20260419.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common/tecc_logging.h
 */

#ifndef TECC_LOGGING_H
#define TECC_LOGGING_H

#include <stdbool.h>
#include <stdio.h>

/* Include PostgreSQL frontend headers for pg_fprintf */
#include "postgres_fe.h"

/* PostgreSQL兼容的日志级别定义 */
#define TECC_LOG_PANIC    23  /* 系统崩溃级错误 */
#define TECC_LOG_FATAL    22  /* 致命错误，终止进程 */
#define TECC_LOG_ERROR    21  /* 用户错误，终止事务 */
#define TECC_LOG_WARNING  19  /* 警告信息 */
#define TECC_LOG_NOTICE   18  /* 通知信息 */
#define TECC_LOG_INFO     17  /* 信息性消息 */
#define TECC_LOG_LOG      15  /* 服务器操作消息 */
#define TECC_LOG_DEBUG1   14  /* 最上层调试信息 */
#define TECC_LOG_DEBUG2   13  /* 上层调试信息 */
#define TECC_LOG_DEBUG3   12  /* 中层调试信息 */
#define TECC_LOG_DEBUG4   11  /* 下层调试信息 */
#define TECC_LOG_DEBUG5   10  /* 最底层详细调试 */

/* 环境变量名称 */
#define TECC_LOG_LEVEL_ENV "TECC_LOG_LEVEL"

/* 默认日志级别（只显示错误） */
#define TECC_LOG_DEFAULT_LEVEL TECC_LOG_ERROR

/* 函数原型 */
extern int tecc_get_log_level(void);
extern int tecc_parse_log_level(const char *level_str);
extern const char *tecc_log_level_to_string(int level);
extern bool tecc_should_log(int current_level, int configured_level);
extern void tecc_reset_log_level_cache(void);

/* 通用日志宏（不含文件名和行号，避免行号偏移影响回归测试） */
#define TECC_LOG(level, ...) \
    do { \
        if (tecc_should_log((level), tecc_get_log_level())) { \
            pg_fprintf(stderr, "TECC %s: ", \
                    tecc_log_level_to_string(level)); \
            pg_fprintf(stderr, __VA_ARGS__); \
            pg_fprintf(stderr, "\n"); \
        } \
    } while (0)

/* 简化日志宏（不含文件名和行号，用于SAFETY CHECK等确定性消息） */
#define TECC_LOG_MSG(level, ...) \
    do { \
        if (tecc_should_log((level), tecc_get_log_level())) { \
            pg_fprintf(stderr, "TECC %s: ", \
                    tecc_log_level_to_string(level)); \
            pg_fprintf(stderr, __VA_ARGS__); \
            pg_fprintf(stderr, "\n"); \
        } \
    } while (0)

#endif /* TECC_LOGGING_H */
/* End - ReqID:TECC4.0 */