/*-------------------------------------------------------------------------
 *
 * File: pg_contrib_audit.h
 *
 * Abstract: 
 *	  definition of highgo hooks
 *
 * Authored by wanghaoyan@highgo.com 20240401.
 *
 * Copyright (c) 2009-2024 HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *      src/include/pg_contrib_audit.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_CONTRIB_AUDIT_H
#define PG_CONTRIB_AUDIT_H

#include "utils/palloc.h"
#include "libpq/libpq-be.h"
#include "utils/relcache.h"
#include "nodes/parsenodes.h"

#define MAC_LEN 32
#define CLIENTNAME_LEN 64
#define CLIENTIP_LEN 64
#define CLIENTPORT_LEN 32
#define AUTHMETHOD_LEN 16

#define InvalidObjType  (-2)
#define MAX_OBJECTOIDS 1024
#define MAX_NCONFOIDS 1024

#define START_S		"START"
#define STOP_S		"STOP"
#define	LOGIN_S		"LOGIN"
#define LOGOUT_S	"LOGOUT"
#define RELOAD_S	"RELOAD"
#define ALTER_GUC_S "ALTER GUC"
#define AUDIT_STORAGE_INVALID_S "AUDIT STORAGE INVALID"
#define	IMPORT_S	"IMPORT"
#define EXPORT_S	"EXPORT"
#define MANDATORY_AUDIT_ON_S "AUDIT ON"
#define MANDATORY_AUDIT_OFF_S "AUDIT OFF"
#define LOCK_ACCOUNT_S			"LOCK ACCOUNT"
#define SWITCHOVER_S			"SWITCH OVER"
#define PG_DUMP_S		"PG_DUMP"
#define PG_DUMPALL_S		"PG_DUMPALL"
#define PG_RESTORE_S	"PG_RESTORE"
#define PG_BASEBACKUP_S "PG_BASEBACKUP"
#define PG_PERMISSION_S "PERMISSION"
#define AUDIT_SPACE_ALARM_S "SPACE ALARM"
#define AUDIT_SPACE_LIMIT_S "SPACE LIMIT"
#define AUDIT_DISK_FULL_S "DISK FULL"

#define AUDIT_PRIVILEGE_SYSTEM_S "SYSTEM"
#define AUDIT_PRIVILEGE_DATABASE_S "DATABASE"
#define AUDIT_PRIVILEGE_SCHEMAOBJECT_S "SCHEMAOBJECT"
/* Begin - BUG#00004718 */
#define AUDIT_PRIVILEGE_INSTANCE_S	"INSTANCE"
/* End - BUG#00004718 */

#define AUDIT_LAUNCHER_S "pg_audit launcher"

typedef enum AuditSystemEvent
{
	SYSTEMAUDIT_START = 1,
	SYSTEMAUDIT_STOP,
	SYSTEMAUDIT_LOGIN,
	SYSTEMAUDIT_LOGOUT,
	SYSTEMAUDIT_IMPORT,
	SYSTEMAUDIT_EXPORT,
	SYSTEMAUDIT_LOCK_ACCCOUNT,
	SYSTEMAUDIT_SWITCHOVER,
	SYSTEMAUDIT_PERMISSION,
	SYSTEMAUDIT_SPACE_ALARM,
	SYSTEMAUDIT_RELOAD,
	SYSTEMAUDIT_ALTER_GUC,
	SYSTEMAUDIT_PG_DUMP,
	SYSTEMAUDIT_PG_DUMPALL,
	SYSTEMAUDIT_PG_RESTORE,
	SYSTEMAUDIT_PG_BASEBACKUP,
	SYSTEMAUDIT_AUDIT_STORAGE_INVALID,
	SYSTEMAUDIT_AUDIT_SPACE_LIMIT,
	SYSTEMAUDIT_AUDIT_DISK_FULL,
	MANDATORY_AUDIT_ON, //It must be the second to last one
	MANDATORY_AUDIT_OFF //It must be the last one
}AuditSystemEvent;

