/***************************************************************
 *
 * MySQL mysuint2 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint2_in(pg_catalog.cstring)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','mysuint2_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint2_out(ivymy.mysuint2)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint2_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint2_recv(internal)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','mysuint2_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_send(ivymy.mysuint2)
RETURNS bytea
AS 'MODULE_PATHNAME','mysuint2_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint2_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint2_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_scale(ivymy.mysuint2, pg_catalog.int4)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','mysuint2_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysuint2' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysuint2_in')
WHERE oid = 9206;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysuint2_out')
WHERE oid = 9206;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysuint2_recv')
WHERE oid = 9206;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysuint2_send')
WHERE oid = 9206;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint2_typmodin')
WHERE oid = 9206;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint2_typmodout')
WHERE oid = 9206;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysuint2_typmodin')
WHERE oid = 9207;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysuint2_typmodout')
WHERE oid = 9207;
---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysuint2(pg_catalog.int8)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','int_mysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int8 AS ivymy.mysuint2)
WITH FUNCTION ivymy.int_mysuint2(pg_catalog.int8)
AS ASSIGNMENT;


CREATE FUNCTION ivymy.int_mysuint2(pg_catalog.int2)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','int_mysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int2 AS ivymy.mysuint2)
WITH FUNCTION ivymy.int_mysuint2(pg_catalog.int2)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int_mysuint2(pg_catalog.int4)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','int_mysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.mysuint2)
WITH FUNCTION ivymy.int_mysuint2(pg_catalog.int4)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.numeric_mysuint2(pg_catalog.numeric)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME','numeric_mysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.numeric AS ivymy.mysuint2)
WITH FUNCTION ivymy.numeric_mysuint2(pg_catalog.numeric)
AS ASSIGNMENT;
/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysuint2 AS ivymy.mysuint4) WITH INOUT AS IMPLICIT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint2_eq(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_eq(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint2_ne(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ne(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint2_lt(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_lt(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint2_gt(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_gt(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint2_le(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_le(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint2_ge(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ge(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint2_ge(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ge(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ge(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint2_ge(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_ge(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint2_seq(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(ivymy.mysuint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(ivymy.mysuint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(pg_catalog.int2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(pg_catalog.int4, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_seq(pg_catalog.int8, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint2_add(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_addmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_addmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_addmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_add(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_addmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.mysuint2_negative(ivymy.mysuint2)
RETURNS  pg_catalog.int4
AS 'MODULE_PATHNAME','mysuint2_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint2_sub(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_submysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint2
AS 'MODULE_PATHNAME','mysuint2_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_submysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_submysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sub(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint2
AS 'MODULE_PATHNAME','int8_submysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint2_mul(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_mulmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mul(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mul(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mul(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint2_mul(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_mulmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mul(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_mulmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mul(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_mulmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysuint2_div(ivymy.mysuint2, ivymy.mysuint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint2_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(ivymy.mysuint2, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint2_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(ivymy.mysuint2, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint2_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(ivymy.mysuint2, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint2_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(pg_catalog.int2, ivymy.mysuint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(pg_catalog.int4, ivymy.mysuint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_div(pg_catalog.int8, ivymy.mysuint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_exact_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_exact_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_exact_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_exact_divmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint2_mod(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_modmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint2_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(pg_catalog.int2, ivymy.mysuint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(pg_catalog.int4, ivymy.mysuint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_mod(pg_catalog.int8, ivymy.mysuint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint2_bitand(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitandmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitandmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitandmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitand(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitandmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint2_bitor(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitor(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint2_nega(ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_negamysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint2_bitxor(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitxormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitxormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitxormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_bitxor(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitxormysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint2_rightmov(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_rightmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_rightmov(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint2_leftmov(ivymy.mysuint2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_leftmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(ivymy.mysuint2, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(ivymy.mysuint2, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(ivymy.mysuint2, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint2_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(pg_catalog.int2, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(pg_catalog.int4, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_leftmov(pg_catalog.int8, ivymy.mysuint2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint2_and(ivymy.mysuint2, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_andmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(ivymy.mysuint2, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(ivymy.mysuint2, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(ivymy.mysuint2, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(pg_catalog.int2, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(pg_catalog.int4, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_and(pg_catalog.int8, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysuint2_or(ivymy.mysuint2, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_ormysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(ivymy.mysuint2, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_orint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(ivymy.mysuint2, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_orint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(ivymy.mysuint2, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_orint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(pg_catalog.int2, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_ormysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(pg_catalog.int4, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_ormysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_or(pg_catalog.int8, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_ormysuint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint2_invert(ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint2_invertmysuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--mysuint2 to boolean
CREATE FUNCTION ivymy.mysuint2_boolean(ivymy.mysuint2) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint2_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysuint2 AS boolean)
WITH FUNCTION ivymy.mysuint2_boolean(ivymy.mysuint2)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint2_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint2_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint2_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint2_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint2_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint2_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint2_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint2_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint2_negative,
	rightarg = ivymy.mysuint2
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint2_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint2_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint2_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint2_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint2_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint2_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint2_nega,
	rightarg = ivymy.mysuint2
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint2_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint2_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint2_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint2_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint2_or,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint2
);

--operator ||
CREATE OPERATOR ! (
	procedure = ivymy.mysuint2_invert,
	rightarg = ivymy.mysuint2
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint2_like(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint2_like(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint2_notlike(ivymy.mysuint2, ivymy.mysuint2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint2_notlike(ivymy.mysuint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint2_like,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint2_like,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint2_notlike,
	leftarg = ivymy.mysuint2,
	rightarg = ivymy.mysuint2
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint2_notlike,
	leftarg = ivymy.mysuint2,
	rightarg = pg_catalog.int8
);

/*
 * B-tree Index Support
 * Create Operator family 'mysuint2_ops' for type 'mysuint2'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.mysuint2, ivymy.mysuint2)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint2_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint2_ops' */
