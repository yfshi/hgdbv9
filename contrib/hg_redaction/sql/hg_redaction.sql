/** prerequisites for preparing to CREATE a policy **/
create extension hg_redaction ;

show search_path ;
alter system set search_path to "$user", public, hg_security;
select pg_reload_conf();

--first step: 
--			 set hg_redaction.enable to on.
set hg_redaction.enable to on;

--seccond step:
--				create a superuser.
create user redactionuser with CREATEROLE CREATEDB REPLICATION BYPASSRLS ;


--third step:
--			 create table and prepare data.
\c - redactionuser
create table test_redaction (id int, name varchar(22), salary money, hiredate timestamp, telephone text);

\c - syssso
set hg_redaction.enable to on;
--the fourth step: 
--				   1) create redaction policy.
create redaction policy p1 on test_redaction for(user = 'markbo') add column salary using redact_cash, add column telephone using redact_phone;

--				   2) query two tables hg_redaction_policy and hg_redaction_column,
--					  column polscope in hg_redaction_column defaults value is query:1,
--					  column polexception is equal:16.

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--the fifth step:
--				 create a policy with the same name on a same table,
--				 expecting an error, and it is not allowed to have a
--				 strategy with the same name on the same table.
\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user = 'markbo') add column name using redact_name, add column hiredate using redact_all;

--step six: 
--			create a policy with a different name on the same table
create redaction policy p2 on test_redaction for(user = 'markbo') add column name using redact_name, add column hiredate using redact_all;
--			query table hg_redaction_policy, hg_redaction_column to see if it is successful.
\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--step seven: 
--			 drop redaction policy.
\c - syssso
set hg_redaction.enable to on;
drop redaction policy p1 on test_redaction;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
drop redaction policy p2 on test_redaction;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--eighth step: 
--			  create policy setting with options and for expressions.
\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user != 'markbo, hr', ip = '192.168.1.130') add column salary using redact_digits with options(SCOPE query, EXCEPTION none), add column telephone using redact_phone with options(SCOPE top_tlist, EXCEPTION equal) ;

--			query table hg_redaction_policy, hg_redaction_column, check the options value
--			in hg_redaction_policy, and check whether the scope and exception in table
--			hg_redaction_column are the set values.
\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--			create policy setting with options and for expressions.
\c - syssso
set hg_redaction.enable to on;
create redaction policy p2 on test_redaction for(user != 'markbo, hr', ip = '192.168.1.130') add column name using redact_name with options(SCOPE top_tlist_or_error, EXCEPTION none), add column hiredate using redact_all with options(SCOPE query, EXCEPTION equal);
--			query table hg_redaction_policy, hg_redaction_column, check the options value
--			in hg_redaction_policy, and check whether the scope and exception in table
--			hg_redaction_column are the set values.
\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;


--ninth step:
--			 drop redaction policy.
\c - syssso
set hg_redaction.enable to on;
drop redaction policy p1 on test_redaction;
drop redaction policy p2 on test_redaction;

--tenth step:
--			 create a policy with the same name on a different table.
--			 create a new table.
\c - redactionuser
set hg_redaction.enable to on;
create table test_redaction1 (id int, name varchar(22), salary int, hiredate timestamp, telephone text);


--			create a policy with the same name on a different table.
--			expecting successful.
\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user != 'markbo, hr', ip = '192.168.1.130') add column salary using redact_digits with options(SCOPE query, EXCEPTION none), add column telephone using redact_phone with options(SCOPE top_tlist, EXCEPTION equal) ;

create redaction policy p1 on test_redaction1 for(user != 'markbo, hr', ip = '192.168.1.130') add column salary using redact_digits with options(SCOPE query, EXCEPTION none), add column telephone using redact_phone with options(SCOPE top_tlist, EXCEPTION equal) ;

--			query table hg_redaction_policy and table hg_redaction_column.
\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;


--eleventh step:
--			 drop redaction policy.
\c - syssso
set hg_redaction.enable to on;
drop redaction policy p1 on test_redaction;
drop redaction policy p1 on test_redaction1;

---------------------------------------------------------------
---------------------------------------------------------------
/**     ALTER data redaction policy     **/

