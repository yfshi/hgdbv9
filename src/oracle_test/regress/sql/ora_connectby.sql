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

--bug#Z8247
CREATE TABLE emp_hier_test (
    empno  NUMBER,
    ename  VARCHAR2(30),
    mgr    NUMBER,
    deptno NUMBER,
    sal    NUMBER
);

INSERT INTO emp_hier_test VALUES (1,  'CEO',     NULL, 10, 30000);
INSERT INTO emp_hier_test VALUES (2,  'VP_A',    1,    10, 20000);
INSERT INTO emp_hier_test VALUES (3,  'VP_B',    1,    20, 18000);
INSERT INTO emp_hier_test VALUES (4,  'DEV_A1',  2,    10, 10000);
INSERT INTO emp_hier_test VALUES (5,  'DEV_A2',  2,    10,  9000);
INSERT INTO emp_hier_test VALUES (6,  'DEV_B1',  3,    20,  8000);
INSERT INTO emp_hier_test VALUES (7,  'DEV_B2',  3,    20,  7500);
INSERT INTO emp_hier_test VALUES (8,  'INTERN1', 4,    10,  3000);
INSERT INTO emp_hier_test VALUES (9,  'INTERN2', 4,    10,  2500);

--9 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    RANK() OVER (
        PARTITION BY LEVEL
        ORDER BY sal DESC
    ) AS sal_rank
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, sal_rank;

--9 ROWS
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    RANK() OVER W AS sal_rank
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
window W AS ( PARTITION BY LEVEL
        ORDER BY sal DESC)
ORDER BY lvl, sal_rank;

--7 rows
SELECT *
FROM (
    SELECT
        LEVEL AS lvl,
        empno,
        ename,
        sal,
        ROW_NUMBER() OVER (
            PARTITION BY LEVEL
            ORDER BY sal DESC
        ) AS rn
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
)
WHERE rn <= 2
ORDER BY lvl, rn;

--9 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    SUM(sal) OVER (
        PARTITION BY LEVEL
    ) AS level_total,
    ROUND(
        sal / SUM(sal) OVER (PARTITION BY LEVEL) * 100,
        2
    ) AS pct
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, empno;

--9 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    SYS_CONNECT_BY_PATH(ename, '/') AS path,
    ROW_NUMBER() OVER (
        PARTITION BY LEVEL
        ORDER BY sal DESC
    ) AS rn
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, rn;

--9 rows
SELECT
    CONNECT_BY_ROOT empno AS root_empno,
    CONNECT_BY_ROOT ename AS root_name,
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    ROW_NUMBER() OVER (
        PARTITION BY CONNECT_BY_ROOT empno
        ORDER BY sal DESC
    ) AS rn
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY root_empno, rn;

--6 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    deptno,
    sal,
    ROW_NUMBER() OVER (
        PARTITION BY deptno
        ORDER BY sal DESC
    ) AS rn_in_dept
FROM emp_hier_test
WHERE sal >= 8000
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY deptno, rn_in_dept;

--9 ROWS
SELECT
    t.*,
    ROW_NUMBER() OVER (
        PARTITION BY t.lvl
        ORDER BY t.sal DESC
    ) AS rn
FROM (
    SELECT
        LEVEL AS lvl,
        empno,
        ename,
        mgr,
        sal
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
) t
ORDER BY t.lvl, rn;

