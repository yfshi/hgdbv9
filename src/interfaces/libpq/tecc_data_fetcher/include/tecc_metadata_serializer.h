/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_metadata_serializer.h
 *    TECC metadata serialization and deserialization
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_metadata_serializer.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_METADATA_SERIALIZER_H
#define TECC_METADATA_SERIALIZER_H

#include "tecc_data_fetcher_types.h"

/* KEK元数据序列化 */
extern size_t tecc_serialize_kek_metadata(const TeccKekMetadata *kek, void **buffer);
extern TeccKekMetadata *tecc_deserialize_kek_metadata(const void *buffer, size_t buffer_size);

/* DEK元数据序列化 */
extern size_t tecc_serialize_dek_metadata(const TeccDekMetadata *dek, void **buffer);
extern TeccDekMetadata *tecc_deserialize_dek_metadata(const void *buffer, size_t buffer_size);

/* 加密列元数据序列化 */
extern size_t tecc_serialize_column_metadata(const TeccEncryptedColumnMetadata *column, void **buffer);
extern TeccEncryptedColumnMetadata *tecc_deserialize_column_metadata(const void *buffer, size_t buffer_size);

/* 缓存键生成函数 */
extern char *tecc_generate_kek_cache_key(const char *key_name, const char *key_namespace);
extern char *tecc_generate_dek_cache_key(const char *key_name, const char *key_namespace);
extern char *tecc_generate_column_cache_key(Oid table_oid, int column_attnum);

#endif /* TECC_METADATA_SERIALIZER_H */
/* End - ReqID:TECC4.0 */