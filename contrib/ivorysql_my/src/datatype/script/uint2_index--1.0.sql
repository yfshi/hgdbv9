/***************************************************************
 *
 * MySQL uint2 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint_eq(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint2, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint21_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint2, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint24_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint2, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint28_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint2, rightarg = ivymy.uint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint2int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint2, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/**************************************** UINT2 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'uint2_ops' for type 'uint2'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  ivymy.tinyint)  RETURNS integer AS 'MODULE_PATHNAME','uint2int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','uint2int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(pg_catalog.int2, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int2uint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','uint2int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(pg_catalog.int4, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int4uint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','uint2int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(pg_catalog.int8, ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','int8uint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','uint21_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','uint22_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','uint24_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint2_cmp(ivymy.uint2,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','uint28_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.uint2_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'uint2_ops' */
/*CREATE OPERATOR FAMILY ivymy.uint2_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'uint2' */
CREATE OPERATOR CLASS ivymy.uint2_ops
	DEFAULT FOR TYPE ivymy.uint2 USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.uint2, ivymy.uint2),
		OPERATOR		2		<= (ivymy.uint2, ivymy.uint2),
		OPERATOR		3		=  (ivymy.uint2, ivymy.uint2),
		OPERATOR		4		>= (ivymy.uint2, ivymy.uint2),
		OPERATOR		5		>  (ivymy.uint2, ivymy.uint2),
		FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, ivymy.uint2),
		FUNCTION		2		ivymy.uint2_sortsupport(internal);
ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int4),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint2),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint2),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint2),
	FUNCTION		1		ivymy.uint2_cmp(pg_catalog.int4, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int2),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint2),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint2),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint2),
	FUNCTION		1		ivymy.uint2_cmp(pg_catalog.int2, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint2, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint2, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint2, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint2, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint2, pg_catalog.int8),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, pg_catalog.int8),

	OPERATOR		1		< (pg_catalog.int8, ivymy.uint2),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		3		= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint2),
	OPERATOR		5		> (pg_catalog.int8, ivymy.uint2),
	FUNCTION		1		ivymy.uint2_cmp(pg_catalog.int8, ivymy.uint2),


	OPERATOR		1		<  (ivymy.uint2, ivymy.uint1),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint1),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint1),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint1),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint1),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint2, ivymy.uint4),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint4),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint4),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint4),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint4),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, ivymy.uint4),

	OPERATOR		1		<  (ivymy.uint2, ivymy.uint8),
	OPERATOR		2		<= (ivymy.uint2, ivymy.uint8),
	OPERATOR		3		=  (ivymy.uint2, ivymy.uint8),
	OPERATOR		4		>= (ivymy.uint2, ivymy.uint8),
	OPERATOR		5		>  (ivymy.uint2, ivymy.uint8),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, ivymy.uint8),

	OPERATOR		1		<  (ivymy.uint2, ivymy.tinyint),
	OPERATOR		2		<= (ivymy.uint2, ivymy.tinyint),
	OPERATOR		3		=  (ivymy.uint2, ivymy.tinyint),
	OPERATOR		4		>= (ivymy.uint2, ivymy.tinyint),
	OPERATOR		5		>  (ivymy.uint2, ivymy.tinyint),
	FUNCTION		1		ivymy.uint2_cmp(ivymy.uint2, ivymy.tinyint);

/* HASH index support */
CREATE FUNCTION ivymy.uint2_hash(ivymy.uint2)
RETURNS integer
AS 'MODULE_PATHNAME','uint2_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint2_hash_extended(ivymy.uint2, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','uint2_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.uint2_ops
	DEFAULT FOR TYPE ivymy.uint2 USING hash AS
		OPERATOR		1		= (ivymy.uint2, ivymy.uint2),
		FUNCTION		1		ivymy.uint2_hash(ivymy.uint2),
		FUNCTION		2		ivymy.uint2_hash_extended(ivymy.uint2, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;