--9 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    SUM(sal) OVER (
        ORDER BY LEVEL, empno
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS running_sal
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, empno;

--9 rows
SELECT
    LEVEL AS lvl,
    empno,
    ename,
    sal,
    SYS_CONNECT_BY_PATH(ename, '/') AS path,
    DENSE_RANK() OVER (
        PARTITION BY LEVEL
        ORDER BY sal DESC
    ) AS dense_rank_in_level
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, dense_rank_in_level;

--4 rows
SELECT DISTINCT
    LEVEL AS lvl
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl;

--5 rows
SELECT DISTINCT
    LEVEL AS lvl,
    mgr
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, mgr;

--9 rows
SELECT DISTINCT
    SYS_CONNECT_BY_PATH(ename, '/') AS path
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY path;

--4 rows
SELECT DISTINCT
    CONNECT_BY_ROOT empno AS root_empno,
    CONNECT_BY_ROOT ename AS root_name,
    LEVEL AS lvl
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY root_empno, lvl;


--6 rows
SELECT DISTINCT
    LEVEL AS lvl,
    deptno,
    COUNT(*) OVER (
        PARTITION BY LEVEL, deptno
    ) AS cnt_by_level_dept
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, deptno;

--6 rows
SELECT DISTINCT
    t.lvl,
    t.deptno
FROM (
    SELECT
        LEVEL AS lvl,
        empno,
        ename,
        mgr,
        deptno,
        sal
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
) t
ORDER BY t.lvl, t.deptno;

--5 rows
SELECT DISTINCT
    LEVEL AS lvl,
    CONNECT_BY_ISLEAF AS is_leaf
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, is_leaf;

--9 rows
SELECT DISTINCT
    LEVEL AS lvl,
    PRIOR empno AS parent_empno,
    empno,
    ename
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, parent_empno, empno;

--5 rows
SELECT DISTINCT
    LEVEL AS lvl,
    deptno
FROM emp_hier_test
WHERE sal >= 8000
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY lvl, deptno;

--6 rows
SELECT
    LEVEL AS lvl,
    deptno
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
GROUP BY LEVEL, deptno
ORDER BY lvl, deptno;

--oracle doesn't support distinct on
--4 rows
SELECT DISTINCT ON (LEVEL)
       LEVEL lvl,
       empno,
       ename,
       sal
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY LEVEL, sal DESC;

--6 rows
SELECT DISTINCT ON (LEVEL, deptno)
       LEVEL lvl,
       deptno,
       empno,
       ename,
       sal
FROM emp_hier_test
START WITH mgr IS NULL
CONNECT BY PRIOR empno = mgr
ORDER BY LEVEL, deptno, sal DESC;


--4 rows
SELECT DISTINCT ON (lvl)
       lvl,
       empno,
       ename,
       sal,
       rn
FROM (
    SELECT
        LEVEL lvl,
        empno,
        ename,
        sal,
        ROW_NUMBER() OVER(
            PARTITION BY LEVEL
            ORDER BY sal DESC
        ) rn
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
) t
ORDER BY lvl, rn;

--into
DECLARE
    v_empno NUMBER;
    v_ename VARCHAR2(30);
BEGIN
    SELECT empno, ename
    INTO v_empno, v_ename
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
    AND LEVEL = 1;

    raise info '%', 'TC38: ' || v_empno || ',' || v_ename;
END;
/


DECLARE
    v_empno NUMBER;
    v_ename VARCHAR2(30);
    v_rn    NUMBER;
BEGIN
    SELECT empno, ename, rn
    INTO v_empno, v_ename, v_rn
    FROM (
        SELECT
            empno,
            ename,
            ROW_NUMBER() OVER (
                ORDER BY sal DESC
            ) rn
        FROM emp_hier_test
    )
    WHERE rn = 1;

    raise info '%','TC39: ' || v_empno || ',' || v_ename || ',' || v_rn;
END;
/

DECLARE
    v_cnt NUMBER;
BEGIN
    SELECT COUNT(*)
    INTO v_cnt
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr;

    raise info '%', 'TC44 count: ' || v_cnt;
END;
/


---test complex come from foreign
CREATE TABLE T_BILLMETHOD (
    FMETHODNO NUMBER NOT NULL,
    FPARENT NUMBER NULL,
    FSYS VARCHAR2(20) NULL,
    FUSED NUMBER(1) NULL,
    FDATARTAG NUMBER(1) NULL,
    FLEVEL NUMBER NULL,
    FMENUORDER NUMBER NULL,
    FNAME VARCHAR2(100) NULL ,
    CONSTRAINT T_BILLMETHOD_PKEY PRIMARY KEY (fmethodno)
);

INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(1, NULL, 'APP', 1, 1, 1, 1, 'Root_1');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(2, NULL, NULL, 0, 1, 1, 2, 'Root_2');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(3, NULL, 'SYS', 1, 1, 1, 3, 'Root_3');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(4, NULL, 'APP', 0, 1, 1, 4, 'Root_4');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(5, NULL, NULL, 1, 1, 1, 5, 'Root_5');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6, NULL, 'SYS', 0, 1, 1, 6, 'Root_6');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(7, NULL, 'APP', 1, 1, 1, 7, 'Root_7');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(8, NULL, NULL, 0, 1, 1, 8, 'Root_8');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(9, 4, NULL, 0, 0, 2, 47.4361239367861, 'Menu_9');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(10, 5, NULL, 0, 0, 2, 24.8728496173327, 'Menu_10');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(11, 1, NULL, 1, 0, 2, 21.7437472821834, 'Menu_11');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(12, 5, NULL, 0, 1, 2, 78.0902816295508, 'Menu_12');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(13, 8, NULL, 0, 0, 2, 47.9599865919571, 'Menu_13');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(14, 13, NULL, 1, 1, 3, 92.7552172322765, 'Menu_14');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(15, 14, NULL, 0, 1, 4, 79.1807505382483, 'Menu_15');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(16, 3, NULL, 1, 1, 2, 94.1096128522209, 'Menu_16');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(17, 7, NULL, 0, 0, 2, 56.9804081527125, 'Menu_17');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(18, 7, NULL, 0, 1, 2, 20.2841950279075, 'Menu_18');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(19, 5, NULL, 0, 0, 2, 50.8360836314022, 'Menu_19');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(20, 11, NULL, 1, 0, 3, 96.9523941473993, 'Menu_20');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(21, 3, NULL, 0, 0, 2, 96.2502200039686, 'Menu_21');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(22, 18, NULL, 1, 1, 3, 64.467469772373, 'Menu_22');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(23, 7, NULL, 0, 0, 2, 67.0693925922032, 'Menu_23');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(24, 22, NULL, 0, 0, 4, 30.8669397541289, 'Menu_24');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(25, 4, NULL, 1, 1, 2, 36.8707009116873, 'Menu_25');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(26, 3, NULL, 1, 1, 2, 51.061329533087, 'Menu_26');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(27, 6, NULL, 0, 0, 2, 17.3320069203035, 'Menu_27');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(28, 6, NULL, 1, 1, 2, 60.7609370767282, 'Menu_28');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(29, 24, NULL, 1, 0, 5, 35.2913405190383, 'Menu_29');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(30, 12, NULL, 1, 0, 3, 82.4841814159467, 'Menu_30');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(31, 6, NULL, 0, 1, 2, 46.27268850649, 'Menu_31');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(32, 8, NULL, 1, 0, 2, 52.5929449413242, 'Menu_32');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(33, 21, NULL, 0, 0, 3, 13.9202758799187, 'Menu_33');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(34, 7, NULL, 1, 0, 2, 96.3584843796794, 'Menu_34');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(35, 28, NULL, 0, 0, 3, 7.83648129939617, 'Menu_35');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(36, 13, NULL, 1, 1, 3, 4.92298515506014, 'Menu_36');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(37, 10, NULL, 1, 1, 3, 65.8583858271884, 'Menu_37');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(38, 2, NULL, 0, 0, 2, 58.8199065529393, 'Menu_38');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(39, 37, NULL, 1, 0, 4, 14.7827787943568, 'Menu_39');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(40, 22, NULL, 1, 0, 4, 11.2714443328181, 'Menu_40');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(41, 21, NULL, 1, 1, 3, 32.1096455233537, 'Menu_41');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(42, 28, NULL, 1, 1, 3, 56.301861538784, 'Menu_42');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(43, 30, NULL, 0, 0, 4, 1.24450723556782, 'Menu_43');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(44, 33, NULL, 1, 0, 4, 11.354399929432, 'Menu_44');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(45, 12, NULL, 0, 0, 3, 99.970247899817, 'Menu_45');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(46, 22, NULL, 1, 1, 4, 98.6492720297557, 'Menu_46');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(47, 8, NULL, 1, 1, 2, 85.8105991813868, 'Menu_47');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(48, 18, NULL, 0, 1, 3, 28.2703611820447, 'Menu_48');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(49, 5, NULL, 0, 0, 2, 87.8724238049663, 'Menu_49');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(50, 46, NULL, 1, 1, 5, 45.4191670700427, 'Menu_50');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(51, 28, NULL, 1, 0, 3, 54.4647317027599, 'Menu_51');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(52, 20, NULL, 0, 0, 4, 34.3509739486882, 'Menu_52');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(53, 30, NULL, 1, 0, 4, 94.4877492568665, 'Menu_53');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(54, 23, NULL, 0, 0, 3, 61.5773867649743, 'Menu_54');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(55, 49, NULL, 1, 0, 3, 39.5602183125853, 'Menu_55');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(56, 35, NULL, 0, 0, 4, 82.1267922512041, 'Menu_56');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(57, 34, NULL, 1, 0, 3, 19.7687638527682, 'Menu_57');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(59, 13, NULL, 0, 0, 3, 67.2114948164476, 'Menu_59');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(60, 48, NULL, 1, 1, 4, 53.7710114300975, 'Menu_60');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(61, 47, NULL, 1, 1, 3, 60.9394650650039, 'Menu_61');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(62, 14, NULL, 0, 1, 4, 90.895317600123, 'Menu_62');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(63, 18, NULL, 1, 0, 3, 86.3131709318989, 'Menu_63');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(64, 9, NULL, 1, 1, 3, 6.64179479983412, 'Menu_64');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(66, 4, NULL, 1, 1, 2, 86.531789542334, 'Menu_66');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(67, 23, NULL, 0, 1, 3, 86.7022250670779, 'Menu_67');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(68, 55, NULL, 1, 1, 4, 96.430931932885, 'Menu_68');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(71, 11, NULL, 1, 1, 3, 38.0878984573824, 'Menu_71');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(72, 30, NULL, 1, 0, 4, 31.0466570066276, 'Menu_72');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(73, 14, NULL, 1, 0, 4, 62.8729934165914, 'Menu_73');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(74, 38, NULL, 1, 1, 3, 36.7783542152481, 'Menu_74');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(75, 8, NULL, 0, 0, 2, 31.4791906445288, 'Menu_75');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(76, 54, NULL, 1, 0, 4, 4.23152666209469, 'Menu_76');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(77, 58, NULL, 0, 1, 6, 63.1330173507724, 'Menu_77');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(78, 5, NULL, 1, 0, 2, 37.5937613578239, 'Menu_78');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(79, 77, NULL, 1, 0, 7, 67.0152786262098, 'Menu_79');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(81, 79, NULL, 0, 1, 8, 37.5371440628079, 'Menu_81');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(82, 18, NULL, 1, 0, 3, 75.5480256935598, 'Menu_82');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(83, 56, NULL, 0, 0, 5, 22.1571491356965, 'Menu_83');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(84, 65, NULL, 0, 1, 6, 96.391460038888, 'Menu_84');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(85, 83, NULL, 1, 0, 6, 54.2351061431455, 'Menu_85');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(86, 22, NULL, 1, 0, 4, 10.667311761918, 'Menu_86');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(87, 50, NULL, 1, 0, 6, 75.5420663378248, 'Menu_87');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(88, 11, NULL, 1, 0, 3, 5.58947104623043, 'Menu_88');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(89, 55, NULL, 0, 1, 4, 83.9785435151186, 'Menu_89');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(90, 62, NULL, 0, 1, 5, 34.5693336753828, 'Menu_90');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(91, 63, NULL, 1, 0, 4, 84.5329746204781, 'Menu_91');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(92, 13, NULL, 0, 1, 3, 33.4930546079597, 'Menu_92');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(93, 10, NULL, 0, 1, 3, 83.1237653576657, 'Menu_93');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(94, 19, NULL, 0, 0, 3, 26.5198482635361, 'Menu_94');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(96, 49, NULL, 1, 1, 3, 2.73521390934783, 'Menu_96');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(97, 40, NULL, 1, 0, 5, 66.0905546428137, 'Menu_97');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(98, 87, NULL, 1, 0, 7, 78.5024884192919, 'Menu_98');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(99, 33, NULL, 0, 0, 4, 6.28144238770677, 'Menu_99');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(101, 26, NULL, 1, 0, 3, 66.3688203138628, 'Menu_101');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(102, 32, NULL, 1, 0, 3, 28.808190113302, 'Menu_102');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(103, 96, NULL, 1, 0, 4, 68.1003288889434, 'Menu_103');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(104, 36, NULL, 0, 1, 4, 79.167968387138, 'Menu_104');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(106, 69, NULL, 0, 0, 4, 19.2980205228923, 'Menu_106');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(107, 35, NULL, 1, 0, 4, 84.7545796420123, 'Menu_107');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(108, 24, NULL, 1, 0, 5, 67.4128261190027, 'Menu_108');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(109, 15, NULL, 0, 1, 5, 48.5113772458812, 'Menu_109');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(110, 35, NULL, 0, 0, 4, 19.864212210389, 'Menu_110');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(111, 2, NULL, 1, 0, 2, 67.2682831370146, 'Menu_111');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(113, 92, NULL, 0, 0, 4, 80.940883336097, 'Menu_113');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(114, 84, NULL, 1, 0, 7, 43.7852865504077, 'Menu_114');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(115, 68, NULL, 1, 0, 5, 25.8019081751172, 'Menu_115');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(116, 49, NULL, 0, 0, 3, 99.8673925669944, 'Menu_116');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(117, 56, NULL, 1, 1, 5, 86.6134057074962, 'Menu_117');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(118, 45, NULL, 0, 1, 4, 79.4036382974844, 'Menu_118');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(119, 75, NULL, 1, 1, 3, 95.3690791503141, 'Menu_119');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(120, 61, NULL, 0, 0, 4, 88.4832519253618, 'Menu_120');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(121, 78, NULL, 0, 0, 3, 5.71381341474526, 'Menu_121');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(122, 94, NULL, 1, 1, 4, 47.7826438692702, 'Menu_122');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(123, 52, NULL, 1, 1, 5, 89.887074530743, 'Menu_123');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(124, 29, NULL, 1, 0, 6, 5.36735815566862, 'Menu_124');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(125, 71, NULL, 1, 1, 4, 62.7814199575564, 'Menu_125');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(126, 31, NULL, 1, 1, 3, 19.1777403566397, 'Menu_126');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(127, 57, NULL, 0, 0, 4, 73.5702810755135, 'Menu_127');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(128, 27, NULL, 0, 1, 3, 76.8303189361153, 'Menu_128');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(129, 110, NULL, 0, 1, 5, 54.3490300362482, 'Menu_129');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(130, 97, NULL, 0, 1, 6, 32.6393994473024, 'Menu_130');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(131, 39, NULL, 0, 1, 5, 53.194347201091, 'Menu_131');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(132, 42, NULL, 0, 0, 4, 18.2737497548066, 'Menu_132');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(133, 53, NULL, 0, 0, 5, 73.8908313625964, 'Menu_133');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(135, 41, NULL, 0, 1, 4, 55.6545390302443, 'Menu_135');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(136, 10, NULL, 1, 1, 3, 96.5485709802978, 'Menu_136');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(137, 40, NULL, 1, 0, 5, 97.056140211444, 'Menu_137');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(139, 74, NULL, 0, 0, 4, 67.2478213547749, 'Menu_139');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(140, 21, NULL, 1, 1, 3, 91.4916139080241, 'Menu_140');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(141, 136, NULL, 0, 0, 4, 32.0733135066826, 'Menu_141');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(142, 135, NULL, 1, 0, 5, 81.6649977916005, 'Menu_142');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(143, 41, NULL, 0, 1, 4, 94.8157487262884, 'Menu_143');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(144, 4, NULL, 0, 1, 2, 26.0189527529854, 'Menu_144');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(145, 117, NULL, 0, 1, 6, 94.1357790639727, 'Menu_145');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(146, 105, NULL, 1, 1, 5, 18.4420977671959, 'Menu_146');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(147, 65, NULL, 1, 0, 6, 6.40847201112289, 'Menu_147');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(148, 57, NULL, 1, 1, 4, 8.79545117666076, 'Menu_148');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(149, 5, NULL, 1, 0, 2, 61.6301839300016, 'Menu_149');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(150, 138, NULL, 0, 1, 5, 95.8173747204738, 'Menu_150');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(151, 41, NULL, 0, 1, 4, 38.7900650399312, 'Menu_151');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(152, 120, NULL, 1, 1, 5, 15.8562758716955, 'Menu_152');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(153, 14, NULL, 1, 0, 4, 10.3172489963182, 'Menu_153');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(154, 54, NULL, 1, 0, 4, 46.8688475651547, 'Menu_154');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(155, 102, NULL, 0, 1, 4, 75.3209989439298, 'Menu_155');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(156, 64, NULL, 0, 1, 4, 21.5347896557467, 'Menu_156');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(157, 124, NULL, 0, 1, 7, 59.6269237272697, 'Menu_157');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(159, 26, NULL, 0, 1, 3, 83.3276498853132, 'Menu_159');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(160, 58, NULL, 1, 1, 6, 96.390428737501, 'Menu_160');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(161, 64, NULL, 0, 1, 4, 26.8414551553079, 'Menu_161');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(162, 78, NULL, 1, 0, 3, 85.5147951701609, 'Menu_162');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(163, 132, NULL, 0, 1, 5, 25.0552734190296, 'Menu_163');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(164, 54, NULL, 1, 1, 4, 93.6404661282327, 'Menu_164');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(165, 87, NULL, 1, 0, 7, 44.9840911247379, 'Menu_165');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(166, 30, NULL, 0, 0, 4, 23.8938680939353, 'Menu_166');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(167, 54, NULL, 0, 1, 4, 94.6425197319645, 'Menu_167');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(168, 160, NULL, 0, 0, 7, 69.1046975759734, 'Menu_168');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(169, 88, NULL, 1, 1, 4, 7.12490875773955, 'Menu_169');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(170, 126, NULL, 1, 0, 4, 84.7947764269999, 'Menu_170');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(171, 82, NULL, 1, 0, 4, 4.67769064447287, 'Menu_171');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(172, 147, NULL, 1, 1, 7, 73.336390476991, 'Menu_172');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(173, 169, NULL, 1, 0, 5, 80.6346001203881, 'Menu_173');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(174, 156, NULL, 1, 1, 5, 58.8677427435099, 'Menu_174');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(175, 70, NULL, 0, 1, 5, 79.4465012961875, 'Menu_175');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(176, 18, NULL, 1, 1, 3, 14.8429032935752, 'Menu_176');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(177, 144, NULL, 1, 0, 3, 59.371285779185, 'Menu_177');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(178, 128, NULL, 0, 0, 4, 27.8658719794966, 'Menu_178');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(179, 133, NULL, 0, 0, 6, 95.897415039631, 'Menu_179');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(180, 12, NULL, 0, 0, 3, 98.9074460492888, 'Menu_180');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(181, 73, NULL, 0, 1, 5, 34.243192815859, 'Menu_181');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(182, 50, NULL, 0, 0, 6, 20.4837725663987, 'Menu_182');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(183, 52, NULL, 0, 0, 5, 19.4556678275507, 'Menu_183');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(184, 91, NULL, 1, 0, 5, 89.1399817587536, 'Menu_184');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(185, 20, NULL, 0, 0, 4, 45.3434182795329, 'Menu_185');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(186, 165, NULL, 1, 0, 8, 57.824426074544, 'Menu_186');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(187, 162, NULL, 1, 1, 4, 6.11294061021661, 'Menu_187');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(188, 41, NULL, 0, 0, 4, 89.5082881655068, 'Menu_188');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(189, 139, NULL, 1, 0, 5, 46.8074966907133, 'Menu_189');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(191, 45, NULL, 0, 1, 4, 39.0802755064861, 'Menu_191');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(192, 21, NULL, 0, 1, 3, 11.4688217624962, 'Menu_192');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(193, 189, NULL, 1, 0, 6, 43.5222217374615, 'Menu_193');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(194, 109, NULL, 1, 0, 6, 84.3061680620732, 'Menu_194');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(196, 35, NULL, 1, 1, 4, 36.7967073828207, 'Menu_196');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(197, 115, NULL, 0, 0, 6, 2.65788826985105, 'Menu_197');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(198, 152, NULL, 0, 1, 6, 47.8189759562948, 'Menu_198');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(199, 112, NULL, 1, 1, 5, 30.8487122668323, 'Menu_199');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(200, 2, NULL, 1, 1, 2, 15.3811877304938, 'Menu_200');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(201, 180, NULL, 0, 1, 4, 60.6614866927911, 'Menu_201');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(202, 143, NULL, 0, 1, 5, 24.7843222263427, 'Menu_202');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(203, 47, NULL, 1, 1, 3, 9.4555891768756, 'Menu_203');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(204, 186, NULL, 0, 1, 9, 38.8654859653839, 'Menu_204');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(205, 159, NULL, 0, 0, 4, 42.1720449131962, 'Menu_205');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(206, 78, NULL, 1, 1, 3, 62.0732147018577, 'Menu_206');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(207, 106, NULL, 0, 0, 5, 84.6988723581425, 'Menu_207');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(208, 88, NULL, 1, 1, 4, 32.5539557483093, 'Menu_208');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(209, 162, NULL, 0, 1, 4, 89.6502492505505, 'Menu_209');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(210, 182, NULL, 1, 1, 7, 67.7869984442716, 'Menu_210');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(211, 97, NULL, 0, 1, 6, 12.6750669511203, 'Menu_211');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(212, 30, NULL, 0, 0, 4, 33.5048577752443, 'Menu_212');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(213, 113, NULL, 1, 1, 5, 96.8961193132261, 'Menu_213');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(214, 14, NULL, 1, 0, 4, 38.5332537578013, 'Menu_214');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(215, 30, NULL, 1, 0, 4, 72.5732416666788, 'Menu_215');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(216, 194, NULL, 1, 0, 7, 94.9852441220891, 'Menu_216');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(217, 177, NULL, 0, 1, 4, 5.71916508359797, 'Menu_217');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(218, 203, NULL, 1, 0, 4, 20.6334673237735, 'Menu_218');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(219, 188, NULL, 0, 0, 5, 38.698104116549, 'Menu_219');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(220, 108, NULL, 1, 1, 6, 32.4803459827994, 'Menu_220');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(221, 173, NULL, 0, 1, 6, 48.4444792204868, 'Menu_221');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(222, 19, NULL, 0, 0, 3, 38.0831862679428, 'Menu_222');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(223, 86, NULL, 0, 1, 5, 12.0033837212402, 'Menu_223');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(225, 54, NULL, 1, 0, 4, 90.2255079817692, 'Menu_225');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(226, 152, NULL, 1, 1, 6, 75.5957014291448, 'Menu_226');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(227, 220, NULL, 1, 0, 7, 29.2172049519389, 'Menu_227');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(228, 184, NULL, 0, 1, 6, 29.7344621020385, 'Menu_228');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(229, 226, NULL, 0, 1, 7, 27.0104804299307, 'Menu_229');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(230, 55, NULL, 0, 1, 4, 76.8347812548913, 'Menu_230');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(231, 55, NULL, 1, 1, 4, 70.0420169203225, 'Menu_231');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(232, 4, NULL, 0, 1, 2, 14.3716343273193, 'Menu_232');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(233, 159, NULL, 1, 0, 4, 55.1246592347853, 'Menu_233');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(234, 17, NULL, 0, 1, 3, 17.8533714515409, 'Menu_234');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(235, 89, NULL, 0, 0, 5, 81.9100308192049, 'Menu_235');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(236, 123, NULL, 1, 1, 6, 48.9420663841414, 'Menu_236');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(237, 131, NULL, 1, 0, 6, 95.7883466517334, 'Menu_237');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(238, 40, NULL, 0, 0, 5, 15.4029521799839, 'Menu_238');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(239, 71, NULL, 0, 1, 4, 22.7230323305872, 'Menu_239');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(240, 151, NULL, 1, 1, 5, 17.9520697348546, 'Menu_240');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(241, 120, NULL, 1, 1, 5, 20.135810703534, 'Menu_241');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(242, 84, NULL, 0, 0, 7, 28.2038852304237, 'Menu_242');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(243, 205, NULL, 0, 1, 5, 33.836330647501, 'Menu_243');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(244, 102, NULL, 0, 1, 4, 83.6446760531981, 'Menu_244');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(245, 166, NULL, 1, 0, 5, 39.0574780591932, 'Menu_245');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(246, 244, NULL, 0, 0, 5, 95.1498866326957, 'Menu_246');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(247, 153, NULL, 0, 0, 5, 6.178180923215, 'Menu_247');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(248, 192, NULL, 1, 1, 4, 84.4649660090178, 'Menu_248');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(249, 129, NULL, 1, 0, 6, 41.6376348298469, 'Menu_249');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(250, 4, NULL, 0, 1, 2, 91.9406108765901, 'Menu_250');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(251, 78, NULL, 0, 1, 3, 66.8400906980411, 'Menu_251');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(252, 203, NULL, 1, 0, 4, 34.4852267398849, 'Menu_252');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(253, 150, NULL, 1, 1, 6, 64.8419635533849, 'Menu_253');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(254, 230, NULL, 0, 0, 5, 24.1969418400764, 'Menu_254');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(255, 65, NULL, 1, 1, 6, 57.8403218344487, 'Menu_255');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(256, 62, NULL, 0, 1, 5, 44.9814116873683, 'Menu_256');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(257, 39, NULL, 1, 0, 5, 46.3374415585265, 'Menu_257');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(258, 234, NULL, 1, 1, 4, 50.0986953889949, 'Menu_258');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(259, 80, NULL, 0, 0, 4, 68.3478929132289, 'Menu_259');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(260, 25, NULL, 0, 1, 3, 48.3383062628763, 'Menu_260');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(261, 81, NULL, 1, 1, 9, 41.4062926094366, 'Menu_261');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(262, 54, NULL, 1, 1, 4, 31.3545001643049, 'Menu_262');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(263, 195, NULL, 0, 1, 4, 74.2936316594802, 'Menu_263');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(264, 43, NULL, 1, 1, 5, 29.1268602158353, 'Menu_264');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(265, 135, NULL, 1, 1, 5, 90.346407697137, 'Menu_265');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(266, 189, NULL, 0, 1, 6, 6.6154395960544, 'Menu_266');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(267, 257, NULL, 1, 1, 6, 95.1783633655987, 'Menu_267');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(268, 143, NULL, 0, 0, 5, 81.1082511235947, 'Menu_268');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(269, 37, NULL, 1, 0, 4, 91.6098134726193, 'Menu_269');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(270, 169, NULL, 1, 1, 5, 87.4268787808645, 'Menu_270');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(272, 107, NULL, 1, 0, 5, 30.1880411180837, 'Menu_272');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(273, 158, NULL, 0, 1, 6, 97.8153911189541, 'Menu_273');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(274, 175, NULL, 1, 1, 6, 68.8375814874734, 'Menu_274');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(275, 104, NULL, 1, 1, 5, 7.83942398710989, 'Menu_275');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(277, 250, NULL, 0, 0, 3, 85.2939279538797, 'Menu_277');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(278, 88, NULL, 0, 0, 4, 84.2136608011355, 'Menu_278');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(279, 89, NULL, 0, 1, 5, 64.490455133337, 'Menu_279');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(280, 86, NULL, 0, 0, 5, 95.7368769064747, 'Menu_280');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(281, 74, NULL, 1, 0, 4, 79.8421929224758, 'Menu_281');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(282, 55, NULL, 0, 1, 4, 89.9576355348597, 'Menu_282');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(283, 75, NULL, 0, 1, 3, 84.1413943775018, 'Menu_283');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(284, 239, NULL, 1, 0, 5, 29.6346698254302, 'Menu_284');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(285, 173, NULL, 1, 0, 6, 42.1439116340247, 'Menu_285');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(286, 237, NULL, 1, 1, 7, 42.3020237213613, 'Menu_286');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(287, 147, NULL, 1, 1, 7, 84.0958245885669, 'Menu_287');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(288, 106, NULL, 0, 0, 5, 45.8717761417278, 'Menu_288');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(289, 94, NULL, 0, 1, 4, 43.8034676970327, 'Menu_289');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(290, 190, NULL, 1, 0, 5, 16.8521979284961, 'Menu_290');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(291, 6, NULL, 0, 0, 2, 8.89638856170201, 'Menu_291');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(292, 172, NULL, 1, 1, 8, 95.8220964273748, 'Menu_292');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(293, 35, NULL, 1, 1, 4, 38.3281644234532, 'Menu_293');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(294, 266, NULL, 0, 1, 7, 3.52834774498573, 'Menu_294');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(295, 168, NULL, 1, 1, 8, 86.4894708798686, 'Menu_295');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(296, 197, NULL, 1, 0, 7, 73.7454749282714, 'Menu_296');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(297, 89, NULL, 0, 0, 5, 20.1943679082892, 'Menu_297');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(298, 268, NULL, 0, 1, 6, 20.6534192836133, 'Menu_298');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(299, 88, NULL, 1, 0, 4, 16.3594765906998, 'Menu_299');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(300, 122, NULL, 1, 0, 5, 74.8770917895272, 'Menu_300');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(301, 121, NULL, 0, 0, 4, 65.8981926401571, 'Menu_301');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(302, 256, NULL, 1, 1, 6, 80.6331537372722, 'Menu_302');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(303, 263, NULL, 1, 0, 5, 54.25921804895, 'Menu_303');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(305, 292, NULL, 1, 1, 9, 43.8280056098142, 'Menu_305');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(307, 226, NULL, 0, 1, 7, 98.4962953578307, 'Menu_307');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(309, 285, NULL, 0, 1, 7, 23.4092625626557, 'Menu_309');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(310, 255, NULL, 0, 1, 7, 18.7428108299061, 'Menu_310');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(311, 50, NULL, 1, 1, 6, 42.8250990395119, 'Menu_311');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(312, 87, NULL, 1, 0, 7, 11.260285746995, 'Menu_312');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(313, 213, NULL, 1, 0, 6, 64.9564857074056, 'Menu_313');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(314, 280, NULL, 0, 1, 6, 70.8849395174002, 'Menu_314');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(315, 25, NULL, 0, 1, 3, 78.9867594330863, 'Menu_315');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(316, 196, NULL, 1, 0, 5, 54.6458311346313, 'Menu_316');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(317, 215, NULL, 0, 1, 5, 44.2210092865995, 'Menu_317');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(318, 88, NULL, 0, 1, 4, 52.2360294989433, 'Menu_318');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(319, 124, NULL, 1, 1, 7, 25.8066726566738, 'Menu_319');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(320, 48, NULL, 1, 0, 4, 21.2457717765631, 'Menu_320');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(321, 151, NULL, 1, 1, 5, 26.3771723514847, 'Menu_321');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(322, 131, NULL, 0, 1, 6, 94.3950910396383, 'Menu_322');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(323, 312, NULL, 1, 1, 8, 4.19680360184925, 'Menu_323');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(324, 185, NULL, 1, 1, 5, 32.7938367622788, 'Menu_324');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(325, 266, NULL, 0, 0, 7, 85.9358961639731, 'Menu_325');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(326, 128, NULL, 0, 1, 4, 86.3944797920088, 'Menu_326');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(327, 87, NULL, 1, 1, 7, 69.0059142306427, 'Menu_327');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(328, 205, NULL, 1, 0, 5, 83.3402614747454, 'Menu_328');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(329, 99, NULL, 0, 1, 5, 43.4835562096374, 'Menu_329');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(330, 180, NULL, 0, 1, 4, 66.1164754861131, 'Menu_330');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(331, 264, NULL, 0, 0, 6, 20.52124663137, 'Menu_331');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(332, 26, NULL, 0, 0, 3, 32.301162367359, 'Menu_332');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(333, 201, NULL, 0, 0, 5, 59.6578616321492, 'Menu_333');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(334, 166, NULL, 1, 0, 5, 3.84650878006317, 'Menu_334');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(335, 141, NULL, 1, 0, 5, 93.4440249400683, 'Menu_335');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(338, 270, NULL, 1, 1, 6, 24.2175985743739, 'Menu_338');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(339, 277, NULL, 1, 1, 4, 21.1224630783388, 'Menu_339');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(340, 86, NULL, 0, 0, 5, 75.2750836839891, 'Menu_340');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(341, 88, NULL, 1, 0, 4, 73.1803243464888, 'Menu_341');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(342, 123, NULL, 1, 1, 6, 6.79243875534213, 'Menu_342');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(343, 233, NULL, 0, 0, 5, 37.5254077046318, 'Menu_343');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(344, 65, NULL, 0, 1, 6, 47.660538992821, 'Menu_344');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(345, 344, NULL, 1, 0, 7, 51.5168940092692, 'Menu_345');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(346, 236, NULL, 0, 0, 7, 66.1291632169399, 'Menu_346');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(347, 249, NULL, 0, 1, 7, 59.0131491940726, 'Menu_347');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(348, 67, NULL, 0, 0, 4, 20.88021014746, 'Menu_348');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(349, 92, NULL, 0, 0, 4, 91.5685067147559, 'Menu_349');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(350, 121, NULL, 1, 0, 4, 55.7008706902412, 'Menu_350');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(351, 187, NULL, 1, 1, 5, 23.6084937136397, 'Menu_351');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(352, 158, NULL, 0, 1, 6, 10.3528262410665, 'Menu_352');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(353, 126, NULL, 0, 1, 4, 90.0168104449952, 'Menu_353');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(354, 19, NULL, 0, 1, 3, 9.470881933518, 'Menu_354');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(355, 285, NULL, 1, 1, 7, 27.758632680123, 'Menu_355');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(356, 315, NULL, 1, 1, 4, 9.41746988681496, 'Menu_356');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(357, 41, NULL, 0, 1, 4, 50.3623436943068, 'Menu_357');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(358, 128, NULL, 1, 0, 4, 31.6893585119539, 'Menu_358');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(359, 342, NULL, 1, 1, 7, 10.517635517747, 'Menu_359');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(360, 5, NULL, 1, 0, 2, 34.5741432164319, 'Menu_360');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(361, 99, NULL, 1, 1, 5, 30.29361303158, 'Menu_361');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(362, 253, NULL, 1, 0, 7, 84.1287083196353, 'Menu_362');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(363, 238, NULL, 0, 0, 6, 55.1564666965288, 'Menu_363');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(364, 320, NULL, 0, 0, 5, 65.5365107144567, 'Menu_364');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(365, 80, NULL, 0, 0, 4, 42.0196305538374, 'Menu_365');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(367, 267, NULL, 1, 1, 7, 13.5594726744105, 'Menu_367');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(368, 199, NULL, 0, 1, 6, 56.7177662045156, 'Menu_368');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(370, 140, NULL, 1, 1, 4, 66.7099446181963, 'Menu_370');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(371, 222, NULL, 1, 1, 4, 65.6900286274717, 'Menu_371');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(372, 290, NULL, 1, 1, 6, 54.6966961746723, 'Menu_372');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(373, 249, NULL, 1, 1, 7, 15.2210927204157, 'Menu_373');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(374, 145, NULL, 1, 0, 7, 54.4656854242697, 'Menu_374');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(375, 29, NULL, 1, 1, 6, 74.3875867042748, 'Menu_375');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(376, 158, NULL, 1, 1, 6, 99.3349337981335, 'Menu_376');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(377, 248, NULL, 1, 0, 5, 69.3636292465239, 'Menu_377');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(378, 147, NULL, 0, 0, 7, 78.5982045351999, 'Menu_378');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(379, 243, NULL, 1, 1, 6, 10.2000761647627, 'Menu_379');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(380, 205, NULL, 1, 1, 5, 35.1123133350361, 'Menu_380');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(381, 103, NULL, 0, 1, 5, 22.9962953295972, 'Menu_381');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(382, 241, NULL, 1, 0, 6, 48.9280578534207, 'Menu_382');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(383, 145, NULL, 0, 1, 7, 99.8174560854178, 'Menu_383');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(384, 91, NULL, 0, 1, 5, 12.9413653197588, 'Menu_384');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(385, 82, NULL, 1, 0, 4, 17.3357891917362, 'Menu_385');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(386, 130, NULL, 1, 1, 7, 66.7844493324528, 'Menu_386');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(387, 41, NULL, 0, 1, 4, 12.1752597783696, 'Menu_387');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(388, 232, NULL, 1, 1, 3, 30.1403415485958, 'Menu_388');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(389, 101, NULL, 1, 0, 4, 57.7783121716164, 'Menu_389');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(390, 61, NULL, 0, 0, 4, 94.4737920279103, 'Menu_390');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(391, 69, NULL, 0, 1, 4, 69.9062616852392, 'Menu_391');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(392, 238, NULL, 0, 1, 6, 72.3594808168567, 'Menu_392');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(393, 220, NULL, 0, 0, 7, 30.8478140476927, 'Menu_393');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(394, 203, NULL, 1, 0, 4, 36.790833418593, 'Menu_394');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(395, 222, NULL, 1, 1, 4, 95.6810141051171, 'Menu_395');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(396, 2, NULL, 1, 0, 2, 85.1207213805677, 'Menu_396');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(397, 211, NULL, 0, 0, 7, 46.6824058799685, 'Menu_397');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(398, 396, NULL, 1, 1, 3, 22.6756740452261, 'Menu_398');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(400, 258, NULL, 1, 1, 5, 44.4882884962029, 'Menu_400');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(401, 223, NULL, 1, 0, 6, 69.5999607487975, 'Menu_401');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(403, 51, NULL, 1, 1, 4, 73.8276137017239, 'Menu_403');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(404, 96, NULL, 0, 1, 4, 54.0360727809612, 'Menu_404');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(405, 210, NULL, 0, 0, 8, 29.506195940139, 'Menu_405');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(406, 329, NULL, 1, 0, 6, 14.6937006630739, 'Menu_406');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(408, 330, NULL, 0, 1, 5, 17.893836249408, 'Menu_408');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(409, 16, NULL, 0, 1, 3, 81.0467221246887, 'Menu_409');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(410, 235, NULL, 1, 1, 6, 50.3548767961029, 'Menu_410');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(411, 202, NULL, 1, 1, 6, 90.6740434765089, 'Menu_411');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(412, 257, NULL, 0, 1, 6, 22.2468038941245, 'Menu_412');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(414, 212, NULL, 0, 1, 5, 49.1830970259927, 'Menu_414');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(415, 221, NULL, 1, 0, 7, 16.2456049764613, 'Menu_415');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(416, 358, NULL, 1, 0, 5, 89.3530683798769, 'Menu_416');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(417, 330, NULL, 1, 1, 5, 82.0133564972081, 'Menu_417');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(418, 171, NULL, 0, 1, 5, 10.3439948402204, 'Menu_418');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(420, 229, NULL, 0, 1, 8, 52.4550891276476, 'Menu_420');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(421, 89, NULL, 1, 0, 5, 47.2574985975752, 'Menu_421');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(423, 46, NULL, 1, 1, 5, 30.2386118747382, 'Menu_423');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(424, 18, NULL, 1, 1, 3, 2.31444927131558, 'Menu_424');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(425, 60, NULL, 0, 1, 5, 58.1100989070284, 'Menu_425');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(426, 403, NULL, 0, 0, 5, 80.5561298880013, 'Menu_426');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(427, 274, NULL, 0, 0, 7, 55.5680036521866, 'Menu_427');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(428, 375, NULL, 0, 0, 7, 10.6838397563799, 'Menu_428');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(429, 11, NULL, 1, 0, 3, 35.6939655249846, 'Menu_429');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(430, 127, NULL, 0, 1, 5, 48.7926506280509, 'Menu_430');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(431, 224, NULL, 1, 0, 5, 83.9839171218452, 'Menu_431');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(432, 133, NULL, 1, 0, 6, 30.261375039655, 'Menu_432');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(433, 182, NULL, 1, 1, 7, 79.09423287924, 'Menu_433');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(434, 46, NULL, 1, 0, 5, 87.2865229721869, 'Menu_434');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(435, 245, NULL, 1, 1, 6, 60.5109711008671, 'Menu_435');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(436, 60, NULL, 0, 1, 5, 77.4011049125137, 'Menu_436');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(437, 131, NULL, 1, 0, 6, 60.0850968167488, 'Menu_437');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(439, 193, NULL, 0, 0, 7, 67.7850172733096, 'Menu_439');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(440, 27, NULL, 1, 1, 3, 38.5307546838963, 'Menu_440');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(441, 209, NULL, 0, 1, 5, 52.9031539077066, 'Menu_441');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(442, 76, NULL, 0, 0, 5, 8.91045923710526, 'Menu_442');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(443, 124, NULL, 1, 1, 7, 72.3208476678654, 'Menu_443');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(444, 435, NULL, 0, 1, 7, 76.2028878097573, 'Menu_444');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(445, 307, NULL, 0, 1, 8, 61.8416330659232, 'Menu_445');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(446, 415, NULL, 1, 0, 8, 93.0152474915372, 'Menu_446');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(447, 355, NULL, 1, 1, 8, 96.4685912752653, 'Menu_447');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(448, 140, NULL, 1, 0, 4, 97.8507663968863, 'Menu_448');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(449, 353, NULL, 1, 1, 5, 51.0717444366326, 'Menu_449');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(450, 8, NULL, 1, 1, 2, 17.919449390423, 'Menu_450');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(451, 95, NULL, 1, 0, 5, 70.6672707966065, 'Menu_451');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(453, 95, NULL, 0, 1, 5, 32.4727331971518, 'Menu_453');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(454, 237, NULL, 1, 0, 7, 36.0922393496672, 'Menu_454');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(455, 232, NULL, 0, 0, 3, 8.78326656299734, 'Menu_455');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(456, 44, NULL, 1, 0, 5, 82.5702450144181, 'Menu_456');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(457, 320, NULL, 0, 1, 5, 7.25636754454206, 'Menu_457');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(458, 136, NULL, 0, 1, 4, 93.7619058338727, 'Menu_458');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(459, 177, NULL, 1, 1, 4, 96.7116773031131, 'Menu_459');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(460, 153, NULL, 0, 0, 5, 98.8109075502062, 'Menu_460');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(461, 411, NULL, 1, 1, 7, 93.8520952413387, 'Menu_461');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(462, 320, NULL, 0, 1, 5, 34.8951180516116, 'Menu_462');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(463, 324, NULL, 0, 1, 6, 76.0739529890947, 'Menu_463');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(464, 8, NULL, 1, 0, 2, 98.3589955703082, 'Menu_464');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(466, 465, NULL, 0, 0, 10, 8.0850942499317, 'Menu_466');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(467, 387, NULL, 0, 0, 5, 33.2880194416953, 'Menu_467');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(468, 222, NULL, 1, 0, 4, 31.6074311701784, 'Menu_468');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(469, 243, NULL, 1, 0, 6, 66.5182022595693, 'Menu_469');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(470, 352, NULL, 1, 0, 7, 95.9900938649264, 'Menu_470');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(471, 9, NULL, 0, 0, 3, 94.2544553813828, 'Menu_471');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(472, 170, NULL, 1, 0, 5, 69.4151180862331, 'Menu_472');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(473, 30, NULL, 1, 1, 4, 62.4613802674321, 'Menu_473');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(474, 210, NULL, 1, 1, 8, 5.43939007865594, 'Menu_474');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(475, 265, NULL, 0, 1, 6, 13.2994927928644, 'Menu_475');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(476, 175, NULL, 1, 1, 6, 94.1846862459806, 'Menu_476');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(477, 202, NULL, 0, 0, 6, 79.379171092674, 'Menu_477');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(478, 139, NULL, 0, 1, 5, 5.15889087455147, 'Menu_478');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(479, 30, NULL, 1, 0, 4, 33.2645429380338, 'Menu_479');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(480, 24, NULL, 1, 1, 5, 37.5237325581187, 'Menu_480');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(481, 1, NULL, 1, 0, 2, 44.9260178114719, 'Menu_481');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(482, 198, NULL, 1, 0, 7, 51.1485847729763, 'Menu_482');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(483, 15, NULL, 0, 1, 5, 70.6011104919024, 'Menu_483');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(484, 182, NULL, 0, 0, 7, 92.6195175407835, 'Menu_484');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(485, 53, NULL, 0, 1, 5, 17.2547696255191, 'Menu_485');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(486, 236, NULL, 1, 1, 7, 69.8483881229885, 'Menu_486');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(487, 433, NULL, 1, 1, 8, 13.0342167776029, 'Menu_487');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(488, 26, NULL, 0, 1, 3, 98.5228731057567, 'Menu_488');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(489, 84, NULL, 0, 0, 7, 80.1762940696404, 'Menu_489');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(490, 383, NULL, 1, 0, 8, 3.09773147173816, 'Menu_490');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(491, 363, NULL, 0, 0, 7, 6.61478021593558, 'Menu_491');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(492, 174, NULL, 1, 1, 6, 91.7748595927941, 'Menu_492');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(493, 18, NULL, 1, 0, 3, 72.0073363459348, 'Menu_493');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(494, 180, NULL, 1, 1, 4, 7.40024504584698, 'Menu_494');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(495, 493, NULL, 0, 0, 4, 17.2141382752162, 'Menu_495');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(496, 413, NULL, 0, 0, 6, 89.9483389290806, 'Menu_496');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(498, 71, NULL, 1, 0, 4, 55.619356277618, 'Menu_498');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(499, 30, NULL, 1, 1, 4, 26.8820516998951, 'Menu_499');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(500, 385, NULL, 0, 0, 5, 45.8896666703354, 'Menu_500');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640528, 32, NULL, 0, 1, 3, 1.01759141481331, 'Menu_271');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640454, 74, NULL, 0, 0, 4, 85.7354723584726, 'Menu_134');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640477, 269, NULL, 0, 1, 5, 39.0235647386552, 'Menu_369');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640460, 180, NULL, 1, 0, 4, 92.2550495657388, 'Menu_190');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(664046, 18, NULL, 1, 0, 3, 81.9643386894949, 'Menu_69');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640421, 45, NULL, 1, 1, 4, 60.9626643919389, 'Menu_336');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640515, 48, NULL, 1, 1, 4, 66.6742180505519, 'Menu_138');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640504, 289, NULL, 0, 0, 5, 41.1647039685639, 'Menu_452');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640510, 26, NULL, 0, 0, 3, 44.7371383499946, 'Menu_195');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640462, 87, NULL, 0, 0, 7, 57.3198877091935, 'Menu_100');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640473, 60, NULL, 1, 1, 5, 17.0456073970901, 'Menu_407');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640489, 76, NULL, 0, 0, 5, 98.861157876259, 'Menu_337');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(121220, 92, NULL, 1, 0, 4, 92.6198860303574, 'Menu_224');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640461, 52, NULL, 1, 0, 5, 99.4406622748911, 'Menu_438');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640512, 64, NULL, 0, 1, 4, 78.2546390301739, 'Menu_70');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640422, 291, NULL, 0, 0, 3, 96.6762634353868, 'Menu_419');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640447, 124, NULL, 1, 1, 7, 68.1845396840253, 'Menu_304');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640506, 58, NULL, 1, 0, 6, 81.7411605942285, 'Menu_308');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640499, 209, NULL, 1, 0, 5, 99.005540905745, 'Menu_399');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640456, 53, NULL, 1, 0, 5, 32.8924709058112, 'Menu_413');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640487, 200, NULL, 1, 1, 3, 44.4748341778992, 'Menu_497');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640503, 88, NULL, 1, 0, 4, 76.2737106602535, 'Menu_112');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640529, 81, NULL, 0, 1, 9, 71.9671352907521, 'Menu_465');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640459, 78, NULL, 1, 1, 3, 91.1872297923612, 'Menu_306');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640443, 304, NULL, 0, 1, 8, 66.730897712607, 'Menu_422');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640481, 33, NULL, 1, 0, 4, 92.0013213345876, 'Menu_366');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640527, 44, NULL, 0, 1, 5, 70.4329019879439, 'Menu_158');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640500, 37, NULL, 1, 0, 4, 75.8001510892712, 'Menu_105');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640509, 56, NULL, 1, 1, 5, 12.4994898423552, 'Menu_58');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640466, 55, NULL, 1, 1, 4, 76.8664414807517, 'Menu_95');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640478, 232, NULL, 0, 1, 3, 3.62528721759177, 'Menu_276');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640498, 51, NULL, 0, 1, 4, 29.3074556450885, 'Menu_402');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(512009, 62, NULL, 0, 0, 5, 57.7482157676091, 'Menu_65');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(6640441, NULL, 'SYS', 1, 1, 1, 999, 'IN_LIST_STANDALONE');
INSERT INTO  T_BILLMETHOD (FMETHODNO, FPARENT, FSYS, FUSED, FDATARTAG, FLEVEL, FMENUORDER, FNAME) VALUES(512399, 1, 'SYS', 1, 1, 2, 500, 'IN_LIST_CHILD');

