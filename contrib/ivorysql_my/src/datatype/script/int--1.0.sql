/***************************************************************
 *
 * type int1 and int3 operator support
 *
 ***************************************************************/

/* Begin ReqID: SRS-MYSQL-NUMBERICTYPE-INT1 */
alter domain ivymy.tinyint add constraint tinyint_check check(value >= -128 and value <= 127);
--"int1 sign +"
CREATE FUNCTION ivymy.myint1_positive(ivymy.tinyint)
RETURNS ivymy.tinyint
AS 'MODULE_PATHNAME','myint1_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint1_positive,
	rightarg = ivymy.tinyint
);
/* End ReqID: SRS-MYSQL-NUMBERICTYPE-INT1 */

/* Begin - ReqID: SRS-MYSQL-NUMBERICTYPE-INT3 */
alter domain ivymy.mediumint add constraint mediumint_check check(value >= -8388608 and value <= 8388607);

--  "+"
CREATE FUNCTION ivymy.myint3_pl(ivymy.mediumint, ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'int4pl'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint3_pl,
	leftarg = ivymy.mediumint,
	rightarg = ivymy.mediumint
);

--  "-"
CREATE FUNCTION ivymy.myint3_mi(ivymy.mediumint, ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'int4mi'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint3_mi,
	leftarg = ivymy.mediumint,
	rightarg = ivymy.mediumint
);

--"sign +"
CREATE FUNCTION ivymy.myint3_positive(ivymy.mediumint)
RETURNS ivymy.mediumint
AS 'MODULE_PATHNAME','myint3_positive'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.+ (
	procedure = ivymy.myint3_positive,
	rightarg = ivymy.mediumint
);

--"sign -"
CREATE FUNCTION ivymy.myint3_negative(ivymy.mediumint)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','myint3_negative'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.- (
	procedure = ivymy.myint3_negative,
	rightarg = ivymy.mediumint
);
/* End - ReqID: SRS-MYSQL-NUMBERICTYPE-INT3 */

