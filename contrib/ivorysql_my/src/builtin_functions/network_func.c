/*-------------------------------------------------------------------------------
 *
 * File: network_func.c
 *
 * Abstract:
 * 		This file contains the implementation of MySQL's
 *		network related built-in functions.
 *
 * Authored by wangjie@highgo.com, 20250702.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/builtin_functions/network_func.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/inet.h"
#include "utils/guc.h"
#include "utils/palloc.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


PG_FUNCTION_INFO_V1(inetaton);
PG_FUNCTION_INFO_V1(inetntoa);
PG_FUNCTION_INFO_V1(inetpton);
PG_FUNCTION_INFO_V1(inetntop);


// Number of bytes in an IPv6 address (should be 16)
static const int IN6ADDRSZ = sizeof(struct in6_addr);

// Number of bytes in an IPv4 address (should be 4)
static const int INADDRSZ = sizeof(struct in_addr);

// Half the size of an IPv6 address, often used for internal calculations (8 bytes)
static const int IN6ADDRNWS = sizeof(struct in6_addr) / 2;

// Valid hexadecimal digits used for parsing IPv6 components
static const char HEXDIGS[] = "0123456789abcdef";

// Minimum length of a valid IPv4 address string (e.g., "1.1.1.1")
static const int IPV4_LEN_MIN = 7;

// Maximum length of a valid IPv4 address string (e.g., "255.255.255.255")
static const int IPV4_LEN_MAX = 15;

// Maximum number of colon (':') separators in a full IPv6 string (e.g., "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")
static const int IN6_SEP_MAX = 7;

// Minimum number of colon separators in a valid IPv6 address (e.g., "::1")
static const int IN6_SEP_MIN = 2;

// Maximum number of parts in a dot-separated IPv4 address (a.b.c.d = 4 parts)
static const int INADDPARTNUM = 4;


static bool
ptonipv4(const char *src, int length, unsigned char *address)
{
	int value = 0;
	int part_len = 0;
	int dot_num = 0;
	char c = 0;
	const char *p;
	unsigned char *bytes = address;

	/* Check if the input length is within valid IPv4 address length range */
	if (length < IPV4_LEN_MIN || length > IPV4_LEN_MAX)
		return false;

	/* Iterate through each character in the input string */
	for (p = src; (p - src) < length; p++)
	{
		c = *p;
		if (isdigit(c))
		{
			++part_len;
			/* Reject segments longer than 3 digits */
			if (part_len > INADDPARTNUM - 1)
				return false;

			value = value * 10 + (c - '0');
			/* Each segment must be <= 255 */
			if (value > 255)
				return false;
		}
		else if (c == '.')
		{
			if (part_len == 0)
				return false;

			bytes[dot_num++] = (unsigned char)value;
			value = 0;
			part_len = 0;
			/* There must be at most 3 dots in a valid IPv4 address */
			if (dot_num > INADDPARTNUM - 1)
				return false;
		}
		else
		{
			return false;
		}
	}

	/* After parsing, input must not end with a dot and must have exactly 3 dots */
	if (c == '.' || dot_num != INADDPARTNUM - 1)
		return false;

	bytes[INADDPARTNUM - 1] = (unsigned char)value;
	return true;
}