CREATE TABLE T_ROLE (
    FROLENO NUMBER NOT NULL,
    FSTATUS NUMBER(1) NULL,
    CONSTRAINT T_ROLE_PKEY PRIMARY KEY (froleno)
);

INSERT INTO  T_ROLE (FROLENO, FSTATUS) VALUES(10, 1);
INSERT INTO  T_ROLE (FROLENO, FSTATUS) VALUES(20, 0);
INSERT INTO  T_ROLE (FROLENO, FSTATUS) VALUES(30, 1);

CREATE TABLE  T_ROLEMETHOD (
    FROLENO NUMBER NULL,
    FMETHODNO NUMBER NULL
);

INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 1);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 2);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 3);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 14);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 22);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 36);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 37);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 41);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 6640441);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(10, 512399);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(30, 12);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(30, 16);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(30, 18);
INSERT INTO  T_ROLEMETHOD (FROLENO, FMETHODNO) VALUES(30, 25);


CREATE TABLE  T_USERROLE (
    FROLENO NUMBER NULL,
    FUSERNO NUMBER NULL
);

INSERT INTO  T_USERROLE (FROLENO, FUSERNO) VALUES(10, 181556);
INSERT INTO  T_USERROLE (FROLENO, FUSERNO) VALUES(30, 181556);
INSERT INTO  T_USERROLE (FROLENO, FUSERNO) VALUES(10, 999999);
INSERT INTO  T_USERROLE (FROLENO, FUSERNO) VALUES(20, 181556);

