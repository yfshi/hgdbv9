/*
 * contrib/mys_btree_gist/mys_btree_gist--1.0.sql
 * Gist/GIN index ops
 */

CREATE FUNCTION ivymy.gbtreekey8_in(cstring)
RETURNS ivymy.gbtreekey8
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey8_out(ivymy.gbtreekey8)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey8 (
	INTERNALLENGTH = 8,
	INPUT  = ivymy.gbtreekey8_in,
	OUTPUT = ivymy.gbtreekey8_out
);

CREATE FUNCTION ivymy.gbtreekey16_in(cstring)
RETURNS ivymy.gbtreekey16
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey16_out(ivymy.gbtreekey16)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey16 (
	INTERNALLENGTH = 16,
	INPUT  = ivymy.gbtreekey16_in,
	OUTPUT = ivymy.gbtreekey16_out
);

CREATE FUNCTION ivymy.gbtreekey32_in(cstring)
RETURNS ivymy.gbtreekey32
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey32_out(ivymy.gbtreekey32)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey32 (
	INTERNALLENGTH = 32,
	INPUT  = ivymy.gbtreekey32_in,
	OUTPUT = ivymy.gbtreekey32_out
);

CREATE FUNCTION ivymy.gbtreekey_var_in(cstring)
RETURNS ivymy.gbtreekey_var
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey_var_out(gbtreekey_var)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey_var (
	INTERNALLENGTH = VARIABLE,
	INPUT  = ivymy.gbtreekey_var_in,
	OUTPUT = ivymy.gbtreekey_var_out,
	STORAGE = EXTENDED
);

CREATE FUNCTION ivymy.gbt_decompress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

--
--
--
-- ivymy.mysyear ops
--
--
--

CREATE FUNCTION ivymy.gbt_year_consistent(internal, ivymy.year, int2, oid, internal)
RETURNS bool
AS 'MODULE_PATHNAME', 'gbt_year_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_union(internal, internal)
RETURNS gbtreekey8
AS 'MODULE_PATHNAME', 'gbt_year_union'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_compress'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_penalty(internal, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_penalty'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_same(ivymy.gbtreekey8, ivymy.gbtreekey8, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_same'
LANGUAGE C IMMUTABLE STRICT;

-- Create the year operator class
CREATE OPERATOR CLASS gist_mysyear_ops
DEFAULT FOR TYPE ivymy.year USING gist
AS
	OPERATOR	1	ivymy.< (ivymy.year, ivymy.year),
	OPERATOR	2	ivymy.<=(ivymy.year, ivymy.year),
	OPERATOR	3	ivymy.= (ivymy.year, ivymy.year),
	OPERATOR	4	ivymy.>=(ivymy.year, ivymy.year),
	OPERATOR	5	ivymy.> (ivymy.year, ivymy.year),
	FUNCTION	1	ivymy.gbt_year_consistent (internal, ivymy.year, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_year_union (internal, internal),
	FUNCTION	3	ivymy.gbt_year_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_year_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_year_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_year_same (ivymy.gbtreekey8, ivymy.gbtreekey8, internal),
	STORAGE		gbtreekey8;

ALTER OPERATOR FAMILY gist_mysyear_ops USING gist ADD
	OPERATOR	6	ivymy.<> (ivymy.year, ivymy.year);