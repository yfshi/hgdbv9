/***************************************************************
 *
 * MySQL uint1 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint_eq(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint1, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','uint12_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint1, rightarg = ivymy.uint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint1, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','uint14_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint1, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint1, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','uint18_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint1, rightarg = ivymy.uint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.uint_eq(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ne(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_lt(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_gt(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_le(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint_ge(ivymy.uint1, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','uint1int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.uint_eq, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.uint_ne, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.uint_lt, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.uint_gt, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.uint_le, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.uint_ge, leftarg = ivymy.uint1, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/**************************************** UINT1 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'uint1_ops' for type 'uint1'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  ivymy.tinyint)  RETURNS integer AS 'MODULE_PATHNAME','uint1int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','uint1int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(pg_catalog.int2, ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','int2uint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','uint1int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(pg_catalog.int4, ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','int4uint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','uint1int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(pg_catalog.int8, ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','int8uint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  ivymy.uint1)  RETURNS integer AS 'MODULE_PATHNAME','uint11_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  ivymy.uint2)  RETURNS integer AS 'MODULE_PATHNAME','uint12_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  ivymy.uint4)  RETURNS integer AS 'MODULE_PATHNAME','uint14_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.uint1_cmp(ivymy.uint1,  ivymy.uint8)  RETURNS integer AS 'MODULE_PATHNAME','uint18_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.uint1_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'uint1_ops' 
CREATE OPERATOR FAMILY ivymy.uint1_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'uint1' */
CREATE OPERATOR CLASS ivymy.uint1_ops
	DEFAULT FOR TYPE ivymy.uint1 USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.uint1, ivymy.uint1),
		OPERATOR		2		<= (ivymy.uint1, ivymy.uint1),
		OPERATOR		3		=  (ivymy.uint1, ivymy.uint1),
		OPERATOR		4		>= (ivymy.uint1, ivymy.uint1),
		OPERATOR		5		>  (ivymy.uint1, ivymy.uint1),
		FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, ivymy.uint1),
		FUNCTION		2		ivymy.uint1_sortsupport(internal);
ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.uint1, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.uint1, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.uint1, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.uint1, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.uint1, pg_catalog.int4),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.uint1),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.uint1),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.uint1),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.uint1),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.uint1),
	FUNCTION		1		ivymy.uint1_cmp(pg_catalog.int4, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint1, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.uint1, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.uint1, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.uint1, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.uint1, pg_catalog.int2),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.uint1),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.uint1),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.uint1),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.uint1),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.uint1),
	FUNCTION		1		ivymy.uint1_cmp(pg_catalog.int2, ivymy.uint1),

	OPERATOR		1		<  (ivymy.uint1, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.uint1, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.uint1, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.uint1, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.uint1, pg_catalog.int8),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, pg_catalog.int8),

	OPERATOR		1		< (pg_catalog.int8, ivymy.uint1),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.uint1),
	OPERATOR		3		= (pg_catalog.int8, ivymy.uint1),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.uint1),
	OPERATOR		5		> (pg_catalog.int8, ivymy.uint1),
	FUNCTION		1		ivymy.uint1_cmp(pg_catalog.int8, ivymy.uint1),


	OPERATOR		1		<  (ivymy.uint1, ivymy.uint2),
	OPERATOR		2		<= (ivymy.uint1, ivymy.uint2),
	OPERATOR		3		=  (ivymy.uint1, ivymy.uint2),
	OPERATOR		4		>= (ivymy.uint1, ivymy.uint2),
	OPERATOR		5		>  (ivymy.uint1, ivymy.uint2),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, ivymy.uint2),

	OPERATOR		1		<  (ivymy.uint1, ivymy.uint4),
	OPERATOR		2		<= (ivymy.uint1, ivymy.uint4),
	OPERATOR		3		=  (ivymy.uint1, ivymy.uint4),
	OPERATOR		4		>= (ivymy.uint1, ivymy.uint4),
	OPERATOR		5		>  (ivymy.uint1, ivymy.uint4),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, ivymy.uint4),

	OPERATOR		1		<  (ivymy.uint1, ivymy.uint8),
	OPERATOR		2		<= (ivymy.uint1, ivymy.uint8),
	OPERATOR		3		=  (ivymy.uint1, ivymy.uint8),
	OPERATOR		4		>= (ivymy.uint1, ivymy.uint8),
	OPERATOR		5		>  (ivymy.uint1, ivymy.uint8),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, ivymy.uint8),

	OPERATOR		1		<  (ivymy.uint1, ivymy.tinyint),
	OPERATOR		2		<= (ivymy.uint1, ivymy.tinyint),
	OPERATOR		3		=  (ivymy.uint1, ivymy.tinyint),
	OPERATOR		4		>= (ivymy.uint1, ivymy.tinyint),
	OPERATOR		5		>  (ivymy.uint1, ivymy.tinyint),
	FUNCTION		1		ivymy.uint1_cmp(ivymy.uint1, ivymy.tinyint);

/* HASH index support */
CREATE FUNCTION ivymy.uint1_hash(ivymy.uint1)
RETURNS integer
AS 'MODULE_PATHNAME','uint1_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint1_hash_extended(ivymy.uint1, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','uint1_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.uint1_ops
	DEFAULT FOR TYPE ivymy.uint1 USING hash AS
		OPERATOR		1		= (ivymy.uint1, ivymy.uint1),
		FUNCTION		1		ivymy.uint1_hash(ivymy.uint1),
		FUNCTION		2		ivymy.uint1_hash_extended(ivymy.uint1, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;
