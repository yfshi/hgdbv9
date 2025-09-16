/* -------------------------------------------------------------------------
 *
 * File: pg_audit.h
 *
 * Abstract:
 * This file is the specific implementation of security audit.
 *
 * Authored by wanghaoyan@highgo.com, 20240327
 *
 * Copyright:
 * 	Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved
 *
 * Identification: 
 *	contrib/pg_audit/pg_audit.h
 *
 * -------------------------------------------------------------------------
 */

#ifndef  H_HGAUDIT_H
#define  H_HGAUDIT_H

#include "postgres.h"
#include "storage/lwlock.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "storage/fd.h"
#include "storage/latch.h"
#include "utils/timestamp.h"

#include <dirent.h>


#define TABLE_S "TABLE"
#define VIEW_S "VIEW"
#define COLUMN_S "COLUMN"
#define SEQUENCE_S "SEQUENCE"
#define PROCEDURE_S "PROCEDURE"
#define FUNCTION_S "FUNCTION"

/*
	审计语句类型对应的字符串
 */
#define	CREATE_ROLE_S	"CREATE ROLE"
#define	CREATE_DOMAIN_S	"CREATE DOMAIN"
#define	CREATE_PROCEDURE_S	"CREATE PROCEDURE"
#define	CREATE_INDEX_S	"CREATE INDEX"
#define	CREATE_SCHEMA_S	"CREATE SCHEMA"
#define	CREATE_SEQUENCE_S	"CREATE SEQUENCE"
#define	CREATE_TABLE_S	"CREATE TABLE"
#define	CREATE_TABLE_AS_S	"CREATE TABLE AS"
#define	CREATE_TRIGGER_S	"CREATE TRIGGER"
#define	CREATE_USER_S	"CREATE USER"
#define	CREATE_VIEW_S	"CREATE VIEW"
#define	CREATE_EXTENSION_S	"CREATE EXTENSION"
#define	CREATE_DATABASE_S	"CREATE DATABASE"
#define	ALTER_ROLE_S	"ALTER ROLE"
#define	ALTER_DOMAIN_S	"ALTER DOMAIN"
#define	ALTER_PROCEDURE_S	"ALTER PROCEDURE"
#define	ALTER_INDEX_S	"ALTER INDEX"
#define	ALTER_SYSTEM_S	"ALTER SYSTEM"
#define	ALTER_SCHEMA_S	"ALTER SCHEMA"
#define	ALTER_SEQUENCE_S	"ALTER SEQUENCE"
#define	ALTER_TABLE_S	"ALTER TABLE"
#define	ALTER_TRIGGER_S	"ALTER TRIGGER"
#define	ALTER_USER_S	"ALTER USER"
#define	ALTER_VIEW_S	"ALTER VIEW"
#define	ALTER_EXTENSION_S	"ALTER EXTENSION"
#define	ALTER_DATABASE_S	"ALTER DATABASE"
#define	DROP_DOMAIN_S	"DROP DOMAIN"
#define	DROP_PROCEDURE_S	"DROP PROCEDURE"
#define	DROP_ROLE_S	"DROP ROLE"
#define	DROP_INDEX_S	"DROP INDEX"
#define	DROP_SCHEMA_S	"DROP SCHEMA"
#define	DROP_SEQUENCE_S	"DROP SEQUENCE"
#define	DROP_TABLE_S	"DROP TABLE"
#define	DROP_TRIGGER_S	"DROP TRIGGER"
#define	DROP_USER_S	"DROP USER"
#define	DROP_VIEW_S	"DROP VIEW"
#define	DROP_DATABASE_S	"DROP DATABASE"
#define	DROP_EXTENSION_S	"DROP EXTENSION"
#define	SELECT_S	"SELECT"
#define	SELECT_INTO_S	"SELECT INTO"
#define	INSERT_S	"INSERT"
#define	UPDATE_S	"UPDATE"
#define	DELETE_S	"DELETE"
#define	GRANT_S	"GRANT"
#define GRANT_ROLE_S "GRANT ROLE"
#define	REVOKE_S	"REVOKE"
#define	REVOKE_ROLE_S	"REVOKE ROLE"
#define	COMMENT_S	"COMMENT"
#define	RESET_S	"RESET"
#define	SET_S	"SET"
#define	TRUNCATE_S	"TRUNCATE"
#define	COPY_TO_S	"COPY TO"
#define	COPY_FROM_S	"COPY FROM"
#define	REINDEX_S	"REINDEX"
#define INVALID_STMT_S "INVALID STMT"
#define LOCK_S	"LOCK"
#define CALL_S	"CALL"
#define AUDIT_S	"AUDIT"
#define NOAUDIT_S	"NOAUDIT"
#define CREATE_FUNCTION_S  "CREATE FUNCTION"
#define DROP_FUNCTION_S  "DROP FUNCTION"
#define ALTER_FUNCTION_S	"ALTER FUNCTION"
#define CREATE_TABLESPACE_S	"CREATE TABLESPACE"
#define ALTER_TABLESPACE_S	"ALTER TABLESPACE"
#define DROP_TABLESPACE_S	"DROP TABLESPACE"
#define CREATE_MATLIZED_VIEW_S "CREATE MATERIALIZED VIEW"
#define ALTER_MATLIZED_VIEW_S "ALTER MATERIALIZED VIEW"
#define DROP_MATLIZED_VIEW_S "DROP MATERIALIZED VIEW"
#define BEGIN_S "BEGIN"
#define COMMIT_S "COMMIT"
#define START_TRANSACTION_S "START TRANSACTION"
#define ROLLBACK_S "ROLLBACK"
#define SAVEPOINT_S "SAVEPOINT"
#define ROLLBACK_TO_S "ROLLBACK TO"
#define ROLLBACK_PREPARED_S "ROLLBACK PREPARED"
#define RELEASE_S	"RELEASE"
#define RELEASE_SAVEPOINT_S "RELEASE SAVEPOINT"