--5 rows
SELECT
  DISTINCT t.*
FROM
  t_billmethod t
WHERE
  (t.fsys IS NOT NULL AND fused = 1)
 OR fmethodno IN ( 512399, 121220, 513399, 121199, 512010, 512009, 512079, 120199, 6640421, 6640422, 664044, 6640441, 664045, 664046, 6640442, 6640442, 6640443, 6640444, 6640445, 6640446, 6640447, 6640448, 6640449, 6640450, 6640451, 6640452, 6640453, 6640454, 6640453, 6640453, 6640454, 6640455, 6640456, 6640457, 6640458, 6640459, 6640460, 6640461, 6640462, 6640463, 6640464, 6640465, 6640466, 6640463, 6640463, 6640463, 6640464, 6640465, 6640466, 6640467, 6640468, 6640469, 6640470, 6640471, 6640472, 6640473, 6640474, 6640475, 6640476, 6640477, 6640478, 6640479, 6640480, 6640481, 6640485, 6640486, 6640487, 6640488, 6640489, 6640490, 6640491, 6640492, 6640493, 6640494, 6640495, 6640496, 6640497, 6640498, 6640499, 6640500, 6640501, 6640502, 6640503, 6640504, 6640505, 6640506, 6640507, 6640508, 6640509, 6640510, 6640511, 6640512, 6640513, 6640514, 6640515, 6640516, 6640517, 6640518, 6640519, 6640520, 6640521, 6640522, 6640523, 6640524, 6640525, 6640526, 6640527, 6640528, 6640529, 6640530, 220009, 405099, 123199, 6640526)
