/* Begin - ReqID: SRS-MYSQL-DATATYPE-INT1 */

CREATE FUNCTION ivymy.int1_in(pg_catalog.cstring, oid, int4, int8)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int1_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int1_out(ivymy.tinyint)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','int1_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int1_recv(internal, oid, int4, int8)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int1_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int1_send(ivymy.tinyint)
RETURNS bytea
AS 'MODULE_PATHNAME','int1_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int1_scale(ivymy.tinyint, pg_catalog.int4)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int1_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'int1' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='int1_in')
WHERE oid = 9200;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='int1_out')
WHERE oid = 9200;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='int1_recv')
WHERE oid = 9200;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='int1_send')
WHERE oid = 9200;

CREATE FUNCTION ivymy.int1_int2(ivymy.tinyint)
RETURNS pg_catalog.int2
AS 'MODULE_PATHNAME','int1_int2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.tinyint AS pg_catalog.int2)
WITH FUNCTION ivymy.int1_int2(ivymy.tinyint)
AS IMPLICIT;

/* Begin - BUG#Z7200 */
CREATE FUNCTION ivymy.int1_int8(ivymy.tinyint)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int1_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.tinyint AS pg_catalog.int8)
WITH FUNCTION ivymy.int1_int8(ivymy.tinyint)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int1_uint8(ivymy.tinyint, int, bool, int8)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int1_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.tinyint AS ivymy.uint8)
WITH FUNCTION ivymy.int1_uint8(ivymy.tinyint, int, bool, int8)
AS ASSIGNMENT;
/* End - BUG#Z7200 */

CREATE FUNCTION ivymy.int8_int1(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int8_int1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.tinyint)
WITH FUNCTION ivymy.int8_int1(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_int1(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int4_int1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.tinyint)
WITH FUNCTION ivymy.int4_int1(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_int1(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int2_int1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.tinyint)
WITH FUNCTION ivymy.int2_int1(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int1_boolean(ivymy.tinyint)
RETURNS boolean 
AS 'MODULE_PATHNAME','int1_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.tinyint AS boolean)
WITH FUNCTION ivymy.int1_boolean(ivymy.tinyint)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.boolean_int1(pg_catalog.bool) 
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','boolean_int1'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.bool AS ivymy.tinyint)
WITH FUNCTION ivymy.boolean_int1(pg_catalog.bool)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.tinyint)
WITH FUNCTION numeric_tinyint(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int1_numeric(ivymy.tinyint)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int1_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.tinyint AS pg_catalog.numeric)
WITH FUNCTION ivymy.int1_numeric(ivymy.tinyint)
AS ASSIGNMENT;

--"int1 sign +"
CREATE FUNCTION ivymy.int1_positive(ivymy.tinyint)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','int1_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.int1_positive,
	rightarg = ivymy.tinyint
);

--  "<<"
CREATE FUNCTION ivymy.int1_left_shift(ivymy.tinyint, ivymy.tinyint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int1_left_shift'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.<< (
	procedure = ivymy.int1_left_shift,
	leftarg = ivymy.tinyint,
	rightarg = ivymy.tinyint,
	commutator = <<
);

--  ">>"
CREATE FUNCTION ivymy.int1_right_shift(ivymy.tinyint, ivymy.tinyint)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','int1_right_shift'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.>> (
	procedure = ivymy.int1_right_shift,
	leftarg = ivymy.tinyint,
	rightarg = ivymy.tinyint,
	commutator = >>
);

/* End - ReqID: SRS-MYSQL-DATATYPE-INT1 */