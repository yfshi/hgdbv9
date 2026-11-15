/*-------------------------------------------------------------------------
 *
 * cipherdata_hex.c
 *	  Hexadecimal encoding/decoding utilities for CipherDataType
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/utils/tecc/cipherdata_hex.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/tecc/cipherdata_hex.h"
#include "utils/cipherdatatype.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "utils/memutils.h"

/* Begin - ReqID:TECC4.0 */

/*
 * is_valid_hex_string - Validate hexadecimal string format
 *
 * Returns true if the string is a valid hexadecimal string.
 * Supports strings with or without PostgreSQL hex prefix ("\x").
 */
bool
is_valid_hex_string(const char *str)
{
	const char *p;
	size_t len;

	if (str == NULL)
		return false;

	/* Strip prefix if present */
	p = strip_hex_prefix(str);
	len = strlen(p);

	/* Empty string is not valid */
	if (len == 0)
		return false;

	/* Length must be even (each byte needs two hex characters) */
	if (len % 2 != 0)
		return false;

	/* Validate each character is a valid hex digit */
	for (; *p != '\0'; p++)
	{
		if (!((*p >= '0' && *p <= '9') ||
			  (*p >= 'a' && *p <= 'f') ||
			  (*p >= 'A' && *p <= 'F')))
			return false;
	}

	return true;
}

/* ReqID:TECC4.0 - hex 解码 LUT：字符 → 值（'0'-'9'→0-9、'a'-'f'/'A'-'F'→10-15，
 * 非法字符→-1）。替代 hex_char_to_int 的 3 次 if 比较；注意不能用指定初始化器
 * （未列项默认 0 会把非法字符误判为合法），故显式列出全部 256 项 */
