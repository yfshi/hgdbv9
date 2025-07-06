/***************************************************************
 *
 * MySQL uint2 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint21_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint24_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint28_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/**************************************** UINT2 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'mysuint2_ops' for type 'uint2'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','mysuint2int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(pg_catalog.int2, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int2mysuint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','mysuint2int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(pg_catalog.int4, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int4mysuint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','mysuint2int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(pg_catalog.int8, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int8mysuint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','mysuint21_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','mysuint22_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','mysuint24_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint2_cmp(ivymy.uint2,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','mysuint28_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.mysuint2_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint2_ops' */
/*CREATE OPERATOR FAMILY ivymy.mysuint2_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'oramysuint2' */
CREATE OPERATOR CLASS ivymy.mysuint2_ops
	DEFAULT FOR TYPE ivymy.uint2 USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.uint2, ivymy.uint2),
		OPERATOR		2		<= (ivymy.uint2, ivymy.uint2),
		OPERATOR		3		=  (ivymy.uint2, ivymy.uint2),
		OPERATOR		4		>= (ivymy.uint2, ivymy.uint2),
		OPERATOR		5		>  (ivymy.uint2, ivymy.uint2),
		FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, ivymy.uint2),
		FUNCTION		2		ivymy.mysuint2_sortsupport(internal);
ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int4),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint2),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint2),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint2),
	FUNCTION		1		ivymy.mysuint2_cmp(pg_catalog.int4, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int2),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint2),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint2),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint2),
	FUNCTION		1		ivymy.mysuint2_cmp(pg_catalog.int2, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int8),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, pg_catalog.int8),

	OPERATOR		1		< (pg_catalog.int8, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		3		= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		5		> (pg_catalog.int8, ivymy.uint2),
	FUNCTION		1		ivymy.mysuint2_cmp(pg_catalog.int8, ivymy.uint2),


	OPERATOR		1		<  (ivymy.uint2, ivymy.uint1),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint1),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint1),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint1),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint1),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint2, ivymy.uint4),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint4),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint4),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint4),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint4),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, ivymy.uint4),

	OPERATOR		1		<  (ivymy.uint2, ivymy.uint8),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint8),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint8),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint8),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint8),
	FUNCTION		1		ivymy.mysuint2_cmp(ivymy.uint2, ivymy.uint8);

/* HASH index support */
CREATE FUNCTION ivymy.mysuint2_hash(ivymy.uint2)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint2_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint2_hash_extended(ivymy.uint2, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint2_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint2_ops
	DEFAULT FOR TYPE ivymy.uint2 USING hash AS
		OPERATOR		1		= (ivymy.uint2, ivymy.uint2),
		FUNCTION		1		ivymy.mysuint2_hash(ivymy.uint2),
		FUNCTION		2		ivymy.mysuint2_hash_extended(ivymy.uint2, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;
