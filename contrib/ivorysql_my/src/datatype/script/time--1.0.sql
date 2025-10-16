/***************************************************************
 *
 * MySQL date type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdate_in(pg_catalog.cstring)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','mysdate_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdate_out(ivymy.mysdate)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdate_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdate_recv(internal)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','mysdate_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdate_send(ivymy.mysdate)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdate_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysdate' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdate_in')
WHERE oid = 9281;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdate_out')
WHERE oid = 9281;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdate_recv')
WHERE oid = 9281;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdate_send')
WHERE oid = 9281;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdate(pg_catalog.numeric)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','numeric_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdate)
WITH FUNCTION ivymy.int_mysdate(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysdate)
WITH FUNCTION ivymy.int_mysdate(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mysdate)
WITH FUNCTION ivymy.int_mysdate(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mysdate)
WITH FUNCTION ivymy.numeric_mysdate(pg_catalog.numeric)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysdate AS pg_catalog.date)
WITH INOUT
AS ASSIGNMENT;

CREATE CAST (pg_catalog.date AS ivymy.mysdate)
WITH INOUT
AS ASSIGNMENT;

CREATE CAST (pg_catalog.timestamp AS ivymy.mysdate)
WITH INOUT
AS ASSIGNMENT;

/* set CURRENT_TIMESTAMP */
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdate)
WITH INOUT
AS ASSIGNMENT;
/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_eq(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ne(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_lt(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_gt(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_le(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdate_ge(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_ge(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(pg_catalog.int2, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysdate'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(pg_catalog.int4, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysdate'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(pg_catalog.int8, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysdate'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_seq(pg_catalog.numeric, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmysdate'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysdate_add(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_adddate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_addmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_addmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_addmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_add(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysdate_sub(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_subdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_submysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_submysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_submysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sub(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysdate_mul(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_muldate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_mulmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_mulmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_mulmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mul(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysdate_div(ivymy.mysdate, ivymy.mysdate)
RETURNS float8
AS 'MODULE_PATHNAME','mysdate_divdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(ivymy.mysdate, pg_catalog.int2)
RETURNS float8
AS 'MODULE_PATHNAME','mysdate_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(ivymy.mysdate, pg_catalog.int4)
RETURNS float8
AS 'MODULE_PATHNAME','mysdate_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(ivymy.mysdate, pg_catalog.int8)
RETURNS float8
AS 'MODULE_PATHNAME','mysdate_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(pg_catalog.int2, ivymy.mysdate)
RETURNS float8
AS 'MODULE_PATHNAME','int2_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(pg_catalog.int4, ivymy.mysdate)
RETURNS float8
AS 'MODULE_PATHNAME','int4_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(pg_catalog.int8, ivymy.mysdate)
RETURNS float8
AS 'MODULE_PATHNAME','int8_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_div(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_exact_divdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysdate_mod(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_moddate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_mod(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysdate_bitand(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitanddate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitand(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysdate_bitor(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitordate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitor(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysdate_nega(ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_negadate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysdate_bitxor(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitxordate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_bitxor(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysdate_rightmov(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_rightmovdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_rightmov(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysdate_leftmov(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_leftmovdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_leftmov(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysdate_and(ivymy.mysdate, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_anddate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(ivymy.mysdate, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(ivymy.mysdate, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(ivymy.mysdate, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(ivymy.mysdate, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(pg_catalog.int2, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(pg_catalog.int4, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(pg_catalog.int8, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_and(pg_catalog.numeric, ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysdate_cat(ivymy.mysdate, ivymy.mysdate)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysdate_invert(ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_invertmysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--date to boolean
CREATE FUNCTION ivymy.mysdate_boolean(ivymy.mysdate) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdate_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysdate AS boolean)
WITH FUNCTION ivymy.mysdate_boolean(ivymy.mysdate)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = text,
	rightarg = text
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdate_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR - (
	procedure = ivymy.mysdate_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR * (
	procedure = ivymy.mysdate_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR / (
	procedure = ivymy.mysdate_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR % (
	procedure = ivymy.mysdate_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR & (
	procedure = ivymy.mysdate_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR | (
	procedure = ivymy.mysdate_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysdate_nega,
	rightarg = ivymy.mysdate
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdate_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdate_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR << (
	procedure = ivymy.mysdate_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdate
);

CREATE OPERATOR && (
	procedure = ivymy.mysdate_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdate
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdate_cat,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysdate_invert,
	rightarg = ivymy.mysdate
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

/*
 * B-tree Index Support
 * Create Operator family 'mysdatetime_ops' for type 'mysdate/mysdatetime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdate_cmp(ivymy.mysdate, ivymy.mysdate)
RETURNS integer
AS 'MODULE_PATHNAME','mysdate_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetime_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradate' */
CREATE OPERATOR CLASS ivymy.mysdate_ops
	DEFAULT FOR TYPE ivymy.mysdate USING btree FAMILY ivymy.mysdatetime_ops AS
		OPERATOR		1		< (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		2		<= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		3		= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		4		>= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		5		> (ivymy.mysdate, ivymy.mysdate),
		FUNCTION		1		ivymy.mysdate_cmp(ivymy.mysdate, ivymy.mysdate),
		FUNCTION		2		ivymy.mysdate_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdate_hash(ivymy.mysdate)
RETURNS integer
AS 'MODULE_PATHNAME','mysdate_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdate_hash_extended(ivymy.mysdate, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mys_date_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdate_ops
	DEFAULT FOR TYPE ivymy.mysdate USING hash AS
		OPERATOR		1		= (ivymy.mysdate, ivymy.mysdate),
		FUNCTION		1		ivymy.mysdate_hash(ivymy.mysdate),
		FUNCTION		2		ivymy.mysdate_hash_extended(ivymy.mysdate, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;


-- Begin - ReqID:SRS-MYSQL-DATETIMETYPE 
/***************************************************************
 *
 * MySQL datetime type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdatetime_in(pg_catalog.cstring)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetime_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetime_out(ivymy.mysdatetime)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdatetime_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetime_recv(internal)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetime_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetime_send(ivymy.mysdatetime)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdatetime_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysdatetime' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_in')
WHERE oid = 9283;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_out')
WHERE oid = 9283;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_recv')
WHERE oid = 9283;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_send')
WHERE oid = 9283;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdatetime(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','numeric_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mysdatetime)
WITH FUNCTION ivymy.numeric_mysdatetime(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysdatetime AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamp_mysdatetime(pg_catalog.timestamp)
RETURNS ivymy.mysdatetime
AS $$
    SELECT to_char($1, 'YYYY-MM-DD HH24:MI:SS.MS')::ivymy.mysdatetime;
$$
LANGUAGE SQL
STRICT PARALLEL SAFE;

CREATE CAST (pg_catalog.timestamp AS ivymy.mysdatetime)
WITH FUNCTION ivymy.timestamp_mysdatetime(pg_catalog.timestamp)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.date AS ivymy.mysdatetime)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamptz_mysdatetime(pg_catalog.timestamptz)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','timestamptz_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdatetime)
WITH FUNCTION ivymy.timestamptz_mysdatetime(pg_catalog.timestamptz)
AS ASSIGNMENT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_eq(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ne(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_lt(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_gt(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_le(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdatetime_ge(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_ge(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(pg_catalog.int2, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysdatetime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(pg_catalog.int4, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysdatetime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(pg_catalog.int8, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysdatetime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmysdatetime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysdatetime_add(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_adddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_addmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_addmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_addmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_add(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysdatetime_sub(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_subdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_submysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_submysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_submysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sub(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysdatetime_mul(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_muldatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_mulmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_mulmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_mulmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mul(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysdatetime_div(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS float8
AS 'MODULE_PATHNAME','mysdatetime_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(ivymy.mysdatetime, pg_catalog.int2)
RETURNS float8
AS 'MODULE_PATHNAME','mysdatetime_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(ivymy.mysdatetime, pg_catalog.int4)
RETURNS float8
AS 'MODULE_PATHNAME','mysdatetime_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(ivymy.mysdatetime, pg_catalog.int8)
RETURNS float8
AS 'MODULE_PATHNAME','mysdatetime_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(pg_catalog.int2, ivymy.mysdatetime)
RETURNS float8
AS 'MODULE_PATHNAME','int2_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(pg_catalog.int4, ivymy.mysdatetime)
RETURNS float8
AS 'MODULE_PATHNAME','int4_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(pg_catalog.int8, ivymy.mysdatetime)
RETURNS float8
AS 'MODULE_PATHNAME','int8_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_div(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_exact_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysdatetime_mod(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_moddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_mod(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysdatetime_bitand(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitanddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitand(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysdatetime_bitor(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitor(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysdatetime_nega(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_negadatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysdatetime_bitxor(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitxordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_bitxor(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysdatetime_rightmov(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_rightmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_rightmov(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysdatetime_leftmov(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_leftmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_leftmov(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysdatetime_and(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_anddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(ivymy.mysdatetime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(ivymy.mysdatetime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(ivymy.mysdatetime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(ivymy.mysdatetime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(pg_catalog.int2, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(pg_catalog.int4, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(pg_catalog.int8, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_and(pg_catalog.numeric, ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysdatetime_cat(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----operator invert !
CREATE FUNCTION ivymy.mysdatetime_invert(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_invertmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--datetime to boolean
CREATE FUNCTION ivymy.mysdatetime_boolean(ivymy.mysdatetime) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdatetime_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysdatetime AS boolean)
WITH FUNCTION ivymy.mysdatetime_boolean(ivymy.mysdatetime)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetime_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetime_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetime_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetime_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetime_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetime_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetime_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysdatetime_nega,
	rightarg = ivymy.mysdatetime
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetime_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetime_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetime_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetime
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetime_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetime
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdatetime_cat,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysdatetime_invert,
	rightarg = ivymy.mysdatetime
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);
/*
 * B-tree Index Support
 * Create Operator family 'mysdatetimetime_ops' for type 'mysdatetime/mysdatetimetime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdatetime_cmp(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetime_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetime_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetimetime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetime_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING btree FAMILY ivymy.mysdatetimetime_ops AS
		OPERATOR		1		< (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		2		<= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		3		= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		4		>= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		5		> (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		1		ivymy.mysdatetime_cmp(ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		2		ivymy.mysdatetime_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdatetime_hash(ivymy.mysdatetime)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetime_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetime_hash_extended(ivymy.mysdatetime, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysdatetime_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdatetime_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING hash AS
		OPERATOR		1		= (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		1		ivymy.mysdatetime_hash(ivymy.mysdatetime),
		FUNCTION		2		ivymy.mysdatetime_hash_extended(ivymy.mysdatetime, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

/***************************************************************
 *
 * MySQL datetimes type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdatetimes_in(pg_catalog.cstring)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mysdatetimes_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetimes_out(ivymy.mysdatetimes)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdatetimes_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetimes_recv(internal)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mysdatetimes_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_send(ivymy.mysdatetimes)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdatetimes_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysdatetimes_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysdatetimes_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetimes_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mysdatetimes' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_in')
WHERE oid = 9285;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_out')
WHERE oid = 9285;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_recv')
WHERE oid = 9285;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_send')
WHERE oid = 9285;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodin')
WHERE oid = 9285;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodout')
WHERE oid = 9285;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodin')
WHERE oid = 9286;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodout')
WHERE oid = 9286;
---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int8, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int4, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int2, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdatetimes(pg_catalog.numeric, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','numeric_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int8, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int4, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int2, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.numeric_mysdatetimes(pg_catalog.numeric, integer, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysdatetimes AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mysdatetimes(pg_catalog.date, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgdate_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgdate_mysdatetimes(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mysdatetimes(pg_catalog.timestamp, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtimestamp_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtimestamp_mysdatetimes(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mysdatetimes(pg_catalog.timestamptz, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtimestamptz_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtimestamptz_mysdatetimes(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mysdatetimes(pg_catalog.time, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtime_mysdatetimes(pg_catalog.time, integer)
AS ASSIGNMENT;
/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_eq(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ne(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_lt(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_gt(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_le(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mysdatetimes_ge(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_ge(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_seqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_seq(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmysdatetimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mysdatetimes_add(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_adddatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_addmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_addmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_addmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_add(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mysdatetimes_sub(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_subdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_submysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_submysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_submysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sub(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mysdatetimes_mul(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_muldatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_mulmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_mulmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_mulmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mul(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mysdatetimes_div(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_div(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_exact_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mysdatetimes_mod(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_moddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetimes_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_modmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_modmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_modmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_mod(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mysdatetimes_bitand(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitanddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitand(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mysdatetimes_bitor(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitor(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysdatetimes_nega(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_negadatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mysdatetimes_rightmov(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_rightmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_rightmov(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mysdatetimes_leftmov(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_leftmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_leftmov(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mysdatetimes_and(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_anddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_and(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mysdatetimes_cat(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysdatetimes_invert(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_invertmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--datetime to boolean
CREATE FUNCTION ivymy.mysdatetimes_boolean(ivymy.mysdatetimes) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdatetimes_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysdatetimes AS boolean)
WITH FUNCTION ivymy.mysdatetimes_boolean(ivymy.mysdatetimes)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mysdatetimes_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR - (
	procedure = ivymy.mysdatetimes_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR * (
	procedure = ivymy.mysdatetimes_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR / (
	procedure = ivymy.mysdatetimes_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR % (
	procedure = ivymy.mysdatetimes_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR & (
	procedure = ivymy.mysdatetimes_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR | (
	procedure = ivymy.mysdatetimes_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysdatetimes_nega,
	rightarg = ivymy.mysdatetimes
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mysdatetimes_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR << (
	procedure = ivymy.mysdatetimes_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR && (
	procedure = ivymy.mysdatetimes_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdatetimes_cat,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysdatetimes_invert,
	rightarg = ivymy.mysdatetimes
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mysdatetimestime_ops' for type 'mysdatetimes/mysdatetimestime/mysdatetimes.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdatetimes_cmp(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetimes_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetimes_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetimestime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetimes_ops
	DEFAULT FOR TYPE ivymy.mysdatetimes USING btree FAMILY ivymy.mysdatetimestime_ops AS
		OPERATOR		1		< (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		2		<= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		3		= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		4		>= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		5		> (ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		1		ivymy.mysdatetimes_cmp(ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		2		ivymy.mysdatetimes_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdatetimes_hash(ivymy.mysdatetimes)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetimes_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_hash_extended(ivymy.mysdatetimes, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysdatetimes_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdatetimes_ops
	DEFAULT FOR TYPE ivymy.mysdatetimes USING hash AS
		OPERATOR		1		= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		1		ivymy.mysdatetimes_hash(ivymy.mysdatetimes),
		FUNCTION		2		ivymy.mysdatetimes_hash_extended(ivymy.mysdatetimes, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

-- End - ReqID:SRS-MYSQL-DATETIMETYPE 

-- Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE 
/***************************************************************
 *
 * MySQL timestamp type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimestamp_in(pg_catalog.cstring)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimestamp_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamp_out(ivymy.mystimestamp)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimestamp_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamp_recv(internal)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimestamp_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamp_send(ivymy.mystimestamp)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimestamp_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mystimestamp' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_in')
WHERE oid = 9287;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_out')
WHERE oid = 9287;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_recv')
WHERE oid = 9287;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_send')
WHERE oid = 9287;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimestamp(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','numeric_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimestamp)
WITH FUNCTION ivymy.numeric_mystimestamp(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystimestamp AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE CAST (pg_catalog.timestamp AS ivymy.mystimestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamptz_mystimestamp(pg_catalog.timestamptz)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','timestamptz_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimestamp)
WITH FUNCTION ivymy.timestamptz_mystimestamp(pg_catalog.timestamptz)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.date AS ivymy.mystimestamp)
WITH INOUT
AS ASSIGNMENT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_eq(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ne(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_lt(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_gt(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_le(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimestamp_ge(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_ge(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(pg_catalog.int2, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystimestamp'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(pg_catalog.int4, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystimestamp'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(pg_catalog.int8, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmystimestamp'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_seq(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmystimestamp'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_addtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamp_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_addmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_addmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_addmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_add(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_subtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamp_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_submystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_submystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_submystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sub(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_multimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamp_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_mulmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_mulmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_mulmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mul(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS float8
AS 'MODULE_PATHNAME','mystimestamp_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, pg_catalog.int2)
RETURNS float8
AS 'MODULE_PATHNAME','mystimestamp_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, pg_catalog.int4)
RETURNS float8
AS 'MODULE_PATHNAME','mystimestamp_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, pg_catalog.int8)
RETURNS float8
AS 'MODULE_PATHNAME','mystimestamp_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamp_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(pg_catalog.int2, ivymy.mystimestamp)
RETURNS float8
AS 'MODULE_PATHNAME','int2_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(pg_catalog.int4, ivymy.mystimestamp)
RETURNS float8
AS 'MODULE_PATHNAME','int4_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(pg_catalog.int8, ivymy.mystimestamp)
RETURNS float8
AS 'MODULE_PATHNAME','int8_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_div(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_modtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamp_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_mod(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitand(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitor(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimestamp_nega(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_negatimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_bitxor(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_rightmov(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_leftmov(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(pg_catalog.int2, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(pg_catalog.int4, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(pg_catalog.int8, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_and(pg_catalog.numeric, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystimestamp_invert(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_invertmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--timestamp to boolean
CREATE FUNCTION ivymy.mystimestamp_boolean(ivymy.mystimestamp) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimestamp_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimestamp AS boolean)
WITH FUNCTION ivymy.mystimestamp_boolean(ivymy.mystimestamp)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimestamp_nega,
	rightarg = ivymy.mystimestamp
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamp
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamp
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimestamp_invert,
	rightarg = ivymy.mystimestamp
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestamptime_ops' for type 'mystimestamp/mystimestamptime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimestamp_cmp(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamp_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimestamp_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestamptime_ops USING btree;

/* CREATE OPERATOR CLASS for 'mystimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamp_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING btree FAMILY ivymy.mystimestamptime_ops AS
		OPERATOR		1		< (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		2		<= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		3		= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		4		>= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		5		> (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		1		ivymy.mystimestamp_cmp(ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		2		ivymy.mystimestamp_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimestamp_hash(ivymy.mystimestamp)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamp_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamp_hash_extended(ivymy.mystimestamp, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimestamp_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimestamp_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING hash AS
		OPERATOR		1		= (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		1		ivymy.mystimestamp_hash(ivymy.mystimestamp),
		FUNCTION		2		ivymy.mystimestamp_hash_extended(ivymy.mystimestamp, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

/***************************************************************
 *
 * MySQL timestamps type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimestamps_in(pg_catalog.cstring)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamps_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamps_out(ivymy.mystimestamps)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimestamps_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamps_recv(internal)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamps_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_send(ivymy.mystimestamps)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimestamps_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mystimestamps_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mystimestamps_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimestamps_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mystimestamps' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_in')
WHERE oid = 9289;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_out')
WHERE oid = 9289;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_recv')
WHERE oid = 9289;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_send')
WHERE oid = 9289;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodin')
WHERE oid = 9289;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodout')
WHERE oid = 9289;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodin')
WHERE oid = 9290;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodout')
WHERE oid = 9290;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimestamps(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','numeric_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimestamps)
WITH FUNCTION ivymy.numeric_mystimestamps(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystimestamps AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystimestamps(pg_catalog.date, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgdate_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgdate_mystimestamps(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystimestamps(pg_catalog.timestamp, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtimestamp_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtimestamp_mystimestamps(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystimestamps(pg_catalog.timestamptz, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtimestamptz_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtimestamptz_mystimestamps(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mystimestamps(pg_catalog.time, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtime_mystimestamps(pg_catalog.time, integer)
AS ASSIGNMENT;
/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_eq(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ne(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_lt(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_gt(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_le(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimestamps_ge(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_ge(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(pg_catalog.int2, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystimestamps'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(pg_catalog.int4, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystimestamps'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(pg_catalog.int8, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmystimestamps'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_seq(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmystimestamps'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mystimestamps_add(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_addtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_addmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_addmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_addmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_add(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystimestamps_sub(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_subtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_submystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_submystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_submystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sub(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystimestamps_mul(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_multimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_mulmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_mulmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_mulmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mul(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystimestamps_div(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_div(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_exact_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystimestamps_mod(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_modtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimestamps_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_modmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_modmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_modmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_mod(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystimestamps_bitand(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitandtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitand(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystimestamps_bitor(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitor(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimestamps_nega(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_negatimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mystimestamps_rightmov(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_rightmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_rightmov(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystimestamps_leftmov(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_leftmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_leftmov(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mystimestamps_and(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_andtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_and(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mystimestamps_cat(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystimestamps_invert(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_invertmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--timestamp to boolean
CREATE FUNCTION ivymy.mystimestamps_boolean(ivymy.mystimestamps) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimestamps_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimestamps AS boolean)
WITH FUNCTION ivymy.mystimestamps_boolean(ivymy.mystimestamps)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimestamps_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR - (
	procedure = ivymy.mystimestamps_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR * (
	procedure = ivymy.mystimestamps_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR / (
	procedure = ivymy.mystimestamps_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR % (
	procedure = ivymy.mystimestamps_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR & (
	procedure = ivymy.mystimestamps_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR | (
	procedure = ivymy.mystimestamps_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimestamps_nega,
	rightarg = ivymy.mystimestamps
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimestamps_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR << (
	procedure = ivymy.mystimestamps_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR && (
	procedure = ivymy.mystimestamps_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystimestamps_cat,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimestamps_invert,
	rightarg = ivymy.mystimestamps
);

----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestampstime_ops' for type 'mystimestamps/mystimestampstime/mystimestamps.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimestamps_cmp(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamps_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimestamps_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestampstime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oratimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamps_ops
	DEFAULT FOR TYPE ivymy.mystimestamps USING btree FAMILY ivymy.mystimestampstime_ops AS
		OPERATOR		1		< (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		2		<= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		3		= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		4		>= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		5		> (ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		1		ivymy.mystimestamps_cmp(ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		2		ivymy.mystimestamps_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimestamps_hash(ivymy.mystimestamps)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamps_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_hash_extended(ivymy.mystimestamps, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimestamps_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimestamps_ops
	DEFAULT FOR TYPE ivymy.mystimestamps USING hash AS
		OPERATOR		1		= (ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		1		ivymy.mystimestamps_hash(ivymy.mystimestamps),
		FUNCTION		2		ivymy.mystimestamps_hash_extended(ivymy.mystimestamps, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

-- End - ReqID:SRS-MYSQL-TIMESTAMPTYPE 

-- Begin - ReqID:SRS-MYSQL-TIMETYPE 
/***************************************************************
 *
 * MySQL time type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystime_in(pg_catalog.cstring)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_out(ivymy.mystime)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystime_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_recv(internal)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_send(ivymy.mystime)
RETURNS bytea
AS 'MODULE_PATHNAME','mystime_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mystime' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystime_in')
WHERE oid = 9291;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystime_out')
WHERE oid = 9291;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystime_recv')
WHERE oid = 9291;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystime_send')
WHERE oid = 9291;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','numeric_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystime)
WITH FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_numeric(ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mystime AS pg_catalog.numeric)
WITH FUNCTION ivymy.mystime_numeric(ivymy.mystime)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystime AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.date AS ivymy.mystime)
WITH FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamp_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamp AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamptz_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamptz AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.time AS ivymy.mystime)
WITH INOUT
AS ASSIGNMENT;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_eq(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ne(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_lt(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_gt(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_le(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystime_ge(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_ge(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(pg_catalog.int2, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_seqmystime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(pg_catalog.int4, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(pg_catalog.int8, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmystime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_seq(pg_catalog.numeric, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmystime'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_addtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_addmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_addmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_addmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_add(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_subtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_submystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_submystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_submystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sub(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_multime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_mulmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_mulmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_mulmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mul(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_divtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_div(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_modtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_modmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_modmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_modmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_mod(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitandtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitand(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitortime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitor(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystime_nega(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_negatime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxortime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_bitxor(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmovtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_rightmov(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmovtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_leftmov(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_andtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(pg_catalog.int2, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(pg_catalog.int4, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(pg_catalog.int8, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_and(pg_catalog.numeric, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mystime_cat(ivymy.mystime, ivymy.mystime)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystime_invert(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_invertmystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--time to boolean
CREATE FUNCTION ivymy.mystime_boolean(ivymy.mystime) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystime_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS boolean)
WITH FUNCTION ivymy.mystime_boolean(ivymy.mystime)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystime_nega,
	rightarg = ivymy.mystime
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystime
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystime
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystime
);

CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystime
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystime_cat,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystime_invert,
	rightarg = ivymy.mystime
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_int4_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_int4_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimetime_ops' for type 'mystime/mystimetime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystime_ops' */
CREATE OPERATOR FAMILY ivymy.mystimetime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oratime' */
CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING btree FAMILY ivymy.mystimetime_ops AS
		OPERATOR		1		< (ivymy.mystime, ivymy.mystime),
		OPERATOR		2		<= (ivymy.mystime, ivymy.mystime),
		OPERATOR		3		= (ivymy.mystime, ivymy.mystime),
		OPERATOR		4		>= (ivymy.mystime, ivymy.mystime),
		OPERATOR		5		> (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime),
		FUNCTION		2		ivymy.mystime_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystime_hash(ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_hash_extended(ivymy.mystime, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystime_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING hash AS
		OPERATOR		1		= (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_hash(ivymy.mystime),
		FUNCTION		2		ivymy.mystime_hash_extended(ivymy.mystime, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

/***************************************************************
 *
 * MySQL times type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimes_in(pg_catalog.cstring)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimes_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimes_out(ivymy.mystimes)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimes_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimes_recv(internal)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimes_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_send(ivymy.mystimes)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimes_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mystimes_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mystimes_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimes_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mystimes' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimes_in')
WHERE oid = 9293;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimes_out')
WHERE oid = 9293;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimes_recv')
WHERE oid = 9293;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimes_send')
WHERE oid = 9293;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodin')
WHERE oid = 9293;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodout')
WHERE oid = 9293;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodin')
WHERE oid = 9294;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodout')
WHERE oid = 9294;

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int8, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int4, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int2, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimes(pg_catalog.numeric, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','numeric_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int8, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int4, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int2, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimes)
WITH FUNCTION ivymy.numeric_mystimes(pg_catalog.numeric, integer, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_numeric(ivymy.mystimes, integer)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mystimes AS pg_catalog.numeric)
WITH FUNCTION ivymy.mystimes_numeric(ivymy.mystimes, integer)
AS ASSIGNMENT;

CREATE CAST (ivymy.mystimes AS ivymy.mystime)
WITH INOUT
AS ASSIGNMENT;

---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystimes AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystimes(pg_catalog.date, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgdate_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mystimes)
WITH FUNCTION ivymy.pgdate_mystimes(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystimes(pg_catalog.timestamp, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtimestamp_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtimestamp_mystimes(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystimes(pg_catalog.timestamptz, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtimestamptz_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtimestamptz_mystimes(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mystimes(pg_catalog.time, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtime_mystimes(pg_catalog.time, integer)
AS ASSIGNMENT;
/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eqnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_eq(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_eqmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_nemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_nemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_nemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_nenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ne(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_nemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ltnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_lt(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_ltmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gtnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_gt(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gtmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_lenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_lemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_lemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_lemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_le(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_lemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_genum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.mystimes_ge(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_ge(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_gemystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seqnum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(pg_catalog.int2, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_seqmystimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(pg_catalog.int4, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqmystimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(pg_catalog.int8, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqmystimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_seq(pg_catalog.numeric, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','num_seqmystimes'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mystimes_add(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_addtimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_addnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_addmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_addmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_addmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_add(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_addmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystimes_sub(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_subtimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_subnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_submystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_submystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_submystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sub(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_submystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystimes_mul(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_muldatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_mulnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_mulmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_mulmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_mulmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mul(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_mulmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystimes_div(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_div(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_exact_divdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_exact_divnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_exact_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_exact_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_exact_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_exact_divmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystimes_mod(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_moddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystimes_modnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_modmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_modmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_modmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_mod(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','num_modmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystimes_bitand(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitanddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitandnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitandmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitandmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitandmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitand(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitandmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystimes_bitor(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitor(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimes_nega(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_negatimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.mystimes_rightmov(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_rightmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_rightmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_rightmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_rightmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_rightmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_rightmov(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_rightmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystimes_leftmov(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_leftmovdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_leftmovnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_leftmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_leftmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_leftmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_leftmov(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_leftmovmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.mystimes_and(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_anddatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_andnum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_andmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_andmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_andmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_and(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_andmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mystimes_cat(ivymy.mystimes, ivymy.mystimes)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystimes_invert(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_invertmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--datetime to boolean
CREATE FUNCTION ivymy.mystimes_boolean(ivymy.mystimes) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimes_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimes AS boolean)
WITH FUNCTION ivymy.mystimes_boolean(ivymy.mystimes)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = text,
	rightarg = text
);*/

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.mystimes_add,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR - (
	procedure = ivymy.mystimes_sub,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR * (
	procedure = ivymy.mystimes_mul,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR / (
	procedure = ivymy.mystimes_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR % (
	procedure = ivymy.mystimes_mod,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR & (
	procedure = ivymy.mystimes_bitand,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR | (
	procedure = ivymy.mystimes_bitor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimes_nega,
	rightarg = ivymy.mystimes
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR >> (
	procedure = ivymy.mystimes_rightmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR << (
	procedure = ivymy.mystimes_leftmov,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR && (
	procedure = ivymy.mystimes_and,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystimes_cat,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimes_invert,
	rightarg = ivymy.mystimes
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int4_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int4_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestime_ops' for type 'mystimes/mystimestime/mystimes.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimes_cmp(ivymy.mystimes, ivymy.mystimes)
RETURNS integer
AS 'MODULE_PATHNAME','mystimes_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimes_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mystimes_ops
	DEFAULT FOR TYPE ivymy.mystimes USING btree FAMILY ivymy.mystimestime_ops AS
		OPERATOR		1		< (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		2		<= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		3		= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		4		>= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		5		> (ivymy.mystimes, ivymy.mystimes),
		FUNCTION		1		ivymy.mystimes_cmp(ivymy.mystimes, ivymy.mystimes),
		FUNCTION		2		ivymy.mystimes_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimes_hash(ivymy.mystimes)
RETURNS integer
AS 'MODULE_PATHNAME','mystimes_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_hash_extended(ivymy.mystimes, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimes_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimes_ops
	DEFAULT FOR TYPE ivymy.mystimes USING hash AS
		OPERATOR		1		= (ivymy.mystimes, ivymy.mystimes),
		FUNCTION		1		ivymy.mystimes_hash(ivymy.mystimes),
		FUNCTION		2		ivymy.mystimes_hash_extended(ivymy.mystimes, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

-- End - ReqID:SRS-MYSQL-TIMETYPE 

-- mysql datetype
CREATE FUNCTION ivymy.mysdate_mystime(ivymy.mysdate) 
RETURNS ivymy.mystime 
AS 'MODULE_PATHNAME','mysdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystime)
WITH FUNCTION ivymy.mysdate_mystime(ivymy.mysdate)
AS ASSIGNMENT;

CREATE CAST (ivymy.mysdate AS ivymy.mysdatetime)  WITH INOUT AS IMPLICIT;
CREATE CAST (ivymy.mysdate AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;

CREATE CAST (ivymy.mysdatetime AS ivymy.mysdate)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mysdatetime)  WITH INOUT AS IMPLICIT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mysdate)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;

CREATE CAST (ivymy.mystimestamp AS ivymy.mysdate)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamp AS ivymy.mysdatetime) WITH INOUT AS IMPLICIT;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystimestamp) WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdate)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdatetime) WITH INOUT AS IMPLICIT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mysdate(ivymy.mystime) 
RETURNS ivymy.mysdate 
AS 'MODULE_PATHNAME','mystime_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mysdate)
WITH FUNCTION ivymy.mystime_mysdate(ivymy.mystime)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mysdatetime(ivymy.mystime) 
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystime_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime) 
RETURNS ivymy.mystimestamp 
AS 'MODULE_PATHNAME','mystime_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mystimestamp)
WITH FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mysdate(ivymy.mystimes) 
RETURNS ivymy.mysdate 
AS 'MODULE_PATHNAME','mystimes_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mysdate)
WITH FUNCTION ivymy.mystimes_mysdate(ivymy.mystimes)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mysdatetime(ivymy.mystimes) 
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimes_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimes AS ivymy.mysdatetime)
WITH FUNCTION ivymy.mystimes_mysdatetime(ivymy.mystimes)
AS IMPLICIT;

CREATE FUNCTION ivymy.mystimes_mystimestamp(ivymy.mystimes) 
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimes_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mystimestamp)
WITH FUNCTION ivymy.mystimes_mystimestamp(ivymy.mystimes)
AS ASSIGNMENT;

-- mysdate to datetimes timestamps times
CREATE FUNCTION ivymy.mysdate_mysdatetimes(ivymy.mysdate, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mysdate_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mysdate_mysdatetimes(ivymy.mysdate, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdate_mystimestamps(ivymy.mysdate, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdate_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdate_mystimestamps(ivymy.mysdate, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdate_mystimes(ivymy.mysdate, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdate_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdate_mystimes(ivymy.mysdate, integer)
AS ASSIGNMENT;

-- mysdatetime to datetimes timestamps times
CREATE FUNCTION ivymy.mysdatetime_mysdatetimes(ivymy.mysdatetime, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mysdatetime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mysdatetime_mysdatetimes(ivymy.mysdatetime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetime_mystimestamps(ivymy.mysdatetime, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdatetime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdatetime_mystimestamps(ivymy.mysdatetime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetime_mystimes(ivymy.mysdatetime, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdatetime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdatetime_mystimes(ivymy.mysdatetime, integer)
AS ASSIGNMENT;
-- mysdatetimes to timestamps times
CREATE FUNCTION ivymy.mysdatetimes_mystimestamps(ivymy.mysdatetimes, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mysdatetimes_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mysdatetimes_mystimestamps(ivymy.mysdatetimes, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdatetimes_mystimes(ivymy.mysdatetimes, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mysdatetimes_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystimes)
WITH FUNCTION ivymy.mysdatetimes_mystimes(ivymy.mysdatetimes, integer)
AS ASSIGNMENT;


-- mystimestamp to datetimes timestamps times
CREATE FUNCTION ivymy.mystimestamp_mysdatetimes(ivymy.mystimestamp, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mystimestamp_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimestamp_mysdatetimes(ivymy.mystimestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimestamp_mystimestamps(ivymy.mystimestamp, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamp_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystimestamp_mystimestamps(ivymy.mystimestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimestamp_mystimes(ivymy.mystimestamp, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimestamp_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystimes)
WITH FUNCTION ivymy.mystimestamp_mystimes(ivymy.mystimestamp, integer)
AS ASSIGNMENT;
-- mystimestamps to datetimes times
CREATE FUNCTION ivymy.mystimestamps_mysdatetimes(ivymy.mystimestamps, integer) 
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mystimestamps_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamps AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimestamps_mysdatetimes(ivymy.mystimestamps, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimestamps_mystimes(ivymy.mystimestamps, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimestamps_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystimes)
WITH FUNCTION ivymy.mystimestamps_mystimes(ivymy.mystimestamps, integer)
AS ASSIGNMENT;

-- mystime to datetimes timestamps times
CREATE FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mystime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystime AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mystimes)
WITH FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer)
AS IMPLICIT;

-- mystimes to datetimes timestamps
CREATE FUNCTION ivymy.mystimes_mysdatetimes(ivymy.mystimes, integer) 
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mystimes_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystimes_mysdatetimes(ivymy.mystimes, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystimes_mystimestamps(ivymy.mystimes, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimes_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimes AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystimes_mystimestamps(ivymy.mystimes, integer)
AS ASSIGNMENT;

--
-- brin index support for all data type 
--
CREATE FUNCTION ivymy.brin_minmax_opcinfo(internal)
RETURNS internal
AS 'brin_minmax_opcinfo'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_add_value(internal, internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_add_value'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_consistent(internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_consistent'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.brin_minmax_union(internal, internal, internal)
RETURNS boolean
AS 'brin_minmax_union'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* CREATE OPERATOR FAMILY */
CREATE OPERATOR FAMILY ivymy.mys_mysdate_minmax_ops USING brin;

/* CREATE OPERATOR CLASS for 'ivymy.mysdate' */
CREATE OPERATOR CLASS ivymy.mysdate_minmax_ops
	DEFAULT FOR TYPE ivymy.mysdate USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	2 	<= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	3 	=  (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	4  	>= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR	5  	>  (ivymy.mysdate, ivymy.mysdate),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);

/* CREATE OPERATOR CLASS for 'ivymy.mysdatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetime_minmax_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	2 	<= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	3 	=  (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	4  	>= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR	5  	>  (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);
		
		/* CREATE OPERATOR CLASS for 'ivymy.mystimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamp_minmax_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	2 	<= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	3 	=  (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	4  	>= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR	5  	>  (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);
		
		/* CREATE OPERATOR CLASS for 'ivymy.mystime' */
CREATE OPERATOR CLASS ivymy.mystime_minmax_ops
	DEFAULT FOR TYPE ivymy.mystime USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mystime, ivymy.mystime),
		OPERATOR	2 	<= (ivymy.mystime, ivymy.mystime),
		OPERATOR	3 	=  (ivymy.mystime, ivymy.mystime),
		OPERATOR	4  	>= (ivymy.mystime, ivymy.mystime),
		OPERATOR	5  	>  (ivymy.mystime, ivymy.mystime),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);



-- Convert MySQL date, time, time(N), datetime, datetime(N), timestamp, timestamp(N) to year
CREATE OR REPLACE FUNCTION ivymy.date2year(ivymy.mysdate)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'date_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdate AS year)
WITH FUNCTION ivymy.date2year(ivymy.mysdate) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.time2year(ivymy.mystime)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'time_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystime as year)
WITH FUNCTION ivymy.time2year(ivymy.mystime) AS ASSIGNMENT;

-- Fix Bug#Z5406 begin
CREATE OR REPLACE FUNCTION ivymy.times2year(ivymy.mystimes)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'times_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimes as year)
WITH FUNCTION ivymy.times2year(ivymy.mystimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.datetime2year(ivymy.mysdatetime)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'datetime_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdatetime as year)
WITH FUNCTION ivymy.datetime2year(ivymy.mysdatetime) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.datetimes2year(ivymy.mysdatetimes)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'datetimes_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mysdatetimes as year)
WITH FUNCTION ivymy.datetimes2year(ivymy.mysdatetimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamp2year(ivymy.mystimestamp)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamp_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimestamp as year)
WITH FUNCTION ivymy.timestamp2year(ivymy.mystimestamp) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamps2year(ivymy.mystimestamps)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamps_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystimestamps as year)
WITH FUNCTION ivymy.timestamps2year(ivymy.mystimestamps) AS ASSIGNMENT;
-- Fix Bug#Z5406 end

-- cast time as bigint
CREATE FUNCTION ivymy.time2int8(ivymy.mystime)
RETURNS bigint
AS 'MODULE_PATHNAME','time2int8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mystime as bigint)
WITH FUNCTION ivymy.time2int8(ivymy.mystime);

-- cast datetime as bigint
CREATE FUNCTION ivymy.datetime2int8(ivymy.mysdatetime)
RETURNS bigint
AS 'MODULE_PATHNAME','datetime2int8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mysdatetime as bigint)
WITH FUNCTION ivymy.datetime2int8(ivymy.mysdatetime);

-- cast datetime(x) as bigint
CREATE FUNCTION ivymy.datetimes2int8(ivymy.mysdatetimes)
RETURNS bigint
AS 'MODULE_PATHNAME','datetimes2int8'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mysdatetimes as bigint)
WITH FUNCTION ivymy.datetimes2int8(ivymy.mysdatetimes);
