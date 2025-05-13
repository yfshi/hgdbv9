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

#include "access/detoast.h"
#include "fmgr.h"
#include "utils/array.h"
#include "utils/guc.h"

#define VAL(CH)			((CH) - '0')
#define DIG(VAL)		((VAL) + '0')

#define BINARY_MAX_LENGTH       255
#define VARBINARY_MAX_LENGTH    65532
#define TINYBLOB_MAX_LENGTH     255
#define BLOB_MAX_LENGTH         65535
#define MEDIUMBLOB_MAX_LENGTH   16777215
#define LONGBLOB_MAX_LENGTH     4294967295

extern bytea *truncate_bytea_with_len(const bytea *b, int len);
extern bool mys_get_varbinary_bool(bytea *arg);

extern Datum binary_typmodout(PG_FUNCTION_ARGS);
extern Datum binary_typmodin(PG_FUNCTION_ARGS);
extern Datum varbinary_typmodin(PG_FUNCTION_ARGS);
extern Datum varbinary2bool(PG_FUNCTION_ARGS);
extern Datum anybinary_cmp(PG_FUNCTION_ARGS);
extern int32 anybinary_typmodin(ArrayType *ta, int64 max_length, const char *typname);
extern char *anybinary_typmodout(int32 typmod);
extern Datum anybinary_hash(PG_FUNCTION_ARGS);
extern Datum anybinary_hash_extended(PG_FUNCTION_ARGS);

#endif