/********************************************************************************
*
* GIN_BTREE_CONSISTENT_BINARY FUNCTION FOR ALL BINARY DATATYPE
* REFERENCE FORM ORACLE
*
********************************************************************************/

CREATE FUNCTION ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal)
RETURNS bool
AS 'MODULE_PATHNAME', 'gin_btree_consistent_binary'
LANGUAGE C STRICT IMMUTABLE;

/***************************************************************
 *
 * mysql binary type support
 *
 ***************************************************************/

CREATE FUNCTION ivymy.binary_in(CSTRING, oid, int, int8)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.binary_out(ivymy.binary)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.binary_recv(internal, oid, int, int8)
RETURNS ivymy.binary
AS 'bytearecv'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.binary_send(ivymy.binary)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.binary_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','binary_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.binary_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='binary_in')
WHERE oid = 9240;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='binary_out')
WHERE oid = 9240;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='binary_recv')
WHERE oid = 9240;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='binary_send')
WHERE oid = 9240;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='binary_typmodin')
WHERE oid = 9240 or oid = 9241;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='binary_typmodout')
WHERE oid = 9240 or oid = 9241;


/********************************************************************************
 *
 * BINARY CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.binary(ivymy.binary, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary_bool(ivymy.binary, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(bytea, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(ivymy.varbinary, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(ivymy.myblob, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.binary(ivymy.longblob, integer, boolean)
RETURNS ivymy.binary
AS 'MODULE_PATHNAME','binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.binary AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.binary, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS pg_catalog.bool)
WITH FUNCTION ivymy.binary_bool(ivymy.binary, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.binary)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.binary)
WITH FUNCTION ivymy.binary(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.varbinary AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.varbinary, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.myblob AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.myblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.tinyblob AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.tinyblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.mediumblob AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.mediumblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.longblob AS ivymy.binary)
WITH FUNCTION ivymy.binary(ivymy.longblob, integer, boolean)
AS IMPLICIT;

/********************************************************************************
 *
 * BINARY OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.binary_eq(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.binary_ne(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.binary_lt(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.binary_gt(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.binary_le(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.binary_ge(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.binary_seq(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.binary_like(ivymy.binary, ivymy.binary)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.binary_notlike(ivymy.binary, ivymy.binary)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function div
CREATE OR REPLACE FUNCTION ivymy.binary_div(ivymy.binary, ivymy.binary)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_div'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function div
CREATE OR REPLACE FUNCTION ivymy.anybinary_div_floor(ivymy.binary, ivymy.binary)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_div_floor'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ivymy.// (
	procedure = ivymy.anybinary_div_floor,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.binary_eq,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.binary_ne,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.binary_gt,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.binary_lt,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.binary_le,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.binary_ge,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.binary_seq,
	leftarg = ivymy.binary,
	rightarg = ivymy.binary
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.binary_like,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.binary_notlike,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary,
	negator = ~~
);

----operator /
CREATE OPERATOR ivymy./ (
	procedure = ivymy.binary_div,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function and
CREATE OR REPLACE FUNCTION ivymy.binary_and(ivymy.binary, ivymy.binary)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.binary_and,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function not
CREATE OR REPLACE FUNCTION ivymy.binary_not(ivymy.binary)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.binary_not,
	rightarg  = ivymy.binary
);

----function bitand
CREATE OR REPLACE FUNCTION ivymy.binary_bitand(ivymy.binary, ivymy.binary)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.binary_bitand,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function bitor
CREATE OR REPLACE FUNCTION ivymy.binary_bitor(ivymy.binary, ivymy.binary)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.binary_bitor,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.binary_bitxor(ivymy.binary, ivymy.binary)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.binary_bitxor,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.binary_bitneg(ivymy.binary)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.binary_bitneg,
	rightarg  = ivymy.binary
);

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.binary_bitshiftleft(ivymy.binary, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.binary_bitshiftleft,
	leftarg   = ivymy.binary,
	rightarg  = integer
);

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.binary_bitshiftright(ivymy.binary, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.binary_bitshiftright,
	leftarg   = ivymy.binary,
	rightarg  = integer
);

----function binary_plus
CREATE OR REPLACE FUNCTION ivymy.binary_plus(ivymy.binary, ivymy.binary)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_plus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator +
CREATE OPERATOR ivymy.+ (
	procedure = ivymy.binary_plus,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function binary_sub
CREATE OR REPLACE FUNCTION ivymy.binary_sub(ivymy.binary, ivymy.binary)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_sub'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.binary_sub,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function binary_minus
CREATE OR REPLACE FUNCTION ivymy.binary_minus(ivymy.binary)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.binary_minus,
	rightarg  = ivymy.binary
);

----function binary_mul
CREATE OR REPLACE FUNCTION ivymy.binary_mul(ivymy.binary, ivymy.binary)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_mul'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator *
CREATE OPERATOR ivymy.* (
	procedure = ivymy.binary_mul,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

----function binary_mod
CREATE OR REPLACE FUNCTION ivymy.binary_mod(ivymy.binary, ivymy.binary)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_mod'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator %
CREATE OPERATOR ivymy.% (
	procedure = ivymy.binary_mod,
	leftarg   = ivymy.binary,
	rightarg  = ivymy.binary
);

/********************************************************************************
 *
 * CREATE BINARY OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
CREATE OPERATOR FAMILY ivymy.binary_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.binary_minmax_ops
    DEFAULT FOR TYPE ivymy.binary USING brin FAMILY ivymy.binary_minmax_ops AS
        OPERATOR        1       < (ivymy.binary, ivymy.binary),
        OPERATOR        2       <= (ivymy.binary, ivymy.binary),
        OPERATOR        3       = (ivymy.binary, ivymy.binary),
        OPERATOR        4       >= (ivymy.binary, ivymy.binary),
        OPERATOR        5       > (ivymy.binary, ivymy.binary),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/* binary_cmp gin and btree use */
