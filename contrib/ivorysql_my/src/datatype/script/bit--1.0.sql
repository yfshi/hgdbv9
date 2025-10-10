/***************************************************************
 *
 * ReqID:SRS-MYSQL-BITTYPE
 * bit type support
 *
 ***************************************************************/

-- datatype bit
-- ALTER DOMAIN ivymy.mysbit ADD CONSTRAINT mysbit_check CHECK(length(VALUE) <= 64);
CREATE FUNCTION ivymy.mysbit_in(pg_catalog.cstring, oid, int, int8)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','mysql_bit_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysbit_out(ivymy.mysbit)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysql_bit_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysbit_recv(internal, oid, int, int8)
RETURNS ivymy.mysbit
AS 'bit_recv'
LANGUAGE INTERNAL
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mysbit_send(ivymy.mysbit)
RETURNS bytea
AS 'bit_send'
LANGUAGE INTERNAL
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mysbit_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'bittypmodin'
LANGUAGE INTERNAL
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE FUNCTION ivymy.mysbit_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'bittypmodout'
LANGUAGE INTERNAL
PARALLEL SAFE
IMMUTABLE
STRICT;

-- Register the basic I/O for 'mysbit' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysbit_in')
WHERE oid = 9220;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysbit_out')
WHERE oid = 9220;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysbit_recv')
WHERE oid = 9220;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysbit_send')
WHERE oid = 9220;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysbit_typmodin')
WHERE oid = 9220;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysbit_typmodout')
WHERE oid = 9220;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysbit_typmodin')
WHERE oid = 9221;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysbit_typmodout')
WHERE oid = 9221;

-- FOR COPY FROM IN
CREATE FUNCTION ivymy.mysbit_in2(pg_catalog.cstring, pg_catalog.oid, pg_catalog.int4)
RETURNS ivymy.mysbit_2
AS 'MODULE_PATHNAME','mysql_bit_in2'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysbit_in2')
WHERE oid = 9222;

CREATE FUNCTION ivymy.mysbit_out2(ivymy.mysbit_2)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysql_bit_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysbit_out2')
WHERE oid = 9222;

--
-- CREATE CAST
--
--pgbit to mysbit
CREATE FUNCTION ivymy.pgbit_mysbit(pg_catalog.bit, pg_catalog.int4)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','mysbitfrompgbit'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.bit AS ivymy.mysbit)
WITH FUNCTION ivymy.pgbit_mysbit(pg_catalog.bit, pg_catalog.int4)
AS IMPLICIT;

--bool to mysbit
CREATE FUNCTION ivymy.bool_mysbit(pg_catalog.bool, pg_catalog.int4)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','bitfrombool'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.bool AS ivymy.mysbit)
WITH FUNCTION ivymy.bool_mysbit(pg_catalog.bool, pg_catalog.int4)
AS ASSIGNMENT;

--uint8 to mysbit
CREATE FUNCTION ivymy.uint8_mysbit(ivymy.uint8, pg_catalog.int4)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','bitfromuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (ivymy.uint8 AS ivymy.mysbit)
WITH FUNCTION ivymy.uint8_mysbit(ivymy.uint8, pg_catalog.int4)
AS ASSIGNMENT;

--int4 to mysbit
CREATE FUNCTION ivymy.int4_mysbit(pg_catalog.int4, pg_catalog.int4)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','bitfromuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysbit)
WITH FUNCTION ivymy.int4_mysbit(pg_catalog.int4, pg_catalog.int4)
AS ASSIGNMENT;

--int8 to mysbit
CREATE FUNCTION ivymy.int8_mysbit(pg_catalog.int8, pg_catalog.int4)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','bitfromuint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (pg_catalog.int8 AS ivymy.mysbit)
WITH FUNCTION ivymy.int8_mysbit(pg_catalog.int8, pg_catalog.int4)
AS ASSIGNMENT;

--varbinary to mysbit
CREATE FUNCTION ivymy.varbinary_mysbit(ivymy.varbinary, pg_catalog.int4, bool, int8)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','bitfromvarbinary'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (ivymy.varbinary AS ivymy.mysbit)
WITH FUNCTION ivymy.varbinary_mysbit(ivymy.varbinary, pg_catalog.int4, bool, int8)
AS ASSIGNMENT;

--mysbit to pgbit
CREATE FUNCTION ivymy.mysbit_pgbit(ivymy.mysbit, pg_catalog.int4)
RETURNS pg_catalog.bit
AS 'bit'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE CAST (ivymy.mysbit AS pg_catalog.bit)
WITH FUNCTION ivymy.mysbit_pgbit(ivymy.mysbit, pg_catalog.int4)
AS ASSIGNMENT;

