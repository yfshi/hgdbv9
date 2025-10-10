/*-------------------------------------------------------------------------
 *
 * varlena.h
 *	  Functions for the variable-length built-in types.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/varlena.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VARLENA_H
#define VARLENA_H

#include "nodes/pg_list.h"
#include "utils/sortsupport.h"

/* Begin - Req: SRS-SQL-NLSSORT */
typedef bytea * (*nlssort_hook_type)(const char *src, int32_t len, Oid collid, bool trim);
extern PGDLLIMPORT nlssort_hook_type nlssort_hook;
/* End - Req: SRS-SQL-NLSSORT */

/* Begin - Req: SRS-MYSQL-CHARSET */
typedef int (*mysql_strcmp_hook_type)(unsigned char *arg1, int len1, unsigned char *arg2, int len2, Oid collid, bool trim);
extern PGDLLIMPORT mysql_strcmp_hook_type mysql_strcoll_hook;

typedef Datum (*mysql_hash_hook_type)(const unsigned char *key, int32_t len, Oid coll);
extern PGDLLIMPORT mysql_hash_hook_type mysql_hash_hook;
/* End - Req: SRS-MYSQL-CHARSET */

extern int	varstr_cmp(const char *arg1, int len1, const char *arg2, int len2, Oid collid);
extern void varstr_sortsupport(SortSupport ssup, Oid typid, Oid collid);
extern int	varstr_levenshtein(const char *source, int slen,
							   const char *target, int tlen,
							   int ins_c, int del_c, int sub_c,
							   bool trusted);
extern int	varstr_levenshtein_less_equal(const char *source, int slen,
										  const char *target, int tlen,
										  int ins_c, int del_c, int sub_c,
										  int max_d, bool trusted);
extern List *textToQualifiedNameList(text *textval);
extern bool SplitIdentifierString(char *rawstring, char separator,
								  List **namelist);
/* Begin - BUG#Z0001310 */
extern bool SplitStringIgnoreSpaceAndBracket(char *rawstring, char separator,
						 List **namelist);
/* End - BUG#Z00001310 */

/* Begin - BUG#0000049 */
extern bool SplitIdentifierStringForSearchPath(char *rawstring, char separator,
					  				List **namelist);
/* End - BUG#0000049 */
extern bool SplitDirectoriesString(char *rawstring, char separator,
								   List **namelist);
extern bool SplitGUCList(char *rawstring, char separator,
						 List **namelist);
extern text *replace_text_regexp(text *src_text, void *regexp,
								 text *replace_text, bool glob);

/* Begin - ReqID:SRS-MYSQL-BINARYTYPE */
extern int32 text_length(Datum str);
extern text *text_substring(Datum str, int32 start, int32 length, bool length_not_specified);
extern text *text_catenate(text *t1, text *t2);
/* End - ReqID:SRS-MYSQL-BINARYTYPE */

/* Begin - SRS-MYSQL-HEX-LITERAL */
extern bool isxdigits_n(const char *instr, size_t n);
/* End - SRS-MYSQL-HEX-LITERAL */
#endif
