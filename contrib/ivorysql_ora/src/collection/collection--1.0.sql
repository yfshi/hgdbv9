/***************************************************************
 *
 * collection type
 *
 ***************************************************************/

CREATE FUNCTION sys.collection_init()
RETURN void
AS 'MODULE_PATHNAME', 'collection_init'
LANGUAGE C
IMMUTABLE;


CREATE FUNCTION sys.collection_first(associative_array, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_first'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_last(associative_array, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_last'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_count(associative_array)
RETURNS INT
AS 'MODULE_PATHNAME','collection_count'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_limit(associative_array)
RETURNS INT
AS 'MODULE_PATHNAME','collection_limit'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_prior(associative_array, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_prior'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_next(associative_array, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_next'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_exists(associative_array, anyelement)
RETURNS boolean
AS 'MODULE_PATHNAME','collection_exists'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(associative_array)
RETURNS associative_array
AS 'MODULE_PATHNAME','collection_delete'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(associative_array, anyelement)
RETURNS associative_array
AS 'MODULE_PATHNAME','collection_delete1'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(associative_array, anyelement, anyelement)
RETURNS associative_array
AS 'MODULE_PATHNAME','collection_delete2'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_first(nested_table, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_first'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_last(nested_table, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_last'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_count(nested_table)
RETURNS INT
AS 'MODULE_PATHNAME','collection_count'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_limit(nested_table)
RETURNS INT
AS 'MODULE_PATHNAME','collection_limit'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_prior(nested_table, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_prior'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_next(nested_table, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_next'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_exists(nested_table, anyelement)
RETURNS boolean
AS 'MODULE_PATHNAME','collection_exists'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(nested_table)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_delete'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(nested_table, anyelement)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_delete1'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(nested_table, anyelement, anyelement)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_delete2'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.nested_table_constructor(int8, anyarray)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_constructor'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.varray_constructor(int8, anyarray)
RETURNS varray
AS 'MODULE_PATHNAME','collection_constructor'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_trim(nested_table)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_trim'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_trim(nested_table, anyelement)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_trim_n'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(nested_table)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_extend'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(nested_table, anyelement)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_extend_n'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(nested_table, anyelement, anyelement)
RETURNS nested_table
AS 'MODULE_PATHNAME','collection_extend_ni'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.nested_table_eq(nested_table, nested_table)
RETURNS boolean
AS 'MODULE_PATHNAME','nested_table_eq'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.nested_table_ne(nested_table, nested_table)
RETURNS boolean
AS 'MODULE_PATHNAME','nested_table_ne'
LANGUAGE C
IMMUTABLE;

CREATE OPERATOR = (
    PROCEDURE = sys.nested_table_eq,
	LEFTARG = nested_table,
	RIGHTARG = nested_table
);

CREATE OPERATOR <> (
    PROCEDURE = sys.nested_table_ne,
	LEFTARG = nested_table,
	RIGHTARG = nested_table
);

CREATE FUNCTION sys.collection_first(varray, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_first'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_last(varray, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_last'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_count(varray)
RETURNS INT
AS 'MODULE_PATHNAME','collection_count'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_limit(varray)
RETURNS INT
AS 'MODULE_PATHNAME','collection_limit'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_prior(varray, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_prior'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_next(varray, anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME','collection_next'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_exists(varray, anyelement)
RETURNS boolean
AS 'MODULE_PATHNAME','collection_exists'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(varray)
RETURNS varray
AS 'MODULE_PATHNAME','collection_delete'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(varray, anyelement)
RETURNS varray
AS 'MODULE_PATHNAME','collection_delete1'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_delete(varray, anyelement, anyelement)
RETURNS varray
AS 'MODULE_PATHNAME','collection_delete2'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_trim(varray)
RETURNS varray
AS 'MODULE_PATHNAME','collection_trim'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_trim(varray, anyelement)
RETURNS varray
AS 'MODULE_PATHNAME','collection_trim_n'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(varray)
RETURNS varray
AS 'MODULE_PATHNAME','collection_extend'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(varray, anyelement)
RETURNS varray
AS 'MODULE_PATHNAME','collection_extend_n'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_extend(varray, anyelement, anyelement)
RETURNS varray
AS 'MODULE_PATHNAME','collection_extend_ni'
LANGUAGE C
IMMUTABLE;

/* Begin - ReqID:SRS-SQL-Collection */
CREATE FUNCTION sys.sql_collection_constructor(anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME','sql_collection_constructor'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_to_array(nested_table, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'collection_to_array'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.collection_to_array(varray, anyarray)
RETURNS anyarray
AS 'MODULE_PATHNAME', 'collection_to_array'
LANGUAGE C
IMMUTABLE;

CREATE FUNCTION sys.array_to_collection(anyelement, anyarray)
RETURNS anyelement
AS 'MODULE_PATHNAME', 'array_to_collection'
LANGUAGE C
IMMUTABLE;
/* End - ReqID:SRS-SQL-Collection */
