/***************************************************************
 *
 * MySQL mysuint8 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint8_in(pg_catalog.cstring)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint8_out(ivymy.mysuint8)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysuint8_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysuint8_recv(internal)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_send(ivymy.mysuint8)
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

CREATE FUNCTION ivymy.mysuint8_scale(ivymy.mysuint8, pg_catalog.int4)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysuint8' in pg_type.
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
CREATE FUNCTION ivymy.int_mysuint8(pg_catalog.int8)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','int_mysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int8 AS ivymy.mysuint8)
WITH FUNCTION ivymy.int_mysuint8(pg_catalog.int8)
AS ASSIGNMENT;


CREATE FUNCTION ivymy.int_mysuint8(pg_catalog.int2)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','int_mysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.int2 AS ivymy.mysuint8)
WITH FUNCTION ivymy.int_mysuint8(pg_catalog.int2)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int_mysuint8(pg_catalog.int4)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','int_mysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.mysuint8)
WITH FUNCTION ivymy.int_mysuint8(pg_catalog.int4)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.numeric_mysuint8(pg_catalog.numeric)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME','numeric_mysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.numeric AS ivymy.mysuint8)
WITH FUNCTION ivymy.numeric_mysuint8(pg_catalog.numeric)
AS ASSIGNMENT;
/* CREATE CAST */
---------- Compatible Postgresql Cast ----------


