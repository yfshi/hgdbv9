/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_metadata_query.c
 *    TECC metadata query parsing implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_metadata_query.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_metadata_query.h"
#include "tecc_error_handler.h"
#include "tecc_key_args_parser.h"
/* ReqID:TECC4.0 - include temporarily disabled, header not yet created */
/* #include "tecc_icu_collation.h" */

/* libpq函数声明 */
#include "libpq-fe.h"

/* 数据获取器函数声明 */
#include "tecc_data_fetcher.h"

/* 前端列表操作函数声明 */
#include "../frontend_parser/tecc_nodes.h"

/* 元数据缓存结构定义 */
#include "../../tecc_cache/tecc_metadata_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* bytea解码函数声明 */
#include "tecc_bytea_decoder.h"

/*
 * tecc_parse_kek_fields_from_current_row - 解析游标当前行的KEK字段（不推进游标）
 *
 * hg_kek_info 的每一行都带完整的KEK字段（列0-6）与一个参数（列7-9），
 * 因此 KEK 列表的按 kek_oid 分组必须由调用方在"行"这一层完成：
 * 由本函数取出当前行的字段，再由调用方决定它归属哪个 KEK 对象。
 *
 * cursor: 查询游标（当前行）
 * result: 输出KEK元数据指针（仅字段，args留空由调用方追加）
 * 返回: 错误码
 */