--first step:
--			 create redaction policy.
\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user = 'hr') add column salary using redact_cash, add column telephone using redact_phone;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--socond step:
--			  alter redaction policy name.
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction rename to newp1;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy newp1 on test_redaction rename to newp11;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy newp11 on test_redaction rename to p1;

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--third step:
--			 alter redaction policy for cause.
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction for (user != 'alice', ip = '192.168.88.98');

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction for (user = 'hr');

\c - redactionuser
set hg_redaction.enable to on;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;

--the fourth step:
--				  alter redaction policy enable and disable. 
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction disable;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction enable;
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;

--the fifth step:
--			alter redaction policy p1 add column;
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction add name using redact_name;

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--step six:
--			alter redaction policy p1 drop column;
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction drop column name;

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

--step seven:
--			alter redaction policy p1 alter column information;
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction modify column salary using redact_all with options(SCOPE top_tlist, EXCEPTION none);

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction modify column salary using redact_cash with options(SCOPE top_tlist, EXCEPTION none);

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction modify column salary with options(SCOPE top_tlist_or_error, EXCEPTION none);

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction modify column salary with options (SCOPE top_tlist_or_error, EXCEPTION none), telephone using redact_all with options (SCOPE top_tlist_or_error, EXCEPTION none);

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction modify column salary using redact_cash with options(SCOPE top_tlist_or_error, EXCEPTION none);

\c - redactionuser
set hg_redaction.enable to on;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;


--eighth step:
--			alter redaction policy p1 drop column;
\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction drop column telephone;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy p1 on test_redaction drop column salary;

\c - redactionuser
set hg_redaction.enable to on;
--			expecting no column on test_redaction.
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;
--			expecting: the policy p1 is existed on test_redaction.
select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;


--tenth step:
--			 drop policy
\c - syssso
set hg_redaction.enable to on;
drop redaction policy p1 on test_redaction;


---------------------------------------------------------------
---------------------------------------------------------------
/**     DROP data redaction policy     **/
--first step:
--			 create a policy, and drop the policy.
create redaction policy p1 on test_redaction for(user = 'hr') add column salary using redact_cash, add column telephone using redact_phone;

drop redaction policy p1 on test_redaction;

--first step:
--			 drop a policy that does not exist.
drop redaction policy p1 on test_redaction;
--			 drop a policy that does not exist(use if exists).
drop redaction policy if exists p1 on test_redaction;

\c - redactionuser
--			 drop table.
drop table test_redaction, test_redaction1;

---------------------------------------------------------------
---------------------------------------------------------------
/**     Query table     **/

--prepare

create user hr;
create user alice;

-- one part
--first step:
--			 create table and insert data.
create table test_redaction (id int GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY, name varchar(22), salary money, hiredate timestamp, telephone text);
insert into test_redaction values (1, 'aaa', 10000, '2023-06-21 14:00:00', '18917810222');
insert into test_redaction values (2, 'bbb', 20000, '2023-06-21 14:00:00', '18817810222');
insert into test_redaction values (3, 'ccc', 30000, '2023-06-21 14:00:00', '18717810222');
insert into test_redaction values (4, 'ddd', 40000, '2023-06-21 14:00:00', '18617810222');

--seccond step:
\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user = 'hr') add column salary using redact_cash, add column telephone using redact_phone;

select polname, tabname, enable, poloptions from hg_security.hg_redaction_policy;
select polname, attname, procname, polscope, polexception from hg_security.hg_redaction_column;

-- third step:
--			  change user to hr，alice query table.
\c - redactionuser
grant SELECT on test_redaction to hr ;
grant SELECT on test_redaction to alice ;
\c - hr
set hg_redaction.enable to on;

select * from test_redaction;	-- expecting column salary and telephone have data redaction
select * from test_redaction where salary = 0::money;	--expecting no data
select * from test_redaction where salary = 30000::money;	--expecting have one data entry
select * from test_redaction where telephone = '186****0222';	--expecting no data
select * from test_redaction where telephone = '18617810222';	--expecting have one data entry

select telephone = '186****0222' from test_redaction;	--expecting all false
select telephone = '18617810222' from test_redaction;	--expecting: have a true, telephone is '18617810222'

