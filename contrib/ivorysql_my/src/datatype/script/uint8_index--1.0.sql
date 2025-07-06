/***************************************************************
 *
 * MySQL mysuint type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint8, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint81_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint8, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint82_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint8, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint84_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint8, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/**************************************** UINT8 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'mysuint8_ops' for type 'uint8'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','mysuint8int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(pg_catalog.int2, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int2mysuint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','mysuint8int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(pg_catalog.int4, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int4mysuint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','mysuint8int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(pg_catalog.int8, ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','int8mysuint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','mysuint81_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','mysuint82_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','mysuint84_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint8_cmp(ivymy.uint8,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','mysuint88_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.mysuint8_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint8_ops' */
/*CREATE OPERATOR FAMILY ivymy.mysuint8_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'oramysuint8' */
CREATE OPERATOR CLASS ivymy.mysuint8_ops
	DEFAULT FOR TYPE ivymy.uint8 USING btree FAMILY integer_ops AS
		OPERATOR		1		< (ivymy.uint8, ivymy.uint8),
		OPERATOR		2		<= (ivymy.uint8, ivymy.uint8),
		OPERATOR		3		= (ivymy.uint8, ivymy.uint8),
		OPERATOR		4		>= (ivymy.uint8, ivymy.uint8),
		OPERATOR		5		> (ivymy.uint8, ivymy.uint8),
		FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, ivymy.uint8),
		FUNCTION		2		ivymy.mysuint8_sortsupport(internal);

ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int4),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint8),
	FUNCTION		1		ivymy.mysuint8_cmp(pg_catalog.int4, ivymy.uint8),

	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int2),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint8),
	FUNCTION		1		ivymy.mysuint8_cmp(pg_catalog.int2, ivymy.uint8),

	
	OPERATOR		1		<  (ivymy.uint8, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint8, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint8, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint8, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint8, pg_catalog.int8),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, pg_catalog.int8),

	OPERATOR		1		<  (pg_catalog.int8, ivymy.uint8),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint8),
	OPERATOR		3		=  (pg_catalog.int8, ivymy.uint8),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint8),
	OPERATOR		5		>  (pg_catalog.int8, ivymy.uint8),
	FUNCTION		1		ivymy.mysuint8_cmp(pg_catalog.int8, ivymy.uint8),
	
	OPERATOR		1		<  (ivymy.uint8, ivymy.uint1),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint1),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint1),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint1),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint1),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint8, ivymy.uint2),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint2),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint2),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint2),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint2),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint8, ivymy.uint4),
	OPERATOR		2		<= (ivymy.uint8, ivymy.uint4),
	OPERATOR		3		=  (ivymy.uint8, ivymy.uint4),
	OPERATOR		4		>= (ivymy.uint8, ivymy.uint4),
	OPERATOR		5		>  (ivymy.uint8, ivymy.uint4),
	FUNCTION		1		ivymy.mysuint8_cmp(ivymy.uint8, ivymy.uint4);

/* HASH index support */
CREATE FUNCTION ivymy.mysuint8_hash(ivymy.uint8)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint8_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint8_hash_extended(ivymy.uint8, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint8_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint8_ops
	DEFAULT FOR TYPE ivymy.uint8 USING hash AS
		OPERATOR		1		= (ivymy.uint8, ivymy.uint8),
		FUNCTION		1		ivymy.mysuint8_hash(ivymy.uint8),
		FUNCTION		2		ivymy.mysuint8_hash_extended(ivymy.uint8, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