static TeccFetcherErrorCode
tecc_parse_kek_fields_from_current_row(TeccCursorInterface *cursor, TeccKekMetadata **result)
{
    TeccKekMetadata *kek = NULL;
    const char *value;
    time_t create_time;

    *result = NULL;

    /* 分配内存 */
    kek = (TeccKekMetadata *)pg_malloc(sizeof(TeccKekMetadata));
    if (kek == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 初始化结构 */
    memset(kek, 0, sizeof(TeccKekMetadata));

    /* 调试：打印所有列的值 */
    {
        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        int column_count = cursor->get_column_count(cursor);
        int i;
        TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Total columns: %d", column_count);
        for (i = 0; i < column_count; i++) {
            const char *col_value = cursor->get_column(cursor, i);
            TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Column %d: '%s'", i, col_value ? col_value : "(NULL)");
        }
    }

    value = cursor->get_column(cursor, 0); /* kek_oid */
    if (value != NULL)
    {
        unsigned int oid_value;
        TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Column 0 (kek_oid) value: '%s'", value);
        if (sscanf(value, "%u", &oid_value) == 1)
        {
            kek->kek_oid = (Oid)oid_value;
            TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Parsed kek_oid: %u from value: %s", kek->kek_oid, value);
        }
        else
        {
            TECC_LOG(TECC_LOG_WARNING, "tecc_parse_kek_metadata: Failed to parse kek_oid from value: %s", value);
        }
    }
    else
    {
        TECC_LOG(TECC_LOG_WARNING, "tecc_parse_kek_metadata: kek_oid column value is NULL");
    }

    value = cursor->get_column(cursor, 1); /* key_name */
    if (value != NULL)
        kek->key_name = pg_strdup(value);

    value = cursor->get_column(cursor, 2); /* key_namespace */
    if (value != NULL)
        kek->key_namespace = pg_strdup(value);

    value = cursor->get_column(cursor, 3); /* key_owner_oid */
    if (value != NULL)
    {
        unsigned int oid_value;
        if (sscanf(value, "%u", &oid_value) == 1)
        {
            kek->key_owner_oid = (Oid)oid_value;
        }
    }

    value = cursor->get_column(cursor, 4); /* key_owner_name */
    if (value != NULL)
        kek->key_owner_name = pg_strdup(value);

    value = cursor->get_column(cursor, 5); /* key_acl */
    if (value != NULL)
        kek->key_acl = pg_strdup(value);

    value = cursor->get_column(cursor, 6); /* create_date */
    if (value != NULL)
    {
        /* 解析时间戳 */
        if (sscanf(value, "%ld", &create_time) == 1)
        {
            kek->create_date = create_time;
        }
    }

    /* 设置结果（args留空，由调用方按行追加） */
    *result = kek;

    return TECC_FETCHER_OK;
}

/*
 * tecc_kek_append_arg_from_current_row - 把游标当前行的参数追加到KEK（不推进游标）
 *
 * 对应 hg_kek_info 的列7-9（function_name / arg_key / arg_value）。
 * 列数<10（无参数的批量查询形态）时不做任何事。
 *
 * cursor: 查询游标（当前行）
 * kek: 目标KEK元数据
 */
static void
tecc_kek_append_arg_from_current_row(TeccCursorInterface *cursor, TeccKekMetadata *kek)
{
    const char *function_name;
    const char *arg_key;
    const char *arg_value;
    TeccKeyArg *arg;

    if (cursor->get_column_count(cursor) < 10)
    {
        return;
    }

    function_name = cursor->get_column(cursor, 7);
    arg_key = cursor->get_column(cursor, 8);
    arg_value = cursor->get_column(cursor, 9);

    if (function_name == NULL || arg_key == NULL)
    {
        return;
    }

    if (kek->args == NULL)
    {
        kek->args = (TeccKeyArg *)pg_malloc(sizeof(TeccKeyArg));
    }
    else
    {
        kek->args = (TeccKeyArg *)repalloc(kek->args,
            sizeof(TeccKeyArg) * (kek->args_count + 1));
    }
    if (kek->args == NULL)
    {
        return;
    }

    arg = &kek->args[kek->args_count];
    memset(arg, 0, sizeof(TeccKeyArg));
    arg->function_name = pg_strdup(function_name);
    arg->key = pg_strdup(arg_key);
    arg->value = (arg_value != NULL) ? decode_bytea_value(arg_value) : NULL;
    kek->args_count++;
}

/*
 * tecc_parse_kek_metadata - 解析KEK元数据
 *
 * 用于按名称/按OID取单个KEK的查询：从游标当前推进一行开始，读取该KEK的字段，
 * 并收集属于同一 kek_oid 的所有参数行。多KEK的列表查询不走本函数，见
 * tecc_parse_kek_list（那里必须逐行分组，不能靠"读到别的KEK就停"来划分）。
 *
 * state: TECC状态
 * cursor: 查询游标
 * result: 输出KEK元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_parse_kek_metadata(const TeccState *state, TeccCursorInterface *cursor, TeccKekMetadata **result)
{
    TeccFetcherErrorCode error_code;
    TeccKekMetadata *kek = NULL;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Before validation, column count: %d", cursor->get_column_count(cursor));
    if (!tecc_validate_kek_result(cursor))
    {
        TECC_LOG(TECC_LOG_ERROR, "tecc_parse_kek_metadata: KEK result validation failed, column count: %d", cursor->get_column_count(cursor));
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 获取第一行数据 */
    TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Starting to parse KEK metadata, calling cursor->next()");
    if (cursor->next(cursor) != TECC_FETCHER_OK) {
        TECC_LOG(TECC_LOG_WARNING, "tecc_parse_kek_metadata: No rows returned from query");
        return TECC_FETCHER_ERROR_PARSE;
    }
    TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_metadata: Successfully fetched first row");

    error_code = tecc_parse_kek_fields_from_current_row(cursor, &kek);
    if (error_code != TECC_FETCHER_OK)
    {
        return error_code;
    }

    /* Begin - ReqID:TECC4.0 */
    /* 收集本KEK的参数行：首行也要取（当前行即该KEK的第一行），之后逐行推进，
     * 遇到 kek_oid 变化的行即停止且不消费它——单KEK查询不会有这种行，此处仅
     * 防御查询意外返回了其他KEK（多KEK列表由 tecc_parse_kek_list 处理）。 */
    tecc_kek_append_arg_from_current_row(cursor, kek);
    while (cursor->next(cursor) == TECC_FETCHER_OK)
    {
        const char *oid_str = cursor->get_column(cursor, 0);
        unsigned int row_oid = 0;

        if (oid_str != NULL)
            sscanf(oid_str, "%u", &row_oid);
        if (row_oid != 0 && kek->kek_oid != 0 && row_oid != kek->kek_oid)
            break;  /* 下一个KEK的行，停止（不消费此行） */

        tecc_kek_append_arg_from_current_row(cursor, kek);
    }
    /* End - ReqID:TECC4.0 */

    /* 设置结果 */
    *result = kek;

    return TECC_FETCHER_OK;
}