START WITH
  t.fmethodno IN (
    SELECT
      rm.fmethodno
    FROM
      t_rolemethod rm
    WHERE
      rm.froleno IN (
        SELECT
          ur.froleno
        FROM
          t_userrole ur,
          t_role r
        WHERE
          ur.froleno = r.froleno
          AND r.fstatus = 1
          AND ur.fuserno = 181556))
  AND t.fdatartag = 1
  CONNECT BY
    PRIOR t.fparent = t.fmethodno
ORDER BY
  t.flevel,
  t.fmenuorder;

---test more tables
CREATE TABLE dept_hier_test (
    deptno NUMBER,
    dname  VARCHAR2(30)
);

CREATE TABLE emp_bonus_test (
    empno NUMBER,
    bonus NUMBER
);

INSERT INTO dept_hier_test VALUES (10, 'yanfa');
INSERT INTO dept_hier_test VALUES (20, 'cheshi');

INSERT INTO emp_bonus_test VALUES (1,  5000);
INSERT INTO emp_bonus_test VALUES (2,  3000);
INSERT INTO emp_bonus_test VALUES (3,  2000);
INSERT INTO emp_bonus_test VALUES (4,  1000);
INSERT INTO emp_bonus_test VALUES (5,   800);
INSERT INTO emp_bonus_test VALUES (6,   600);
INSERT INTO emp_bonus_test VALUES (7,   500);
INSERT INTO emp_bonus_test VALUES (8,   200);
INSERT INTO emp_bonus_test VALUES (9,   100);

