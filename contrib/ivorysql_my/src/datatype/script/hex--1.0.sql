/***************************************************************
 *
 * MySQL ivy_hex string support
 *
***************************************************************/
CREATE FUNCTION ivymy.hex_in(cstring)
RETURNS ivy_hex
AS 'MODULE_PATHNAME','hex_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.hex_out(ivy_hex)
RETURNS cstring
AS 'MODULE_PATHNAME','hex_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

-- Register the basic I/O for 'ivy_hex' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='hex_in')
WHERE oid = 9270;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='hex_out')
WHERE oid = 9270;

/* Begin - bug#z6031 */
CREATE FUNCTION ivymy.hex_positive(ivy_hex)
RETURNS varbinary
AS 'MODULE_PATHNAME','hex_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---- operator -
CREATE OPERATOR + (
	procedure = ivymy.hex_positive,
	rightarg = ivymy.ivy_hex
);
/* End - bug#z6031 */

/* Begin - bug#z6039 */
CREATE FUNCTION ivymy.hex_negative(ivy_hex)
RETURNS double
AS 'MODULE_PATHNAME','hex_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---- operator -
CREATE OPERATOR - (
	procedure = ivymy.hex_negative,
	rightarg = ivymy.ivy_hex
);
/* End - bug#z6039 */

---- operator ivy_hex = ivy_hex
CREATE FUNCTION ivymy.hex_eq(ivy_hex, ivy_hex)
RETURNS int
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR = (
	procedure = hex_eq,
	leftarg = ivy_hex,
	rightarg = ivy_hex
);

/* Begin - bug#z6052 */
CREATE FUNCTION ivymy.hex_eq(binary, ivy_hex)
RETURNS int
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR = (
	procedure = hex_eq,
	leftarg = binary,
	rightarg = ivy_hex
);

CREATE FUNCTION ivymy.hex_eq(ivy_hex, binary)
RETURNS int
AS 'MODULE_PATHNAME','anybinary_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR = (
	procedure = hex_eq,
	leftarg = ivy_hex,
	rightarg = binary
);
/* End - bug#z6052 */

---- operator ivy_hex != ivy_hex
CREATE FUNCTION ivymy.hex_ne(ivy_hex, ivy_hex)
RETURNS int
AS 'MODULE_PATHNAME','anybinary_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR != (
	procedure = hex_ne,
	leftarg = ivy_hex,
	rightarg = ivy_hex
);

--
-- cast (ivy_hex as text)
--
CREATE CAST(ivy_hex as text) 
WITHOUT FUNCTION
AS ASSIGNMENT;

/* Begin - bug#z6026 */
CREATE FUNCTION ivymy.hex2bpchar(ivy_hex, int, bool, int8)
RETURNS bpchar
AS 'MODULE_PATHNAME', 'hex2bpchar'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

