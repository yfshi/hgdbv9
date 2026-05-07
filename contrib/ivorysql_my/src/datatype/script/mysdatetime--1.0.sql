
-- Begin - ReqID:SRS-MYSQL-DATETIMETYPE 
/***************************************************************
 *
 * MySQL datetime type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdatetime_in(pg_catalog.cstring)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetime_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetime_out(ivymy.mysdatetime)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdatetime_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetime_recv(internal)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetime_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetime_send(ivymy.mysdatetime)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdatetime_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysdatetime' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_in')
WHERE oid = 9283;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_out')
WHERE oid = 9283;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_recv')
WHERE oid = 9283;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdatetime_send')
WHERE oid = 9283;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdatetime_ne(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mysdatetime_lt(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdatetime_gt(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysdatetime_le(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdatetime_ge(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdatetime_seq(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_seq(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_seqstr'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysdatetime_nega(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_negadatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mysdatetime_cat(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----operator invert !
CREATE FUNCTION ivymy.mysdatetime_invert(ivymy.mysdatetime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetime_invertmysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);


----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdatetime_ne,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);


--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdatetime_gt,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdatetime_lt,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdatetime_le,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdatetime_ge,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);


--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetime_seq,
	leftarg = text,
	rightarg = text
);*/

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysdatetime_nega,
	rightarg = ivymy.mysdatetime
);
--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdatetime_cat,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysdatetime_invert,
	rightarg = ivymy.mysdatetime
);

-- eq: mysdatetime = numeric
CREATE FUNCTION ivymy.mysdatetime_eq(ivymy.mysdatetime, numeric)
RETURNS bool
AS 'select $1::int8::numeric = $2::numeric;'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.= (
	procedure = ivymy.mysdatetime_eq,
	leftarg = ivymy.mysdatetime,
	rightarg = numeric
);
----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_like(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetime_notlike(ivymy.mysdatetime, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetime_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetime_like,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = ivymy.mysdatetime
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetime_notlike,
	leftarg = ivymy.mysdatetime,
	rightarg = pg_catalog.int8
);
/*
 * B-tree Index Support
 * Create Operator family 'mysdatetimetime_ops' for type 'mysdatetime/mysdatetimetime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdatetime_cmp(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetime_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetime_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetime_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetimetime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetime_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING btree FAMILY ivymy.mysdatetimetime_ops AS
		OPERATOR		1		< (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		2		<= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		3		= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		4		>= (ivymy.mysdatetime, ivymy.mysdatetime),
		OPERATOR		5		> (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		1		ivymy.mysdatetime_cmp(ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		2		ivymy.mysdatetime_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdatetime_hash(ivymy.mysdatetime)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetime_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetime_hash_extended(ivymy.mysdatetime, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysdatetime_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdatetime_ops
	DEFAULT FOR TYPE ivymy.mysdatetime USING hash AS
		OPERATOR		1		= (ivymy.mysdatetime, ivymy.mysdatetime),
		FUNCTION		1		ivymy.mysdatetime_hash(ivymy.mysdatetime),
		FUNCTION		2		ivymy.mysdatetime_hash_extended(ivymy.mysdatetime, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysdatetime_larger(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME', 'mysdatetime_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysdatetime) (
  SFUNC = ivymy.mysdatetime_larger,
  STYPE = ivymy.mysdatetime,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysdatetime_smaller(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME', 'mysdatetime_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysdatetime) (
  SFUNC = ivymy.mysdatetime_smaller,
  STYPE = ivymy.mysdatetime,
  SORTOP = <,
  PARALLEL = SAFE
);

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetime(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','int_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdatetime(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','numeric_mysdatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mysdatetime)
WITH FUNCTION ivymy.int_mysdatetime(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mysdatetime)
WITH FUNCTION ivymy.numeric_mysdatetime(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysdatetime AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamp_mysdatetime(pg_catalog.timestamp)
RETURNS ivymy.mysdatetime
AS $$
    SELECT to_char($1, 'YYYY-MM-DD HH24:MI:SS.MS')::ivymy.mysdatetime;
$$
LANGUAGE SQL
STRICT PARALLEL SAFE;

CREATE CAST (pg_catalog.timestamp AS ivymy.mysdatetime)
WITH FUNCTION ivymy.timestamp_mysdatetime(pg_catalog.timestamp)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.date AS ivymy.mysdatetime)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamptz_mysdatetime(pg_catalog.timestamptz)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','timestamptz_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdatetime)
WITH FUNCTION ivymy.timestamptz_mysdatetime(pg_catalog.timestamptz)
AS ASSIGNMENT;

--datetime to boolean
CREATE FUNCTION ivymy.mysdatetime_boolean(ivymy.mysdatetime) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdatetime_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdatetime AS boolean)
WITH FUNCTION ivymy.mysdatetime_boolean(ivymy.mysdatetime)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdatetime_float4(ivymy.mysdatetime)
RETURNS float4
AS 'select $1::int8::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdatetime as float4) WITH FUNCTION mysdatetime_float4(ivymy.mysdatetime) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdatetime_float8(ivymy.mysdatetime)
RETURNS float8
AS 'select $1::int8::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdatetime as float8) WITH FUNCTION mysdatetime_float8(ivymy.mysdatetime) AS ASSIGNMENT;

-- to int8
CREATE CAST (ivymy.mysdatetime as bigint) WITHOUT FUNCTION AS IMPLICIT;

-- to numeric
CREATE FUNCTION ivymy.mysdatetime_numeric(ivymy.mysdatetime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdatetime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (ivymy.mysdatetime AS pg_catalog.numeric) WITH FUNCTION ivymy.mysdatetime_numeric(ivymy.mysdatetime) AS ASSIGNMENT;
