/***************************************************************
 *
 * MySQL uint2 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint2_in(pg_catalog.cstring)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','uint2_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_out(ivymy.uint2)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint2_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_recv(internal)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','uint2_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_send(ivymy.uint2)
RETURNS bytea
AS 'MODULE_PATHNAME','uint2_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint2_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint2_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_scale(ivymy.uint2, pg_catalog.int4)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','uint2_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint2' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='uint2_in')
WHERE oid = 9206;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='uint2_out')
WHERE oid = 9206;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='uint2_recv')
WHERE oid = 9206;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='uint2_send')
WHERE oid = 9206;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint2_typmodin')
WHERE oid = 9206;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint2_typmodout')
WHERE oid = 9206;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint2_typmodin')
WHERE oid = 9207;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint2_typmodout')
WHERE oid = 9207;

-- Compatible MySQL Cast
CREATE FUNCTION ivymy.int8_uint2(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','int8_uint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint2)
WITH FUNCTION ivymy.int8_uint2(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint2(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','int4_uint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint2)
WITH FUNCTION ivymy.int4_uint2(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint2(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','int2_uint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint2)
WITH FUNCTION ivymy.int2_uint2(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint2)
WITH FUNCTION numeric_uint2(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* Begin - BUG#Z7615 */
CREATE FUNCTION ivymy.uint2_numeric(ivymy.uint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint2_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint2 AS pg_catalog.numeric)
WITH FUNCTION ivymy.uint2_numeric(ivymy.uint2)
AS ASSIGNMENT;
/* End - BUG#Z7615 */