typedef enum AuditResult
{
	AUDIT_SUCCESS = 's',
	AUDIT_FAIL = 'f',
	AUDIT_ALL = 'a',
	AUDIT_KEEP = 'k', //Used to record the action of modifying audit policies, returning this value without processing
	AUDIT_NONE = 'n'
}AuditResult;

typedef enum AuditRiskLevel
{
	AUDIT_RISK_INVALID = '/',
	AUDIT_RISK_LEVEL0 = '0',
	AUDIT_RISK_LEVEL1 = '1',
	AUDIT_RISK_LEVEL2 = '2',
	AUDIT_RISK_LEVEL3 = '3',
	AUDIT_RISK_LEVEL4 = '4',
	AUDIT_RISK_LEVEL_OTHER
}AuditRiskLevel;

typedef enum AuditType
{
	AUDIT_TYPE_INVALID = 'i',
	AUDIT_TYPE_STATEMENT = 's',
	AUDIT_TYPE_PRIVILEGE = 'p',
	AUDIT_TYPE_USER = 'u',
	AUDIT_TYPE_OBJECT = 'o',
	AUDIT_TYPE_SYSYEM = 'y',
	AUDIT_TYPE_MANDATORY = 'm'
}AuditType;

typedef enum AuditStmtType
{
	INVALID_STMT = '0',
	ALTER_DATABASE,
	ALTER_ROLE,
	ALTER_SCHEMA,
	ALTER_USER,
	CREATE_DATABASE,
	CREATE_ROLE,
	CREATE_SCHEMA,
	CREATE_USER,
	DROP_DATABASE,
	DROP_ROLE,
	DROP_SCHEMA,
	DROP_USER,
	GRANT_ROLE,
	REVOKE_ROLE,
	CREATE_DOMAIN,
	CREATE_PROCEDURE,
	CREATE_INDEX,
	CREATE_SEQUENCE,
	CREATE_TABLE,
	CREATE_TABLE_AS,
	CREATE_TRIGGER,
	CREATE_VIEW,
	CREATE_EXTENSION,
	ALTER_DOMAIN,
	ALTER_PROCEDURE,
	ALTER_INDEX,
	ALTER_SYSTEM,
	ALTER_SEQUENCE,
	ALTER_TABLE,
	ALTER_TRIGGER,
	ALTER_VIEW,
	ALTER_EXTENSION,
	DROP_DOMAIN,
	DROP_PROCEDURE,
	DROP_INDEX,
	DROP_SEQUENCE,
	DROP_TABLE,
	DROP_TRIGGER,
	DROP_VIEW,
	DROP_EXTENSION,
	SELECT_T,
	SELECT_INTO,
	INSERT_T,
	UPDATE_T,
	DELETE_T,
	GRANT_T,
	REVOKE_T,
	COMMENT_T,
	RESET_T,
	SET_T,
	TRUNCATE_T,
	COPY_TO,
	COPY_FROM,
	REINDEX_T,
	LOCK_T,
	CALL_T,
	CREATE_FUNCTION,
	DROP_FUNCTION,
	ALTER_FUNCTION,
	CREATE_TABLESPACE,
	ALTER_TABLESPACE,
	DROP_TABLESPACE,
	CREATE_MATLIZED_VIEW,
	ALTER_MATLIZED_VIEW,
	DROP_MATLIZED_VIEW,
	BEGIN_T,
	COMMIT_T,
	START_TRANSACTION,
	ROLLBACK_T,
	SAVEPOINT_T,
	ROLLBACK_TO_T,
	ROLLBACK_PREPARE_T,
	RELEASE_T,
	START_T,
	STOP_T,
	LOGIN_T,
	LOGOUT_T,
	RELOAD_T,
	AUDIT_STORAGE_INVALID_T,
	ALTER_GUC_T,
	IMPORT_T,
	EXPORT_T,
	PG_PERMISSION_T,
	PG_DUMP_T,
	PG_DUMPALL_T,
	PG_RESTORE_T,
	PG_BASEBACKUP_T,
	MANDATORY_AUDIT_ON_T,
	MANDATORY_AUDIT_OFF_T,
	LOCK_ACCCOUNT,
	SWITCHOVER,
	ALL_T
}AuditStmtType;

