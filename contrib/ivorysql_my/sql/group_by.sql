-- always ok in mysql
drop table t;
create table t(a int);
insert into t values (1);
insert into t values (2);
select count(*) from t order by a;


-- mysql not support this synatx, pg should error
drop table t;
create table t (x int);
insert into t values (0);
insert into t values (1);
insert into t values (2);
insert into t values (3);
insert into t values (4);
select rank(x) within group (order by x) from t;
set sql_mode = '';
select rank(x) within group (order by x) from t;
reset sql_mode;


