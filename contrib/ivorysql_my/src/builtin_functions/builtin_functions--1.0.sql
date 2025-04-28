/***************************************************************
 *
 * Character datatype functions. 
 *
 ***************************************************************/

-- function isnull
CREATE OR REPLACE FUNCTION ivymy.isnull(anyelement)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'mys_isnull'
LANGUAGE C IMMUTABLE PARALLEL SAFE;