/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysuint8_eq(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_eq(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.mysuint8_ne(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ne(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysuint8_lt(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_lt(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysuint8_gt(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_gt(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysuint8_le(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_le(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysuint8_ge(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_ge(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_ge(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysuint8_seq(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.mysuint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.mysuint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int2, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int4, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_seq(pg_catalog.int8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysuint8_add(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_add(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_addmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysuint8_sub(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sub(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_submysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysuint8_mul(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mul(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_mulmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysuint8_div(ivymy.mysuint8, ivymy.mysuint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.mysuint8, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.mysuint8, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(ivymy.mysuint8, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysuint8_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int2, ivymy.mysuint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int4, ivymy.mysuint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_div(pg_catalog.int8, ivymy.mysuint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_exact_divmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysuint8_mod(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_mod(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_modmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitand(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitandmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitor(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysuint8_nega(ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_negamysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_bitxor(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_bitxormysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_rightmov(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_rightmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.mysuint8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.mysuint8, pg_catalog.int2)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.mysuint8, pg_catalog.int4)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(ivymy.mysuint8, pg_catalog.int8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','mysuint8_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int2, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int2_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int4, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int4_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_leftmov(pg_catalog.int8, ivymy.mysuint8)
RETURNS  ivymy.mysuint8
AS 'MODULE_PATHNAME','int8_leftmovmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysuint8_and(ivymy.mysuint8, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.mysuint8, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.mysuint8, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(ivymy.mysuint8, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int2, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int4, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_and(pg_catalog.int8, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysuint8_or(ivymy.mysuint8, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_ormysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(ivymy.mysuint8, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_orint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(ivymy.mysuint8, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_orint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(ivymy.mysuint8, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_orint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(pg_catalog.int2, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_ormysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(pg_catalog.int4, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_ormysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_or(pg_catalog.int8, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_ormysuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysuint8_invert(ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysuint8_invertmysuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--mysuint8 to boolean
CREATE FUNCTION ivymy.mysuint8_boolean(ivymy.mysuint8) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysuint8_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysuint8 AS boolean)
WITH FUNCTION ivymy.mysuint8_boolean(ivymy.mysuint8)
AS ASSIGNMENT;

--mysuint8 to numeric
CREATE FUNCTION ivymy.mysuint8_numeric(ivymy.mysuint8)
RETURNS pg_catalog.numeric 
AS 'MODULE_PATHNAME','mysuint8_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysuint8 AS pg_catalog.numeric)
WITH FUNCTION ivymy.mysuint8_numeric(ivymy.mysuint8)
AS IMPLICIT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysuint8_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysuint8_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysuint8_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysuint8_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysuint8_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysuint8_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysuint8_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysuint8_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR - (
	procedure = ivymy.mysuint8_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR * (
	procedure = ivymy.mysuint8_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR / (
	procedure = ivymy.mysuint8_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR % (
	procedure = ivymy.mysuint8_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR & (
	procedure = ivymy.mysuint8_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR | (
	procedure = ivymy.mysuint8_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysuint8_nega,
	rightarg = ivymy.mysuint8
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysuint8_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysuint8_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR << (
	procedure = ivymy.mysuint8_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR && (
	procedure = ivymy.mysuint8_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR || (
	procedure = ivymy.mysuint8_or,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysuint8
);

--operator ||
CREATE OPERATOR ! (
	procedure = ivymy.mysuint8_invert,
	rightarg = ivymy.mysuint8
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysuint8_like(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_like(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_notlike(ivymy.mysuint8, ivymy.mysuint8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysuint8_notlike(ivymy.mysuint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysuint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint8_like,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysuint8_like,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint8_notlike,
	leftarg = ivymy.mysuint8,
	rightarg = ivymy.mysuint8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysuint8_notlike,
	leftarg = ivymy.mysuint8,
	rightarg = pg_catalog.int8
);

/*
 * B-tree Index Support
 * Create Operator family 'mysuint8_ops' for type 'mysuint8'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.mysuint8, ivymy.mysuint8)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint8_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint8_ops' */
CREATE OPERATOR FAMILY ivymy.mysuint8_ops USING btree;

/* CREATE OPERATOR CLASS for 'oramysuint8' */
CREATE OPERATOR CLASS ivymy.mysuint8_ops
	DEFAULT FOR TYPE ivymy.mysuint8 USING btree FAMILY ivymy.mysuint8_ops AS
		OPERATOR		1		< (ivymy.mysuint8, ivymy.mysuint8),
		OPERATOR		2		<= (ivymy.mysuint8, ivymy.mysuint8),
		OPERATOR		3		= (ivymy.mysuint8, ivymy.mysuint8),
		OPERATOR		4		>= (ivymy.mysuint8, ivymy.mysuint8),
		OPERATOR		5		> (ivymy.mysuint8, ivymy.mysuint8),
		FUNCTION		1		ivymy.mysuint8_cmp(ivymy.mysuint8, ivymy.mysuint8),
		FUNCTION		2		ivymy.mysuint8_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysuint8_hash(ivymy.mysuint8)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint8_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_hash_extended(ivymy.mysuint8, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint8_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint8_ops
	DEFAULT FOR TYPE ivymy.mysuint8 USING hash AS
		OPERATOR		1		= (ivymy.mysuint8, ivymy.mysuint8),
		FUNCTION		1		ivymy.mysuint8_hash(ivymy.mysuint8),
		FUNCTION		2		ivymy.mysuint8_hash_extended(ivymy.mysuint8, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysuint8_larger(ivymy.mysuint8, ivymy.mysuint8)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME', 'mysuint8_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysuint8) (
  SFUNC = ivymy.mysuint8_larger,
  STYPE = ivymy.mysuint8,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysuint8_smaller(ivymy.mysuint8, ivymy.mysuint8)
RETURNS ivymy.mysuint8
AS 'MODULE_PATHNAME', 'mysuint8_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysuint8) (
  SFUNC = ivymy.mysuint8_smaller,
  STYPE = ivymy.mysuint8,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.mysuint8_avg_accum(pg_catalog.int8[], ivymy.mysuint8)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'mysuint8_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.mysuint8) (
   sfunc = mysuint8_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.mysuint8_sum(pg_catalog.int8, ivymy.mysuint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'mysuint8_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.mysuint8) (
  SFUNC = ivymy.mysuint8_sum,
  STYPE = pg_catalog.int8
);
