-- string as column alias
select 1 as 'string_alias';

-- TypeCast(::) will ignore ANSI_QUOTES and identify double quotes as IDENT flag always
set sql_mode = 'ANSI_QUOTES';
select 1::text, "this is a string", 1::"text", 1::text;

set sql_mode = '';
select 1::text, "this is a string", 1::"text", 1::text;
\gdesc

-- Begin - bug#6888
CREATE TABLE test_real1(col REAL);
DESC test_real1;

SET sql_mode = 'REAL_AS_FLOAT';
CREATE TEMPORARY TABLE test_real2(col REAL);
DESC test_real2;

drop table test_real1, test_real2;
-- End - bug#6888

set sql_mode = default;

/* begin OA#2799 */
CREATE TABLE proj_income_info (
    id integer PRIMARY KEY,
    pid integer REFERENCES proj_income_info(id)
);

INSERT INTO proj_income_info VALUES (1, NULL), (2, 1);
SELECT * FROM proj_income_info ORDER BY id;

set sql_mode = '';
INSERT INTO proj_income_info VALUES (3, NULL), (4, 2);
SELECT * FROM proj_income_info ORDER BY id;

-- Double backticks are escaped as a backtick.
CREATE TABLE `fk``self` (
    `id``col` integer PRIMARY KEY,
    `pid``col` integer REFERENCES `fk``self` (`id``col`)
);
INSERT INTO `fk``self` VALUES (1, NULL), (2, 1);
SELECT * FROM `fk``self`;

reset sql_mode;
DROP TABLE proj_income_info;
DROP TABLE `fk``self`;
/* begin OA#2799 */