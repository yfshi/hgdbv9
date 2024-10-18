/* Operator function (oravarcharchar, oravarcharchar) */
CREATE FUNCTION sys.oravarcharchareq_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharne_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharle_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharchargt_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharlt_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharge_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharchar, oravarcharchar) */
CREATE OPERATOR #= (
	procedure = sys.oravarcharchareq_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oravarcharchargt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oravarcharcharlt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oravarcharcharne_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oravarcharcharge_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oravarcharcharle_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (text, text) */
CREATE FUNCTION sys.texteq_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','texteq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.textne_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','textne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.text_le_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','text_le_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.text_gt_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','text_gt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.text_lt_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','text_lt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.text_ge_explicit(text, text)
RETURNS boolean
AS 'MODULE_PATHNAME','text_ge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


/* CREATE OPERATOR for (text, text) */
CREATE OPERATOR #= (
	procedure = texteq_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = text_gt_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = text_lt_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = textne_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = text_ge_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = text_le_explicit,
	leftarg = text,
	rightarg = text,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharchar, oravarcharbyte) */
CREATE FUNCTION sys.oravarcharchareq_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharne_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharle_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharchargt_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharlt_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharcharge_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharchar, oravarcharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oravarcharchareq_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oravarcharchargt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oravarcharcharlt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oravarcharcharne_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oravarcharcharge_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oravarcharcharle_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharbyte, oravarcharbyte) */
CREATE FUNCTION sys.oravarcharbyteeq_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytene_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytele_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytegt_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytelt_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytege_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharbyte, oravarcharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oravarcharbyteeq_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oravarcharbytegt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oravarcharbytelt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oravarcharbytene_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oravarcharbytege_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oravarcharbytele_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharbyte, oravarcharchar) */
CREATE FUNCTION sys.oravarcharbyteeq_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytene_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytele_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytegt_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytelt_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oravarcharbytege_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharbyte, oravarcharchar) */
CREATE OPERATOR #= (
	procedure = sys.oravarcharbyteeq_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oravarcharbytegt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oravarcharbytelt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oravarcharbytene_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oravarcharbytege_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oravarcharbytele_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oracharchar, oravarcharchar) */
CREATE FUNCTION sys.oracle_char_varchar2_eq_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ne_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_le_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_gt_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_lt_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ge_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oracharchar, oravarcharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracle_char_varchar2_eq_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_char_varchar2_gt_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_char_varchar2_lt_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_char_varchar2_ne_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_char_varchar2_ge_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_char_varchar2_le_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oracharchar, oravarcharbyte) */
CREATE FUNCTION sys.oracle_char_varchar2_eq_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ne_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_le_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_gt_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_lt_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ge_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oracharchar, oravarcharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oracle_char_varchar2_eq_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_char_varchar2_gt_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_char_varchar2_lt_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_char_varchar2_ne_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_char_varchar2_ge_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_char_varchar2_le_explicit,
	leftarg = sys.oracharchar,
	rightarg = sys.oravarcharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oracharbyte, oravarcharchar) */
CREATE FUNCTION sys.oracle_char_varchar2_eq_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ne_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_le_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_gt_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_lt_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ge_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oracharbyte, oravarcharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracle_char_varchar2_eq_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_char_varchar2_gt_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_char_varchar2_lt_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_char_varchar2_ne_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_char_varchar2_ge_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_char_varchar2_le_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oracharbyte, oravarcharbyte) */
CREATE FUNCTION sys.oracle_char_varchar2_eq_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ne_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_le_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_gt_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_lt_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_char_varchar2_ge_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oracharbyte, oravarcharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracle_char_varchar2_eq_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_char_varchar2_gt_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_char_varchar2_lt_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_char_varchar2_ne_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_char_varchar2_ge_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_char_varchar2_le_explicit,
	leftarg = sys.oracharbyte,
	rightarg = sys.oravarcharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharchar, oracharchar) */
CREATE FUNCTION sys.oracle_varchar2_char_eq_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ne_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_le_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_gt_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_lt_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ge_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharchar,oracharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracle_varchar2_char_eq_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_varchar2_char_gt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_varchar2_char_lt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_varchar2_char_ne_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_varchar2_char_ge_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_varchar2_char_le_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharchar, oracharbyte) */
CREATE FUNCTION sys.oracle_varchar2_char_eq_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ne_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_le_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_gt_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_lt_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ge_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharchar,oracharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oracle_varchar2_char_eq_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_varchar2_char_gt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_varchar2_char_lt_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_varchar2_char_ne_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_varchar2_char_ge_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_varchar2_char_le_explicit,
	leftarg = sys.oravarcharchar,
	rightarg = sys.oracharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* Operator function (oravarcharbyte, oracharbyte) */
