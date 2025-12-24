/*-------------------------------------------------------------------------
 *
 * scansup.h
 *	  scanner support routines used by the core lexer
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/scansup.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef SCANSUP_H
#define SCANSUP_H

extern char *downcase_truncate_identifier(const char *ident, int len,
										  bool warn);

extern char *downcase_identifier(const char *ident, int len,
								 bool warn, bool truncate);

/* BEGIN - MySQL case sensitive indentify */
extern char *original_truncate_identifier(const char *ident, int len,
										   bool warn);
extern char *original_identifier(const char *ident, int len,
								 bool warn, bool truncate);
/* END - MySQL case sensitive indentify */

/* BEGIN - case sensitive indentify */
extern char *upcase_identifier(const char *ident, int len,
							   bool warn, bool truncate);
extern char *identifier_case_transform(const char *ident, int len);
extern bool identifier_is_all_lower(const char *ident, int len);
extern bool identifier_is_all_upper(const char *ident, int len);
/* END - case sensitive indentify */

extern char *string_case_transform(const char *ident, int len); /* BUG#Z1423 */
extern void truncate_identifier(char *ident, int len, bool warn);

extern bool scanner_isspace(char ch);

#endif							/* SCANSUP_H */
