SET plisql.plisql_default_into_strict = false;
SET plisql.variable_conflict = 'use_column';

create table Tree(son varchar2(10), father varchar2(10));
insert into tree values('lyd','lds');
insert into tree values('lhr','lyd');
insert into tree values('lds','lqc');
insert into tree values('dd','lyd');
insert into tree values('lys','lqc');
insert into tree values('lqs','lqc');
insert into tree values('lc','lqs');
insert into tree values('lxm','lqs');
insert into tree values('lxd','lys');
insert into tree values('xd1','lxd');
insert into tree values('xd2','lxd');
insert into tree values('xd2_1','xd2');
insert into tree values('xd2_2','xd2');
insert into tree values('dd1','dd');
select *from tree;
select tree.*,level from tree start with father='lqc' connect by prior son = father;
select tree.*,level from tree connect by prior son = father start with father='lqc' ;
select tree.*,level from tree connect by son = father start with father='lqc';
select tree.*,level from tree connect by prior son = father;
select tree.*,level from tree start with father='lqc' connect by son = father;
select tree.*,level,sys_connect_by_path(father,'=>')from tree start with father='lqc' connect by prior son = father;
select trdee.*,level,sys_connect_by_path(father,'=>') as tree from tree start with father='lqc' connect by prior son = father;
select tree.*,level from tree start with father='lyd' connect by prior son = father;
select t.son,level from tree t start with father='lyd' connect by prior son = father;
select son,level from tree t start with father='lyd' connect by prior son = father;
select  sys_connect_by_path(son,'->'), CONNECT_BY_ROOT father ROOT  from tree start with father='lqc' connect by prior son = father;
select  sys_connect_by_path(son,'->'), CONNECT_BY_ROOT (father) ROOT  from tree start with father='lqc' connect by prior son = father;
select sys_connect_by_path(son,'->'), CONNECT_BY_ROOT (son) ROOT  from tree start with father='lqc' connect by prior son = father;
select level,father,sys_connect_by_path(father,'->'), CONNECT_BY_ROOT (father) ROOT  from tree start with father='lqc' connect by prior son = father;
select tree.*,level from tree start with father='lqc' connect by prior son = father order by 1;
select tree.*,level from tree start with father='lqc' connect by son = prior father;
select tree.*,level from tree start with father='lqc' connect by father = prior son;
select tree.*,level,connect_by_isleaf from tree start with father='lqc' connect by prior son = father;
select tree.*,level,connect_by_isleaf from tree start with father='lqc' connect by NOCYCLE prior son = father;
insert into tree values('lqc','lyd');
select tree.*,level,connect_by_isleaf from tree start with father='lqc' connect by prior son = father;
select tree.*,level,connect_by_isleaf from tree start with father='lqc' connect by NOCYCLE prior son = father;

create table hier
(parents varchar2(20),
child varchar2(20)
);
insert into hier values('Earth','Asia');
insert into hier values('Earth','Australia');
insert into hier values('Earth','Europe');
insert into hier values('Earth','North America');
insert into hier values('Asia','China');
insert into hier values('Asia','Japan');
insert into hier values('Australia','New South Wales');
insert into hier values('New South Wales','Sydney');
insert into hier values('California','Redwood Shores');
insert into hier values('Canada','Ontario');
insert into hier values('China','Beijing');
insert into hier values('England','London');
insert into hier values('Europe','United Kingdom');
insert into hier values('Japan','Osaka');
insert into hier values('Japan','Tokyo');
insert into hier values('North America','Canada');
insert into hier values('North America','USA');
insert into hier values('Ontario','Ottawa');
insert into hier values('Ontario','Toronto');
insert into hier values('USA','California');
insert into hier values('United Kingdom','England');

select *from hier start with parents= 'Earth' connect by prior child = parents;
select parents, child ,level from hier  connect by prior child = parents;
select parents, child ,level from hier start with parents= 'Earth' connect by child = parents;
select level,lpad(' ',level*3)||child child  from hier start with parents ='Earth' connect by prior child = parents;
select parents, child ,level,sys_connect_by_path(child,'->'),connect_by_root(parents) root from hier start with parents= 'Earth' connect by prior child = parents;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by prior child = parents;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by child = prior parents;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by prior parents = child;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root,connect_by_isleaf isleaf from hier connect by prior child = parents;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by NOCYCLE prior child = parents;
insert into hier values('Asia','Earth');
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by prior child = parents;
select parents, child ,level,sys_connect_by_path(child,'->') path,connect_by_root(parents) root from hier connect by NOCYCLE prior child = parents;

drop table Tree;
drop table hier;

--test connect by keywords
--oracle raise error,but we ok
create table connect(id integer);

--oracle raise error, but we ok
create table test_c(connect integer);

INSERT INTO test_c(connect) VALUES(1);

--oracle raise error, but we ok
SELECT connect FROM test_c;


--clean data
DROP TABLE connect;
DROP TABLE test_c;

--failed
create table start(id integer);


--test ok
create or replace function test_level(level integer) return integer is
begin
  return level;
end;
/

select test_level(level=>23);

select * from test_level(level=>23);

drop function test_level(integer);

--test oracle official website example

--init data
CREATE TABLE employees(employee_id integer, last_name varchar(256), manager_id integer);
insert into employees values(101, 'Kochhar', 100);
insert into employees values(108, 'Greenberg', 101);
insert into employees values(109, 'Faviet', 108);
insert into employees values(110, 'Chen', 108);
insert into employees values(111, 'Sciarra', 108);
insert into employees values(112, 'Urman', 108);
insert into employees values(113, 'Popp', 108);
insert into employees values(200, 'Whalen', 101);
insert into employees values(203, 'Mavris', 101);
insert into employees values(204, 'Baer', 101);

--test connect by
SELECT employee_id, last_name, manager_id
   FROM employees
   CONNECT BY PRIOR employee_id = manager_id order by employee_id;


--test level
SELECT employee_id, last_name, manager_id, LEVEL
   FROM employees
   CONNECT BY PRIOR employee_id = manager_id order by employee_id;

--ok
SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY last_name;

--ok
SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY 1;

SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY 2;

--error
SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY 5;

--ok
SELECT last_name as alias_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY alias_name;

--ok
SELECT last_name as alias_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY length(alias_name);

--error
SELECT last_name as alias_name, employee_id, manager_id, LEVEL
      FROM employees hh
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY hh.alias_name;

--error
SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees hh
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY prior last_name;

--oracle ok,but we doesn't support
SELECT last_name, employee_id, manager_id, 'aa' || last_name as aliasname, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY aliasname;

--error
SELECT last_name as alias_name, employee_id as mind, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR mind = manager_id
      ORDER SIBLINGS BY length(alias_name);

--ok
SELECT *
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id
      ORDER SIBLINGS BY 1;


SELECT last_name, employee_id, manager_id, LEVEL
      FROM employees
      START WITH employee_id = 101
      CONNECT BY PRIOR employee_id = manager_id;

insert into employees values(100, 'ABloop', 108);

SELECT last_name "Employee",
   LEVEL, SYS_CONNECT_BY_PATH(last_name, '/') "Path"
   FROM employees
   WHERE level <= 3
   START WITH last_name = 'Kochhar'
   CONNECT BY PRIOR employee_id = manager_id;

--doesn't support connect_by_iscycle
SELECT last_name "Employee", CONNECT_BY_ISCYCLE "Cycle",
   LEVEL, SYS_CONNECT_BY_PATH(last_name, '/') "Path"
   FROM employees
   WHERE level <= 3
   START WITH last_name = 'Kochhar'
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id
   ORDER BY "Employee", "Cycle", LEVEL, "Path";

SELECT last_name "Employee",
   LEVEL, SYS_CONNECT_BY_PATH(last_name, '/') "Path"
   FROM employees
   WHERE level <= 3
   START WITH last_name = 'Kochhar'
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id
   ORDER BY "Employee", LEVEL, "Path";

--test ok
create table warehouses(warehouse_id integer, name varchar(23));
insert into warehouses values(1,'aa');
insert into warehouses values(2,'bb');
insert into warehouses values(3,'cc');

--test ok
SELECT level, warehouse_id, name FROM
   (SELECT ROWNUM r, warehouse_id,name FROM warehouses)
   WHERE CONNECT_BY_ISLEAF = 1
   START WITH r = 1
   CONNECT BY r = PRIOR r + 1
   ORDER BY warehouse_id;


--test ok
SELECT level, warehouse_id, name FROM
   (SELECT ROWNUM r, warehouse_id,name FROM warehouses)
   START WITH r = 1
   CONNECT BY r = PRIOR r + 1
   ORDER BY warehouse_id;

--test ok
SELECT LTRIM(SYS_CONNECT_BY_PATH (warehouse_id,','),',') FROM
   (SELECT ROWNUM r, warehouse_id FROM warehouses)
   WHERE CONNECT_BY_ISLEAF = 1
   START WITH r = 1
   CONNECT BY r = PRIOR r + 1
   ORDER BY warehouse_id;

--test bug#373
set enable_mergejoin = off;
set enable_nestloop = off;

WITH RECURSIVE level__query AS (
SELECT __new_alias__1.r,
__new_alias__1.name,
ARRAY[(ROW(__new_alias__1.r))::text] AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1
WHERE (__new_alias__1.r = 1)
UNION ALL
SELECT __new_alias__1.r,
__new_alias__1.name,
array_append(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text) AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1,
level__query level__query_1
WHERE (cycle_check(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text, false) AND (__new_alias__1.r = (level__query_1.r + 1)))
)
SELECT level__query.r,
level__query.name,
level__query.cte_0_array__path
FROM level__query;

set enable_hashjoin = off;
set enable_mergejoin = on;

WITH RECURSIVE level__query AS (
SELECT __new_alias__1.r,
__new_alias__1.name,
ARRAY[(ROW(__new_alias__1.r))::text] AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1
WHERE (__new_alias__1.r = 1)
UNION ALL
SELECT __new_alias__1.r,
__new_alias__1.name,
array_append(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text) AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1,
level__query level__query_1
WHERE (cycle_check(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text, false) AND (__new_alias__1.r = (level__query_1.r + 1)))
)
SELECT level__query.r,
level__query.name,
level__query.cte_0_array__path
FROM level__query;

set enable_mergejoin = off;
set enable_nestloop = on;

WITH RECURSIVE level__query AS (
SELECT __new_alias__1.r,
__new_alias__1.name,
ARRAY[(ROW(__new_alias__1.r))::text] AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1
WHERE (__new_alias__1.r = 1)
UNION ALL
SELECT __new_alias__1.r,
__new_alias__1.name,
array_append(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text) AS cte_0_array__path
FROM ( SELECT rownum AS r,
warehouses.name
FROM warehouses) __new_alias__1,
level__query level__query_1
WHERE (cycle_check(level__query_1.cte_0_array__path, (ROW(__new_alias__1.r))::text, false) AND (__new_alias__1.r = (level__query_1.r + 1)))
)
SELECT level__query.r,
level__query.name,
level__query.cte_0_array__path
FROM level__query;

set enable_mergejoin = on;
set enable_hashjoin = on;

drop table warehouses;

--test connect_by_root
--its a bug
SELECT last_name "Employee", CONNECT_BY_ROOT last_name "Manager",
   level - 1 "Pathlen", SYS_CONNECT_BY_PATH(last_name, '/') "Path"
   FROM employees
   WHERE LEVEL > 1
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id
   ORDER BY "Employee", "Manager", "Pathlen", "Path";