/*
 * tecc_parse_dek_metadata - 解析DEK元数据
 *
 * cursor: 查询游标
 * result: 输出DEK元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_parse_dek_metadata(const TeccState *state, TeccCursorInterface *cursor, TeccDekMetadata **result)
{
    TeccDekMetadata *dek = NULL;
    const char *value;
    time_t create_time;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    if (!tecc_validate_dek_result(cursor))
    {
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 分配内存 */
    dek = (TeccDekMetadata *)pg_malloc(sizeof(TeccDekMetadata));
    if (dek == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 初始化结构 */
    memset(dek, 0, sizeof(TeccDekMetadata));

    /* 获取第一行数据（参照 tecc_parse_kek_metadata 的cursor推进模式） */
    if (cursor->next(cursor) != TECC_FETCHER_OK)
    {
        pg_free(dek);
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 解析字段 */
    value = cursor->get_column(cursor, 0); /* dek_oid */
    if (value != NULL)
    {
        /* 调试日志 */
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG: Parsing dek_oid from column 0: '%s'", value);
        sscanf(value, "%u", &dek->dek_oid);
    }
    else
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG: Column 0 (dek_oid) is NULL");
    }

    value = cursor->get_column(cursor, 1); /* key_name */
    if (value != NULL)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG: Parsing key_name from column 1: '%s'", value);
        dek->key_name = pg_strdup(value);
    }
    else
    {
        TECC_LOG(TECC_LOG_DEBUG1, "DEBUG: Column 1 (key_name) is NULL");
    }

    value = cursor->get_column(cursor, 2); /* column_key_distributed_id */
    if (value != NULL)
        sscanf(value, "%u", &dek->column_key_distributed_id);

    value = cursor->get_column(cursor, 3); /* kek_oid */
    if (value != NULL)
        sscanf(value, "%u", &dek->kek_oid);

    value = cursor->get_column(cursor, 4); /* kek_name */
    if (value != NULL)
        dek->kek_name = pg_strdup(value);

    value = cursor->get_column(cursor, 5); /* key_namespace */
    if (value != NULL)
        dek->key_namespace = pg_strdup(value);

    value = cursor->get_column(cursor, 6); /* key_owner_oid */
    if (value != NULL)
    {
        unsigned int oid_value;
        if (sscanf(value, "%u", &oid_value) == 1)
        {
            dek->key_owner_oid = (Oid)oid_value;
        }
    }

    value = cursor->get_column(cursor, 7); /* key_owner_name */
    if (value != NULL)
        dek->key_owner_name = pg_strdup(value);

    value = cursor->get_column(cursor, 8); /* key_acl */
    if (value != NULL)
        dek->key_acl = pg_strdup(value);

    value = cursor->get_column(cursor, 9); /* create_date */
    if (value != NULL)
    {
        if (sscanf(value, "%ld", &create_time) == 1)
        {
            dek->create_date = create_time;
        }
    }

    /* 解析DEK参数（含encrypted_key）。
     * tecc_parse_dek_args内部处理两种模式：
     *   列数>=13：返回NULL，由下面的do-while处理（含OID边界检查）
     *   列数<13：单独执行SQL查询按名称加载args */
    dek->args = tecc_parse_dek_args(state, cursor, &dek->args_count);

    /* 如果列数>=13（批量查询含args），逐行收集当前DEK的所有args。
     * 仿照 tecc_parse_kek_metadata 的 do-while 模式，增加OID边界检查：
     * 遇到不同dek_oid的行时break且不消费，由 tecc_parse_dek_list 外层while处理。 */
    if (cursor->get_column_count(cursor) >= 13)
    {
        List *args_list = NIL;
        int args_count = 0;

        do {
            const char *function_name;
            const char *arg_key;
            const char *arg_value;
            unsigned int row_oid = 0;
            const char *oid_str;

            /* 检查是否还在同一DEK（比较第0列dek_oid） */
            oid_str = cursor->get_column(cursor, 0);
            if (oid_str != NULL)
                sscanf(oid_str, "%u", &row_oid);
            if (row_oid != 0 && dek->dek_oid != 0 && row_oid != dek->dek_oid)
                break;  /* 下一个DEK的行，停止（不消费此行） */

            function_name = cursor->get_column(cursor, 10);
            arg_key = cursor->get_column(cursor, 11);
            arg_value = cursor->get_column(cursor, 12);

            if (function_name != NULL && arg_key != NULL)
            {
                TeccKeyArg *arg = (TeccKeyArg *)pg_malloc(sizeof(TeccKeyArg));
                if (arg != NULL)
                {
                    memset(arg, 0, sizeof(TeccKeyArg));
                    arg->function_name = pg_strdup(function_name);
                    arg->key = pg_strdup(arg_key);
                    arg->value = (arg_value != NULL) ? decode_bytea_value(arg_value) : NULL;
                    args_list = lappend(args_list, arg);
                    args_count++;
                }
            }
        } while (cursor->next(cursor) == TECC_FETCHER_OK);

        /* 将列表转换为数组（与KEK端逻辑一致） */
        if (args_count > 0)
        {
            ListCell *cell;
            int i;

            dek->args = (TeccKeyArg *)pg_malloc(sizeof(TeccKeyArg) * args_count);
            if (dek->args != NULL)
            {
                i = 0;
                foreach(cell, args_list)
                {
                    TeccKeyArg *arg = (TeccKeyArg *)lfirst(cell);
                    dek->args[i] = *arg;
                    i++;
                }
                dek->args_count = args_count;
            }

            /* 释放列表元素（数组已复制数据） */
            foreach(cell, args_list)
            {
                TeccKeyArg *arg = (TeccKeyArg *)lfirst(cell);
                pg_free(arg);
            }
        }
        if (args_list != NIL)
            list_free(args_list);
    }

    /* 设置结果 */
    *result = dek;

    return TECC_FETCHER_OK;
}

