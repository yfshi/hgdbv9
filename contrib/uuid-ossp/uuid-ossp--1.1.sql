/* contrib/uuid-ossp/uuid-ossp--1.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use '''CREATE EXTENSION "uuid-ossp"''' to load this file. \quit

CREATE FUNCTION pg_catalog.uuid_nil()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_nil'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_ns_dns()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_ns_dns'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_ns_url()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_ns_url'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_ns_oid()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_ns_oid'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_ns_x500()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_ns_x500'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_generate_v1()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v1'
VOLATILE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_generate_v1mc()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v1mc'
VOLATILE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_generate_v3(namespace uuid, name text)
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v3'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_generate_v4()
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v4'
VOLATILE STRICT LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION pg_catalog.uuid_generate_v5(namespace uuid, name text)
RETURNS uuid
AS 'MODULE_PATHNAME', 'uuid_generate_v5'
IMMUTABLE STRICT LANGUAGE C PARALLEL SAFE;

/* Begin - BUG#M0000359 */
CREATE FUNCTION pg_catalog.sys_guid()
RETURNS bytea
AS 'MODULE_PATHNAME', 'ora_sys_guid'
VOLATILE STRICT LANGUAGE C;
/* End - BUG#M0000359 */
