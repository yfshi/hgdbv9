/***************************************************************
 *
 * MySQL uint1 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint1_in(pg_catalog.cstring)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','uint1_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_out(ivymy.uint1)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint1_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_recv(internal)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','uint1_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_send(ivymy.uint1)
RETURNS bytea
AS 'MODULE_PATHNAME','uint1_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint1_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint1_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_scale(ivymy.uint1, pg_catalog.int4)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','uint1_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint1' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='uint1_in')
WHERE oid = 9202;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='uint1_out')
WHERE oid = 9202;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='uint1_recv')
WHERE oid = 9202;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='uint1_send')
WHERE oid = 9202;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint1_typmodin')
WHERE oid = 9202;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint1_typmodout')
WHERE oid = 9202;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint1_typmodin')
WHERE oid = 9203;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint1_typmodout')
WHERE oid = 9203;

CREATE FUNCTION ivymy.int8_uint1(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int8_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint1)
WITH FUNCTION ivymy.int8_uint1(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint1(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int4_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint1)
WITH FUNCTION ivymy.int4_uint1(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint1(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','int2_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint1)
WITH FUNCTION ivymy.int2_uint1(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint1)
WITH FUNCTION numeric_uint1(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* Begin - BUG#Z7615 */
CREATE FUNCTION ivymy.uint1_numeric(ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint1_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint1 AS pg_catalog.numeric)
WITH FUNCTION ivymy.uint1_numeric(ivymy.uint1)
AS ASSIGNMENT;
/* End - BUG#Z7615 */

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
CREATE FUNCTION ivymy.boolean_uint1(pg_catalog.bool) 
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME','boolean_uint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.uint1)
WITH FUNCTION ivymy.boolean_uint1(pg_catalog.bool)
AS ASSIGNMENT;

--
-- cast (uint1 as uint8)
--
CREATE CAST (ivymy.uint1 AS ivymy.uint8) WITH INOUT AS IMPLICIT;