--9 ROWS
SELECT
    LEVEL AS lvl,
    e.empno,
    e.ename,
    d.dname,
    e.sal,
    ROW_NUMBER() OVER (
        PARTITION BY LEVEL, d.deptno
        ORDER BY e.sal DESC
    ) AS rn
FROM emp_hier_test e
JOIN dept_hier_test d
  ON e.deptno = d.deptno
START WITH e.mgr IS NULL
CONNECT BY PRIOR e.empno = e.mgr
ORDER BY lvl, d.dname, rn;

--9 ROWS
SELECT
    LEVEL AS lvl,
    e.empno,
    e.ename,
    e.sal,
    NVL(b.bonus, 0) AS bonus,
    e.sal + NVL(b.bonus, 0) AS total_income,
    RANK() OVER (
        PARTITION BY LEVEL
        ORDER BY e.sal + NVL(b.bonus, 0) DESC
    ) AS income_rank
FROM emp_hier_test e
LEFT JOIN emp_bonus_test b
  ON e.empno = b.empno
START WITH e.mgr IS NULL
CONNECT BY PRIOR e.empno = e.mgr
ORDER BY lvl, income_rank;

--6 ROWS
SELECT DISTINCT
    LEVEL AS lvl,
    d.deptno,
    d.dname
FROM emp_hier_test e
JOIN dept_hier_test d
  ON e.deptno = d.deptno