CREATE FUNCTION ivymy.binary_cmp(ivymy.binary, ivymy.binary)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_binary(ivymy.binary, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_binary(ivymy.binary, ivymy.binary, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_binary(ivymy.binary, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_binary_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_binary_ops
DEFAULT FOR TYPE ivymy.binary USING gin
AS
	OPERATOR		1		< (ivymy.binary, ivymy.binary),
	OPERATOR		2		<= (ivymy.binary, ivymy.binary),
	OPERATOR		3		= (ivymy.binary, ivymy.binary),
	OPERATOR		4		>= (ivymy.binary, ivymy.binary),
	OPERATOR		5		> (ivymy.binary, ivymy.binary),
	FUNCTION		1		ivymy.binary_cmp(ivymy.binary, ivymy.binary),
	FUNCTION		2		ivymy.gin_extract_value_binary(ivymy.binary, internal),
	FUNCTION		3		ivymy.gin_extract_query_binary(ivymy.binary, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_binary(ivymy.binary, ivymy.binary, int2, internal),
STORAGE			ivymy.binary;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.binary_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME','binary_sortsupport'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR FAMILY ivymy.binary_ops USING btree;

CREATE OPERATOR CLASS ivymy.binary_ops
	DEFAULT FOR TYPE ivymy.binary USING btree FAMILY ivymy.binary_ops AS
		OPERATOR		1		< (ivymy.binary, ivymy.binary),
		OPERATOR		2		<= (ivymy.binary, ivymy.binary),
		OPERATOR		3		= (ivymy.binary, ivymy.binary),
		OPERATOR		4		>= (ivymy.binary, ivymy.binary),
		OPERATOR		5		> (ivymy.binary, ivymy.binary),
		FUNCTION		1		ivymy.binary_cmp(ivymy.binary, ivymy.binary),
		FUNCTION		2		ivymy.binary_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.binary_hash(ivymy.binary)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_hash'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.binary_hash_extended(ivymy.binary, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_hash_extended'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR CLASS ivymy.binary_ops
DEFAULT FOR TYPE ivymy.binary USING hash AS
  OPERATOR 1 = (ivymy.binary, ivymy.binary),
  FUNCTION 1 ivymy.binary_hash(ivymy.binary),
  FUNCTION 2 ivymy.binary_hash_extended(ivymy.binary, bigint);

------------------------------------------------------------------------------------------------
-------------------------------------NEXT DATATYPE LINE-----------------------------------------
------------------------------------------------------------------------------------------------

/********************************************************************************
 *
 * VARBINARY DATATYPE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.varbinary_in(CSTRING)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.varbinary_out(ivymy.varbinary)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.varbinary_recv(internal)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.varbinary_send(ivymy.varbinary)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.varbinary_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','varbinary_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.varbinary_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='varbinary_in')
WHERE oid = 9242;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='varbinary_out')
WHERE oid = 9242;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='varbinary_recv')
WHERE oid = 9242;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='varbinary_send')
WHERE oid = 9242;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='varbinary_typmodin')
WHERE oid = 9242 or oid = 9243;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='varbinary_typmodout')
WHERE oid = 9242 or oid = 9243;

/********************************************************************************
 *
 * VARBINARY CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.varbinary(ivymy.varbinary, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary_bool(ivymy.varbinary, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(bytea, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(ivymy.binary, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(ivymy.myblob, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.varbinary(ivymy.longblob, integer, boolean)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','varbinary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.varbinary AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.varbinary, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.varbinary AS pg_catalog.bool)
WITH FUNCTION ivymy.varbinary_bool(ivymy.varbinary, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.varbinary AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.varbinary)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.myblob AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.myblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.tinyblob AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.tinyblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.mediumblob AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.mediumblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.longblob AS ivymy.varbinary)
WITH FUNCTION ivymy.varbinary(ivymy.longblob, integer, boolean)
AS ASSIGNMENT;

/* Begin BUG#5048 5070 */
-- cast varbinary to text
CREATE FUNCTION ivymy.varbinary2text(ivymy.varbinary, integer, boolean)
RETURNS text
AS 'MODULE_PATHNAME','varbinary2text'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS text)
WITH FUNCTION ivymy.varbinary2text(ivymy.varbinary, integer, boolean)
AS IMPLICIT;

-- cast varbinary to bpchar
CREATE FUNCTION ivymy.varbinary2bpchar(ivymy.varbinary, integer, boolean)
RETURNS bpchar
AS 'MODULE_PATHNAME','varbinary2bpchar'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS bpchar)
WITH FUNCTION ivymy.varbinary2bpchar(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

-- cast varbinary to varchar
CREATE FUNCTION ivymy.varbinary2varchar(ivymy.varbinary, integer, boolean)
RETURNS varchar
AS 'MODULE_PATHNAME','varbinary2varchar'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS varchar)
WITH FUNCTION ivymy.varbinary2varchar(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

-- cast varbinary to int2
CREATE FUNCTION ivymy.varbinary2int2(ivymy.varbinary, integer, boolean, int8)
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','varlena_int2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS pg_catalog.int2)
WITH FUNCTION ivymy.varbinary2int2(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to int4
CREATE FUNCTION ivymy.varbinary2int4(ivymy.varbinary, integer, boolean, int8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','varlena_int4'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS pg_catalog.int4)
WITH FUNCTION ivymy.varbinary2int4(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to int8
CREATE FUNCTION ivymy.varbinary2int8(ivymy.varbinary, integer, boolean, int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','varlena_int8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS pg_catalog.int8)
WITH FUNCTION ivymy.varbinary2int8(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to uint2
CREATE FUNCTION ivymy.varbinary2uint2(ivymy.varbinary, integer, boolean, int8)
RETURNS uint2
AS 'MODULE_PATHNAME','varlena_uint2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS uint2)
WITH FUNCTION ivymy.varbinary2uint2(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to uint4
CREATE FUNCTION ivymy.varbinary2uint4(ivymy.varbinary, integer, boolean, int8)
RETURNS uint4
AS 'MODULE_PATHNAME','varlena_uint4'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS uint4)
WITH FUNCTION ivymy.varbinary2uint4(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to uint8
CREATE FUNCTION ivymy.varbinary2uint8(ivymy.varbinary, integer, boolean, int8)
RETURNS uint8
AS 'MODULE_PATHNAME','varlena_uint8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS uint8)
WITH FUNCTION ivymy.varbinary2uint8(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to numeric
CREATE FUNCTION ivymy.varbinary_numeric(ivymy.varbinary, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS numeric)
WITH FUNCTION ivymy.varbinary_numeric(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;


-- cast varbinary to date
CREATE FUNCTION ivymy.varbinary2date(ivymy.varbinary, integer, boolean)
RETURNS date
AS 'MODULE_PATHNAME','varbinary2date'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS date)
WITH FUNCTION ivymy.varbinary2date(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

-- cast varbinary to time
CREATE FUNCTION ivymy.varbinary2time(ivymy.varbinary, integer, boolean)
RETURNS time
AS 'MODULE_PATHNAME','varbinary2time'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS time)
WITH FUNCTION ivymy.varbinary2time(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

-- cast varbinary to datetime
CREATE FUNCTION ivymy.varbinary2datetime(ivymy.varbinary, integer, boolean)
RETURNS datetime
AS 'MODULE_PATHNAME','varbinary2datetime'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS datetime)
WITH FUNCTION ivymy.varbinary2datetime(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

/* End BUG#5048 5070 */

/********************************************************************************
 *
 * BINARY OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.varbinary_eq(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.varbinary_ne(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.varbinary_lt(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.varbinary_gt(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.varbinary_le(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.varbinary_ge(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.varbinary_seq(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.varbinary_like(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.varbinary_notlike(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function and
CREATE OR REPLACE FUNCTION ivymy.varbinary_and(ivymy.varbinary, ivymy.varbinary)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function not
CREATE OR REPLACE FUNCTION ivymy.varbinary_not(ivymy.varbinary)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitand
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitand(ivymy.varbinary, ivymy.varbinary)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitor
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitor(ivymy.varbinary, ivymy.varbinary)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitxor(ivymy.varbinary, ivymy.varbinary)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitneg(ivymy.varbinary)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitshiftleft(ivymy.varbinary, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.varbinary_bitshiftright(ivymy.varbinary, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.varbinary_eq,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.varbinary_ne,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.varbinary_gt,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.varbinary_lt,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.varbinary_le,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.varbinary_ge,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.varbinary_seq,
	leftarg = ivymy.varbinary,
	rightarg = ivymy.varbinary
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.varbinary_like,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.varbinary_notlike,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary,
	negator = ~~
);

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.varbinary_and,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary
);

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.varbinary_not,
	rightarg  = ivymy.varbinary
);

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.varbinary_bitand,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary
);

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.varbinary_bitor,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary
);

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.varbinary_bitxor,
	leftarg   = ivymy.varbinary,
	rightarg  = ivymy.varbinary
);

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.varbinary_bitneg,
	rightarg  = ivymy.varbinary
);

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.varbinary_bitshiftleft,
	leftarg   = ivymy.varbinary,
	rightarg  = integer
);

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.varbinary_bitshiftright,
	leftarg   = ivymy.varbinary,
	rightarg  = integer
);

/********************************************************************************
 *
 * CREATE VARBINARY OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
--reference from Oracle
CREATE OPERATOR FAMILY ivymy.varbinary_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.varbinary_minmax_ops
    DEFAULT FOR TYPE ivymy.varbinary USING brin FAMILY ivymy.varbinary_minmax_ops AS
        OPERATOR        1       < (ivymy.varbinary, ivymy.varbinary),
        OPERATOR        2       <= (ivymy.varbinary, ivymy.varbinary),
        OPERATOR        3       = (ivymy.varbinary, ivymy.varbinary),
        OPERATOR        4       >= (ivymy.varbinary, ivymy.varbinary),
        OPERATOR        5       > (ivymy.varbinary, ivymy.varbinary),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/*varbinary_cmp gin and btree use*/
CREATE FUNCTION ivymy.varbinary_cmp(ivymy.varbinary, ivymy.varbinary)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_varbinary(ivymy.varbinary, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_varbinary(ivymy.varbinary, ivymy.varbinary, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_varbinary(ivymy.varbinary, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_varbinary_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_varbinary_ops
DEFAULT FOR TYPE ivymy.varbinary USING gin
AS
	OPERATOR		1		< (ivymy.varbinary, ivymy.varbinary),
	OPERATOR		2		<= (ivymy.varbinary, ivymy.varbinary),
	OPERATOR		3		= (ivymy.varbinary, ivymy.varbinary),
	OPERATOR		4		>= (ivymy.varbinary, ivymy.varbinary),
	OPERATOR		5		> (ivymy.varbinary, ivymy.varbinary),
	FUNCTION		1		ivymy.varbinary_cmp(ivymy.varbinary, ivymy.varbinary),
	FUNCTION		2		ivymy.gin_extract_value_varbinary(ivymy.varbinary, internal),
	FUNCTION		3		ivymy.gin_extract_query_varbinary(ivymy.varbinary, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_varbinary(ivymy.varbinary, ivymy.varbinary, int2, internal),
STORAGE			ivymy.varbinary;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.varbinary_sortsupport(internal)
	RETURNS void
	AS 'MODULE_PATHNAME','varbinary_sortsupport'
	LANGUAGE C
	PARALLEL SAFE
	STRICT
	IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.varbinary_ops USING btree;

CREATE OPERATOR CLASS ivymy.varbinary_ops
	DEFAULT FOR TYPE ivymy.varbinary USING btree FAMILY ivymy.varbinary_ops AS
		OPERATOR		1		< (ivymy.varbinary, ivymy.varbinary),
		OPERATOR		2		<= (ivymy.varbinary, ivymy.varbinary),
		OPERATOR		3		= (ivymy.varbinary, ivymy.varbinary),
		OPERATOR		4		>= (ivymy.varbinary, ivymy.varbinary),
		OPERATOR		5		> (ivymy.varbinary, ivymy.varbinary),
		FUNCTION		1		ivymy.varbinary_cmp(ivymy.varbinary, ivymy.varbinary),
		FUNCTION		2		ivymy.varbinary_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.varbinary_hash(ivymy.varbinary)
	RETURNS integer
	AS 'MODULE_PATHNAME','anybinary_hash'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE FUNCTION ivymy.varbinary_hash_extended(ivymy.varbinary, bigint)
	RETURNS bigint
	AS 'MODULE_PATHNAME','anybinary_hash_extended'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE OPERATOR CLASS ivymy.varbinary_ops
DEFAULT FOR TYPE ivymy.varbinary USING hash AS
  OPERATOR 1 = (ivymy.varbinary, ivymy.varbinary),
  FUNCTION 1 ivymy.varbinary_hash(ivymy.varbinary),
  FUNCTION 2 ivymy.varbinary_hash_extended(ivymy.varbinary, bigint);


------------------------------------------------------------------------------------------------
-------------------------------------NEXT DATATYPE LINE-----------------------------------------
------------------------------------------------------------------------------------------------

/********************************************************************************
 *
 * MYBLOB DATATYPE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.myblob_in (CSTRING)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.myblob_out (ivymy.myblob)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.myblob_recv (internal)
RETURNS ivymy.myblob
AS 'bytearecv'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.myblob_send (ivymy.myblob)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.myblob_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','myblob_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.myblob_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='myblob_in')
WHERE oid = 9234;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='myblob_out')
WHERE oid = 9234;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='myblob_recv')
WHERE oid = 9234;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='myblob_send')
WHERE oid = 9234;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='myblob_typmodin')
WHERE oid = 9234 or oid = 9235;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='myblob_typmodout')
WHERE oid = 9234 or oid = 9235;;

CREATE FUNCTION ivymy.bit_count(ivymy.binary)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_count(ivymy.varbinary)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_binary'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_count(ivymy.myblob)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_blob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_count(ivymy.tinyblob)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_blob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_count(ivymy.mediumblob)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_blob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.bit_count(ivymy.longblob)
RETURNS int4
AS 'MODULE_PATHNAME','mys_bit_count_blob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

/********************************************************************************
 *
 * BLOB CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.myblob(ivymy.myblob, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob_bool(ivymy.myblob, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.pgtext2myblob(pg_catalog.text, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','pgtext2myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(bytea, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(ivymy.binary, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(ivymy.varbinary, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.myblob(ivymy.longblob, integer, boolean)
RETURNS ivymy.myblob
AS 'MODULE_PATHNAME','myblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.myblob AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.myblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.myblob AS pg_catalog.bool)
WITH FUNCTION ivymy.myblob_bool(ivymy.myblob, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.myblob AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.myblob)
WITH FUNCTION ivymy.pgtext2myblob(pg_catalog.text, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.varbinary AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.tinyblob AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.tinyblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.mediumblob AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.mediumblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.longblob AS ivymy.myblob)
WITH FUNCTION ivymy.myblob(ivymy.longblob, integer, boolean)
AS ASSIGNMENT;

/********************************************************************************
 *
 * BINARY OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.myblob_eq(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.myblob_ne(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.myblob_lt(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.myblob_gt(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.myblob_le(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.myblob_ge(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.myblob_seq(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.myblob_like(ivymy.myblob, ivymy.myblob)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.myblob_notlike(ivymy.myblob, ivymy.myblob)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.myblob_eq,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.myblob_ne,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.myblob_gt,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.myblob_lt,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.myblob_le,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.myblob_ge,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.myblob_seq,
	leftarg = ivymy.myblob,
	rightarg = ivymy.myblob
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.myblob_like,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.myblob_notlike,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob,
	negator = ~~
);

----function and
CREATE OR REPLACE FUNCTION ivymy.myblob_and(ivymy.myblob, ivymy.myblob)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.myblob_and,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);


----function not
CREATE OR REPLACE FUNCTION ivymy.myblob_not(ivymy.myblob)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.myblob_not,
	rightarg  = ivymy.myblob
);

----function bitand
CREATE OR REPLACE FUNCTION ivymy.myblob_bitand(ivymy.myblob, ivymy.myblob)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.myblob_bitand,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function bitor
CREATE OR REPLACE FUNCTION ivymy.myblob_bitor(ivymy.myblob, ivymy.myblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.myblob_bitor,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.myblob_bitxor(ivymy.myblob, ivymy.myblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.myblob_bitxor,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.myblob_bitneg(ivymy.myblob)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.myblob_bitneg,
	rightarg  = ivymy.myblob
);

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.myblob_bitshiftleft(ivymy.myblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.myblob_bitshiftleft,
	leftarg   = ivymy.myblob,
	rightarg  = integer
);

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.myblob_bitshiftright(ivymy.myblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.myblob_bitshiftright,
	leftarg   = ivymy.myblob,
	rightarg  = integer
);

----function blob_plus
CREATE OR REPLACE FUNCTION ivymy.myblob_plus(ivymy.myblob, ivymy.myblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_plus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator +
CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myblob_plus,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function blob_sub
CREATE OR REPLACE FUNCTION ivymy.myblob_sub(ivymy.myblob, ivymy.myblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_sub'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.myblob_sub,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function blob_minus
CREATE OR REPLACE FUNCTION ivymy.myblob_minus(ivymy.myblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.myblob_minus,
	rightarg  = ivymy.myblob
);

----function blob_mul
CREATE OR REPLACE FUNCTION ivymy.myblob_mul(ivymy.myblob, ivymy.myblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_mul'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator *
CREATE OPERATOR ivymy.* (
	procedure = ivymy.myblob_mul,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

----function blob_mod
CREATE OR REPLACE FUNCTION ivymy.myblob_mod(ivymy.myblob, ivymy.myblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_mod'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator %
CREATE OPERATOR ivymy.% (
	procedure = ivymy.myblob_mod,
	leftarg   = ivymy.myblob,
	rightarg  = ivymy.myblob
);

/********************************************************************************
 *
 * CREATE MYBLOB OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
--reference from Oracle
CREATE OPERATOR FAMILY ivymy.myblob_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.myblob_minmax_ops
    DEFAULT FOR TYPE ivymy.myblob USING brin FAMILY ivymy.myblob_minmax_ops AS
        OPERATOR        1       < (ivymy.myblob, ivymy.myblob),
        OPERATOR        2       <= (ivymy.myblob, ivymy.myblob),
        OPERATOR        3       = (ivymy.myblob, ivymy.myblob),
        OPERATOR        4       >= (ivymy.myblob, ivymy.myblob),
        OPERATOR        5       > (ivymy.myblob, ivymy.myblob),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/*myblob_cmp gin and btree use*/
CREATE FUNCTION ivymy.myblob_cmp(ivymy.myblob, ivymy.myblob)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_myblob(ivymy.myblob, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_myblob(ivymy.myblob, ivymy.myblob, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_myblob(ivymy.myblob, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_myblob_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_myblob_ops
DEFAULT FOR TYPE ivymy.myblob USING gin
AS
	OPERATOR		1		< (ivymy.myblob, ivymy.myblob),
	OPERATOR		2		<= (ivymy.myblob, ivymy.myblob),
	OPERATOR		3		= (ivymy.myblob, ivymy.myblob),
	OPERATOR		4		>= (ivymy.myblob, ivymy.myblob),
	OPERATOR		5		> (ivymy.myblob, ivymy.myblob),
	FUNCTION		1		ivymy.myblob_cmp(ivymy.myblob, ivymy.myblob),
	FUNCTION		2		ivymy.gin_extract_value_myblob(ivymy.myblob, internal),
	FUNCTION		3		ivymy.gin_extract_query_myblob(ivymy.myblob, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_myblob(ivymy.myblob, ivymy.myblob, int2, internal),
STORAGE			ivymy.myblob;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.myblob_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME','myblob_sortsupport'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.myblob_ops USING btree;

CREATE OPERATOR CLASS ivymy.myblob_ops
	DEFAULT FOR TYPE ivymy.myblob USING btree FAMILY ivymy.myblob_ops AS
		OPERATOR		1		< (ivymy.myblob, ivymy.myblob),
		OPERATOR		2		<= (ivymy.myblob, ivymy.myblob),
		OPERATOR		3		= (ivymy.myblob, ivymy.myblob),
		OPERATOR		4		>= (ivymy.myblob, ivymy.myblob),
		OPERATOR		5		> (ivymy.myblob, ivymy.myblob),
		FUNCTION		1		ivymy.myblob_cmp(ivymy.myblob, ivymy.myblob),
		FUNCTION		2		ivymy.myblob_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.myblob_hash(ivymy.myblob)
	RETURNS integer
	AS 'MODULE_PATHNAME','anybinary_hash'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE FUNCTION ivymy.myblob_hash_extended(ivymy.myblob, bigint)
	RETURNS bigint
	AS 'MODULE_PATHNAME','anybinary_hash_extended'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE OPERATOR CLASS ivymy.myblob_ops
DEFAULT FOR TYPE ivymy.myblob USING hash AS
  OPERATOR 1 = (ivymy.myblob, ivymy.myblob),
  FUNCTION 1 ivymy.myblob_hash(ivymy.myblob),
  FUNCTION 2 ivymy.myblob_hash_extended(ivymy.myblob, bigint);

------------------------------------------------------------------------------------------------
-------------------------------------NEXT DATATYPE LINE-----------------------------------------
------------------------------------------------------------------------------------------------

/********************************************************************************
 *
 * TINYBLOB DATATYPE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.tinyblob_in (CSTRING)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.tinyblob_out (ivymy.tinyblob)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.tinyblob_recv (internal)
RETURNS ivymy.tinyblob
AS 'bytearecv'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.tinyblob_send (ivymy.tinyblob)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.tinyblob_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','tinyblob_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.tinyblob_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='tinyblob_in')
WHERE oid = 9232;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='tinyblob_out')
WHERE oid = 9232;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='tinyblob_recv')
WHERE oid = 9232;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='tinyblob_send')
WHERE oid = 9232;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='tinyblob_typmodin')
WHERE oid = 9232 or oid = 9233;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='tinyblob_typmodout')
WHERE oid = 9232 or oid = 9233;

/********************************************************************************
 *
 * TINYBLOB CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.tinyblob(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob_bool(ivymy.tinyblob, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(bytea, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(ivymy.binary, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(ivymy.varbinary, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(ivymy.myblob, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','tinyblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.tinyblob(ivymy.longblob, integer, boolean)
RETURNS ivymy.tinyblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.tinyblob AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.tinyblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.tinyblob AS pg_catalog.bool)
WITH FUNCTION ivymy.tinyblob_bool(ivymy.tinyblob, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.tinyblob AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.tinyblob)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.varbinary AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.myblob AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.myblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.mediumblob AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.mediumblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.longblob AS ivymy.tinyblob)
WITH FUNCTION ivymy.tinyblob(ivymy.longblob, integer, boolean)
AS ASSIGNMENT;

/********************************************************************************
 *
 * TINYBLOB OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.tinyblob_eq(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.tinyblob_ne(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.tinyblob_lt(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.tinyblob_gt(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.tinyblob_le(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.tinyblob_ge(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.tinyblob_seq(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.tinyblob_like(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.tinyblob_notlike(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.tinyblob_eq,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.tinyblob_ne,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.tinyblob_gt,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.tinyblob_lt,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.tinyblob_le,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.tinyblob_ge,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.tinyblob_seq,
	leftarg = ivymy.tinyblob,
	rightarg = ivymy.tinyblob
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.tinyblob_like,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.tinyblob_notlike,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob,
	negator = ~~
);

----function and
CREATE OR REPLACE FUNCTION ivymy.tinyblob_and(ivymy.tinyblob, ivymy.tinyblob)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.tinyblob_and,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);


----function not
CREATE OR REPLACE FUNCTION ivymy.tinyblob_not(ivymy.tinyblob)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.tinyblob_not,
	rightarg  = ivymy.tinyblob
);

----function bitand
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitand(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.tinyblob_bitand,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function bitor
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitor(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.tinyblob_bitor,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitxor(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.tinyblob_bitxor,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitneg(ivymy.tinyblob)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.tinyblob_bitneg,
	rightarg  = ivymy.tinyblob
);

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitshiftleft(ivymy.tinyblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.tinyblob_bitshiftleft,
	leftarg   = ivymy.tinyblob,
	rightarg  = integer
);

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.tinyblob_bitshiftright(ivymy.tinyblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.tinyblob_bitshiftright,
	leftarg   = ivymy.tinyblob,
	rightarg  = integer
);

----function tinyblob_plus
CREATE OR REPLACE FUNCTION ivymy.tinyblob_plus(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_plus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator +
CREATE OPERATOR ivymy.+ (
	procedure = ivymy.tinyblob_plus,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function tinyblob_sub
CREATE OR REPLACE FUNCTION ivymy.tinyblob_sub(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_sub'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.tinyblob_sub,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function tinyblob_minus
CREATE OR REPLACE FUNCTION ivymy.tinyblob_minus(ivymy.tinyblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.tinyblob_minus,
	rightarg  = ivymy.tinyblob
);

----function tinyblob_mul
CREATE OR REPLACE FUNCTION ivymy.tinyblob_mul(ivymy.tinyblob, ivymy.tinyblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_mul'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator *
CREATE OPERATOR ivymy.* (
	procedure = ivymy.tinyblob_mul,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

----function tinyblob_mod
CREATE OR REPLACE FUNCTION ivymy.tinyblob_mod(ivymy.tinyblob, ivymy.tinyblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_mod'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator %
CREATE OPERATOR ivymy.% (
	procedure = ivymy.tinyblob_mod,
	leftarg   = ivymy.tinyblob,
	rightarg  = ivymy.tinyblob
);

/********************************************************************************
 *
 * CREATE TINYBLOB OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
--reference from Oracle
CREATE OPERATOR FAMILY ivymy.tinyblob_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.tinyblob_minmax_ops
    DEFAULT FOR TYPE ivymy.tinyblob USING brin FAMILY ivymy.tinyblob_minmax_ops AS
        OPERATOR        1       < (ivymy.tinyblob, ivymy.tinyblob),
        OPERATOR        2       <= (ivymy.tinyblob, ivymy.tinyblob),
        OPERATOR        3       = (ivymy.tinyblob, ivymy.tinyblob),
        OPERATOR        4       >= (ivymy.tinyblob, ivymy.tinyblob),
        OPERATOR        5       > (ivymy.tinyblob, ivymy.tinyblob),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/*tinyblob_cmp gin and btree use*/
CREATE FUNCTION ivymy.tinyblob_cmp(ivymy.tinyblob, ivymy.tinyblob)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_tinyblob(ivymy.tinyblob, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_tinyblob(ivymy.tinyblob, ivymy.tinyblob, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_tinyblob(ivymy.tinyblob, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_tinyblob_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_tinyblob_ops
DEFAULT FOR TYPE ivymy.tinyblob USING gin
AS
	OPERATOR		1		< (ivymy.tinyblob, ivymy.tinyblob),
	OPERATOR		2		<= (ivymy.tinyblob, ivymy.tinyblob),
	OPERATOR		3		= (ivymy.tinyblob, ivymy.tinyblob),
	OPERATOR		4		>= (ivymy.tinyblob, ivymy.tinyblob),
	OPERATOR		5		> (ivymy.tinyblob, ivymy.tinyblob),
	FUNCTION		1		ivymy.tinyblob_cmp(ivymy.tinyblob, ivymy.tinyblob),
	FUNCTION		2		ivymy.gin_extract_value_tinyblob(ivymy.tinyblob, internal),
	FUNCTION		3		ivymy.gin_extract_query_tinyblob(ivymy.tinyblob, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_tinyblob(ivymy.tinyblob, ivymy.tinyblob, int2, internal),
STORAGE			ivymy.tinyblob;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.tinyblob_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME','tinyblob_sortsupport'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.tinyblob_ops USING btree;

CREATE OPERATOR CLASS ivymy.tinyblob_ops
	DEFAULT FOR TYPE ivymy.tinyblob USING btree FAMILY ivymy.tinyblob_ops AS
		OPERATOR		1		< (ivymy.tinyblob, ivymy.tinyblob),
		OPERATOR		2		<= (ivymy.tinyblob, ivymy.tinyblob),
		OPERATOR		3		= (ivymy.tinyblob, ivymy.tinyblob),
		OPERATOR		4		>= (ivymy.tinyblob, ivymy.tinyblob),
		OPERATOR		5		> (ivymy.tinyblob, ivymy.tinyblob),
		FUNCTION		1		ivymy.tinyblob_cmp(ivymy.tinyblob, ivymy.tinyblob),
		FUNCTION		2		ivymy.tinyblob_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.tinyblob_hash(ivymy.tinyblob)
	RETURNS integer
	AS 'MODULE_PATHNAME','anybinary_hash'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE FUNCTION ivymy.tinyblob_hash_extended(ivymy.tinyblob, bigint)
	RETURNS bigint
	AS 'MODULE_PATHNAME','anybinary_hash_extended'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE OPERATOR CLASS ivymy.tinyblob_ops
DEFAULT FOR TYPE ivymy.tinyblob USING hash AS
  OPERATOR 1 = (ivymy.tinyblob, ivymy.tinyblob),
  FUNCTION 1 ivymy.tinyblob_hash(ivymy.tinyblob),
  FUNCTION 2 ivymy.tinyblob_hash_extended(ivymy.tinyblob, bigint);

------------------------------------------------------------------------------------------------
-------------------------------------NEXT DATATYPE LINE-----------------------------------------
------------------------------------------------------------------------------------------------

/********************************************************************************
 *
 * MEDIUMBLOB DATATYPE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.mediumblob_in(CSTRING)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mediumblob_out(ivymy.mediumblob)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mediumblob_recv (internal)
RETURNS ivymy.mediumblob
AS 'bytearecv'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mediumblob_send (ivymy.mediumblob)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mediumblob_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','mediumblob_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mediumblob_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='mediumblob_in')
WHERE oid = 9236;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='mediumblob_out')
WHERE oid = 9236;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='mediumblob_recv')
WHERE oid = 9236;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='mediumblob_send')
WHERE oid = 9236;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='mediumblob_typmodin')
WHERE oid = 9236 or oid = 9237;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='mediumblob_typmodout')
WHERE oid = 9236 or oid = 9237;

/********************************************************************************
 *
 * MEDIUMBLOB CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.mediumblob(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob_bool(ivymy.mediumblob, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(bytea, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(ivymy.binary, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(ivymy.varbinary, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(ivymy.myblob, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.mediumblob(ivymy.longblob, integer, boolean)
RETURNS ivymy.mediumblob
AS 'MODULE_PATHNAME','mediumblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.mediumblob AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.mediumblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.mediumblob AS pg_catalog.bool)
WITH FUNCTION ivymy.mediumblob_bool(ivymy.mediumblob, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.mediumblob AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.mediumblob)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.varbinary AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.myblob AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.myblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.tinyblob AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.tinyblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.longblob AS ivymy.mediumblob)
WITH FUNCTION ivymy.mediumblob(ivymy.longblob, integer, boolean)
AS ASSIGNMENT;
/********************************************************************************
 *
 * MEDIUMBLOB OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.mediumblob_eq(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mediumblob_ne(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mediumblob_lt(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mediumblob_gt(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.mediumblob_le(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mediumblob_ge(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mediumblob_seq(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.mediumblob_like(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.mediumblob_notlike(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.mediumblob_eq,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.mediumblob_ne,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.mediumblob_gt,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.mediumblob_lt,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.mediumblob_le,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.mediumblob_ge,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.mediumblob_seq,
	leftarg = ivymy.mediumblob,
	rightarg = ivymy.mediumblob
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.mediumblob_like,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.mediumblob_notlike,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob,
	negator = ~~
);

----function and
CREATE OR REPLACE FUNCTION ivymy.mediumblob_and(ivymy.mediumblob, ivymy.mediumblob)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.mediumblob_and,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function not
CREATE OR REPLACE FUNCTION ivymy.mediumblob_not(ivymy.mediumblob)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.mediumblob_not,
	rightarg  = ivymy.mediumblob
);

----function bitand
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitand(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.mediumblob_bitand,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function bitor
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitor(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.mediumblob_bitor,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitxor(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.mediumblob_bitxor,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitneg(ivymy.mediumblob)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.mediumblob_bitneg,
	rightarg  = ivymy.mediumblob
);

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitshiftleft(ivymy.mediumblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.mediumblob_bitshiftleft,
	leftarg   = ivymy.mediumblob,
	rightarg  = integer
);

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.mediumblob_bitshiftright(ivymy.mediumblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.mediumblob_bitshiftright,
	leftarg   = ivymy.mediumblob,
	rightarg  = integer
);

----function mediumblob_plus
CREATE OR REPLACE FUNCTION ivymy.mediumblob_plus(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_plus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator +
CREATE OPERATOR ivymy.+ (
	procedure = ivymy.mediumblob_plus,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function mediumblob_sub
CREATE OR REPLACE FUNCTION ivymy.mediumblob_sub(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_sub'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.mediumblob_sub,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function mediumblob_minus
CREATE OR REPLACE FUNCTION ivymy.mediumblob_minus(ivymy.mediumblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.mediumblob_minus,
	rightarg  = ivymy.mediumblob
);

----function mediumblob_mul
CREATE OR REPLACE FUNCTION ivymy.mediumblob_mul(ivymy.mediumblob, ivymy.mediumblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_mul'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator *
CREATE OPERATOR ivymy.* (
	procedure = ivymy.mediumblob_mul,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

----function mediumblob_mod
CREATE OR REPLACE FUNCTION ivymy.mediumblob_mod(ivymy.mediumblob, ivymy.mediumblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_mod'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator %
CREATE OPERATOR ivymy.% (
	procedure = ivymy.mediumblob_mod,
	leftarg   = ivymy.mediumblob,
	rightarg  = ivymy.mediumblob
);

/********************************************************************************
 *
 * CREATE MEDIUMBLOB OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
--reference from Oracle
CREATE OPERATOR FAMILY ivymy.mediumblob_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.mediumblob_minmax_ops
    DEFAULT FOR TYPE ivymy.mediumblob USING brin FAMILY ivymy.mediumblob_minmax_ops AS
        OPERATOR        1       < (ivymy.mediumblob, ivymy.mediumblob),
        OPERATOR        2       <= (ivymy.mediumblob, ivymy.mediumblob),
        OPERATOR        3       = (ivymy.mediumblob, ivymy.mediumblob),
        OPERATOR        4       >= (ivymy.mediumblob, ivymy.mediumblob),
        OPERATOR        5       > (ivymy.mediumblob, ivymy.mediumblob),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/*mediumblob_cmp gin and btree use*/
CREATE FUNCTION ivymy.mediumblob_cmp(ivymy.mediumblob, ivymy.mediumblob)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_mediumblob(ivymy.mediumblob, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mediumblob(ivymy.mediumblob, ivymy.mediumblob, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mediumblob(ivymy.mediumblob, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_mediumblob_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_mediumblob_ops
DEFAULT FOR TYPE ivymy.mediumblob USING gin
AS
	OPERATOR		1		< (ivymy.mediumblob, ivymy.mediumblob),
	OPERATOR		2		<= (ivymy.mediumblob, ivymy.mediumblob),
	OPERATOR		3		= (ivymy.mediumblob, ivymy.mediumblob),
	OPERATOR		4		>= (ivymy.mediumblob, ivymy.mediumblob),
	OPERATOR		5		> (ivymy.mediumblob, ivymy.mediumblob),
	FUNCTION		1		ivymy.mediumblob_cmp(ivymy.mediumblob, ivymy.mediumblob),
	FUNCTION		2		ivymy.gin_extract_value_mediumblob(ivymy.mediumblob, internal),
	FUNCTION		3		ivymy.gin_extract_query_mediumblob(ivymy.mediumblob, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mediumblob(ivymy.mediumblob, ivymy.mediumblob, int2, internal),
STORAGE			ivymy.mediumblob;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.mediumblob_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME','mediumblob_sortsupport'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.mediumblob_ops USING btree;

CREATE OPERATOR CLASS ivymy.mediumblob_ops
	DEFAULT FOR TYPE ivymy.mediumblob USING btree FAMILY ivymy.mediumblob_ops AS
		OPERATOR		1		< (ivymy.mediumblob, ivymy.mediumblob),
		OPERATOR		2		<= (ivymy.mediumblob, ivymy.mediumblob),
		OPERATOR		3		= (ivymy.mediumblob, ivymy.mediumblob),
		OPERATOR		4		>= (ivymy.mediumblob, ivymy.mediumblob),
		OPERATOR		5		> (ivymy.mediumblob, ivymy.mediumblob),
		FUNCTION		1		ivymy.mediumblob_cmp(ivymy.mediumblob, ivymy.mediumblob),
		FUNCTION		2		ivymy.mediumblob_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mediumblob_hash(ivymy.mediumblob)
	RETURNS integer
	AS 'MODULE_PATHNAME','anybinary_hash'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE FUNCTION ivymy.mediumblob_hash_extended(ivymy.mediumblob, bigint)
	RETURNS bigint
	AS 'MODULE_PATHNAME','anybinary_hash_extended'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE OPERATOR CLASS ivymy.mediumblob_ops
DEFAULT FOR TYPE ivymy.mediumblob USING hash AS
  OPERATOR 1 = (ivymy.mediumblob, ivymy.mediumblob),
  FUNCTION 1 ivymy.mediumblob_hash(ivymy.mediumblob),
  FUNCTION 2 ivymy.mediumblob_hash_extended(ivymy.mediumblob, bigint);

------------------------------------------------------------------------------------------------
-------------------------------------NEXT DATATYPE LINE-----------------------------------------
------------------------------------------------------------------------------------------------

/********************************************************************************
 *
 * LONGBLOB DATATYPE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.longblob_in (CSTRING)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.longblob_out (ivymy.longblob)
RETURNS cstring
AS 'byteaout'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.longblob_recv (internal)
RETURNS ivymy.longblob
AS 'bytearecv'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.longblob_send (ivymy.longblob)
RETURNS bytea
AS 'byteasend'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.longblob_typmodin(CSTRING[])
RETURNS int4
AS 'MODULE_PATHNAME','longblob_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.longblob_typmodout(int4)
RETURNS cstring
AS 'MODULE_PATHNAME','binary_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput = (SELECT oid FROM pg_proc WHERE proname='longblob_in')
WHERE oid = 9238;
UPDATE pg_type
SET typoutput = (SELECT oid FROM pg_proc WHERE proname='longblob_out')
WHERE oid = 9238;

UPDATE pg_type
SET typreceive = (SELECT oid FROM pg_proc WHERE proname='longblob_recv')
WHERE oid = 9238;
UPDATE pg_type
SET typsend = (SELECT oid FROM pg_proc WHERE proname='longblob_send')
WHERE oid = 9238;

UPDATE pg_type
SET typmodin = (SELECT oid FROM pg_proc WHERE proname='longblob_typmodin')
WHERE oid = 9238 or oid = 9239;
UPDATE pg_type
SET typmodout = (SELECT oid FROM pg_proc WHERE proname='longblob_typmodout')
WHERE oid = 9238 or oid = 9239;

/********************************************************************************
 *
 * LONGBLOB CAST
 * REFERENCE FROM ORACLE
 *
 ********************************************************************************/
CREATE FUNCTION ivymy.longblob(ivymy.longblob, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob_bool(ivymy.longblob, integer, boolean, int8)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','binary_bool'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(bytea, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(ivymy.binary, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(ivymy.varbinary, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(ivymy.myblob, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(ivymy.tinyblob, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE FUNCTION ivymy.longblob(ivymy.mediumblob, integer, boolean)
RETURNS ivymy.longblob
AS 'MODULE_PATHNAME','longblob'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

---------- Compatible Mysql Cast -----------------------
----reference from oracle
CREATE CAST (ivymy.longblob AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.longblob, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.longblob AS pg_catalog.bool)
WITH FUNCTION ivymy.longblob_bool(ivymy.longblob, integer, boolean, int8)
AS IMPLICIT;

CREATE CAST (ivymy.longblob AS bytea)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE CAST (pg_catalog.text AS ivymy.longblob)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (bytea AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(bytea, integer, boolean)
AS IMPLICIT;

CREATE CAST (ivymy.binary AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.varbinary AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.varbinary, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.myblob AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.myblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.tinyblob AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.tinyblob, integer, boolean)
AS ASSIGNMENT;

CREATE CAST (ivymy.mediumblob AS ivymy.longblob)
WITH FUNCTION ivymy.longblob(ivymy.mediumblob, integer, boolean)
AS ASSIGNMENT;

/********************************************************************************
 *
 * LONGBLOB OPERATOR
 *
 ********************************************************************************/
----function eq
CREATE FUNCTION ivymy.longblob_eq(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.longblob_ne(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.longblob_lt(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_lt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.longblob_gt(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_gt'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function le
CREATE FUNCTION ivymy.longblob_le(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_le'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.longblob_ge(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_ge'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.longblob_seq(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'MODULE_PATHNAME','anybinary_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function like
CREATE OR REPLACE FUNCTION ivymy.longblob_like(ivymy.longblob, ivymy.longblob)
RETURNS boolean AS $$
SELECT $1::bytea like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----function notlike
CREATE OR REPLACE FUNCTION ivymy.longblob_notlike(ivymy.longblob, ivymy.longblob)
RETURNS boolean AS $$
SELECT $1::bytea not like $2::bytea;
$$ LANGUAGE SQL IMMUTABLE STRICT;

----operator =
CREATE OPERATOR ivymy.= (
	procedure = ivymy.longblob_eq,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <> and !=
CREATE OPERATOR ivymy.<> (
	procedure = ivymy.longblob_ne,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = !=,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

----operator >
CREATE OPERATOR ivymy.> (
	procedure = ivymy.longblob_gt,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR ivymy.< (
	procedure = ivymy.longblob_lt,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator <=
CREATE OPERATOR ivymy.<= (
	procedure = ivymy.longblob_le,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

----operator >=
CREATE OPERATOR ivymy.>= (
	procedure = ivymy.longblob_ge,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <=>
CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.longblob_seq,
	leftarg = ivymy.longblob,
	rightarg = ivymy.longblob
);

----operator ~~
CREATE OPERATOR ivymy.~~ (
	procedure = ivymy.longblob_like,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob,
	negator = !~~
);

----operator !~~
CREATE OPERATOR ivymy.!~~ (
	procedure = ivymy.longblob_notlike,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob,
	negator = ~~
);

----function and
CREATE OR REPLACE FUNCTION ivymy.longblob_and(ivymy.longblob, ivymy.longblob)
RETURNS boolean
AS 'select $1::double and $2::double'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &&
CREATE OPERATOR ivymy.&& (
	procedure = ivymy.longblob_and,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function not
CREATE OR REPLACE FUNCTION ivymy.longblob_not(ivymy.longblob)
RETURNS boolean
AS 'select !($1::double)'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator !
CREATE OPERATOR ivymy.! (
	procedure = ivymy.longblob_not,
	rightarg  = ivymy.longblob
);

----function bitand
CREATE OR REPLACE FUNCTION ivymy.longblob_bitand(ivymy.longblob, ivymy.longblob)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitand'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator &
CREATE OPERATOR ivymy.& (
	procedure = ivymy.longblob_bitand,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function bitor
CREATE OR REPLACE FUNCTION ivymy.longblob_bitor(ivymy.longblob, ivymy.longblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator |
CREATE OPERATOR ivymy.| (
	procedure = ivymy.longblob_bitor,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function bitxor
CREATE OR REPLACE FUNCTION ivymy.longblob_bitxor(ivymy.longblob, ivymy.longblob)
RETURNS bigint
----uint8
AS 'MODULE_PATHNAME','varbinary_bitxor'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ^
CREATE OPERATOR ivymy.^ (
	procedure = ivymy.longblob_bitxor,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function bitneg
CREATE OR REPLACE FUNCTION ivymy.longblob_bitneg(ivymy.longblob)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','varbinary_bitneg'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator ~
CREATE OPERATOR ivymy.~ (
	procedure = ivymy.longblob_bitneg,
	rightarg  = ivymy.longblob
);

----function bitshiftleft
CREATE OR REPLACE FUNCTION ivymy.longblob_bitshiftleft(ivymy.longblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftleft'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator <<
CREATE OPERATOR ivymy.<< (
	procedure = ivymy.longblob_bitshiftleft,
	leftarg   = ivymy.longblob,
	rightarg  = integer
);

----function bitshiftright
CREATE OR REPLACE FUNCTION ivymy.longblob_bitshiftright(ivymy.longblob, integer)
RETURNS bigint
AS 'MODULE_PATHNAME','varbinary_bitshiftright'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator >>
CREATE OPERATOR ivymy.>> (
	procedure = ivymy.longblob_bitshiftright,
	leftarg   = ivymy.longblob,
	rightarg  = integer
);

----function longblob_plus
CREATE OR REPLACE FUNCTION ivymy.longblob_plus(ivymy.longblob, ivymy.longblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_plus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator +
CREATE OPERATOR ivymy.+ (
	procedure = ivymy.longblob_plus,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function longblob_sub
CREATE OR REPLACE FUNCTION ivymy.longblob_sub(ivymy.longblob, ivymy.longblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_sub'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.longblob_sub,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function longblob_minus
CREATE OR REPLACE FUNCTION ivymy.longblob_minus(ivymy.longblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.longblob_minus,
	rightarg  = ivymy.longblob
);

/* Begin - bug#z6052 */
CREATE OR REPLACE FUNCTION ivymy.varbinary_minus(ivymy.varbinary)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_minus'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator -
CREATE OPERATOR ivymy.- (
	procedure = ivymy.varbinary_minus,
	rightarg  = ivymy.varbinary
);
/* End - bug#z6052 */

----function longblob_mul
CREATE OR REPLACE FUNCTION ivymy.longblob_mul(ivymy.longblob, ivymy.longblob)
RETURNS bigint
AS 'MODULE_PATHNAME','anybinary_mul'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator *
CREATE OPERATOR ivymy.* (
	procedure = ivymy.longblob_mul,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

----function longblob_mod
CREATE OR REPLACE FUNCTION ivymy.longblob_mod(ivymy.longblob, ivymy.longblob)
RETURNS float8
AS 'MODULE_PATHNAME','anybinary_mod'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----operator %
CREATE OPERATOR ivymy.% (
	procedure = ivymy.longblob_mod,
	leftarg   = ivymy.longblob,
	rightarg  = ivymy.longblob
);

/********************************************************************************
 *
 * CREATE LONGBLOB OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
--reference from Oracle
CREATE OPERATOR FAMILY ivymy.longblob_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.longblob_minmax_ops
    DEFAULT FOR TYPE ivymy.longblob USING brin FAMILY ivymy.longblob_minmax_ops AS
        OPERATOR        1       < (ivymy.longblob, ivymy.longblob),
        OPERATOR        2       <= (ivymy.longblob, ivymy.longblob),
        OPERATOR        3       = (ivymy.longblob, ivymy.longblob),
        OPERATOR        4       >= (ivymy.longblob, ivymy.longblob),
        OPERATOR        5       > (ivymy.longblob, ivymy.longblob),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);

/*longblob_cmp gin and btree use*/
CREATE FUNCTION ivymy.longblob_cmp(ivymy.longblob, ivymy.longblob)
RETURNS integer
AS 'MODULE_PATHNAME','anybinary_cmp'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

/* GIN index support procedure */
CREATE FUNCTION ivymy.gin_extract_value_longblob(ivymy.longblob, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_longblob(ivymy.longblob, ivymy.longblob, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_longblob(ivymy.longblob, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.gin_longblob_ops USING gin;

CREATE OPERATOR CLASS ivymy.gin_longblob_ops
DEFAULT FOR TYPE ivymy.longblob USING gin
AS
	OPERATOR		1		< (ivymy.longblob, ivymy.longblob),
	OPERATOR		2		<= (ivymy.longblob, ivymy.longblob),
	OPERATOR		3		= (ivymy.longblob, ivymy.longblob),
	OPERATOR		4		>= (ivymy.longblob, ivymy.longblob),
	OPERATOR		5		> (ivymy.longblob, ivymy.longblob),
	FUNCTION		1		ivymy.longblob_cmp(ivymy.longblob, ivymy.longblob),
	FUNCTION		2		ivymy.gin_extract_value_longblob(ivymy.longblob, internal),
	FUNCTION		3		ivymy.gin_extract_query_longblob(ivymy.longblob, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent_binary(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_longblob(ivymy.longblob, ivymy.longblob, int2, internal),
STORAGE			ivymy.longblob;

/* B-tree index support procedure */
--reference from date
CREATE FUNCTION ivymy.longblob_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME','longblob_sortsupport'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.longblob_ops USING btree;

CREATE OPERATOR CLASS ivymy.longblob_ops
	DEFAULT FOR TYPE ivymy.longblob USING btree FAMILY ivymy.longblob_ops AS
		OPERATOR		1		< (ivymy.longblob, ivymy.longblob),
		OPERATOR		2		<= (ivymy.longblob, ivymy.longblob),
		OPERATOR		3		= (ivymy.longblob, ivymy.longblob),
		OPERATOR		4		>= (ivymy.longblob, ivymy.longblob),
		OPERATOR		5		> (ivymy.longblob, ivymy.longblob),
		FUNCTION		1		ivymy.longblob_cmp(ivymy.longblob, ivymy.longblob),
		FUNCTION		2		ivymy.longblob_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.longblob_hash(ivymy.longblob)
	RETURNS integer
	AS 'MODULE_PATHNAME','anybinary_hash'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE FUNCTION ivymy.longblob_hash_extended(ivymy.longblob, bigint)
	RETURNS bigint
	AS 'MODULE_PATHNAME','anybinary_hash_extended'
	LANGUAGE C
	PARALLEL SAFE
	IMMUTABLE STRICT;

CREATE OPERATOR CLASS ivymy.longblob_ops
DEFAULT FOR TYPE ivymy.longblob USING hash AS
  OPERATOR 1 = (ivymy.longblob, ivymy.longblob),
  FUNCTION 1 ivymy.longblob_hash(ivymy.longblob),
  FUNCTION 2 ivymy.longblob_hash_extended(ivymy.longblob, bigint);

-- cast binary to numeric
CREATE FUNCTION ivymy.binary_numeric(ivymy.binary, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.binary AS numeric)
WITH FUNCTION ivymy.binary_numeric(ivymy.binary, integer, boolean)
AS ASSIGNMENT;

-- cast blob to numeric
CREATE FUNCTION ivymy.blob_numeric(ivymy.myblob, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.myblob AS numeric)
WITH FUNCTION ivymy.blob_numeric(ivymy.myblob, integer, boolean)
AS ASSIGNMENT;

-- cast tinyblob to numeric
CREATE FUNCTION ivymy.tinyblob_numeric(ivymy.tinyblob, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.tinyblob AS numeric)
WITH FUNCTION ivymy.tinyblob_numeric(ivymy.tinyblob, integer, boolean)
AS ASSIGNMENT;

-- cast mediumblob to numeric
CREATE FUNCTION ivymy.mediumblob_numeric(ivymy.mediumblob, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.mediumblob AS numeric)
WITH FUNCTION ivymy.mediumblob_numeric(ivymy.mediumblob, integer, boolean)
AS ASSIGNMENT;

-- cast longblob to numeric
CREATE FUNCTION ivymy.longblob_numeric(ivymy.longblob, integer, boolean)
RETURNS numeric
AS 'MODULE_PATHNAME','varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.longblob AS numeric)
WITH FUNCTION ivymy.longblob_numeric(ivymy.longblob, integer, boolean)
AS ASSIGNMENT;

/* Begin - bug#z5800 */
-- cast int to binary
CREATE FUNCTION ivymy.int_binary(int)
RETURNS ivymy.binary
AS 'select $1::text::binary;'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (int AS ivymy.binary)
WITH FUNCTION ivymy.int_binary(int)
AS ASSIGNMENT;

-- cast numeric to binary
CREATE FUNCTION ivymy.numeric_binary(numeric)
RETURNS ivymy.binary
AS 'select $1::text::binary;'
LANGUAGE SQL
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (numeric AS ivymy.binary)
WITH FUNCTION ivymy.numeric_binary(numeric)
AS ASSIGNMENT;
/* End - bug#z5800 */

-- cast binary to double
CREATE FUNCTION ivymy.binary_double(ivymy.binary, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.binary AS double)
WITH FUNCTION ivymy.binary_double(ivymy.binary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast varbinary to double
CREATE FUNCTION ivymy.varbinary_double(ivymy.varbinary, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.varbinary AS double)
WITH FUNCTION ivymy.varbinary_double(ivymy.varbinary, integer, boolean, int8)
AS ASSIGNMENT;

-- cast myblob to double
CREATE FUNCTION ivymy.myblob_double(ivymy.myblob, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.myblob AS double)
WITH FUNCTION ivymy.myblob_double(ivymy.myblob, integer, boolean, int8)
AS ASSIGNMENT;

-- cast tinyblob to double
CREATE FUNCTION ivymy.tinyblob_double(ivymy.tinyblob, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.tinyblob AS double)
WITH FUNCTION ivymy.tinyblob_double(ivymy.tinyblob, integer, boolean, int8)
AS ASSIGNMENT;

-- cast mediumblob to double
CREATE FUNCTION ivymy.mediumblob_double(ivymy.mediumblob, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.mediumblob AS double)
WITH FUNCTION ivymy.mediumblob_double(ivymy.mediumblob, integer, boolean, int8)
AS ASSIGNMENT;

-- cast longblob to double
CREATE FUNCTION ivymy.longblob_double(ivymy.longblob, integer, boolean, int8)
RETURNS double
AS 'MODULE_PATHNAME','varlena_float8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (ivymy.longblob AS double)
WITH FUNCTION ivymy.longblob_double(ivymy.longblob, integer, boolean, int8)
AS ASSIGNMENT;

/* Begin - bug#6051 */
CREATE FUNCTION binary_smaller(binary, binary)
RETURNS binary
AS 'MODULE_PATHNAME', 'anybinary_smaller'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE AGGREGATE min(binary)  (
    SFUNC = binary_smaller,
    STYPE = binary,
    SORTOP = <,
    PARALLEL = SAFE,
    COMBINEFUNC = binary_smaller
);

CREATE FUNCTION binary_larger(binary, binary)
RETURNS binary
AS 'MODULE_PATHNAME', 'anybinary_larger'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE AGGREGATE max(binary)  (
    SFUNC = binary_larger,
    STYPE = binary,
    SORTOP = >,
    PARALLEL = SAFE,
    COMBINEFUNC = binary_larger
);

CREATE FUNCTION ivymy.binary_sum(float8, binary)
RETURNS float8
AS 'MODULE_PATHNAME', 'anybinary_sum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE AGGREGATE ivymy.sum(binary) (
  SFUNC = ivymy.binary_sum,
  STYPE = float8
);

CREATE FUNCTION ivymy.binary_avg_accum(float8[], binary)
RETURNS float8[]
AS 'MODULE_PATHNAME', 'binary_avg_accum'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE AGGREGATE ivymy.avg(binary) (
   sfunc = binary_avg_accum, 
   stype = float8[],
   finalfunc = float8_avg,
   initcond = '{0,0,0}'
);
/* End - bug#6051 */