static bool
ptonipv6(const char *src, int length, char *address)
{
	const char *p;
	const char *part_begin;
	char *bytes = address;
	char *bytes_end = bytes + IN6ADDRSZ;
	char *dst = bytes;
	char *blank = NULL;
	int part_len = 0;
	int part_digit = 0;

	/* Check if the input length is within the minimum and maximum IPv6 address length */
	if (length < IN6_SEP_MIN || length > IN6ADDRNWS * 4 + IN6_SEP_MAX)
		return false;

	p = src;

	/* Handle special case: IPv6 address starting with "::" */
	if (*p == ':')
	{
		++p;
		if (*p != ':')
			return false; /* Single colon at start is invalid */
	}

	part_begin = p;

	/* Parse each character in the input string */
	for (; ((p - src) < length); p++)
	{
		char c;

		if (!*p)
			break; /* End of string */

		c = *p;
		if (c == ':')
		{
			part_begin = p + 1;  /* Next part starts after ':' */

			if (!part_len)
			{
				/* Two colons "::" found */
				if (blank)
					return false; /* More than one "::" not allowed */

				blank = dst; /* Mark the location for zero compression */
				continue;
			}

			/* Check if next part is valid */
			if (((part_begin - src) >= length) || !*(part_begin))
				return false;

			/* Check buffer overflow */
			if (dst + 2 > bytes_end)
				return false;

			/* Store the parsed 16-bit part into output buffer in network byte order */
			dst[0] = (unsigned char)(part_digit >> 8) & 0xff;
			dst[1] = (unsigned char)part_digit & 0xff;
			dst += 2;

			/* Reset current part */
			part_len = 0;
			part_digit = 0;
		}
		else if (c == '.')
		{
			int v4_start;

			if (dst + INADDRSZ > bytes_end)
				return false;

			v4_start = part_begin - src;

			/* Convert the IPv4 part to binary and store in output buffer */
			if (!ptonipv4(part_begin, length - v4_start, (unsigned char *)dst))
				return false;

			dst += INADDRSZ;  /* Move pointer after IPv4 bytes */
			part_len = 0;

			break; /* IPv6 ends here, IPv4 embedded */
		}
		else
		{
			/* Parse hex digit */
			const char *hdp = strchr(HEXDIGS, tolower(c));

			/* Invalid hex digit or too many digits in a part */
			if (!hdp || part_len >= 4)
				return false;

			/* Accumulate hex value for current part */
			part_digit <<= 4;
			part_digit |= hdp - HEXDIGS;
			Assert(part_digit <= 0xffff);
			part_len++;
		}
	}

	/* Store last part if any digits parsed */
	if (part_len > 0)
	{
		if (dst + 2 > bytes_end)
			return false;

		dst[0] = (unsigned char)(part_digit >> 8) & 0xff;
		dst[1] = (unsigned char)part_digit & 0xff;
		dst += 2;
	}

	/* Handle "::" zero compression if it exists */
	if (blank)
	{
		size_t i,bytes_to_move;

		if (dst == bytes_end)
			return false;

		bytes_to_move = dst - blank;

		/* Shift the bytes after "::" to the end of the buffer */
		for (i = 1; i <= bytes_to_move; ++i)
		{
			*(bytes_end - (ssize_t)i) = blank[bytes_to_move - i];
			blank[bytes_to_move - i] = 0;
		}
		dst = bytes_end;
	}

	if (dst < bytes_end)
		return false;

	return true;
}

/**
 * find_gap - Finds the longest consecutive sequence of zeros in an IPv6 address array.
 *
 * @src: Pointer to an array of 16-bit unsigned integers representing IPv6 address segments.
 * @pos: Pointer to an integer to store the starting index of the longest zero sequence found.
 * @length: Pointer to an integer to store the length of the longest zero sequence found.
 *
 * This function scans through the array 'src' (of length IN6ADDRNWS) to find the
 * longest contiguous sequence of zero-valued segments, which is useful for
 * IPv6 address shortening using the "::" notation.
 * 
 * The function updates *pos and *length with the start index and length of this longest zero sequence.
 */
