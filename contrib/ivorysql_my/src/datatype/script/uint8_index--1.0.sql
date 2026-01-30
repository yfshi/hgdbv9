/***************************************************************
 *
 * MySQL uint type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint_eq(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','uint81_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint82_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint84_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint8int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint8, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/**************************************** UINT8 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'uint8_ops' for type 'uint8'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  ivymy.tinyint)  RETURNS integer AS 'MODULE_PATHNAME','uint8int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','uint8int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(pg_catalog.int2, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int2uint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','uint8int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(pg_catalog.int4, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int4uint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','uint8int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(pg_catalog.int8, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int8uint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','uint81_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','uint82_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','uint84_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint8_cmp(ivymy.uint8,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','uint88_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.uint8_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'uint8_ops' */
/*CREATE OPERATOR FAMILY ivymy.uint8_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'uint8' */
CREATE OPERATOR CLASS ivymy.uint8_ops
	DEFAULT FOR TYPE ivymy.uint8 USING btree FAMILY integer_ops AS
		OPERATOR		1		< (ivymy.uint8, ivymy.uint8),
		OPERATOR		2		<= (ivymy.uint8, ivymy.uint8),
		OPERATOR		3		= (ivymy.uint8, ivymy.uint8),
		OPERATOR		4		>= (ivymy.uint8, ivymy.uint8),
		OPERATOR		5		> (ivymy.uint8, ivymy.uint8),
		FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, ivymy.uint8),
		FUNCTION		2		ivymy.uint8_sortsupport(internal);

ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int4),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint8),
	FUNCTION		1		ivymy.uint8_cmp(pg_catalog.int4, ivymy.uint8),

	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int2),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint8),
	FUNCTION		1		ivymy.uint8_cmp(pg_catalog.int2, ivymy.uint8),

	
	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int8),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, pg_catalog.int8),

	OPERATOR		1		<  (pg_catalog.int8, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int8, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int8, ivymy.uint8),
	FUNCTION		1		ivymy.uint8_cmp(pg_catalog.int8, ivymy.uint8),
	
	OPERATOR		1		<  (ivymy.uint8, ivymy.uint1),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint1),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint1),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint1),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint1),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint8, ivymy.uint2),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint2),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint2),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint2),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint2),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint8, ivymy.uint4),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint4),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint4),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint4),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint4),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, ivymy.uint4),

	OPERATOR		1		<  (ivymy.uint8, ivymy.tinyint),
	OPERATOR		2		<= (ivymy.uint8, ivymy.tinyint),
	OPERATOR		3		=  (ivymy.uint8, ivymy.tinyint),
	OPERATOR		4		>= (ivymy.uint8, ivymy.tinyint),
	OPERATOR		5		>  (ivymy.uint8, ivymy.tinyint),
	FUNCTION		1		ivymy.uint8_cmp(ivymy.uint8, ivymy.tinyint);

/* HASH index support */
CREATE FUNCTION ivymy.uint8_hash(ivymy.uint8)
RETURNS integer
AS 'MODULE_PATHNAME','uint8_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint8_hash_extended(ivymy.uint8, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','uint8_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.uint8_ops
	DEFAULT FOR TYPE ivymy.uint8 USING hash AS
		OPERATOR		1		= (ivymy.uint8, ivymy.uint8),
		FUNCTION		1		ivymy.uint8_hash(ivymy.uint8),
		FUNCTION		2		ivymy.uint8_hash_extended(ivymy.uint8, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