CREATE FUNCTION sys.oracle_varchar2_char_eq_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ne_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_le_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_gt_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_lt_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ge_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharbyte,oracharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oracle_varchar2_char_eq_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_varchar2_char_gt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_varchar2_char_lt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_varchar2_char_ne_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_varchar2_char_ge_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_varchar2_char_le_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);


/* Operator function (oravarcharbyte, oracharchar) */
CREATE FUNCTION sys.oracle_varchar2_char_eq_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchareq_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ne_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharne_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_le_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharle_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_gt_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarchargt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_lt_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharlt_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.oracle_varchar2_char_ge_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS boolean
AS 'MODULE_PATHNAME','oravarcharge_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

/* CREATE OPERATOR for (oravarcharbyte,oracharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracle_varchar2_char_eq_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracle_varchar2_char_gt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracle_varchar2_char_lt_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracle_varchar2_char_ne_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracle_varchar2_char_ge_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracle_varchar2_char_le_explicit,
	leftarg = sys.oravarcharbyte,
	rightarg = sys.oracharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* create operator for oracharchar and oracharbyte */

/* CREATE OPERATOR for (oracharchar, oracharchar) */
CREATE OPERATOR #= (
	procedure = sys.oracharchareq,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracharchargt,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracharcharlt,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracharcharne,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracharcharge,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracharcharle,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);
 
/* CREATE OPERATOR for (oracharbyte, oracharbyte) */
CREATE OPERATOR #= (
	procedure = sys.oracharbyteeq,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.oracharbytegt,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.oracharbytelt,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.oracharbytene,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.oracharbytege,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.oracharbytele,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* CREATE OPERATOR for (oracharchar, oracharbyte) */
CREATE OPERATOR #= (
	procedure = sys.orachar_byteeq,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.orachar_bytegt,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.orachar_bytelt,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.orachar_bytene,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.orachar_bytege,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.orachar_bytele,
	leftarg = sys.oracharchar,
	rightarg = sys.oracharbyte,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

/* CREATE OPERATOR for (oracharbyte, oracharchar) */
CREATE OPERATOR #= (
	procedure = sys.orabyte_chareq,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #=,
	negator = #<>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

CREATE OPERATOR #> (
	procedure = sys.orabyte_chargt,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #<,
	negator = #<=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #< (
	procedure = sys.orabyte_charlt,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #>,
	negator = #>=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

CREATE OPERATOR #<> (
	procedure = sys.orabyte_charne,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #<>,
	negator = #=,
	restrict = neqsel,
	join = neqjoinsel
);

CREATE OPERATOR #>= (
	procedure = sys.orabyte_charge,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #<=,
	negator = #<,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

CREATE OPERATOR #<= (
	procedure = sys.orabyte_charle,
	leftarg = sys.oracharbyte,
	rightarg = sys.oracharchar,
	commutator = #>=,
	negator = #>,
	restrict = scalarltsel,
	join = scalarltjoinsel
);



/* Hash index support */
CREATE FUNCTION sys.hashoratext(text)
RETURNS integer
AS 'MODULE_PATHNAME','hashoratext'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

/* CREATE OPERATOR FAMILY */
CREATE OPERATOR FAMILY sys.ora_charater_ops_explicit USING hash;
CREATE OPERATOR FAMILY sys.ora_text_ops_explicit USING hash;

/* CREATE OPERATOR CLASS for 'oracharchar' */
CREATE OPERATOR CLASS sys.oracharchar_ops_explicit
    FOR TYPE sys.oracharchar USING hash FAMILY sys.ora_charater_ops_explicit AS
		OPERATOR        1       #= (sys.oracharchar, sys.oracharchar),
        FUNCTION        1       sys.oracharcharhash(sys.oracharchar),
		FUNCTION        2 (sys.oracharchar, sys.oracharchar)       sys.hashoracharcharextended(sys.oracharchar, bigint);

