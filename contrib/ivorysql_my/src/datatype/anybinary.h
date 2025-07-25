/*-------------------------------------------------------------------------
 *
 * anybinary.h
 *   for mysql binary type
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/anybinary.h
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#ifndef MYS_BINARY_H
#define MYS_BINARY_H

#include "utils/guc.h"

#define VAL(CH)			((CH) - '0')
#define HEX_LEN(len)		((len - 2 + 1) / 2)

extern bytea *truncate_bytea_with_len(const bytea *b, int len);
extern Datum varbinary2bool(PG_FUNCTION_ARGS);
extern Datum anybinary_cmp(PG_FUNCTION_ARGS);
extern int32 anybinary_typmodin(ArrayType *ta, int64 max_length, const char *typname);

#endif