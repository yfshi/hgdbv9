/*-------------------------------------------------------------------------
 *
 * sql_security.h
 *    SQL injection and XSS detection for pg_audit.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SQL_SENTINEL_H
#define PG_SQL_SENTINEL_H

extern void sql_sentinel_init(void);
extern void sql_sentinel_fini(void);
extern void sql_sentinel_block_pending(const char *query_string);

#endif