CREATE OPERATOR FAMILY ivymy.mysuint2_ops USING btree;

/* CREATE OPERATOR CLASS for 'oramysuint2' */
CREATE OPERATOR CLASS ivymy.mysuint2_ops
	DEFAULT FOR TYPE ivymy.mysuint2 USING btree FAMILY ivymy.mysuint2_ops AS
		OPERATOR		1		< (ivymy.mysuint2, ivymy.mysuint2),
		OPERATOR		2		<= (ivymy.mysuint2, ivymy.mysuint2),
		OPERATOR		3		= (ivymy.mysuint2, ivymy.mysuint2),
		OPERATOR		4		>= (ivymy.mysuint2, ivymy.mysuint2),
		OPERATOR		5		> (ivymy.mysuint2, ivymy.mysuint2),
		FUNCTION		1		ivymy.mysuint2_cmp(ivymy.mysuint2, ivymy.mysuint2),
		FUNCTION		2		ivymy.mysuint2_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysuint2_hash(ivymy.mysuint2)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint2_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_hash_extended(ivymy.mysuint2, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint2_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint2_ops
	DEFAULT FOR TYPE ivymy.mysuint2 USING hash AS
		OPERATOR		1		= (ivymy.mysuint2, ivymy.mysuint2),
		FUNCTION		1		ivymy.mysuint2_hash(ivymy.mysuint2),
		FUNCTION		2		ivymy.mysuint2_hash_extended(ivymy.mysuint2, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint2_larger(ivymy.mysuint2, ivymy.mysuint2)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME', 'mysuint2_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysuint2) (
  SFUNC = ivymy.mysuint2_larger,
  STYPE = ivymy.mysuint2,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint2_smaller(ivymy.mysuint2, ivymy.mysuint2)
RETURNS ivymy.mysuint2
AS 'MODULE_PATHNAME', 'mysuint2_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysuint2) (
  SFUNC = ivymy.mysuint2_smaller,
  STYPE = ivymy.mysuint2,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint2_avg_accum(pg_catalog.int8[], ivymy.mysuint2)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'mysuint2_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.mysuint2) (
   sfunc = mysuint2_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.mysuint2_sum(pg_catalog.int8, ivymy.mysuint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'mysuint2_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.mysuint2) (
  SFUNC = ivymy.mysuint2_sum,
  STYPE = pg_catalog.int8
);