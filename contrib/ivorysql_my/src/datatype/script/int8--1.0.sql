CREATE FUNCTION ivymy.int8_seq(int8, int4)
RETURNS bool
AS 'select $1 <=> $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR <=> (
	procedure = ivymy.int8_seq,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_div(int8, int8)
RETURNS numeric
AS 'MODULE_PATHNAME', 'int8_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.int8_div,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_div(int8, int4)
RETURNS numeric
AS 'MODULE_PATHNAME', 'int8_div_int4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.int8_div,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_div(int4, int8)
RETURNS numeric
AS 'select $1::int8 / $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR / (
	procedure = ivymy.int8_div,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_exact_div(int8, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'int8_exact_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.int8_exact_div,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_exact_div(int8, int4)
RETURNS int4
AS 'select $1 div $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.int8_exact_div,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_exact_div(int4, int8)
RETURNS int4
AS 'select $1::int8 div $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR // (
	procedure = ivymy.int8_exact_div,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_mod(int8, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'int8_mod'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.int8_mod,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_mod(int8, int4)
RETURNS int4
AS 'select $1 % $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.int8_mod,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_mod(int8, int2)
RETURNS int4
AS 'select $1 % $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.int8_mod,
	leftarg = int8,
	rightarg = int2
);

CREATE FUNCTION ivymy.int8_mod(int4, int8)
RETURNS int4
AS 'select $1::int8 % $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR % (
	procedure = ivymy.int8_mod,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_bitand(int8, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'int8_bitand'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.int8_bitand,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_bitand(int8, int4)
RETURNS uint8
AS 'select $1 & $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.int8_bitand,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_bitand(int4, int8)
RETURNS uint8
AS 'select $1::int8 & $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR & (
	procedure = ivymy.int8_bitand,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_bitor(int8, int4)
RETURNS uint8
AS 'select $1 | $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.int8_bitor,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_bitor(int4, int8)
RETURNS uint8
AS 'select $1::int8 | $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR | (
	procedure = ivymy.int8_bitor,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_bitxor(int8, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'int8_bitxor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.int8_bitxor,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_bitxor(int8, int4)
RETURNS uint8
AS 'select $1 ^ $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.int8_bitxor,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_bitxor(int4, int8)
RETURNS uint8
AS 'select $1::int8 ^ $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ^ (
	procedure = ivymy.int8_bitxor,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_rightmov(int8, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'int8_rightmov'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.int8_rightmov,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_rightmov(int4, int8)
RETURNS uint8
AS 'select $1::int8 >> $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR >> (
	procedure = ivymy.int8_rightmov,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_leftmov(int8, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'int8_leftmov'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.int8_leftmov,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_leftmov(int8, int4)
RETURNS uint8
AS 'select $1 << $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.int8_leftmov,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_leftmov(int4, int8)
RETURNS uint8
AS 'select $1::int8 << $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR << (
	procedure = ivymy.int8_leftmov,
	leftarg = int4,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_and(int8, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'int8_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.int8_and,
	leftarg = int8,
	rightarg = int8
);

CREATE FUNCTION ivymy.int8_and(int8, int4)
RETURNS uint8
AS 'select $1 && $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.int8_and,
	leftarg = int8,
	rightarg = int4
);

CREATE FUNCTION ivymy.int8_and(int4, int8)
RETURNS uint8
AS 'select $1::int8 && $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR && (
	procedure = ivymy.int8_and,
	leftarg = int4,
	rightarg = int8
);
