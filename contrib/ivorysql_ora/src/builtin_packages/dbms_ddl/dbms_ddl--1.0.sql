/* Begin - OA-REQ-2198 */
CREATE OR REPLACE PROCEDURE sys.dbmsddl_alter_compile(type text, schema text, name text, reuse_settings boolean default false)
AS 'MODULE_PATHNAME','dbmsddl_alter_compile'
LANGUAGE C;

CREATE OR REPLACE PACKAGE SYS.DBMS_DDL AUTHID DEFINER
AS
	PROCEDURE ALTER_COMPILE(
		type varchar2,
		schema varchar2,
		name varchar2,
		reuse_settings boolean default false);
END;

CREATE OR REPLACE PACKAGE BODY SYS.DBMS_DDL
AS
	/* ALTER_COMPILE */
	PROCEDURE ALTER_COMPILE(
		type varchar2,
		schema varchar2,
		name varchar2,
		reuse_settings boolean default false)
	IS
	BEGIN
		sys.dbmsddl_alter_compile(type, schema, name, reuse_settings);
	END;
END;

GRANT EXECUTE ON PACKAGE SYS.DBMS_DDL TO public;
/* End - OA-REQ-2198 */
