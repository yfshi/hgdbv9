/* src/pl/plisql/src/plisql--1.0.sql */

-- The language object, but not the functions, can be owned by a non-superuser.
ALTER LANGUAGE plisql OWNER TO @extowner@;

COMMENT ON LANGUAGE plisql IS 'PL/iSQL procedural language';


/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
CREATE FUNCTION sys.plisql_sqlerrm(integer)
RETURNS text
AS 'MODULE_PATHNAME', 'plisql_sqlerrm_int4'
LANGUAGE C STRICT;


CREATE FUNCTION sys.plisql_sqlcode()
RETURNS integer
AS 'MODULE_PATHNAME', 'plisql_sqlcode'
LANGUAGE C;

CREATE FUNCTION sys.plisql_error_stack()
RETURNS text
AS 'MODULE_PATHNAME', 'plisql_error_stack'
LANGUAGE C;

CREATE FUNCTION sys.plisql_sqlerrm()
RETURNS text
AS 'MODULE_PATHNAME', 'plisql_sqlerrm'
LANGUAGE C;

CREATE PROCEDURE sys.plisql_raise_application_error(integer,
text,
boolean)
AS 'MODULE_PATHNAME', 'plisql_raise_application_error'
LANGUAGE C;
/* End - ReqID:SRS-PLSQL-EXCEPTION */

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
CREATE DOMAIN PLS_INTEGER AS INTEGER;
CREATE DOMAIN BINARY_INTEGER AS INTEGER;
CREATE DOMAIN SIMPLE_INTEGER AS INTEGER;
/* End - ReqID:SRS-PLSQL-SUBTYPE */