\c - alice
set hg_redaction.enable to on;

select * from test_redaction;	-- expecting no data redaction

\c - redactionuser
set hg_redaction.enable to on;

\c - syssso
set hg_redaction.enable to on;
drop redaction policy p1 on test_redaction;

--fourth step:
create redaction policy p1 on test_redaction for(user != 'hr') add column salary using redact_cash, add column telephone using redact_phone;		  

-- step six:
--			change user to hr，alice query table.
\c - hr
set hg_redaction.enable to on;

select * from test_redaction;	-- expecting no data redaction

\c - alice
set hg_redaction.enable to on;

select * from test_redaction;	-- expecting column salary and telephone have data redaction
select * from test_redaction where salary = 0::money;	--expecting no data
select * from test_redaction where salary = 30000::money;	--expecting have one data entry
select * from test_redaction where telephone = '186****0222';	--expecting no data
select * from test_redaction where telephone = '18617810222';	--expecting have one data entry

select * from (select * from test_redaction)t; --expecting all data, have redaction

select * from test_redaction where salary in (select salary from test_redaction); --expecting no data
select * from test_redaction where telephone in (select telephone from test_redaction); --expecting no data


\c - syssso
set hg_redaction.enable to on;

drop redaction policy p1 on test_redaction;

\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user != 'hr') add column salary using redact_cash with options(SCOPE top_tlist, EXCEPTION none), add column telephone using redact_phone with options(SCOPE top_tlist_or_error, EXCEPTION none);

\c - alice
set hg_redaction.enable to on;

select * from test_redaction ;	-- expecting column salary, telephone have redaction
select * from test_redaction where salary = 0::money; -- expecting column salary, telephone have edaction
select * from test_redaction where salary = 30000::money;	--expecting have one data entry

select * from (select * from test_redaction) t; --expecting an error
select id, name, salary, hiredate, telephone from (select * from test_redaction) t; --expecting an error
select * from (select id, name, salary, hiredate from test_redaction)t; --expecting no data redaction
--expecting no data redaction
select id, name, salary, hiredate from (select id, name, salary, hiredate from test_redaction)t;

select * from test_redaction where salary in (select salary from test_redaction); --expecting all data redaction
select * from test_redaction where telephone in (select telephone from test_redaction); --expecting an error

\c - redactionuser
set hg_redaction.enable to on;

--drop redaction policy p1 on test_redaction;

-- second part

CREATE TABLE employees (
 id  integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
 name  varchar(40) NOT NULL,
 ssn  varchar(11) NOT NULL,
 phone  varchar(10),
 birthday  date,
 salary  int,
 email  varchar(100)
);

-- Insert some data
INSERT INTO employees (name, ssn, phone, birthday, salary, email)
VALUES
( 'Sally Sample', '020-78-9345', '5081234567', '1961-02-02', 51234.34,
'sally.sample@enterprisedb.com'),
( 'Jane Doe', '123-33-9345', '6171234567', '1963-02-14', 62500.00,
'jane.doe@gmail.com'),
( 'Bill Foo', '123-89-9345', '9781234567','1963-02-14', 45350,
'william.foe@hotmail.com');

\c - syssso
set hg_redaction.enable to on;
CREATE REDACTION POLICY redact_policy_personal_info ON employees FOR (session_user != 'hr')
		ADD COLUMN ssn USING redact_all WITH OPTIONS (SCOPE query, EXCEPTION none),
		ADD COLUMN salary USING redact_digits WITH OPTIONS (SCOPE query, EXCEPTION equal);

\c - redactionuser
grant select on employees to alice;

\c - alice
set hg_redaction.enable to on;

select * from employees; --all redaction
select * from employees where ssn = '123-33-9345'; --no data
select * from employees where ssn = 'xxxxxxxxxxx'; --all redaction
select * from employees where ssn in (select ssn from employees);	--all redaction
select ssn = 'xxxxxxxxxxx' from employees;  --all true
select ssn = '123-33-9345' from employees;	--all false
with ep as (select * from employees) select * from ep;	--all redaction
with ep as (select * from employees where ssn = '123-33-9345') select * from ep;	--no data
with ep as (select * from employees where ssn = 'xxxxxxxxxxx') select * from ep;	--all redaction
select * from employees where salary = 0;	--no data
select * from employees where salary = 45350;	--one entry
select * from employees where salary in (select salary from employees);		--no data
select salary = 45350 from employees;	-- have a true
select salary = 0 from employees;	--all false
with ep as (select * from employees where salary = 62500) select * from ep;	--one entry


