/***************************************************************
 *
 * MySQL uint4 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint4_in(pg_catalog.cstring, oid, integer, bigint)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint4_out(ivymy.uint4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint4_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint4_recv(internal, oid, integer, bigint)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint4_send(ivymy.uint4)
RETURNS bytea
AS 'MODULE_PATHNAME','mysuint4_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint4_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint4_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint4_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint4_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint4_scale(ivymy.uint4, pg_catalog.int4)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint4' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysuint4_in')
WHERE oid = 9214;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysuint4_out')
WHERE oid = 9214;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysuint4_recv')
WHERE oid = 9214;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysuint4_send')
WHERE oid = 9214;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodin')
WHERE oid = 9214;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodout')
WHERE oid = 9214;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodin')
WHERE oid = 9215;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint4_typmodout')
WHERE oid = 9215;

-- Compatible MySQL Cast
CREATE FUNCTION ivymy.int8_uint4(pg_catalog.int8)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','int8_uint4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint4)
WITH FUNCTION ivymy.int8_uint4(pg_catalog.int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint4(pg_catalog.int4)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','int4_uint4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint4)
WITH FUNCTION ivymy.int4_uint4(pg_catalog.int4)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint4(pg_catalog.int2)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','int2_uint4'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint4)
WITH FUNCTION ivymy.int2_uint4(pg_catalog.int2)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint4)
WITH FUNCTION numeric_uint4(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.uint4_numeric(ivymy.uint4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint4_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint4 AS pg_catalog.numeric)
WITH FUNCTION ivymy.uint4_numeric(ivymy.uint4)
AS ASSIGNMENT;

-- to bpchar
CREATE CAST (ivymy.uint4 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (ivymy.uint4 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (varchar as uint4)
--
CREATE FUNCTION ivymy.text_uint4(pg_catalog.varchar, int, bool, int8)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME', 'varlena_uint4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.varchar as ivymy.uint4) 
WITH FUNCTION ivymy.text_uint4(pg_catalog.varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bpchar as uint4)
--
CREATE FUNCTION ivymy.text_uint4(pg_catalog.bpchar, int, bool, int8)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME', 'varlena_uint4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.bpchar as ivymy.uint4) 
WITH FUNCTION ivymy.text_uint4(pg_catalog.bpchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (text as uint4)
--
CREATE FUNCTION ivymy.text_uint4(pg_catalog.text, int, bool, int8)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME', 'varlena_uint4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.text as ivymy.uint4) 
WITH FUNCTION ivymy.text_uint4(pg_catalog.text, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bool as uint4)
--
CREATE FUNCTION ivymy.boolean_uint4(pg_catalog.bool) 
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME','boolean_uint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.uint4)
WITH FUNCTION ivymy.boolean_uint4(pg_catalog.bool)
AS ASSIGNMENT;

--
-- cast (uint4 as uint8)
--
CREATE CAST (ivymy.uint4 AS ivymy.uint8) WITH INOUT AS IMPLICIT;


/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint4_eq(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_eq(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint4_ne(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ne(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint4_lt(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_lt(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint4_gt(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_gt(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint4_le(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_le(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint4_ge(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ge(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint4_ge(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ge(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ge(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint4_ge(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_ge(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint4_seq(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(ivymy.uint4, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(ivymy.uint4, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(pg_catalog.int2, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(pg_catalog.int4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_seq(pg_catalog.int8, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint4_add(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_addmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_addmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_addmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_add(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_addmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.mysuint4_negative(ivymy.uint4)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint4_sub(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_submysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_submysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_submysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sub(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int8_submysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint4_mul(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_mulmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mul(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mul(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mul(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint4_mul(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_mulmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mul(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_mulmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mul(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_mulmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysuint4_div(ivymy.uint4, ivymy.uint4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint4_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(ivymy.uint4, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint4_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(ivymy.uint4, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint4_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(ivymy.uint4, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint4_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(pg_catalog.int2, ivymy.uint4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(pg_catalog.int4, ivymy.uint4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_div(pg_catalog.int8, ivymy.uint4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_exact_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int8_exact_divmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint4_mod(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_modmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint4_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_modmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_modmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_mod(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int8_modmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint4_bitand(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitandmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitandmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitandmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitand(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitandmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint4_bitor(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitor(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint4_nega(ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_negamysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint4_bitxor(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitxormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_bitxor(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxormysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint4_rightmov(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_rightmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_rightmov(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint4_leftmov(ivymy.uint4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_leftmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(ivymy.uint4, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(ivymy.uint4, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(ivymy.uint4, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint4_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(pg_catalog.int2, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(pg_catalog.int4, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_leftmov(pg_catalog.int8, ivymy.uint4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint4_and(ivymy.uint4, ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_andmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(ivymy.uint4, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(ivymy.uint4, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(ivymy.uint4, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(pg_catalog.int2, ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(pg_catalog.int4, ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_and(pg_catalog.int8, ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint4_invert(ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint4_invertmysuint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--uint4 to boolean
CREATE FUNCTION ivymy.mysuint4_boolean(ivymy.uint4) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint4_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint4 AS boolean)
WITH FUNCTION ivymy.mysuint4_boolean(ivymy.uint4)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint4_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint4_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint4_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint4_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint4_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint4_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint4_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint4_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint4_negative,
	rightarg = ivymy.uint4
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint4_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint4_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint4_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint4_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint4_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint4_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint4_nega,
	rightarg = ivymy.uint4
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint4_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint4_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint4_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint4_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint4
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysuint4_invert,
	rightarg = ivymy.uint4
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint4_like(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint4_like(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint4_like(ivymy.uint4, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint4_notlike(ivymy.uint4, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint4_notlike(ivymy.uint4, ivymy.uint4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint4_notlike(ivymy.uint4, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint4_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint4_like,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint4_like,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint4_like,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint4_notlike,
	leftarg = ivymy.uint4,
	rightarg = ivymy.uint4
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint4_notlike,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint4_notlike,
	leftarg = ivymy.uint4,
	rightarg = pg_catalog.text
);

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint4_larger(ivymy.uint4, ivymy.uint4)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME', 'mysuint4_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint4) (
  SFUNC = ivymy.mysuint4_larger,
  STYPE = ivymy.uint4,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint4_smaller(ivymy.uint4, ivymy.uint4)
RETURNS ivymy.uint4
AS 'MODULE_PATHNAME', 'mysuint4_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint4) (
  SFUNC = ivymy.mysuint4_smaller,
  STYPE = ivymy.uint4,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint4_avg_accum(pg_catalog.int8[], ivymy.uint4)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'mysuint4_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint4) (
   sfunc = mysuint4_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.mysuint4_sum(pg_catalog.int8, ivymy.uint4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'mysuint4_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint4) (
  SFUNC = ivymy.mysuint4_sum,
  STYPE = pg_catalog.int8
);