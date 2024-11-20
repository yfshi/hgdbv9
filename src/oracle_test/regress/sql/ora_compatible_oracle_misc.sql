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

/* Begin - BUG#M0000390 */
create table test_m390(id int, name varchar2(24));
insert into test_m390 values(1, 'zhangsan');
insert into test_m390 values(2, 'lisi');
insert into test_m390 values(3, 'wangerma');
select * from test_m390;
delete test_m390 where id = 2;
select * from test_m390;
delete test_m390;
select * from test_m390;
drop table test_m390;
/* End - BUG#M0000390 */

/* Begin - BUG#M0000408 */
create table testm#408$#(c#408$# integer);
insert into testm#408$#(c#408$#) values(123);
select c#408$# from testm#408$#;
drop table testm#408$#;
/* End - BUG#M0000408 */

/* Begin - BUG#M0000424 */
create table test_m424(item varchar2(64), year timestamp, quantity int);
select nvl(item, 'aaa') a , nvl(year, '1990-1-1') b , sum(quantity) from test_m424 group by item, rollup(year);
drop table test_m424;
/* End - BUG#M0000424 */