/* Begin - BUG#Z7200 */
CREATE FUNCTION ivymy.uint1_int8(ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint1 AS pg_catalog.int8)
WITH FUNCTION ivymy.uint1_int8(ivymy.uint1)
AS ASSIGNMENT;
/* End - BUG#Z7200 */

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.uint1_eq(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_equint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_equint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_eq(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_equint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function ne
CREATE FUNCTION ivymy.uint1_ne(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_neuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_neuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ne(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_neuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.uint1_lt(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_lt(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.uint1_gt(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_gt(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.uint1_le(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_leuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_leuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_le(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_leuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.uint1_ge(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ge(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint1_ge(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ge(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ge(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_geuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint1_ge(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_geuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_ge(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_geuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.uint1_seq(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(ivymy.uint1, pg_catalog.int2)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(ivymy.uint1, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(pg_catalog.int2, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(pg_catalog.int4, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_sequint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_seq(pg_catalog.int8, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_sequint1'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.uint1_add(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_adduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_adduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_adduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_add(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_adduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function negative
CREATE FUNCTION ivymy.uint1_negative(ivymy.uint1)
RETURNS  pg_catalog.int4
AS 'MODULE_PATHNAME','uint1_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.uint1_sub(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_subuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint1
AS 'MODULE_PATHNAME','uint1_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_subuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_subuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sub(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint1
AS 'MODULE_PATHNAME','int8_subuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.uint1_mul(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_muluint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mul(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mul(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mul(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.uint1_mul(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_muluint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mul(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_muluint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mul(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_muluint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.uint1_div(ivymy.uint1, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint1_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(ivymy.uint1, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint1_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(ivymy.uint1, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint1_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(ivymy.uint1, pg_catalog.int8)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','uint1_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(pg_catalog.int2, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int2_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(pg_catalog.int4, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int4_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_div(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int8_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_exact_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int2_exact_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','int4_exact_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_exact_divuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.uint1_mod(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_moduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint4
AS 'MODULE_PATHNAME','uint1_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(pg_catalog.int2, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int2_moduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(pg_catalog.int4, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int4_moduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_mod(pg_catalog.int8, ivymy.uint1)
RETURNS  pg_catalog.int8
AS 'MODULE_PATHNAME','int8_moduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.uint1_bitand(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitanduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitanduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitanduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitand(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitanduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.uint1_bitor(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitor(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.uint1_nega(ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_negauint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.uint1_bitxor(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitxoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_bitxor(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxoruint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.uint1_rightmov(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_rightmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_rightmov(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.uint1_leftmov(ivymy.uint1, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_leftmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(ivymy.uint1, pg_catalog.int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(ivymy.uint1, pg_catalog.int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(ivymy.uint1, pg_catalog.int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','uint1_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(pg_catalog.int2, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(pg_catalog.int4, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_leftmov(pg_catalog.int8, ivymy.uint1)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.uint1_and(ivymy.uint1, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_anduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(ivymy.uint1, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(ivymy.uint1, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(ivymy.uint1, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(pg_catalog.int2, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_anduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(pg_catalog.int4, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_anduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.uint1_and(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_anduint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.uint1_invert(ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint1_invertuint1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

--uint1 to boolean
CREATE FUNCTION ivymy.uint1_boolean(ivymy.uint1) 
RETURNS boolean 
AS 'MODULE_PATHNAME','uint1_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint1 AS boolean)
WITH FUNCTION ivymy.uint1_boolean(ivymy.uint1)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
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
	procedure = ivymy.uint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	merges
);

CREATE OPERATOR = (
	procedure = ivymy.uint1_eq,
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
	procedure = ivymy.uint1_ne,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.uint1_ne,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.uint1_gt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.uint1_lt,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.uint1_le,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = >=,
	negator = >,
	restrict = scalarlesel,
	join = scalarlejoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.uint1_ge,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = <=,
	negator = <,
	restrict = scalargesel,
	join = scalargejoinsel

);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR <=> (
	procedure = ivymy.uint1_seq,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.uint1_add,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.uint1_negative,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR - (
	procedure = ivymy.uint1_sub,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR * (
	procedure = ivymy.uint1_mul,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR / (
	procedure = ivymy.uint1_div,
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
	procedure = ivymy.uint1_mod,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR % (
	procedure = ivymy.uint1_mod,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR & (
	procedure = ivymy.uint1_bitand,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR | (
	procedure = ivymy.uint1_bitor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.uint1_nega,
	rightarg = ivymy.uint1
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ^ (
	procedure = ivymy.uint1_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR >> (
	procedure = ivymy.uint1_rightmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR << (
	procedure = ivymy.uint1_leftmov,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.uint1
);

CREATE OPERATOR && (
	procedure = ivymy.uint1_and,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.uint1
);

--operator ||
CREATE OPERATOR ! (
	procedure = ivymy.uint1_invert,
	rightarg = ivymy.uint1
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.uint1_like(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint1_like(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint1_like(ivymy.uint1, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint1_notlike(ivymy.uint1, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint1_notlike(ivymy.uint1, ivymy.uint1)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.uint1_notlike(ivymy.uint1, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','uint1_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE OPERATOR ~~ (
	procedure = ivymy.uint1_like,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint1_like,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.uint1_like,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = ivymy.uint1
);

CREATE OPERATOR !~~ (
	procedure = ivymy.uint1_notlike,
	leftarg = ivymy.uint1,
	rightarg = pg_catalog.int8
);

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.uint1_larger(ivymy.uint1, ivymy.uint1)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'uint1_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.uint1) (
  SFUNC = ivymy.uint1_larger,
  STYPE = ivymy.uint1,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.uint1_smaller(ivymy.uint1, ivymy.uint1)
RETURNS ivymy.uint1
AS 'MODULE_PATHNAME', 'uint1_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.uint1) (
  SFUNC = ivymy.uint1_smaller,
  STYPE = ivymy.uint1,
  SORTOP = <,
  PARALLEL = SAFE
);

/* AVG */
CREATE FUNCTION ivymy.uint1_avg_accum(pg_catalog.int8[], ivymy.uint1)
RETURNS pg_catalog.int8[]
AS 'MODULE_PATHNAME', 'uint1_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(ivymy.uint1) (
   sfunc = uint1_avg_accum, 
   stype = bigint[],
   finalfunc = int8_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.uint1_sum(pg_catalog.int8, ivymy.uint1)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'uint1_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(ivymy.uint1) (
  SFUNC = ivymy.uint1_sum,
  STYPE = pg_catalog.int8
);