static const signed char tecc_hex_value[256] = {
	/* 0x00-0x2F: 非法 */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 0x30-0x39: '0'-'9' → 0-9；0x3A-0x40 非法 */
	0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
	/* 0x41-0x46: 'A'-'F' → 10-15；0x47-0x60 非法 */
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 0x61-0x66: 'a'-'f' → 10-15；0x67-0x7F 非法 */
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 0x80-0xFF: 非法 */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

/*
 * hex_char_to_int - Convert a hex character to integer value
 */
static int
hex_char_to_int(char c)
{
	/* ReqID:TECC4.0 - LUT 查表（单次数组访问），替代 3 次 if 比较链 */
	return tecc_hex_value[(unsigned char) c];
}

/*
 * hex_to_binary - Convert hexadecimal string to binary data
 */
unsigned char *
hex_to_binary(const char *hex_str, size_t *len)
{
	const char *p;
	unsigned char *result;
	size_t hex_len;
	size_t i;

	if (hex_str == NULL || len == NULL)
		return NULL;

	/* Strip prefix if present */
	p = strip_hex_prefix(hex_str);
	hex_len = strlen(p);

	/* ReqID:TECC4.0 - 长度校验（空串/奇数长度）替代 is_valid_hex_string 的
	 * 独立全串扫描；字符合法性由下方 LUT 转换时检测（非法→-1），避免两次遍历 */
	if (hex_len == 0 || hex_len % 2 != 0)
		return NULL;

	/* Calculate binary length (2 hex chars per byte) */
	*len = hex_len / 2;

	/* Allocate memory for binary data */
	result = (unsigned char *) palloc(*len);
	if (result == NULL)
		return NULL;

	/* Convert hex to binary */
	for (i = 0; i < *len; i++)
	{
		int high = hex_char_to_int(p[i * 2]);
		int low = hex_char_to_int(p[i * 2 + 1]);

		if (high < 0 || low < 0)
		{
			pfree(result);
			return NULL;
		}

		result[i] = (high << 4) | low;
	}

	return result;
}

/* ReqID:TECC4.0 - hex 编码 LUT：字节 → 2 个 hex 字符。
 * 服务端 cipherdata_out 输出路径（binary_to_hex）占 int4_scan 文本协议
 * 耗时 81.6%（perf 实测），LUT 查表替代逐字节 %02x printf 格式化可提升 5-10 倍 */
static const char tecc_hex_digits[] = "0123456789abcdef";

/*
 * binary_to_hex - Convert binary data to hexadecimal string
 */
char *
binary_to_hex(const unsigned char *data, size_t len)
{
	StringInfoData buf;
	size_t i;

	if (data == NULL)
		return NULL;

	/* Initialize string buffer with prefix */
	initStringInfo(&buf);
	appendStringInfoString(&buf, HEX_PREFIX);

	/* ReqID:TECC4.0 - 一次性扩容，避免逐字节 realloc 检查 */
	enlargeStringInfo(&buf, len * 2 + 1);

	/* ReqID:TECC4.0 - LUT 查表：每字节 2 次数组访问，
	 * 替代 appendStringInfo("%02x") 逐字节 printf 格式化 */
	for (i = 0; i < len; i++)
	{
		unsigned char b = data[i];
		buf.data[buf.len++] = tecc_hex_digits[b >> 4];
		buf.data[buf.len++] = tecc_hex_digits[b & 0x0F];
	}
	buf.data[buf.len] = '\0';

	return buf.data;
}

/*
 * hex_to_cipherdata - Convert hexadecimal string to CipherDataType
 */
CipherDataType *
hex_to_cipherdata(const char *hex_str)
{
	size_t binary_len;
	unsigned char *binary_data;
	CipherDataType *result;
	char *ptr;
	int32 version;
	Oid dek_dist_oid;
	int32 rnd_len, det_len, ope_len, search_len;
	int32 eg_add_len, eg_mul_len;

	if (hex_str == NULL)
		return NULL;

	/* Convert hex string to binary */
	binary_data = hex_to_binary(hex_str, &binary_len);
	if (binary_data == NULL)
		return NULL;

	/* Check minimum size: vl_len_ + version + dek_dist_oid */
	if (binary_len < CIPHERDATA_MIN_SIZE)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Parse base header (binary_data includes vl_len_ at offset 0) */
	ptr = (char *) binary_data;
	ptr += sizeof(int32);	/* skip vl_len_ */
	version = *(int32 *) ptr;	/* packed: major|mask */
	ptr += sizeof(int32);
	dek_dist_oid = *(Oid *) ptr;
	ptr += sizeof(Oid);

	/* ReqID:TECC4.0 - The length fields live in the header the version
	 * describes, so the buffer must be able to hold that header before we read
	 * them; the minimum-size test above only covers version + DEK id. */
	if (binary_len < CIPHERDATA_HEADER_SIZE(version))
	{
		pfree(binary_data);
		return NULL;
	}

	/* ReqID:TECC4.0 - Read length fields.
	 * v1.0: always four base lengths, never EG.
	 * Legacy fixed v2.0: all six lengths, with no segment mask.
	 * Compact v2.0: one length per segment whose mask bit is set. */
	if (CIPHERDATA_MAJOR_VERSION(version) == CIPHERDATA_VERSION_1_0)
	{
		rnd_len = *(int32 *) ptr;	ptr += sizeof(int32);
		det_len = *(int32 *) ptr;	ptr += sizeof(int32);
		ope_len = *(int32 *) ptr;	ptr += sizeof(int32);
		search_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_add_len = 0;
		eg_mul_len = 0;
	}
	else if (CIPHERDATA_IS_LEGACY_FIXED_V2(version))
	{
		if (binary_len < CIPHERDATA_HEADER_SIZE_V2_FIXED)
		{
			pfree(binary_data);
			return NULL;
		}
		rnd_len = *(int32 *) ptr;	ptr += sizeof(int32);
		det_len = *(int32 *) ptr;	ptr += sizeof(int32);
		ope_len = *(int32 *) ptr;	ptr += sizeof(int32);
		search_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_add_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_mul_len = *(int32 *) ptr;	ptr += sizeof(int32);
	}
	else
	{
		uint16	mask = CIPHERDATA_SEG_MASK(version);

		rnd_len = (mask & CIPHERDATA_SEG_RND) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_RND)		ptr += sizeof(int32);
		det_len = (mask & CIPHERDATA_SEG_DET) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_DET)		ptr += sizeof(int32);
		ope_len = (mask & CIPHERDATA_SEG_OPE) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_OPE)		ptr += sizeof(int32);
		search_len = (mask & CIPHERDATA_SEG_SEARCH) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_SEARCH)	ptr += sizeof(int32);
		eg_add_len = (mask & CIPHERDATA_SEG_EG_ADD) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_EG_ADD)	ptr += sizeof(int32);
		eg_mul_len = (mask & CIPHERDATA_SEG_EG_MUL) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_EG_MUL)	ptr += sizeof(int32);
	}

	/* ReqID:TECC4.0 - Reject negative segment lengths: they drive both the
	 * allocation size and the payload offsets, so a negative value would shift
	 * every later segment out of the structure. */
	if (rnd_len < 0 || det_len < 0 || ope_len < 0 || search_len < 0 ||
		eg_add_len < 0 || eg_mul_len < 0)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Validate total size.  ReqID:TECC4.0 - Computed in int64: in int32 an
	 * overflowing sum wraps onto the buffer length and lets an absurd segment
	 * length reach the copies. */
	{
		int64		expected = (int64) CIPHERDATA_HEADER_SIZE(version) +
			(int64) rnd_len + (int64) det_len + (int64) ope_len +
			(int64) search_len + (int64) eg_add_len + (int64) eg_mul_len;

		if ((int64) binary_len != expected)
		{
			pfree(binary_data);
			return NULL;
		}
	}

	/* ReqID:TECC4.0 - A mask that disagrees with the lengths actually parsed
	 * would send the copies below past the end of the structure. */
	if (!cipherdata_parsed_layout_ok(version, rnd_len, det_len, ope_len,
									 search_len, eg_add_len, eg_mul_len))
	{
		pfree(binary_data);
		return NULL;
	}

	/* Allocate CipherDataType.  ReqID:TECC4.0 - Reserve the length fields the
	 * kept version implies, not only those of the non-empty segments. */
	result = cipherdata_alloc_for_version(rnd_len, det_len, ope_len, search_len,
										  eg_add_len, eg_mul_len,
										  dek_dist_oid, version);
	if (result == NULL)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Set version */
	/* Modify - ReqID:TECC4.0 - Keep the compact version generated by the
	 * allocator when importing a legacy fixed V2 value. */
	if (!CIPHERDATA_IS_LEGACY_FIXED_V2(version))
		result->version = version;

	/* Copy ciphertexts */
	if (rnd_len > 0)
		memcpy(CIPHERDATA_RND_PTR(result), ptr, rnd_len);
	ptr += rnd_len;

	if (det_len > 0)
		memcpy(CIPHERDATA_DET_PTR(result), ptr, det_len);
	ptr += det_len;

	if (ope_len > 0)
		memcpy(CIPHERDATA_OPE_PTR(result), ptr, ope_len);
	ptr += ope_len;

	if (search_len > 0)
		memcpy(CIPHERDATA_SEARCH_PTR(result), ptr, search_len);

	/* ReqID:TECC4.0 - Copy EG ciphertexts */
	ptr += search_len;
	if (eg_add_len > 0)
		memcpy(CIPHERDATA_EG_ADD_PTR(result), ptr, eg_add_len);
	ptr += eg_add_len;
	if (eg_mul_len > 0)
		memcpy(CIPHERDATA_EG_MUL_PTR(result), ptr, eg_mul_len);

	/* Free binary data */
	pfree(binary_data);

	return result;
}

/*
 * strip_hex_prefix - Strip PostgreSQL hex prefix if present
 */
const char *
strip_hex_prefix(const char *hex_str)
{
	if (has_hex_prefix(hex_str))
		return hex_str + HEX_PREFIX_LEN;
	return hex_str;
}

/*
 * has_hex_prefix - Check if string has PostgreSQL hex prefix
 */
bool
has_hex_prefix(const char *hex_str)
{
	if (hex_str == NULL)
		return false;

	if (strlen(hex_str) < HEX_PREFIX_LEN)
		return false;

	return (hex_str[0] == '\\' && hex_str[1] == 'x');
}

/* End - ReqID:TECC4.0 */