SELECT last_name "Employee", CONNECT_BY_ROOT last_name "Manager",
   level "Pathlen", SYS_CONNECT_BY_PATH(last_name, '/') "Path"
   FROM employees
   WHERE LEVEL > 1
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id
   ORDER BY "Employee", "Manager", "Pathlen", "Path";


--test group by
SELECT name, SUM(employee_id) "Total_Salary" FROM (
   SELECT CONNECT_BY_ROOT last_name as name,employee_id
      FROM employees
      CONNECT BY NOCYCLE PRIOR employee_id = manager_id)
      GROUP BY name
   ORDER BY name, "Total_Salary";

drop table employees;

--test connect_by_iscycle
CREATE TABLE employees(employee_id integer, last_name varchar(256), manager_id integer);
insert into employees values(101, 'Kochhar', 100);
insert into employees values(108, 'Greenberg', 101);
insert into employees values(109, 'Faviet', 108);
insert into employees values(110, 'Chen', 108);
insert into employees values(111, 'Sciarra', 108);
insert into employees values(112, 'Urman', 108);
insert into employees values(113, 'Popp', 108);
insert into employees values(200, 'Whalen', 101);
insert into employees values(203, 'Mavris', 101);
insert into employees values(204, 'Baer', 101);

SELECT employee_id, last_name, manager_id, connect_by_iscycle "cycle"
   FROM employees
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id start with employee_id = 101;

insert into employees values(100, 'ABloop', 108);

SELECT employee_id, last_name, manager_id, connect_by_iscycle "cycle"
   FROM employees
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id start with employee_id = 101;


--test all pseudocolumn
SELECT employee_id, last_name, manager_id, connect_by_iscycle "cycle", connect_by_isleaf "leaf",
   connect_by_root last_name "manager", SYS_CONNECT_BY_PATH(last_name, '/') FROM employees
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id start with employee_id = 101;

--test connectby level/rownum
select dual.*, level, rownum from dual connect by level < 10;
select dual.*, level, rownum from dual connect by rownum < 10;

create table test_row(name varchar(23));
insert into test_row values('a1');
insert into test_row values('a2');

select test_row.*,level, rownum from test_row connect by level < 3;

--maybe doesn't like oracle
select test_row.*,level, rownum from test_row connect by rownum < 3;


--test pseudocolumn doesn't directly as column
SELECT SYS_CONNECT_BY_PATH(last_name, '/') || 'sjdfljs' "sys_path" FROM employees
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id start with employee_id = 101;

--as function argument
SELECT string_agg(connect_by_iscycle::text,',') "is_cycle", string_agg(connect_by_isleaf::text, '/') "is_leaf" FROM employees
   CONNECT BY NOCYCLE PRIOR employee_id = manager_id start with employee_id = 101;

drop table test_row;
drop table employees;


--test table name is not the same
--failed
select mds.*, level from dual connect by level < 2;

--failed
select sum(test.id), level from dual connect by level < 3;

--test order siblings by
--init data
CREATE TABLE employees(employee_id integer, last_name varchar(256), manager_id integer, first_name varchar(256));
insert into employees values(101, 'Kochhar', 100, 'aaa');
insert into employees values(108, 'Greenberg', 101, 'bbb');
insert into employees values(109, 'Faviet', 108, 'ccc');
insert into employees values(110, 'Chen', 108, 'ddd');
insert into employees values(111, 'Sciarra', 108, 'ddd');
insert into employees values(111, 'Sciarra', 108, 'aaa');
insert into employees values(112, 'Urman', 108, 'ddd');
insert into employees values(113, 'Popp', 108, 'ddd');
insert into employees values(200, 'Whalen', 101, 'eee');
insert into employees values(203, 'Mavris', 101, 'fff');
insert into employees values(204, 'Baer', 101, 'ggg');

--common select doesn't support order siblings by
select * from employees order siblings by last_name;

select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id;

 --test order by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id order by last_name;


--test order siblings by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id order siblings by last_name;


--test two column order siblings by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id order siblings by last_name, first_name;


select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id order siblings by last_name, first_name desc;


select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 prior employee_id = manager_id order siblings by last_name desc, first_name;


--test conect by (prior xxx)
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 (manager_id = prior employee_id);

 --test order by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
(prior employee_id = manager_id) order by last_name;


--test order siblings by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 (prior employee_id = manager_id) order siblings by last_name;


--test two column order siblings by
select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 (manager_id = prior employee_id) order siblings by last_name, first_name;


select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 (prior employee_id = manager_id) order siblings by last_name, first_name desc;


select employee_id, last_name, manager_id,level, first_name from employees start with employee_id = 101 connect by
 (manager_id = prior employee_id) order siblings by last_name desc, first_name;

drop table employees;

--test by rownum
select level,rownum from dual connect by rownum < 1;

select level,rownum from dual connect by rownum < 2;

select level,rownum from dual connect by rownum < 3;

select level,rownum from dual connect by rownum < 4;

select level,rownum from dual connect by rownum > 5;

select *,level,rownum from dual connect by rownum < 9;

select dual.*,level,rownum from dual connect by rownum < 9;

create table test1_row(id integer, name varchar(23));

select id,name,level,rownum from test1_row connect by rownum < 1;

insert into test1_row values(1,'aa');

select id,name,level,rownum from test1_row connect by rownum < 1;

select id,name,level,rownum from test1_row connect by rownum < 2;

select id,name,level,rownum from test1_row connect by rownum < 3;

insert into test1_row values(2,'bb');

select id,name,level,rownum from test1_row connect by rownum < 1;

select id,name,level,rownum from test1_row connect by rownum < 2;

select id,name,level,rownum from test1_row connect by rownum < 3;

select id,name,level,rownum from test1_row connect by rownum < 4;

insert into test1_row values(3, 'cc');

select id,name,level,rownum from test1_row connect by rownum < 1;

select id,name,level,rownum from test1_row connect by rownum < 2;

select id,name,level,rownum from test1_row connect by rownum < 3;

select id,name,level,rownum from test1_row connect by rownum < 4;

select id,name,level,rownum from test1_row connect by rownum < 4 start with id = 1;

insert into test1_row values(1,'dd');

select id,name,level,rownum from test1_row connect by rownum < 4 start with id = 1;

drop table test1_row;

--test prior operator
create table employee(
       emp_id number(18),
       lead_id number(18),
       emp_name varchar2(200),
       salary number(10,2),
       dept_no varchar2(8)
);

create table test_connectby( emp_id number(18),
       lead_id number(18),
       emp_name varchar2(200),
       salary number(10,2),
       dept_no varchar2(8));

insert into test_connectby values('1',0,'king','1000000.00','001');
insert into test_connectby values('2',1,'jack','50500.00','002');
insert into test_connectby values('3',1,'arise','60000.00','003');
insert into test_connectby values('4',2,'scott','30000.00','002');
insert into test_connectby values('5',2,'tiger','25000.00','002');
insert into test_connectby values('6',3,'wudde','23000.00','003');
insert into test_connectby values('7',3,'joker','21000.00','003');

insert into employee values('1',0,'king','1000000.00','001');
insert into employee values('2',1,'jack','50500.00','002');
insert into employee values('3',1,'arise','60000.00','003');
insert into employee values('4',2,'scott','30000.00','002');
insert into employee values('5',2,'tiger','25000.00','002');
insert into employee values('6',3,'wudde','23000.00','003');
insert into employee values('7',3,'joker','21000.00','003');

--test ok
select level, prior emp_name as lead_name, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_name, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior employee.emp_name, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test failed
select level,emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1 where prior emp_name = 'king';

--test ok
select level,emp_name from employee  where emp_name = 'king' connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level,emp_name from employee  where prior emp_name = 'king' connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level,emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select connect_by_root emp_name, connect_by_root emp_id from employee connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok 
select sys_connect_by_path(emp_name, '$$'), sys_connect_by_path(emp_id, '@@') from employee connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select connect_by_root emp_name, connect_by_root emp_id from employee  where connect_by_root emp_id = 1 connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select connect_by_root emp_name, connect_by_root emp_id from employee connect by prior emp_id = lead_id start with emp_id = 1 order by connect_by_root emp_name;

--test ok
select connect_by_root emp_name, connect_by_root emp_id from employee connect by prior emp_id = lead_id start with emp_id = 1 order by sys_connect_by_path(emp_name, '@@');

--failed
select connect_by_root emp_name, connect_by_root emp_id from employee  where sys_connect_by_path(emp_id,'@@') = 'xiexie' connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select connect_by_root emp_name, connect_by_root employee.emp_id from employee connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select sys_connect_by_path(emp_name, '&'), sys_connect_by_path(employee.emp_id,'@@') from employee connect by prior emp_id = lead_id start with emp_id = 1 order by emp_name;

--test ok
select level,emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1 order by prior emp_name, emp_name;

--test ok
select level,emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1 order by prior emp_id + 1,emp_name;

--test failed
select level,emp_name from employee order by emp_name connect by prior emp_id = lead_id start with emp_id = 1;

--test failed
select * from employee order by prior emp_name;

--test prior prior
--test failed
select level, prior (prior emp_name) as lead_name, emp_name from employee connect by prior (emp_id + 1) = lead_id start with emp_id = 1;

--test failed
select level, prior prior emp_name as lead_name, emp_name from employee connect by prior (emp_id + 1) = lead_id start with emp_id = 1;

--test failed
select level, prior emp_name as lead_name, emp_name from employee connect by prior (prior emp_id + 1) = lead_id start with emp_id = 1;

--test ok
select level, prior emp_name as lead_name, emp_name from employee connect by prior (emp_id + 1) = lead_id start with emp_id = 1;

--test failed
select level, prior (level + 1) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test failed
select level, prior (rownum + 1) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test failed
select level, prior emp_name as lead_name, emp_name from employee connect by prior (level + 1) = lead_id start with emp_id = 1;

--test ok
select emp_name from employee connect by level < 1 and prior (emp_id + 1) = lead_id;

--test ok
select level, prior emp_name as lead_name, emp_name from employee connect by level < 100 and prior emp_id = lead_id start with emp_id = 1;

insert into employee values('2',4,'tigers','25000.00','002');
--test failed --loop failed
select level, prior emp_name as lead_name, emp_name from employee connect by level < 100 and prior emp_id = lead_id start with emp_id = 1;

delete from employee where emp_id = 2 and lead_id = 4;

--test ok
select level, prior emp_name as lead_name, emp_name from employee connect by level < 100 and  emp_id = lead_id start with emp_id = 1;

create or replace function str_concat(id varchar) return varchar is
begin
  return 'xiexie';
end;
/

--test failed
select prior emp_name, emp_name, prior str_concat(prior emp_name) from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select prior emp_name, emp_name, prior str_concat(emp_name) from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select prior emp_name, emp_name, str_concat(prior emp_name) from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, str_concat(prior emp_name) as pname, emp_name from employee connect by prior employee.emp_id = prior employee.lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, prior str_concat(emp_name) as pname, emp_name from employee connect by prior employee.emp_id = prior employee.lead_id start with emp_id = 1;


drop function str_concat(varchar);


select *,level from employee connect by nocycle prior emp_id in (select emp_id from employee where emp_id > 2) order siblings by emp_id;

