--
-- Test Cases For Global Temporary Table
--

--
-- 1. global temporary table default options
--
create global temporary table g_tmp_noop(a int);
create unique index u_idx on g_tmp_noop(a);
begin;
insert into g_tmp_noop values(generate_series(1, 10000));
insert into g_tmp_noop values(1);
select * from g_tmp_noop where a < 5;
end;
select * from g_tmp_noop where a = 5;

begin;
insert into g_tmp_noop values(generate_series(1, 10000));
set enable_seqscan = false;
explain (costs off) select * from g_tmp_noop where a = 100;
commit;
drop table g_tmp_noop;

create global temporary table g_tmp_noop(a int primary key);
begin;
insert into g_tmp_noop values(generate_series(1, 10000));
set enable_seqscan = false;
explain (costs off) select * from g_tmp_noop where a = 100;
commit;
drop table g_tmp_noop;

create global temporary table g_tmp_noop(a int);
alter table g_tmp_noop add b int;
begin;
insert into g_tmp_noop values(generate_series(1, 10000), generate_series(100, 10000));
select * from g_tmp_noop where a < 5;
end;
drop table g_tmp_noop;

--
-- 2. global temporary table on commit preserve rows
--
create global temporary table g_tmp_pr(a int) ON COMMIT PRESERVE ROWS;
create unique index u_idx on g_tmp_pr(a);
insert into g_tmp_pr values(generate_series(1, 10000));
insert into g_tmp_pr values(1); -- error
set enable_seqscan = false;
select * from g_tmp_pr where a = 100;
explain (costs off) select * from g_tmp_pr where a = 100;
truncate table g_tmp_pr;
drop table g_tmp_pr;

create global temporary table g_tmp_pr(a int primary key) ON COMMIT PRESERVE ROWS;
insert into g_tmp_pr values(generate_series(1, 10000));
insert into g_tmp_pr values(1); -- error
insert into g_tmp_pr values(null); -- error
set enable_seqscan = false;
select * from g_tmp_pr where a = 100;
explain (costs off) select * from g_tmp_pr where a = 100;
truncate table g_tmp_pr;
drop table g_tmp_pr;

create global temporary table g_tmp_pr(a int) ON COMMIT PRESERVE ROWS;
alter table g_tmp_pr add b int;
insert into g_tmp_pr values(generate_series(1, 10000), generate_series(10, 10000));
drop table g_tmp_pr; -- error
truncate table g_tmp_pr;
drop table g_tmp_pr;

create global temp table g_tmp_pr(a int, b text) ON COMMIT PRESERVE ROWS;
insert into g_tmp_pr values(generate_series(1, 10000));
create unique index u_idx on g_tmp_pr(a); -- error
truncate table g_tmp_pr;
create unique index u_idx on g_tmp_pr(a);
alter table g_tmp_pr add c int;
alter table g_tmp_pr add primary key(a);
insert into g_tmp_pr(a, b) values(1, 'abc'), (2, 'abc');
insert into g_tmp_pr(a) values(1); -- error
insert into g_tmp_pr(a, b) values(null, 1); -- error
insert into g_tmp_pr values(1, 'abc', 1); -- error
select * from g_tmp_pr;
update g_tmp_pr set b = 'def';
select * from g_tmp_pr;
delete from g_tmp_pr;
select * from g_tmp_pr;
truncate table g_tmp_pr;
drop table g_tmp_pr;

--
-- 3. global temporary table on commit delete rows
--
create global temporary table g_tmp_dr(a int) ON COMMIT DELETE ROWS;
create unique index u_idx on g_tmp_dr(a);
begin;
insert into g_tmp_dr values(generate_series(1, 10000));
select * from g_tmp_dr where a = 100;
set enable_seqscan = false;
explain (costs off) select * from g_tmp_dr where a = 100;
commit;
drop table g_tmp_dr;

