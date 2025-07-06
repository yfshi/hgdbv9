/***************************************************************
 *
 * MySQL mysuint type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint4, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','mysuint41_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint4, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint4, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint42_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint4, rightarg = ivymy.uint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.uint4, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint48_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.uint4, rightarg = ivymy.uint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);


/**************************************** UINT4 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'mysuint4_ops' for type 'uint4'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','mysuint4int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(pg_catalog.int2, ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','int2mysuint4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','mysuint4int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(pg_catalog.int4, ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','int4mysuint4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','mysuint4int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(pg_catalog.int8, ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','int8mysuint4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','mysuint41_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','mysuint42_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','mysuint44_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint4_cmp(ivymy.uint4,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','mysuint48_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.mysuint4_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint4_ops' 
CREATE OPERATOR FAMILY ivymy.mysuint4_ops_btree USING btree;*/

/* CREATE OPERATOR CLASS for 'oramysuint4' */
CREATE OPERATOR CLASS ivymy.mysuint4_class
	default FOR TYPE ivymy.uint4 USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.uint4, ivymy.uint4),
		OPERATOR		2		<= (ivymy.uint4, ivymy.uint4),
		OPERATOR		3		=  (ivymy.uint4, ivymy.uint4),
		OPERATOR		4		>= (ivymy.uint4, ivymy.uint4),
		OPERATOR		5		>  (ivymy.uint4, ivymy.uint4),
		FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, ivymy.uint4),
		FUNCTION		2		(ivymy.uint4, ivymy.uint4) ivymy.mysuint4_sortsupport(internal);

ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint4, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint4, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint4, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint4, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint4, pg_catalog.int4),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint4),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint4),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint4),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint4),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint4),
	FUNCTION		1		ivymy.mysuint4_cmp(pg_catalog.int4, ivymy.uint4),

	OPERATOR		1		<  (ivymy.uint4, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint4, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint4, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint4, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint4, pg_catalog.int2),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint4),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint4),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint4),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint4),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint4),
	FUNCTION		1		ivymy.mysuint4_cmp(pg_catalog.int2, ivymy.uint4),

	
	OPERATOR		1		<  (ivymy.uint4, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint4, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint4, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint4, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint4, pg_catalog.int8),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, pg_catalog.int8),

	OPERATOR		1		<  (pg_catalog.int8, ivymy.uint4),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint4),
	OPERATOR		3		=  (pg_catalog.int8, ivymy.uint4),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint4),
	OPERATOR		5		>  (pg_catalog.int8, ivymy.uint4),
	FUNCTION		1		ivymy.mysuint4_cmp(pg_catalog.int8, ivymy.uint4),


	OPERATOR		1		<  (ivymy.uint4, ivymy.uint1),
	OPERATOR		2		<= (ivymy.uint4, ivymy.uint1),
	OPERATOR		3		=  (ivymy.uint4, ivymy.uint1),
	OPERATOR		4		>= (ivymy.uint4, ivymy.uint1),
	OPERATOR		5		>  (ivymy.uint4, ivymy.uint1),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint4, ivymy.uint2),
	OPERATOR		2		<= (ivymy.uint4, ivymy.uint2),
	OPERATOR		3		=  (ivymy.uint4, ivymy.uint2),
	OPERATOR		4		>= (ivymy.uint4, ivymy.uint2),
	OPERATOR		5		>  (ivymy.uint4, ivymy.uint2),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint4, ivymy.uint8),
	OPERATOR		2		<= (ivymy.uint4, ivymy.uint8),
	OPERATOR		3		=  (ivymy.uint4, ivymy.uint8),
	OPERATOR		4		>= (ivymy.uint4, ivymy.uint8),
	OPERATOR		5		>  (ivymy.uint4, ivymy.uint8),
	FUNCTION		1		ivymy.mysuint4_cmp(ivymy.uint4, ivymy.uint8);


/* HASH index support */
CREATE FUNCTION ivymy.mysuint4_hash(ivymy.uint4)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint4_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint4_hash_extended(ivymy.uint4, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint4_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint4_ops_hash
	DEFAULT FOR TYPE ivymy.uint4 USING hash AS
		OPERATOR		1		= (ivymy.uint4, ivymy.uint4),
		FUNCTION		1		ivymy.mysuint4_hash(ivymy.uint4),
		FUNCTION		2		ivymy.mysuint4_hash_extended(ivymy.uint4, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;