select emp_id,CONNECT_BY_ISCYCLE as c1 from employee connect by nocycle emp_id <= lead_id order siblings by emp_id;

--test failed
select level, prior emp_name as lead_name, emp_name from employee connect by prior emp_id = lead_id start with prior emp_id = 1;

--test failed
select level, prior emp_name as lead_name, emp_name from employee connect by prior level < 2;

--test ok
select level, prior emp_name as lead_name, emp_name from employee connect by level < 2;

--test faield
select level, prior emp_name as lead_name, emp_name from employee connect by  prior (level < 2);

--test ok
select level, prior emp_id as lead_emp_id, prior (emp_id + 1) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, prior (emp_id + emp_id) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, (emp_id + prior emp_id) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, emp_name,rownum from employee connect by rownum < 5 and prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, prior (2 + 1) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, (prior emp_id) lead_emp_id, prior (2 + 1) as plevel, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, prior (2 + 1) as plevel, emp_name from employee connect by prior emp_id = prior lead_id start with emp_id = 1;

--test ok
select level, prior emp_id as lead_emp_id, prior (2 + 1) as plevel, emp_name from employee connect by prior employee.emp_id = prior employee.lead_id start with emp_id = 1;

--test failed
select prior emp_name as lead_name,emp_name from test_connectby;

--test bug#0000294
--test ok
select emp_id, level from employee connect by nocycle prior emp_id = lead_id start with emp_id >1 and level is not null order siblings by emp_id;

--test ok
select emp_id, level from employee connect by nocycle prior emp_id = lead_id start with emp_id >1 and level < 6 order siblings by emp_id;

--test bug#0000293, oracle test ok but we doesn't support keyword as alias
--test ok
select emp_id, level, connect_by_iscycle nocycle from employee connect by nocycle prior emp_id = lead_id start with emp_id >1 and level < 6 order siblings by emp_id;

--test bug#0000292
select emp_id, level, connect_by_iscycle  from employee where connect_by_iscycle = 1 connect by nocycle prior emp_id = lead_id start with emp_id >1 and level < 6 order siblings by emp_id;

--test bug#0000290
select emp_id, level, (connect_by_root emp_id)  from employee where (connect_by_root emp_id) = 1 connect by prior emp_id = lead_id start with emp_id between 10 and 20 order siblings by emp_id;

--test failed
select emp_id, level from employee connect by prior emp_id = lead_id  and (connect_by_root emp_id) = 1 start with emp_id between 10 and 20  order siblings by emp_id;

--test failed
select emp_id, level from employee connect by prior emp_id = lead_id start with (connect_by_root emp_id) = 1 order siblings by emp_id;


--test bug#0000288
select emp_id ,CONNECT_BY_ISCYCLE as c1 from employee connect by nocycle prior employee.emp_id = employee.lead_id start with emp_id = 1;

--test bug#0000287
select emp_id, (prior emp_id) ids from employee connect by prior emp_id = lead_id start with emp_id = 1;

create table employeess(
       nocycle number(18),
       lead_id number(18),
       emp_name varchar2(200),
       salary number(10,2),
       dept_no varchar2(8)
);

--test ok
insert into employeess values(1,0,'aa', 2,'2');
insert into employeess values(2,1,'bb',2,'2');
select nocycle, emp_name,prior nocycle from employeess connect by nocycle nocycle = prior lead_id;

drop table employeess;
drop table employee;
drop table test_connectby;

--test bug#337
create table h1(c1 int,c2 int);
insert into h1 values(1,1);
insert into h1 values(2,2);
insert into h1 values(3,3);
select * from h1 where prior(c1) is not null connect by nocycle prior (c1)= prior(c2);

drop table h1;

--test bug#292
CREATE TABLE connectemp(employee_id integer, last_name varchar(256), manager_id integer);
insert into connectemp values(101, 'Kochhar', 100);
insert into connectemp values(108, 'Greenberg', 101);
insert into connectemp values(109, 'Faviet', 108);
insert into connectemp values(110, 'Chen', 108);
insert into connectemp values(111, 'Sciarra', 108);
insert into connectemp values(112, 'Urman', 108);
insert into connectemp values(113, 'Popp', 108);
insert into connectemp values(200, 'Whalen', 101);
insert into connectemp values(203, 'Mavris', 101);
insert into connectemp values(204, 'Baer', 101);
insert into connectemp values(100, 'ABloop', 108);
insert into connectemp values(100, 'ABloop', 110);

SELECT employee_id, manager_id, connect_by_iscycle "cycle"
FROM connectemp where connect_by_iscycle =1 CONNECT BY NOCYCLE prior employee_id = manager_id start with employee_id >100;

SELECT employee_id, manager_id, connect_by_iscycle "cycle"
FROM connectemp where connect_by_iscycle = 1 CONNECT BY NOCYCLE prior employee_id = manager_id start with employee_id >100 order siblings by employee_id desc;

SELECT employee_id, manager_id, connect_by_iscycle "cycle"
FROM connectemp where connect_by_iscycle = 1 CONNECT BY NOCYCLE prior employee_id = manager_id start with employee_id >100 order siblings by employee_id;

SELECT employee_id, manager_id, connect_by_iscycle "cycle"
FROM connectemp where connect_by_iscycle =1 CONNECT BY NOCYCLE prior employee_id = manager_id start with employee_id = 101;

SELECT employee_id, manager_id, connect_by_iscycle "cycle"
FROM connectemp where connect_by_iscycle =1 CONNECT BY NOCYCLE prior employee_id = manager_id start with employee_id = 108;

select employee_id, manager_id,level,connect_by_iscycle, connect_by_isleaf from (select * from connectemp) connect by nocycle prior employee_id = manager_id start with employee_id > 100 order siblings by last_name;

drop table connectemp;

--test priorxx as a column
create table employee(
       emp_id number(18),
       lead_id number(18),
       emp_name varchar2(200),
       salary number(10,2),
       prioremp_id number(18),
       dept_no varchar2(8)
);

insert into employee values('1',0,'king','1000000.00','10001','001');
insert into employee values('2',1,'jack','50500.00','10002','002');
insert into employee values('3',1,'arise','60000.00','10003','003');
insert into employee values('4',2,'scott','30000.00','10004','002');
insert into employee values('5',2,'tiger','25000.00','10005','002');
insert into employee values('6',3,'wudde','23000.00','10006','003');
insert into employee values('7',3,'joker','21000.00','10007','003');

select level, prior emp_id, prioremp_id, emp_name from employee connect by prior emp_id = lead_id start with emp_id = 1;

select public.employee.emp_id, level, prior public.employee.emp_name from public.employee connect by prior emp_id = lead_id start with emp_id = 1;

drop table employee;

--test more than two tables
create table h1(c1 integer, name varchar(23));
create table h2(c1 integer, name varchar(23));

insert into h1 values(1,'aa');
insert into h1 values(2,'bb');
insert into h1 values(3,'cc');
insert into h1 values(4,'dd');

insert into h2 values(1,'aa');
insert into h2 values(2,'bb');
insert into h2 values(3,'cc');
insert into h2 values(4,'dd');

--test ok
select * from h1,h1 hh where h1.c1=hh.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok
select level, h1.c1,h2.c1,prior h1.c1, prior h2.c1 from h1,h2  where h1.c1=h2.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h2.c1) start with h1.c1 =1;

--test ok
select level, h1.c1, h2.c1 from h1 left join h2 on h1.c1 = h2.c1 connect by nocycle prior h1.c1 = prior h1.c1 start with h1.c1 = 1;

