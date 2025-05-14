/***************************************************************
 *
 * MySQL mysuint1 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint1_in(pg_catalog.cstring)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','mysuint1_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint1_out(ivymy.mysuint1)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint1_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint1_recv(internal)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','mysuint1_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_send(ivymy.mysuint1)
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

CREATE FUNCTION ivymy.mysuint1_scale(ivymy.mysuint1, pg_catalog.int4)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','mysuint1_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysuint1' in pg_type.
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
---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysuint1(pg_catalog.int8)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','int_mysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int8 AS ivymy.mysuint1)
WITH FUNCTION ivymy.int_mysuint1(pg_catalog.int8)
AS ASSIGNMENT;


CREATE FUNCTION ivymy.int_mysuint1(pg_catalog.int2)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','int_mysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int2 AS ivymy.mysuint1)
WITH FUNCTION ivymy.int_mysuint1(pg_catalog.int2)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int_mysuint1(pg_catalog.int4)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','int_mysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.mysuint1)
WITH FUNCTION ivymy.int_mysuint1(pg_catalog.int4)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.numeric_mysuint1(pg_catalog.numeric)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME','numeric_mysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.numeric AS ivymy.mysuint1)
WITH FUNCTION ivymy.numeric_mysuint1(pg_catalog.numeric)
AS ASSIGNMENT;
/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysuint1 AS ivymy.mysuint4) WITH INOUT AS IMPLICIT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint1_eq(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_eq(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint1_ne(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ne(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint1_lt(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_lt(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint1_gt(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_gt(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint1_le(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_le(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint1_ge(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_ge(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_ge(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint1_seq(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.mysuint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.mysuint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int2, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int4, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_seq(pg_catalog.int8, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint1_add(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_add(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_addmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint1_sub(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint1
AS 'MODULE_PATHNAME','mysuint1_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sub(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint1
AS 'MODULE_PATHNAME','int8_submysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint1_mul(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mul(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_mulmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysuint1_div(ivymy.mysuint1, ivymy.mysuint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.mysuint1, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.mysuint1, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(ivymy.mysuint1, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint1_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int2, ivymy.mysuint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int4, ivymy.mysuint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_div(pg_catalog.int8, ivymy.mysuint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int2_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','int4_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_exact_divmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint1_mod(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint4
AS 'MODULE_PATHNAME','mysuint1_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int2, ivymy.mysuint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int4, ivymy.mysuint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_mod(pg_catalog.int8, ivymy.mysuint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitand(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitandmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitor(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint1_nega(ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_negamysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_bitxor(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitxormysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_rightmov(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.mysuint1, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.mysuint1, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.mysuint1, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(ivymy.mysuint1, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint1_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int2, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int4, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_leftmov(pg_catalog.int8, ivymy.mysuint1)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint1_and(ivymy.mysuint1, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.mysuint1, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.mysuint1, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(ivymy.mysuint1, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int2, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int4, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_and(pg_catalog.int8, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysuint1_or(ivymy.mysuint1, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_ormysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(ivymy.mysuint1, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_orint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(ivymy.mysuint1, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_orint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(ivymy.mysuint1, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_orint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(pg_catalog.int2, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_ormysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(pg_catalog.int4, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_ormysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_or(pg_catalog.int8, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_ormysuint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint1_invert(ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint1_invertmysuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--mysuint1 to boolean
CREATE FUNCTION ivymy.mysuint1_boolean(ivymy.mysuint1) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint1_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysuint1 AS boolean)
WITH FUNCTION ivymy.mysuint1_boolean(ivymy.mysuint1)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint1_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint1_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint1_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint1_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint1_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint1_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint1_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint1_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint1_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint1_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint1_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint1_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint1_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint1_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint1_nega,
	rightarg = ivymy.mysuint1
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint1_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint1_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint1_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint1_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint1_or,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint1
);

--operator ||
CREATE OPERATOR ! (
	procedure = ivymy.mysuint1_invert,
	rightarg = ivymy.mysuint1
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint1_like(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_like(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_notlike(ivymy.mysuint1, ivymy.mysuint1)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint1_notlike(ivymy.mysuint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint1_like,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint1_like,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint1_notlike,
	leftarg = ivymy.mysuint1,
	rightarg = ivymy.mysuint1
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint1_notlike,
	leftarg = ivymy.mysuint1,
	rightarg = pg_catalog.int8
);

/*
 * B-tree Index Support
 * Create Operator family 'mysuint1_ops' for type 'mysuint1'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint1)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint1_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint1_ops' */
CREATE OPERATOR FAMILY ivymy.mysuint1_ops USING btree;

/* CREATE OPERATOR CLASS for 'oramysuint1' */
CREATE OPERATOR CLASS ivymy.mysuint1_ops
	DEFAULT FOR TYPE ivymy.mysuint1 USING btree FAMILY ivymy.mysuint1_ops AS
		OPERATOR		1		< (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		2		<= (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		3		= (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		4		>= (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		5		> (ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		2		ivymy.mysuint1_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysuint1_hash(ivymy.mysuint1)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint1_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_hash_extended(ivymy.mysuint1, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint1_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint1_ops
	DEFAULT FOR TYPE ivymy.mysuint1 USING hash AS
		OPERATOR		1		= (ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		1		ivymy.mysuint1_hash(ivymy.mysuint1),
		FUNCTION		2		ivymy.mysuint1_hash_extended(ivymy.mysuint1, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint1_larger(ivymy.mysuint1, ivymy.mysuint1)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME', 'mysuint1_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysuint1) (
  SFUNC = ivymy.mysuint1_larger,
  STYPE = ivymy.mysuint1,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint1_smaller(ivymy.mysuint1, ivymy.mysuint1)
RETURNS ivymy.mysuint1
AS 'MODULE_PATHNAME', 'mysuint1_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysuint1) (
  SFUNC = ivymy.mysuint1_smaller,
  STYPE = ivymy.mysuint1,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint1_avg_accum(pg_catalog.int8[], ivymy.mysuint1)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'mysuint1_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.mysuint1) (
   sfunc = mysuint1_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.mysuint1_sum(pg_catalog.int8, ivymy.mysuint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'mysuint1_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.mysuint1) (
  SFUNC = ivymy.mysuint1_sum,
  STYPE = pg_catalog.int8
);