/*
 * File: test_normalizer_basic.c
 * Abstract: 规范化接口层基础测试程序
 * Authored by <wangxp@highgo.com>, 20260312.
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 * Identification: src/test/modules/test_normalizer/test_normalizer_basic.c
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

/* ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 */

/* 简单的测试函数，用于验证模块加载 */
PG_FUNCTION_INFO_V1(test_normalizer_all);
Datum
test_normalizer_all(PG_FUNCTION_ARGS)
{
    PG_RETURN_TEXT_P(cstring_to_text("TECC normalizer test passed"));
}

/* End - ReqID:TECC4.0 */