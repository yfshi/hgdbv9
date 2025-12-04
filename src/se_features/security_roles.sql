-- defautl instance-level user and roles create while initdb
CREATE USER SYSSSO PASSWORD '$SYSSSO_PASSWORD';
CREATE USER SYSSAO PASSWORD '$SYSSAO_PASSWORD';

CREATE ROLE SYSDBA_ROLE ADMIN "$POSTGRES"; 
CREATE ROLE SYSSSO_ROLE ADMIN SYSSSO;
CREATE ROLE SYSSAO_ROLE ADMIN SYSSAO;

-- to make default role cannot be dropped, we pin it as a internal object
INSERT INTO pg_shdepend SELECT 0,0,0,0, tableoid,oid, 'p' FROM pg_authid WHERE rolname = 'sysdba_role';
INSERT INTO pg_shdepend SELECT 0,0,0,0, tableoid,oid, 'p' FROM pg_authid WHERE rolname = 'syssso_role';
INSERT INTO pg_shdepend SELECT 0,0,0,0, tableoid,oid, 'p' FROM pg_authid WHERE rolname = 'syssao_role';

/* Begin bug#z4364 */
/* 
 * pg_reload_conf() gives member of syssso_role ability to reset all params
 * in .conf and auto.conf. This is just a rough approach. It might be replaced
 * by a refined version latter
 */
GRANT EXECUTE ON FUNCTION pg_catalog.pg_reload_conf() to syssso_role;
/* End bug#z4364 */