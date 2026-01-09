/***************************************************************
 *
 * MySQL uint1 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint1_in(pg_catalog.cstring)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','mysuint1_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint1_out(ivymy.uint1)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint1_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint1_recv(internal)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','mysuint1_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_send(ivymy.uint1)
RETURNS bytea
AS 'MODULE_PATHNAME','mysuint1_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint1_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint1_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_scale(ivymy.uint1, pg_catalog.int4)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','mysuint1_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint1' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysuint1_in')
WHERE oid = 9202;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysuint1_out')
WHERE oid = 9202;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysuint1_recv')
WHERE oid = 9202;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysuint1_send')
WHERE oid = 9202;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint1_typmodin')
WHERE oid = 9202;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint1_typmodout')
WHERE oid = 9202;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint1_typmodin')
WHERE oid = 9203;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint1_typmodout')
WHERE oid = 9203;

CREATE FUNCTION ivymy.int8_uint1(pg_catalog.int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int8_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint1)
WITH FUNCTION ivymy.int8_uint1(pg_catalog.int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint1(pg_catalog.int4)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int4_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint1)
WITH FUNCTION ivymy.int4_uint1(pg_catalog.int4)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint1(pg_catalog.int2)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int2_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint1)
WITH FUNCTION ivymy.int2_uint1(pg_catalog.int2)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint1)
WITH FUNCTION numeric_uint1(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

-- to bpchar
CREATE CAST (ivymy.uint1 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (ivymy.uint1 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (varchar as uint1)
--
CREATE FUNCTION ivymy.text_uint1(pg_catalog.varchar, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'varlena_uint1'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.varchar as ivymy.uint1) 
WITH FUNCTION ivymy.text_uint1(pg_catalog.varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bpchar as uint1)
--
CREATE FUNCTION ivymy.text_uint1(pg_catalog.bpchar, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'varlena_uint1'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.bpchar as ivymy.uint1) 
WITH FUNCTION ivymy.text_uint1(pg_catalog.bpchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (text as uint1)
--
CREATE FUNCTION ivymy.text_uint1(pg_catalog.text, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'varlena_uint1'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.text as ivymy.uint1) 
WITH FUNCTION ivymy.text_uint1(pg_catalog.text, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bool as uint1)
--
CREATE FUNCTION ivymy.boolean_mysuint1(pg_catalog.bool) 
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','boolean_mysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.uint1)
WITH FUNCTION ivymy.boolean_mysuint1(pg_catalog.bool)
AS ASSIGNMENT;

--
-- cast (uint1 as uint8)
--
CREATE CAST (ivymy.uint1 AS ivymy.uint8) WITH INOUT AS IMPLICIT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint1_eq(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint1_ne(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint1_lt(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint1_gt(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint1_le(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint1_ge(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_ge(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint1_seq(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint1_add(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.mysuint1_negative(ivymy.uint1)
RETURNS  pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint1_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint1_sub(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint1
AS 'MODULE_PATHNAME','mysuint1_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint1
AS 'MODULE_PATHNAME','int8_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint1_mul(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysuint1_div(ivymy.uint1, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.uint1, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.uint1, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.uint1, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int2, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int4, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint1_mod(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','mysuint1_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int2, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int4, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int8, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint1_nega(ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_negamysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint1_and(ivymy.uint1, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.uint1, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.uint1, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.uint1, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int2, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int4, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint1_invert(ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_invertmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--uint1 to boolean
CREATE FUNCTION ivymy.mysuint1_boolean(ivymy.uint1) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint1_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint1 AS boolean)
WITH FUNCTION ivymy.mysuint1_boolean(ivymy.uint1)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint1_negative,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint1_nega,
	rightarg = ivymy.uint1
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator ||
CREATE OPERATOR ! (
	procedure = ivymy.mysuint1_invert,
	rightarg = ivymy.uint1
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint1_like(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_like(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_like(ivymy.uint1, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_notlike(ivymy.uint1, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_notlike(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_notlike(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint1_like,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint1_like,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint1_like,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint1_larger(ivymy.uint1, ivymy.uint1)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'mysuint1_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint1) (
  SFUNC = ivymy.mysuint1_larger,
  STYPE = ivymy.uint1,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint1_smaller(ivymy.uint1, ivymy.uint1)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'mysuint1_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint1) (
  SFUNC = ivymy.mysuint1_smaller,
  STYPE = ivymy.uint1,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint1_avg_accum(pg_catalog.int8[], ivymy.uint1)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'mysuint1_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint1) (
   sfunc = mysuint1_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.mysuint1_sum(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'mysuint1_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint1) (
  SFUNC = ivymy.mysuint1_sum,
  STYPE = pg_catalog.int8
);