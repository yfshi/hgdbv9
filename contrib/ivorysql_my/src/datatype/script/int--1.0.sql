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