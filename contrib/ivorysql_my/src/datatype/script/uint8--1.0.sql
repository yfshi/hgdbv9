/***************************************************************
 *
 * MySQL uint8 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint8_in(pg_catalog.cstring, oid, integer, bigint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint8_out(ivymy.uint8)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint8_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint8_recv(internal, oid, integer, bigint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_send(ivymy.uint8)
RETURNS bytea
AS 'MODULE_PATHNAME','mysuint8_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint8_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint8_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_scale(ivymy.uint8, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint8' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysuint8_in')
WHERE oid = 9218;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysuint8_out')
WHERE oid = 9218;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysuint8_recv')
WHERE oid = 9218;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysuint8_send')
WHERE oid = 9218;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint8_typmodin')
WHERE oid = 9218;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint8_typmodout')
WHERE oid = 9218;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint8_typmodin')
WHERE oid = 9219;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint8_typmodout')
WHERE oid = 9219;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_uint8(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint8)
WITH FUNCTION ivymy.int_uint8(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;


CREATE FUNCTION ivymy.int_uint8(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint8)
WITH FUNCTION ivymy.int_uint8(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int_uint8(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint8)
WITH FUNCTION ivymy.int_uint8(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.numeric_uint8(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','numeric_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.numeric AS ivymy.uint8)
WITH FUNCTION ivymy.numeric_uint8(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* Begin BUG#5048 5070 */
-- cast uint8 to int2
CREATE FUNCTION ivymy.uint8_int2(ivymy.uint8, int, bool, int8)
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','uint8_int2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as pg_catalog.int2)
WITH FUNCTION ivymy.uint8_int2(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to int4
CREATE FUNCTION ivymy.uint8_int4(ivymy.uint8, int, bool, int8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint8_int4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as pg_catalog.int4)
WITH FUNCTION ivymy.uint8_int4(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to int8
CREATE FUNCTION ivymy.uint8_int8(ivymy.uint8, int, bool, int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint8_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as pg_catalog.int8)
WITH FUNCTION ivymy.uint8_int8(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to uint1
CREATE FUNCTION ivymy.uint8_uint1(ivymy.uint8, int, bool, int8)
RETURNS uint1
AS 'MODULE_PATHNAME','uint8_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as uint1)
WITH FUNCTION ivymy.uint8_uint1(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to uint2
CREATE FUNCTION ivymy.uint8_uint2(ivymy.uint8, int, bool, int8)
RETURNS uint2
AS 'MODULE_PATHNAME','uint8_uint2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as uint2)
WITH FUNCTION ivymy.uint8_uint2(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to uint4
CREATE FUNCTION ivymy.uint8_uint4(uint8, int, bool, int8)
RETURNS uint4
AS 'MODULE_PATHNAME','uint8_uint4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (uint8 as uint4)
WITH FUNCTION ivymy.uint8_uint4(uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to float4
CREATE FUNCTION ivymy.uint8_float4(ivymy.uint8, int, bool, int8)
RETURNS float4
AS 'MODULE_PATHNAME','uint8_float4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as float4)
WITH FUNCTION ivymy.uint8_float4(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;

-- cast uint8 to float8
CREATE FUNCTION ivymy.uint8_float8(ivymy.uint8, int, bool, int8)
RETURNS float8
AS 'MODULE_PATHNAME','uint8_float8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.uint8 as float8)
WITH FUNCTION ivymy.uint8_float8(ivymy.uint8, int, bool, int8)
AS ASSIGNMENT;
/* End BUG#5048 5070 */


-- to bpchar
CREATE CAST (ivymy.uint8 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (ivymy.uint8 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (varchar as uint8)
--
CREATE FUNCTION ivymy.text_uint8(pg_catalog.varchar, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'varlena_uint8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.varchar as ivymy.uint8) 
WITH FUNCTION ivymy.text_uint8(pg_catalog.varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bpchar as uint8)
--
CREATE FUNCTION ivymy.text_uint8(pg_catalog.bpchar, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'varlena_uint8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.bpchar as ivymy.uint8) 
WITH FUNCTION ivymy.text_uint8(pg_catalog.bpchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (text as uint8)
--
CREATE FUNCTION ivymy.text_uint8(pg_catalog.text, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'varlena_uint8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.text as ivymy.uint8) 
WITH FUNCTION ivymy.text_uint8(pg_catalog.text, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bool as uint8)
--
CREATE FUNCTION ivymy.boolean_uint8(pg_catalog.bool) 
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','boolean_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.uint8)
WITH FUNCTION ivymy.boolean_uint8(pg_catalog.bool)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------


/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint8_eq(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint8_ne(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint8_lt(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint8_gt(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint8_le(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint8_ge(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_ge(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint8_seq(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint8_add(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.mysuint8_negative(ivymy.uint8)
RETURNS  bigint
AS 'MODULE_PATHNAME','mysuint8_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint8_sub(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint8_mul(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* Begin - BUG#6039 */
CREATE FUNCTION ivymy.mysuint8_mul(double, ivymy.uint8)
RETURNS  double
AS 'MODULE_PATHNAME','double_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.uint8, double)
RETURNS  double
AS 'MODULE_PATHNAME','uint8_muldouble'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
/* End - BUG#6039 */

----function divide
CREATE FUNCTION ivymy.mysuint8_div(ivymy.uint8, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.uint8, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.uint8, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.uint8, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int2, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int4, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int8, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint8_mod(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int2, ivymy.uint8)
RETURNS int8
AS 'MODULE_PATHNAME','int2_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int4, ivymy.uint8)
RETURNS  int8
AS 'MODULE_PATHNAME','int4_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int8, ivymy.uint8)
RETURNS  int8
AS 'MODULE_PATHNAME','int8_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint8_nega(ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_negamysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint8_and(ivymy.uint8, ivymy.uint8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint8_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.uint8, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.uint8, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.uint8, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int2, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int4, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int8, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.uint8cat(uint8, uint8)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL

PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint8_invert(ivymy.uint8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint8_invertmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--uint8 to boolean
CREATE FUNCTION ivymy.int8_boolean(pg_catalog.int8) 
RETURNS boolean 
AS 'MODULE_PATHNAME','int8_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS boolean)
WITH FUNCTION ivymy.int8_boolean(pg_catalog.int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysuint8_boolean(ivymy.uint8) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint8_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint8 AS boolean)
WITH FUNCTION ivymy.mysuint8_boolean(ivymy.uint8)
AS ASSIGNMENT;

--uint8 to numeric
CREATE FUNCTION ivymy.mysuint8_numeric(ivymy.uint8)
RETURNS pg_catalog.numeric 
AS 'MODULE_PATHNAME','mysuint8_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint8 AS pg_catalog.numeric)
WITH FUNCTION ivymy.mysuint8_numeric(ivymy.uint8)
AS IMPLICIT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint8_negative,
	rightarg = ivymy.uint8
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = double,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.uint8,
	rightarg = double
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint8_nega,
	rightarg = ivymy.uint8
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.uint8cat,
	leftarg = uint8,
	rightarg = uint8
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysuint8_invert,
	rightarg = ivymy.uint8
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint8_like(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_like(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_like(ivymy.uint8, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_notlike(ivymy.uint8, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_notlike(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_notlike(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint8_like,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint8_like,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint8_like,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint8_larger(ivymy.uint8, ivymy.uint8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'mysuint8_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint8) (
  SFUNC = ivymy.mysuint8_larger,
  STYPE = ivymy.uint8,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint8_smaller(ivymy.uint8, ivymy.uint8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'mysuint8_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint8) (
  SFUNC = ivymy.mysuint8_smaller,
  STYPE = ivymy.uint8,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint8_avg_accum(internal, ivymy.uint8)
RETURNS internal
AS 'MODULE_PATHNAME', 'mysuint8_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_avg(internal)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME', 'mysuint8_avg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint8) (
   sfunc = mysuint8_avg_accum, 
   stype = internal,
   finalfunc = mysuint8_avg
);

/* sum */
CREATE FUNCTION ivymy.mysuint8_sum(pg_catalog.numeric, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME', 'mysuint8_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint8) (
  SFUNC = ivymy.mysuint8_sum,
  STYPE = pg_catalog.numeric
);

--
-- CREATE FUNCTION
--
CREATE FUNCTION ivymy.MOD(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint8_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* Begin - bug#z6043 */
CREATE FUNCTION ivymy.varchar_add_uint8(text, uint8)
RETURNS numeric
AS 'select $1::numeric + $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.varchar_add_uint8,
	leftarg = text,
	rightarg = uint8
);

CREATE FUNCTION ivymy.char_add_uint8(char, uint8)
RETURNS numeric
AS 'select $1::numeric + $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.char_add_uint8,
	leftarg = char,
	rightarg = uint8
);

CREATE FUNCTION ivymy.uint8_add_char(uint8, char)
RETURNS numeric
AS 'select $1 + $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.uint8_add_char,
	leftarg = uint8,
	rightarg = char
);
/* End - bug#z6043 */