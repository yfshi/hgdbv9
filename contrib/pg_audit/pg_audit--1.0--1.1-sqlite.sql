
CREATE SERVER IF NOT EXISTS sqlite_server 
FOREIGN DATA WRAPPER sqlite_fdw
OPTIONS (database 'pgaudit/sqlite/sqlite.db');

CREATE FOREIGN TABLE hg_security.sqlite_audit_log(
	confid oid,
	sessionoid integer,
	useroid oid,
	username text,
	roleoid oid,
	rolename text,
	audittime text,
	result "char",
	namespaceid oid,
	namespacename text,
	objectoid oid,
	objectname text,
	colname text,
	classid oid,
	objecttype text,
	audittype "char",
	commandid integer,
	actionname text,
	clientport text,
	clientname text,
	clientip text,
	clientmac text,
	clientappname text,
	sqltext text,
	errtext text,
	authmethod text,
	privlevel text)
	SERVER sqlite_server
	OPTIONS (table 'pg_sqlite_audit_log');
	
GRANT ALL ON FUNCTION sqlite_fdw_handler TO syssao;
GRANT ALL ON FUNCTION sqlite_fdw_handler TO syssao_role;

ALTER TABLE hg_security.sqlite_audit_log OWNER TO syssao;
GRANT SELECT ON hg_security.sqlite_audit_log TO syssso;

GRANT SELECT ON hg_security.sqlite_audit_log to syssao_role;
GRANT SELECT ON hg_security.sqlite_audit_log to syssso_role;