\c - redactionuser
set hg_redaction.enable to on;

\c - syssso
set hg_redaction.enable to on;
DROP REDACTION POLICY redact_policy_personal_info ON employees;

CREATE REDACTION POLICY redact_policy_personal_info ON employees FOR (session_user != 'hr')
		ADD COLUMN ssn USING redact_all WITH OPTIONS (SCOPE top_tlist, EXCEPTION none),
		ADD COLUMN salary USING redact_digits WITH OPTIONS (SCOPE top_tlist, EXCEPTION equal);

\c - alice
set hg_redaction.enable to on;

select * from employees;	--all redaction
select * from employees where ssn = '123-33-9345';	--one entry
select * from employees where ssn = 'xxxxxxxxxxx';	--no data
select * from employees where ssn in (select ssn from employees);	--all redaction
select ssn = 'xxx-xx-9345' from employees;	--all true
select ssn = '123-33-9345' from employees;	--all false
select * from (select * from employees) t;	--no redaction
select * from employees where ssn in (select ssn from employees where ssn = '123-33-9345'); --one entry
with ep as (select * from employees) select * from ep;	--no redaction
with ep as (select * from employees where ssn = '123-33-9345') select * from ep;	--one entry
with ep as (select * from employees where ssn = 'xxxxxxxxxxx') select * from ep;	--no data
select * from employees where salary = 0;	--no data
select * from employees where salary = 45350; 	--one entry
select * from employees where salary in (select salary from employees);	--all redaction
select salary = 45350 from employees;	--one true
select salary = 0 from employees;	--all false
with ep as (select * from employees where salary = 45350) select * from ep; --one entry
with ep as (select * from employees where salary = 0) select * from ep; --no data

\c - syssso
set hg_redaction.enable to on;

DROP REDACTION POLICY redact_policy_personal_info ON employees;

CREATE REDACTION POLICY redact_policy_personal_info ON employees FOR (session_user != 'hr')
		ADD COLUMN ssn USING redact_all WITH OPTIONS (SCOPE top_tlist_or_error, EXCEPTION none),
		ADD COLUMN salary USING redact_digits WITH OPTIONS (SCOPE top_tlist_or_error, EXCEPTION equal);

\c - alice
set hg_redaction.enable to on;

select * from employees;	--all redaction
select * from employees where ssn = '123-33-9345';	--error
select * from employees where ssn = 'xxxxxxxxxxx';  --error
select * from employees where ssn in (select ssn from employees);	--error
select ssn = 'xxxxxxxxxxx' from employees;	--all true
select ssn = '123-33-9345' from employees;	--all false
select * from (select * from employees) t;	--error
with ep as (select * from employees) select * from ep;	--error
with ep as (select * from employees where ssn = '123-33-9345') select * from ep;  --error
with ep as (select * from employees where ssn = 'xxxxxxxxxxx') select * from ep;  --error
select * from employees where salary = 0; --no data
select * from employees where salary = 45350; --one data
select * from employees where salary in (select salary from employees);		--error
select salary = 45350 from employees;	--one true
select salary = 0 from employees;		--all false
with ep as (select * from employees where salary = 45350) select * from ep;		--error
with ep as (select * from employees where salary = 0::money) select * from ep;	--error


select * from employees cross join test_redaction;
select * from employees inner join test_redaction on employees.id = test_redaction.id;
select * from employees left outer join test_redaction on employees.id = test_redaction.id;
select * from employees right outer join test_redaction on employees.id = test_redaction.id;
select * from employees full outer join test_redaction on employees.id = test_redaction.id;


CREATE TABLE employees1 (
 id          integer GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY,
 name        varchar(40) NOT NULL,
 ssn         varchar(11) NOT NULL,
 phone       varchar(10),
 birthday    date,
 salary      int,
 email       varchar(100)
);

