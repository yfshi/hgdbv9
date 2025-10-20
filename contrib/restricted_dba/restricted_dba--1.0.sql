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

GRANT USAGE ON SCHEMA hg_secsch TO public;