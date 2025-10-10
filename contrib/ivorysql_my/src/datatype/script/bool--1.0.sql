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
