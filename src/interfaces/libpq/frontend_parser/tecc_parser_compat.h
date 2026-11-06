/*
 * 兼容性头文件
 * 将简化接口映射到原始接口
 */

#ifndef TECC_PARSER_COMPAT_H
#define TECC_PARSER_COMPAT_H

#include "tecc_parser_simple.h"

/* 兼容性定义 */
#define TeccParseResult void
#define ParserError void
#define LocationInfo void

/* 兼容性函数声明 */
static inline void tecc_parser_init_result(void *result) {
    /* 简化实现：什么都不做 */
    (void)result;
}

static inline void tecc_parser_free_result(void *result) {
    /* 简化实现：什么都不做 */
    (void)result;
}

#endif /* TECC_PARSER_COMPAT_H */