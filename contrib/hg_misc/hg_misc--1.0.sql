/* contrib/hg_misc/hg_misc--1.0.sql */
--complain if script is sourced in psql rather than via ALTER EXTENSION
\echo Use "CREATE EXTENSION hg_misc" to load this file. \quit

/* Begin - ReqID:SRS-MISC-LICENSE */
CREATE FUNCTION highgo_version()
RETURNS text
AS 'MODULE_PATHNAME', 'highgo_version'
LANGUAGE C STRICT PARALLEL RESTRICTED;

-- Register functions.
CREATE FUNCTION get_lic_baseinfo()
RETURNS text
AS 'MODULE_PATHNAME', 'get_lic_baseinfo'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION get_lic_funcinfo(OUT id integer,
	OUT name text,
	OUT description text,
	OUT enabled boolean
)
RETURNS SETOF record
as 'MODULE_PATHNAME', 'get_lic_funcinfo'
LANGUAGE C STRICT VOLATILE PARALLEL SAFE;

-- Register views on the function for ease of use.
CREATE VIEW v_lic_baseinfo AS
	SELECT * FROM get_lic_baseinfo();

CREATE VIEW v_lic_funcinfo AS
	SELECT * FROM get_lic_funcinfo();

GRANT SELECT ON v_lic_baseinfo TO PUBLIC;
GRANT SELECT ON v_lic_funcinfo TO PUBLIC;
/* End - ReqID:SRS-MISC-LICENSE */