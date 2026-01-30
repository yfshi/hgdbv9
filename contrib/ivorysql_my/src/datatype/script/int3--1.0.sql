/* Begin - ReqID: SRS-MYSQL-DATATYPE-INT3 */

CREATE FUNCTION ivymy.int3_in(pg_catalog.cstring, oid, int4, int8)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int3_in'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int3_out(ivymy.mediumint)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','int3_out'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int3_recv(internal, oid, int4, int8)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int3_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int3_send(ivymy.mediumint)
RETURNS bytea
AS 'MODULE_PATHNAME','int3_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.int3_scale(ivymy.mediumint, pg_catalog.int4)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int3_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'int3' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='int3_in')
WHERE oid = 9208;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='int3_out')
WHERE oid = 9208;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='int3_recv')
WHERE oid = 9208;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='int3_send')
WHERE oid = 9208;

CREATE CAST (ivymy.mediumint AS pg_catalog.int4)
WITHOUT FUNCTION
AS IMPLICIT;

CREATE FUNCTION ivymy.int8_int3(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int8_int3'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mediumint)
WITH FUNCTION ivymy.int8_int3(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int4_int3(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int4_int3'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (pg_catalog.int4 AS ivymy.mediumint)
WITH FUNCTION ivymy.int4_int3(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int3_boolean(ivymy.mediumint)
RETURNS boolean 
AS 'MODULE_PATHNAME','int3_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mediumint AS boolean)
WITH FUNCTION ivymy.int3_boolean(ivymy.mediumint)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.int3_numeric(ivymy.mediumint)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','int3_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mediumint AS pg_catalog.numeric)
WITH FUNCTION ivymy.int3_numeric(ivymy.mediumint)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mediumint)
WITH FUNCTION numeric_mediumint(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

--  "+"
CREATE FUNCTION ivymy.int3_pl(ivymy.mediumint, ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'int4pl'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.int3_pl,
	leftarg = ivymy.mediumint,
	rightarg = ivymy.mediumint
);

--  "-"
CREATE FUNCTION ivymy.int3_mi(ivymy.mediumint, ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'int4mi'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.int3_mi,
	leftarg = ivymy.mediumint,
	rightarg = ivymy.mediumint
);

--"sign +"
CREATE FUNCTION ivymy.int3_positive(ivymy.mediumint)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','int3_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.int3_positive,
	rightarg = ivymy.mediumint
);

--"sign -"
CREATE FUNCTION ivymy.int3_negative(ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','int3_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.int3_negative,
	rightarg = ivymy.mediumint
);

CREATE FUNCTION ivymy.int3_cat(ivymy.mediumint, ivymy.mediumint)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR || (
	procedure = ivymy.int3_cat,
	leftarg = ivymy.mediumint,
	rightarg = ivymy.mediumint
);

/* End - ReqID: SRS-MYSQL-DATATYPE-INT3 */