--test ok no rows
select * from h1,h1 hh where h1.c1=hh.c1 and h1.c1 = h1.c1 + 1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 48 rows
select * from h1,h1 hh where h1.c1=hh.c1 or h1.c1 = hh.c1 + 1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok no rows
select * from h1,h1 hh where h1.c1=hh.c1 and (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 48 rows
select * from h1,h1 hh where h1.c1=hh.c1 or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 49 rows
select * from h1,h1 hh where h1.c1 = 1 and h1.c1=hh.c1 or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 8 rows
select * from h1,h1 hh where h1.c1 = hh.c1  or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 158 rows
select * from h1,h1 hh where h1.c1 = hh.c1  or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 158 rows
select * from h1,h1 hh where h1.c1 = hh.c1  or (prior(h1.c1) >2 and h1.c1 = hh.c1 + 1) or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 45 rows
select * from h1,h1 hh where h1.c1 = hh.c1  or (h1.c1 = hh.c1 + 1 and h1.c1 = hh.c1 + 3 or prior(h1.c1) >2) or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 49 rows
select * from h1,h1 hh where h1.c1 = 1 or h1.c1=hh.c1 or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 48 rows
select * from h1,h1 hh where h1.c1=hh.c1 or (h1.c1 = hh.c1 + 1 and prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 1280 rows
select * from h1,h1 hh where prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 1972 rows
select * from h1,h1 hh connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 0 rows
select * from h1,h1 hh where h1.c1 > hh.c1 and (prior(h1.c1) >2 and h1.c1 = hh.c1) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 0 rows
select * from h1,h1 hh where h1.c1 > hh.c1 and (prior(h1.c1) >2 or h1.c1 = hh.c1) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 134 rows
select * from h1,h1 hh where h1.c1 > hh.c1 or h1.c1 = hh.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok no rows
select * from h1,h1 hh where h1.c1 > hh.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 8 rows
select * from h1,h1 hh where h1.c1=hh.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test ok 201 rows
select * from h1,h1 hh where prior(h1.c1) >2 and (h1.c1>1  or h1.c1 = hh.c1) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 201 rows
select * from h1,h1 hh where prior(h1.c1) >2 and (h1.c1>1  or h1.c1 = hh.c1) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 201 rows
select * from h1,h1 hh where prior(h1.c1) >2 and (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 145 rows
select * from h1,h1 hh where prior(h1.c1) >2  and h1.c1 = prior(hh.c1) and (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 201 rows
select * from h1,h1 hh where prior(h1.c1) >2  and h1.c1 + hh.c1 = 2 and (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from public.h1,h1 hh where h1.c1 = 1 or h1.c1 = public.h1.c1 + 1 or (h1.c1 = hh.c1 + 1 or prior(h1.c1) >2) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1;

--test function argument has columns
create or replace function test_join(id integer) return integer is
begin
  return id;
end;
/
--test ok 15 rows
select * from h1,h2 where test_join(h1.c1) = test_join(h2.c1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

drop function test_join(integer);

create or replace function test_join(id integer,id2 integer) return integer is
begin
  if (id = id2) then
     return 0;
  else
     return 1;
  end if;
end;
/

--test ok 15 rows
select * from h1,h2 where test_join(h1.c1, h2.c1) = 0 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 120 rows
select * from h1,h2 where test_join(h1.c1, prior h2.c1) = 0 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where test_join(h1.c1, prior h2.c1) = 0 and test_join(h1.c1, h2.c1) = 0 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select level, prior h1.name, h2.name, connect_by_root h1.c1, sys_connect_by_path(h2.c1, '@@') from h1,h2 where test_join(h1.c1, h2.c1) = 0 connect by nocycle prior (h1.c1) = prior(h1.c1)
and h1.c1 > 1 start with h1.c1 > 1;

drop function test_join(integer, integer);
drop table h1;
drop table h2;

--test bug#0000408
create table h1(c1 integer, name varchar(23));
create table h2(c1 integer, name varchar(23));

insert into h1 values(1,'aa');
insert into h1 values(2,'bb');
insert into h1 values(3,'cc');
insert into h1 values(4,'dd');

insert into h2 values(1,'aa');
insert into h2 values(2,'bb');
insert into h2 values(3,'cc');
insert into h2 values(4,'dd');

--test ok 225 rows
select * from h1,h2 where not (h1.c1 = h2.c1 and h1.c1 = 23) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 225 rows
select * from h1,h2 where not (h1.c1 = h2.c1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where not (h1.c1 = 1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where h1.c1 = h2.c1 and not (h1.c1 = 1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where h1.c1 = h2.c1 and not (h1.c1 = h2.c1 + 1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 164 rows
select * from h1,h2 where h1.c1 = 2 and not (h1.c1 = h1.c1 + 1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where h1.c1 is not null connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where h1.c1 + h2.c1 is not null connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.c1 is  null connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok
create or replace function test_join(id integer,id2 integer) return integer is
begin
  if (id = id2) then
     return 0;
  else
     return 1;
  end if;
end;
/

--test ok no rows
select * from h1,h2 where test_join(h1.c1, 23) = 0  connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where test_join(h1.c1, h2.c1) = 0  connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where test_join(h1.c1, h2.c1)  is null connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where test_join(h1.c1, 23)  is null connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where h1.c1::boolean is true connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where (h1.c1 + h2.c1)::boolean  is false connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 64 rows
select * from h1,h2 where h1.c1 between h2.c1 + 1 and h2.c1 + 100 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.c1 between h1.c1 + 1 and h2.c1 + 100 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.c1 between h1.c1 + 1 and h1.c1 + 100 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where h1.c1 not between h1.c1 + 1 and h1.c1 + 100 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.c1 not between h2.c1 + 1 and h1.c1 + 100 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 10 rows
select * from h1,h2 where h1.c1 between h2.c1 + 1 and h1.c1 + 100 and h1.c1 = h2.c1 + 1 and h2.c1 > 1 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 225 rows
select * from h1,h2 where test_join(h1.c1, 23) = test_join(h2.c1,1) or test_join(h1.c1,23) = test_join(h1.c1,23) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where test_join(h1.c1, 23) not in (h2.c1) or test_join(h1.c1,23) in (h1.c1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where test_join(h1.c1, 23) not in (23,h2.c1) or test_join(h1.c1,23) in (h2.c1,h1.c1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where h1.name like h2.name connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.name like 'xiexie' connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.name like 'xiexie' and h1.name not like h2.name connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where h1.name in (select name from h1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where 1= 1 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 492 rows
select * from h1,h2 where test_join(h1.c1, 23) not in (23,h2.c1,h1.c1 + 1) or test_join(h1.c1,23) in (h2.c1,h1.c1,h2.c1 + 23) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test bug#0000435
--test ok 123 rows
select * from h1,h2 where h1.name in (select h3.name from h2 h3 where h3.name = h2.name) and h1.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 164 rows
select * from h1,h2 where h1.name in (select 'cc'::text from h1) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 164 rows
select * from h1,h2 where h1.name in (select 'cc'::text from h1 where h1.name = h2.name) connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 41 rows
select * from h1,h2 where h1.c1 + h2.c1 in (select 3 from h1)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok no rows
select * from h1,h2 where h1.c1 + h2.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name) and h1.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test failed
select * from h1,h2 where h1.name in (select h3.name from h2 h3 where h3.name = prior h2.name) and h1.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test failed
select * from h1,h2 where h1.name in (select h3.name from h2 h3 where level < 1) and h1.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test failed
select * from h1,h2 where h1.name in (select prior h1.name from h2 h3) and h1.c1 in (select h3.c1 from h2 h3 where h3.name = h2.name)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;


--test ok 123 rows
select * from h1,h2 where h1.name in (select  h2.name from h2 h3)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 123 rows
select * from h1,h2 where h2.name in (select  h1.name from h2)
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where h2.name in (select  h1.name from h2) and h1.name = h2.name or h1.c1 < 4
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test 10 rows
select * from h1,h2 where h2.c1 + h1.c1 = any (select h1.c1 from h2) and h1.name = h2.name or h1.c1 < 4
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

--test ok 15 rows
select * from h1,h2 where h2.name = all (select h1.name from h2) and h1.name = h2.name or h1.c1 < 4
 connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1;

drop function test_join(integer, integer);
drop table h1;
drop table h2;

--test bug#0000530
create table GT_GET_VALID_KM_MATERIAL(ITEMCODE integer, P_ITEMCODE integer);
create table HZBB_TAB_DBDX(DZID integer, PZH varchar(23));

SELECT DISTINCT T.ITEMCODE ,CONNECT_BY_ROOT T.ITEMCODE
FROM GT_GET_VALID_KM_MATERIAL T
CONNECT BY ITEMCODE = PRIOR P_ITEMCODE
START WITH ITEMCODE IN (SELECT DZID FROM HZBB_TAB_DBDX WHERE PZH IS NOT NULL);

drop table GT_GET_VALID_KM_MATERIAL;
drop table HZBB_TAB_DBDX;


--test bug#0000538
create table Tree(son varchar2(10), father varchar2(10));

insert into tree values('lyd','lds');
insert into tree values('lhr','lyd');
insert into tree values('lds','lqc');
insert into tree values('dd','lyd');
insert into tree values('lys','lqc');

select sys_connect_by_path(father || 'xiexie','=>') from tree start with father='lqc' connect by prior son = father;
select connect_by_root(father || 'xiexie') from tree start with father='lqc' connect by prior son = father;
select connect_by_root father || 'xiexie' from tree start with father='lqc' connect by prior son = father;
select connect_by_root (father || 'xiexiesfsfdsfdsfsfs') from tree start with father='lqc' connect by prior son = father;

drop table tree;

--test bug#446
create table tree(id number , father number, grandfather number,grand_siblings number, fat_siblings number);

insert into tree values(1 , 0 , 0 , 0 , 0);
insert into tree values(2 , 1 , 0 , 1 , 1);
insert into tree values(4 , 2 , 1 , 1 , 1);
insert into tree values(5 , 2 , 1 , 1 , 1);
insert into tree values(6 , 2 , 1 , 1 , 1);
insert into tree values(10 , 4 , 2 , 1 , 3);
insert into tree values(11 , 4 , 2 , 1 , 3);
insert into tree values(12 , 5 , 2 , 2 , 3);
insert into tree values(13 , 5 , 2 , 2 , 3);
insert into tree values(16 , 10, 4 , 3 , 2);
insert into tree values(17 , 6 , 2 , 1 , 3);
insert into tree values(20 , 11, 4 , 3 , 2);

create table sibtree(id number , father number, grandfather number);

insert into sibtree values(3 , 0 , 0 );
insert into sibtree values(7 , 3 , 0 );
insert into sibtree values(8 , 3 , 0 );
insert into sibtree values(9 , 7 , 3 );
insert into sibtree values(2 , 8 , 3 );
insert into sibtree values(5 , 8 , 3 );
insert into sibtree values(4 , 2 , 8 );
insert into sibtree values(12 , 5 , 8 );
insert into sibtree(id,father) values(13 , 9 );
insert into sibtree(father,grandfather) values( 4 ,2);

--10 rows, but order maybe different from oracle, because outjoin restul order is different
select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)=sibtree.id
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)= 2 and level < 3
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)= 2 and  tree.id(+)=sibtree.id
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)=sibtree.id and tree.id(+)= 2
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;


select tree.id,sibtree.id,level from tree, sibtree
where tree.fat_siblings(+)= 2 and  tree.id(+)=sibtree.id
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.fat_siblings(+)= 2 and tree.id(+) = 2 and level < 3
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.fat_siblings(+)= 2 and tree.id = sibtree.id and level < 3
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.fat_siblings(+)= 2 and tree.id = sibtree.id and tree.id(+) = 2
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)= 2 and  tree.id(+)=sibtree.id and tree.fat_siblings(+)= 2
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree
where tree.id(+)= 2 and  tree.id(+)=sibtree.id and sibtree.id(+)= 2
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

create table yyy(id integer,name varchar(23));

select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.fat_siblings(+)= 2 and yyy.id(+) = sibtree.id and level < 3
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.fat_siblings(+)= 2 and yyy.id(+) = tree.fat_siblings and level < 3
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.id(+)= 2 and yyy.id = tree.id(+)
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.fat_siblings(+)= 2 and yyy.id = tree.id(+)
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.fat_siblings = 2 and yyy.id = tree.id(+)
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

--failed
select tree.id,sibtree.id,level from tree, sibtree,yyy
where tree.fat_siblings = 2 and yyy.id(+) = tree.id(+)
connect by nocycle prior tree.father =sibtree.father order siblings by tree.id;

drop table yyy;
drop table tree;
drop table sibtree;

--test connect by has outjoin copy from ora_outerjoin_op.sql
CREATE TABLE toj1 (
id   int,
id2  int,
name  VARCHAR(10)
);

CREATE TABLE toj2 (
id   int,
id2  int,
name  VARCHAR(10)
);
CREATE TABLE toj3 (
id   int,
id2  int,
name  VARCHAR(10)
);
CREATE TABLE toj4 (
id   int,
id2  int,
name  VARCHAR(10)
);

INSERT INTO toj1 VALUES(1,0,'A');
INSERT INTO toj1 VALUES(3,0,'C');

INSERT INTO toj2 VALUES(1,0,'AA');
INSERT INTO toj2 VALUES(2,0,'BB');

INSERT INTO toj3 VALUES(3,0,'CCC');
INSERT INTO toj3 VALUES(4,0,'DDD');

INSERT INTO toj4 VALUES(4,0,'DDDD');

--muiti tables using (+), all SQL  execute successfully
select *from toj1, toj2 where toj1.id(+) = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3,toj4 where toj1.id = toj2.id(+) and toj3.id = toj4.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2 and toj1.name(+) = toj2.name connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj2.id = toj1.id(+) and toj2.id (+)= toj3.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id (+)= toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1 t1, toj1 t2 where t1.id(+) = t2.id2 connect by nocycle prior t1.id = t2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj2.id = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and 1 = 1 and toj2.id = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj2.id = toj1.id and toj2.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj2.id = toj3.id(+) and toj2.id = toj1.id connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2, toj3,toj4 where
     toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj3.id(+) = toj4.id connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj2.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id (+)= toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id (+)= toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id = toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id = toj1.id(+) connect by nocycle prior toj1.id = toj2.id;

--some exeception cases, raise error
select *from toj1, toj2, toj3 where toj2.id = toj1.id(+) and toj2.id (+)= toj5.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2 = toj2.id2(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where abs(toj1.id(+)+toj1.id2) = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj1.id2 = toj2.id2(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj2.id(+) = toj1.id connect by nocycle prior toj1.id = toj2.id;
--ok
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id = toj1.id(+) connect by nocycle prior toj1.id = toj2.id;

--error
select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj2.id(+) = toj1.id connect by nocycle prior toj1.id = toj2.id;
--error
select *from toj1, toj2,toj3, toj5 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;
--error
select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj4.id(+) = toj1.id connect by nocycle prior toj1.id = toj2.id;

--ok
select *from toj1, toj2, toj3 where toj2.id(+) = toj1.id and toj2.id (+)= toj3.id connect by nocycle prior toj1.id = toj2.id;

--others cases
create or replace function outerjoin_f11(p1 int, p2 int) return integer as
begin
	return 1;
end;
/

create or replace function outerjoin_f12(p1 boolean) return int as
begin
	return 0;
end;
/

--execute successfully
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id, toj1.id)  = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) = abs(-20 * abs(toj2.id)) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id = abs(toj2.id(+) + 1) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id = outerjoin_f11(toj2.id(+),1) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3  where  toj2.id - toj2.id = toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj2.id = toj1.id(+) - toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id = toj2.id(+) + toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id  - toj1.id2 = toj2.id(+) + toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) + toj2.id2 = toj2.id connect by nocycle prior toj1.id = toj2.id;

--ok
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id, toj3.id)  = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id(+), toj3.id)  = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2,toj3 where toj1.id = abs(toj2.id(+) + toj3.id) connect by nocycle prior toj1.id = toj2.id;
--error
select *from toj1, toj2 where toj1.id = abs(toj2.id(+) + toj2.id) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id = toj2.id(+) + toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id  - toj2.id = toj2.id(+) + toj1.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) + toj1.id2 = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where toj1.id(+) + toj2.id2 = toj2.id - toj1.id2 connect by nocycle prior toj1.id = toj2.id;
--ok
select *from toj1, toj2, toj3 where toj1.id(+) + toj2.id2 = toj2.id - toj3.id2 connect by nocycle prior toj1.id = toj2.id;
--error
select *from toj1, toj2 where toj1.id(+)  = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;

--IN , NOT IN
select * from toj1, toj2 where toj1.id in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id in (toj2.id(+) + 1) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id in (toj2.id(+) - toj1.id2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj1.id2  in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id in (toj2.id) connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id not in (toj2.id(+) - toj1.id2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj1.id2  not in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id, toj2.id2) connect by nocycle prior toj1.id = toj2.id;

--error
select * from toj1, toj2 where toj1.id in (toj2.id(+) , 1) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id in (toj2.id(+) - toj2.id2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id2  in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj1.id2 in (toj2.id) connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id not in (toj2.id(+) - toj2.id2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id2  not in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id, toj1.id2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj1.id2 not in (toj2.id) connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id(+) in  (select toj2.id from toj2) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) in  (select toj2.id from toj2, toj3 where toj2.id(+) = toj3.id) connect by nocycle prior toj1.id = toj2.id;

--between and, not between and
select * from  toj1, toj2 where toj1.id  between toj2.id and 1 connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id(+)  between toj2.id and 1 connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id(+) - toj2.id2  between toj2.id and 1 connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) and 1 connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) - toj1.id and 1 connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+) connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+) - toj1.id connect by nocycle prior toj1.id = toj2.id;
select * from  toj1 t1, toj1 t2 where t1.id between t2.id(+) and t1.id connect by nocycle prior t1.id = t2.id;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) and toj2.id connect by nocycle prior toj1.id = toj2.id;

select * from  toj1, toj2 where toj1.id(+) not  between toj2.id and 1 connect by nocycle prior toj1.id = toj2.id; --error
select * from  toj1, toj2 where toj1.id not between toj2.id(+) and 1 connect by nocycle prior toj1.id = toj2.id; --error
select * from  toj1, toj2 where toj1.id(+)  between toj2.id and toj2.id2(+) connect by nocycle prior toj1.id = toj2.id; --error
select * from  toj1, toj2 where toj1.id(+)  between toj2.id(+) and toj2.id2(+) connect by nocycle prior toj1.id = toj2.id; --error
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj1.id2(+) connect by nocycle prior toj1.id = toj2.id; --error
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+)  - toj2.id connect by nocycle prior toj1.id = toj2.id; --error