#define ALL_S			"ALL"

#define ALTER_S			"ALTER"
#define DROP_S			"DROP"
#define CREATE_S		"CREATE"

#define PG_AUDIT_LOG "pg_audit_log"
#define SQLITE_AUDIT_LOG "sqlite_audit_log"

/*
审计语句类型
*/

#define AUDIT_TYPE_SYSYEM_S "system"
#define AUDIT_TYPE_STATEMENT_S "statement"
#define AUDIT_TYPE_OBJECT_S "object"

bool receive_sighup;

#define AUDIT_TYPE_MANDATORY_S "mandatory"

typedef enum AuditChange
{
	AUDIT_UNABLE = 0,
	AUDIT_EANBLE = 1
}AuditChange;

#define SUCCESSFUL_S "SUCCESSFUL"
#define FAIL_S "FAIL"
#define CASCADE_S "CASCADE"

#define OFF_S "OFF"
#define SUCC_S "SUCC"

#define AUDIT_MB (1024*1024)


typedef enum Auditcheck
{
	AUDIT_SUCC = '1',
	AUDIT_NOT_SUCC,
	AUDIT_SUCC_CAS,
	AUDIT_NOT_SUCC_CAS
}Auditcheck;



typedef struct AuditObjects
{
	ObjectType			objtype;		// objtype
	List		*objlist;
}AuditObjects;

/*
审计对象信息
*/
typedef struct ObjectInfo
{
	char* 			schema;
	char* 			object;
	char* 			attr;
	Oid				objectid;
	Oid				schemaid;
	AttrNumber		attnum;
	//ObjectType		objectType;
}ObjectInfo;


typedef struct dirs_fctx
{
	DIR 	   *auditDir;
	DIR 	   *auditArchDir;
	char	   *auditpath;
}dirs_fctx;


typedef enum AduitTupleType
{
	STMT_TUPLE,
	OBJ_TUPLE
}AduitTupleType;




/* 审计用户与被审计用户关系链表 */
typedef struct AuditUser
{
	Oid roleoid;		//审计用户角色oid
	char *auditrolename;//审计用户角色名
	List *auditedrole;	//被审计用户名
	struct AuditUser *next;//下一个节点
}AuditUser;

/* 审计用户与被审计用户关系链表 */
typedef struct AuditedOid
{
	bool is_common;
	Oid roledoid;		//审计用户角色oid
	Oid auditoid[3];
	struct AuditedOid *next;//下一个节点
}AuditedOid;


#endif