static void
find_gap(unsigned short *src, int *pos, int *length)
{
	int tmplen = -1; /* temporary length of current zero sequence */
	int tmppos = -1; /* temporary start position of current zero sequence */

	for (int i = 0; i < IN6ADDRNWS; i++)
	{
		if (src[i] != 0)
		{
			/* Current segment is non-zero */
			if (tmppos >= 0)
			{
				if (tmplen > *length)
				{
					*pos = tmppos;
					*length = tmplen;
				}
				tmppos = -1;
				tmplen = -1;
			}
		}
		else
		{
			/* Current segment is zero */
			if (tmppos >= 0)
			{
				tmplen++;
			}
			else
			{
				tmppos = i;
				tmplen = 1;
			}
		}
	}

	/* Check at end of array if last zero sequence is longest */
	if (tmppos >= 0)
	{
		if (tmplen > *length)
			*pos = tmppos;
		*length = tmplen;
	}
	return;
}

/**
 * ntopipv4 - Convert IPv4 address from binary to dotted-decimal string format.
 *
 * @src: Pointer to an in_addr struct containing the IPv4 address in network byte order.
 * @str: Pointer to a character buffer where the resulting IPv4 string will be stored.
 *
 * This function converts the 4-byte IPv4 address pointed to by 'src' into the
 * standard dotted-decimal string representation (e.g., "192.168.0.1") and
 * stores it in the buffer 'str'.
 *
 * The buffer 'str' must be large enough to hold the IPv4 string (recommended size: INET_ADDRSTRLEN).
 */
static void
ntopipv4(const struct in_addr *src, char *str)
{
	const unsigned char *bytes = (const unsigned char *)src;
	snprintf(str, INET_ADDRSTRLEN, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);

	return;
}

/**
 * ntopipv6 - Convert IPv6 address from binary to colon-hexadecimal string format.
 *
 * @src: Pointer to an in6_addr struct containing the IPv6 address in network byte order.
 * @str: Pointer to a character buffer where the resulting IPv6 string will be stored.
 *
 * This function converts the 16-byte IPv6 address pointed to by 'src' into the
 * standard colon-separated hexadecimal string representation (e.g., "2001:0db8::1").
 * It performs zero compression as per IPv6 notation rules.
 *
 * The buffer 'str' must be large enough to hold the IPv6 string (recommended size: INET6_ADDRSTRLEN).
 */
static void
ntopipv6(const struct in6_addr *src, char *str)
{
	int pos = -1;
	int length = -1;
	char *p;
	const unsigned char *bytes = (const unsigned char *)src;
	uint16 *words = (uint16 *)palloc0(IN6ADDRNWS *sizeof(uint16));

	/* Convert 16 bytes into 8 16-bit words in network order */
	for (int i = 0; i < IN6ADDRNWS; ++i)
		words[i] = (bytes[2 * i] << 8) + bytes[2 * i + 1];

	/* Find the longest sequence of zero words for "::" compression */
	find_gap(words, &pos, &length);
	p = str;

	for (int i = 0; i < IN6ADDRNWS; ++i)
	{
		if (i == pos)
		{
			/* Insert "::" for the longest zero run */
			if (i == 0)
			{
				*p = ':';
				++p;
			}

			*p = ':';
			++p;

			i += length - 1;
		}
		/* Check for IPv4-mapped IPv6 addresses (last 32 bits IPv4) */
		else if (i == IN6ADDRNWS - 2 && pos == 0 &&
				(length == IN6ADDRNWS - 2 || (length == IN6ADDRNWS - 3 && words[IN6ADDRNWS - 3] == 0xffff)))
		{
			/* Convert last 4 bytes as IPv4 address */
			ntopipv4((const struct in_addr *)(bytes + 12), p);
			return;
		}
		else
		{
			/* Format the current 16-bit word in hex */
			int rc = 0;
			rc = snprintf(p, INET_ADDRSTRLEN, "%x", words[i]);
			if (rc < 0)
				return;
			p += rc;

			if (i != IN6ADDRNWS - 1)
			{
				*p = ':';
				++p;
			}
		}
	}
	*p = 0;
	return;
}


