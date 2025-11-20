/***************************************************************
 *
 * type float and double operator support
 *
 ***************************************************************/
-- ~
CREATE FUNCTION ivymy.mysfloat8_not(float8)
RETURNS ivymy.uint8 /* BUG#Z5183 */
AS 'MODULE_PATHNAME','mysfloat8_not'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.~ (
	procedure = ivymy.mysfloat8_not,
	rightarg = float8
);

-- -
CREATE FUNCTION ivymy.mysfloat4um(float4)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat4um'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.mysfloat4um,
	rightarg = float4
	);

-- !
CREATE FUNCTION ivymy.mysfloat8_logic_not(float8)
RETURNS bool
AS 'MODULE_PATHNAME','mysfloat8_logic_not'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.!(
	procedure = ivymy.mysfloat8_logic_not,
	rightarg = float8
);

-- <=>
CREATE FUNCTION ivymy.mysfloat4_safeeq(float4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME','mysfloat4safeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.mysfloat4_safeeq,
	leftarg = float4,
	rightarg = float4,
	restrict = eqsel,
	join = eqjoinsel
);

CREATE FUNCTION ivymy.mysfloat8_safeeq(float8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysfloat8safeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.mysfloat8_safeeq,
	leftarg = float8,
	rightarg = float8,
	restrict = eqsel,
	join = eqjoinsel
);

-- &
CREATE FUNCTION ivymy.mysfloat8_and(float8, float8)
RETURNS ivymy.uint8 /* BUG#Z5355 */
AS 'MODULE_PATHNAME','mysfloat8_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.& (
	procedure = ivymy.mysfloat8_and,
	leftarg = float8,
	rightarg = float8
);

-- <<
CREATE FUNCTION ivymy.mysfloat8_shl(float8, float8)
RETURNS  ivymy.uint8 /* BUG#Z5355 */
AS 'MODULE_PATHNAME','mysfloat8_shl'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.<< (
	procedure = ivymy.mysfloat8_shl,
	leftarg = float8,
	rightarg = float8
);

-- >>
CREATE FUNCTION ivymy.mysfloat8_shr(float8, float8)
RETURNS ivymy.uint8 /* BUG#Z5355 */
AS 'MODULE_PATHNAME','mysfloat8_shr'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.>> (
	procedure = ivymy.mysfloat8_shr,
	leftarg = float8,
	rightarg = float8
);

-- |
CREATE FUNCTION ivymy.mysfloat8_or(float8, float8)
RETURNS ivymy.uint8 /* BUG#Z5355 */
AS 'MODULE_PATHNAME','mysfloat8_or'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.| (
	procedure = ivymy.mysfloat8_or,
	leftarg = float8,
	rightarg = float8
);

-- ^
CREATE FUNCTION ivymy.mysfloat8_xor(float8, float8)
RETURNS ivymy.uint8 /* BUG#Z5355 */
AS 'MODULE_PATHNAME','mysfloat8_xor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.^ (
	procedure = ivymy.mysfloat8_xor,
	leftarg = float8,
	rightarg = float8
);

-- *
CREATE FUNCTION ivymy.mysfloat4_mul(float4, float4)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat4_mul'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.* (
	procedure = ivymy.mysfloat4_mul,
	leftarg = float4,
	rightarg = float4
);

-- +
CREATE FUNCTION ivymy.mysfloat4_pl(float4, float4)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat4_pl'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.mysfloat4_pl,
	leftarg = float4,
	rightarg = float4
);

-- -
CREATE FUNCTION ivymy.mysfloat4_mi(float4, float4)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat4_mi'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.mysfloat4_mi,
	leftarg = float4,
	rightarg = float4
);

-- &&
CREATE FUNCTION ivymy.mysfloat8_logic_and(float8,float8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysfloat8_logic_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.&& (
	procedure = ivymy.mysfloat8_logic_and,
	leftarg = float8,
	rightarg = float8
);

-- /
CREATE FUNCTION ivymy.mysfloat48_div(float4, float8)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat48_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.mysfloat48_div,
	leftarg = float4,
	rightarg = float8
);

CREATE FUNCTION ivymy.mysfloat8_div(float8, float8)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat8_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.mysfloat8_div,
	leftarg = float8,
	rightarg = float8
);

CREATE FUNCTION ivymy.mysfloat84_div(float8, float4)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat84_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.mysfloat84_div,
	leftarg = float8,
	rightarg = float4
);

/* Begin - BUG#Z5351 */
CREATE FUNCTION ivymy.mysfloat4_div(float4, float4)
RETURNS float8
AS $$
	select ivymy.mysfloat84_div($1::float8, $2)
$$ LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.mysfloat4_div,
	leftarg = float4,
	rightarg = float4
);
/* End - BUG#Z5351 */