typedef enum AuditPrivilege
{
	AUDIT_PRIVILEGE_SYSTEM,
	AUDIT_PRIVILEGE_DATABASE,
	AUDIT_PRIVILEGE_SCHEMAOBJECT,
	AUDIT_PRIVILEGE_INSTACE
}AuditPrivilege;

typedef enum AuditCmdResult
{
	CMD_COMMON = 0,
	CMD_PG_DUMP_SUC = 1,
	CMD_PG_DUMP_FAIL = 2,
	CMD_PG_RESTORE_SUC = 3,
	CMD_PG_RESTORE_FAIL = 4,
	CMD_PG_DUMPALL_SUC = 5,
	CMD_PG_DUMPALL_FAIL = 6,
	CMD_PG_BASEBACK_SUC = 7,
	CMD_PG_BASEBACK_FAIL = 8
}AuditCmdResult;

typedef struct ClientInfo
{
	char	appname[CLIENTNAME_LEN];
	char	clientname[CLIENTNAME_LEN];
	char 	port[CLIENTPORT_LEN];
	char	mac[MAC_LEN];
	char	ip[CLIENTIP_LEN];		//ipv4 length = 32 IPv6 length = 128
	char	authmethod[AUTHMETHOD_LEN];
	AuditCmdResult	pg_dump_restore;	// 1 for pg_dump , 3 for pg_restore, 5 pg_dumpall, 7 pg_basebackup
}ClientInfo;

typedef struct ServerInfo
{
	unsigned char	mac[MAC_LEN];
	char	ip[256];	//ipv4 length = 32 IPv6 length = 128
	char 	port[NI_MAXSERV];
}ServerInfo;

typedef struct
{
	//ObjectType		objecttype;
	Oid				classId;		/* Class Id from pg_class */
	Oid				objectId;		/* OID of the object */
	char 		   *objectName;
	Oid				schemaId;
	char		   *schemaName;
	char		   *objectClassdesc;
	char			*colname;
	Oid				indrelId;
	AttrNumber		attnum;
	
}AuditEvent_Obj;

typedef struct AuditEvent
{
	char	  	actionname[32];
	Oid			confoids[MAX_NCONFOIDS];
	char		conftypes[MAX_NCONFOIDS];
	int32		nconfoids;
	
	/*
	 * Since audit happens in a separate transaction,
	 * target object may not exist. We should save
	 * object info when found the object.
	 */
	List	   *objects;
	Bitmapset   *columns[MAX_OBJECTOIDS];
	int32		sessionid;
	Oid			useroid;	
	char	   *username;
	Oid			roleid;
	char	   *rolename;
	TimestampTz	audittime;	
	Oid			commandid;
	char		result;	
	char	   *errtext;		
	char	   *sqltext;		
	char		clientname[CLIENTNAME_LEN];	// MyProcPort->remote_hostname
	char		clientip[CLIENTIP_LEN];	// MyProcPort->remote_host
	char		clientmac[MAC_LEN];
	char		clientport[CLIENTPORT_LEN];	// MyProcPort->remote_port
	char		clientappname[CLIENTNAME_LEN];	// application_name
	char		authmethod[AUTHMETHOD_LEN];
	AuditPrivilege	privilege;
	/* Record for baseline threshold */
	char	   *setname;
	char	   *setvalue;
	MemoryContext	auditContext;
} AuditEvent;


typedef struct AuditEventStackItem
{
	struct AuditEventStackItem *next;
	AuditEvent *auditEvent;
} AuditEventStackItem;

typedef enum AduitRetenSwitch
{
	RETEN_TIME,
	RETEN_SPACE
}AduitRetenSwitch;

typedef enum AuditOpresSwitch
{
	AUDIT_OPER_OFF = 0,		/* disabled */
	AUDIT_OPER_SUCC,
	AUDIT_OPER_FAIL,
	AUDIT_OPER_ALL
} AuditOpresSwitch;