-- Insert some data
INSERT INTO employees1 (name, ssn, phone, birthday, salary, email)
VALUES
( 'Sally Sample', '020-78-9345', '5081234567', '1961-02-02', 51234.34,
'sally.sample@enterprisedb.com'),
( 'Jane Doe', '123-33-9345', '6171234567', '1963-02-14', 62500.00,
'jane.doe@gmail.com'),
( 'Bill Foo', '123-89-9345', '9781234567','1963-02-14', 45350,
'william.foe@hotmail.com');

select salary from employees union select salary from employees1; -- 3 entry

\c - syssso
set hg_redaction.enable to on;

CREATE REDACTION POLICY redact_policy_personal_info ON employees1 FOR (session_user != 'hr')
		ADD COLUMN ssn USING redact_all WITH OPTIONS (SCOPE query, EXCEPTION none),
		ADD COLUMN salary USING redact_digits WITH OPTIONS (SCOPE query, EXCEPTION equal);

DROP REDACTION POLICY redact_policy_personal_info ON employees;
CREATE REDACTION POLICY redact_policy_personal_info ON employees FOR (session_user != 'hr')
		ADD COLUMN ssn USING redact_all WITH OPTIONS (SCOPE query, EXCEPTION none),
		ADD COLUMN salary USING redact_digits WITH OPTIONS (SCOPE query, EXCEPTION equal);
DROP REDACTION POLICY p1 ON test_redaction;
create redaction policy p1 on test_redaction for(user != 'hr') add column salary using redact_cash, add column telephone using redact_phone;		  


\c - alice
set hg_redaction.enable to on;

select salary from employees union select salary from employees1; -- one entry

select * from employees inner join test_redaction on employees.id = test_redaction.id;

select * from employees inner join test_redaction on employees.id = test_redaction.id union select * from employees1 inner join test_redaction on employees1.id = test_redaction.id;

\c - syssso
set hg_redaction.enable to on;

DROP REDACTION POLICY redact_policy_personal_info ON employees1;
DROP REDACTION POLICY redact_policy_personal_info ON employees;
DROP REDACTION POLICY p1 ON test_redaction;

\c - redactionuser
DROP TABLE test_redaction;
DROP TABLE employees;
\c - alice
DROP TABLE employees1;


---------------------------------------------------------------
---------------------------------------------------------------
/**     Redaction function and type test   **/
\c - redactionuser
create table test_redaction (id int, name varchar(22), bckcard varchar(20), compemial varchar(20), selfemail varchar(20), backendcard text, selfexplain varchar(100), randmnum varchar(100), idcard varchar(20), salary money, hiredate timestamp, telephone text, comment varchar(200));

insert into test_redaction values (1, '张三丰', '4880-9898-4545-2525', 'zhangsan@highgo.com', 'zhangsan@163.com', '500-210-180-1888', 'zhegshiige shenme ne', '1001222123212', '130623202101010623', 30000, '2023-06-21 14:00:00', '18917810222', 'this is a redaction test');

insert into test_redaction values (2, '李四', '4880-9898-4545-2525', 'lisi@highgo.com', 'lisi@163.com', '500-210-180-1888', 'zhegshiige shenme ne', '1001222123212', '130623202101010623', 20000, '2023-06-21 14:00:00', '18917810222', 'this is a redaction test');

insert into test_redaction values (3, '王五五子', '4880-9898-4545-2525', 'wangwu@highgo.com', 'wangwu@163.com', '500-210-180-1888', 'zhegshiige shenme ne', '1001222123212', '130623202101010623', 30000, '2023-06-21 14:00:00', '18917810222', 'this is a redaction test');


\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user != 'hr') add column id using redact_digits, add column name using redact_name, add column bckcard using redact_bankcard, add column compemial using redact_emailname, add column selfemail using redact_emailfull, add column backendcard using redact_digits, add column selfexplain using redact_shuffle, add column randmnum using redact_random, add column idcard using redact_idcard, add column salary using redact_cash,  add column hiredate using redact_all, add column telephone using  redact_phone, add column comment using redact_all;