--like, not like
select * from  toj1, toj2 where toj2.name LIKE toj1.name(+) connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj2.name(+) LIKE toj1.name connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj2.name NOT LIKE toj1.name(+) connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj2.name(+) NOT LIKE toj1.name connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2,toj3, toj4 where
     toj1.id(+) = toj2.id and
	 toj3.id(+) = toj4.id and
	 toj2.id(+) = toj3.id and
	 toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;

--bug0000313
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id = toj2.id(+) and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id(+) = 3 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and toj1.id(+) between 2 and 3 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and
   toj1.id(+) between toj2.id and 3 connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 between toj1.id(+) and 3 connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 between 0 and toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 between toj1.id2(+) and toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 between 1 and toj3.id(+) connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2
  and 1 between toj1.id and toj2.id connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id in (toj2.id(+)) and 1 in (toj1.id) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 in (toj1.id) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) = toj2.id and 1 not in (toj2.id, toj2.id2) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 not in (toj1.id2, toj2.id2) connect by nocycle prior toj1.id = toj2.id;

select *from toj1, toj2 where toj1.id(+) = 1 and  toj1.id(+) = toj2.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) = 1 and  toj1.id = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj3.id(+) = 3 and toj1.id(+) = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = 1 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) between 2 and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) between toj2.id and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 between toj1.id(+) and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 between 0 and toj1.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 between toj1.id2(+) and toj1.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 between 1 and toj3.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2 where 1 between toj1.id and toj2.id and toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2 connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where 1 in (toj1.id) and toj1.id in (toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 in (toj1.id) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where 1 not in (toj2.id, toj2.id2) and toj1.id(+) = toj2.id connect by nocycle prior toj1.id = toj2.id;
select *from toj1, toj2, toj3 where 1 not in (toj1.id2, toj2.id2) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+) connect by nocycle prior toj1.id = toj2.id;

--bug0000328
select * from toj1,toj2 where toj1.id(+) in (1,2,null) connect by nocycle prior toj1.id = toj2.id;

--bug0000321
select * from toj1,toj2 where toj1.id = toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id in toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) in toj1.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id between toj1.id(+) and 1 connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) between toj1.id and 1 connect by nocycle prior toj1.id = toj2.id;

--bug0000312
select * from toj1,toj2 where toj1.id= toj2.id(+) + toj2.id2(+) connect by nocycle prior toj1.id = toj2.id order by toj1.id;
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) > 2 connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 in ( toj1.id(+) + toj1.id2(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 not in (toj1.id(+) + toj1.id2(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 between toj1.id2(+) and toj1.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 between toj1.id(+) and toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) between toj2.id and 2 connect by nocycle prior toj1.id = toj2.id;
--the cases should raise error
select * from toj1,toj2,toj3 where toj1.id = toj2.id(+) + toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2,toj3 where toj2.id(+) + toj3.id(+) > toj1.id  connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 in ( toj1.id(+) + toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) + toj2.id(+) in 1 connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2,toj3 where toj1.id(+) + toj2.id(+) in toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where 1 not in (toj1.id(+) + toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) + toj2.id(+) not in 1 connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2,toj3 where toj1.id(+) + toj2.id(+) not in toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) between toj2.id(+) and 2 connect by nocycle prior toj1.id = toj2.id;
--bug0000322
select * from toj1,toj2 where toj1.id(+) + toj2.id(+)> 2 connect by nocycle prior toj1.id = toj2.id;--should raise error

--bug0000325
select * from toj1,toj2,toj3 where toj1.id between toj2.id(+) and toj3.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+) connect by nocycle prior toj1.id = toj2.id;

--bug0000317
insert into toj1(id2, name) values(0,'N');
insert into toj2(id2, name) values(0,'NN');

select * from toj1, toj2 where toj1.id = toj2.id(+) and toj1.id(+) is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) is not null and toj1.id = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) is not null and toj1.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) = toj3.id and toj1.id(+) is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id(+) is not null and toj1.id = toj2.id(+) and toj1.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj2.id2(+) is not null connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id - toj2.id(+) is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) is not null and toj1.id2 = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id2 = toj2.id(+) and toj1.id - toj2.id(+) is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id2 is not null connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id = toj2.id(+) and toj1.id(+) is null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) is null and toj1.id = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) is null and toj1.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) = toj3.id and toj1.id(+) is null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2, toj3 where toj1.id(+) is null and toj1.id = toj2.id(+) and toj1.id(+) = toj3.id connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj2.id2(+) is null connect by nocycle prior toj1.id = toj2.id;

select * from toj1, toj2 where toj1.id - toj2.id(+) is null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id(+) - toj2.id is null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) is null and toj1.id2 = toj2.id(+) connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id2 = toj2.id(+) and toj1.id - toj2.id(+) is null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id2 is null connect by nocycle prior toj1.id = toj2.id;

--ok
select * from toj1, toj2,toj3 where toj1.id - toj2.id(+) + toj3.id is not null connect by nocycle prior toj1.id = toj2.id;
--error
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id(+) is not null connect by nocycle prior toj1.id = toj2.id;
select * from toj1, toj2,toj3 where toj1.id - toj2.id(+) + toj3.id(+) is not null connect by nocycle prior toj1.id = toj2.id;

--bug0000327
--should raise error
select toj1.id, toj3.id from toj1,toj2,toj3 where not (toj1.id between toj2.id(+) and toj3.id) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id between toj2.id(+) and 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id >= toj2.id(+) and toj1.id <= 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id between toj2.id(+) and 1 and toj1.id > 0) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id between toj2.id(+) and 1) and toj1.id > 0) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+)=toj3.id and toj1.id = 1 or toj1.id2(+) <toj3.id2 ) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not((toj1.id(+)=toj3.id and toj1.id = 1) or toj1.id2(+) <toj3.id2 ) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) not in (toj3.id, 1)) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(not(not(toj1.id(+) not in (toj3.id, 1)))) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(not(toj1.id(+) in (toj3.id, 1))) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(toj1.id(+) = 1 or toj1.id(+) = toj3.id)) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(not(toj1.id(+) = 1 and toj1.id(+) = toj3.id))) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id,toj2.id from toj1,toj3,toj2 where not (toj1.name ||'c' like (toj2.name(+) || 'c') and toj3.id(+)>toj1.id)  connect by nocycle prior toj1.id = toj3.id order by toj1.id;
select toj1.id, toj3.id from toj1,toj3 where not( toj1.id(+) is not null or toj1.id(+) = toj3.id) and not(toj1.id(+) = toj3.id and toj1.id(+) = 1) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id = toj2.id or toj2.id = 1) or toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id or toj2.id = 1) and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id  and toj1.id(+) = 1) or (toj2.id = 1 and toj1.id(+) = 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id  and toj1.id(+) = 1) or toj2.id = 1 connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  and not( toj1.id(+) = 1 and toj3.id2 = 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  or not( toj1.id(+) = 1 or toj3.id2 = 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where  toj1.id = toj2.id or toj2.id = 1 and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where  toj1.id = toj2.id or (toj2.id = 1 and toj1.id(+) = 1) connect by nocycle prior toj1.id = toj2.id;

--execute success
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) in (toj3.id, 1)) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(not(toj1.id(+) not in (toj3.id, 1))) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where toj1.id(+) not in (toj3.id, 1) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(not(not(toj1.id(+) in (toj3.id, 1)))) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj2,toj3 where not ( NOT(toj1.id between toj2.id(+) and toj3.id) or toj1.id > 0) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id = toj2.id(+)) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id not between toj2.id(+) and 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id between toj2.id(+) and 1)) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id >= toj2.id(+) and toj1.id <= 1)) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) in (toj3.id, 1)) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(toj1.id(+) = 1 and toj1.id(+) = toj3.id)) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj3.id from toj1,toj3 where not( toj1.id(+) is not null or toj1.id(+) = toj3.id) connect by nocycle prior toj1.id = toj3.id;
select toj1.id, toj2.id from toj1,toj2 where not (not(toj1.id = toj2.id or toj2.id = 1) or toj1.id(+) = 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id = toj2.id or toj2.id = 1) and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  and not( toj3.id = 1 and toj3.id2 = 1) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not(toj1.id = toj2.id) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where not(toj1.id = toj2.id) connect by nocycle prior toj1.id = toj2.id;
select toj1.id, toj2.id from toj1,toj2 where  (toj1.id = toj2.id or toj2.id = 1) and toj1.id(+) = 1 connect by nocycle prior toj1.id = toj2.id;

