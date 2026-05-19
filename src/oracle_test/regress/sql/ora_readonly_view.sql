--
-- read onlye view test
--
create table emp_ro(empno int primary key, ename text not null, deptno int, sal numeric);

insert into emp_ro values (1, 'SMITH', 20, 800),(2, 'ALLEN', 30, 1600),(3, 'WARD', 30, 1250),(4, 'JONES', 20, 2975), (5, 'CLARK', 10, 2450);

CREATE VIEW emp_ro_v AS SELECT empno, ename, deptno, sal FROM emp_ro WITH READ ONLY;
SELECT * FROM emp_ro_v ORDER BY empno;

--insert
insert into emp_ro_v values (6, 'MARTIN', 30, 1250);--expect error
insert into emp_ro_v select 6, 'MARTIN', 30, 1250;--expect error

--update
update emp_ro_v set sal= sal + 100 where empno = 1;--expect error

--delete
delete from emp_ro_v where empno = 1;--expect error

--copy
COPY emp_ro_v FROM STDIN;
6 JONES 20 6000
\.


--create or replace view
create or replace view replace_test_v AS SELECT empno, ename, deptno, sal from emp_ro;
insert into replace_test_v values (7, 'BLAKE', 30, 2850);
create or replace view replace_test_v AS SELECT empno, ename, deptno, sal from emp_ro with read only;
insert into replace_test_v values (8, 'SCOTT', 20, 3000);

--test triggers
CREATE TABLE emp_trigger_source(
	empno int PRIMARY KEY,
	ename text
	);

CREATE OR REPLACE FUNCTION trg_func_insert_ro_v()
RETURNS trigger
LANGUAGE plpgsql
AS $$
BEGIN
	INSERT INTO emp_ro_v VALUES (NEW.empno, NEW.ename, 99, 999);
	RETURN NEW;
END;
$$;
/

create trigger trg_attempt_insert_ro_v
AFTER INSERT ON emp_trigger_source
FOR EACH ROW
EXECUTE FUNCTION trg_func_insert_ro_v();
/

INSERT INTO emp_trigger_source VALUES (9, 'Stephon');--error

-- test view depends on a read-only view
create view emp_ro_v_views as select * from emp_ro_v;
insert into emp_ro_v_views values (6, 'James', 20, '3800');--expect error

drop view emp_ro_v_views;
drop view emp_ro_v;
drop view replace_test_v;
drop trigger trg_attempt_insert_ro_v;
drop function trg_func_insert_ro_v;
drop table emp_ro;
drop table emp_trigger_source;