/* CREATE OPERATOR CLASS for 'oracharbyte' */
CREATE OPERATOR CLASS sys.oracharbyte_ops_explicit
    FOR TYPE sys.oracharbyte USING hash FAMILY sys.ora_charater_ops_explicit AS
		OPERATOR        1       #= (sys.oracharbyte, sys.oracharbyte),
        FUNCTION        1       sys.oracharbytehash(sys.oracharbyte),
		FUNCTION        2 (sys.oracharbyte, sys.oracharbyte)       sys.hashoracharbyteextended(sys.oracharbyte, bigint);

/* CREATE OPERATOR CLASS for 'oravarcharchar' */
CREATE OPERATOR CLASS sys.oravarcharchar_ops_explicit
    FOR TYPE sys.oravarcharchar USING hash FAMILY sys.ora_charater_ops_explicit AS
		OPERATOR        1       #= (sys.oravarcharchar, sys.oravarcharchar),
        FUNCTION        1       sys.hashoravarchar(sys.oravarcharchar),
		FUNCTION        2       sys.hashoravarcharcharextended(sys.oravarcharchar, bigint);

/* CREATE OPERATOR CLASS for 'oravarcharbyte' */
CREATE OPERATOR CLASS sys.oravarcharbyte_ops_explicit
    FOR TYPE sys.oravarcharbyte USING hash FAMILY sys.ora_charater_ops_explicit AS
		OPERATOR        1       #= (sys.oravarcharbyte, sys.oravarcharbyte),
        FUNCTION        1       sys.hashoravarchar(sys.oravarcharbyte),
		FUNCTION        2       sys.hashoravarcharbyteextended(sys.oravarcharbyte, bigint);

CREATE OPERATOR CLASS sys.oratext_ops_explicit
    FOR TYPE text USING hash FAMILY sys.ora_text_ops_explicit AS
		OPERATOR        1       #= (text, text),
        FUNCTION        1       sys.hashoratext(text),
		FUNCTION        2       hashtextextended(text, bigint);

ALTER OPERATOR FAMILY sys.ora_charater_ops_explicit USING hash ADD
	-- Cross data type comparison
	OPERATOR        1       #= (sys.oravarcharchar, sys.oravarcharbyte),
	OPERATOR        1       #= (sys.oravarcharbyte, sys.oravarcharchar),
	OPERATOR        1       #= (sys.oracharchar, sys.oracharbyte),
	OPERATOR        1       #= (sys.oracharbyte, sys.oracharchar),
	OPERATOR        1       #= (sys.oracharchar, sys.oravarcharchar),
	OPERATOR        1       #= (sys.oracharchar, sys.oravarcharbyte),
	OPERATOR        1       #= (sys.oracharbyte, sys.oravarcharchar),
	OPERATOR        1       #= (sys.oracharbyte, sys.oravarcharbyte),
	OPERATOR        1       #= (sys.oravarcharchar, sys.oracharchar),
	OPERATOR        1       #= (sys.oravarcharchar, sys.oracharbyte),
	OPERATOR        1       #= (sys.oravarcharbyte, sys.oracharchar),
	OPERATOR        1       #= (sys.oravarcharbyte, sys.oracharbyte);



