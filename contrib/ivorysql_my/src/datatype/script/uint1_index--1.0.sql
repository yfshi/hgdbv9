/***************************************************************
 *
 * MySQL mysuint1 type support
 *
***************************************************************/

CREATE FUNCTION ivymy.mysuint_eq(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.mysuint1, ivymy.mysuint2) RETURNS boolean AS 'MODULE_PATHNAME','mysuint12_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.mysuint1, ivymy.mysuint4) RETURNS boolean AS 'MODULE_PATHNAME','mysuint14_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.mysuint_eq(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ne(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_lt(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_gt(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_le(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint_ge(ivymy.mysuint1, ivymy.mysuint8) RETURNS boolean AS 'MODULE_PATHNAME','mysuint18_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.mysuint_eq, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel,  merges);
CREATE OPERATOR != (procedure = ivymy.mysuint_ne, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.mysuint_lt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.mysuint_gt, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.mysuint_le, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.mysuint_ge, leftarg = ivymy.mysuint1, rightarg = ivymy.mysuint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);


/**************************************** UINT1 *********************************************************/
/*
 * B-tree Index Support
 * Create Operator family 'mysuint1_ops' for type 'mysuint1'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','mysuint1int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(pg_catalog.int2, ivymy.mysuint1)  RETURNS integer AS 'MODULE_PATHNAME','int2mysuint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','mysuint1int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(pg_catalog.int4, ivymy.mysuint1)  RETURNS integer AS 'MODULE_PATHNAME','int4mysuint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','mysuint1int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(pg_catalog.int8, ivymy.mysuint1)  RETURNS integer AS 'MODULE_PATHNAME','int8mysuint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  ivymy.mysuint1)  RETURNS integer AS 'MODULE_PATHNAME','mysuint11_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  ivymy.mysuint2)  RETURNS integer AS 'MODULE_PATHNAME','mysuint12_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  ivymy.mysuint4)  RETURNS integer AS 'MODULE_PATHNAME','mysuint14_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.mysuint1_cmp(ivymy.mysuint1,  ivymy.mysuint8)  RETURNS integer AS 'MODULE_PATHNAME','mysuint18_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.mysuint1_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysuint1_ops' 
CREATE OPERATOR FAMILY ivymy.mysuint1_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'oramysuint1' */
CREATE OPERATOR CLASS ivymy.mysuint1_ops
	DEFAULT FOR TYPE ivymy.mysuint1 USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		2		<= (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		3		=  (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		4		>= (ivymy.mysuint1, ivymy.mysuint1),
		OPERATOR		5		>  (ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		2		ivymy.mysuint1_sortsupport(internal);
ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.mysuint1, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.mysuint1, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.mysuint1, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.mysuint1, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.mysuint1, pg_catalog.int4),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, pg_catalog.int4),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.mysuint1),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.mysuint1),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.mysuint1),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.mysuint1),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.mysuint1),
	FUNCTION		1		ivymy.mysuint1_cmp(pg_catalog.int4, ivymy.mysuint1),

	OPERATOR		1		<  (ivymy.mysuint1, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.mysuint1, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.mysuint1, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.mysuint1, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.mysuint1, pg_catalog.int2),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, pg_catalog.int2),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.mysuint1),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.mysuint1),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.mysuint1),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.mysuint1),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.mysuint1),
	FUNCTION		1		ivymy.mysuint1_cmp(pg_catalog.int2, ivymy.mysuint1),

	OPERATOR		1		<  (ivymy.mysuint1, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.mysuint1, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.mysuint1, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.mysuint1, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.mysuint1, pg_catalog.int8),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, pg_catalog.int8),

	OPERATOR		1		< (pg_catalog.int8, ivymy.mysuint1),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.mysuint1),
	OPERATOR		3		= (pg_catalog.int8, ivymy.mysuint1),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.mysuint1),
	OPERATOR		5		> (pg_catalog.int8, ivymy.mysuint1),
	FUNCTION		1		ivymy.mysuint1_cmp(pg_catalog.int8, ivymy.mysuint1),


	OPERATOR		1		<  (ivymy.mysuint1, ivymy.mysuint2),
	OPERATOR		2		<= (ivymy.mysuint1, ivymy.mysuint2),
	OPERATOR		3		=  (ivymy.mysuint1, ivymy.mysuint2),
	OPERATOR		4		>= (ivymy.mysuint1, ivymy.mysuint2),
	OPERATOR		5		>  (ivymy.mysuint1, ivymy.mysuint2),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint2),

	OPERATOR		1		<  (ivymy.mysuint1, ivymy.mysuint4),
	OPERATOR		2		<= (ivymy.mysuint1, ivymy.mysuint4),
	OPERATOR		3		=  (ivymy.mysuint1, ivymy.mysuint4),
	OPERATOR		4		>= (ivymy.mysuint1, ivymy.mysuint4),
	OPERATOR		5		>  (ivymy.mysuint1, ivymy.mysuint4),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint4),

	OPERATOR		1		<  (ivymy.mysuint1, ivymy.mysuint8),
	OPERATOR		2		<= (ivymy.mysuint1, ivymy.mysuint8),
	OPERATOR		3		=  (ivymy.mysuint1, ivymy.mysuint8),
	OPERATOR		4		>= (ivymy.mysuint1, ivymy.mysuint8),
	OPERATOR		5		>  (ivymy.mysuint1, ivymy.mysuint8),
	FUNCTION		1		ivymy.mysuint1_cmp(ivymy.mysuint1, ivymy.mysuint8);


/* HASH index support */
CREATE FUNCTION ivymy.mysuint1_hash(ivymy.mysuint1)
RETURNS integer
AS 'MODULE_PATHNAME','mysuint1_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysuint1_hash_extended(ivymy.mysuint1, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysuint1_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysuint1_ops
	DEFAULT FOR TYPE ivymy.mysuint1 USING hash AS
		OPERATOR		1		= (ivymy.mysuint1, ivymy.mysuint1),
		FUNCTION		1		ivymy.mysuint1_hash(ivymy.mysuint1),
		FUNCTION		2		ivymy.mysuint1_hash_extended(ivymy.mysuint1, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;
