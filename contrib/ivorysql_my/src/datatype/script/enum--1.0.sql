/***************************************************************
 *
 * MySQL anyenum type support
 *
***************************************************************/


/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.anyenum_eq(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_eqint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_eqanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_eqint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_eqanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_eqint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_eqanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_eqtext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_eq(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_eqanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.anyenum_ne(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_neint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_neint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_neint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_neanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_neanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_neanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_netext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ne(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_neanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
----function lt

CREATE FUNCTION ivymy.anyenum_lt(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_ltint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_ltint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_ltint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_ltanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_ltanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_ltanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_lttext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_lt(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_ltanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
----function gt

CREATE FUNCTION ivymy.anyenum_gt(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_gtint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_gtint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_gtint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_gtanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_gtanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_gtanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_gttext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_gt(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_gtanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.anyenum_le(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_leint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_leint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_leint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_leanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_leanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_leanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_letext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_le(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_leanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.anyenum_ge(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_geint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.anyenum_ge(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_geint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ge(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_geint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ge(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int2_geanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.anyenum_ge(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_geanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ge(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_geanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ge(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_getext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_ge(text, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','text_geanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.anyenum_seq(anyenum, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_seqanyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(anyenum, int2)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_seqint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(anyenum, int4)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_seqint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_seqint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(int2, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqanyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(int4, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int4_seqanyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_seq(int8, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','int8_seqanyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.anyenum_add(anyenum, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_addanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(anyenum, int2)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_addint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(anyenum, int4)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_addint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(anyenum, int8)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_addint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(int2, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int2_addanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(int4, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int4_addanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_add(int8, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int8_addanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function -
CREATE FUNCTION ivymy.anyenum_negative(anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.anyenum_sub(anyenum, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_subanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(anyenum, int2)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_subint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(anyenum, int4)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_subint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(anyenum, int8)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_subint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(int2, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int2_subanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(int4, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int4_subanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_sub(int8, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int8_subanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.anyenum_mul(anyenum, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_mulanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mul(anyenum, int2)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_mulint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mul(anyenum, int4)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_mulint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mul(anyenum, int8)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_mulint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE FUNCTION ivymy.anyenum_mul(int2, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int2_mulanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mul(int4, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int4_mulanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mul(int8, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int8_mulanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.anyenum_div(anyenum, anyenum)
RETURNS float8
AS 'MODULE_PATHNAME','anyenum_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(anyenum, int2)
RETURNS float8
AS 'MODULE_PATHNAME','anyenum_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(anyenum, int4)
RETURNS float8
AS 'MODULE_PATHNAME','anyenum_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(anyenum, int8)
RETURNS float8
AS 'MODULE_PATHNAME','anyenum_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(int2, anyenum)
RETURNS float8
AS 'MODULE_PATHNAME','int2_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(int4, anyenum)
RETURNS float8
AS 'MODULE_PATHNAME','int4_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_div(int8, anyenum)
RETURNS float8
AS 'MODULE_PATHNAME','int8_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(anyenum, anyenum)
RETURNS  int4
AS 'MODULE_PATHNAME','anyenum_exact_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(anyenum, int2)
RETURNS  int4
AS 'MODULE_PATHNAME','anyenum_exact_divint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(anyenum, int4)
RETURNS  int4
AS 'MODULE_PATHNAME','anyenum_exact_divint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(anyenum, int8)
RETURNS  int4
AS 'MODULE_PATHNAME','anyenum_exact_divint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(int2, anyenum)
RETURNS  int4
AS 'MODULE_PATHNAME','int2_exact_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(int4, anyenum)
RETURNS  int4
AS 'MODULE_PATHNAME','int4_exact_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(int8, anyenum)
RETURNS  int4
AS 'MODULE_PATHNAME','int8_exact_divanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.anyenum_mod(anyenum, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_modanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(anyenum, int2)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_modint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(anyenum, int4)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_modint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(anyenum, int8)
RETURNS  float8
AS 'MODULE_PATHNAME','anyenum_modint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(int2, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int2_modanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(int4, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int4_modanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_mod(int8, anyenum)
RETURNS  float8
AS 'MODULE_PATHNAME','int8_modanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.anyenum_bitand(anyenum, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitandanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(anyenum, int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitandint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(anyenum, int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitandint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(anyenum, int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitandint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(int2, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitandanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(int4, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitandanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitand(int8, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitandanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.anyenum_bitor(anyenum, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(anyenum, int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(anyenum, int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(anyenum, int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(int2, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(int4, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitor(int8, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.anyenum_nega(anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_negaanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.anyenum_bitxor(anyenum, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitxoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(anyenum, int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(anyenum, int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(anyenum, int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(int2, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_bitxoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(int4, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_bitxoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_bitxor(int8, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_bitxoranyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function >>
CREATE FUNCTION ivymy.anyenum_rightmov(anyenum, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_rightmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(anyenum, int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_rightmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(anyenum, int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_rightmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(anyenum, int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_rightmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(int2, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_rightmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(int4, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_rightmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_rightmov(int8, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_rightmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.anyenum_leftmov(anyenum, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_leftmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(anyenum, int2)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_leftmovint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(anyenum, int4)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_leftmovint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(anyenum, int8)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','anyenum_leftmovint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(int2, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int2_leftmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(int4, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int4_leftmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_leftmov(int8, anyenum)
RETURNS  ivymy.uint8
AS 'MODULE_PATHNAME','int8_leftmovanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(anyenum)
RETURNS int8
AS 'MODULE_PATHNAME','anyenum_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


----function &&
CREATE FUNCTION ivymy.anyenum_and(anyenum, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_andanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(anyenum, int2)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_andint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(anyenum, int4)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_andint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(anyenum, int8)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_andint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(int2, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int2_andanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(int4, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int4_andanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(int8, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int8_andanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(anyenum, boolean)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_andbool'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_and(boolean, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','bool_andanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.anyenum_or(anyenum, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_oranyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(anyenum, int2)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_orint2'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(anyenum, int4)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_orint4'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(anyenum, int8)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_orint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(int2, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int2_oranyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(int4, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int4_oranyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(int8, anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','int8_oranyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(anyenum, bool)
RETURNS bool
AS 'MODULE_PATHNAME','anyenum_orbool'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.anyenum_or(bool, anyenum)
RETURNS bool
AS 'MODULE_PATHNAME','bool_oranyenum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.anyenum_invert(anyenum)
RETURNS int4
AS 'MODULE_PATHNAME','anyenum_invertanyenum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

----operator =
CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = anyenum,
	rightarg = int2,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = int2,
	rightarg = anyenum,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = anyenum,
	rightarg = int4,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = int4,
	rightarg = anyenum,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = anyenum,
	rightarg = int8,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
	procedure = ivymy.anyenum_eq,
	leftarg = int8,
	rightarg = anyenum,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE OPERATOR = (
       procedure = ivymy.anyenum_eq,
       leftarg = anyenum,
       rightarg = text,
       commutator = =,
       negator = <>,
       restrict = eqsel,
       join = eqjoinsel
);

CREATE OPERATOR = (
       procedure = ivymy.anyenum_eq,
       leftarg = text,
       rightarg = anyenum,
       commutator = =,
       negator = <>,
       restrict = eqsel,
       join = eqjoinsel
);

----operator <
CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = anyenum,
	rightarg = int2,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = int2,
	rightarg = anyenum,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = anyenum,
	rightarg = int4,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = int4,
	rightarg = anyenum,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = anyenum,
	rightarg = int8,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
	procedure = ivymy.anyenum_ne,
	leftarg = int8,
	rightarg = anyenum,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR <> (
       procedure = ivymy.anyenum_ne,
       leftarg = anyenum,
       rightarg = text,
       commutator = <>,
       negator = =,
       restrict = neqsel,
       join = neqjoinsel
);
CREATE OPERATOR <> (
       procedure = ivymy.anyenum_ne,
       leftarg = text,
       rightarg = anyenum,
       commutator = <>,
       negator = =,
       restrict = neqsel,
       join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = anyenum,
	rightarg = int2,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = anyenum,
	rightarg = int4,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = anyenum,
	rightarg = int8,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel

);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = int2,
	rightarg = anyenum,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = int4,
	rightarg = anyenum,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = int8,
	rightarg = anyenum,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = anyenum,
	rightarg = text,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);
CREATE OPERATOR > (
	procedure = ivymy.anyenum_gt,
	leftarg = text,
	rightarg = anyenum,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);
----operator <

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = anyenum,
	rightarg = int2,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = int2,
	rightarg = anyenum,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = anyenum,
	rightarg = int4,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = int4,
	rightarg = anyenum,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = anyenum,
	rightarg = int8,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = int8,
	rightarg = anyenum,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = anyenum,
	rightarg = text,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);
CREATE OPERATOR < (
	procedure = ivymy.anyenum_lt,
	leftarg = text,
	rightarg = anyenum,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);
--operator <=

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = anyenum,
	rightarg = int2,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = anyenum,
	rightarg = int4,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = anyenum,
	rightarg = int8,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = int2,
	rightarg = anyenum,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = int4,
	rightarg = anyenum,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = int8,
	rightarg = anyenum,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = anyenum,
	rightarg = text,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR <= (
	procedure = ivymy.anyenum_le,
	leftarg = text,
	rightarg = anyenum,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);
--operator >=

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = anyenum,
	rightarg = int2,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = anyenum,
	rightarg = int4,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = anyenum,
	rightarg = int8,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = int2,
	rightarg = anyenum,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = int4,
	rightarg = anyenum,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = int8,
	rightarg = anyenum,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = anyenum,
	rightarg = text,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);
CREATE OPERATOR >= (
	procedure = ivymy.anyenum_ge,
	leftarg = text,
	rightarg = anyenum,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);
--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = anyenum,
	rightarg = anyenum
);
CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR <=> (
	procedure = ivymy.anyenum_seq,
	leftarg = int8,
	rightarg = anyenum
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = anyenum,
	rightarg = anyenum,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = anyenum,
	rightarg = int2,
	commutator = +
);


CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = anyenum,
	rightarg = int4,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = anyenum,
	rightarg = int8,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = int2,
	rightarg = anyenum,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = int4,
	rightarg = anyenum,
	commutator = +
);

CREATE OPERATOR + (
	procedure = ivymy.anyenum_add,
	leftarg = int8,
	rightarg = anyenum,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.anyenum_negative,
	rightarg = anyenum
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR - (
	procedure = ivymy.anyenum_sub,
	leftarg = int8,
	rightarg = anyenum
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR * (
	procedure = ivymy.anyenum_mul,
	leftarg = int8,
	rightarg = anyenum
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR / (
	procedure = ivymy.anyenum_div,
	leftarg = int8,
	rightarg = anyenum
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = int8,
	rightarg = anyenum
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR % (
	procedure = ivymy.anyenum_mod,
	leftarg = int8,
	rightarg = anyenum
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR & (
	procedure = ivymy.anyenum_bitand,
	leftarg = int8,
	rightarg = anyenum
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR | (
	procedure = ivymy.anyenum_bitor,
	leftarg = int8,
	rightarg = anyenum
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.anyenum_nega,
	rightarg = anyenum
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR ^ (
	procedure = ivymy.anyenum_bitxor,
	leftarg = int8,
	rightarg = anyenum
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR >> (
	procedure = ivymy.anyenum_rightmov,
	leftarg = int8,
	rightarg = anyenum
);


--operator <<
CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR << (
	procedure = ivymy.anyenum_leftmov,
	leftarg = int8,
	rightarg = anyenum
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = int8,
	rightarg = anyenum
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = anyenum,
	rightarg = bool
);

CREATE OPERATOR && (
	procedure = ivymy.anyenum_and,
	leftarg = bool,
	rightarg = anyenum
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = anyenum,
	rightarg = int2
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = anyenum,
	rightarg = int4
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = anyenum,
	rightarg = int8
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = int2,
	rightarg = anyenum
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = int4,
	rightarg = anyenum
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = int8,
	rightarg = anyenum
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = anyenum,
	rightarg = bool
);

CREATE OPERATOR || (
	procedure = ivymy.anyenum_or,
	leftarg = bool,
	rightarg = anyenum
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.anyenum_invert,
	rightarg = anyenum
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.anyenum_like(anyenum, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_anyenum_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.anyenum_like(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.anyenum_like(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.anyenum_notlike(anyenum, anyenum)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_anyenum_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.anyenum_notlike(anyenum, int8)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.anyenum_notlike(anyenum, text)
RETURNS boolean
AS 'MODULE_PATHNAME','anyenum_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.anyenum_like,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR ~~ (
	procedure = ivymy.anyenum_like,
	leftarg = anyenum,
	rightarg = int8
);
CREATE OPERATOR ~~ (
	procedure = ivymy.anyenum_like,
	leftarg = anyenum,
	rightarg = text
);

CREATE OPERATOR !~~ (
	procedure = ivymy.anyenum_notlike,
	leftarg = anyenum,
	rightarg = anyenum
);

CREATE OPERATOR !~~ (
	procedure = ivymy.anyenum_notlike,
	leftarg = anyenum,
	rightarg = int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.anyenum_notlike,
	leftarg = anyenum,
	rightarg = text
);

/* AVG */
CREATE FUNCTION ivymy.anyenum_avg_accum(float8[], anyenum)
RETURNS float8[]
AS 'MODULE_PATHNAME', 'anyenum_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.anyenum_avg(float8[])
RETURNS float8
AS 'MODULE_PATHNAME', 'anyenum_avg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.avg(anyenum) (
   sfunc = anyenum_avg_accum, 
   stype = float8[],
   finalfunc = anyenum_avg,
   initcond = '{0,0}'
);

/* sum */
CREATE FUNCTION ivymy.anyenum_sum(float8, anyenum)
RETURNS float8
AS 'MODULE_PATHNAME', 'anyenum_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(anyenum) (
  SFUNC = ivymy.anyenum_sum,
  STYPE = float8
);

-- for pg_dump
CREATE FUNCTION ivymy.format_anonymous_enum(oid)
RETURNS text
AS 'MODULE_PATHNAME','format_anonymous_enum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.enum_cmp(anyenum, text)
RETURNS int
AS 'MODULE_PATHNAME','enum_cmp_text'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

-- Add this operator family for partition table prune
ALTER OPERATOR FAMILY enum_ops USING btree ADD
	OPERATOR		3		=  (anyenum, text),
	FUNCTION		1		ivymy.enum_cmp(anyenum, text);
