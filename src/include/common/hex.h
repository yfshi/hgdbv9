/*------------------------------------------------------------------------
 *
 *	hex.h
 *	  Encoding and decoding routines for hex strings.
 *
 *	Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *		  src/include/common/hex.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *------------------------------------------------------------------------
 */

#ifndef COMMON_HEX_H
#define COMMON_HEX_H

extern uint64 pg_hex_decode(const char *src, size_t srclen,
							char *dst, size_t dstlen);
extern uint64 pg_hex_encode(const char *src, size_t srclen,
							char *dst, size_t dstlen);
extern uint64 pg_hex_enc_len(size_t srclen);
extern uint64 pg_hex_dec_len(size_t srclen);

/* Begin - SRS-HEX-LITERAL */
extern char mys_get_hex(const char *cp);
extern uint64 mys_hex_decode(const char *src, size_t srclen, char *dst);
/* End - SRS-HEX-LITERAL */
#endif							/* COMMON_HEX_H */
