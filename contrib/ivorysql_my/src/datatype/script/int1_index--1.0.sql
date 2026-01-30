CREATE FUNCTION ivymy.int1int1_eq(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int1_ne(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int1_lt(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int1_gt(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int1_le(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int1_ge(ivymy.tinyint, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int1int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1int1_eq, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1int1_ne, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1int1_lt, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1int1_gt, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1int1_le, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1int1_ge, leftarg = ivymy.tinyint, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1int2_eq(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int2_ne(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int2_lt(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int2_gt(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int2_le(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int2_ge(ivymy.tinyint, pg_catalog.int2) RETURNS boolean AS 'MODULE_PATHNAME','int1int2_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1int2_eq, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1int2_ne, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1int2_lt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1int2_gt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1int2_le, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1int2_ge, leftarg = ivymy.tinyint, rightarg = pg_catalog.int2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1int4_eq(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int4_ne(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int4_lt(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int4_gt(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int4_le(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int4_ge(ivymy.tinyint, pg_catalog.int4) RETURNS boolean AS 'MODULE_PATHNAME','int1int4_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1int4_eq, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1int4_ne, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1int4_lt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1int4_gt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1int4_le, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1int4_ge, leftarg = ivymy.tinyint, rightarg = pg_catalog.int4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1int8_eq(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int8_ne(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int8_lt(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int8_gt(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int8_le(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1int8_ge(ivymy.tinyint, pg_catalog.int8) RETURNS boolean AS 'MODULE_PATHNAME','int1int8_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1int8_eq, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1int8_ne, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1int8_lt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1int8_gt, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1int8_le, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1int8_ge, leftarg = ivymy.tinyint, rightarg = pg_catalog.int8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1uint1_eq(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint1_ne(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint1_lt(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint1_gt(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint1_le(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint1_ge(ivymy.tinyint, ivymy.uint1) RETURNS boolean AS 'MODULE_PATHNAME','int1uint1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1uint1_eq, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1uint1_ne, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1uint1_lt, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = >,  negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1uint1_gt, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = <,  negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1uint1_le, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1uint1_ge, leftarg = ivymy.tinyint, rightarg = ivymy.uint1, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1uint2_eq(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint2_ne(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint2_lt(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint2_gt(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint2_le(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint2_ge(ivymy.tinyint, ivymy.uint2) RETURNS boolean AS 'MODULE_PATHNAME','int1uint2_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1uint2_eq, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1uint2_ne, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1uint2_lt, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = >,  negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1uint2_gt, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = <,  negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1uint2_le, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1uint2_ge, leftarg = ivymy.tinyint, rightarg = ivymy.uint2, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1uint4_eq(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint4_ne(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint4_lt(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint4_gt(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint4_le(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint4_ge(ivymy.tinyint, ivymy.uint4) RETURNS boolean AS 'MODULE_PATHNAME','int1uint4_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1uint4_eq, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1uint4_ne, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1uint4_lt, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = >,  negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1uint4_gt, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = <,  negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1uint4_le, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1uint4_ge, leftarg = ivymy.tinyint, rightarg = ivymy.uint4, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int1uint8_eq(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint8_ne(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint8_lt(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint8_gt(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint8_le(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1uint8_ge(ivymy.tinyint, ivymy.uint8) RETURNS boolean AS 'MODULE_PATHNAME','int1uint8_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int1uint8_eq, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int1uint8_ne, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int1uint8_lt, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = >,  negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int1uint8_gt, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = <,  negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int1uint8_le, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int1uint8_ge, leftarg = ivymy.tinyint, rightarg = ivymy.uint8, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int2int1_eq(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int2int1_ne(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int2int1_lt(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int2int1_gt(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int2int1_le(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int2int1_ge(pg_catalog.int2, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int2int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int2int1_eq, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int2int1_ne, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int2int1_lt, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int2int1_gt, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int2int1_le, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int2int1_ge, leftarg = pg_catalog.int2, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int4int1_eq(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int4int1_ne(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int4int1_lt(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int4int1_gt(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int4int1_le(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int4int1_ge(pg_catalog.int4, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int4int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int4int1_eq, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int4int1_ne, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int4int1_lt, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int4int1_gt, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int4int1_le, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int4int1_ge, leftarg = pg_catalog.int4, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

CREATE FUNCTION ivymy.int8int1_eq(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_eq' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int8int1_ne(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_ne' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int8int1_lt(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_lt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int8int1_gt(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_gt' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int8int1_le(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_le' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int8int1_ge(pg_catalog.int8, ivymy.tinyint) RETURNS boolean AS 'MODULE_PATHNAME','int8int1_ge' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE OPERATOR =  (procedure = ivymy.int8int1_eq, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = =, negator = <>,restrict = eqsel, join = eqjoinsel, merges);
CREATE OPERATOR != (procedure = ivymy.int8int1_ne, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = <>, negator = =, restrict = neqsel, join = neqjoinsel);
CREATE OPERATOR <  (procedure = ivymy.int8int1_lt, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = >,	negator = >=, restrict = scalargesel, join = scalargejoinsel);
CREATE OPERATOR >  (procedure = ivymy.int8int1_gt, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = <,	negator = <=, restrict = scalargtsel, join = scalargtjoinsel);
CREATE OPERATOR <= (procedure = ivymy.int8int1_le, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = >=, negator = >, restrict = scalarlesel, join = scalarlejoinsel);
CREATE OPERATOR >= (procedure = ivymy.int8int1_ge, leftarg = pg_catalog.int8, rightarg = ivymy.tinyint, commutator = <=, negator = <, restrict = scalargesel, join = scalargejoinsel);

/*
 * B-tree Index Support
 * Create Operator family 'int1_ops' for type 'int1'.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, ivymy.tinyint) RETURNS integer AS 'MODULE_PATHNAME','int1int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int2) RETURNS integer AS 'MODULE_PATHNAME','int1int2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(pg_catalog.int2, ivymy.tinyint) RETURNS integer AS 'MODULE_PATHNAME','int2int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int4) RETURNS integer AS 'MODULE_PATHNAME','int1int4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(pg_catalog.int4, ivymy.tinyint) RETURNS integer AS 'MODULE_PATHNAME','int4int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int8) RETURNS integer AS 'MODULE_PATHNAME','int1int8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(pg_catalog.int8, ivymy.tinyint) RETURNS integer AS 'MODULE_PATHNAME','int8int1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, ivymy.uint1) RETURNS integer AS 'MODULE_PATHNAME','int1uint1_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, ivymy.uint2) RETURNS integer AS 'MODULE_PATHNAME','int1uint2_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, ivymy.uint4) RETURNS integer AS 'MODULE_PATHNAME','int1uint4_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;
CREATE FUNCTION ivymy.int1_cmp(ivymy.tinyint, ivymy.uint8) RETURNS integer AS 'MODULE_PATHNAME','int1uint8_cmp' LANGUAGE C PARALLEL SAFE STRICT IMMUTABLE LEAKPROOF;

CREATE FUNCTION ivymy.int1_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME', 'btint1sortsupport'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'int1_ops'
CREATE OPERATOR FAMILY ivymy.int1_ops USING btree;*/

/* CREATE OPERATOR CLASS for 'int1' */
CREATE OPERATOR CLASS ivymy.int1_ops
	DEFAULT FOR TYPE ivymy.tinyint USING btree FAMILY integer_ops AS
		OPERATOR		1		<  (ivymy.tinyint, ivymy.tinyint),
		OPERATOR		2		<= (ivymy.tinyint, ivymy.tinyint),
		OPERATOR		3		=  (ivymy.tinyint, ivymy.tinyint),
		OPERATOR		4		>= (ivymy.tinyint, ivymy.tinyint),
		OPERATOR		5		>  (ivymy.tinyint, ivymy.tinyint),
		FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, ivymy.tinyint),
		FUNCTION		2		ivymy.int1_sortsupport(internal);

ALTER OPERATOR FAMILY integer_ops USING btree ADD
	-- Cross data type comparison
	OPERATOR		1		<  (ivymy.tinyint, pg_catalog.int2),
	OPERATOR		2		<= (ivymy.tinyint, pg_catalog.int2),
	OPERATOR		3		=  (ivymy.tinyint, pg_catalog.int2),
	OPERATOR		4		>= (ivymy.tinyint, pg_catalog.int2),
	OPERATOR		5		>  (ivymy.tinyint, pg_catalog.int2),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int2),

	OPERATOR		1		<  (ivymy.tinyint, pg_catalog.int4),
	OPERATOR		2		<= (ivymy.tinyint, pg_catalog.int4),
	OPERATOR		3		=  (ivymy.tinyint, pg_catalog.int4),
	OPERATOR		4		>= (ivymy.tinyint, pg_catalog.int4),
	OPERATOR		5		>  (ivymy.tinyint, pg_catalog.int4),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int4),

	OPERATOR		1		<  (ivymy.tinyint, pg_catalog.int8),
	OPERATOR		2		<= (ivymy.tinyint, pg_catalog.int8),
	OPERATOR		3		=  (ivymy.tinyint, pg_catalog.int8),
	OPERATOR		4		>= (ivymy.tinyint, pg_catalog.int8),
	OPERATOR		5		>  (ivymy.tinyint, pg_catalog.int8),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, pg_catalog.int8),

	OPERATOR		1		<  (ivymy.tinyint, ivymy.uint1),
	OPERATOR		2		<= (ivymy.tinyint, ivymy.uint1),
	OPERATOR		3		=  (ivymy.tinyint, ivymy.uint1),
	OPERATOR		4		>= (ivymy.tinyint, ivymy.uint1),
	OPERATOR		5		>  (ivymy.tinyint, ivymy.uint1),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, ivymy.uint1),

	OPERATOR		1		<  (ivymy.tinyint, ivymy.uint2),
	OPERATOR		2		<= (ivymy.tinyint, ivymy.uint2),
	OPERATOR		3		=  (ivymy.tinyint, ivymy.uint2),
	OPERATOR		4		>= (ivymy.tinyint, ivymy.uint2),
	OPERATOR		5		>  (ivymy.tinyint, ivymy.uint2),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, ivymy.uint2),

	OPERATOR		1		<  (ivymy.tinyint, ivymy.uint4),
	OPERATOR		2		<= (ivymy.tinyint, ivymy.uint4),
	OPERATOR		3		=  (ivymy.tinyint, ivymy.uint4),
	OPERATOR		4		>= (ivymy.tinyint, ivymy.uint4),
	OPERATOR		5		>  (ivymy.tinyint, ivymy.uint4),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, ivymy.uint4),

	OPERATOR		1		<  (ivymy.tinyint, ivymy.uint8),
	OPERATOR		2		<= (ivymy.tinyint, ivymy.uint8),
	OPERATOR		3		=  (ivymy.tinyint, ivymy.uint8),
	OPERATOR		4		>= (ivymy.tinyint, ivymy.uint8),
	OPERATOR		5		>  (ivymy.tinyint, ivymy.uint8),
	FUNCTION		1		ivymy.int1_cmp(ivymy.tinyint, ivymy.uint8),

	OPERATOR		1		<  (pg_catalog.int2, ivymy.tinyint),
	OPERATOR		2		<= (pg_catalog.int2, ivymy.tinyint),
	OPERATOR		3		=  (pg_catalog.int2, ivymy.tinyint),
	OPERATOR		4		>= (pg_catalog.int2, ivymy.tinyint),
	OPERATOR		5		>  (pg_catalog.int2, ivymy.tinyint),
	FUNCTION		1		ivymy.int1_cmp(pg_catalog.int2, ivymy.tinyint),

	OPERATOR		1		<  (pg_catalog.int4, ivymy.tinyint),
	OPERATOR		2		<= (pg_catalog.int4, ivymy.tinyint),
	OPERATOR		3		=  (pg_catalog.int4, ivymy.tinyint),
	OPERATOR		4		>= (pg_catalog.int4, ivymy.tinyint),
	OPERATOR		5		>  (pg_catalog.int4, ivymy.tinyint),
	FUNCTION		1		ivymy.int1_cmp(pg_catalog.int4, ivymy.tinyint),

	OPERATOR		1		<  (pg_catalog.int8, ivymy.tinyint),
	OPERATOR		2		<= (pg_catalog.int8, ivymy.tinyint),
	OPERATOR		3		=  (pg_catalog.int8, ivymy.tinyint),
	OPERATOR		4		>= (pg_catalog.int8, ivymy.tinyint),
	OPERATOR		5		>  (pg_catalog.int8, ivymy.tinyint),
	FUNCTION		1		ivymy.int1_cmp(pg_catalog.int8, ivymy.tinyint);

/* HASH index support */
CREATE FUNCTION ivymy.int1_hash(ivymy.tinyint)
RETURNS integer
AS 'MODULE_PATHNAME','int1_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int1_hash_extended(ivymy.tinyint, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','int1_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.int1_ops
	DEFAULT FOR TYPE ivymy.tinyint USING hash AS
		OPERATOR		1		= (ivymy.tinyint, ivymy.tinyint),
		FUNCTION		1		ivymy.int1_hash(ivymy.tinyint),
		FUNCTION		2		ivymy.int1_hash_extended(ivymy.tinyint, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;