/*
 * tecc_parse_encrypted_column_metadata - 解析加密列元数据
 *
 * cursor: 查询游标
 * result: 输出加密列元数据指针
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_parse_encrypted_column_metadata(TeccCursorInterface *cursor,
                                     TeccEncryptedColumnMetadata **result)
{
    TeccEncryptedColumnMetadata *column = NULL;
    const char *value;
    time_t create_time;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    if (!tecc_validate_column_result(cursor))
    {
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 分配内存 */
    column = (TeccEncryptedColumnMetadata *)pg_malloc(sizeof(TeccEncryptedColumnMetadata));
    if (column == NULL)
    {
        return TECC_FETCHER_ERROR_MEMORY;
    }

    /* 初始化结构 */
    memset(column, 0, sizeof(TeccEncryptedColumnMetadata));

    /* 解析字段 */
    value = cursor->get_column(cursor, 0); /* table_oid */
    if (value != NULL)
        sscanf(value, "%u", &column->table_oid);

    value = cursor->get_column(cursor, 1); /* table_name */
    if (value != NULL)
        column->table_name = pg_strdup(value);

    value = cursor->get_column(cursor, 2); /* column_attnum */
    if (value != NULL)
        sscanf(value, "%hd", &column->column_attnum);

    value = cursor->get_column(cursor, 3); /* column_name */
    if (value != NULL)
        column->column_name = pg_strdup(value);

    value = cursor->get_column(cursor, 4); /* data_type_original_oid */
    if (value != NULL)
        sscanf(value, "%u", &column->data_type_original_oid);

    value = cursor->get_column(cursor, 5); /* data_type_original_mod */
    if (value != NULL)
        sscanf(value, "%d", &column->data_type_original_mod);

    value = cursor->get_column(cursor, 6); /* dek_name */
    if (value != NULL)
        column->dek_name = pg_strdup(value);

    value = cursor->get_column(cursor, 7); /* encryption_type */
    if (value != NULL)
        sscanf(value, "%d", &column->encryption_type);

    value = cursor->get_column(cursor, 8); /* collation_locale */
    if (value != NULL && strlen(value) > 0)
    {
        column->collation_locale = pg_strdup(value);
        /* 设置collation_oid为InvalidOid，因为前端不需要OID（locale已足够） */
        column->collation_oid = InvalidOid;
    }
    else
    {
        /* 如果没有排序规则locale，设置为NULL */
        column->collation_locale = NULL;
        column->collation_oid = InvalidOid;
    }

    value = cursor->get_column(cursor, 9); /* column_key_id */
    if (value != NULL)
        sscanf(value, "%u", &column->column_key_id);

    value = cursor->get_column(cursor, 10); /* create_date */
    if (value != NULL)
    {
        if (sscanf(value, "%ld", &create_time) == 1)
        {
            column->create_date = create_time;
        }
    }

    /* ReqID:TECC4.0 - Parse DEK permission from query */
    value = cursor->get_column(cursor, 11); /* has_dek_permission */
    if (value != NULL && strcmp(value, "t") == 0)
        column->has_dek_permission = true;
    else
        column->has_dek_permission = false;

    /* ReqID:TECC4.0 - 一列多 DEK：HE 加法/乘法 DEK 名（纯 HE 列 dek_name 为 NULL） */
    value = cursor->get_column(cursor, 12); /* he_add_dek */
    if (value != NULL && strlen(value) > 0)
        column->he_add_dek = pg_strdup(value);
    value = cursor->get_column(cursor, 13); /* he_mul_dek */
    if (value != NULL && strlen(value) > 0)
        column->he_mul_dek = pg_strdup(value);

    /* 设置creator字段为InvalidOid（hg_encrypted_columns表没有creator字段） */
    column->creator = InvalidOid;

    /* 设置结果 */
    *result = column;

    return TECC_FETCHER_OK;
}


