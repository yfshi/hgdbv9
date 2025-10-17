/***************************************************************
 *
 * int4 type support
 *
 ***************************************************************/

-- CREATE OPERATOR

--  "&"
CREATE FUNCTION ivymy.myint4_bit_and(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_bit_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.& (
	procedure = ivymy.myint4_bit_and,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = &
);

CREATE FUNCTION ivymy.int4_and(int2, int4)
RETURNS ivymy.uint8
AS 'select $1::int & $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.& (
	procedure = ivymy.int4_and,
	leftarg = int2,
	rightarg = int4
);

--  "|"
CREATE FUNCTION ivymy.myint4_bit_or(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_bit_or'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.| (
	procedure = ivymy.myint4_bit_or,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = |
);

--  "+"
CREATE FUNCTION ivymy.myint4_pl(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','myint4_pl'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint4_pl,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = +
);

--  "-"
CREATE FUNCTION ivymy.myint4_mi(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','myint4_mi'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint4_mi,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = -
);

--  "*"
CREATE FUNCTION ivymy.myint4_mul(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','myint4_mul'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.* (
	procedure = ivymy.myint4_mul,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = *
);


--  "/"
CREATE FUNCTION ivymy.myint4_div(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','myint4_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.myint4_div,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = /
);

CREATE FUNCTION ivymy.myint42_div(pg_catalog.int4, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','myint4_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.myint42_div,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int2,
	commutator = /
);

--  "//"
CREATE FUNCTION ivymy.myint4_exact_div(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_exact_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.// (
	procedure = ivymy.myint4_exact_div,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = //
);

--  "%"
CREATE FUNCTION ivymy.myint4_mod(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_mod'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.myint4_mod,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = %
);

CREATE FUNCTION ivymy.myint4_mod(int2, int4)
RETURNS pg_catalog.int4
AS 'select $1::int4 % $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.myint4_mod,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int4,
	commutator = %
);

CREATE FUNCTION ivymy.myint4_mod(int4, int2)
RETURNS pg_catalog.int4
AS 'select $1 % $2::int4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.myint4_mod,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int2,
	commutator = %
);


--  "^"
CREATE FUNCTION ivymy.myint4_bit_xor(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_bit_xor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.^ (
	procedure = ivymy.myint4_bit_xor,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = ^
);

--  "&&"
CREATE FUNCTION ivymy.myint4_and(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_and'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.&& (
	procedure = ivymy.myint4_and,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = &&
);

--  "<<"
CREATE FUNCTION ivymy.myint4_left_shift(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_left_shift'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.<< (
	procedure = ivymy.myint4_left_shift,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = <<
);

--  ">>"
CREATE FUNCTION ivymy.myint4_right_shift(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_right_shift'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.>> (
	procedure = ivymy.myint4_right_shift,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4,
	commutator = >>
);
--  "<=>"
CREATE FUNCTION ivymy.myint4_safeeq(pg_catalog.int4, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_safeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.myint4_safeeq,
	leftarg = pg_catalog.int4,
	rightarg = pg_catalog.int4
	-- restrict = eqsel,
	-- join = eqjoinsel
);

--  "!"
CREATE FUNCTION ivymy.myint4_not(pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_not'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR !(
	procedure = ivymy.myint4_not,
	rightarg = pg_catalog.int4
);

--  "~"
CREATE FUNCTION ivymy.myint4_bit_neg(pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint4_bit_neg'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.~ (
	procedure = ivymy.myint4_bit_neg,
	rightarg = pg_catalog.int4
);

--"sign +"
CREATE FUNCTION ivymy.myint4_positive(pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint4_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint4_positive,
	rightarg = pg_catalog.int4
);

--"sign -"
CREATE FUNCTION ivymy.myint4_negative(pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','myint4_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint4_negative,
	rightarg = pg_catalog.int4
);

-- to bpchar
CREATE CAST (pg_catalog.int4 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (pg_catalog.int4 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

