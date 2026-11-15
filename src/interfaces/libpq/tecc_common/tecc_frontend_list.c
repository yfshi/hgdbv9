/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_frontend_list.c
 *
 * Abstract:
 *   Frontend list operations implementation for TECC modules.
 *   Provides frontend-compatible list operations that can be used by all TECC modules.
 *   Based on the implementation from frontend_parser/tecc_nodes.c.
 *
 * Authored by <wangxp@highgo.com>, 20260416.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common/tecc_frontend_list.c
 */

#include "tecc_frontend_list.h"
#include "../frontend_parser/tecc_nodes.h"  /* 包含freeNode声明 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* ==================== 内存管理函数 ==================== */

/* 前端环境使用标准C函数 */
void *tecc_palloc(size_t size) {
    return malloc(size);
}

/* pg_calloc的替代实现 */
void *pg_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void tecc_pfree(void *ptr) {
    free(ptr);
}

char *tecc_pstrdup(const char *str) {
    size_t len;
    char *new_str;

    if (!str) return NULL;
    len = strlen(str) + 1;
    new_str = (char *)tecc_palloc(len);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}

/* ==================== 列表操作函数 ==================== */

List *list_make1(void *datum) {
    List *list;
    ListCell *cell;

    list = (List *)tecc_palloc(sizeof(List));
    if (!list) return NULL;

    list->type = T_List;
    list->list_type = T_List;
    list->length = 1;

    cell = (ListCell *)tecc_palloc(sizeof(ListCell));
    if (!cell) {
        tecc_pfree(list);
        return NULL;
    }

    cell->data.ptr_value = datum;
    cell->next = NULL;

    list->head = cell;
    list->tail = cell;
    list->next = NULL;

    return list;
}

List *list_make2(void *datum1, void *datum2) {
    List *list = list_make1(datum1);
    if (!list) return NULL;

    return lappend(list, datum2);
}

List *list_make3(void *datum1, void *datum2, void *datum3) {
    List *list = list_make2(datum1, datum2);
    if (!list) return NULL;

    return lappend(list, datum3);
}

List *lappend(List *list, void *datum) {
    ListCell *new_cell;

    if (!list) {
        return list_make1(datum);
    }

    new_cell = (ListCell *)tecc_palloc(sizeof(ListCell));
    if (!new_cell) return list;

    new_cell->data.ptr_value = datum;
    new_cell->next = NULL;

    /* 更新尾指针 */
    if (list->tail) {
        list->tail->next = new_cell;
    } else {
        list->head = new_cell;
    }
    list->tail = new_cell;
    list->length++;

    return list;
}

List *lappend_int(List *list, int datum) {
    ListCell *new_cell;

    if (!list) {
        list = (List *)tecc_palloc(sizeof(List));
        if (!list) return NULL;

        list->type = T_List;
        list->list_type = T_IntList;
        list->length = 0;
        list->head = NULL;
        list->tail = NULL;
        list->next = NULL;
    }

    new_cell = (ListCell *)tecc_palloc(sizeof(ListCell));
    if (!new_cell) return list;

    new_cell->data.int_value = datum;
    new_cell->next = NULL;

    /* 更新尾指针 */
    if (list->tail) {
        list->tail->next = new_cell;
    } else {
        list->head = new_cell;
    }
    list->tail = new_cell;
    list->length++;

    return list;
}

List *lappend_oid(List *list, Oid datum) {
    ListCell *new_cell;

    if (!list) {
        list = (List *)tecc_palloc(sizeof(List));
        if (!list) return NULL;

        list->type = T_List;
        list->list_type = T_OidList;
        list->length = 0;
        list->head = NULL;
        list->tail = NULL;
        list->next = NULL;
    }

    new_cell = (ListCell *)tecc_palloc(sizeof(ListCell));
    if (!new_cell) return list;

    new_cell->data.oid_value = datum;
    new_cell->next = NULL;

    /* 更新尾指针 */
    if (list->tail) {
        list->tail->next = new_cell;
    } else {
        list->head = new_cell;
    }
    list->tail = new_cell;
    list->length++;

    return list;
}

