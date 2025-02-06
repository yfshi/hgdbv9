/*-------------------------------------------------------------------------
 *
 * hg_utf8_collate.h
 *	  routines to support manipulation of the pg_collation relation
 *
 * Portions Copyright (c) 2023, Highgo ltd. MySQL compatible Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/include/catalog/hg_utf8_collate.h
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_UTF8_COLLATION_H
#define HG_UTF8_COLLATION_H

int varstr_cmp_by_builtin_collations(char* arg1, int len1, char* arg2, int len2, Oid collid);
Datum hash_text_by_builtin_colltions(const unsigned char *key, size_t len, Oid collid);
void check_binary_collation(Oid collation, Oid type_oid);
bool is_support_mysql_format_collation(Oid collation);
Oid binary_need_transform_typeid(Oid typeoid, Oid* collation);
int matchtext_utf8mb4(unsigned char* t, int tlen, unsigned char* p, int plen);
bool is_mysql_format_collation(Oid collation);
extern bool texteq_with_collation(PG_FUNCTION_ARGS);


#define IS_UTF8_GENERAL_COLLATION(colloid) \
	((colloid == UTF8MB4_GENERAL_CI_COLLATION_OID) || \
	 (colloid == UTF8MB4_UNICODE_CI_COLLATION_OID) || \
	 (colloid == UTF8_GENERAL_CI_COLLATION_OID) || \
	 (colloid == UTF8_UNICODE_CI_COLLATION_OID))

#define COLLATION_IN_MYSQL_FORMAT(colloid) \
	((colloid) >= BINARY_COLLATION_OID && (colloid) <= UTF8_BIN_COLLATION_OID)

#define COLLATION_HAS_INVALID_ENCODING(colloid) \
	((colloid) < BINARY_COLLATION_OID)


#endif   /* HG_UTF8_COLLATION_H */
