-- oracle offical test case
create table employees(last_name varchar2(10), salary number, job_id varchar(10));
insert into employees values('Colmenares', 2500, 'PU_CLERK');
insert into employees values('Baida', 2900, 'PU_CLERK');
insert into employees values('Himuro', 2600, 'PU_CLERK');
insert into employees values('Tobias', 2800, 'PU_CLERK');
insert into employees values('Khoo', 3100, 'PU_CLERK');
insert into employees values('Zoom', 2800, 'PU_PUBLIC');
show numw
select last_name, salary, RATIO_TO_REPORT(salary) OVER() AS rr
	FROM employees
	WHERE job_id = 'PU_CLERK'
	ORDER BY last_name,salary,rr;

-- if need show the same scale, using set numw to set
-- after set numw, the result is the same with offical test case
set numw 10
select last_name, salary, RATIO_TO_REPORT(salary) OVER() AS rr
        FROM employees
        WHERE job_id = 'PU_CLERK'
        ORDER BY last_name,salary,rr;

-- reset numw
set numw -1
show numw
drop table employees;

--
-- more test cases
--
create table rtr_test(a int, b float(5), c number, d varchar2(20));
insert into rtr_test values(10, 10, 10.1, '10.1');
insert into rtr_test values(11, 11, 11.1, '11.1');
insert into rtr_test values(12, 11, 12.1, '12.1');

-- using in oracle parser, will fail in pg parser
\parser
set ivorysql.compatible_mode = 'pg';
\parser
select ratio_to_report(a) from rtr_test;
reset ivorysql.compatible_mode;
\parser

-- fail, ratio_to_report() need over()
select ratio_to_report(a) from rtr_test;
-- fail, ratio_to_report() need one args
select ratio_to_report() over() from rtr_test;
select ratio_to_report(a,b) over() from rtr_test;
-- fail, ratio_to_report need ()
-- V9 report an 'syntax error'
-- oracel report 'FROM keyword not found where expected'
select ratio_to_report over() from rtr_test;

-- over() just support partition by or NULL
-- success
select ratio_to_report(d) over() from rtr_test;
select d,ratio_to_report(d) over() AS ratio from rtr_test order by d;
select b,d,ratio_to_report(d) over(partition by b) AS ratio from rtr_test order by b,d;
-- fail as syntax error
select ratio_to_report(d) over(partition by b order by c) from rtr_test;
select ratio_to_report(d) over(order by c) from rtr_test;
select ratio_to_report(d) over(group by c) from rtr_test;

-- explain
explain(verbose, costs off) select ratio_to_report(a) over() from rtr_test;
explain(verbose, costs off) select ratio_to_report(b) over() from rtr_test;
explain(verbose, costs off) select ratio_to_report(c) over() from rtr_test;
explain(verbose, costs off) select ratio_to_report(d) over() from rtr_test;

-- drop table
drop table rtr_test;

-- oracle number
create table rtr_test(n number, f float, bf binary_float, bd binary_double);
insert into rtr_test values(10.101, 11.1111, 12.1212, 12.1212);
insert into rtr_test values(20.202, 21.2121, 22.2222, 22.2222);
insert into rtr_test values(30.303, 31.3131, 33.3333, 33.3333);
-- number
select ratio_to_report(n) over() from rtr_test;
-- float
select f,ratio_to_report(f) over() AS ratio from rtr_test;
-- binary_float
-- sum(binary_float) output binary_float
explain(verbose, costs off) select sys.sum(bf) over() from rtr_test order by bf;
-- ratio_to_report() outoput binary_float
select bf,ratio_to_report(bf) over() from rtr_test order by bf;
explain(verbose, costs off) select bf,ratio_to_report(bf) over() from rtr_test order by bf;
-- binary_double
-- sum(binary_double) output binary_double
explain(verbose, costs off) select sys.sum(bd) over() from rtr_test;
select (bd / sys.sum(bd) over()) from rtr_test;
explain(verbose, costs off) select (bd / sys.sum(bd) over()) from rtr_test;
-- ratio_to_report() output binary_double
select bd,ratio_to_report(bd) over() AS bd_ratio from rtr_test order by bd;
explain(verbose, costs off) select bd,ratio_to_report(bd) over() AS bd_ratio from rtr_test;
-- number + binary_double
select n,bd,ratio_to_report(n+bd) over() AS bd_ratio from rtr_test order by bd;
explain(verbose, costs off) select bd,ratio_to_report(bd) over() AS bd_ratio from rtr_test;
-- expr
select f+2,ratio_to_report(f+2) over() from rtr_test order by f;
select bd*2,ratio_to_report(bd*2) over() AS "BD*2" from rtr_test order by bd;
select ratio_to_report(min(bd)) over() AS ratio from rtr_test;
select ratio_to_report(to_char(bf)) over() AS "TO_CHAR" from rtr_test;
-- partition by
insert into rtr_test values(40.404, 21.2121, 44.4444, 44.444444);
insert into rtr_test values(50.505, 31.3131, 55.5555, 55.555555);
select f,n,ratio_to_report(n) over(partition by f) AS PYF from rtr_test order by f,n;
select f,bf,ratio_to_report(bf) over(partition by f) AS PYF from rtr_test order by f,bf;
-- drop table
drop table rtr_test;

-- oracle character
create table rtr_test(i int, vc varchar2(20), pt varchar2(20));
insert into rtr_test values(10, '10.111111', 'aaaa');
insert into rtr_test values(20, '20.222222', 'aaaa');
insert into rtr_test values(20, '0', 'aaaa');
insert into rtr_test values(30, 'dfg', 'bbbb');
insert into rtr_test values(40, '-10', 'bbbb');
insert into rtr_test values(40, '10', 'bbbb');
insert into rtr_test values(50, '50.55555', 'bbbb');
-- varchar2
select ratio_to_report(vc) over() from rtr_test where i<=20;
-- fail as 'dfg' could not cast to number
select ratio_to_report(vc) over() from rtr_test where i<=30;
-- denominator is zero ( 10 + (-10) = 0 )
select ratio_to_report(vc) over() AS denominator_is_zero from rtr_test where i=40;
-- expr
select vc+1,ratio_to_report(vc+1) over() AS "VC+1" from rtr_test where i<=20 order by vc;
select vc*2,ratio_to_report(vc*2) over() AS "VC*2" from rtr_test where i<=20 order by vc;
select ratio_to_report(max(vc)) over() AS "MAX(VC)" from rtr_test where i<=20;
select ratio_to_report(to_char(vc)) over() AS "TO_CHAR" from rtr_test where i<=20;
-- partition by
select pt,i,ratio_to_report(i) over(partition by pt) from rtr_test order by pt,i;
select i,ratio_to_report(i) over(partition by i) from rtr_test rtr_test order by i;
-- drop table
drop table rtr_test;

-- oracle datetime datatype
create table rtr_test(i int, ts timestamp with time zone, idts interval day to second);
insert into rtr_test values(10,'2026-02-25 10:10:10', INTERVAL '1 03:04:05' DAY TO SECOND);
insert into rtr_test values(20,'2026-02-25 20:20:20', INTERVAL '2 03:04:05' DAY TO SECOND);
-- timestamp, fail
select ratio_to_report(ts) over() from rtr_test;
-- interval, fail
select ratio_to_report(idts) over() from rtr_test;
-- drop table
drop table rtr_test;
