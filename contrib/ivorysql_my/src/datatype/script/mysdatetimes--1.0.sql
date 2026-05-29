-- Begin - ReqID:SRS-MYSQL-DATETIMETYPE 
/***************************************************************
 *
 * MySQL datetimes type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdatetimes_in(pg_catalog.cstring)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mysdatetimes_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetimes_out(ivymy.mysdatetimes)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdatetimes_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdatetimes_recv(internal)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','mysdatetimes_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_send(ivymy.mysdatetimes)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdatetimes_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mysdatetimes_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mysdatetimes_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mysdatetimes_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mysdatetimes' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_in')
WHERE oid = 9285;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_out')
WHERE oid = 9285;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_recv')
WHERE oid = 9285;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_send')
WHERE oid = 9285;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodin')
WHERE oid = 9285;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodout')
WHERE oid = 9285;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodin')
WHERE oid = 9286;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mysdatetimes_typmodout')
WHERE oid = 9286;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdatetimes_eq(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdatetimes_ne(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mysdatetimes_lt(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdatetimes_gt(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


--function le
CREATE FUNCTION ivymy.mysdatetimes_le(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdatetimes_ge(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdatetimes_seq(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mysdatetimes_nega(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_negadatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int2, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int4, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.int8, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_bitxor(pg_catalog.numeric, ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mysdatetimes_cat(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mysdatetimes_invert(ivymy.mysdatetimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdatetimes_invertmysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdatetimes_eq,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdatetimes_ne,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);


--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdatetimes_gt,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdatetimes_lt,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdatetimes_le,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdatetimes_ge,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

/*
CREATE OPERATOR <=> (
	procedure = ivymy.mysdatetimes_seq,
	leftarg = text,
	rightarg = text
);*/

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mysdatetimes_nega,
	rightarg = ivymy.mysdatetimes
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mysdatetimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mysdatetimes_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mysdatetimes
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdatetimes_cat,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mysdatetimes_invert,
	rightarg = ivymy.mysdatetimes
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_like(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdatetimes_notlike(ivymy.mysdatetimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdatetimes_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdatetimes_like,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = ivymy.mysdatetimes
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdatetimes_notlike,
	leftarg = ivymy.mysdatetimes,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mysdatetimestime_ops' for type 'mysdatetimes/mysdatetimestime/mysdatetimes.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdatetimes_cmp(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetimes_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdatetimes_sortsupport(internal)
RETURNS void
AS 'numeric_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetimes_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetimestime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mysdatetimes_ops
	DEFAULT FOR TYPE ivymy.mysdatetimes USING btree FAMILY ivymy.mysdatetimestime_ops AS
		OPERATOR		1		< (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		2		<= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		3		= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		4		>= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		OPERATOR		5		> (ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		1		ivymy.mysdatetimes_cmp(ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		2		ivymy.mysdatetimes_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdatetimes_hash(ivymy.mysdatetimes)
RETURNS integer
AS 'MODULE_PATHNAME','mysdatetimes_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdatetimes_hash_extended(ivymy.mysdatetimes, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mysdatetimes_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdatetimes_ops
	DEFAULT FOR TYPE ivymy.mysdatetimes USING hash AS
		OPERATOR		1		= (ivymy.mysdatetimes, ivymy.mysdatetimes),
		FUNCTION		1		ivymy.mysdatetimes_hash(ivymy.mysdatetimes),
		FUNCTION		2		ivymy.mysdatetimes_hash_extended(ivymy.mysdatetimes, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysdatetimes_larger(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'mysdatetimes_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysdatetimes) (
  SFUNC = ivymy.mysdatetimes_larger,
  STYPE = ivymy.mysdatetimes,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysdatetimes_smaller(ivymy.mysdatetimes, ivymy.mysdatetimes)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME', 'mysdatetimes_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysdatetimes) (
  SFUNC = ivymy.mysdatetimes_smaller,
  STYPE = ivymy.mysdatetimes,
  SORTOP = <,
  PARALLEL = SAFE
);

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int8, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int4, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdatetimes(pg_catalog.int2, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','int_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdatetimes(pg_catalog.numeric, integer, bool, int8)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','numeric_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int8, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int4, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.int_mysdatetimes(pg_catalog.int2, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.numeric_mysdatetimes(pg_catalog.numeric, integer, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mysdatetimes AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mysdatetimes(pg_catalog.date, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgdate_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgdate_mysdatetimes(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mysdatetimes(pg_catalog.timestamp, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtimestamp_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtimestamp_mysdatetimes(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mysdatetimes(pg_catalog.timestamptz, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtimestamptz_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtimestamptz_mysdatetimes(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mysdatetimes(pg_catalog.time, integer)
RETURNS ivymy.mysdatetimes
AS 'MODULE_PATHNAME','pgtime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.pgtime_mysdatetimes(pg_catalog.time, integer)
AS ASSIGNMENT;

--datetime to boolean
CREATE FUNCTION ivymy.mysdatetimes_boolean(ivymy.mysdatetimes) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdatetimes_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mysdatetimes AS boolean)
WITH FUNCTION ivymy.mysdatetimes_boolean(ivymy.mysdatetimes)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdatetimes_float4(ivymy.mysdatetimes)
RETURNS float4
AS 'select $1::numeric::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdatetimes as float4) WITH FUNCTION mysdatetimes_float4(ivymy.mysdatetimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdatetimes_float8(ivymy.mysdatetimes)
RETURNS float8
AS 'select $1::numeric::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdatetimes as float8) WITH FUNCTION mysdatetimes_float8(ivymy.mysdatetimes) AS ASSIGNMENT;

-- to int8
CREATE OR REPLACE FUNCTION ivymy.mysdatetimes_int8(ivymy.mysdatetimes)
RETURNS int8
AS 'MODULE_PATHNAME', 'mysdatetimes_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;
CREATE CAST (ivymy.mysdatetimes as bigint)
WITH FUNCTION mysdatetimes_int8(ivymy.mysdatetimes)
AS ASSIGNMENT;

-- to numeric
CREATE CAST (ivymy.mysdatetimes AS pg_catalog.numeric) WITHOUT FUNCTION AS IMPLICIT;

-- End - ReqID:SRS-MYSQL-DATETIMETYPE 