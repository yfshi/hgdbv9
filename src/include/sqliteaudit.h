#include "postgres_fe.h"
#include <stdio.h>
#ifdef USE_SQLITE
#include <sqlite3.h>
#endif
#include "port.h"
#include <stdio.h>
#include "common/logging.h"
#include "common/username.h"
#include "port.h"

#define AUDIT_TYPE_SYSYEM_S "system"
#define AUDIT_TYPE_STATEMENT_S "statement"
#define AUDIT_TYPE_PRIVILEGE_S "privilege"
#define AUDIT_TYPE_USER_S "user"
#define AUDIT_TYPE_OBJECT_S "object"
#define AUDIT_TYPE_MANDATORY_S "mandatory"

#ifdef USE_SQLITE
static inline void audit_tools(char *data_path, char *toolname, char result, int audit_type, char *actionname, char *errtext)
{
	sqlite3	   *conn = NULL;
	const char *user;
	char	   *errstr;
	time_t		timer;
	char	   *err;
	int			rc;
	char ts[128];
	char		inserttext[1024] = {0};
	char	sqlitepath[128];
	char	*resultstr = "SUCCESSFUL";
	char	*typetag = AUDIT_TYPE_STATEMENT_S;

	timer = time(NULL);
	strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&timer));

	user = getenv("PGUSER");
	if (!user)
	{
		user = get_user_name(&errstr);
		if (!user)
		{
			pg_log_fatal("%s", errstr);
			exit(-1);
		}
	}

	switch (audit_type)
	{
		case 's':
			typetag = AUDIT_TYPE_STATEMENT_S;
			break;
		case 'p':
			typetag = AUDIT_TYPE_PRIVILEGE_S;
			break;
		case 'u':
			typetag = AUDIT_TYPE_USER_S;
			break;
		case 'o':
			typetag = AUDIT_TYPE_OBJECT_S;
			break;
		case 'y':
			typetag = AUDIT_TYPE_SYSYEM_S;
			break;
		case 'm':
			typetag = AUDIT_TYPE_MANDATORY_S;
			break;
		default:
			break;
	}

	if (result == 'f')
		resultstr = "FAIL";

	snprintf(sqlitepath, sizeof(sqlitepath), "%s%s", data_path, "/pgaudit/sqlite/sqlite.db");
	rc = sqlite3_open(sqlitepath, &conn);
	if (rc != SQLITE_OK)
		exit(-1);
	snprintf(inserttext, sizeof(inserttext), "insert into pg_sqlite_audit_log(sessionoid, useroid, username, audittime, result, audittype, commandid, actionname, clientport, clientname, clientip, clientmac, clientappname, errtext, authmethod, privlevel) values(%d, %d, \'%s\', \'%s\', \'%s\', \'%s\', 0, \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\')", 
						0, 0, user, ts, resultstr, typetag, actionname,
						"", toolname, "", "",
						"", errtext, "", "SYSTEM");
	rc = sqlite3_exec(conn, inserttext, NULL, NULL, &err);
	if (rc != SQLITE_OK)
	{
		pg_log_error("Failed to insert in SQLite DB,SQLite error \"%s\"", err);
		sqlite3_free(err);
	}
	sqlite3_close(conn);
	conn = NULL;
}
#endif
