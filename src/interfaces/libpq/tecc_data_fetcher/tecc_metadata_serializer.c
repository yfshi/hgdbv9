/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_metadata_serializer.c
 *    TECC metadata serialization and deserialization implementation
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_metadata_serializer.c
 *
 *-------------------------------------------------------------------------
 */
#include "tecc_metadata_serializer.h"
#include "tecc_error_handler.h"
#include "../../tecc_cache/tecc_metadata_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 序列化头部结构 */
typedef struct {
    uint32_t magic;         /* 魔数：0x54454343 (TECC) */
    uint32_t version;       /* 版本号：1 */
    uint32_t data_type;     /* 数据类型：1=KEK, 2=DEK, 3=Column */
    uint32_t data_size;     /* 数据总大小 */
    time_t timestamp;       /* 序列化时间戳 */
} TeccSerializationHeader;

/* 序列化魔数和版本 */
#define TECC_SERIALIZATION_MAGIC 0x54454343  /* "TECC" */
#define TECC_SERIALIZATION_VERSION 1

/* 数据类型 */
#define TECC_DATA_TYPE_KEK 1
#define TECC_DATA_TYPE_DEK 2
#define TECC_DATA_TYPE_COLUMN 3

/*
 * tecc_serialize_kek_metadata - 序列化KEK元数据
 *
 * kek: KEK元数据
 * buffer: 输出缓冲区指针
 * 返回: 序列化数据大小
 */
