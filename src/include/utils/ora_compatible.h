/*--------------------------------------------------------------------
 *
 * ora_compatible.h
 *
 * Definition enumeration structure is fro supporting different compatibility modes.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/utils/ora_compatible.h
 *
 * add the file for requirement "SQL PARSER"
 *
 *----------------------------------------------------------------------
 */

#ifndef ORA_COMPATIBLE_H
#define ORA_COMPATIBLE_H

#define ORA_SEARCH_PATH "sys,\"$user\", public"
#define DB_MODE_PARMATER "ivorysql.database_mode"	/* ReqID:SRS-MISC-GUC */

typedef enum DBMode
{
	DB_PG = 0,
	DB_ORACLE = 1, /* ReqID:SRS-CMD-PGDUMP */
	DB_MYSQL = 2,	/* ReqID:SRS-MISC-MYSQL-FRAME */
	DB_NATIVE = 3	/* ReqID:SRS-CMD-PGDUMP */
}DBMode;


typedef enum CompatibleMode
{
	PG_MODE = 0,
	ORA_MODE,
	MYS_MODE		/* ReqID:SRS-MISC-MYSQL-FRAME */
} CompatibleMode;

/* BEGIN - ReqID:SRS-MISC-USER */
/* BEGIN - bug#M0000261 */
typedef enum AssignDumpMode
{
	ASSIGNDUMPMODE_UNKNOWN = 0,
	ASSIGNDUMPMODE_PG,
	ASSIGNDUMPMODE_ORACLE
}AssignDumpMode;
/* END bug#M0000261 */

typedef enum DBDump
{
	PG_DUMP = 0,
	ORA_DUMP
}DBDump;

typedef enum DBGrantUser
{
	PG_GRANTUSER = 0,
	ORA_GRANTUSER
}DBGrantUser;
/* END - ReqID:SRS-MISC-USER */

/* BEGIN - case sensitive indentify */
typedef enum CaseSwitchMode
{
	NORMAL = 0,
	INTERCHANGE,
	LOWERCASE
}CaseSwitchMode;
/* END - case sensitive indentify */

/* Begin - BUG#Z091 */
typedef enum
{
	NLS_LENGTH_BYTE,
	NLS_LENGTH_CHAR
} NlsLengthSemantics;
/* End - BUG#Z091 */

/* Begin - ReqID:SRS-SQL-MERGE */
typedef enum
{
	OMERGE_TUNING_OFF = 0,
	OMERGE_TUNING_ON,
	OMERGE_TUNING_FORCE
} OraMergeTuning;
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - BUG#M0000201: support question marks as binding variables */
typedef enum
{
	FROM_UNKNOWN,
	FROM_JDBC,
	FROM_C,
	FROM_OCI,
	FROM_LIBPQ,
	FROM_ODBC	/* BUG#M0000346 */
} WhereFrom;
/* End - BUG#M0000201: support question marks as binding variables */

/* Begin - SRS-SQL-NLSSORT */
typedef enum
{
	ORA_MAX_STRING_SIZE_STANDARD,
	ORA_MAX_STRING_SIZE_EXTENDED
} MaxStringSize;
/* End - SRS-SQL-NLSSORT */

extern bool ora_strtoint64(const char *num, const char *dtype, int64 *retval);	/* BUG#M0000194 */

#endif							/* ORA_COMPATIBLE_H */

