------------------------------------------------------------------
-- mysql numeric : cat
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_cat(numeric, numeric)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.|| (
	procedure = ivymy.numeric_cat,
	leftarg = numeric,
	rightarg = numeric,
	commutator = ||
);

CREATE FUNCTION ivymy.numeric_bool(numeric)
RETURNS bool
AS 'select (!(!$1))::bool;'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE CAST (numeric AS bool)
WITH FUNCTION ivymy.numeric_bool(numeric)
AS ASSIGNMENT;

------------------------------------------------------------------
-- mysql numeric : eq
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_eq(numeric, int8)
RETURNS bool
AS 'MODULE_PATHNAME', 'numeric_eq_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR = (
	procedure = ivymy.numeric_eq,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_eq(numeric, int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'numeric_eq_int4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR = (
	procedure = ivymy.numeric_eq,
	leftarg = numeric,
	rightarg = int4
);

------------------------------------------------------------------
-- mysql numeric : ne
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_ne(numeric, int8)
RETURNS bool
AS 'select $1 != $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <> (
	procedure = ivymy.numeric_ne,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_ne(int8, numeric)
RETURNS bool
AS 'select $1::numeric != $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <> (
	procedure = ivymy.numeric_ne,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_ne(numeric, int4)
RETURNS bool
AS 'select $1 != $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <> (
	procedure = ivymy.numeric_ne,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_ne(int4, numeric)
RETURNS bool
AS 'select $1::numeric != $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <> (
	procedure = ivymy.numeric_ne,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : gt
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_gt(numeric, int8)
RETURNS bool
AS 'select $1 > $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR > (
	procedure = ivymy.numeric_gt,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_gt(int8, numeric)
RETURNS bool
AS 'select $1::numeric > $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR > (
	procedure = ivymy.numeric_gt,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_gt(numeric, int4)
RETURNS bool
AS 'select $1 > $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR > (
	procedure = ivymy.numeric_gt,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_gt(int4, numeric)
RETURNS bool
AS 'select $1::numeric > $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR > (
	procedure = ivymy.numeric_gt,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : lt
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_lt(numeric, int8)
RETURNS bool
AS 'select $1 < $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR < (
	procedure = ivymy.numeric_lt,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_lt(int8, numeric)
RETURNS bool
AS 'select $1::numeric < $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR < (
	procedure = ivymy.numeric_lt,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_lt(numeric, int4)
RETURNS bool
AS 'select $1 < $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR < (
	procedure = ivymy.numeric_lt,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_lt(int4, numeric)
RETURNS bool
AS 'select $1::numeric < $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR < (
	procedure = ivymy.numeric_lt,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : le
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_le(numeric, int8)
RETURNS bool
AS 'select $1 <= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <= (
	procedure = ivymy.numeric_le,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_le(int8, numeric)
RETURNS bool
AS 'select $1::numeric <= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <= (
	procedure = ivymy.numeric_le,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_le(numeric, int4)
RETURNS bool
AS 'select $1 <= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <= (
	procedure = ivymy.numeric_le,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_le(int4, numeric)
RETURNS bool
AS 'select $1::numeric <= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <= (
	procedure = ivymy.numeric_le,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : ge
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_ge(numeric, int8)
RETURNS bool
AS 'select $1 >= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >= (
	procedure = ivymy.numeric_ge,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_ge(int8, numeric)
RETURNS bool
AS 'select $1::numeric >= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >= (
	procedure = ivymy.numeric_ge,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_ge(numeric, int4)
RETURNS bool
AS 'select $1 >= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >= (
	procedure = ivymy.numeric_ge,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_ge(int4, numeric)
RETURNS bool
AS 'select $1::numeric >= $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >= (
	procedure = ivymy.numeric_ge,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : seq
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_seq(numeric, int4)
RETURNS bool
AS 'select $1 <=> $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.numeric_seq,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_seq(int4, numeric)
RETURNS bool
AS 'select $1::numeric <=> $2'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.numeric_seq,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_seq(numeric, int8)
RETURNS bool
AS 'select $1 <=> $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.numeric_seq,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_seq(int8, numeric)
RETURNS bool
AS 'select $1::numeric <=> $2'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.numeric_seq,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_seq(numeric, numeric)
RETURNS bool
AS 'select $1::float8 <=> $2::float8'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.numeric_seq,
	leftarg = numeric,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : add
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_add(numeric, int8)
RETURNS numeric
AS 'MODULE_PATHNAME', 'numeric_add_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR + (
	procedure = ivymy.numeric_add,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_add(int8, numeric)
RETURNS numeric
AS 'MODULE_PATHNAME', 'int8_add_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR + (
	procedure = ivymy.numeric_add,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_add(numeric, int4)