--
-- cast (ivy_hex as bpchar)
--
CREATE CAST(ivy_hex as bpchar) 
WITH FUNCTION ivymy.hex2bpchar(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.hex2varchar(ivy_hex, int, bool, int8)
RETURNS varchar
AS 'MODULE_PATHNAME', 'hex2bpchar'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

--
-- cast (ivy_hex as hex2varchar)
--
CREATE CAST(ivy_hex as varchar) 
WITH FUNCTION ivymy.hex2varchar(ivy_hex, int, bool, int8)
AS ASSIGNMENT;
/* End - bug#z6026 */

--
-- cast (ivy_hex as int2)
--
CREATE FUNCTION ivymy.hex2int2(ivy_hex, int, bool, int8)
RETURNS int2
AS 'MODULE_PATHNAME', 'hex2int2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as int2) 
WITH FUNCTION ivymy.hex2int2(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as int4)
--
CREATE FUNCTION ivymy.hex2int4(ivy_hex, int, bool, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'hex2int4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as int4) 
WITH FUNCTION ivymy.hex2int4(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as int8)
--
CREATE FUNCTION ivymy.hex2int8(ivy_hex, int, bool, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hex2int8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as int8) 
WITH FUNCTION ivymy.hex2int8(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as uint1)
--
CREATE FUNCTION ivymy.hex2uint1(ivy_hex, int, bool, int8)
RETURNS uint1
AS 'MODULE_PATHNAME', 'hex2uint1'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as uint1) 
WITH FUNCTION ivymy.hex2uint1(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as uint2)
--
CREATE FUNCTION ivymy.hex2uint2(ivy_hex, int, bool, int8)
RETURNS uint2
AS 'MODULE_PATHNAME', 'hex2uint2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as uint2) 
WITH FUNCTION ivymy.hex2uint2(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as uint4)
--
CREATE FUNCTION ivymy.hex2uint4(ivy_hex, int, bool, int8)
RETURNS uint4
AS 'MODULE_PATHNAME', 'hex2uint4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as uint4) 
WITH FUNCTION ivymy.hex2uint4(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as uint8)
--
CREATE FUNCTION ivymy.hex2uint8(ivy_hex, int, bool, int8)
RETURNS uint8
AS 'MODULE_PATHNAME', 'hex2uint8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as uint8) 
WITH FUNCTION ivymy.hex2uint8(ivy_hex, int, bool, int8)
AS IMPLICIT;

--
-- cast (ivy_hex as float4)
--
CREATE FUNCTION ivymy.hex2float4(ivy_hex, int, bool, int8)
RETURNS float4
AS 'MODULE_PATHNAME', 'hex2float4'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as float4) 
WITH FUNCTION ivymy.hex2float4(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as float8)
--
CREATE FUNCTION ivymy.hex2float8(ivy_hex, int, bool, int8)
RETURNS float8
AS 'MODULE_PATHNAME', 'hex2float8'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as float8) 
WITH FUNCTION ivymy.hex2float8(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as numeric)
--
CREATE FUNCTION ivymy.hex2numeric(ivy_hex, int, bool, int8)
RETURNS numeric
AS 'MODULE_PATHNAME', 'hex2numeric'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as numeric) 
WITH FUNCTION ivymy.hex2numeric(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as bool)
--
CREATE FUNCTION ivymy.hex2bool(ivy_hex, int, bool, int8)
RETURNS bool
AS 'MODULE_PATHNAME', 'hex2bool'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as bool) 
WITH FUNCTION ivymy.hex2bool(ivy_hex, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as date)
--
CREATE CAST(ivy_hex as mysdate) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (ivy_hex as time)
--
CREATE CAST(ivy_hex as mystime) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (ivy_hex as time(x))
--
CREATE CAST(ivy_hex as mystimes) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (ivy_hex as datetime)
--
CREATE CAST(ivy_hex as mysdatetime) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (ivy_hex as datetime(x))
--
CREATE CAST(ivy_hex as mysdatetimes) 
WITH INOUT
AS ASSIGNMENT;

/* Begin - bug#z6028 */
--
-- cast (ivy_hex as timestamp)
--
CREATE CAST(ivy_hex as mystimestamp) 
WITH INOUT
AS ASSIGNMENT;

--
-- cast (ivy_hex as timestamp(x))
--
CREATE CAST(ivy_hex as mystimestamps) 
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.hex2year(ivy_hex, int4, bool, int8)
RETURNS year
AS 'MODULE_PATHNAME','hex2year'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

--
-- cast (ivy_hex as year)
--
CREATE CAST(ivy_hex as year) 
WITH FUNCTION ivymy.hex2year(ivy_hex, int4, bool, int8)
AS ASSIGNMENT;
/* End - bug#z6028 */

--
-- cast (ivy_hex as mysbit)
--
CREATE FUNCTION ivymy.hex2mysbit(ivy_hex, int4, bool, int8)
RETURNS mysbit
AS 'MODULE_PATHNAME','bitfromvarbinary'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as mysbit) 
WITH FUNCTION ivymy.hex2mysbit(ivy_hex, int4, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as bit)
--
CREATE FUNCTION ivymy.hex2bit(ivy_hex, int4, bool, int8)
RETURNS pg_catalog.bit
AS 'MODULE_PATHNAME','hex2bit'
LANGUAGE C
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(ivy_hex as pg_catalog.bit) 
WITH FUNCTION ivymy.hex2bit(ivy_hex, int4, bool, int8)
AS ASSIGNMENT;

--
-- cast (ivy_hex as binary)
--
CREATE CAST(ivy_hex as binary) 
WITHOUT FUNCTION
AS ASSIGNMENT;

--
-- cast (ivy_hex as varbinary)
--
CREATE CAST(ivy_hex as varbinary) 
WITHOUT FUNCTION
AS ASSIGNMENT;

--
-- cast (ivy_hex as tinyblob)
--
CREATE CAST(ivy_hex as tinyblob) 
WITHOUT FUNCTION
AS ASSIGNMENT;

--
-- cast (ivy_hex as myblob)
--
CREATE CAST(ivy_hex as myblob) 
WITHOUT FUNCTION
AS ASSIGNMENT;

--
-- cast (ivy_hex as mediumblob)
--
CREATE CAST(ivy_hex as mediumblob) 
WITHOUT FUNCTION
AS ASSIGNMENT;

--
-- cast (ivy_hex as longblob)
--
CREATE CAST(ivy_hex as longblob) 
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE CAST (ivy_hex as bytea)
WITHOUT FUNCTION
AS ASSIGNMENT;

CREATE FUNCTION pg_catalog.position(ivy_hex, ivy_hex)
RETURNS int
AS 'select pg_catalog.position($1::bytea, $2::bytea)'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;