/*
 * tecc_validate_kek_result - 验证KEK查询结果结构
 *
 * cursor: 查询游标
 * 返回: true如果结果结构有效
 */
bool
tecc_validate_kek_result(TeccCursorInterface *cursor)
{
    int column_count;

    if (cursor == NULL)
    {
        return false;
    }

    column_count = cursor->get_column_count(cursor);

    /* KEK查询可能返回7列（批量查询）或10列（单个查询包含参数） */
    TECC_LOG(TECC_LOG_DEBUG1, "tecc_validate_kek_result: Column count: %d", column_count);
    if (column_count != 7 && column_count != 10)
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "Invalid column count for KEK query: expected 7, 6 or 9, got %d", column_count);
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE, error_msg);
        TECC_LOG(TECC_LOG_ERROR, "tecc_validate_kek_result: %s", error_msg);
        return false;
    }

    /* 验证列名（可选，但推荐） */
    /* 这里可以添加列名验证逻辑 */

    return true;
}

/*
 * tecc_validate_dek_result - 验证DEK查询结果结构
 *
 * cursor: 查询游标
 * 返回: true如果结果结构有效
 */
bool
tecc_validate_dek_result(TeccCursorInterface *cursor)
{
    int column_count;

    if (cursor == NULL)
    {
        return false;
    }

    column_count = cursor->get_column_count(cursor);

    /* DEK查询可能返回10列（批量查询）或13列（单个查询包含参数） */
    if (column_count != 10 && column_count != 13)
    {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
                 "Invalid column count for DEK query: expected 10, 9 or 12, got %d", column_count);
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE, error_msg);
        return false;
    }

    return true;
}

/*
 * tecc_validate_column_result - 验证加密列查询结果结构
 *
 * cursor: 查询游标
 * 返回: true如果结果结构有效
 */
