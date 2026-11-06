/*-------------------------------------------------------------------------
 *
 * tecc_typecast.h
 *	  Type casting utilities for TECC encrypted types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/tecc_typecast.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_TYPECAST_H
#define TECC_TYPECAST_H

#include "c.h"
#include "fmgr.h"
#include "utils/cipherdatatype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Begin - ReqID:TECC4.0 */

/*
 * Type conversion functions for encrypted types
 *
 * These functions perform type conversions between encrypted types.
 * Since the server only stores encrypted data, the conversion is
 * essentially a copy operation with validation.
 */

/* int2_enc conversion functions */
extern Datum int2_enc_to_int4_enc(PG_FUNCTION_ARGS);
extern Datum int2_enc_to_int8_enc(PG_FUNCTION_ARGS);
extern Datum int2_enc_to_float4_enc(PG_FUNCTION_ARGS);
extern Datum int2_enc_to_float8_enc(PG_FUNCTION_ARGS);
extern Datum int2_enc_to_numeric_enc(PG_FUNCTION_ARGS);

/* int4_enc conversion functions */
extern Datum int4_enc_to_int8_enc(PG_FUNCTION_ARGS);
extern Datum int4_enc_to_float4_enc(PG_FUNCTION_ARGS);
extern Datum int4_enc_to_float8_enc(PG_FUNCTION_ARGS);
extern Datum int4_enc_to_numeric_enc(PG_FUNCTION_ARGS);

/* int8_enc conversion functions */
extern Datum int8_enc_to_numeric_enc(PG_FUNCTION_ARGS);

/* float4_enc conversion functions */
extern Datum float4_enc_to_float8_enc(PG_FUNCTION_ARGS);
extern Datum float4_enc_to_numeric_enc(PG_FUNCTION_ARGS);

/* float8_enc conversion functions */
extern Datum float8_enc_to_numeric_enc(PG_FUNCTION_ARGS);

/* string type conversion functions */
extern Datum text_enc_to_varchar_enc(PG_FUNCTION_ARGS);
extern Datum text_enc_to_bpchar_enc(PG_FUNCTION_ARGS);
extern Datum varchar_enc_to_text_enc(PG_FUNCTION_ARGS);
extern Datum varchar_enc_to_bpchar_enc(PG_FUNCTION_ARGS);
extern Datum bpchar_enc_to_text_enc(PG_FUNCTION_ARGS);
extern Datum bpchar_enc_to_varchar_enc(PG_FUNCTION_ARGS);

/* Internal validation functions */
extern bool is_type_conversion_allowed(Oid source_type, Oid target_type);
extern bool check_dek_consistency_for_conversion(CipherDataType *source);

/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif	/* TECC_TYPECAST_H */