--test failed
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = 1) or (toj1.id = 2);
select toj1.id, toj2.id from toj1,toj2 where (toj1.id = 2) or (toj1.id(+) = 1);
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = 1) or (toj2.id(+) = 2);

DROP VIEW  v1;
DROP TABLE toj1;
DROP TABLE toj2;
DROP TABLE toj3;
DROP TABLE toj4;
drop function outerjoin_f11(integer,integer);
drop function outerjoin_f12(boolean);

--test bug#0000730
create table t_bug730(id int);
select id from t_bug730 t start with t.id = 1 connect by prior 1 = 1;
drop table t_bug730;

--bug#985
create table Tree(son varchar2(10), father varchar2(10));

insert into tree values('lyd','lds');
insert into tree values('lhr','lyd');
insert into tree values('lds','lqc');
insert into tree values('dd','lyd');

--one row
select t.*,level from tree t where t.son = 'lds' start with father='lqc' connect by prior son = father;

--one row
select t.*,level from public.tree t where t.son = 'lds' start with father='lqc' connect by prior son = father;

--one row
select t.*,level from public.tree t where t.son = 'lds' start with t.father='lqc' connect by prior t.son = t.father;

--create view
create view test_view as select t.*,level from public.tree t where t.son = 'lds' start with t.father='lqc' connect by prior t.son = t.father;

--one row
select * from test_view;

--clean data;              1
drop view test_view;
drop table tree;


create table Tree(son varchar2(256), father varchar2(256));

create or replace function test_f(id varchar2) return varchar2 is
  var1 varchar2(256);
BEGIN
 raise info '%','id = ' || id;
 select t.son || t.father || id into var1 from tree t
 WHERE rownum = 1 start with t.father=id
 connect by prior t.son = t.father;
 return var1;
end;
/

--null and print id = abc
select test_f('abc') from dual;

--insert into
INSERT INTO tree VALUES('one','zero');
INSERT INTO tree VALUES('two','one');

--print onezerozero and id = zero
select test_f('zero') from dual;

create or replace function test_f1(id varchar2) return integer is
  var1 varchar2(256);
  var2 integer;
  var3 varchar2(256);
  c1 sys_refcursor;
begin
 OPEN c1 FOR SELECT id,level,son FROM tree connect by prior son = father start
 WITH father = id;
 FETCH c1 INTO var1,var2,var3;
 CLOSE c1;
 raise info '%','var1 = ' || var1 || ',var2 = '|| var2 || ',var3 =' || var3;
 return 23;
end;
/

create or replace function test_f2(id varchar2) return integer is
  var1 varchar2(256);
  var2 integer;
  var3 varchar2(256);
  c1 sys_refcursor;
begin
 OPEN c1 FOR SELECT $1,level,son FROM tree connect by prior son = father start
 WITH father = $1;
 FETCH c1 INTO var1,var2,var3;
 CLOSE c1;
 raise info '%','var1 = ' || var1 || ',var2 = '|| var2 || ',var3 =' || var3;
 return 23;
end;
/


--print var1 = one,var2 = 1,var3 =two
SELECT test_f1('one') FROM dual;

--print var1 = one,var2 = 1,var3 =two
SELECT test_f2('one') FROM dual;


create or replace function test_f3(id varchar2) return varchar2 is
  var1 varchar2(256);
BEGIN
 raise info '%','id = ' || id;
 select t.son || t.father || id  into var1 from tree t
 WHERE rownum = 1 start with t.father='one'
 connect by prior t.son = t.father;
 return var1;
end;
/

--print twooneone
SELECT test_f3('one') FROM dual;

--clean data
DROP FUNCTION test_f3;
DROP FUNCTION test_f2;
DROP FUNCTION test_f1;
DROP FUNCTION test_f;
DROP TABLE tree;

--bug#1085
--print 1 2 3
with tmp as (
select 1 as id, 2 as sjzzid,3 as rs from dual)
select id,sjzzid,(select sum(rs) from tmp a
start with a.id = b.id connect by prior a.id = a.sjzzid ) rs
from tmp b;


--ok
with tmp as (
select 1 as id, 2 as sjzzid,3 as rs from dual)
select sum(rs) from tmp a
start with a.id = 2 connect by prior a.id = a.sjzzid;

--print 3
with tmp as (
select 1 as id, 2 as sjzzid,3 as rs from dual)
select (select sum(rs) from tmp a
start with a.id = b.id connect by prior a.id = a.sjzzid ) rs
from tmp b;

--print 3
with tmp as (
select 1 as id, 2 as sjzzid,3 as rs from dual)
select (select sum(rs) from tmp a
start with a.id = 1 connect by prior a.id = a.sjzzid ) rs
from tmp b;

--print 1
with tmp as (
select 1 as id, 2 as sjzzid,3 as rs from dual)
select (select b.id from tmp a
start with a.id = b.id connect by prior a.id = a.sjzzid ) rs
from tmp b;

create table test_connect(id integer,son integer, father integer, name varchar2(256));

--ok
create or replace function test_f(id integer) return integer is
  var1 integer;
begin
  select id into var1 from test_connect a connect by prior son = father
  start with a.id = id;
  return var1;
end;
/

--ok
select test_f(23) from dual;

--ok
create or replace function test_f(id integer) return integer is
  var1 integer;
begin
  select id into var1 from test_connect a connect by prior son = father
  start with a.id = 1;
  return var1;
end;
/

--ok
select test_f(23) from dual;

--ok
drop function test_f;
drop table test_connect;


--test bug#M000092
CREATE OR REPLACE FUNCTION GET_JSON_VALUES(P_JSON_STR IN VARCHAR2, P_KEY IN VARCHAR2)
  RETURN VARCHAR2 IS
   cursor v_cur(V_STR varchar2, V_KEY varchar2) IS
     SELECT RTRIM(REPLACE(STR,'"' || V_KEY || '":"',''), '"') VAL
    FROM (SELECT REGEXP_SUBSTR(V_STR, '[^,]+', 1, ROWNUM::integer) STR
    FROM DUAL
    CONNECT BY ROWNUM <=
                   LENGTH(V_STR) - LENGTH(REPLACE(V_STR, ',')) + 1)
         WHERE STR LIKE '"' || V_KEY || '":%';
  V_RSL VARCHAR2(600);
  V_STR VARCHAR2(600);
BEGIN
  OPEN V_CUR(V_STR, P_KEY);
  FETCH V_CUR INTO V_RSL;
  CLOSE V_CUR;
  RETURN V_RSL;
END;
/

--ok
SELECT GET_JSON_VALUES('abcdefgh123','abc') FROM DUAL;

--clean data
DROP function GET_JSON_VALUES;


--Begin - bug#M0000162
create table Tree(son varchar2(10), father varchar2(10));
insert into tree values('lyd','lds');
insert into tree values('lhr','lyd');
insert into tree values('lds','lqc');
insert into tree values('dd','lyd');
insert into tree values('lys','lqc');
insert into tree values('lqs','lqc');
insert into tree values('lc','lqs');
insert into tree values('lxm','lqs');
insert into tree values('lxd','lys');
insert into tree values('xd1','lxd');
insert into tree values('xd2','lxd');
insert into tree values('xd2_1','xd2');
insert into tree values('xd2_2','xd2');
insert into tree values('dd1','dd');
select *from tree;

create table test_insert(id varchar2(256),son varchar2(256), father varchar2(256));

select CONNECT_BY_ROOT t.father as depart_father,t.*  from tree t start with father='lqc' connect by prior son = father
   order by son, father;

insert into test_insert select * from (
  select * from (select CONNECT_BY_ROOT t.father as depart_father,t.*  from tree t start with father='lqc' connect by prior son = father)) a;

select * from test_insert order by id, son;

drop table test_insert;
drop table Tree;
--End - bug#M0000162

---test bugM0000279
create table Tree(son varchar2(10), father varchar2(10));

insert into tree values('lyd','lds');
insert into tree values('lhr','lyd');
insert into tree values('lds','lqc');
insert into tree values('dd','lyd');
insert into tree values('lys','lqc');
insert into tree values('lqs','lqc');
insert into tree values('lc','lqs');
insert into tree values('lxm','lqs');
insert into tree values('lxd','lys');
insert into tree values('xd1','lxd');
insert into tree values('xd2','lxd');
insert into tree values('xd2_1','xd2');
insert into tree values('xd2_2','xd2');
insert into tree values('dd1','dd');
insert into tree values('lqc','lyd');

declare
 cursor c1 is
   select tree.*,level,connect_by_isleaf from tree start with father='lqc'
   connect by prior son = father;
 var1 c1%rowtype;
begin
  open c1;
  fetch c1 into var1;
exception
  when others then
     raise info '%',sqlerrm;
     rollback;
end;
/

--clean data
DROP TABLE tree;

--BUG#Z0000954
create table test_id(vessel_id number,total_hatch number);

insert into test_id values(1,2);
insert into test_id values(2,3);
insert into test_id values(3,4);
insert into test_id values(1,6);
insert into test_id values(1,2);
insert into test_id values(10,100000);

--ok 100005
select COUNT(*)  from test_id connect by rownum <= total_hatch;

--ok 100017
select count(*) from test_id,
(select rownum rn from test_id connect by rownum <= total_hatch ) b
where total_hatch >= rn;

--test bug#M326
CREATE TABLE test_for(id integer,name varchar2(256));

INSERT INTO test_for VALUES(1,'one');
INSERT INTO test_for VALUES(2,'two');
INSERT INTO test_for VALUES(3,'three');

--19 rows
SELECT test_for.*,test_id.* FROM test_for,test_id connect BY rownum < id ORDER BY id,name;

--19 rows
SELECT * FROM test_for,test_id connect BY rownum < id ORDER BY id,name;

DROP TABLE test_id;
DROP TABLE test_for;


--test a dropped column
create table test_id(vessel_id number,total_hatch number, mds varchar2(256));

ALTER TABLE test_id DROP COLUMN mds;

insert into test_id values(1,2);
insert into test_id values(2,3);
insert into test_id values(3,4);
insert into test_id values(1,6);
insert into test_id values(1,2);
insert into test_id values(10,100000);

--ok 100005
select COUNT(*)  from test_id connect by rownum <= total_hatch;

--test guc variable
SET plisql.connect_by_rownum_use_func = false;
SELECT COUNT(*) FROM test_id connect BY rownum < 4;
SET plisql.connect_by_rownum_use_func = true;
SELECT COUNT(*) FROM test_id connect BY rownum < 4;

DROP TABLE test_id;