bool
tecc_validate_column_result(TeccCursorInterface *cursor)
{
    int column_count;

    if (cursor == NULL)
    {
        return false;
    }

    column_count = cursor->get_column_count(cursor);

    /* 加密列查询应返回14列：table_oid, table_name, column_attnum, column_name,
       data_type_original_oid, data_type_original_mod, dek_name, encryption_type,
       collation_locale, column_key_id, create_date, has_dek_permission,
       he_add_dek, he_mul_dek。
       ReqID:TECC4.0 - pg_upgrade V4.0.0 兼容：V4.0.0 视图无 he_add_dek/
       he_mul_dek 两列，数据获取器缺列时不查，结果允许 12 列
       （解析器按位置取值，越界列返回 NULL，he 字段保持 NULL）。 */
    if (column_count != 14 && column_count != 12)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE,
                               "Invalid column count for column query");
        return false;
    }

    return true;
}

/*
 * tecc_parse_kek_list - 解析KEK列表
 *
 * cursor: 查询游标
 * result: 输出KEK元数据列表
 * 返回: 错误码
 *
 * 设计原则: 数据获取器负责解析原始查询结果为结构化元数据对象列表。
 * 缓存模块直接存储这些已解析的对象，无需再次解析。
 *
 * ReqID:TECC4.0 - hg_kek_info 是"每个KEK的每个参数各一行"（同一KEK的字段列
 * 0-6 在它的每一行上重复），所以必须**逐行**按 kek_oid 分组：oid 与当前KEK
 * 相同的行只贡献一个参数，不同的行才新建KEK对象。不能用"读到别的KEK就停、
 * 把该行留给下一次调用"的写法——cursor->next() 前进一行，被留下的那行会被
 * 下一次 next() 直接跳过，导致每个KEK丢掉首个参数行（如 KEY_PATH）。
 */
TeccFetcherErrorCode
tecc_parse_kek_list(const TeccState *state, TeccCursorInterface *cursor, List **result)
{
    List *kek_list = NULL;
    TeccKekMetadata *prev_kek = NULL;
    TeccFetcherErrorCode error_code;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    if (!tecc_validate_kek_result(cursor))
    {
        TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_list: KEK result validation failed");
        return TECC_FETCHER_ERROR_PARSE;
    }

    TECC_LOG(TECC_LOG_DEBUG1, "tecc_parse_kek_list: Starting to parse KEK list, column count: %d", cursor->get_column_count(cursor));

    /* Begin - ReqID:TECC4.0 */
    while (cursor->next(cursor) == TECC_FETCHER_OK)
    {
        TeccKekMetadata *row_kek = NULL;

        error_code = tecc_parse_kek_fields_from_current_row(cursor, &row_kek);
        if (error_code != TECC_FETCHER_OK)
        {
            ListCell *cell;

            foreach(cell, kek_list)
            {
                tecc_free_kek_metadata((TeccKekMetadata *)lfirst(cell));
            }
            if (kek_list != NIL)
            {
                list_free(kek_list);
            }
            return error_code;
        }

        if (prev_kek != NULL && prev_kek->kek_oid == row_kek->kek_oid
            && prev_kek->kek_oid != 0)
        {
            /* 同一KEK的后续行：字段与首行相同，只保留首行的对象 */
            tecc_free_kek_metadata(row_kek);
        }
        else
        {
            kek_list = lappend(kek_list, row_kek);
            prev_kek = row_kek;
        }

        /* 本行携带的参数追加到所属KEK（列数<10时为空操作） */
        tecc_kek_append_arg_from_current_row(cursor, prev_kek);
    }
    /* End - ReqID:TECC4.0 */

    /* 返回已解析的列表 */
    *result = kek_list;
    return TECC_FETCHER_OK;
}