--mysbit to int8
CREATE FUNCTION ivymy.mysbit_int8(ivymy.mysbit, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'bittoint8'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE CAST (ivymy.mysbit AS pg_catalog.int8)
WITH FUNCTION ivymy.mysbit_int8(ivymy.mysbit, pg_catalog.int4)
AS ASSIGNMENT;

--mysbit to bool
CREATE FUNCTION ivymy.mysbit_bool(ivymy.mysbit, pg_catalog.int4)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','bittobool'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (ivymy.mysbit AS pg_catalog.bool)
WITH FUNCTION ivymy.mysbit_bool(ivymy.mysbit, pg_catalog.int4)
AS ASSIGNMENT;

--mysbit to uint8
CREATE FUNCTION ivymy.mysbit_uint8(ivymy.mysbit, pg_catalog.int4)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','bittouint8'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (ivymy.mysbit AS ivymy.uint8)
WITH FUNCTION ivymy.mysbit_uint8(ivymy.mysbit, pg_catalog.int4)
AS IMPLICIT;

--mysbit to text
CREATE FUNCTION ivymy.mysbit_text(ivymy.mysbit, pg_catalog.int4)
RETURNS text
AS 'MODULE_PATHNAME','bittotext'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
STRICT;

CREATE CAST (ivymy.mysbit AS text)
WITH FUNCTION ivymy.mysbit_text(ivymy.mysbit, pg_catalog.int4)
AS ASSIGNMENT;

/* Begin - bug#z5805 */
--pg bit to uint8
CREATE FUNCTION ivymy.bit_uint8(pg_catalog.bit, int4, bool, int8)
RETURNS uint8
AS 'MODULE_PATHNAME','bit_uint8'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE CAST (pg_catalog.bit AS uint8)
WITH FUNCTION ivymy.bit_uint8(pg_catalog.bit, int4, bool, int8)
AS ASSIGNMENT;
/* End - bug#z5805 */

/* Begin - bug#z5768 */
CREATE FUNCTION ivymy.mysbit_varbinary(ivymy.mysbit, int)
RETURNS ivymy.varbinary
AS 'MODULE_PATHNAME','bittobinary'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysbit AS varbinary)
WITH FUNCTION ivymy.mysbit_varbinary(ivymy.mysbit, int4)
AS ASSIGNMENT;
/* End - bug#z5768 */

