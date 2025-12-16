-- per database objects


CREATE FUNCTION is_sysdba(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'is_sysdba'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION is_syssso(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'is_syssso'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION is_syssao(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'is_syssao'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/* Begin - bug#z6385 */
CREATE FUNCTION is_superuser(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'is_superuser'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION has_createrole(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'has_createrole'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION has_createdb(oid)
RETURNS boolean
AS 'MODULE_PATHNAME', 'has_createdb'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

GRANT EXECUTE ON FUNCTION is_superuser TO PUBLIC;
GRANT EXECUTE ON FUNCTION has_createrole TO PUBLIC;
GRANT EXECUTE ON FUNCTION has_createdb TO PUBLIC;
/* End - bug#z6385 */

CREATE FUNCTION check_se_param_change()
RETURNS text
AS 'MODULE_PATHNAME', 'check_se_param_change'
LANGUAGE C STRICT;

CREATE FUNCTION set_backend_se_param(oid, text, boolean)
RETURNS text
AS 'MODULE_PATHNAME', 'set_backend_se_param'
LANGUAGE C STRICT;

CREATE FUNCTION reset_all_se_param()
RETURNS text
AS 'MODULE_PATHNAME', 'reset_all_se_param'
LANGUAGE C STRICT;

--	/* Begin - Bug#z4795 */
--	/* displaying "superuser" is not permitted for dky*/
CREATE VIEW pg_role AS
    SELECT
        rolname,
        rolsuper AS roladmin,
        rolinherit,
        rolcreaterole,
        rolcreatedb,
        rolcanlogin,
        rolreplication,
        rolconnlimit,
        '********'::text as rolpassword,
        rolvaliduntil,
        rolbypassrls,
        setconfig as rolconfig,
        pg_authid.oid
    FROM pg_authid LEFT JOIN pg_db_role_setting s
    ON (pg_authid.oid = setrole AND setdatabase = 0);

grant select on pg_role to public;
--	/* End - Bug#z4795 */

GRANT USAGE ON SCHEMA hg_security TO public;