-- to bpchar
CREATE CAST (ivymy.uint2 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (ivymy.uint2 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (varchar as uint2)
--
CREATE FUNCTION ivymy.text_uint2(pg_catalog.varchar, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME', 'varlena_uint2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.varchar as ivymy.uint2) 
WITH FUNCTION ivymy.text_uint2(pg_catalog.varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bpchar as uint2)
--
CREATE FUNCTION ivymy.text_uint2(pg_catalog.bpchar, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME', 'varlena_uint2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.bpchar as ivymy.uint2) 
WITH FUNCTION ivymy.text_uint2(pg_catalog.bpchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (text as uint2)
--
CREATE FUNCTION ivymy.text_uint2(pg_catalog.text, int, bool, int8)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME', 'varlena_uint2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(pg_catalog.text as ivymy.uint2) 
WITH FUNCTION ivymy.text_uint2(pg_catalog.text, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (bool as uint2)
--
CREATE FUNCTION ivymy.boolean_uint2(pg_catalog.bool) 
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME','boolean_uint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.uint2)
WITH FUNCTION ivymy.boolean_uint2(pg_catalog.bool)
AS ASSIGNMENT;

--
-- cast (uint2 as uint8)
--
CREATE CAST (ivymy.uint2 AS ivymy.uint8) WITH INOUT AS IMPLICIT;

/* Begin - BUG#Z7200 */
CREATE FUNCTION ivymy.uint2_int8(ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint2 AS pg_catalog.int8)
WITH FUNCTION ivymy.uint2_int8(ivymy.uint2)
AS ASSIGNMENT;
/* End - BUG#Z7200 */

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.uint2_eq(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_equint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_equint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_eq(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_equint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.uint2_ne(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_neuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_neuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ne(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_neuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.uint2_lt(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_lt(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.uint2_gt(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_gt(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.uint2_le(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_leuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_leuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_le(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_leuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.uint2_ge(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ge(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint2_ge(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ge(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ge(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_geuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint2_ge(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_geuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_ge(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_geuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.uint2_seq(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(ivymy.uint2, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(ivymy.uint2, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(pg_catalog.int2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(pg_catalog.int4, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_seq(pg_catalog.int8, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_sequint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.uint2_add(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_adduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_adduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_adduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_add(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_adduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.uint2_negative(ivymy.uint2)
RETURNS  pg_catalog.int4
AS 'MODULE_PATHNAME','uint2_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.uint2_sub(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_subuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint2
AS 'MODULE_PATHNAME','uint2_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_subuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_subuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sub(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint2
AS 'MODULE_PATHNAME','int8_subuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.uint2_mul(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_muluint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mul(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mul(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mul(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint2_mul(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_muluint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mul(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_muluint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mul(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_muluint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.uint2_div(ivymy.uint2, ivymy.uint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint2_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(ivymy.uint2, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint2_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(ivymy.uint2, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint2_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(ivymy.uint2, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint2_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(pg_catalog.int2, ivymy.uint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(pg_catalog.int4, ivymy.uint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_div(pg_catalog.int8, ivymy.uint2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_exact_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_exact_divuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.uint2_mod(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_moduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint2_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(pg_catalog.int2, ivymy.uint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int2_moduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(pg_catalog.int4, ivymy.uint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int4_moduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_mod(pg_catalog.int8, ivymy.uint2)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int8_moduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.uint2_bitand(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitanduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitanduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitanduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitand(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitanduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.uint2_bitor(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitor(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.uint2_nega(ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_negauint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.uint2_bitxor(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitxoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_bitxor(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxoruint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.uint2_rightmov(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_rightmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_rightmov(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.uint2_leftmov(ivymy.uint2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_leftmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(ivymy.uint2, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(ivymy.uint2, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(ivymy.uint2, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint2_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(pg_catalog.int2, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(pg_catalog.int4, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_leftmov(pg_catalog.int8, ivymy.uint2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.uint2_and(ivymy.uint2, ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_anduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(ivymy.uint2, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(ivymy.uint2, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(ivymy.uint2, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(pg_catalog.int2, ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_anduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(pg_catalog.int4, ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_anduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint2_and(pg_catalog.int8, ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_anduint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.uint2_invert(ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint2_invertuint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--uint2 to boolean
CREATE FUNCTION ivymy.uint2_boolean(ivymy.uint2) 
RETURNS boolean 
AS 'MODULE_PATHNAME','uint2_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint2 AS boolean)
WITH FUNCTION ivymy.uint2_boolean(ivymy.uint2)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint2_eq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint2_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint2_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint2_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint2_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint2_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint2_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint2_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.uint2_negative,
	rightarg = ivymy.uint2
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR - (
	procedure = ivymy.uint2_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR * (
	procedure = ivymy.uint2_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR / (
	procedure = ivymy.uint2_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR % (
	procedure = ivymy.uint2_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR & (
	procedure = ivymy.uint2_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR | (
	procedure = ivymy.uint2_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.uint2_nega,
	rightarg = ivymy.uint2
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint2_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint2_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR << (
	procedure = ivymy.uint2_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint2
);

CREATE OPERATOR && (
	procedure = ivymy.uint2_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint2
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.uint2_invert,
	rightarg = ivymy.uint2
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.uint2_like(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint2_like(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint2_like(ivymy.uint2, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint2_notlike(ivymy.uint2, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint2_notlike(ivymy.uint2, ivymy.uint2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint2_notlike(ivymy.uint2, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.uint2_like,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint2_like,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint2_like,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint2_notlike,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint2_notlike,
	leftarg = ivymy.uint2,
	rightarg = ivymy.uint2
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint2_notlike,
	leftarg = ivymy.uint2,
	rightarg = pg_catalog.int8
);


--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.uint2_larger(ivymy.uint2, ivymy.uint2)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME', 'uint2_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint2) (
  SFUNC = ivymy.uint2_larger,
  STYPE = ivymy.uint2,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.uint2_smaller(ivymy.uint2, ivymy.uint2)
RETURNS ivymy.uint2
AS 'MODULE_PATHNAME', 'uint2_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint2) (
  SFUNC = ivymy.uint2_smaller,
  STYPE = ivymy.uint2,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.uint2_avg_accum(pg_catalog.int8[], ivymy.uint2)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'uint2_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint2) (
   sfunc = uint2_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.uint2_sum(pg_catalog.int8, ivymy.uint2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'uint2_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint2) (
  SFUNC = ivymy.uint2_sum,
  STYPE = pg_catalog.int8
);