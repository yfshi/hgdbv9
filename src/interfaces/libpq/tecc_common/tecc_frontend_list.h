/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_frontend_list.h
 *
 * Abstract:
 *   Frontend list operations for TECC modules.
 *   Provides frontend-compatible list operations that can be used by all TECC modules.
 *   Based on the implementation from frontend_parser/tecc_nodes.h/c.
 *
 * Authored by <wangxp@highgo.com>, 20260416.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common/tecc_frontend_list.h
 */

#ifndef TECC_FRONTEND_LIST_H
#define TECC_FRONTEND_LIST_H

#include "postgres_fe.h"
#include "c.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ==================== 基础类型定义 ==================== */

/* 节点标签类型 - 前端使用int类型 */
typedef int NodeTag;

/* 列表类型枚举 */
typedef enum ListType
{
    T_List,                     /* 指针列表 */
    T_IntList,                  /* 整数列表 */
    T_OidList                   /* OID列表 */
} ListType;

/* 列表单元格 */
typedef struct ListCell
{
    union
    {
        void       *ptr_value;
        int         int_value;
        Oid         oid_value;
    } data;
    struct ListCell *next;
} ListCell;

/* 列表结构 */
typedef struct List
{
    NodeTag     type;           /* 节点类型标识 */
    ListType    list_type;      /* 列表类型 */
    int         length;         /* 列表长度 */
    ListCell   *head;           /* 头指针 */
    ListCell   *tail;           /* 尾指针 */
    struct List *next;          /* 下一个列表（用于嵌套） */
} List;

/* ==================== 宏定义 ==================== */

/* 列表遍历宏 */
#define foreach(cell, lst) \
    for ((cell) = (lst) ? (lst)->head : NULL; \
         (cell) != NULL; \
         (cell) = (cell)->next)

/* 空列表定义 */
#define NIL ((List *) NULL)

/* ==================== 函数声明 ==================== */

/* 内存管理函数 */
extern void *tecc_palloc(size_t size);
extern void tecc_pfree(void *ptr);
extern char *tecc_pstrdup(const char *str);
extern void *pg_calloc(size_t nmemb, size_t size);

/* 列表创建函数 */
extern List *list_make1(void *datum);
extern List *list_make2(void *datum1, void *datum2);
extern List *list_make3(void *datum1, void *datum2, void *datum3);

/* 列表操作函数 */
extern List *lappend(List *list, void *datum);
extern List *lappend_int(List *list, int datum);
extern List *lappend_oid(List *list, Oid datum);
extern List *list_delete_nth_cell(List *list, int n);
extern void list_free(List *list);
extern void list_free_deep(List *list);
extern int list_length(const List *list);
extern void *list_nth(const List *list, int n);
extern void *lfirst(const ListCell *cell);
extern void *llast(const List *list);
extern void *linitial(const List *list);

/* 注意：strVal和makeStringConst函数在tecc_nodes.h中定义
 * 这些函数是前端解析器特有的，这里不重复声明
 */

#endif /* TECC_FRONTEND_LIST_H */
/* End - ReqID:TECC4.0 */