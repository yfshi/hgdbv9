/***************************************************************
 *
 * character datatype support
 *
 ***************************************************************/

-- datatype tinytext mediumtext longtext
ALTER DOMAIN ivymy.tinytext ADD CONSTRAINT tinytext_check CHECK(length(VALUE) <= 255);
ALTER DOMAIN ivymy.mediumtext ADD CONSTRAINT mediumtext_check CHECK(length(VALUE) <= 16777215);

--
-- text + text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_pl(text, text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystext_pl'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.+ (
	FUNCTION = ivymy.mystext_pl,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text - text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_mi(text, text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystext_mi'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.- (
	FUNCTION = ivymy.mystext_mi,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text * text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_mul(text, text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystext_mul'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.* (
	PROCEDURE = ivymy.mystext_mul,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text / text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_div(text, text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystext_div'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy./ (
	PROCEDURE = ivymy.mystext_div,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text % text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_mod(text, text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystext_mod'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.% (
	PROCEDURE = ivymy.mystext_mod,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text // text
--
CREATE OR REPLACE FUNCTION ivymy.mys_div(text, text)
	RETURNS int8
	AS 'MODULE_PATHNAME', 'mystext_modiv'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.// (
	PROCEDURE = ivymy.mys_div,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text + float8
--
CREATE OR REPLACE FUNCTION ivymy.mystextfloat8_pl(text, pg_catalog.float8)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystextfloat8_pl'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.+ (
	FUNCTION = mystextfloat8_pl,
	LEFTARG = text,
	RIGHTARG = pg_catalog.float8
);

--
-- ! text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_logic_not(text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'mystext_logic_not'
	LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.! (
	FUNCTION = ivymy.mystext_logic_not,
	RIGHTARG = text
);

--
-- text && text
--
CREATE OR REPLACE FUNCTION ivymy.mystext_logic_and(text, text)
	RETURNS bool
	AS 'MODULE_PATHNAME', 'mystext_logic_and'
	LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.&& (
	FUNCTION = ivymy.mystext_logic_and,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text ^ text
--
CREATE OR REPLACE FUNCTION mystext_xor(text, text)
		RETURNS pg_catalog.int8
		AS 'MODULE_PATHNAME', 'mystext_xor'
		LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.^ (
	FUNCTION = mystext_xor,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- cast (text as bool)
--
CREATE OR REPLACE FUNCTION ivymy.text_bool(text, int) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'text_bool'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as bool) 
WITH FUNCTION ivymy.text_bool(text, int) 
AS ASSIGNMENT;

--
-- cast (text as int2)
--
CREATE FUNCTION ivymy.text_int2(text, int, bool, int8) 
	RETURNS pg_catalog.int2
	AS 'MODULE_PATHNAME', 'varlena_int2'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as pg_catalog.int2)
WITH FUNCTION ivymy.text_int2(text, int, bool, int8) 
AS ASSIGNMENT;

--
-- cast (text as int4)
--
CREATE FUNCTION ivymy.text_int4(text, int, bool, int8) 
	RETURNS pg_catalog.int4
	AS 'MODULE_PATHNAME', 'varlena_int4'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as pg_catalog.int4)
WITH FUNCTION ivymy.text_int4(text, int, bool, int8) 
AS ASSIGNMENT;

--
-- cast (text as int8)
--
CREATE FUNCTION ivymy.text_int8(text, int, bool, int8)
	RETURNS pg_catalog.int8
	AS 'MODULE_PATHNAME', 'varlena_int8'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as pg_catalog.int8) 
WITH FUNCTION ivymy.text_int8(text, int, bool, int8) 
AS ASSIGNMENT;

--
-- cast (text as float8)
--
CREATE OR REPLACE FUNCTION ivymy.text_float8(text)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'text_float8'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as pg_catalog.float8) WITH FUNCTION ivymy.text_float8(text) AS IMPLICIT;

--
-- cast (varchar as int2)
--
CREATE FUNCTION ivymy.mys_varchar2int2(varchar)
RETURNS int2
AS 'MODULE_PATHNAME', 'varlena_int2'
LANGUAGE C 
STRICT 
PARALLEL SAFE 
IMMUTABLE;

CREATE CAST(varchar as int2) 
WITH FUNCTION ivymy.mys_varchar2int2(varchar) 
AS ASSIGNMENT;

--
-- cast (varchar as int4)
--
CREATE FUNCTION ivymy.mys_varchar2int4(varchar)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'varlena_int4'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST(varchar as pg_catalog.int4) WITH FUNCTION ivymy.mys_varchar2int4(varchar) AS ASSIGNMENT;

--
-- cast (varchar as int8)
--
CREATE FUNCTION ivymy.mys_varchar2int8(varchar)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME', 'varlena_int8'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST(varchar as pg_catalog.int8) WITH FUNCTION ivymy.mys_varchar2int8(varchar) AS ASSIGNMENT;

-- Begin - ReqID:SRS-MYSQL-SET
--
-- cast (varchar as numeric)
--
create or replace function ivymy.mysvarchar_to_numeric(var pg_catalog.varchar)
returns pg_catalog.numeric
as 'MODULE_PATHNAME', 'mysvarchar_to_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (pg_catalog.varchar AS pg_catalog.numeric) WITH FUNCTION ivymy.mysvarchar_to_numeric(var pg_catalog.varchar) AS ASSIGNMENT;
-- End - ReqID:SRS-MYSQL-SET