/* Btree index support */
CREATE FUNCTION sys.bt_oravarchar_cmp_explicit(sys.oravarcharchar, sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_cmp_explicit(sys.oravarcharchar, sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_cmp_explicit(sys.oravarcharbyte, sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_cmp_explicit(sys.oravarcharbyte, sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;



CREATE FUNCTION sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharbyte, sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharchar, sys.oravarcharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharbyte, sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharchar, sys.oravarcharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharbyte, sys.oracharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharbyte, sys.oracharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharchar, sys.oracharbyte)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharchar, sys.oracharchar)
RETURNS integer
AS 'MODULE_PATHNAME','bt_oravarchar_cmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.orabttextcmp_explicit(text, text)
RETURNS integer
AS 'MODULE_PATHNAME','orabttextcmp_explicit'
LANGUAGE C
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION sys.btoravarcharcharsortsupport_explicit(internal)
RETURNS void
AS 'MODULE_PATHNAME','btoravarcharcharsortsupport_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION sys.btoravarcharbytesortsupport_explicit(internal)
RETURNS void
AS 'MODULE_PATHNAME','btoravarcharbytesortsupport_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION sys.btoratextsortsupport_explicit(internal)
RETURNS void
AS 'MODULE_PATHNAME','btoratextsortsupport_explicit'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR FAMILY sys.ora_character_ops_explicit USING btree;
CREATE OPERATOR FAMILY sys.ora_text_ops_explicit USING btree;

/* CREATE OPERATOR CLASS for 'oracharchar' */
CREATE OPERATOR CLASS sys.oracharchar_ops_explicit
    FOR TYPE sys.oracharchar USING btree FAMILY sys.ora_character_ops_explicit AS
        OPERATOR        1       #< (sys.oracharchar, sys.oracharchar),
        OPERATOR        2       #<= (sys.oracharchar, sys.oracharchar),
        OPERATOR        3       #= (sys.oracharchar, sys.oracharchar),
        OPERATOR        4       #>= (sys.oracharchar, sys.oracharchar),
        OPERATOR        5       #> (sys.oracharchar, sys.oracharchar),
        FUNCTION        1       sys.oracharcharcmp(sys.oracharchar, sys.oracharchar),
		FUNCTION        2 (sys.oracharchar, sys.oracharchar)       sys.oracharchar_sortsupport(internal),
		FUNCTION        4 (sys.oracharchar, sys.oracharchar)       sys.btoravarstrequalimage(oid);

/* CREATE OPERATOR CLASS for 'oracharbyte' */
CREATE OPERATOR CLASS sys.oracharbyte_ops_explicit
    FOR TYPE sys.oracharbyte USING btree FAMILY sys.ora_character_ops_explicit AS
        OPERATOR        1       #< (sys.oracharbyte, sys.oracharbyte),
        OPERATOR        2       #<= (sys.oracharbyte, sys.oracharbyte),
        OPERATOR        3       #= (sys.oracharbyte, sys.oracharbyte),
        OPERATOR        4       #>= (sys.oracharbyte, sys.oracharbyte),
        OPERATOR        5       #> (sys.oracharbyte, sys.oracharbyte),
        FUNCTION        1       sys.oracharbytecmp(sys.oracharbyte, sys.oracharbyte),
		FUNCTION        2 (sys.oracharbyte, sys.oracharbyte)       sys.oracharbyte_sortsupport(internal),
		FUNCTION        4 (sys.oracharbyte, sys.oracharbyte)       sys.btoravarstrequalimage(oid);

/* CREATE OPERATOR CLASS for 'oravarcharchar' */
CREATE OPERATOR CLASS sys.oravarcharchar_ops_explicit
    FOR TYPE sys.oravarcharchar USING btree FAMILY sys.ora_character_ops_explicit AS
        OPERATOR        1       #< (sys.oravarcharchar, sys.oravarcharchar),
        OPERATOR        2       #<= (sys.oravarcharchar, sys.oravarcharchar),
        OPERATOR        3       #= (sys.oravarcharchar, sys.oravarcharchar),
        OPERATOR        4       #>= (sys.oravarcharchar, sys.oravarcharchar),
        OPERATOR        5       #> (sys.oravarcharchar, sys.oravarcharchar),
        FUNCTION        1       sys.bt_oravarchar_cmp_explicit(sys.oravarcharchar, sys.oravarcharchar),
        FUNCTION        2 (sys.oravarcharchar, sys.oravarcharchar)       sys.btoravarcharcharsortsupport_explicit(internal),
		FUNCTION        4 (sys.oravarcharchar, sys.oravarcharchar)       sys.btoravarstrequalimage(oid);

/* CREATE OPERATOR CLASS for 'oravarcharbyte' */
CREATE OPERATOR CLASS sys.oravarcharbyte_ops_explicit
    FOR TYPE sys.oravarcharbyte USING btree FAMILY sys.ora_character_ops_explicit AS
        OPERATOR        1       #< (sys.oravarcharbyte, sys.oravarcharbyte),
        OPERATOR        2       #<= (sys.oravarcharbyte, sys.oravarcharbyte),
        OPERATOR        3       #= (sys.oravarcharbyte, sys.oravarcharbyte),
        OPERATOR        4       #>= (sys.oravarcharbyte, sys.oravarcharbyte),
        OPERATOR        5       #> (sys.oravarcharbyte, sys.oravarcharbyte),
        FUNCTION        1       sys.bt_oravarchar_cmp_explicit(sys.oravarcharbyte, sys.oravarcharbyte),	
        FUNCTION        2 (sys.oravarcharbyte, sys.oravarcharbyte)       sys.btoravarcharbytesortsupport_explicit(internal),
		FUNCTION        4 (sys.oravarcharbyte, sys.oravarcharbyte)       sys.btoravarstrequalimage(oid);


/* CREATE OPERATOR CLASS for 'text' */
CREATE OPERATOR CLASS sys.oratext_ops_explicit
    FOR TYPE text USING btree FAMILY sys.ora_text_ops_explicit AS
        OPERATOR        1       #< (text, text),
        OPERATOR        2       #<= (text, text),
        OPERATOR        3       #= (text, text),
        OPERATOR        4       #>= (text, text),
        OPERATOR        5       #> (text, text),
        FUNCTION        1       sys.orabttextcmp_explicit(text, text),	
        FUNCTION        2 (text, text)       sys.btoratextsortsupport_explicit(internal),
		FUNCTION        4 (text, text)       sys.btoravarstrequalimage(oid);

ALTER OPERATOR FAMILY sys.ora_character_ops_explicit USING btree ADD
	-- Cross data type comparison
	OPERATOR        1       #< (sys.oracharchar, sys.oracharbyte),
	OPERATOR        2       #<= (sys.oracharchar, sys.oracharbyte),
	OPERATOR        3       #= (sys.oracharchar, sys.oracharbyte),
	OPERATOR        4       #>= (sys.oracharchar, sys.oracharbyte),
	OPERATOR        5       #> (sys.oracharchar, sys.oracharbyte),
	FUNCTION        1       sys.orachar_bytecmp(sys.oracharchar, sys.oracharbyte),
	FUNCTION        2 (sys.oracharchar, sys.oracharbyte)       sys.oracharchar_sortsupport(internal),
	
	-- Cross data type comparison
	OPERATOR        1       #< (sys.oracharbyte, sys.oracharchar),
	OPERATOR        2       #<= (sys.oracharbyte, sys.oracharchar),
	OPERATOR        3       #= (sys.oracharbyte, sys.oracharchar),
	OPERATOR        4       #>= (sys.oracharbyte, sys.oracharchar),
	OPERATOR        5       #> (sys.oracharbyte, sys.oracharchar),
	FUNCTION        1       sys.orabyte_charcmp(sys.oracharbyte, sys.oracharchar),
	FUNCTION        2 (sys.oracharbyte, sys.oracharchar)       sys.oracharbyte_sortsupport(internal),

	-- Cross data type comparison
	OPERATOR        1       #< (sys.oravarcharchar, sys.oravarcharbyte),
	OPERATOR        2       #<= (sys.oravarcharchar, sys.oravarcharbyte),
	OPERATOR        3       #= (sys.oravarcharchar, sys.oravarcharbyte),
	OPERATOR        4       #>= (sys.oravarcharchar, sys.oravarcharbyte),
	OPERATOR        5       #> (sys.oravarcharchar, sys.oravarcharbyte),
	FUNCTION        1       sys.bt_oravarchar_cmp_explicit(sys.oravarcharchar, sys.oravarcharbyte),
	FUNCTION        2 (sys.oravarcharchar, sys.oravarcharbyte)       sys.btoravarcharcharsortsupport_explicit(internal),
	
	-- Cross data type comparison
	OPERATOR        1       #< (sys.oravarcharbyte, sys.oravarcharchar),
	OPERATOR        2       #<= (sys.oravarcharbyte, sys.oravarcharchar),
	OPERATOR        3       #= (sys.oravarcharbyte, sys.oravarcharchar),
	OPERATOR        4       #>= (sys.oravarcharbyte, sys.oravarcharchar),
	OPERATOR        5       #> (sys.oravarcharbyte, sys.oravarcharchar),
	FUNCTION        1       sys.bt_oravarchar_cmp_explicit(sys.oravarcharbyte, sys.oravarcharchar),
	FUNCTION        2 (sys.oravarcharbyte, sys.oravarcharchar)       sys.btoravarcharbytesortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oracharchar, sys.oravarcharchar),
	OPERATOR        2       #<= (sys.oracharchar, sys.oravarcharchar),
	OPERATOR        3       #= (sys.oracharchar, sys.oravarcharchar),
	OPERATOR        4       #>= (sys.oracharchar, sys.oravarcharchar),
	OPERATOR        5       #> (sys.oracharchar, sys.oravarcharchar),
	FUNCTION        1       sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharchar, sys.oravarcharchar),
	FUNCTION        2 (sys.oracharchar, sys.oravarcharchar)       sys.btoravarcharcharsortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oracharchar, sys.oravarcharbyte),
	OPERATOR        2       #<= (sys.oracharchar, sys.oravarcharbyte),
	OPERATOR        3       #= (sys.oracharchar, sys.oravarcharbyte),
	OPERATOR        4       #>= (sys.oracharchar, sys.oravarcharbyte),
	OPERATOR        5       #> (sys.oracharchar, sys.oravarcharbyte),
	FUNCTION        1       sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharchar, sys.oravarcharbyte),
	FUNCTION        2 (sys.oracharchar, sys.oravarcharbyte)       sys.btoravarcharbytesortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oracharbyte, sys.oravarcharchar),
	OPERATOR        2       #<= (sys.oracharbyte, sys.oravarcharchar),
	OPERATOR        3       #= (sys.oracharbyte, sys.oravarcharchar),
	OPERATOR        4       #>= (sys.oracharbyte, sys.oravarcharchar),
	OPERATOR        5       #> (sys.oracharbyte, sys.oravarcharchar),
	FUNCTION        1       sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharbyte, sys.oravarcharchar),
	FUNCTION        2 (sys.oracharbyte, sys.oravarcharchar)       sys.btoravarcharcharsortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oracharbyte, sys.oravarcharbyte),
	OPERATOR        2       #<= (sys.oracharbyte, sys.oravarcharbyte),
	OPERATOR        3       #= (sys.oracharbyte, sys.oravarcharbyte),
	OPERATOR        4       #>= (sys.oracharbyte, sys.oravarcharbyte),
	OPERATOR        5       #> (sys.oracharbyte, sys.oravarcharbyte),
	FUNCTION        1       sys.bt_orachar_oravarchar_cmp_explicit(sys.oracharbyte, sys.oravarcharbyte),
	FUNCTION        2 (sys.oracharbyte, sys.oravarcharbyte)       sys.btoravarcharbytesortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oravarcharchar, sys.oracharchar),
	OPERATOR        2       #<= (sys.oravarcharchar, sys.oracharchar),
	OPERATOR        3       #= (sys.oravarcharchar, sys.oracharchar),
	OPERATOR        4       #>= (sys.oravarcharchar, sys.oracharchar),
	OPERATOR        5       #> (sys.oravarcharchar, sys.oracharchar),
	FUNCTION        1       sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharchar, sys.oracharchar),
	FUNCTION        2 (sys.oravarcharchar, sys.oracharchar)       sys.btoravarcharcharsortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oravarcharchar, sys.oracharbyte),
	OPERATOR        2       #<= (sys.oravarcharchar, sys.oracharbyte),
	OPERATOR        3       #= (sys.oravarcharchar, sys.oracharbyte),
	OPERATOR        4       #>= (sys.oravarcharchar, sys.oracharbyte),
	OPERATOR        5       #> (sys.oravarcharchar, sys.oracharbyte),
	FUNCTION        1       sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharchar, sys.oracharbyte),
	FUNCTION        2 (sys.oravarcharchar, sys.oracharbyte)       sys.btoravarcharcharsortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oravarcharbyte, sys.oracharchar),
	OPERATOR        2       #<= (sys.oravarcharbyte, sys.oracharchar),
	OPERATOR        3       #= (sys.oravarcharbyte, sys.oracharchar),
	OPERATOR        4       #>= (sys.oravarcharbyte, sys.oracharchar),
	OPERATOR        5       #> (sys.oravarcharbyte, sys.oracharchar),
	FUNCTION        1       sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharbyte, sys.oracharchar),
	FUNCTION        2 (sys.oravarcharbyte, sys.oracharchar)       sys.btoravarcharbytesortsupport_explicit(internal),

	OPERATOR        1       #< (sys.oravarcharbyte, sys.oracharbyte),
	OPERATOR        2       #<= (sys.oravarcharbyte, sys.oracharbyte),
	OPERATOR        3       #= (sys.oravarcharbyte, sys.oracharbyte),
	OPERATOR        4       #>= (sys.oravarcharbyte, sys.oracharbyte),
	OPERATOR        5       #> (sys.oravarcharbyte, sys.oracharbyte),
	FUNCTION        1       sys.bt_oravarchar_orachar_cmp_explicit(sys.oravarcharbyte, sys.oracharbyte),
	FUNCTION        2 (sys.oravarcharbyte, sys.oracharbyte)       sys.btoravarcharbytesortsupport_explicit(internal);
