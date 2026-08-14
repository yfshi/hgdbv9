--
-- cast (varchar as int2)
--
CREATE FUNCTION ivymy.varchar_int2(varchar, int, bool, int8)
RETURNS int2
AS 'MODULE_PATHNAME', 'varlena_int2'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST(varchar as int2)
WITH FUNCTION ivymy.varchar_int2(varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (varchar as int4)
--
CREATE FUNCTION ivymy.varchar_int4(varchar, int, bool, int8)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'varlena_int4'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST(varchar as pg_catalog.int4)
WITH FUNCTION ivymy.varchar_int4(varchar, int, bool, int8)
AS ASSIGNMENT;

--
-- cast (varchar as int8)
--
CREATE FUNCTION ivymy.varchar_int8(varchar, int, bool, int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'varlena_int8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST(varchar as pg_catalog.int8)
WITH FUNCTION ivymy.varchar_int8(varchar, int, bool, int8)
AS ASSIGNMENT;

-- Begin - ReqID:SRS-MYSQL-SET
--
-- cast (varchar as numeric)
--
create or replace function ivymy.varchar_numeric(varchar, int, bool, int8)
returns numeric
as 'MODULE_PATHNAME', 'varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (varchar AS numeric)
WITH FUNCTION ivymy.varchar_numeric(varchar, int, bool, int8)
AS ASSIGNMENT;
-- End - ReqID:SRS-MYSQL-SET

/* Begin - ReqID:SRS-MYSQL-SQL-Alter */
--
-- cast (varchar as float4)
--
CREATE CAST (varchar AS float4)
WITH FUNCTION ivymy.text_float4(text, integer, boolean, int8)
AS ASSIGNMENT;

--
-- cast (varchar as float8)
--
CREATE CAST (varchar AS float8)
WITH FUNCTION ivymy.text_float8(text, integer, boolean, int8)
AS ASSIGNMENT;
/* End - ReqID:SRS-MYSQL-SQL-Alter */