create global temporary table g_tmp_dr(a int) ON COMMIT DELETE ROWS;
create index gdr_idx on g_tmp_dr(a);
alter table g_tmp_dr add b int;
begin;
insert into g_tmp_dr values(generate_series(1, 10000), generate_series(1, 10000));
select * from g_tmp_dr where a < 5;
set enable_seqscan = false;
explain (costs off) select * from g_tmp_dr where a = 100;
end;
drop table g_tmp_dr;

create global temp table g_tmp_dr(a int, b text) ON COMMIT DELETE ROWS;
begin;
insert into g_tmp_dr values(1, 'abc');
select * from g_tmp_dr;
update g_tmp_dr set b = 'def';
select * from g_tmp_dr;
delete from g_tmp_dr;
select * from g_tmp_dr;
end;
drop table g_tmp_dr;

--
-- 4. serial column and identity column
--
create global temp table g_tmp_noop_seq(a int, b SERIAL);
begin;
insert into g_tmp_noop_seq values(generate_series(1, 10));
select * from g_tmp_noop_seq;
commit;

select * from g_tmp_noop_seq;

begin;
insert into g_tmp_noop_seq values(generate_series(1, 5));
select * from g_tmp_noop_seq;
commit;
select * from g_tmp_noop_seq;
drop table g_tmp_noop_seq;

create global temp table g_tmp_pr_seq(a int, b SERIAL) on commit preserve rows;
insert into g_tmp_pr_seq values(generate_series(1, 10));
select * from g_tmp_pr_seq;
truncate g_tmp_pr_seq;
insert into g_tmp_pr_seq values(generate_series(1, 5));
select * from g_tmp_pr_seq;
truncate g_tmp_pr_seq;
drop table g_tmp_pr_seq;

create global temp table g_tmp_dr_seq(a int, b SERIAL) on commit delete rows;
begin;
insert into g_tmp_dr_seq values(generate_series(1, 10));
select * from g_tmp_dr_seq;
commit;

select * from g_tmp_dr_seq;

begin;
insert into g_tmp_dr_seq values(generate_series(1, 5));
select * from g_tmp_dr_seq;
commit;
select * from g_tmp_dr_seq;
drop table g_tmp_dr_seq;

create global temporary table temp2(
	id int generated always as identity,
	col1 varchar2(20),
	col2 varchar2(20)
);
begin;
insert into temp2(col1, col2) values('col1_1', '111111');
insert into temp2(col1, col2) values('col1_2', '222222');
insert into temp2(col1, col2) values('col1_3', '333333');
select * from temp2;
commit;
select * from temp2;

begin;
insert into temp2(col1, col2) values('col1_4', '444444');
insert into temp2(col1, col2) values('col1_5', '555555');
select * from temp2;
commit;
select * from temp2;
drop table temp2;


create global temporary table temp2(
	id int generated always as identity,
	col1 varchar(100),
	col2 varchar(100)
) on commit preserve rows;

insert into temp2(col1, col2) values('col1_1', '111111');
insert into temp2(col1, col2) values('col1_2', '222222');
insert into temp2(col1, col2) values('col1_3', '333333');

select * from temp2;
truncate table temp2;

insert into temp2(col1, col2) values('col1_4', '444444');
insert into temp2(col1, col2) values('col1_5', '555555');

select * from temp2;
truncate table temp2;
drop table temp2;

--
-- 5. Constraints and Foreign key reference
--
CREATE TABLE s1(
    id  INT NOT NULL PRIMARY KEY,
    sp  VARCHAR2(20)
);
CREATE GLOBAL TEMPORARY TABLE t1(
    id  INT,
    sid INT,
    tp  VARCHAR2(20),
    FOREIGN KEY(sid) REFERENCES s1(id) ON DELETE CASCADE
) ON COMMIT PRESERVE ROWS;

DROP TABLE s1;

CREATE GLOBAL TEMPORARY TABLE s1(
    id  INT NOT NULL PRIMARY KEY,
    sp  VARCHAR2(20)
);
CREATE GLOBAL TEMPORARY TABLE t1(
    id  INT,
    sid INT,
    tp  VARCHAR2(20),
    FOREIGN KEY(sid) REFERENCES s1(id) ON DELETE CASCADE
) ON COMMIT PRESERVE ROWS;