void list_free(List *list) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    ListCell *cell;

    if (!list) return;

    cell = list->head;
    while (cell) {
        ListCell *next = cell->next;
        tecc_pfree(cell);
        cell = next;
    }

    tecc_pfree(list);
}

void list_free_deep(List *list) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    ListCell *cell;

    if (!list) return;

    cell = list->head;
    while (cell) {
        ListCell *next = cell->next;
        /* 释放元素数据 */
        if (cell->data.ptr_value) {
            /* 检查指针是否看起来有效 */
            if ((uintptr_t)cell->data.ptr_value < 0x1000) {
                /* 小地址，可能是无效指针 - 记录但不释放 */
            } else {
                /* 对于前端解析器，我们不知道指针指向的是什么类型
                 * 最安全的方法是假设它是字符串指针，直接释放 */
                tecc_pfree(cell->data.ptr_value);
            }
        }
        tecc_pfree(cell);
        cell = next;
    }

    tecc_pfree(list);
}

int list_length(const List *list) {
    return list ? list->length : 0;
}

/*
 * Delete the nth cell (zero-based) from list.
 * Returns the modified list, or NIL if list becomes empty.
 */
List *list_delete_nth_cell(List *list, int n)
{
    ListCell *prev_cell;
    ListCell *cell_to_delete;
    int i;

    if (!list || n < 0 || n >= list->length)
        return list;

    /* If deleting the only cell, free the whole list */
    if (list->length == 1)
    {
        list_free(list);
        return NIL;
    }

    /* Find the cell to delete */
    prev_cell = NULL;
    cell_to_delete = list->head;
    for (i = 0; i < n; i++)
    {
        prev_cell = cell_to_delete;
        cell_to_delete = cell_to_delete->next;
    }

    /* Remove the cell from the list */
    if (prev_cell)
    {
        prev_cell->next = cell_to_delete->next;
    }
    else
    {
        /* Deleting the first cell */
        list->head = cell_to_delete->next;
    }

    /* Update tail if deleting the last cell */
    if (cell_to_delete == list->tail)
    {
        list->tail = prev_cell;
    }

    /* Free the cell */
    tecc_pfree(cell_to_delete);

    /* Update list length */
    list->length--;

    return list;
}

void *list_nth(const List *list, int n) {
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    ListCell *cell;
    int i;

    if (!list || n < 0 || n >= list->length) {
        return NULL;
    }

    cell = list->head;
    for (i = 0; i < n; i++) {
        if (!cell) break;
        cell = cell->next;
    }

    return cell ? cell->data.ptr_value : NULL;
}

/* ReqID:TECC4.0 - 返回第 n 个 ListCell 指针（对齐后端 list_nth_cell 语义；
 * 越界返回 NULL）。stmt_processor.c 的 ORDER BY 列名提取依赖此函数。 */
ListCell *list_nth_cell(const List *list, int n) {
    ListCell *cell;
    int i;

    if (!list || n < 0 || n >= list->length) {
        return NULL;
    }

    cell = list->head;
    for (i = 0; i < n; i++) {
        if (!cell) break;
        cell = cell->next;
    }

    return cell;
}

void *lfirst(const ListCell *cell) {
    return cell ? cell->data.ptr_value : NULL;
}

/* Get the first element of a list */
void *linitial(const List *list) {
    if (!list || !list->head) return NULL;
    return list->head->data.ptr_value;
}

/* Get the last element of a list */
void *llast(const List *list) {
    if (!list || !list->tail) return NULL;
    return list->tail->data.ptr_value;
}

/* ==================== 字符串值提取函数 ==================== */

/* strVal和makeStringConst函数现在在tecc_nodes.c中定义 */
/* 这里不再定义，以避免链接冲突 */
/* End - ReqID:TECC4.0 */