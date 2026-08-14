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
-- text - float8
--
CREATE OR REPLACE FUNCTION ivymy.mystextfloat8_mi(text, pg_catalog.float8)
	RETURNS pg_catalog.float8
	AS 'MODULE_PATHNAME', 'mystextfloat8_mi'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.- (
	FUNCTION = mystextfloat8_mi,
	LEFTARG = text,
	RIGHTARG = pg_catalog.float8
);

--
-- text * float8
--
CREATE OR REPLACE FUNCTION ivymy.mystextfloat8_mul(text, float8)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mystextfloat8_mul'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.* (
	FUNCTION = mystextfloat8_mul,
	LEFTARG = text,
	RIGHTARG = float8
);

--
-- text / float8
--
CREATE OR REPLACE FUNCTION ivymy.mystextfloat8_div(text, float8)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mystextfloat8_div'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy./ (
	FUNCTION = mystextfloat8_div,
	LEFTARG = text,
	RIGHTARG = float8
);

--
-- float8 + text 
--
CREATE OR REPLACE FUNCTION ivymy.mysfloat8text_pl(float8, text)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mysfloat8text_pl'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.+ (
	FUNCTION = mysfloat8text_pl,
	LEFTARG = float8,
	RIGHTARG = text
);

--
-- float8 - text 
--
CREATE OR REPLACE FUNCTION ivymy.mysfloat8text_mi(float8, text)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mysfloat8text_mi'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.- (
	FUNCTION = mysfloat8text_mi,
	LEFTARG = float8,
	RIGHTARG = text
);

--
-- float8 * text 
--
CREATE OR REPLACE FUNCTION ivymy.mysfloat8text_mul(float8, text)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mysfloat8text_mul'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.* (
	FUNCTION = mysfloat8text_mul,
	LEFTARG = float8,
	RIGHTARG = text
);

--
-- float8 / text 
--
CREATE OR REPLACE FUNCTION ivymy.mysfloat8text_div(float8, text)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'mysfloat8text_div'
	LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy./ (
	FUNCTION = mysfloat8text_div,
	LEFTARG = float8,
	RIGHTARG = text
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

-- /* Begin - BUG#7845 */
--
-- text ~** text
--
CREATE OR REPLACE FUNCTION ivymy.mysql_text_regexeq(text, text)
		RETURNS bool
		AS 'MODULE_PATHNAME', 'mysql_text_regexeq'
		LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.~** (
	FUNCTION = mysql_text_regexeq,
	LEFTARG = text,
	RIGHTARG = text
);

--
-- text !~** text
--
CREATE OR REPLACE FUNCTION ivymy.mysql_text_regexne(text, text)
		RETURNS bool
		AS 'MODULE_PATHNAME', 'mysql_text_regexne'
		LANGUAGE C STRICT IMMUTABLE PARALLEL SAFE;

CREATE OPERATOR ivymy.!~** (
	FUNCTION = mysql_text_regexne,
	LEFTARG = text,
	RIGHTARG = text
);
-- /* End - BUG#7845 */

--
-- cast (text as bool)
--
CREATE OR REPLACE FUNCTION ivymy.text_bool(text, int, bool, int8) 
	RETURNS bool
	AS 'MODULE_PATHNAME', 'text_bool'
	LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as bool) 
WITH FUNCTION ivymy.text_bool(text, int, bool, int8) 
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
CREATE OR REPLACE FUNCTION ivymy.text_float8(text, int, bool, int8)
RETURNS pg_catalog.float8
AS 'MODULE_PATHNAME', 'varlena_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE CAST(text as pg_catalog.float8)
WITH FUNCTION ivymy.text_float8(text, int, bool, int8)
AS ASSIGNMENT;

/* Begin - bug#z5590 */
create or replace function ivymy.text_numeric(text, int, bool, int8) 
returns numeric
as 'MODULE_PATHNAME', 'varlena_numeric'
LANGUAGE C
STRICT
PARALLEL SAFE
IMMUTABLE;

CREATE CAST (text AS numeric)
WITH FUNCTION ivymy.text_numeric(text, int, bool, int8)
AS ASSIGNMENT;
/* End - bug#z5590 */


CREATE FUNCTION ivymy.text_seq(text, text)
returns bool
as 'MODULE_PATHNAME', 'mystextsafeeq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE;

CREATE OPERATOR <=> (
	procedure = ivymy.text_seq,
	leftarg = text,
	rightarg = text
);

/* Begin OA#2701 */
CREATE FUNCTION ivymy.text_eq_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_eq_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.= (
	procedure = ivymy.text_eq_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.text_ne_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_ne_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.<> (
	procedure = ivymy.text_ne_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.text_lt_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_lt_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.< (
	procedure = ivymy.text_lt_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.text_le_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_le_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.<= (
	procedure = ivymy.text_le_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.text_gt_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_gt_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.> (
	procedure = ivymy.text_gt_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.text_ge_float8(text, float8)
RETURNS bool
AS 'MODULE_PATHNAME', 'varlena_ge_float8'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.>= (
	procedure = ivymy.text_ge_float8,
	leftarg = text,
	rightarg = float8
);

CREATE FUNCTION ivymy.float8_lt_text(float8, text)
RETURNS bool
AS 'MODULE_PATHNAME', 'float8_lt_varlena'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.< (
	procedure = ivymy.float8_lt_text,
	leftarg = float8,
	rightarg = text
);

CREATE FUNCTION ivymy.float8_ge_text(float8, text)
RETURNS bool
AS 'MODULE_PATHNAME', 'float8_ge_varlena'
LANGUAGE C STRICT PARALLEL SAFE IMMUTABLE;

CREATE OPERATOR ivymy.>= (
	procedure = ivymy.float8_ge_text,
	leftarg = float8,
	rightarg = text
);

CREATE FUNCTION ivymy.text_sum(float8, text)
RETURNS float8
AS 'MODULE_PATHNAME', 'varlena_sum'
LANGUAGE C PARALLEL SAFE IMMUTABLE;

CREATE AGGREGATE ivymy.sum(text) (
	SFUNC = ivymy.text_sum,
	STYPE = float8
);
/* End OA#2701 */