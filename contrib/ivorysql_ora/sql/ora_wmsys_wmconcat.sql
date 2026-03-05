--
-- Test wmsys.wmconcat function
--

create table t_varchar(
	val varchar2(10)
);

insert into t_varchar values ('AaaaA');
insert into t_varchar values ('BbbbB');
insert into t_varchar values ('CcccC');
insert into t_varchar values ('DdddD');

select * from t_varchar;
--1
select wmsys.wm_concat(val) as result from t_varchar;

--2, distinct
insert into t_varchar values ('CcccC');

select wmsys.wm_concat(DISTINCT val) as result from t_varchar;

create table t_char(
	val char(10)
);

insert into t_char values ('A');
insert into t_char values ('B');
insert into t_char values ('C');
insert into t_char values ('D');

--1,
select wmsys.wm_concat(val) as result from t_char;

--2, distinct
insert into t_char values ('A');
select wmsys.wm_concat(DISTINCT val) as result from t_char;

create table t_int(
	val integer
);

insert into t_int values (10);
insert into t_int values (20);
insert into t_int values (30);
insert into t_int values (40);

--1
select wmsys.wm_concat(val) as result from t_int;

--2, distinct + int
insert into t_int values (30);
select wmsys.wm_concat(DISTINCT val) as result from t_int;

--3, the parameter is an expression
select wmsys.wm_concat(val * 2) as result from t_int;

create table t_num(
	val number(10, 2)
);

insert into t_num values (1.23);
insert into t_num values (4.5);
insert into t_num values (6.789);
insert into t_num values (8.05);

--1,
select wmsys.wm_concat(val) as result from t_num;

--2, distinct + number
insert into t_num values (6.789);
select wmsys.wm_concat(DISTINCT val) as result from t_num;

--3, number expression
select wmsys.wm_concat(val / 2) as result from t_num;

--4, explicitly use TO_CHAR control format
select wmsys.wm_concat(TO_CHAR(val, 'FM9999990.000')) as result from t_num;
