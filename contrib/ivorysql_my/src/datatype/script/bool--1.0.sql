CREATE FUNCTION ivymy.bool_not(bool)
RETURNS int
AS 'select !($1::int);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ! (
	procedure = ivymy.bool_not,
	rightarg = bool
);

/* Begin - bug#6533 */
CREATE OR REPLACE FUNCTION ivymy.bool_float4(bool)
RETURNS float4
AS $$
SELECT $1::int::float4;
$$
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE CAST(bool as float4)
WITH FUNCTION ivymy.bool_float4(bool)
AS ASSIGNMENT;
/* End - bug#6533 */

/* Begin - bug#6531 */
CREATE OR REPLACE FUNCTION ivymy.bool_float8(bool)
RETURNS float8
AS $$
SELECT $1::int::float8;
$$
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE CAST(bool as float8)
WITH FUNCTION ivymy.bool_float8(bool)
AS ASSIGNMENT;
/* End - bug#6531 */

/* Begin - bug#6529 */
CREATE OR REPLACE FUNCTION ivymy.bool_int8(bool)
RETURNS int8
AS $$
SELECT $1::int::int8;
$$
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE CAST(bool as int8)
WITH FUNCTION ivymy.bool_int8(bool)
AS ASSIGNMENT;
/* End - bug#6529 */

/* Begin - bug#6528 */
CREATE OR REPLACE FUNCTION ivymy.bool_numeric(bool)
RETURNS numeric
AS $$
SELECT $1::int::numeric;
$$
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE CAST(bool as numeric)
WITH FUNCTION ivymy.bool_numeric(bool)
AS ASSIGNMENT;
/* End - bug#6528 */