--
-- ORA_COMPATIBLE_ORACLE_MISC
-- Tests for Oracle Compatible Features in this file.
-- Most of the tests that appear in this file are Oracle features
-- that do not have individual requirement number.
--

/* Begin - BUG#M0000217 */
create table user_info(id int, uname varchar2(64));
insert into user_info values(1, '111');
insert into user_info values(2, '222');
insert into user_info values(3, '333');
insert into user_info values(4, null);

--  ORDER BY clause supports aliases(Oracle-compatible)
-- success
select id, (case uname when null then null else uname end) as bm from user_info order by bm;
select id, (case uname when null then null else uname end) as bm from user_info order by length(bm);
select id, (case uname when null then null else uname end) as bm from user_info order by length(case uname when null then null else uname end);
select id, uname as id from user_info tbl order by tbl.id;

-- error
select id, uname as id from user_info order by id;
select id, (case uname when null then null else uname end) as id from user_info order by length(id);
select id, (case uname when null then null else uname end) as id from user_info order by id-100;

-- Aliases for sub-query in FROM are optional.
-- OK
select id, uname from (select * from user_info);
select id, uname from (select * from user_info) bm;

-- clean
drop table user_info;
/* End - BUG#M0000217 */