--bug#Z1285
create table RYHX_ORGANIZATION(
ID NUMBER not null,
CODE VARCHAR2(32) not null,
NAME VARCHAR2(128) not null,
CODE_LEVEL NUMBER not null,
ORG_TYPE VARCHAR2(16) not null,
PARENT_CODE VARCHAR2(32) not null,
STATUS NUMBER default 0 not null,
STATEFLAG NUMBER default 0 not null,
REMARK VARCHAR2(512),
OPERATOR VARCHAR2(64) not null,
OPERATE_TIME DATE default sysdate()
);

insert into RYHX_ORGANIZATION values(1,'01','wang',1,'vtype',0,1,0,'marki','op',to_date('2024-10-24 23:45:36','yyyy-mm-dd hh24:mi:ss'));
insert into RYHX_ORGANIZATION values(2,'02','wangs',2,'vtype','01',1,0,'marki','op',to_date('2024-10-25 23:45:36','yyyy-mm-dd hh24:mi:ss'));
insert into RYHX_ORGANIZATION values(3,'03','wangss',3,'vtype','02',1,0,'marki','op',to_date('2024-10-26 23:45:36','yyyy-mm-dd hh24:mi:ss'));
insert into RYHX_ORGANIZATION values(4,'04','wangsss',4,'vtype','03',1,0,'marki','op',to_date('2024-10-27 23:45:36','yyyy-mm-dd hh24:mi:ss'));


--execute ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
decode( ( SELECT COUNT ( 1 ) FROM ryhx_organization WHERE parent_code = s.code AND stateflag = 0 ), 0, 'false', 'true' ) AS is_parent,
( SELECT NAME FROM ryhx_organization WHERE code = s.parent_code ) parent_name,
s.ID AS int_id,
s.code,s.code_level,s.org_type,s.parent_code,s.OPERATOR,
to_char(s.operate_time, 'yyyy-MM-dd hh24:mi:ss' ) AS operate_time,s.remark
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
decode( ( SELECT COUNT ( 1 ) FROM ryhx_organization WHERE parent_code = s.code AND stateflag = 0 ), 0, 'false', 'true' ) AS is_parent,
( SELECT NAME FROM ryhx_organization WHERE code = s.parent_code ) parent_name,
s.ID AS int_id,
s.code,
s.code_level,
s.org_type,
s.parent_code,
s.OPERATOR,
to_char( s.operate_time, 'yyyy-MM-dd hh24:mi:ss' ) AS operate_time,
s.remark
FROM
ryhx_organization s
WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;

--ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
s.ID AS int_id,
s.code,s.code_level,s.org_type,s.parent_code,s.OPERATOR
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
( SELECT NAME FROM ryhx_organization WHERE code = s.parent_code ) parent_name,
s.ID AS int_id,
s.code,s.code_level,s.org_type,s.parent_code,s.OPERATOR,
to_char(s.operate_time, 'yyyy-MM-dd hh24:mi:ss' ) AS operate_time,s.remark
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
decode( ( SELECT  ( 1 ) FROM ryhx_organization WHERE parent_code = s.code AND stateflag = 0 ), 0, 'false', 'true' ) AS is_parent,
s.ID AS int_id,
s.code,s.code_level,s.org_type,s.parent_code,s.OPERATOR,
to_char(s.operate_time, 'yyyy-MM-dd hh24:mi:ss' ) AS operate_time,s.remark
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--ok
SELECT
s.code AS ID,
s.NAME,
s.parent_code AS pid,
( SELECT NAME FROM ryhx_organization WHERE code = s.parent_code ) parent_name,
s.ID AS int_id,
s.code,s.code_level,s.org_type,s.parent_code,s.OPERATOR,
to_char(s.operate_time, 'yyyy-MM-dd hh24:mi:ss' ) AS operate_time,s.remark
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--pirnt wang,wangs,wangss
SELECT
( SELECT NAME FROM ryhx_organization WHERE code = s.parent_code ) parent_name
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--faield
SELECT level,
( SELECT NAME FROM ryhx_organization WHERE  CODE_LEVEL = level ) parent_name
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;


--ok
SELECT level,
( SELECT NAME FROM ryhx_organization WHERE  CODE_LEVEL = level start WITH code = '01' connect BY rownum < 1 ) parent_name
FROM ryhx_organization s WHERE
s.stateflag = 0 START WITH s.code = '01' CONNECT BY PRIOR s.code = parent_code;

--clean data
DROP TABLE RYHX_ORGANIZATION;


--test sys_connect_by_path function
create or replace function sys_connect_by_path(id integer) return integer is
begin
  return 1;
end;
/

--error
select sys_connect_by_path(1) from dual;

DROP FUNCTION sys_connect_by_path;


create or replace function PUBLIC.sys_connect_by_path(id integer) return integer is
begin
  return 1;
end;
/

--ok
select PUBLIC.sys_connect_by_path(1) from dual;

--clean
DROP FUNCTION PUBLIC.sys_connect_by_path;

--test extral
CREATE TABLE connect_by_table(id INT, pid INT, name varchar2(256));
INSERT INTO connect_by_table VALUES(1, NULL, 'A');
INSERT INTO connect_by_table VALUES(101, 1, 'B1');
INSERT INTO connect_by_table VALUES(102, 1, 'B2');
INSERT INTO connect_by_table VALUES(10101, 101, 'C0');
INSERT INTO connect_by_table VALUES(10102, 101, 'C1');
INSERT INTO connect_by_table VALUES(10201, 102, 'C2');
INSERT INTO connect_by_table VALUES(102, 10201, 'CYCLE');

--6 rows
SELECT id,pid,name,level,prior id,CONNECT_BY_ISCYCLE as cycle,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by id;

--error
SELECT id,pid,name,level,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by level;

--start with aggreate function
--error
SELECT id,pid,name,level FROM connect_by_table START WITH count(id) = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by id;

--error
SELECT id,pid,name,level FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid and count(id) = 1 order siblings by id;

--ok no rows
SELECT id,pid,name,level FROM connect_by_table START WITH level = 1 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;

--error
SELECT id,pid,name,level FROM connect_by_table START WITH connect_by_iscycle = 1 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;

--error
SELECT id,pid,name,level FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid AND connect_by_iscycle = 1  order siblings by id;

--error
SELECT id,pid,name,connect_by_root level FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by id;

--error
SELECT id,pid,name,level,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR level = pid order siblings by id;

--error
SELECT id,pid,name,level,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by level;

--error
SELECT id,pid,name,level,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by CONNECT_BY_ISCYCLE;

--error
SELECT id,pid,name,level,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM connect_by_table START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by level;

--6 rows
SELECT id,pid,name,level FROM connect_by_table START WITH level = 1 OR id = 1 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;

--start with level = 1
--0 row
SELECT id,pid,name,level FROM connect_by_table START WITH level = 1 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;

-- 17 rows
SELECT id,pid,name,level FROM connect_by_table START WITH level <= 1 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;


--17 rows
SELECT id,pid,name,level FROM connect_by_table START WITH level = 0 CONNECT BY NOCYCLE PRIOR id = pid  order siblings by id;


--7 rows
SELECT id,pid,name,level FROM connect_by_table START WITH level = 0 CONNECT BY NOCYCLE PRIOR id = level  order siblings by id;


--test join
create table h1(c1 integer, name varchar(23));
create table h2(c1 integer, name varchar(23));

insert into h1 values(1,'aa');
insert into h1 values(2,'bb');
insert into h1 values(3,'cc');
insert into h1 values(4,'dd');

insert into h2 values(1,'aa');
insert into h2 values(2,'bb');
insert into h2 values(3,'cc');
insert into h2 values(4,'dd');


--16 rows
select * from h1 LEFT JOIN h1 hh on h1.c1=hh.c1 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--1 rows
select level, h1.c1,h2.c1,prior h1.c1, prior h2.c1 from h1 LEFT JOIN h2 on h1.c1=h2.c1 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h2.c1) start with h1.c1 =1 order siblings by h1.c1,h2.c1,h1.name;