RETURNS numeric
AS 'MODULE_PATHNAME', 'numeric_add_int4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR + (
	procedure = ivymy.numeric_add,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_add(int4, numeric)
RETURNS numeric
AS 'MODULE_PATHNAME', 'int4_add_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR + (
	procedure = ivymy.numeric_add,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : del
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_del(numeric, int8)
RETURNS numeric
AS 'select $1 - $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR - (
	procedure = ivymy.numeric_del,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_del(int8, numeric)
RETURNS numeric
AS 'select $1::numeric - $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR - (
	procedure = ivymy.numeric_del,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_del(numeric, int4)
RETURNS numeric
AS 'select $1 - $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR - (
	procedure = ivymy.numeric_del,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_del(int4, numeric)
RETURNS numeric
AS 'select $1::numeric - $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR - (
	procedure = ivymy.numeric_del,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : *
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_mul(numeric, int8)
RETURNS numeric
AS 'select $1 * $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_mul(int8, numeric)
RETURNS numeric
AS 'select $1::numeric * $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_mul(numeric, int4)
RETURNS numeric
AS 'select $1 * $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_mul(int4, numeric)
RETURNS numeric
AS 'select $1::numeric * $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = int4,
	rightarg = numeric
);

/* End - bug#z6039 */
CREATE FUNCTION ivymy.numeric_mul(uint8, numeric)
RETURNS numeric
AS 'MODULE_PATHNAME', 'uint8_mul_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = uint8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_mul(numeric, uint8)
RETURNS numeric
AS 'MODULE_PATHNAME', 'numeric_mul_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR * (
	procedure = ivymy.numeric_mul,
	leftarg = numeric,
	rightarg = uint8
);
/* End - bug#z6039 */

------------------------------------------------------------------
-- mysql numeric : /
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_div(numeric, int8)
RETURNS numeric
AS 'select $1 / $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_div(int8, numeric)
RETURNS numeric
AS 'select $1::numeric / $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_div(numeric, int4)
RETURNS numeric
AS 'select $1 / $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_div(int4, numeric)
RETURNS numeric
AS 'select $1::numeric / $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_div(numeric, uint4)
RETURNS numeric
AS 'select $1 / $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = numeric,
	rightarg = uint4
);

CREATE FUNCTION ivymy.numeric_div(uint4, numeric)
RETURNS numeric
AS 'select $1::numeric / $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.numeric_div,
	leftarg = uint4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : extra div //
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_extra_div(numeric, int4)
RETURNS int8
AS 'select $1 div $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.numeric_extra_div,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_extra_div(int4, numeric)
RETURNS int8
AS 'select $1::numeric div $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.numeric_extra_div,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_extra_div(numeric, int8)
RETURNS int8
AS 'select $1 div $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.numeric_extra_div,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_extra_div(int8, numeric)
RETURNS int8
AS 'select $1::numeric div $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.numeric_extra_div,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_extra_div(pg_catalog.numeric, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'select $1::float8 div $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.numeric_extra_div,
	leftarg = numeric,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : mod
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_mod(numeric, int8)
RETURNS numeric
AS 'select $1 % $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_mod(int8, numeric)
RETURNS numeric
AS 'select $1::numeric % $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_mod(numeric, int4)
RETURNS numeric
AS 'select $1 % $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_mod(int4, numeric)
RETURNS numeric
AS 'select $1::numeric % $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_mod(numeric, uint4)
RETURNS numeric
AS 'select $1 % $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = numeric,
	rightarg = uint4
);

CREATE FUNCTION ivymy.numeric_mod(uint4, numeric)
RETURNS numeric
AS 'select $1::numeric % $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.numeric_mod,
	leftarg = uint4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : &
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_bitand(numeric, int4)
RETURNS uint8
AS 'select $1 & $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.numeric_bitand,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_bitand(int4, numeric)
RETURNS uint8
AS 'select $1::numeric & $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.numeric_bitand,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_bitand(numeric, int8)
RETURNS uint8
AS 'select $1 & $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.numeric_bitand,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_bitand(int8, numeric)
RETURNS uint8
AS 'select $1::numeric & $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.numeric_bitand,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_bitand(numeric, numeric)
RETURNS uint8
AS 'select $1::float8 & $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.numeric_bitand,
	leftarg = numeric,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : |
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_bitor(numeric, int4)
RETURNS uint8
AS 'select $1 | $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.numeric_bitor,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_bitor(int4, numeric)
RETURNS uint8
AS 'select $1::numeric | $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.numeric_bitor,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_bitor(numeric, int8)
RETURNS uint8
AS 'select $1 | $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.numeric_bitor,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_bitor(int8, numeric)
RETURNS uint8
AS 'select $1::numeric | $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.numeric_bitor,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_bitor(numeric, numeric)
RETURNS uint8
AS 'select $1::float8 | $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.numeric_bitor,
	leftarg = numeric,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : ^
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_bitxor(numeric, int8)
RETURNS uint8
AS 'select $1::int8 ^ $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.numeric_bitxor,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_bitxor(int8, numeric)
RETURNS uint8
AS 'select $1 ^ $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.numeric_bitxor,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_bitxor(numeric, int4)
RETURNS uint8
AS 'select $1::int8 ^ $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.numeric_bitxor,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_bitxor(int4, numeric)
RETURNS uint8
AS 'select $1 ^ $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.numeric_bitxor,
	leftarg = int4,
	rightarg = numeric
);

