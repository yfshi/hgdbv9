/***************************************************************
 *
 * MySQL uint8 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint8_in(pg_catalog.cstring, oid, integer, bigint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','uint8_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_out(ivymy.uint8)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint8_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_recv(internal, oid, integer, bigint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','uint8_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_send(ivymy.uint8)
RETURNS bytea
AS 'MODULE_PATHNAME','uint8_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint8_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint8_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_scale(ivymy.uint8, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','uint8_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint8' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='uint8_in')
WHERE oid = 9218;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='uint8_out')
WHERE oid = 9218;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='uint8_recv')
WHERE oid = 9218;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='uint8_send')
WHERE oid = 9218;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint8_typmodin')
WHERE oid = 9218;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint8_typmodout')
WHERE oid = 9218;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint8_typmodin')
WHERE oid = 9219;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint8_typmodout')
WHERE oid = 9219;

-- Compatible MySQL Cast
CREATE FUNCTION ivymy.int8_uint8(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int8_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint8)
WITH FUNCTION ivymy.int8_uint8(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint8(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int4_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint8)
WITH FUNCTION ivymy.int4_uint8(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint8(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int2_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint8)
WITH FUNCTION ivymy.int2_uint8(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint8)
WITH FUNCTION numeric_uint8(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* Begin BUG#5048 5070 */
-- cast uint8 to int2
CREATE FUNCTION ivymy.uint8_int2(ivymy.uint8, int, bool, int8)
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','uint8_int2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
IMMUTABLE;

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
CREATE FUNCTION ivymy.uint8_eq(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_equint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_equint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_eq(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_equint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.uint8_ne(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_neuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_neuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ne(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_neuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.uint8_lt(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_lt(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.uint8_gt(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_gt(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.uint8_le(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_leuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_leuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_le(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_leuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.uint8_ge(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ge(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint8_ge(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ge(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ge(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_geuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint8_ge(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_geuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_ge(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_geuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.uint8_seq(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(ivymy.uint8, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(ivymy.uint8, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(pg_catalog.int2, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(pg_catalog.int4, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_seq(pg_catalog.int8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_sequint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.uint8_add(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_adduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_adduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_adduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_add(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_adduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.uint8_negative(ivymy.uint8)
RETURNS  bigint
AS 'MODULE_PATHNAME','uint8_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.uint8_sub(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_subuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_subuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_subuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sub(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_subuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.uint8_mul(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_muluint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint8_mul(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_muluint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_muluint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_muluint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* Begin - BUG#6039 */
CREATE FUNCTION ivymy.uint8_mul(double, ivymy.uint8)
RETURNS  double
AS 'MODULE_PATHNAME','double_muluint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mul(ivymy.uint8, double)
RETURNS  double
AS 'MODULE_PATHNAME','uint8_muldouble'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
/* End - BUG#6039 */

----function divide
CREATE FUNCTION ivymy.uint8_div(ivymy.uint8, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint8_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(ivymy.uint8, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint8_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(ivymy.uint8, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint8_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(ivymy.uint8, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint8_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(pg_catalog.int2, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(pg_catalog.int4, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_div(pg_catalog.int8, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_exact_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_exact_divuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.uint8_mod(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_moduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mod(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(pg_catalog.int2, ivymy.uint8)
RETURNS int8
AS 'MODULE_PATHNAME','int2_moduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(pg_catalog.int4, ivymy.uint8)
RETURNS  int8
AS 'MODULE_PATHNAME','int4_moduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_mod(pg_catalog.int8, ivymy.uint8)
RETURNS  int8
AS 'MODULE_PATHNAME','int8_moduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.uint8_bitand(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitanduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitanduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitanduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitand(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitanduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.uint8_bitor(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitor(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.uint8_nega(ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_negauint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.uint8_bitxor(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitxoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_bitxor(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxoruint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.uint8_rightmov(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_rightmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_rightmov(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.uint8_leftmov(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_leftmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(ivymy.uint8, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(ivymy.uint8, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(ivymy.uint8, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(pg_catalog.int2, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(pg_catalog.int4, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_leftmov(pg_catalog.int8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovuint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint8_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.uint8_and(ivymy.uint8, ivymy.uint8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint8_anduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(ivymy.uint8, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint8_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(ivymy.uint8, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint8_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(ivymy.uint8, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint8_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(pg_catalog.int2, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_anduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(pg_catalog.int4, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_anduint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint8_and(pg_catalog.int8, ivymy.uint8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_anduint8'
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
CREATE FUNCTION ivymy.uint8_invert(ivymy.uint8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint8_invertuint8'
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

CREATE FUNCTION ivymy.uint8_boolean(ivymy.uint8) 
RETURNS boolean 
AS 'MODULE_PATHNAME','uint8_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint8 AS boolean)
WITH FUNCTION ivymy.uint8_boolean(ivymy.uint8)
AS ASSIGNMENT;

--uint8 to numeric
CREATE FUNCTION ivymy.uint8_numeric(ivymy.uint8)
RETURNS pg_catalog.numeric 
AS 'MODULE_PATHNAME','uint8_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint8 AS pg_catalog.numeric)
WITH FUNCTION ivymy.uint8_numeric(ivymy.uint8)
AS IMPLICIT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
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
	procedure = ivymy.uint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint8_eq,
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
	procedure = ivymy.uint8_ne,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint8_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint8_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint8_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint8_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint8_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint8_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint8_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.uint8_negative,
	rightarg = ivymy.uint8
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR - (
	procedure = ivymy.uint8_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = double,
	rightarg = ivymy.uint8
);

CREATE OPERATOR * (
	procedure = ivymy.uint8_mul,
	leftarg = ivymy.uint8,
	rightarg = double
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR / (
	procedure = ivymy.uint8_div,
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
	procedure = ivymy.uint8_mod,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR % (
	procedure = ivymy.uint8_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR & (
	procedure = ivymy.uint8_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR | (
	procedure = ivymy.uint8_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.uint8_nega,
	rightarg = ivymy.uint8
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint8_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint8_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR << (
	procedure = ivymy.uint8_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint8
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint8
);

CREATE OPERATOR && (
	procedure = ivymy.uint8_and,
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
	procedure = ivymy.uint8_invert,
	rightarg = ivymy.uint8
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.uint8_like(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint8_like(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint8_like(ivymy.uint8, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint8_notlike(ivymy.uint8, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint8_notlike(ivymy.uint8, ivymy.uint8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint8_notlike(ivymy.uint8, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.uint8_like,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint8_like,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint8_like,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = ivymy.uint8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint8_notlike,
	leftarg = ivymy.uint8,
	rightarg = pg_catalog.int8
);

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.uint8_larger(ivymy.uint8, ivymy.uint8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'uint8_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint8) (
  SFUNC = ivymy.uint8_larger,
  STYPE = ivymy.uint8,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.uint8_smaller(ivymy.uint8, ivymy.uint8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME', 'uint8_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint8) (
  SFUNC = ivymy.uint8_smaller,
  STYPE = ivymy.uint8,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.uint8_avg_accum(internal, ivymy.uint8)
RETURNS internal
AS 'MODULE_PATHNAME', 'uint8_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_avg(internal)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME', 'uint8_avg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint8) (
   sfunc = uint8_avg_accum, 
   stype = internal,
   finalfunc = uint8_avg
);

/* sum */
CREATE FUNCTION ivymy.uint8_sum(pg_catalog.numeric, ivymy.uint8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME', 'uint8_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint8) (
  SFUNC = ivymy.uint8_sum,
  STYPE = pg_catalog.numeric
);

--
-- CREATE FUNCTION
--
CREATE FUNCTION ivymy.MOD(ivymy.uint8, ivymy.uint8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint8_moduint8'
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