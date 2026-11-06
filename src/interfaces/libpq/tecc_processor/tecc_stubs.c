/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_stubs.c
 * Abstract:
 *   Stub implementations for TECC frontend parser functions.
 *   Provides minimal implementations to allow compilation while
 *   frontend parser is not fully implemented.
 *
 * Authored by <wangxp@highgo.com>, 20260405.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_stubs.c
 */

#include "postgres_fe.h"
#include "libpq-fe.h"
#include "frontend_parser/fe_adapter.h"
#include "frontend_parser/tecc_nodes.h"
#include <stdbool.h>

/* 声明tecc_raw_parser函数 */
List *tecc_raw_parser(const char *str);

#ifdef __cplusplus
extern "C" {
#endif

/* Stub implementation for tecc_raw_parser - moved to fe_adapter.c */
#if 0
List *tecc_raw_parser(const char *str)
{
    /* Return empty list for now */
    return NIL;
}
#endif

#if 0
/* Implementation for tecc_fe_raw_parser */
static List *tecc_fe_raw_parser(const char *str)
{
    /* 调用前端解析器的tecc_raw_parser函数 */
    return tecc_raw_parser(str);
}
#endif

#ifdef __cplusplus
}
#endif

/* End - ReqID:TECC4.0 */