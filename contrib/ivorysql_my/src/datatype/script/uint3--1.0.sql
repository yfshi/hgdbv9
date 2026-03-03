/***************************************************************
 *
 * MySQL uint3 type support
 *
***************************************************************/
CREATE FUNCTION ivymy.uint3_in(pg_catalog.cstring, oid, int4, int8)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','uint3_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_out(ivymy.uint3)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint3_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_recv(internal, oid, int4, int8)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','uint3_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_send(ivymy.uint3)
RETURNS bytea
AS 'MODULE_PATHNAME','uint3_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','uint3_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','uint3_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.uint3_scale(ivymy.uint3, pg_catalog.int4)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','uint3_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'uint3' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='uint3_in')
WHERE oid = 9210;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='uint3_out')
WHERE oid = 9210;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='uint3_recv')
WHERE oid = 9210;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='uint3_send')
WHERE oid = 9210;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint3_typmodin')
WHERE oid = 9210;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint3_typmodout')
WHERE oid = 9210;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='uint3_typmodin')
WHERE oid = 9211;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='uint3_typmodout')
WHERE oid = 9211;

-- Compatible MySQL Cast
CREATE CAST (ivymy.uint3 AS ivymy.uint4)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE FUNCTION ivymy.int8_uint3(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','int8_uint3'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.uint3)
WITH FUNCTION ivymy.int8_uint3(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_uint3(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','int4_uint3'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.uint3)
WITH FUNCTION ivymy.int4_uint3(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int2_uint3(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.uint3
AS 'MODULE_PATHNAME','int2_uint3'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int2 AS ivymy.uint3)
WITH FUNCTION ivymy.int2_uint3(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.uint3)
WITH FUNCTION numeric_uint3(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.uint3_boolean(ivymy.uint3)
RETURNS boolean 
AS 'MODULE_PATHNAME','uint3_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint3 AS boolean)
WITH FUNCTION ivymy.uint3_boolean(ivymy.uint3)
AS ASSIGNMENT;

/* Begin - BUG#Z7200 */
CREATE FUNCTION ivymy.uint3_uint8(ivymy.uint3)
RETURNS ivymy.uint8
AS 'MODULE_PATHNAME','uint3_uint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint3 AS ivymy.uint8)
WITH FUNCTION ivymy.uint3_uint8(ivymy.uint3)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.uint3_int8(ivymy.uint3)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','uint3_int8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.uint3 AS pg_catalog.int8)
WITH FUNCTION ivymy.uint3_int8(ivymy.uint3)
AS ASSIGNMENT;
/* End - BUG#Z7200 */