\c - redactionuser
grant select on test_redaction to alice;

\c - alice
set hg_redaction.enable to on;

select id, name, bckcard, compemial, selfemail, backendcard, idcard, hiredate, telephone, comment from test_redaction;

create table test_redaction1 (id int, name varchar(22), bckcard varchar(20), compemial varchar(20), selfemail varchar(20), backendcard text, idcard varchar(20), salary money, hiredate timestamp, telephone text, comment varchar(200));

insert into test_redaction1 select id, name, bckcard, compemial, selfemail, backendcard, idcard, salary, hiredate, telephone, comment from test_redaction;

select * from test_redaction1;

drop table test_redaction1;


\c - syssso
set hg_redaction.enable to on;

drop redaction policy p1 on test_redaction;

\c - redactionuser
drop table  test_redaction;
create table test_redaction (id int, name varchar(22), bckcard varchar(20), compemial varchar(20), selfemail varchar(20), backendcard text, selfexplain varchar(100), randmnum varchar(100), idcard varchar(20), salary money, hiredate timestamp, telephone text, comment varchar(200));

\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on test_redaction for(user != 'hr') add column xxx using redact_digits, add column name using redact_name, add column bckcard using redact_bankcard, add column compemial using redact_emailname, add column selfemail using redact_emailfull, add column backendcard using redact_digits, add column selfexplain using redact_shuffle, add column randmnum using redact_random, add column idcard using redact_idcard, add column salary using redact_cash,  add column hiredate using redact_all, add column telephone using  redact_phone, add column comment using redact_all; -- expecting error
create redaction policy p1 on test_redaction for(user != 'hr') add column id using redact_digits, add column name using redact_name, add column bckcard using redact_bankcard, add column compemial using redact_emailname, add column selfemail using redact_emailfull, add column backendcard using redact_digits, add column selfexplain using redact_shuffle, add column randmnum using redact_random, add column idcard using redact_idcard, add column salary using redact_cash,	add column hiredate using redact_all, add column telephone using  redact_phone, add column comment using redact_all;
drop redaction policy p1 on test_redaction;

\c - redactionuser
drop table  test_redaction;

create table rtest(id int, rflg bool, rtime time, rtimetz timetz, rinterval interval, rtimestamp timestamp, rtimestamptz timestamptz, rtext text, rbpchar char(4), rvarchar varchar, rname name, rint2 int2, rint4 int, rint8 int8, rnumeric numeric, rfloat4 float4, rfloat8 float8, rmoney money);
insert into rtest values (1, true, '20:20:19', '20:20:19 +08', '20:20:19', '2023-12-14 12:12:18', '2023-12-14 12:12:18 +08', 'this is a test', 'abcd', 'abcdabcd', 'zhangsanfeng', 2, 4, 100000, 10000, 12.12, 12.12222, 12);

\c - syssso
set hg_redaction.enable to on;
create redaction policy rp on rtest for (user=hr) add column id using redact_all, add column rflg using redact_all, add column rtime using redact_all, add column rtimetz using redact_all, add column rinterval using redact_all, add column rtimestamp using redact_all, add column rtimestamptz using redact_all, add column rtext using redact_all, add column rbpchar using redact_all, add column rvarchar using redact_all, add column rname using redact_all, add column rint2 using redact_digits, add column rint4 using redact_digits, add column rint8 using redact_digits, add column rnumeric using redact_digits, add column rfloat4 using redact_digits, add column rfloat8 using redact_digits, add column rmoney using redact_cash;

\c - redactionuser
grant select on rtest to hr;

\c - hr
set hg_redaction.enable to on;
select * from rtest;
create table t(id int, rflg bool, rtime time, rtimetz timetz, rinterval interval, rtimestamp timestamp, rtimestamptz timestamptz, rtext text, rbpchar char(4), rvarchar varchar, rname name, rint2 int2, rint4 int, rint8 int8, rnumeric numeric, rfloat4 float4, rfloat8 float8, rmoney money);
insert into t select * from rtest;
select * from t;
set hg_redaction.enable to off;
select * from t;
drop table t;

\c - syssso
set hg_redaction.enable to on;
drop redaction policy rp on rtest;