--16 rows
select * from h1 LEFT JOIN h1 hh on h1.c1=hh.c1 and h1.c1 = h1.c1 + 1 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 79 rows
select * from h1 LEFT JOIN h1 hh on h1.c1=hh.c1 or h1.c1 = hh.c1 + 1  connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test 16 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 > hh.c1 and (h1.c1 = hh.c1 + 1) and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 79 rows
select * from h1 LEFT JOIN h1 hh on h1.c1=hh.c1 or h1.c1 = hh.c1 + 1  connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 16 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = 1 and h1.c1=hh.c1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 16 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = hh.c1 and (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 493 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = hh.c1  or h1.c1 = hh.c1 + 1  or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 493 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = hh.c1  or h1.c1 = hh.c1 + 1 or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 493 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = hh.c1  or (h1.c1 = hh.c1 + 1 and h1.c1 = hh.c1 + 3 ) or (h1.c1>1  or h1.c1=hh.c1 + 2) connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1
order siblings by h1.c1,hh.c1,h1.name;

--test ok 64 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = 1 or h1.c1=hh.c1  connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 79 rows
select * from h1 LEFT JOIN h1 hh on h1.c1=hh.c1 or h1.c1 = hh.c1 + 1  connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 206 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 > hh.c1 or h1.c1 = hh.c1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok no rows
select * from h1,h1 hh where h1.c1 > hh.c1 and prior(h1.c1) >2 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 16 rows
select * from h1 LEFT JOIN h1 hh ON  h1.c1=hh.c1 and h1.c1>1 connect by nocycle prior (h1.c1)= prior(h1.c1) start with h1.c1 =1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 205 rows
select * from h1 LEFT JOIN h1 hh on h1.c1 = hh.c1  or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 492 rows
select * from h1 LEFT JOIN h1 hh on (h1.c1>1  or h1.c1 = hh.c1) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 492 rows
select * from h1 LEFT JOIN h1 hh on (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1 order siblings by h1.c1,hh.c1,h1.name;

-----test start here
--test ok 492 rows
select * from h1 LEFT JOIN h1 hh on (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1 order siblings by h1.c1,hh.c1,h1.name;

--test ok 205 rows
select * from h1 RIGHT JOIN h1 hh on  h1.c1 + hh.c1 = 2 and (h1.c1>1  or (h1.c1 > 2 and h1.c1 = hh.c1)) or (h1.c1 > 1 and h1.c1 < hh.c1 + 2) and (h1.c1 > 2 or h1.c1 < hh.c1 + 4 and  h1.c1 < hh.c1 + 3)
connect by nocycle prior (h1.c1)= prior(h1.c1) and h1.c1>1 start with h1.c1>1 order siblings by h1.c1,hh.c1,h1.name;

--4 rows
SELECT h1.c1,h1.name,level FROM h1 LEFT JOIN h2 ON h1.c1 = 1 AND  h1.c1 = h2.c1
  connect BY nocycle h1.c1 = prior (h2.c1) start WITH h1.c1 >= 1 order siblings by h1.c1,h2.c1,h1.name;


--11 rows
SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 LEFT JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;

explain(costs off) select h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 LEFT JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


--8 rows
SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 right JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


explain(costs off) SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 right JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


--20 rows
SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 full JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;



explain (costs off)  SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 full JOIN connect_by_table h2
ON h1.id = h2.id AND h1.id > 102 connect BY h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;

-- 5row
SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1,connect_by_table h2
where h1.id = h2.id AND h1.id > 102 connect BY nocycle h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


explain (costs off) SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1,connect_by_table h2
where h1.id = h2.id AND h1.id > 102 connect BY nocycle h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


--79 rows
SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 left JOIN connect_by_table h2
ON h1.id > 2 AND h1.id > 102 connect BY nocycle h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;


explain (costs off) SELECT h1.id AS id,h1.pid AS pid,h1.name,level FROM connect_by_table h1 left JOIN connect_by_table h2
ON h1.id > 2 AND h1.id > 102 connect BY nocycle h1.id = prior h2.pid order siblings by h1.id,h1.pid,h1.name;



--test with_clause
with cte_1 as (select * from connect_by_table)
SELECT id,pid,name,level,prior id,CONNECT_BY_ISCYCLE as cycle ,CONNECT_BY_ISLEAF as leaf FROM cte_1 START WITH id = 1 CONNECT BY NOCYCLE PRIOR id = pid order siblings by id;


--test where
--connect_by_iscycle as where ok
select id,pid,name,level, prior id,connect_by_iscycle as cycle,connect_by_isleaf as leaf from connect_by_table where connect_by_iscycle = 1
start with id = 1 connect by nocycle prior id = pid order siblings by id;

--connect_by_isleaf as where ok
select id,pid,name,level,prior id,connect_by_iscycle as cycle,connect_by_isleaf as leaf from connect_by_table where connect_by_isleaf = 1
start with id = 1 connect by nocycle prior id = pid order siblings by id;

--level as where ok
select id,pid,name,level,prior id,connect_by_iscycle as cycle,connect_by_isleaf as leaf from connect_by_table where level = 1
start with id = 1 connect by nocycle prior id = pid order siblings by id;

--connect_by_root as where ok
select id,pid,name,level, prior id,connect_by_iscycle as cycle,connect_by_isleaf as leaf from connect_by_table where connect_by_root id = 1
start with id = 1 connect by nocycle prior id = pid order siblings by id;

--clean data
drop table connect_by_table;
drop table h1;
drop table h2;


--test split where and join clause
create table h1(h1c1 integer,h1name varchar2(23));

create table h2(h2c1 integer,h2name varchar2(23));

insert into h1 values(1,'aa');
insert into h1 values(2,'bb');
insert into h1 values(3,'cc');
insert into h1 values(4,'dd');

insert into h2 values(1,'aa');
insert into h2 values(2,'bb');
insert into h2 values(3,'cc');
insert into h2 values(4,'dd');

--15 rows
select * from h1,h2 where h1c1 = h2c1 or h1c1 > 1 connect by nocycle prior (h1c1) = prior(h2c1) start with h1c1 = 1  order siblings by h1c1,h1name,h2c1,h2name;


--15 ROWS
select * from h1,h2 where h1c1 = h2c1 and h1c1 > 1 connect by nocycle prior (h1c1) = prior(h2c1) start with h1c1 = 1  order siblings by h1c1,h1name,h2c1,h2name;


--15 ROWS
select * from h1,h2 where h1c1(+)  = h2c1 and h1c1 > 1 connect by nocycle prior (h1c1) = prior(h2c1) start with h1c1 = 1  order siblings by h1c1,h1name,h2c1,h2name;

--16 rows
select * from h1,h2 where h1c1 = h2c1 and h1c1 > 1 or h1c1 < 3 connect by nocycle prior (h1c1) = prior(h2c1) start with h1c1 = 1  order siblings by h1c1,h1name,h2c1,h2name;

--clean data
drop table h1;
drop table h2;

--BUG#Z00004575
create table emp_test(emp_id number primary key,emp_name varchar2(50), manager_id number);

insert into emp_test values(1,'CE0', NULL);
insert into emp_test values(2,'managerA', 1);
insert into emp_test values(3,'staffA1', 2);
insert into emp_test values(4,'staffA2', 3);

insert into emp_test values(5,'managerB', 7);
insert into emp_test values(6,'staffB1', 5);
insert into emp_test values(7,'staffB2',6);

insert into emp_test values(8, 'managerC', 9);
insert into emp_test values(9, 'staffC1', 8);

insert into emp_test values(10,'managerD', 10);


select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;


delete from emp_test where emp_id != 10;

--1 row
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;

--ok 1 row
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where connect_by_isleaf = 1
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;

--ok 1 row
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where level = 1
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;


--ok 1 row
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where connect_by_iscycle = 1
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;

--ok 0 ROW
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where prior manager_id = 10
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;


--ok 1 row
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where connect_by_root manager_id = 10
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;

--error
select emp_name,connect_by_iscycle as is_cycle, connect_by_isleaf as is_leaf from emp_test where sys_connect_by_path(manager_id, '/') = '10/'
start with emp_id = 5 or emp_id = 8 or emp_id = 10
connect by nocycle emp_id = prior manager_id;

--clean data
drop table emp_test;

--bug#Z4574
create table employees (
	emp_id number primary key,
	emp_name varchar(50),
	dept_id number,
	salary number,
	mgr_id number,
	status varchar(10) default 'ACTIVE'
);

insert into employees values (1, 'CEO', 10, 10000, NULL, 'ACTIVE');
insert into employees values (2, 'VP1', 10, 8000, 1, 'CEO');
insert into employees values (3, 'VP2', 20, 7500, 1, 'CEO');
insert into employees values (4, 'MGR1', 10, 6000, 2, 'VP1');
insert into employees values (5, 'MGR2', 20, 5500, 3, 'VP2');
insert into employees values (6, 'EMP1', 10, 4000, 4, 'MGR1');
insert into employees values (7, 'EMP2', 20, 3800, 5, 'MGR2');
insert into employees values (8, 'EMP3', 10, 3500, 4, 'MGR1');

--error
select emp_name, sys_connect_by_path(emp_name, '/') as path
from employees
where sys_connect_by_path(emp_name, '/') like '%VP%'
start with mgr_id is null
connect by prior emp_id = mgr_id;


--error
select emp_name, sys_connect_by_path(emp_name, '/') AS path
from employees
start with mgr_id is null
connect by prior emp_id = mgr_id and sys_connect_by_path(emp_name, '/') like '%VP%';


--ok
select emp_name, sys_connect_by_path(emp_name, '/') as path
from employees
start with emp_id = 1
connect by prior emp_id = mgr_id
order siblings by emp_name;


--ok
select emp_name, sys_connect_by_path(emp_name, '/') as path
from employees
start with emp_id = 1
connect by prior emp_name = status
order siblings by emp_name;

--ok
select emp_name, sys_connect_by_path(emp_name, '/') as path
from employees
start with emp_id = 1
connect by prior emp_id = mgr_id and prior emp_name = status
order siblings by emp_name;

--clean data
drop table employees;

--bug#Z4710
create table org_structure (
	emp_id number,
	emp_name varchar(50),
	dept varchar(20),
	salary number,
	manager_id number
);

insert into org_structure values (1, 'CEO', 'CEO', 100000, NULL);
insert into org_structure values (2, 'managerA', 'sale', 80000, 1);
insert into org_structure values (3, 'managerB', 'IT', 85000, 1);
insert into org_structure values (4, 'staffA1', 'sale', 60000, 2);
insert into org_structure values (5, 'staffB1', 'IT', 65000, 3);

--error
select emp_name, avg(salary) from org_structure
start with manager_id is null connect by prior emp_id = manager_id
group by emp_name
order siblings by emp_name;

--ok
select emp_name, avg(salary) from org_structure
start with manager_id is null connect by prior emp_id = manager_id
group by emp_name
order by emp_name;

--clean data
drop table org_structure;

--bug#Z5447
CREATE TABLE zc_sys_mkb (
    mkbh varchar2(6) NOT NULL,
    mkmc varchar2(50),
    url varchar2(200),
    xh number(4,0),
    qxbz varchar2(10),
    icon varchar2(40),
    xtbz varchar2(2),
    czqx number(4,0),
    ywsm number,
    tplx number,
    mklx varchar2(4),
    ywlx varchar2(4),
    mkjs varchar2(100),
    mksm varchar2(100),
    mkmc_eng varchar2(50),
    gid varchar2(50),
    pid varchar2(50)
);

CREATE TABLE ZC_SYS_CZQXB (
	RYBH VARCHAR2(40 char) NOT NULL COLLATE "DEFAULT",
	MKBH VARCHAR2(6 char) NOT NULL COLLATE "DEFAULT",
	CZR VARCHAR2(40 char) NULL COLLATE "DEFAULT",
	CZRQ DATE NULL,
	CZQX NUMBER NULL,
	XTBZ VARCHAR2(2 char) NULL COLLATE "DEFAULT",
	CONSTRAINT ZC_SYS_CZQXB_PK_ZC_SYS_CZQXB PRIMARY KEY (rybh, mkbh)
);

insert into zc_sys_mkb(mkbh,gid,pid,qxbz,ywlx) values(1,1,0,1,'ab3d');
insert into zc_sys_mkb(mkbh,gid,pid,qxbz,ywlx) values(2,2,1,1,'ab3d');

insert into zc_sys_czqxb(RYBH,MKBH) values('000001',1);
insert into zc_sys_czqxb(RYBH,MKBH) values('000001',2);

SELECT g.gid
FROM zc_sys_mkb g
START WITH (mkbh IN (SELECT DISTINCT z.mkbh
FROM ZC_SYS_CZQXB z
LEFT JOIN zc_sys_mkb a
ON z.mkbh = a.mkbh
WHERE z.rybh = '000001'
AND a.mkbh NOT LIKE '88%'
AND instr(ywlx, '3') > 0
AND qxbz = '1'
AND nvl(mklx, '0') <> '9'
AND a.mkbh NOT LIKE '66%'))
CONNECT BY PRIOR g.pid = g.gid order by g.gid;


SELECT g.gid as mds
FROM zc_sys_mkb g
START WITH (mkbh IN (SELECT DISTINCT z.mkbh
FROM ZC_SYS_CZQXB z
LEFT JOIN zc_sys_mkb a
ON z.mkbh = a.mkbh
WHERE z.rybh = '000001'
AND a.mkbh NOT LIKE '88%'
AND instr(ywlx, '3') > 0
AND qxbz = '1'
AND nvl(mklx, '0') <> '9'
AND a.mkbh NOT LIKE '66%'))
CONNECT BY PRIOR g.pid = g.gid order by mds;

create table org_structure (
	emp_id number,
	emp_name varchar(50),
	dept varchar(20),
	salary number,
	manager_id number
);

insert into org_structure values (1, 'CEO', 'CEO', 100000, NULL);
insert into org_structure values (2, 'managerA', 'sale', 80000, 1);
insert into org_structure values (3, 'managerB', 'IT', 85000, 1);
insert into org_structure values (4, 'staffA1', 'sale', 60000, 2);
insert into org_structure values (5, 'staffB1', 'IT', 65000, 3);

--error
select emp_name, avg(salary) from org_structure
start with manager_id is null connect by prior emp_id = manager_id
group by emp_name
order siblings by emp_name;

--ok
select emp_name::varchar2(256), avg(salary) from org_structure
start with manager_id::numeric is null connect by prior emp_id = manager_id
group by emp_name
order by emp_name;


--ok
select emp_name::varchar2(256), avg(salary) from org_structure
start with manager_id::numeric is null connect by prior emp_id::numeric = manager_id
group by emp_name
order by emp_name;

--clean data
drop table zc_sys_mkb;
drop table ZC_SYS_CZQXB;
drop table org_structure;