START WITH e.mgr IS NULL
CONNECT BY PRIOR e.empno = e.mgr
ORDER BY lvl, d.deptno;

--6 ROWS
SELECT DISTINCT
    lvl,
    dname,
    cnt_by_level_dept
FROM (
    SELECT
        LEVEL AS lvl,
        d.dname,
        COUNT(*) OVER (
            PARTITION BY LEVEL, d.deptno
        ) AS cnt_by_level_dept
    FROM emp_hier_test e
    JOIN dept_hier_test d
      ON e.deptno = d.deptno
    START WITH e.mgr IS NULL
    CONNECT BY PRIOR e.empno = e.mgr
) t
ORDER BY lvl, dname;

--9 ROWS
SELECT
    h.lvl,
    h.empno,
    h.ename,
    d.dname,
    h.sal,
    ROW_NUMBER() OVER (
        PARTITION BY h.lvl, d.deptno
        ORDER BY h.sal DESC
    ) AS rn
FROM (
    SELECT
        LEVEL AS lvl,
        empno,
        ename,
        deptno,
        sal
    FROM emp_hier_test
    START WITH mgr IS NULL
    CONNECT BY PRIOR empno = mgr
) h
JOIN dept_hier_test d
  ON h.deptno = d.deptno
ORDER BY h.lvl, d.dname, rn;