typedef enum AuditSwitch
{
	AUDIT_INVALID = -1,
	AUDIT_OFF = 0,		/* disabled */
	AUDIT_ON		/* enabled while server is running normally */
} AuditSwitch;


typedef struct AduitSwitchGUC
{
	AuditSwitch audit_enable;
	AduitRetenSwitch audit_retentionpolicy_switch;
	int audit_retentiondays;
	int	audit_detectiontime;
	int audit_fullmode;
	uint64 audit_spacelimit;
	double audit_spacealarm;
	char *audit_archdir;
	char *remote_archdir;
	AuditSwitch audit_monitoring;
	char *audit_mail;
	int audit_privimode;
	AuditOpresSwitch audit_systemopers;
#ifdef USE_SQLITE
	char *audit_sqlitedbpath;
#endif
	AuditSwitch track_settings;
	bool *audit_pause;    //use share buffer
}AduitSwitchGUC;

extern bool receive_sighup;

extern AduitSwitchGUC audit_GUC;
extern ClientInfo clientInfo;
extern const char *audit_sys_file;

typedef void (*pgaudit_Command_hook_type) (void);
extern pgaudit_Command_hook_type pgaudit_CommandStart_hook ;
extern PGDLLIMPORT pgaudit_Command_hook_type audit_CommandStart_hook;
extern PGDLLIMPORT pgaudit_Command_hook_type audit_aborted_hook;
extern PGDLLIMPORT pgaudit_Command_hook_type audit_commited_hook;
extern PGDLLIMPORT pgaudit_Command_hook_type audit_WriteRecord_hook;

typedef void (*pgaudit_init_hook_type) (List*);
extern PGDLLIMPORT pgaudit_init_hook_type audit_init_hook;

typedef void (*pgaudit_SaveConnectInfo_hook_type) (Port*);
extern PGDLLIMPORT pgaudit_SaveConnectInfo_hook_type audit_SaveConnectInfo_hook;
extern PGDLLIMPORT pgaudit_SaveConnectInfo_hook_type audit_Save_ConnectAuthMethod_hook;

typedef void (*pgaudit_SetServerOption_hook_type) (char*, char*);
extern PGDLLIMPORT pgaudit_SetServerOption_hook_type audit_SetServerOption_hook;

typedef void (*pgaudit_AlterSetGuc_hook_type) (List *parsetreeList);
extern PGDLLIMPORT pgaudit_AlterSetGuc_hook_type audit_AlterSetGuc_hook;
extern PGDLLIMPORT pgaudit_AlterSetGuc_hook_type audit_CreateArchJob_hook;

typedef void (*pgaudit_StopPgagent_hook_type) (void);
extern PGDLLIMPORT pgaudit_StopPgagent_hook_type audit_StopPgagent_hook;

typedef void  (*pgaudit_Object_hook_type)(Oid oid,Relation rel);
extern PGDLLIMPORT pgaudit_Object_hook_type audit_object_hook;

typedef void  (*audit_system_err)(bool flag, int sqlerrcode);
extern PGDLLIMPORT audit_system_err audit_system_event_hook;

#ifdef USE_SQLITE
typedef void  (*audit_open_splite)(AuditSystemEvent event, AuditResult result, char *username, char *errtext);
extern PGDLLIMPORT audit_open_splite sqlite_open_db_for_audit_hook;
#endif

typedef void  (*audit_drop_pg_audit)(char *extname);
extern PGDLLIMPORT audit_drop_pg_audit audit_drop_pg_audit_hook;

typedef void  (*audit_alter_pg_audit_log)(char *relname);
extern PGDLLIMPORT audit_alter_pg_audit_log audit_alter_pg_audit_log_hook;

typedef Node*  (*audit_privi_model)(Node *n, bool iswhereclase);
extern PGDLLIMPORT audit_privi_model audit_privi_mode_hook;

typedef int (*get_children_num) (int target);

typedef void  (*audit_backend_normal)(int shutdown, int shutdown_target, get_children_num children_num, int target);
extern PGDLLIMPORT audit_backend_normal audit_normal_process_hook;


extern void deal_with_system_event_temp(AuditSystemEvent event,AuditResult result);

#endif
