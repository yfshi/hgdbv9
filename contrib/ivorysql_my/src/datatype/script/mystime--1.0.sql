/***************************************************************
 *
 * MySQL time type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystime_in(pg_catalog.cstring)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_out(ivymy.mystime)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystime_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_recv(internal)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_send(ivymy.mystime)
RETURNS bytea
AS 'MODULE_PATHNAME','mystime_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mystime' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystime_in')
WHERE oid = 9291;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystime_out')
WHERE oid = 9291;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystime_recv')
WHERE oid = 9291;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystime_send')
WHERE oid = 9291;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','numeric_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystime)
WITH FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_numeric(ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mystime AS pg_catalog.numeric)
WITH FUNCTION ivymy.mystime_numeric(ivymy.mystime)
AS ASSIGNMENT;

CREATE CAST (ivymy.mystime AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.date AS ivymy.mystime)
WITH FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamp_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamp AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamptz_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamptz AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.time AS ivymy.mystime)
WITH INOUT
AS ASSIGNMENT;

----function eq
CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mystime_add(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_add'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystime_sub(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_sub'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystime_mul(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_mul'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystime_div(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_div'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_exact_divtime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystime_mod(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_mod'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystime_bitand(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitand'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystime_bitor(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystime_nega(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_neg'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystime_bitxor(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bitxor'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function >>
CREATE FUNCTION ivymy.mystime_rightmov(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_rightmov'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystime_leftmov(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_leftmov'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function &&
CREATE FUNCTION ivymy.mystime_and(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_and'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function or
CREATE FUNCTION ivymy.mystime_cat(ivymy.mystime, ivymy.mystime)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystime_invert(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_invert'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--time to boolean
CREATE FUNCTION ivymy.mystime_boolean(ivymy.mystime) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystime_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS boolean)
WITH FUNCTION ivymy.mystime_boolean(ivymy.mystime)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystime_add,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystime_sub,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystime_mul,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystime_div,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystime_mod,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystime_bitand,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystime_bitor,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystime_nega,
	rightarg = ivymy.mystime
);

--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystime_bitxor,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystime_rightmov,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystime_leftmov,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);


--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystime_and,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystime_cat,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystime_invert,
	rightarg = ivymy.mystime
);


----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, ivymy.mystimes)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

-- not like !~~
CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE FUNCTION ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.mystimetime_ops USING btree;

CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING btree FAMILY ivymy.mystimetime_ops AS
		OPERATOR		1		< (ivymy.mystime, ivymy.mystime),
		OPERATOR		2		<= (ivymy.mystime, ivymy.mystime),
		OPERATOR		3		= (ivymy.mystime, ivymy.mystime),
		OPERATOR		4		>= (ivymy.mystime, ivymy.mystime),
		OPERATOR		5		> (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime),
		FUNCTION		2		ivymy.mystime_sortsupport(internal);

CREATE FUNCTION ivymy.mystime_hash(ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_hash_extended(ivymy.mystime, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystime_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING hash AS
		OPERATOR		1		= (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_hash(ivymy.mystime),
		FUNCTION		2		ivymy.mystime_hash_extended(ivymy.mystime, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

CREATE FUNCTION ivymy.mysdate_mystime(ivymy.mysdate) 
RETURNS ivymy.mystime 
AS 'MODULE_PATHNAME','mysdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS ivymy.mystime)
WITH FUNCTION ivymy.mysdate_mystime(ivymy.mysdate)
AS ASSIGNMENT;

CREATE CAST (ivymy.mysdatetime AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mysdatetimes AS ivymy.mystime)      WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamp AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;
CREATE CAST (ivymy.mystimestamps AS ivymy.mystime)     WITH INOUT AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mysdate(ivymy.mystime) 
RETURNS ivymy.mysdate 
AS 'MODULE_PATHNAME','mystime_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mysdate)
WITH FUNCTION ivymy.mystime_mysdate(ivymy.mystime)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime) 
RETURNS ivymy.mystimestamp 
AS 'MODULE_PATHNAME','mystime_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mystimestamp)
WITH FUNCTION ivymy.mystime_mystimestamp(ivymy.mystime)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer) 
RETURNS ivymy.mysdatetimes 
AS 'MODULE_PATHNAME','mystime_mysdatetimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mysdatetimes)
WITH FUNCTION ivymy.mystime_mysdatetimes(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer) 
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mystimestamps)
WITH FUNCTION ivymy.mystime_mystimestamps(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer) 
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS ivymy.mystimes)
WITH FUNCTION ivymy.mystime_mystimes(ivymy.mystime, integer)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.time2year(ivymy.mystime)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'time_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (ivymy.mystime as year)
WITH FUNCTION ivymy.time2year(ivymy.mystime)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mystime_int8(ivymy.mystime)
RETURNS int8
AS 'MODULE_PATHNAME', 'mystime_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;

CREATE CAST (ivymy.mystime as bigint)
WITH FUNCTION mystime_int8(ivymy.mystime)
AS IMPLICIT;

CREATE OPERATOR CLASS ivymy.mystime_minmax_ops
	DEFAULT FOR TYPE ivymy.mystime USING brin FAMILY ivymy.mys_mysdate_minmax_ops AS
		OPERATOR	1 	<  (ivymy.mystime, ivymy.mystime),
		OPERATOR	2 	<= (ivymy.mystime, ivymy.mystime),
		OPERATOR	3 	=  (ivymy.mystime, ivymy.mystime),
		OPERATOR	4  	>= (ivymy.mystime, ivymy.mystime),
		OPERATOR	5  	>  (ivymy.mystime, ivymy.mystime),
		FUNCTION	1	ivymy.brin_minmax_opcinfo(internal),
		FUNCTION	2 	ivymy.brin_minmax_add_value(internal, internal, internal, internal),
		FUNCTION	3 	ivymy.brin_minmax_consistent(internal, internal, internal),
		FUNCTION	4 	ivymy.brin_minmax_union(internal, internal, internal);