--9 ROWS
SELECT
    CONNECT_BY_ROOT e.empno AS root_empno,
    CONNECT_BY_ROOT e.ename AS root_name,
    LEVEL AS lvl,
    e.empno,
    e.ename,
    d.dname,
    e.sal,
    ROW_NUMBER() OVER (
        PARTITION BY CONNECT_BY_ROOT e.empno, d.deptno
        ORDER BY e.sal DESC
    ) AS rn
FROM emp_hier_test e
JOIN dept_hier_test d
  ON e.deptno = d.deptno
START WITH e.mgr IS NULL
CONNECT BY PRIOR e.empno = e.mgr
ORDER BY root_empno, d.dname, rn;

--9 ROWS
SELECT DISTINCT
    LEVEL AS lvl,
    d.dname,
    SYS_CONNECT_BY_PATH(e.ename, '/') AS path
FROM emp_hier_test e
JOIN dept_hier_test d
  ON e.deptno = d.deptno
START WITH e.mgr IS NULL
CONNECT BY PRIOR e.empno = e.mgr
ORDER BY lvl, d.dname, path;

--9 ROWS
SELECT DISTINCT
    lvl,
    dname,
    rn
FROM (
    SELECT
        LEVEL AS lvl,
        e.empno,
        e.ename,
        d.dname,
        ROW_NUMBER() OVER (
            PARTITION BY LEVEL, d.deptno
            ORDER BY e.sal DESC
        ) AS rn
    FROM emp_hier_test e
    JOIN dept_hier_test d
      ON e.deptno = d.deptno
    START WITH e.mgr IS NULL
    CONNECT BY PRIOR e.empno = e.mgr
) t
ORDER BY lvl, dname, rn;

--6 ROWS
SELECT DISTINCT ON (lvl, dname)
       lvl,
       dname,
       empno,
       ename,
       sal
FROM (
    SELECT
        LEVEL AS lvl,
        e.empno,
        e.ename,
        d.dname,
        e.sal
    FROM emp_hier_test e
    JOIN dept_hier_test d
      ON e.deptno = d.deptno
    START WITH e.mgr IS NULL
    CONNECT BY PRIOR e.empno = e.mgr
) t
ORDER BY lvl, dname, sal DESC;

--6 rows
SELECT
    lvl,
    dname,
    COUNT(*) AS cnt
FROM (
    SELECT
        LEVEL AS lvl,
        d.dname,
        ROW_NUMBER() OVER (
            PARTITION BY LEVEL, d.deptno
            ORDER BY e.sal DESC
        ) AS rn
    FROM emp_hier_test e
    JOIN dept_hier_test d
      ON e.deptno = d.deptno
    START WITH e.mgr IS NULL
    CONNECT BY PRIOR e.empno = e.mgr
) t
GROUP BY lvl, dname
ORDER BY lvl, dname;

--clean data
drop table dept_hier_test;
drop table emp_bonus_test;
drop table emp_hier_test;
drop table t_billmethod;
drop table t_role;
drop table t_rolemethod;
drop table t_userrole;

--test connect by with cte
create table test_id(id integer,name varchar2(256),parent integer);

INSERT INTO test_id VALUES(1,'one', NULL);
INSERT INTO test_id VALUES(2,'two', 1);
INSERT INTO test_id VALUES(3,'three',2);
INSERT INTO test_id VALUES(4,'four', 3);

create table test_with(id integer,name varchar2(256),name1 varchar2(256));

INSERT INTO test_with values(100,'test_with_name','test_with_name1');

--ok 4 rows
with tb as(
select id,name,name1 from test_with)
select id,(select b.name from tb b) as t from test_id a
connect by prior id = parent start with parent is null;


--with 4 rows
with tb as(
select id,name,name1 from test_with)
select id,(select name from tb b) as t from test_id a
connect by prior id = parent start with parent is null;

create table test_with2(id integer,name varchar2(256));

INSERT INTO test_with2 VALUES(1000,'test_with2_name');

--with 4 rows
with tb as(
select id,name,name1 from test_with)
select id,(select (select name1 from test_with2) as t from tb b) as t from test_id a
connect by prior id = parent start with parent is null;

--with 4 rows
with tb as(
select id,name,name1,2 as parent  from test_with)
select id,(select (select parent from test_with2) as t from tb b) as t from test_id a
connect by prior id = parent start with parent is null;

--clean data
drop table test_with;
drop table test_id;
drop table test_with2;