------------------------------------------------------------------
-- mysql numeric : >>
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_rightmov(int4, numeric)
RETURNS uint8
AS 'select $1::numeric >> $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.numeric_rightmov,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_rightmov(numeric, int4)
RETURNS uint8
AS 'select $1 >> $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.numeric_rightmov,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_rightmov(int8, numeric)
RETURNS uint8
AS 'select $1::numeric >> $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.numeric_rightmov,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_rightmov(numeric, int8)
RETURNS uint8
AS 'select $1 >> $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.numeric_rightmov,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_rightmov(numeric, numeric)
RETURNS uint8
AS 'select $1::float8 >> $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.numeric_rightmov,
	leftarg = numeric,
	rightarg = numeric
);
------------------------------------------------------------------
-- mysql numeric : <<
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_leftmov(int4, numeric)
RETURNS uint8
AS 'select $1::numeric << $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.numeric_leftmov,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_leftmov(numeric, int4)
RETURNS uint8
AS 'select $1 << $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.numeric_leftmov,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_leftmov(int8, numeric)
RETURNS uint8
AS 'select $1::numeric << $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.numeric_leftmov,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_leftmov(numeric, int8)
RETURNS uint8
AS 'select $1 << $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.numeric_leftmov,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_leftmov(numeric, numeric)
RETURNS uint8
AS 'select $1::float8 << $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.numeric_leftmov,
	leftarg = numeric,
	rightarg = numeric
);
------------------------------------------------------------------
-- mysql numeric : and
------------------------------------------------------------------
CREATE FUNCTION ivymy.numeric_and(int4, numeric)
RETURNS uint8
AS 'select $1::numeric && $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.numeric_and,
	leftarg = int4,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_and(numeric, int4)
RETURNS uint8
AS 'select $1 && $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.numeric_and,
	leftarg = numeric,
	rightarg = int4
);

CREATE FUNCTION ivymy.numeric_and(int8, numeric)
RETURNS uint8
AS 'select $1::numeric && $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.numeric_and,
	leftarg = int8,
	rightarg = numeric
);

CREATE FUNCTION ivymy.numeric_and(numeric, int8)
RETURNS uint8
AS 'select $1 && $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.numeric_and,
	leftarg = numeric,
	rightarg = int8
);

CREATE FUNCTION ivymy.numeric_and(numeric, numeric)
RETURNS uint8
AS 'select $1::float8 && $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.numeric_and,
	leftarg = numeric,
	rightarg = numeric
);
/* Begin - bug#z6342 */
CREATE FUNCTION ivymy.numeric_add_tinyint(numeric, ivymy.tinyint)
RETURNS numeric
AS 'select $1 + $2::numeric'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.numeric_add_tinyint,
	leftarg = numeric,
	rightarg = ivymy.tinyint
);

CREATE FUNCTION ivymy.numeric_add_smallint(numeric, smallint)
RETURNS numeric
AS 'select $1 + $2::int'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.numeric_add_smallint,
	leftarg = numeric,
	rightarg = smallint
);

CREATE FUNCTION ivymy.smallint_add_numeric(smallint, numeric)
RETURNS numeric
AS 'select $1::int + $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.smallint_add_numeric,
	leftarg = smallint,
	rightarg = numeric
);
/* End - bug#z6342 */

CREATE FUNCTION ivymy.number_stddev_pop(internal)
RETURNS numeric
AS 'numeric_stddev_pop'
LANGUAGE internal
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.number_accum(internal, numeric)
RETURNS internal
AS 'numeric_accum'
LANGUAGE internal
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.number_accum_inv(internal, numeric)
RETURNS internal
AS 'numeric_accum_inv'
LANGUAGE internal
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.std(numeric) 
(
	SFUNC = number_accum,
	FINALFUNC = number_stddev_pop,
	MSFUNC = number_accum,
	MINVFUNC = number_accum_inv,
	MFINALFUNC = number_stddev_pop,
	STYPE = internal,
	SSPACE = 128,
	MSTYPE = internal,
	MSSPACE = 128,
	PARALLEL = SAFE
);