\c - redactionuser
drop table rtest;

create table tes(salary money);
insert into tes values (100), (200);

\c - syssso
set hg_redaction.enable to on;
create redaction policy rp on tes add column salary using redact_cash WITH OPTIONS (SCOPE query, EXCEPTION none);

\c - redactionuser
grant select on tes to hr;

\c - hr
create or replace function public.test(col bool) RETURNS bool AS $$
begin
	return $1;
end;
$$ LANGUAGE plpgsql;

set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy rp on tes modify column salary with options(SCOPE query, EXCEPTION equal);
\c - hr
set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy rp on tes modify column salary with options(SCOPE top_tlist, EXCEPTION none);
\c - hr
set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy rp on tes modify column salary with options(SCOPE top_tlist, EXCEPTION equal);
\c - hr
set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy rp on tes modify column salary with options(SCOPE top_tlist_or_error, EXCEPTION none);
\c - hr
set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;

\c - syssso
set hg_redaction.enable to on;
alter redaction policy rp on tes modify column salary with options(SCOPE top_tlist_or_error, EXCEPTION equal);
\c - hr
set hg_redaction.enable to on;
select test(salary = 0::money) from tes;
select test(salary = 100::money) from tes;
select test(salary = 200::money) from tes;
drop function public.test(bool);

\c - syssso
set hg_redaction.enable to on;
drop redaction policy rp on tes;
\c - redactionuser
drop table tes;
set hg_redaction.enable to off;

drop user hr;
drop user alice;

--create user u1 with password 'Highgo@123';
--create user u2 with password 'Highgo@123';
--create user u3 with password 'Highgo@123';
--
--\c - u1;
--create table tb1(id int);
--grant all on tb1 to u2, u3, redactionuser;

--\c - syssso
--select apply_row_policy('tb1'::regclass, 'apply');
--select apply_row_policy('tb1'::regclass, 'enable');
--
--set hg_sysmac.global_table_level to off;
--
--select set_user_seclabel('u1'::regrole, 's1');
--select set_user_seclabel('u2'::regrole, 's2');
--select set_user_seclabel('u3'::regrole, 's3');
--
--select set_table_seclabel('tb1'::regclass, 's3');
--
--
--\c - u1;
--insert into tb1 values (1);
--
--\c - u2;
--insert into tb1 values (2);
--
--\c - u3;
--insert into tb1 values (3);
--
--
--\c - u1
--select seclabel, * from tb1; 
--
--\c - u2
--select seclabel, * from tb1;
--
--\c - u3
--select seclabel, * from tb1;
--
--
--
--
--\c - syssso
--set hg_redaction.enable to on;
--create redaction policy p1 on tb1 add column seclabel using redact_all;
--
--\c - redactionuser
--set hg_redaction.enable to on;
--select seclabel, * from tb1;
--
--COPY tb1 TO STDOUT WITH CSV;
--
--\c - syssso
--set hg_redaction.enable to on;
--drop redaction policy p1 on tb1;
--
--\c - u1
--drop table tb1;


\c - redactionuser
create user u1 with password 'Highgo@123';

\c - u1
create table test(id int, name text);
insert into test values (1, 'zhangsan');
insert into test values (2, 'lisi');

\c - syssso
set hg_redaction.enable to on;

create redaction policy p1 on test add column id using hg_security.redact_digits;
alter redaction policy p1 on test add column name using hg_security.redact_all;

\c - u1
set hg_redaction.enable to on;
select * from test;
drop table test;

\c - redactionuser
drop user u1;

create table tb(id int, name text, addr text);
insert into tb values (1, 'zhangsan', 'beijing'),(2, 'lisi', 'shanghai'),(3, 'zhangwu', 'nanjiang'),(4, 'zhangsan', 'nanjing');

\c - syssso
set hg_redaction.enable to on;
create redaction policy p1 on tb add column id using  hg_security.redact_all, add column name using hg_security.redact_name;

\c - redactionuser
select name from tb group by name;
select * from tb order by id;

set hg_redaction.enable to on;
set se_feature.mode to on;

select name from tb group by name;
select * from tb order by id;

drop table tb;

\c - syssso
set hg_redaction.enable to off;

