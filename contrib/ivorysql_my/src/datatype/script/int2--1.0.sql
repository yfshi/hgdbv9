/***************************************************************
 *
 * ReqID:SRS-MYSQL-NUMBERICTYPE-INT2
 * int2 type support
 *
 ***************************************************************/

-- CREATE OPERATOR

--  "&"
CREATE FUNCTION ivymy.myint2_bit_and(pg_catalog.int2, pg_catalog.int2)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint2_bit_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.& (
	procedure = ivymy.myint2_bit_and,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = &
);

--  "|"
CREATE FUNCTION ivymy.myint2_bit_or(pg_catalog.int2, pg_catalog.int2)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint2_bit_or'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.| (
	procedure = ivymy.myint2_bit_or,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = |
);

--  "+"
CREATE FUNCTION ivymy.myint2_pl(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_pl'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint2_pl,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = +
);

--  "-"
CREATE FUNCTION ivymy.myint2_mi(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_mi'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint2_mi,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = -
);

--  "*"
CREATE FUNCTION ivymy.myint2_mul(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_mul'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.* (
	procedure = ivymy.myint2_mul,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = *
);


--  "/"
CREATE FUNCTION ivymy.myint2_div(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','myint2_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.myint2_div,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = /
);

/* Begin - bug#5166 */
CREATE FUNCTION ivymy.myint2_div(pg_catalog.int2, int4)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','myint2_div_int4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.myint2_div,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int4,
	commutator = /
);
/* End - bug#5166 */

--  "//"
CREATE FUNCTION ivymy.myint2_exact_div(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_exact_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.// (
	procedure = ivymy.myint2_exact_div,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = //
);

--  "%"
CREATE FUNCTION ivymy.myint2_mod(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_mod'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.myint2_mod,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = %
);

--  "^"
CREATE FUNCTION ivymy.myint2_bit_xor(pg_catalog.int2, pg_catalog.int2)
RETURNS ivymy.uint8	
AS 'MODULE_PATHNAME','myint2_bit_xor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.^ (
	procedure = ivymy.myint2_bit_xor,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = ^
);

--  "||"
CREATE FUNCTION ivymy.int2cat(int2, int2)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.|| (
	procedure = ivymy.int2cat,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = ||
);

--  "&&"
CREATE FUNCTION ivymy.myint2_and(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_and'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.&& (
	procedure = ivymy.myint2_and,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2,
	commutator = &&
);

--  "<=>"
CREATE FUNCTION ivymy.myint2_safeeq(pg_catalog.int2, pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_safeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.myint2_safeeq,
	leftarg = pg_catalog.int2,
	rightarg = pg_catalog.int2
);

--  "!"
CREATE FUNCTION ivymy.myint2_not(pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_not'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR !(
	procedure = ivymy.myint2_not,
	rightarg = pg_catalog.int2
);

--  "~"
CREATE FUNCTION ivymy.myint2_bit_neg(pg_catalog.int2)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','myint2_bit_neg'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.~ (
	procedure = ivymy.myint2_bit_neg,
	rightarg = pg_catalog.int2
);

--"sign +"
CREATE FUNCTION ivymy.myint2_positive(pg_catalog.int2)
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','myint2_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint2_positive,
	rightarg = pg_catalog.int2
);

--"sign -"
CREATE FUNCTION ivymy.myint2_negative(pg_catalog.int2)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint2_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint2_negative,
	rightarg = pg_catalog.int2
);

-- CREATE CAST

-- to bpchar
CREATE CAST (pg_catalog.int2 AS pg_catalog.bpchar) 
WITH INOUT
AS ASSIGNMENT;

-- to varchar
CREATE CAST (pg_catalog.int2 AS pg_catalog.varchar) 
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.boolean_myint2(pg_catalog.bool) 
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','boolean_myint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS pg_catalog.int2)
WITH FUNCTION ivymy.boolean_myint2(pg_catalog.bool)
AS ASSIGNMENT;

-- operator xor or and
CREATE FUNCTION ivymy.myint2_boolean(pg_catalog.int2) 
RETURNS pg_catalog.bool 
AS 'MODULE_PATHNAME','myint2_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS pg_catalog.bool)
WITH FUNCTION ivymy.myint2_boolean(pg_catalog.int2)
AS ASSIGNMENT;