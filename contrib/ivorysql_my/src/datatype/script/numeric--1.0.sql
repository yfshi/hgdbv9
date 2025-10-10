CREATE FUNCTION ivymy.numeric_cat(numeric, numeric)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.|| (
	procedure = ivymy.numeric_cat,
	leftarg = numeric,
	rightarg = numeric,
	commutator = ||
);


CREATE FUNCTION ivymy.numeric_bool(numeric)
RETURNS bool
AS 'select (!(!$1))::bool;'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE CAST (numeric AS bool)
WITH FUNCTION ivymy.numeric_bool(numeric)
AS ASSIGNMENT;