DROP TABLE s1;

CREATE GLOBAL TEMPORARY TABLE s1(
    id  INT NOT NULL PRIMARY KEY,
    sp  VARCHAR2(20)
) ON COMMIT PRESERVE ROWS;
CREATE TABLE t1(
    id  INT,
    sid INT,
    tp  VARCHAR2(20),
    FOREIGN KEY(sid) REFERENCES s1(id) ON DELETE CASCADE
);

DROP TABLE s1;

CREATE GLOBAL TEMPORARY TABLE pt4(
    mystr text,
    id    int
) ON COMMIT PRESERVE ROWS;

insert into pt4 values('session 1', 1);
insert into pt4 values('session 1', 2);
insert into pt4 values('session 1', 3);
select * from pt4;
truncate table pt4;
alter table pt4 alter id set not null;
insert into pt4 values('session 1', 6);
insert into pt4 values('session 1', 7);
insert into pt4 values('session 1', null); -- error
select * from pt4;
truncate table pt4;
DROP TABLE pt4;

--
-- 6. Trigger
--
CREATE GLOBAL TEMPORARY TABLE admin_work_area_session(
    id INT,
    class VARCHAR2(20)
) ON COMMIT PRESERVE ROWS;

INSERT INTO admin_work_area_session VALUES(1011, 'class 2');
SELECT * FROM admin_work_area_session;

CREATE OR REPLACE FUNCTION up_trigg_after_stement() RETURNS trigger
AS $$
BEGIN
    RAISE info 'S:After trigger-> Inserting';
    UPDATE admin_work_area_session SET class = 'aaaa';
    RETURN old;
END;
$$ LANGUAGE plpgsql;
/

-- Statement level trigger
CREATE TRIGGER t_after_tt_s AFTER INSERT ON admin_work_area_session
 FOR EACH STATEMENT EXECUTE PROCEDURE up_trigg_after_stement();

INSERT INTO admin_work_area_session VALUES(1056, 'class 22');
SELECT * FROM admin_work_area_session;

DROP TRIGGER t_after_tt_s ON admin_work_area_session;
DROP FUNCTION up_trigg_after_stement;

TRUNCATE TABLE admin_work_area_session;

-- Row level trigger
CREATE OR REPLACE FUNCTION up_trigg_after_row() RETURNS trigger
AS $$
BEGIN
    RAISE info 'R:After trigger-> Inserting';
    RETURN old;
END;
$$ LANGUAGE plpgsql;
/

CREATE TRIGGER t_after_tt_r AFTER INSERT ON admin_work_area_session
  FOR EACH ROW EXECUTE PROCEDURE up_trigg_after_row();

INSERT INTO admin_work_area_session VALUES(1011, 'class 2');
INSERT INTO admin_work_area_session VALUES(1056, 'class 22');
SELECT * FROM admin_work_area_session;

DROP TRIGGER t_after_tt_r ON admin_work_area_session;
DROP FUNCTION up_trigg_after_row;

TRUNCATE TABLE admin_work_area_session;
DROP TABLE admin_work_area_session;

--
-- Test Merge statement
--

CREATE TABLE people_source (
  person_id  INTEGER NOT NULL PRIMARY KEY,
  first_name VARCHAR(20) NOT NULL,
  last_name  VARCHAR(20) NOT NULL,
  title      VARCHAR(10) NOT NULL
);

CREATE GLOBAL TEMPORARY TABLE people_target (
  person_id  INTEGER NOT NULL PRIMARY KEY,
  first_name VARCHAR(20) NOT NULL,
  last_name  VARCHAR(20) NOT NULL,
  title      VARCHAR(10) NOT NULL
)ON COMMIT PRESERVE ROWS;

