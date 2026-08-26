/*-------------------------------------------------------------------------
 *
 * sql_security.h
 *    SQL injection and XSS detection for pg_audit.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_AUDIT_SQL_SECURITY_H
#define PG_AUDIT_SQL_SECURITY_H

extern void audit_sql_security_init(void);
extern void audit_sql_security_fini(void);
extern void audit_sql_security_block_pending(const char *query_string);

#endif