-- //
CREATE FUNCTION ivymy.mysfloat4_moddiv(float4, float4)
RETURNS int8
AS 'MODULE_PATHNAME','mysfloat4_moddiv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysfloat8_moddiv(float8, float8)
RETURNS int8
AS 'MODULE_PATHNAME','mysfloat8_moddiv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(float4, float4)
RETURNS int8
AS 'MODULE_PATHNAME','mysfloat4_moddiv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mys_div(float8, float8)
RETURNS int8
AS 'MODULE_PATHNAME','mysfloat8_moddiv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ivymy.// (
	procedure = ivymy.mysfloat4_moddiv,
	leftarg = float4,
	rightarg = float4
);

CREATE OPERATOR ivymy.// (
	procedure = ivymy.mysfloat8_moddiv,
	leftarg = float8,
	rightarg = float8
);

-- %
CREATE FUNCTION ivymy.mysfloat8_mod(float8, float8)
RETURNS float8
AS 'MODULE_PATHNAME','mysfloat8_mod'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.mysfloat8_mod,
	leftarg = float8,
	rightarg = float8
);

-- XOR
CREATE FUNCTION ivymy.mysfloat4_boolean(float4)
RETURNS boolean 
AS 'MODULE_PATHNAME','mysfloat4_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysfloat8_boolean(float8)
RETURNS boolean 
AS 'MODULE_PATHNAME','mysfloat8_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (float4 AS boolean)
WITH FUNCTION ivymy.mysfloat4_boolean(float4)
AS ASSIGNMENT;

CREATE CAST (float8 AS boolean)
WITH FUNCTION ivymy.mysfloat8_boolean(float8)
AS ASSIGNMENT;

/* Begin - BUG#Z5204 */
--max
CREATE FUNCTION ivymy.mysfloat4larger(float8, float4)
RETURNS float8
AS 'MODULE_PATHNAME', 'mysfloat4larger'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.max(float4) (
	SFUNC = ivymy.mysfloat4larger,
	STYPE = float8,
	SORTOP = >,
	PARALLEL = SAFE
);

--sum
CREATE FUNCTION ivymy.mysfloat84_pl(float8, float4)
RETURNS float8
AS 'MODULE_PATHNAME', 'mysfloat84_pl'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(float4) (
	SFUNC = ivymy.mysfloat84_pl,
	STYPE = float8
);
/* End - BUG#Z5204 */

/* Begin - bug#z5590 */
CREATE FUNCTION ivymy.text_float4(text, integer, boolean, int8)
RETURNS float4 
AS 'MODULE_PATHNAME','varlena_float4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (text as float4)
WITH FUNCTION ivymy.text_float4(text, integer, boolean, int8)
AS ASSIGNMENT;
/* End - bug#z5590 */

/* Begin - bug#z6040 */
CREATE FUNCTION ivymy.float8_add(float8, uint8)
RETURNS float8
AS 'select $1::float8 + $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.float8_add,
	leftarg = float8,
	rightarg = uint8
);

CREATE FUNCTION ivymy.float8_add(uint8, float8)
RETURNS float8
AS 'select $1::float8 + $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.float8_add,
	leftarg = uint8,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_del(float8, uint8)
RETURNS float8
AS 'select $1::float8 - $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.float8_del,
	leftarg = float8,
	rightarg = uint8
);

CREATE FUNCTION ivymy.float8_del(uint8, float8)
RETURNS float8
AS 'select $1::float8 - $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.float8_del,
	leftarg = uint8,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_div(uint8, float8)
RETURNS float8
AS 'select $1::float8 / $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.float8_div,
	leftarg = uint8,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_div(float8, uint8)
RETURNS float8
AS 'select $1::float8 / $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy./ (
	procedure = ivymy.float8_div,
	leftarg = float8,
	rightarg = uint8
);

CREATE FUNCTION ivymy.float8_mod(uint8, float8)
RETURNS float8
AS 'select $1::float8 % $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.float8_mod,
	leftarg = uint8,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_mod(float8, uint8)
RETURNS float8
AS 'select $1::float8 % $2::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.% (
	procedure = ivymy.float8_mod,
	leftarg = float8,
	rightarg = uint8
);

CREATE FUNCTION ivymy.float8_logic_and(uint8, float8)
RETURNS int
AS 'select ($1::float8 && $2::float8)::int'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.&& (
	procedure = ivymy.float8_logic_and,
	leftarg = uint8,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_logic_and(float8, uint8)
RETURNS int
AS 'select ($1::float8 && $2::float8)::int'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.&& (
	procedure = ivymy.float8_logic_and,
	leftarg = float8,
	rightarg = uint8
);
/* End - bug#z6040 */