--
-- CREATE OPERATOR
--
-- "<"
-- index need
CREATE FUNCTION ivymy.mysbit_lt(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_lt'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR < (
	procedure = ivymy.mysbit_lt,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

-- "<="
-- index need
CREATE FUNCTION ivymy.mysbit_le(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_le'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR <= (
	procedure = ivymy.mysbit_le,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = >=,
	negator = <,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

-- "="
-- index need
CREATE FUNCTION ivymy.mysbit_eq(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_eq'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR = (
	procedure = ivymy.mysbit_eq,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel
);

-- "<>"
-- index need
CREATE FUNCTION ivymy.mysbit_ne(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_ne'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR <> (
	procedure = ivymy.mysbit_ne,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

-- ">="
-- index need
CREATE FUNCTION ivymy.mysbit_ge(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_ge'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR >= (
	procedure = ivymy.mysbit_ge,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = =,
	negator = <=,
	restrict = scalargesel,
	join = scalargejoinsel
);

-- ">"
-- index need
CREATE FUNCTION ivymy.mysbit_gt(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bool
AS 'MODULE_PATHNAME','mysbit_gt'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OPERATOR > (
	procedure = ivymy.mysbit_gt,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--  "<=>"
CREATE FUNCTION ivymy.mysbit_safeeq(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysbit_safeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR ivymy.<=> (
	procedure = ivymy.mysbit_safeeq,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit
);

--  "&"
--  aggregate function bit_and need
CREATE FUNCTION ivymy.mysbit_bit_and(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysbit_bit_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.& (
	procedure = ivymy.mysbit_bit_and,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = &
);


--  "^"
CREATE FUNCTION ivymy.mysbit_bit_xor(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysbit_bit_xor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.^ (
	procedure = ivymy.mysbit_bit_xor,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = ^
);

--  "#"
--  aggregate function bit_xor need
CREATE FUNCTION ivymy.mysbit_bit_pgxor(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.bit
AS 'bitxor'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

CREATE OPERATOR ivymy.# (
	procedure = ivymy.mysbit_bit_pgxor,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = #
);

--  "|"
--  aggregate function bit_or need
CREATE FUNCTION ivymy.mysbit_bit_or(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','mysbit_bit_or'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.| (
	procedure = ivymy.mysbit_bit_or,
	leftarg = ivymy.mysbit,
	rightarg = ivymy.mysbit,
	commutator = |
);

--"sign +"
CREATE FUNCTION ivymy.mysbit_positive(ivymy.mysbit)
RETURNS ivymy.mysbit
AS 'MODULE_PATHNAME','mysbit_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.mysbit_positive,
	rightarg = ivymy.mysbit
);


--
-- CREATE AGGREGATE 
--
/* bit_and */
CREATE FUNCTION ivymy.bit_and(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.mysbit
AS 'bit_and'
LANGUAGE INTERNAL
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.bit_and(ivymy.mysbit) 
(
  SFUNC = ivymy.bit_and,
  STYPE = ivymy.mysbit,
  SORTOP = &,
  PARALLEL = SAFE
);

/* bit_or */
CREATE FUNCTION ivymy.bit_or(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.mysbit
AS 'bit_or'
LANGUAGE INTERNAL
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.bit_or(ivymy.mysbit) 
(
  SFUNC = ivymy.bit_or,
  STYPE = ivymy.mysbit,
  SORTOP = |,
  PARALLEL = SAFE
);

/* bit_xor */
CREATE FUNCTION ivymy.bit_xor(ivymy.mysbit, ivymy.mysbit)
RETURNS ivymy.mysbit
AS 'bitxor'
LANGUAGE INTERNAL
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.bit_xor(ivymy.mysbit) 
(
  SFUNC = ivymy.bit_xor,
  STYPE = ivymy.mysbit,
  SORTOP = #,
  PARALLEL = SAFE
);

/********************************************************************************
 *
 * CREATE BIT OPERATOR FAMILY/CLASS AND INDEX
 *
 ********************************************************************************/
/* BRIN index support procedure */
CREATE OPERATOR FAMILY ivymy.mysbit_minmax_ops USING brin;

CREATE OPERATOR CLASS ivymy.mysbit_minmax_ops
    DEFAULT FOR TYPE ivymy.mysbit USING brin FAMILY ivymy.mysbit_minmax_ops AS
        OPERATOR        1       < (ivymy.mysbit, ivymy.mysbit),
        OPERATOR        2       <= (ivymy.mysbit, ivymy.mysbit),
        OPERATOR        3       = (ivymy.mysbit, ivymy.mysbit),
        OPERATOR        4       >= (ivymy.mysbit, ivymy.mysbit),
        OPERATOR        5       > (ivymy.mysbit, ivymy.mysbit),
        FUNCTION        1       ivymy.brin_minmax_opcinfo(internal),
		FUNCTION        2       ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION        3       ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION        4       ivymy.brin_minmax_union(internal, internal, internal);


/*bit_cmp hash and btree use*/
CREATE FUNCTION ivymy.bit_cmp(ivymy.mysbit, ivymy.mysbit)
RETURNS pg_catalog.int4
AS 'bitcmp'
LANGUAGE INTERNAL
IMMUTABLE
STRICT;

/* B-tree index support procedure */
CREATE FUNCTION ivymy.btmysequalimage(oid)
RETURNS pg_catalog.bool
AS 'btequalimage'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.bit_ops USING btree;

CREATE OPERATOR CLASS ivymy.bit_ops
	DEFAULT FOR TYPE ivymy.mysbit USING btree FAMILY ivymy.bit_ops AS
		OPERATOR		1		< (ivymy.mysbit, ivymy.mysbit),
		OPERATOR		2		<= (ivymy.mysbit, ivymy.mysbit),
		OPERATOR		3		= (ivymy.mysbit, ivymy.mysbit),
		OPERATOR		4		>= (ivymy.mysbit, ivymy.mysbit),
		OPERATOR		5		> (ivymy.mysbit, ivymy.mysbit),
		FUNCTION		1		ivymy.bit_cmp(ivymy.mysbit, ivymy.mysbit),
		FUNCTION		4		ivymy.btmysequalimage(oid);

/* hash index support procedure */
CREATE FUNCTION ivymy.mysbit_hash(ivymy.mysbit)
RETURNS integer
AS 'MODULE_PATHNAME','mysbit_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysbit_hash_extended(ivymy.mysbit, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysbit_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysbit_ops
	DEFAULT FOR TYPE ivymy.mysbit USING hash AS
		OPERATOR		1		= (ivymy.mysbit, ivymy.mysbit),
		FUNCTION		1		ivymy.mysbit_hash(ivymy.mysbit),
		FUNCTION		2		ivymy.mysbit_hash_extended(ivymy.mysbit, bigint);