/*
 * inetaton - MySQL-compatible INET_ATON() function
 *
 * Converts a textual IPv4 address (e.g., '192.168.1.1') into a 64-bit integer,
 * following MySQL's behavior:
 *   - Returns NULL if input is NULL or invalid.
 *   - Supports compressed formats like '127.1' or '10.1.2'.
 *   - Handles leading/trailing spaces.
 *   - Returns result in host byte order as bigint.
 */
Datum
inetaton(PG_FUNCTION_ARGS)
{
	uint64 val;
	int n;
	char c;
	Oid argtypeid;
	char *tmpcp, *cp = NULL;
	uint *parts, *pp;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	argtypeid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (!OidIsValid(argtypeid))
		PG_RETURN_NULL();

	if (argtypeid == BOOLOID)
	{
		if (PG_GETARG_BOOL(0))
			PG_RETURN_INT64(1);
		PG_RETURN_INT64(0);
	}
	else
	{
		cp = text_to_cstring(PG_GETARG_TEXT_PP(0));
		tmpcp = cp;
	}

	if (*cp == '\0')
	{
		pfree(tmpcp);
		PG_RETURN_NULL();
	}

	parts = (uint *)palloc0(INADDRSZ * sizeof(uint));
	pp = parts;

	for (;;)
	{
		val = 0;
		while ((c = *cp) != '\0')
		{
			if (isdigit((unsigned char)c))
			{
				val = (val * 10) + (c - '0');
				if (val > 255)
				{
					PG_RETURN_NULL();
				}
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.')
		{
			if (pp >= parts + INADDRSZ - 1 || val > 0xff)
			{
				PG_RETURN_NULL();
			}

			*pp++ = val;
			cp++;
			if (!isdigit((unsigned char)*cp))
			{
				PG_RETURN_NULL();
			}
		}
		else if (*cp == '\0')
		{
			break;
		}
		else
		{
			break;
		}
	}

	while (*cp)
	{
		if (!isspace((unsigned char)*cp++))
		{
			pfree(tmpcp);
			PG_RETURN_NULL();
		}
	}

	n = pp - parts + 1;
	switch (n)
	{
		case 1: /* a -- 32 bits */
			break;
		case 2: /* a.b -- 8.24 bits */
			if (val > 0xffffff)
			{
				PG_RETURN_NULL();
			}
			val |= parts[0] << 24;
			break;
		case 3: /* a.b.c -- 8.8.16 bits */
			if (val > 0xffff)
			{
				PG_RETURN_NULL();
			}
			val |= (parts[0] << 24) | (parts[1] << 16);
			break;
		case 4: /* a.b.c.d -- 8.8.8.8 bits */
			if (val > 0xff)
			{
				PG_RETURN_NULL();
			}
			val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
			break;
		default:
			break;
	}

	pfree(tmpcp);
	PG_RETURN_INT64(val);
}

/**
 * Convert a 32-bit integer or boolean input to its IPv4 string representation.
 * 
 * Input:
 *   - Accepts either a boolean or a 64-bit integer.
 *   - Boolean true converts to "0.0.0.1", false to "0.0.0.0".
 *   - Integer must be within the valid IPv4 range (0 to UINT32_MAX).
 * 
 * Output:
 *   - Returns the IPv4 address in dotted-decimal string format.
 *   - Returns NULL if input is NULL or invalid.
 */
Datum
inetntoa(PG_FUNCTION_ARGS)
{
	Oid argtypeid;
	uint64 n = 0;
	char *res, *digit;
	unsigned char *buf;
	text *result;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	argtypeid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (!OidIsValid(argtypeid))
		PG_RETURN_NULL();

	if (argtypeid == BOOLOID)
		n = PG_GETARG_BOOL(0) ? 1 : 0;
	else
		n = PG_GETARG_INT64(0);

	if (n > UINT32_MAX)
		PG_RETURN_NULL();

	res = (char *)palloc0((IPV4_LEN_MAX + 2) * sizeof(char));
	buf = (unsigned char *)palloc0(INADDRSZ * sizeof(unsigned char));

	digit = (char *)palloc0((INADDRSZ + 1) * sizeof(char));

	*((unsigned int *)buf) = n;

	for (unsigned char *p = buf + INADDRSZ - 1; p >= buf; p--)
	{
		unsigned int c = *p;
		MemSet(digit, 0, (INADDRSZ + 1) * sizeof(char));

		snprintf(digit, sizeof(digit), "%u.", c);
		strcat(res, digit);
	}
	res[strlen(res) - 1] = '\0';
	result = cstring_to_text(res);

	pfree(buf);
	pfree(digit);
	pfree(res);
	PG_RETURN_TEXT_P(result);
}

/**
 * Convert an IPv4 or IPv6 address string to its binary form.
 * 
 * Input:
 *   - A text representation of an IPv4 or IPv6 address.
 * 
 * Output:
 *   - Returns the binary representation of the IP address as text.
 *   - Returns NULL if the input is NULL or not a valid IPv4/IPv6 address.
 * 
 * The function first attempts to parse the input as IPv4. If that fails,
 * it then attempts to parse as IPv6. If both fail, NULL is returned.
 */
Datum
inetpton(PG_FUNCTION_ARGS)
{
	char *ip = NULL;
	char *res_ipv4;
	char *res_ipv6;
	
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	res_ipv4 = (char *)palloc0(INADDRSZ * sizeof(char));
	res_ipv6 = (char *)palloc0(IN6ADDRSZ * sizeof(char));

	ip = text_to_cstring(PG_GETARG_TEXT_PP(0));

	if (ptonipv4(ip, strlen(ip), (unsigned char *)res_ipv4))
	{
		PG_RETURN_TEXT_P(cstring_to_text_with_len(res_ipv4, INADDRSZ));
	}

	if (ptonipv6(ip, strlen(ip), res_ipv6))
	{
		PG_RETURN_TEXT_P(cstring_to_text_with_len(res_ipv6, IN6ADDRSZ));
	}

	pfree(res_ipv4);
	pfree(res_ipv6);
	PG_RETURN_NULL();
}

/**
 * inetntop - Convert a binary IP address to its text representation.
 *
 * This function takes a bytea input representing an IPv4 or IPv6 address
 * in binary form, converts it to the corresponding string format,
 * and returns it as a text Datum.
 *
 * Supported input lengths:
 * - IPv4 address length (INADDRSZ, typically 4 bytes)
 * - IPv6 address length (IN6ADDRSZ, typically 16 bytes)
 */
Datum
inetntop(PG_FUNCTION_ARGS)
{
	Oid argtypeid;
	bytea *in;
	char *ip;
	int len;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	argtypeid = get_fn_expr_argtype(fcinfo->flinfo, 0);
	if (!OidIsValid(argtypeid) || argtypeid == YEAROID || argtypeid == JSONOID)
	{
		ereport(is_strict_mode() ? ERROR : WARNING,
				(errmsg("Incorrect string value for function inet6_ntoa")));
		PG_RETURN_NULL();
	}

	in = PG_GETARG_BYTEA_PP(0);
	ip = VARDATA_ANY(in);
	len = VARSIZE_ANY_EXHDR(in);

	if (len == INADDRSZ)
	{
		char res_address[INET_ADDRSTRLEN] = {0};
		ntopipv4((const struct in_addr *)ip, res_address);
		PG_RETURN_TEXT_P(cstring_to_text(res_address));
	}
	else if (len == IN6ADDRSZ)
	{
		char res_address[INET6_ADDRSTRLEN] = {0};
		ntopipv6((const struct in6_addr *)ip, res_address);
		PG_RETURN_TEXT_P(cstring_to_text(res_address));
	}
	ereport(is_strict_mode() ? ERROR : WARNING,
			(errmsg("Incorrect string value for function inet6_ntoa")));
	PG_RETURN_NULL();
}
