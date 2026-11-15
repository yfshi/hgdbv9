/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_type_output.h
 *
 * Abstract:
 *   Binary-to-text output conversion for TECC decrypted data.
 *   Converts PostgreSQL internal binary format to human-readable text strings.
 *   Reference: backend *_out functions (int2out, int4out, int8out, float4out,
 *   float8out, numeric_out, textout, bpcharout, varcharout).
 *
 * Authored by <wangxp@highgo.com>, 20260424.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_type_output.h
 */

#ifndef TECC_TYPE_OUTPUT_H
#define TECC_TYPE_OUTPUT_H

#include "postgres_fe.h"
#include <stddef.h>

/* ReqID:TECC4.0 - For TeccFormatterErrorCode type */
#include "../tecc_common/tecc_common_types.h"

/* OID constants for base data types */
#define TECC_OUTPUT_INT2OID     21
#define TECC_OUTPUT_INT4OID     23
#define TECC_OUTPUT_INT8OID     20
#define TECC_OUTPUT_FLOAT4OID   700
#define TECC_OUTPUT_FLOAT8OID   701
#define TECC_OUTPUT_NUMERICOID  1700
#define TECC_OUTPUT_VARCHAROID  1043
#define TECC_OUTPUT_BPCHAROID   1042
#define TECC_OUTPUT_TEXTOID     25

/*
 * tecc_type_output_data - Convert decrypted binary data to text string
 *
 * Converts PostgreSQL internal binary format to human-readable text,
 * based on the base data type OID. Reference: backend *_out functions.
 *
 * @param type_oid     Base data type OID (INT2OID=21, INT4OID=23, etc.)
 * @param type_mod     Type modifier (atttypmod) for precision/scale info
 * @param binary_data  Pointer to decrypted binary data
 * @param binary_len   Length of binary data in bytes
 *
 * @return pg_malloc'd null-terminated text string, or NULL on error
 *
 * Caller is responsible for pg_free()ing the returned string.
 */
extern char *tecc_type_output_data(Oid type_oid, int type_mod,
                                   const char *binary_data,
                                   size_t binary_len);

/*
 * Type-specific output functions (exposed for testing)
 */

/* int2: binary is 2-byte little-endian int16 */
extern char *tecc_type_output_int2(const char *data, size_t len);

/* int4: binary is 4-byte little-endian int32 */
extern char *tecc_type_output_int4(const char *data, size_t len);

/* int8: binary is 8-byte little-endian int64 */
extern char *tecc_type_output_int8(const char *data, size_t len);

/* float4: binary is 4-byte IEEE 754 float */
extern char *tecc_type_output_float4(const char *data, size_t len);

/* float8: binary is 8-byte IEEE 754 double */
extern char *tecc_type_output_float8(const char *data, size_t len);

/* numeric: binary is NumericData varlena structure */
extern char *tecc_type_output_numeric(const char *data, size_t len,
                                       int type_mod);

/* text/varchar/bpchar:
 *   RND:  4-byte network byte order length prefix + text data
 *   DET/SEARCH: raw text, no prefix */
extern char *tecc_type_output_text(const char *data, size_t len);

/*
 * ReqID:TECC4.0 - 按列声明长度（typmod）计算字符型文本的有效长度（Bug 9327）。
 *
 * 判据与服务端 varchar_input()/bpchar_input() 一致：按字符计数，超过 N 个字符
 * 时只有超出部分全是空格才被接受（服务端会截掉）。写入侧据此拒绝/截断后再加密，
 * 读取侧（tecc_type_output_data）据此定型回显，历史数据同样收敛到服务端语义。
 *
 * @param data_type      基础类型 OID（VARCHAR/BPCHAR 之外的类型不做处理）
 * @param type_mod       列声明的 typmod（< VARHDRSZ 表示未声明长度）
 * @param text           待判定的文本（不必是 NUL 结尾）
 * @param text_len       文本字节长度
 * @param encoding       字符计数所用编码（如 PG_UTF8）
 * @param effective_len  输出：应采用的长度（未超长时等于 text_len）
 * @return               false 表示超出部分含非空格字符（写入侧应拒绝；读取侧保持原值）
 */
extern bool tecc_char_typmod_effective_len(Oid data_type, int type_mod,
                                           const char *text, size_t text_len,
                                           int encoding, size_t *effective_len);

/*
 * ReqID:TECC4.0 - 显式 cast 语义下的字符型长度定型（Bug 9327，原地截断）。
 *
 * 与服务端 varchar()/bpchar() 的显式 cast 分支一致：超过 N 个字符时静默截断，
 * 超出部分是普通字符也不报错。用于带 typmod 的密态 cast
 * （'x'::varchar_enc(N) ENCRYPTED WITH (...)）：到服务端已是密文，服务端无从做
 * 这次截断，必须在前端补上。截断后再交由写入侧按目标列长度做严格校验
 * （tecc_char_typmod_effective_len）。
 *
 * @param declared_len  类型名上声明的长度（语法里的 N），未声明传 -1
 * @param base_oid      基础类型 OID（VARCHAR/BPCHAR 之外的类型不做处理）
 * @param text          NUL 结尾的明文缓冲区，原地截断
 * @param encoding      字符计数所用编码（如 PG_UTF8）
 */
extern void tecc_char_cast_len_clip(int declared_len, Oid base_oid,
                                    char *text, int encoding);

/*
 * tecc_type_input_numeric_from_text - Convert numeric text to NumericData varlena binary
 *
 * Converts a numeric text string (e.g., "50000.00") to PostgreSQL internal
 * NumericData varlena binary format suitable for RND encryption.
 *
 * @param text       Null-terminated numeric text string
 * @param text_len   Length of text string
 * @param binary_out Output: pg_malloc'd NumericData binary
 * @param binary_len Output: length of binary data
 * @return           0 on success, -1 on error
 */
extern int tecc_type_input_numeric_from_text(const char *text, size_t text_len,
                                              unsigned char **binary_out,
                                              size_t *binary_len);

/*
 * ReqID:TECC4.0 - tecc_formatter_prepare_plaintext
 *   Input validation + text to PG internal binary conversion.
 *
 * Converts SQL text representation to RND-ready plaintext format.
 * For numeric types: text -> PG internal binary (int16/int32/int64/float/double/NumericData).
 * For text types:   text -> 4-byte length prefix + text (RND format).
 *
 * This is the single entry point for pre-encryption data preparation,
 * shared by all algorithm types. DET/OPE/SEARCH normalization is applied
 * on top of this result when needed.
 *
 * @param data_type   Base data type OID (INT2OID=21, INT4OID=23, etc.)
 * @param type_mod    Type modifier (atttypmod), used for NUMERIC scale
 * @param text        Null-terminated input text from SQL layer
 * @param text_len    Length of text string
 * @param binary_out  Output: pg_malloc'd PG internal binary
 * @param binary_len  Output: length of binary data
 * @return            TECC_FORMATTER_SUCCESS on success, error code otherwise
 */
extern TeccFormatterErrorCode tecc_formatter_prepare_plaintext(
    Oid data_type, int type_mod,
    const char *text, size_t text_len,
    unsigned char **binary_out, size_t *binary_len);

#endif /* TECC_TYPE_OUTPUT_H */
/* End - ReqID:TECC4.0 */
