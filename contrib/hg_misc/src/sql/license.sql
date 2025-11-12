/* Begin - ReqID:SRS-MISC-LICENSE */
CREATE FUNCTION pg_catalog.highgo_version()
RETURNS text
AS 'MODULE_PATHNAME', 'highgo_version'
LANGUAGE C STRICT PARALLEL RESTRICTED;

-- Register functions.
CREATE FUNCTION pg_catalog.get_lic_baseinfo()
RETURNS text
AS 'MODULE_PATHNAME', 'get_lic_baseinfo'
LANGUAGE C PARALLEL SAFE;

-- Register views on the function for ease of use.
CREATE VIEW pg_catalog.v_lic_baseinfo AS
	SELECT * FROM get_lic_baseinfo();

GRANT SELECT ON pg_catalog.v_lic_baseinfo TO PUBLIC;
/* End - ReqID:SRS-MISC-LICENSE */

/* Begin - ReqID:SRS-MISC-LICENSE-TEMPLATE */
-- Register functions.
CREATE FUNCTION pg_catalog.get_lic_funcinfo(
	OUT id integer,
	OUT classify text,
	OUT name text,
	OUT status text
)
RETURNS SETOF record
as 'MODULE_PATHNAME', 'get_lic_funcinfo'
LANGUAGE C STRICT VOLATILE PARALLEL SAFE;

-- Register views on the function for case of use.
CREATE VIEW pg_catalog.v_lic_funcinfo AS
	SELECT * FROM get_lic_funcinfo();

GRANT SELECT ON pg_catalog.v_lic_funcinfo TO PUBLIC;
/* End - ReqID:SRS-MISC-LICENSE-TEMPLATE */