size_t
tecc_serialize_kek_metadata(const TeccKekMetadata *kek, void **buffer)
{
    TeccSerializationHeader header;
    char *buf;
    size_t total_size;
    size_t offset;
    int i;

    if (kek == NULL || buffer == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid parameters in serialize_kek_metadata");
        return 0;
    }

    /* 计算总大小 */
    total_size = sizeof(TeccSerializationHeader);
    total_size += sizeof(Oid);     /* kek_oid - 新增字段 */
    total_size += strlen(kek->key_name) + 1;
    total_size += (kek->key_namespace != NULL) ? strlen(kek->key_namespace) + 1 : 1;
    total_size += sizeof(Oid);     /* key_owner_oid - 新增字段 */
    total_size += (kek->key_owner_name != NULL) ? strlen(kek->key_owner_name) + 1 : 1;
    total_size += (kek->key_acl != NULL) ? strlen(kek->key_acl) + 1 : 1;
    total_size += sizeof(time_t);  /* create_date */
    total_size += sizeof(int);     /* args_count */

    /* 计算参数大小 */
    for (i = 0; i < kek->args_count; i++)
    {
        total_size += strlen(kek->args[i].function_name) + 1;
        total_size += strlen(kek->args[i].key) + 1;
        total_size += strlen(kek->args[i].value) + 1;
    }

    /* 分配缓冲区 */
    buf = (char *)pg_malloc(total_size);
    if (buf == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate buffer in serialize_kek_metadata");
        return 0;
    }

    /* 填充头部 */
    header.magic = TECC_SERIALIZATION_MAGIC;
    header.version = TECC_SERIALIZATION_VERSION;
    header.data_type = TECC_DATA_TYPE_KEK;
    header.data_size = total_size;
    header.timestamp = time(NULL);

    memcpy(buf, &header, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 序列化kek_oid字段 */
    memcpy(buf + offset, &kek->kek_oid, sizeof(Oid));
    offset += sizeof(Oid);

    /* 序列化字符串字段 */
    strcpy(buf + offset, kek->key_name);
    offset += strlen(kek->key_name) + 1;

    if (kek->key_namespace != NULL)
    {
        strcpy(buf + offset, kek->key_namespace);
        offset += strlen(kek->key_namespace) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    /* 序列化key_owner_oid */
    memcpy(buf + offset, &kek->key_owner_oid, sizeof(Oid));
    offset += sizeof(Oid);

    /* 序列化key_owner_name */
    if (kek->key_owner_name != NULL)
    {
        strcpy(buf + offset, kek->key_owner_name);
        offset += strlen(kek->key_owner_name) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    if (kek->key_acl != NULL)
    {
        strcpy(buf + offset, kek->key_acl);
        offset += strlen(kek->key_acl) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    /* 序列化create_date */
    memcpy(buf + offset, &kek->create_date, sizeof(time_t));
    offset += sizeof(time_t);

    /* 序列化参数数量 */
    memcpy(buf + offset, &kek->args_count, sizeof(int));
    offset += sizeof(int);

    /* 序列化参数 */
    for (i = 0; i < kek->args_count; i++)
    {
        strcpy(buf + offset, kek->args[i].function_name);
        offset += strlen(kek->args[i].function_name) + 1;

        strcpy(buf + offset, kek->args[i].key);
        offset += strlen(kek->args[i].key) + 1;

        strcpy(buf + offset, kek->args[i].value);
        offset += strlen(kek->args[i].value) + 1;
    }

    *buffer = buf;
    return total_size;
}

/*
 * tecc_deserialize_kek_metadata - 反序列化KEK元数据
 *
 * buffer: 序列化数据缓冲区
 * buffer_size: 缓冲区大小
 * 返回: KEK元数据指针
 */
TeccKekMetadata *
tecc_deserialize_kek_metadata(const void *buffer, size_t buffer_size)
{
    const char *buf = (const char *)buffer;
    TeccSerializationHeader header;
    TeccKekMetadata *kek;
    size_t offset;
    int i;

    if (buffer == NULL || buffer_size < sizeof(TeccSerializationHeader))
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid buffer in deserialize_kek_metadata");
        return NULL;
    }

    /* 读取头部 */
    memcpy(&header, buf, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 验证头部 */
    if (header.magic != TECC_SERIALIZATION_MAGIC ||
        header.version != TECC_SERIALIZATION_VERSION ||
        header.data_type != TECC_DATA_TYPE_KEK ||
        header.data_size != buffer_size)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE,
                               "Invalid serialization header in deserialize_kek_metadata");
        return NULL;
    }

    /* 分配KEK元数据结构 */
    kek = (TeccKekMetadata *)pg_malloc(sizeof(TeccKekMetadata));
    if (kek == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate kek metadata in deserialize");
        return NULL;
    }

    /* 反序列化kek_oid字段 */
    memcpy(&kek->kek_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    /* 反序列化字符串字段 */
    kek->key_name = pg_strdup(buf + offset);
    offset += strlen(kek->key_name) + 1;

    if (buf[offset] != '\0')
    {
        kek->key_namespace = pg_strdup(buf + offset);
        offset += strlen(kek->key_namespace) + 1;
    }
    else
    {
        kek->key_namespace = NULL;
        offset += 1;
    }

    /* 反序列化key_owner_oid */
    memcpy(&kek->key_owner_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    /* 反序列化key_owner_name */
    if (buf[offset] != '\0')
    {
        kek->key_owner_name = pg_strdup(buf + offset);
        offset += strlen(kek->key_owner_name) + 1;
    }
    else
    {
        kek->key_owner_name = NULL;
        offset += 1;
    }

    if (buf[offset] != '\0')
    {
        kek->key_acl = pg_strdup(buf + offset);
        offset += strlen(kek->key_acl) + 1;
    }
    else
    {
        kek->key_acl = NULL;
        offset += 1;
    }

    /* 反序列化create_date */
    memcpy(&kek->create_date, buf + offset, sizeof(time_t));
    offset += sizeof(time_t);

    /* 反序列化参数数量 */
    memcpy(&kek->args_count, buf + offset, sizeof(int));
    offset += sizeof(int);

    /* 分配参数数组 */
    if (kek->args_count > 0)
    {
        kek->args = (TeccKeyArg *)pg_malloc(kek->args_count * sizeof(TeccKeyArg));
        if (kek->args == NULL)
        {
            tecc_free_kek_metadata(kek);
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate args array in deserialize");
            return NULL;
        }

        /* 反序列化参数 */
        for (i = 0; i < kek->args_count; i++)
        {
            kek->args[i].function_name = pg_strdup(buf + offset);
            offset += strlen(kek->args[i].function_name) + 1;

            kek->args[i].key = pg_strdup(buf + offset);
            offset += strlen(kek->args[i].key) + 1;

            kek->args[i].value = pg_strdup(buf + offset);
            offset += strlen(kek->args[i].value) + 1;
        }
    }
    else
    {
        kek->args = NULL;
    }

    return kek;
}

/*
 * tecc_serialize_dek_metadata - 序列化DEK元数据
 *
 * dek: DEK元数据
 * buffer: 输出缓冲区指针
 * 返回: 序列化数据大小
 */
size_t
tecc_serialize_dek_metadata(const TeccDekMetadata *dek, void **buffer)
{
    TeccSerializationHeader header;
    char *buf;
    size_t total_size;
    size_t offset;
    int i;

    if (dek == NULL || buffer == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid parameters in serialize_dek_metadata");
        return 0;
    }

    /* 计算总大小 */
    total_size = sizeof(TeccSerializationHeader);
    total_size += sizeof(Oid);     /* dek_oid - 新增字段 */
    total_size += strlen(dek->key_name) + 1;
    total_size += sizeof(Oid);     /* column_key_distributed_id - 新增字段 */
    total_size += sizeof(Oid);     /* kek_oid - 新增字段 */
    total_size += strlen(dek->kek_name) + 1;
    total_size += (dek->key_namespace != NULL) ? strlen(dek->key_namespace) + 1 : 1;
    total_size += sizeof(Oid);     /* key_owner_oid - 新增字段 */
    total_size += (dek->key_owner_name != NULL) ? strlen(dek->key_owner_name) + 1 : 1;
    total_size += (dek->key_acl != NULL) ? strlen(dek->key_acl) + 1 : 1;
    total_size += sizeof(time_t);  /* create_date */
    total_size += sizeof(int);     /* args_count */

    /* 计算参数大小 */
    for (i = 0; i < dek->args_count; i++)
    {
        total_size += strlen(dek->args[i].function_name) + 1;
        total_size += strlen(dek->args[i].key) + 1;
        total_size += strlen(dek->args[i].value) + 1;
    }

    /* 分配缓冲区 */
    buf = (char *)pg_malloc(total_size);
    if (buf == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate buffer in serialize_dek_metadata");
        return 0;
    }

    /* 填充头部 */
    header.magic = TECC_SERIALIZATION_MAGIC;
    header.version = TECC_SERIALIZATION_VERSION;
    header.data_type = TECC_DATA_TYPE_DEK;
    header.data_size = total_size;
    header.timestamp = time(NULL);

    memcpy(buf, &header, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 序列化dek_oid字段 */
    memcpy(buf + offset, &dek->dek_oid, sizeof(Oid));
    offset += sizeof(Oid);

    /* 序列化字符串字段 */
    strcpy(buf + offset, dek->key_name);
    offset += strlen(dek->key_name) + 1;

    /* 序列化column_key_distributed_id字段 */
    memcpy(buf + offset, &dek->column_key_distributed_id, sizeof(Oid));
    offset += sizeof(Oid);

    /* 序列化kek_oid字段 */
    memcpy(buf + offset, &dek->kek_oid, sizeof(Oid));
    offset += sizeof(Oid);

    strcpy(buf + offset, dek->kek_name);
    offset += strlen(dek->kek_name) + 1;

    if (dek->key_namespace != NULL)
    {
        strcpy(buf + offset, dek->key_namespace);
        offset += strlen(dek->key_namespace) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    /* 序列化key_owner_oid */
    memcpy(buf + offset, &dek->key_owner_oid, sizeof(Oid));
    offset += sizeof(Oid);

    /* 序列化key_owner_name */
    if (dek->key_owner_name != NULL)
    {
        strcpy(buf + offset, dek->key_owner_name);
        offset += strlen(dek->key_owner_name) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    if (dek->key_acl != NULL)
    {
        strcpy(buf + offset, dek->key_acl);
        offset += strlen(dek->key_acl) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    /* 序列化create_date */
    memcpy(buf + offset, &dek->create_date, sizeof(time_t));
    offset += sizeof(time_t);

    /* 序列化参数数量 */
    memcpy(buf + offset, &dek->args_count, sizeof(int));
    offset += sizeof(int);

    /* 序列化参数 */
    for (i = 0; i < dek->args_count; i++)
    {
        strcpy(buf + offset, dek->args[i].function_name);
        offset += strlen(dek->args[i].function_name) + 1;

        strcpy(buf + offset, dek->args[i].key);
        offset += strlen(dek->args[i].key) + 1;

        strcpy(buf + offset, dek->args[i].value);
        offset += strlen(dek->args[i].value) + 1;
    }

    *buffer = buf;
    return total_size;
}

/*
 * tecc_deserialize_dek_metadata - 反序列化DEK元数据
 *
 * buffer: 序列化数据缓冲区
 * buffer_size: 缓冲区大小
 * 返回: DEK元数据指针
 */
TeccDekMetadata *
tecc_deserialize_dek_metadata(const void *buffer, size_t buffer_size)
{
    const char *buf = (const char *)buffer;
    TeccSerializationHeader header;
    TeccDekMetadata *dek;
    size_t offset;
    int i;

    if (buffer == NULL || buffer_size < sizeof(TeccSerializationHeader))
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid buffer in deserialize_dek_metadata");
        return NULL;
    }

    /* 读取头部 */
    memcpy(&header, buf, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 验证头部 */
    if (header.magic != TECC_SERIALIZATION_MAGIC ||
        header.version != TECC_SERIALIZATION_VERSION ||
        header.data_type != TECC_DATA_TYPE_DEK ||
        header.data_size != buffer_size)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE,
                               "Invalid serialization header in deserialize_dek_metadata");
        return NULL;
    }

    /* 分配DEK元数据结构 */
    dek = (TeccDekMetadata *)pg_malloc(sizeof(TeccDekMetadata));
    if (dek == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate dek metadata in deserialize");
        return NULL;
    }

    /* 反序列化dek_oid字段 */
    memcpy(&dek->dek_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    /* 反序列化字符串字段 */
    dek->key_name = pg_strdup(buf + offset);
    offset += strlen(dek->key_name) + 1;

    /* 反序列化column_key_distributed_id字段 */
    memcpy(&dek->column_key_distributed_id, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    /* 反序列化kek_oid字段 */
    memcpy(&dek->kek_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    dek->kek_name = pg_strdup(buf + offset);
    offset += strlen(dek->kek_name) + 1;

    if (buf[offset] != '\0')
    {
        dek->key_namespace = pg_strdup(buf + offset);
        offset += strlen(dek->key_namespace) + 1;
    }
    else
    {
        dek->key_namespace = NULL;
        offset += 1;
    }

    /* 反序列化key_owner_oid */
    memcpy(&dek->key_owner_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    /* 反序列化key_owner_name */
    if (buf[offset] != '\0')
    {
        dek->key_owner_name = pg_strdup(buf + offset);
        offset += strlen(dek->key_owner_name) + 1;
    }
    else
    {
        dek->key_owner_name = NULL;
        offset += 1;
    }

    if (buf[offset] != '\0')
    {
        dek->key_acl = pg_strdup(buf + offset);
        offset += strlen(dek->key_acl) + 1;
    }
    else
    {
        dek->key_acl = NULL;
        offset += 1;
    }

    /* 反序列化create_date */
    memcpy(&dek->create_date, buf + offset, sizeof(time_t));
    offset += sizeof(time_t);

    /* 反序列化参数数量 */
    memcpy(&dek->args_count, buf + offset, sizeof(int));
    offset += sizeof(int);

    /* 分配参数数组 */
    if (dek->args_count > 0)
    {
        dek->args = (TeccKeyArg *)pg_malloc(dek->args_count * sizeof(TeccKeyArg));
        if (dek->args == NULL)
        {
            tecc_free_dek_metadata(dek);
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate args array in deserialize");
            return NULL;
        }

        /* 反序列化参数 */
        for (i = 0; i < dek->args_count; i++)
        {
            dek->args[i].function_name = pg_strdup(buf + offset);
            offset += strlen(dek->args[i].function_name) + 1;

            dek->args[i].key = pg_strdup(buf + offset);
            offset += strlen(dek->args[i].key) + 1;

            dek->args[i].value = pg_strdup(buf + offset);
            offset += strlen(dek->args[i].value) + 1;
        }
    }
    else
    {
        dek->args = NULL;
    }

    return dek;
}

/*
 * tecc_serialize_column_metadata - 序列化加密列元数据
 *
 * column: 加密列元数据
 * buffer: 输出缓冲区指针
 * 返回: 序列化数据大小
 */
size_t
tecc_serialize_column_metadata(const TeccEncryptedColumnMetadata *column, void **buffer)
{
    TeccSerializationHeader header;
    char *buf;
    size_t total_size;
    size_t offset;

    if (column == NULL || buffer == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid parameters in serialize_column_metadata");
        return 0;
    }

    /* 计算总大小 */
    total_size = sizeof(TeccSerializationHeader);
    total_size += sizeof(Oid);     /* table_oid */
    total_size += (column->table_name != NULL) ? strlen(column->table_name) + 1 : 1;
    total_size += sizeof(int);     /* column_attnum */
    total_size += strlen(column->column_name) + 1;
    total_size += sizeof(Oid);     /* data_type_original_oid */
    total_size += strlen(column->dek_name) + 1;
    total_size += sizeof(int);     /* encryption_type */
    total_size += sizeof(Oid);     /* collation_oid */
    total_size += (column->collation_locale != NULL) ? strlen(column->collation_locale) + 1 : 1;
    total_size += sizeof(time_t);  /* create_date */

    /* 分配缓冲区 */
    buf = (char *)pg_malloc(total_size);
    if (buf == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate buffer in serialize_column_metadata");
        return 0;
    }

    /* 填充头部 */
    header.magic = TECC_SERIALIZATION_MAGIC;
    header.version = TECC_SERIALIZATION_VERSION;
    header.data_type = TECC_DATA_TYPE_COLUMN;
    header.data_size = total_size;
    header.timestamp = time(NULL);

    memcpy(buf, &header, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 序列化字段 */
    memcpy(buf + offset, &column->table_oid, sizeof(Oid));
    offset += sizeof(Oid);

    if (column->table_name != NULL)
    {
        strcpy(buf + offset, column->table_name);
        offset += strlen(column->table_name) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    memcpy(buf + offset, &column->column_attnum, sizeof(int));
    offset += sizeof(int);

    strcpy(buf + offset, column->column_name);
    offset += strlen(column->column_name) + 1;

    memcpy(buf + offset, &column->data_type_original_oid, sizeof(Oid));
    offset += sizeof(Oid);

    strcpy(buf + offset, column->dek_name);
    offset += strlen(column->dek_name) + 1;

    memcpy(buf + offset, &column->encryption_type, sizeof(int));
    offset += sizeof(int);

    memcpy(buf + offset, &column->collation_oid, sizeof(Oid));
    offset += sizeof(Oid);

    if (column->collation_locale != NULL)
    {
        strcpy(buf + offset, column->collation_locale);
        offset += strlen(column->collation_locale) + 1;
    }
    else
    {
        buf[offset] = '\0';
        offset += 1;
    }

    memcpy(buf + offset, &column->create_date, sizeof(time_t));
    offset += sizeof(time_t);

    *buffer = buf;
    return total_size;
}

/*
 * tecc_deserialize_column_metadata - 反序列化加密列元数据
 *
 * buffer: 序列化数据缓冲区
 * buffer_size: 缓冲区大小
 * 返回: 加密列元数据指针
 */
TeccEncryptedColumnMetadata *
tecc_deserialize_column_metadata(const void *buffer, size_t buffer_size)
{
    const char *buf = (const char *)buffer;
    TeccSerializationHeader header;
    TeccEncryptedColumnMetadata *column;
    size_t offset;

    if (buffer == NULL || buffer_size < sizeof(TeccSerializationHeader))
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_INVALID_PARAM,
                               "Invalid buffer in deserialize_column_metadata");
        return NULL;
    }

    /* 读取头部 */
    memcpy(&header, buf, sizeof(TeccSerializationHeader));
    offset = sizeof(TeccSerializationHeader);

    /* 验证头部 */
    if (header.magic != TECC_SERIALIZATION_MAGIC ||
        header.version != TECC_SERIALIZATION_VERSION ||
        header.data_type != TECC_DATA_TYPE_COLUMN ||
        header.data_size != buffer_size)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_PARSE,
                               "Invalid serialization header in deserialize_column_metadata");
        return NULL;
    }

    /* 分配加密列元数据结构 */
    column = (TeccEncryptedColumnMetadata *)pg_malloc(sizeof(TeccEncryptedColumnMetadata));
    if (column == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate column metadata in deserialize");
        return NULL;
    }

    /* 反序列化字段 */
    memcpy(&column->table_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    if (buf[offset] != '\0')
    {
        column->table_name = pg_strdup(buf + offset);
        offset += strlen(column->table_name) + 1;
    }
    else
    {
        column->table_name = NULL;
        offset += 1;
    }

    memcpy(&column->column_attnum, buf + offset, sizeof(int));
    offset += sizeof(int);

    column->column_name = pg_strdup(buf + offset);
    offset += strlen(column->column_name) + 1;

    memcpy(&column->data_type_original_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    column->dek_name = pg_strdup(buf + offset);
    offset += strlen(column->dek_name) + 1;

    memcpy(&column->encryption_type, buf + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&column->collation_oid, buf + offset, sizeof(Oid));
    offset += sizeof(Oid);

    if (buf[offset] != '\0')
    {
        column->collation_locale = pg_strdup(buf + offset);
        offset += strlen(column->collation_locale) + 1;
    }
    else
    {
        column->collation_locale = NULL;
        offset += 1;
    }

    memcpy(&column->create_date, buf + offset, sizeof(time_t));
    offset += sizeof(time_t);

    return column;
}

/*
 * tecc_generate_kek_cache_key - 生成KEK缓存键
 *
 * key_name: KEK名称
 * key_namespace: 命名空间
 * 返回: 缓存键字符串
 */
char *
tecc_generate_kek_cache_key(const char *key_name, const char *key_namespace)
{
    char *cache_key;
    size_t len;

    if (key_name == NULL)
    {
        return NULL;
    }

    if (key_namespace != NULL)
    {
        len = strlen("KEK:") + strlen(key_namespace) + strlen(":") + strlen(key_name) + 1;
        cache_key = (char *)pg_malloc(len);
        if (cache_key == NULL)
        {
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate cache key");
            return NULL;
        }
        snprintf(cache_key, len, "KEK:%s:%s", key_namespace, key_name);
    }
    else
    {
        len = strlen("KEK:default:") + strlen(key_name) + 1;
        cache_key = (char *)pg_malloc(len);
        if (cache_key == NULL)
        {
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate cache key");
            return NULL;
        }
        snprintf(cache_key, len, "KEK:default:%s", key_name);
    }

    return cache_key;
}

/*
 * tecc_generate_dek_cache_key - 生成DEK缓存键
 *
 * key_name: DEK名称
 * key_namespace: 命名空间
 * 返回: 缓存键字符串
 */
char *
tecc_generate_dek_cache_key(const char *key_name, const char *key_namespace)
{
    char *cache_key;
    size_t len;

    if (key_name == NULL)
    {
        return NULL;
    }

    if (key_namespace != NULL)
    {
        len = strlen("DEK:") + strlen(key_namespace) + strlen(":") + strlen(key_name) + 1;
        cache_key = (char *)pg_malloc(len);
        if (cache_key == NULL)
        {
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate cache key");
            return NULL;
        }
        snprintf(cache_key, len, "DEK:%s:%s", key_namespace, key_name);
    }
    else
    {
        len = strlen("DEK:default:") + strlen(key_name) + 1;
        cache_key = (char *)pg_malloc(len);
        if (cache_key == NULL)
        {
            tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                                   "Failed to allocate cache key");
            return NULL;
        }
        snprintf(cache_key, len, "DEK:default:%s", key_name);
    }

    return cache_key;
}

/*
 * tecc_generate_column_cache_key - 生成加密列缓存键
 *
 * table_oid: 表OID
 * column_attnum: 列属性编号
 * 返回: 缓存键字符串
 */
char *
tecc_generate_column_cache_key(Oid table_oid, int column_attnum)
{
    char *cache_key;
    size_t len;

    len = strlen("COLUMN:") + 20 + 1 + 10 + 1; /* OID最大10位，attnum最大10位 */
    cache_key = (char *)pg_malloc(len);
    if (cache_key == NULL)
    {
        tecc_log_fetcher_error(TECC_FETCHER_ERROR_MEMORY,
                               "Failed to allocate cache key");
        return NULL;
    }

    snprintf(cache_key, len, "COLUMN:%u:%d", table_oid, column_attnum);
    return cache_key;
}
/* End - ReqID:TECC4.0 */