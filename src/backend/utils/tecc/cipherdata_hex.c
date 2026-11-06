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

/*
 * hex_char_to_int - Convert a hex character to integer value
 */
static int
hex_char_to_int(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1; /* Invalid character */
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

	/* Validate hex string */
	if (!is_valid_hex_string(hex_str))
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

	/* Convert each byte to two hex characters */
	for (i = 0; i < len; i++)
	{
		appendStringInfo(&buf, "%02x", data[i]);
	}

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
	Size rnd_len, det_len, ope_len, search_len;

	if (hex_str == NULL)
		return NULL;

	/* Convert hex string to binary */
	binary_data = hex_to_binary(hex_str, &binary_len);
	if (binary_data == NULL)
		return NULL;

	/* Check minimum size (CIPHERDATA_FIXED_SIZE = 24 bytes, includes vl_len_) */
	if (binary_len < CIPHERDATA_FIXED_SIZE)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Parse header (now includes vl_len_ at offset 0) */
	ptr = (char *) binary_data;
	ptr += sizeof(int32);	/* skip vl_len_ */
	version = *(int32 *) ptr;
	ptr += sizeof(int32);
	dek_dist_oid = *(Oid *) ptr;
	ptr += sizeof(Oid);
	rnd_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	det_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	ope_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	search_len = *(int32 *) ptr;
	ptr += sizeof(int32);

	/* Validate total size */
	if (binary_len != CIPHERDATA_FIXED_SIZE + rnd_len + det_len + ope_len + search_len)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Allocate CipherDataType */
	result = cipherdata_alloc(rnd_len, det_len, ope_len, search_len, dek_dist_oid);
	if (result == NULL)
	{
		pfree(binary_data);
		return NULL;
	}

	/* Set version */
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

	/* Free binary data */
	pfree(binary_data);

	return result;
}

/*
 * cipherdata_to_hex - Convert CipherDataType to hexadecimal string
 */
char *
cipherdata_to_hex(const CipherDataType *cipherdata)
{
	size_t total_size;
	char *binary_buf;
	char *ptr;
	char *hex_str;

	if (cipherdata == NULL || !cipherdata_validate(cipherdata))
		return NULL;

	/* Calculate total size for encoding (now includes vl_len_) */
	total_size = CIPHERDATA_FIXED_SIZE +	/* 28 bytes: vl_len_ + version + dek_dist_oid + 4 lengths */
				 CIPHERDATA_RND_LENGTH(cipherdata) +
				 CIPHERDATA_DET_LENGTH(cipherdata) +
				 CIPHERDATA_OPE_LENGTH(cipherdata) +
				 CIPHERDATA_SEARCH_LENGTH(cipherdata);

	/* Allocate buffer for binary data */
	binary_buf = (char *) palloc(total_size);
	if (binary_buf == NULL)
		return NULL;

	/* Write header (now includes vl_len_ to match CipherDataType struct) */
	ptr = binary_buf;
	SET_VARSIZE(ptr, total_size);
	ptr += sizeof(int32);
	*(int32 *) ptr = cipherdata->version;
	ptr += sizeof(int32);
	*(Oid *) ptr = CIPHERDATA_DEK_DIST_OID(cipherdata);
	ptr += sizeof(Oid);
	*(int32 *) ptr = CIPHERDATA_RND_LENGTH(cipherdata);
	ptr += sizeof(int32);
	*(int32 *) ptr = CIPHERDATA_DET_LENGTH(cipherdata);
	ptr += sizeof(int32);
	*(int32 *) ptr = CIPHERDATA_OPE_LENGTH(cipherdata);
	ptr += sizeof(int32);
	*(int32 *) ptr = CIPHERDATA_SEARCH_LENGTH(cipherdata);
	ptr += sizeof(int32);

	/* Write ciphertexts */
	if (CIPHERDATA_RND_LENGTH(cipherdata) > 0)
	{
		memcpy(ptr, CIPHERDATA_RND_PTR(cipherdata), CIPHERDATA_RND_LENGTH(cipherdata));
		ptr += CIPHERDATA_RND_LENGTH(cipherdata);
	}

	if (CIPHERDATA_DET_LENGTH(cipherdata) > 0)
	{
		memcpy(ptr, CIPHERDATA_DET_PTR(cipherdata), CIPHERDATA_DET_LENGTH(cipherdata));
		ptr += CIPHERDATA_DET_LENGTH(cipherdata);
	}

	if (CIPHERDATA_OPE_LENGTH(cipherdata) > 0)
	{
		memcpy(ptr, CIPHERDATA_OPE_PTR(cipherdata), CIPHERDATA_OPE_LENGTH(cipherdata));
		ptr += CIPHERDATA_OPE_LENGTH(cipherdata);
	}

	if (CIPHERDATA_SEARCH_LENGTH(cipherdata) > 0)
	{
		memcpy(ptr, CIPHERDATA_SEARCH_PTR(cipherdata), CIPHERDATA_SEARCH_LENGTH(cipherdata));
		ptr += CIPHERDATA_SEARCH_LENGTH(cipherdata);
	}

	/* Convert binary to hex */
	hex_str = binary_to_hex((unsigned char *) binary_buf, total_size);

	/* Free binary buffer */
	pfree(binary_buf);

	return hex_str;
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