/*
 * tecc_parse_dek_list - 解析DEK列表
 *
 * cursor: 查询游标
 * result: 输出DEK元数据列表
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_parse_dek_list(const TeccState *state, TeccCursorInterface *cursor, List **result)
{
    List *dek_list = NULL;
    TeccDekMetadata *dek_metadata = NULL;
    TeccFetcherErrorCode error_code;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    if (!tecc_validate_dek_result(cursor))
    {
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 遍历所有行 — 批量查询含args时，同一DEK可能有多行（每行一个arg），需合并 */
    {
        TeccDekMetadata *prev_dek = NULL;

        while ((error_code = tecc_parse_dek_metadata(state, cursor, &dek_metadata)) == TECC_FETCHER_OK)
        {
            if (dek_metadata != NULL)
            {
                /* 检查是否与上一行是同一个DEK（按dek_oid合并args） */
                if (prev_dek != NULL && prev_dek->dek_oid == dek_metadata->dek_oid
                    && prev_dek->dek_oid != 0)
                {
                    /* 同一DEK：将当前行的args追加到prev */
                    if (dek_metadata->args != NULL && dek_metadata->args_count > 0)
                    {
                        int new_count = prev_dek->args_count + dek_metadata->args_count;
                        TeccKeyArg *new_args = (TeccKeyArg *)repalloc(prev_dek->args,
                            sizeof(TeccKeyArg) * new_count);
                        memcpy(new_args + prev_dek->args_count, dek_metadata->args,
                               sizeof(TeccKeyArg) * dek_metadata->args_count);
                        prev_dek->args = new_args;
                        prev_dek->args_count = new_count;
                    }
                    /* 释放当前行（args已转移，需单独释放原指针再置空防止重复释放） */
                    if (dek_metadata->args != NULL)
                        pg_free(dek_metadata->args);
                    dek_metadata->args = NULL;
                    dek_metadata->args_count = 0;
                    tecc_free_dek_metadata(dek_metadata);
                }
                else
                {
                    dek_list = lappend(dek_list, dek_metadata);
                    prev_dek = dek_metadata;
                }
            }
        }
    }

    /* tecc_parse_dek_metadata返回TECC_FETCHER_ERROR_PARSE表示正常结束（无更多行） */
    if (error_code != TECC_FETCHER_ERROR_PARSE)
    {
        if (dek_list != NULL)
        {
            ListCell *cell;
            foreach(cell, dek_list)
            {
                TeccDekMetadata *dek = (TeccDekMetadata *)lfirst(cell);
                if (dek != NULL)
                    tecc_free_dek_metadata(dek);
            }
            list_free(dek_list);
        }
        return error_code;
    }

    *result = dek_list;
    return TECC_FETCHER_OK;
}

/*
 * tecc_parse_columns_list - 解析加密列列表
 *
 * cursor: 查询游标
 * result: 输出加密列元数据列表
 * 返回: 错误码
 */
TeccFetcherErrorCode
tecc_parse_columns_list(TeccCursorInterface *cursor, List **result)
{
    List *column_list = NULL;
    TeccEncryptedColumnMetadata *column_metadata = NULL;
    TeccFetcherErrorCode error_code;

    if (cursor == NULL || result == NULL)
    {
        return TECC_FETCHER_ERROR_INVALID_PARAM;
    }

    /* 初始化输出参数 */
    *result = NULL;

    /* 验证结果结构 */
    if (!tecc_validate_column_result(cursor))
    {
        return TECC_FETCHER_ERROR_PARSE;
    }

    /* 遍历所有行 */
    while (cursor->next(cursor) == TECC_FETCHER_OK)
    {
        /* 解析单行加密列元数据 */
        error_code = tecc_parse_encrypted_column_metadata(cursor, &column_metadata);
        if (error_code != TECC_FETCHER_OK)
        {
            /* 清理已解析的列表 */
            if (column_list != NULL)
            {
                /* 释放列表中的所有列元数据 */
                ListCell *cell;
                foreach(cell, column_list)
                {
                    TeccEncryptedColumnMetadata *col = (TeccEncryptedColumnMetadata *)lfirst(cell);
                    if (col != NULL)
                    {
                        tecc_free_encrypted_column_metadata(col);
                    }
                }
                list_free(column_list);
            }
            return error_code;
        }

        /* 添加到列表 */
        if (column_metadata != NULL)
        {
            column_list = lappend(column_list, column_metadata);
        }
    }

    *result = column_list;
    return TECC_FETCHER_OK;
}

/* End - ReqID:TECC4.0 */