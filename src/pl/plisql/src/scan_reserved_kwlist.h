/*-------------------------------------------------------------------------
 *
 * gram_reserved_kwlist.h
 *
 * The keyword lists are kept in their own source files for use by
 * automatic tools.  The exact representation of a keyword is determined
 * by the SCAN_KEYWORD macro, which is not defined in this file; it can
 * be defined by the caller for special purposes.
 *
 * Portions Copyright (c) 2024, IvorySQL pro
 *
 * src/pl/plisql/src/scan_reserved_kwlist.h
 *
 *-------------------------------------------------------------------------
 */
 
/* Begin - BUG #M0000304 */
typedef struct ScanKeyword
{
	const char *name;			/* in lower case */
	//int16		value;			/* grammar's token code */
	int16		category;		/* see codes above */
} ScanKeyword;

#define SCAN_KEYWORD(a,b) {a,b},

static const ScanKeyword scan_reserved_keywords[] = {
	SCAN_KEYWORD("all", RESERVED_KEYWORD)
	SCAN_KEYWORD("analyse", RESERVED_KEYWORD)		/* British spelling */
	SCAN_KEYWORD("analyze", RESERVED_KEYWORD)
	SCAN_KEYWORD("and", RESERVED_KEYWORD)
	SCAN_KEYWORD("any", RESERVED_KEYWORD)
	SCAN_KEYWORD("array", RESERVED_KEYWORD)
	SCAN_KEYWORD("as", RESERVED_KEYWORD)
	SCAN_KEYWORD("asc", RESERVED_KEYWORD)
	SCAN_KEYWORD("asymmetric", RESERVED_KEYWORD)
	SCAN_KEYWORD("binary_double_infinity", RESERVED_KEYWORD) /* BUG:Z092 */
	SCAN_KEYWORD("binary_double_nan", RESERVED_KEYWORD) /* BUG:Z092 */
	SCAN_KEYWORD("binary_float_infinity", RESERVED_KEYWORD) /* BUG:Z092 */
	SCAN_KEYWORD("binary_float_nan", RESERVED_KEYWORD) /* BUG:Z092 */
	SCAN_KEYWORD("both", RESERVED_KEYWORD)
	SCAN_KEYWORD("case", RESERVED_KEYWORD)
	SCAN_KEYWORD("cast", RESERVED_KEYWORD)
	SCAN_KEYWORD("check", RESERVED_KEYWORD)
	SCAN_KEYWORD("collate", RESERVED_KEYWORD)
	SCAN_KEYWORD("column", RESERVED_KEYWORD)
	SCAN_KEYWORD("constraint", RESERVED_KEYWORD)
	SCAN_KEYWORD("create", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_catalog", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_date", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_role", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_time", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_timestamp", RESERVED_KEYWORD)
	SCAN_KEYWORD("current_user", RESERVED_KEYWORD)
	SCAN_KEYWORD("databaseuser", RESERVED_KEYWORD)			/* ReqID:SRS-MISC-USER */
	SCAN_KEYWORD("default", RESERVED_KEYWORD)
	SCAN_KEYWORD("deferrable", RESERVED_KEYWORD)
	SCAN_KEYWORD("desc", RESERVED_KEYWORD)
	SCAN_KEYWORD("distinct", RESERVED_KEYWORD)
	SCAN_KEYWORD("do", RESERVED_KEYWORD)
	SCAN_KEYWORD("else", RESERVED_KEYWORD)
	SCAN_KEYWORD("end", RESERVED_KEYWORD)
	SCAN_KEYWORD("except", RESERVED_KEYWORD)
	SCAN_KEYWORD("false", RESERVED_KEYWORD)
	SCAN_KEYWORD("fetch", RESERVED_KEYWORD)
	SCAN_KEYWORD("for", RESERVED_KEYWORD)
	SCAN_KEYWORD("foreign", RESERVED_KEYWORD)
	SCAN_KEYWORD("from", RESERVED_KEYWORD)
	SCAN_KEYWORD("grant", RESERVED_KEYWORD)
	SCAN_KEYWORD("group", RESERVED_KEYWORD)
	SCAN_KEYWORD("having", RESERVED_KEYWORD)
	SCAN_KEYWORD("in", RESERVED_KEYWORD)
	SCAN_KEYWORD("infinite", RESERVED_KEYWORD)	/* BUG:Z093 */
	SCAN_KEYWORD("initially", RESERVED_KEYWORD)
	SCAN_KEYWORD("instanceuser", RESERVED_KEYWORD)			/* ReqID:SRS-MISC-USER */
	SCAN_KEYWORD("intersect", RESERVED_KEYWORD)
	SCAN_KEYWORD("into", RESERVED_KEYWORD)
	SCAN_KEYWORD("lateral", RESERVED_KEYWORD)
	SCAN_KEYWORD("leading", RESERVED_KEYWORD)
	SCAN_KEYWORD("limit", RESERVED_KEYWORD)
	SCAN_KEYWORD("localtime", RESERVED_KEYWORD)
	SCAN_KEYWORD("localtimestamp", RESERVED_KEYWORD)
	SCAN_KEYWORD("nan", RESERVED_KEYWORD) /* BUG:Z093 */
	SCAN_KEYWORD("nocopy", RESERVED_KEYWORD) /* ReqID:SRS-SQL-PROC */
	SCAN_KEYWORD("nocycle", RESERVED_KEYWORD)
	SCAN_KEYWORD("not", RESERVED_KEYWORD)
	SCAN_KEYWORD("null", RESERVED_KEYWORD)
	SCAN_KEYWORD("offset", RESERVED_KEYWORD)
	SCAN_KEYWORD("on", RESERVED_KEYWORD)
	SCAN_KEYWORD("only", RESERVED_KEYWORD)
	SCAN_KEYWORD("or", RESERVED_KEYWORD)
	SCAN_KEYWORD("order", RESERVED_KEYWORD)
	SCAN_KEYWORD("placing", RESERVED_KEYWORD)
	SCAN_KEYWORD("primary", RESERVED_KEYWORD)
	SCAN_KEYWORD("references", RESERVED_KEYWORD)
	SCAN_KEYWORD("returning", RESERVED_KEYWORD)
	SCAN_KEYWORD("rownum", RESERVED_KEYWORD)		/* ReqID:SRS-SQL-ROWNUM */
	SCAN_KEYWORD("select", RESERVED_KEYWORD)
	SCAN_KEYWORD("session_user", RESERVED_KEYWORD)
	SCAN_KEYWORD("some", RESERVED_KEYWORD)
	SCAN_KEYWORD("symmetric", RESERVED_KEYWORD)
	SCAN_KEYWORD("table", RESERVED_KEYWORD)
	SCAN_KEYWORD("then", RESERVED_KEYWORD)
	SCAN_KEYWORD("to", RESERVED_KEYWORD)
	SCAN_KEYWORD("trailing", RESERVED_KEYWORD)
	SCAN_KEYWORD("true", RESERVED_KEYWORD)
	SCAN_KEYWORD("union", RESERVED_KEYWORD)
	SCAN_KEYWORD("unique", RESERVED_KEYWORD)
	SCAN_KEYWORD("user", RESERVED_KEYWORD)
	SCAN_KEYWORD("using", RESERVED_KEYWORD)
	SCAN_KEYWORD("values", RESERVED_KEYWORD)
	SCAN_KEYWORD("variadic", RESERVED_KEYWORD)
	SCAN_KEYWORD("when", RESERVED_KEYWORD)
	SCAN_KEYWORD("where", RESERVED_KEYWORD)
	SCAN_KEYWORD("window", RESERVED_KEYWORD)
	SCAN_KEYWORD("with", RESERVED_KEYWORD)
};

static const int num_scan_reserved_keywords = lengthof(scan_reserved_keywords);

typedef enum
{
       PLISQL_IS_INVALID,
       PLISQL_IS_MABY_PROCESS,
       PLISQL_IS_NO_BRACKETS_PROCESS,
       PLISQL_IS_PROCESS,
       PLISQL_NOT_PROCESS
} PLiSQL_CALL;
/* End - BUG #M0000304 */
