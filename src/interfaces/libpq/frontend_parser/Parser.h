/*
 * Copyright (c) 2020 Huawei Technologies Co.,Ltd.
 *
 * openGauss is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 *
 * Parser.h
 *
 * IDENTIFICATION
 *	  src\common\interfaces\libpq\frontend_parser\Parser.h
 *
 * -------------------------------------------------------------------------
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 前端解析器函数声明 */
typedef struct PGClientLogic PGClientLogic;

extern void *fe_raw_parser(PGClientLogic *client_logic, const char *str);
extern void *fe_parser_parse(PGClientLogic *client_logic, const char *query);
extern char **get_next_snippet(char **query_string_single, const char *query_string,
                              const void *query_string_locationlist, int *stmt_num);

#ifdef __cplusplus
}
#endif