INSERT INTO people_source VALUES (2, 'Alice', 'Jones', 'Mrs.');
INSERT INTO people_source VALUES (3, 'Jane', 'Doe', 'Miss');
INSERT INTO people_source VALUES (4, 'Dave', 'Brown', 'Mr');

SELECT * FROM people_target;
SELECT * FROM people_source;

-- merge statement
MERGE INTO people_target pt
USING people_source ps
ON    (pt.person_id = ps.person_id)
WHEN NOT MATCHED THEN
  INSERT (person_id, first_name, last_name, title)
   VALUES (ps.person_id, ps.first_name, ps.last_name, ps.title)
;

SELECT * FROM people_target;

TRUNCATE TABLE people_target;
DROP TABLE people_target;
DROP TABLE people_source;

/* Begin BUG#Z376 */
create global temporary table gtt_create_3(id varchar(20), name varchar(50)) on commit preserve rows;
insert into gtt_create_3 values('1', 'aaa');
insert into gtt_create_3 values('2', 'bbb');

select * from gtt_create_3;

create global temporary table gtt_create_3_as on commit preserve rows as select * from gtt_create_3;
select * from gtt_create_3_as;

truncate table gtt_create_3;
truncate table gtt_create_3_as;

drop table gtt_create_3;
drop table gtt_create_3_as;
/* End BUG#Z376 */

/* Begin - BUG#M0000330 */
-- column name and table name are not the same.
create global temporary table t (a int);
insert into t values(1);
select count(*) from t group by t; -- ERROR:  invalid identifier
drop table t;

-- column name and table are the same.
create global temporary table t (t int);
insert into t values(1);
select count(*) from t group by t;
drop table t;

-- group by with rowid
create global temporary table t (a int) with rowid on commit preserve rows;
insert into t values(1);
select count(*) from t group by t; -- ERROR:  invalid identifier
select count(*) from t group by rowid;
truncate table t;
drop table t;
/* End - BUG#M0000330 */

create global temporary table temp_table (
   id number,
   name varchar2(50)
) on commit delete rows;

CREATE INDEX temp_table_idx ON temp_table (id);

-- reindex index
reindex index temp_table_idx; -- error

-- reindex table
reindex table temp_table; -- error
drop table temp_table;

-- truncate multiples
create global temporary table gtt_truncate(id int) on commit preserve rows;
insert into gtt_truncate values(1);
begin;
truncate gtt_truncate;
truncate gtt_truncate;
end;
drop table gtt_truncate;

/* Begin - OA-REQ-1481 */
CREATE GLOBAL TEMPORARY TABLE t_tmp_drr_call_out (
	ID NUMBER(20, 0) NULL,
	CALL_OUT_ID varchar2(4000) NULL
) ON COMMIT PRESERVE ROWS;

insert into t_tmp_drr_call_out values(1,'sjfsjfjsfjsjfsjfsljfslfjlsjfafjlakjsfdljafdajwoejfwjfelwjfslfj1232324324234242432jlwjlfrjslfjlsjafljlsfjjfafjasjfljajfjlajfljfa');
discard all;
CREATE INDEX t_tmp_index ON t_tmp_drr_call_out USING btree (call_out_id);
insert into t_tmp_drr_call_out values(1,'sjfsjfjsfjsjfsjfsljfslfjlsjfafjlakjsfdljafdajwoejfwjfelwjfslfj1232324324234242432jlwjlfrjslfjlsjafljlsfjjfafjasjfljajfjlajfljfa');
discard all;

drop table t_tmp_drr_call_out;
/* End - OA-REQ-1481 */

/* Begin - OA-REQ-1702 */
create global temporary table g_tmp_noop(a int);

create or replace procedure mult_truncate_gtt()
is
	v_sql varchar2(4000);
begin
	execute immediate 'truncate table g_tmp_noop';

	v_sql := 'insert into g_tmp_noop values(generate_series(1, 10000))';

	execute immediate v_sql;
end;
/

call mult_truncate_gtt();

drop procedure mult_truncate_gtt();
drop table g_tmp_noop;
/* End - OA-REQ-1702 */
