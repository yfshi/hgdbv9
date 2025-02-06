/***************************************************************
 *
 * MySQL case sensitive support function
 *
 ***************************************************************/
CREATE FUNCTION ivymy.myscaselower(oid,oid,integer)
RETURNS integer
AS 'MODULE_PATHNAME','myscaselower'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
