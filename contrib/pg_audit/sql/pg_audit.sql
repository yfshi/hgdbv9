alter system set audit.retentiondays TO -1;
alter system set audit.retentiondays TO 0;
alter system set audit.retentiondays TO 1;
alter system set audit.retentiondays TO 2147483647;
alter system set audit.retentiondays TO 2147483648;

alter system set audit.detectiontime TO -1;
alter system set audit.detectiontime TO 0;
alter system set audit.detectiontime TO 2147483647;
alter system set audit.detectiontime TO 2147483648;

alter system set audit.spacealarm = '';
alter system set audit.spacealarm = '-20%';
alter system set audit.spacealarm = '0%';
alter system set audit.spacealarm = '100%';
alter system set audit.spacealarm = '101%';
alter system set audit.spacealarm = '10%';
alter system set audit.spacealarm = '10a%';
alter system set audit.spacealarm = 'a10%';
alter system set audit.spacealarm = '10';
alter system set audit.spacealarm = '10.1%';
alter system set audit.spacealarm = '.10%';

alter system set audit.spacealarm = 0;
alter system set audit.spacealarm = -0.2;
alter system set audit.spacealarm = 1;
alter system set audit.spacealarm = 1.1;
alter system set audit.spacealarm = 0.1;
alter system set audit.spacealarm = .1;

select * from hg_secsch.pg_audit_stmt('delete');
DO $$
DECLARE
	confoid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'DELETE'
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_rule('rule1',confoid,'successful',true);
end $$;

DO $$
DECLARE
	confoid oid;
	rule_oid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'DELETE'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid
	FROM pg_secmon_rule
	WHERE confid = confoid
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_threshold('thr1',ARRAY[rule_oid]::oidvector,1,1,NULL,NULL,NULL,NULL,NULL,1,true);
end $$;

select * from hg_secsch.pg_audit_stmt('alter system');
DO $$
DECLARE
	confoid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'ALTER SYSTEM'
	LIMIT 1;
	PERFORM hg_secsch.pg_create_secmon_rule('rule2',confoid,'successful',true);
end $$;

DO $$
DECLARE
	confoid oid;
	rule_oid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'ALTER SYSTEM'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid
	FROM pg_secmon_rule
	WHERE confid = confoid
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_threshold('thr2',ARRAY[rule_oid]::oidvector,1,1,'audit.retentiondays','10',NULL,NULL,NULL,1,true);
end $$;

select * from hg_secsch.pg_audit_stmt('create table');
DO $$
DECLARE
	confoid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'CREATE TABLE'
	LIMIT 1;
	PERFORM hg_secsch.pg_create_secmon_rule('rule3',confoid,'successful',true);
end $$;

DO $$
DECLARE
	confoid oid;
	rule_oid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'CREATE TABLE'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid
	FROM pg_secmon_rule
	WHERE confid = confoid
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_threshold('thr3',ARRAY[rule_oid]::oidvector,5,3,NULL,NULL,NULL,NULL,NULL,1,true);
end $$;

create table test (id int);
select * from hg_secsch.pg_audit_object('select','column','test.id');
DO $$
DECLARE
	confoid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'SELECT'
	LIMIT 1;
	PERFORM hg_secsch.pg_create_secmon_rule('rule4',confoid,'successful',true);
end $$;

DO $$
DECLARE
	confoid oid;
	rule_oid oid;
BEGIN
	SELECT confid INTO confoid
	FROM pg_audit_config
	WHERE actionname = 'SELECT'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid
	FROM pg_secmon_rule
	WHERE confid = confoid
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_threshold('thr4',ARRAY[rule_oid]::oidvector,1,1,NULL,NULL,NULL,NULL,'1',1,true);
end $$;

select hg_secsch.pg_clean_secmon_config();
select hg_secsch.pg_clean_audit_config();

select * from hg_secsch.pg_audit_stmt('create table');
select * from hg_secsch.pg_audit_stmt('insert');
select * from hg_secsch.pg_audit_stmt('delete');

DO $$
DECLARE
	confoid_1 oid;
	confoid_2 oid;
	confoid_3 oid;
BEGIN
	SELECT confid INTO confoid_1
	FROM pg_audit_config
	WHERE actionname = 'CREATE TABLE'
	LIMIT 1;
	
	SELECT confid INTO confoid_2
	FROM pg_audit_config
	WHERE actionname = 'INSERT'
	LIMIT 1;
	
	SELECT confid INTO confoid_3
	FROM pg_audit_config
	WHERE actionname = 'DELETE'
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_rule('rule1',confoid_1,'successful',true);
	PERFORM hg_secsch.pg_create_secmon_rule('rule2',confoid_2,'successful',true);
	PERFORM hg_secsch.pg_create_secmon_rule('rule3',confoid_3,'successful',true);
end $$;

DO $$
DECLARE
	confoid_1 oid;
	confoid_2 oid;
	confoid_3 oid;
	rule_oid_1 oid;
	rule_oid_2 oid;
	rule_oid_3 oid;
BEGIN
	SELECT confid INTO confoid_1
	FROM pg_audit_config
	WHERE actionname = 'CREATE TABLE'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid_1
	FROM pg_secmon_rule
	WHERE confid = confoid_1
	LIMIT 1;
	
	SELECT confid INTO confoid_2
	FROM pg_audit_config
	WHERE actionname = 'INSERT'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid_2
	FROM pg_secmon_rule
	WHERE confid = confoid_2
	LIMIT 1;
	
	SELECT confid INTO confoid_3
	FROM pg_audit_config
	WHERE actionname = 'DELETE'
	LIMIT 1;
	
	SELECT ruleoid INTO rule_oid_3
	FROM pg_secmon_rule
	WHERE confid = confoid_3
	LIMIT 1;
	
	PERFORM hg_secsch.pg_create_secmon_threshold('thr1',ARRAY[rule_oid_1,rule_oid_2,rule_oid_3],20,1,NULL,NULL,NULL,NULL,NULL,1,true);
end $$;


select hg_secsch.pg_audit_privilege('create role','all');
create role us1;

select hg_secsch.pg_audit_privilege('create role','successful');
create role us2;
create role us2;

select hg_secsch.pg_audit_privilege('create role','fail');
create role us3;
create role us3;

select hg_secsch.pg_audit_privilege('create role, grant role','all');
create role role1;
GRANT CONNECT ON DATABASE postgres TO role1;

select hg_secsch.pg_noaudit_privilege('create role');
create role us4;

select hg_secsch.pg_audit_privilege('create role','all');
select hg_secsch.pg_noaudit_privilege('create role','successful');
create role us4;
create role us5;

select hg_secsch.pg_audit_privilege('create role','fail');
select hg_secsch.pg_noaudit_privilege('create role','successful');

select hg_secsch.pg_audit_privilege('create role','all');
select hg_secsch.pg_noaudit_privilege('create role','successful');
create role us6;

select hg_secsch.pg_noaudit_privilege('create role','fail');
create role us1;

select hg_secsch.pg_audit_privilege('create role','successful');
select hg_secsch.pg_noaudit_privilege('create role', 'fail');

select hg_secsch.pg_audit_privilege('create role','all');

select hg_secsch.pg_noaudit_privilege('create role','fail');
create role us1;
create role us7;

select hg_secsch.pg_clean_audit_config();

select hg_secsch.pg_audit_privilege('grant role','all');
create role test;
GRANT CONNECT ON DATABASE postgres TO test;

select hg_secsch.pg_audit_privilege('revoke role');
REVOKE CONNECT ON DATABASE postgres FROM test;

select hg_secsch.pg_noaudit_privilege('revoke role','successful');
select hg_secsch.pg_audit_privilege('drop role','successful');
drop role test1;

select hg_secsch.pg_audit_privilege('alter role','fail');
alter role fff WITH PASSWORD '22222222';
select hg_secsch.pg_audit_privilege('create user','all');
create user test1 password '1111';

select hg_secsch.pg_noaudit_privilege('create user','successful');
create user test1 password '1111';
select count(*) from hg_secsch.pg_audit_log;
select * from hg_secsch.pg_clean_auditlog();

select hg_secsch.pg_audit_privilege('drop role','successful');
drop role us1;
drop role us1;
select count(*) from hg_secsch.pg_audit_log;


select hg_secsch.pg_noaudit_privilege('drop role','all'),
select hg_secsch.pg_noaudit_privilege('drop role', 'fail');
drop role us1;
select count(*) from hg_secsch.pg_audit_log;


select hg_secsch.pg_audit_privilege('drop user','successful');
create user us1 password '1111';
drop user us1;
drop user us1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('create schema','fail');
create schema sch01;
create schema sch01;
select count(*) from hg_secsch.pg_audit_log;


select hg_secsch.pg_noaudit_privilege('create schema');
create schema sch01;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('alter schema','all');
ALTER SCHEMA sch01 RENAME TO sch01_new;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_privilege('alter schema');
ALTER SCHEMA sch01_new RENAME TO sch01;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('drop schema');
drop schema sch01;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_privilege('drop schema','all')
drop schema sch01;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('create database','successful');
create database db1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_privilege('create database', 'successful');
create database db2;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('alter database', 'fail');
ALTER DATABASE db1 OWNER TO us2;
ALTER DATABASE db1 OWNER TO us2;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_privilege('alter database', 'fail');
ALTER DATABASE db1 OWNER TO us2;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_privilege('drop database','all');
drop database db1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_privilege('drop database');
drop database db1;
drop database db2;
select count(*) from hg_secsch.pg_audit_log;


select hg_secsch.pg_audit_privilege('set','all');
select hg_secsch.pg_audit_privilege('','all');
select hg_secsch.pg_audit_privilege('all');

select hg_secsch.pg_audit_stmt('create domain','all');
create domain domain_test varchar(50) not null;
select hg_secsch.pg_noaudit_stmt('create domain','successful');
create domain domain_test1 varchar(50) not null;
create domain domain_test1 varchar(50) not null;

select hg_secsch.pg_audit_stmt('create procedure','all');
CREATE PROCEDURE my_procedure(param1 text, param2 text)
LANGUAGE plpgsql
AS
$$
BEGIN
	RAISE NOTICE 'This is a procedure call.';
END;
$$
;
CREATE PROCEDURE my_procedure(param1 text, param2 text)
LANGUAGE plpgsql
AS
$$
BEGIN
RAISE NOTICE 'This is a procedure call.';
END;
$$
;

select count(*) from hg_secsch.pg_audit_log;
select * from hg_secsch.pg_clean_auditlog();

select hg_secsch.pg_noaudit_stmt('create procedure','all');
CREATE PROCEDURE my_procedure(param1 text, param2 text)
LANGUAGE plpgsql
AS
$$
BEGIN
        RAISE NOTICE 'This is a procedure call.';
END;
$$
;
CREATE PROCEDURE my_procedure(param1 text, param2 text)
LANGUAGE plpgsql
AS
$$
BEGIN
RAISE NOTICE 'This is a procedure call.';
END;
$$
;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop procedure','successful');
drop procedure my_procedure;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop procedure', 'fail');
drop procedure my_procedure;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('create index');
create table tb1(id int, name text);
CREATE INDEX idx_id ON tb1 (id);
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create index');
CREATE INDEX idx_id ON tb1 (id);
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop index','fail');
drop index idx_id;
drop index idx_id;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop index');

select hg_secsch.pg_audit_stmt('create sequence','successful');
CREATE SEQUENCE my_sequence  
    START WITH 1  
    INCREMENT BY 1  
    NO MAXVALUE;
CREATE SEQUENCE my_sequence  
    START WITH 1  
    INCREMENT BY 1  
    NO MAXVALUE;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create sequence', 'successful');
CREATE SEQUENCE my_sequence  
    START WITH 1  
    INCREMENT BY 1  
    NO MAXVALUE;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('alter sequence');
ALTER SEQUENCE my_sequence NOCYCLE;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('alter sequence', 'fail');
ALTER SEQUENCE my_sequence NOCYCLE;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop sequence');
drop sequence my_sequence;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop sequence');
drop sequence my_sequence;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('create funciton','fail');
CREATE OR REPLACE FUNCTION my_function(param1 type1, param2 type2)  
RETURNS return_type AS 
$$
  
DECLARE  
    local_variable type;  
BEGIN  
    local_variable := param1 + param2;  
  
    RETURN local_variable;  
END;  

$$
 LANGUAGE plpgsql;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create funciton', 'fail');
CREATE OR REPLACE FUNCTION my_function(param1 type1, param2 type2)  
RETURNS return_type AS 
$$
  
DECLARE  
    local_variable type;  
BEGIN  
    local_variable := param1 + param2;  
  
    RETURN local_variable;  
END;  

$$
 LANGUAGE plpgsql;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop function');
drop function my_function;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop function', 'fail');
drop function my_function;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('create table','all');
create table tb_aa (a int primary key,b varchar);
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create table', 'successful');
create table tb_bb (a int primary key, b varchar);
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('create table as','successful');
create table tb_aa_1 as select * from tb_aa;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create table as', 'successful');
create table tb_aa_2 as select * from tb_aa;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop table','successful');
drop table tb_aa_1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop table', 'successful');
drop table tb_aa_2;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('create view');
create view V1 as select a,b from tb_aa;
create view V1 as select a,b from tb_aa;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('create view');
create view V1 as select a,b from tb_aa;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('alter view');
alter view V1 rename to v1_new;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('alter view', 'successful');
alter view v1_new rename to V1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_stmt('drop view');
drop view V1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_noaudit_stmt('drop view');
drop view V1;
select count(*) from hg_secsch.pg_audit_log;

select hg_secsch.pg_audit_all_user('create domain');
create domain domain_test1 varchar(50) not null;
create domain domain_test1 varchar(50) not null;
create user user1 login;
create user user2 login;
select hg_secsch.pg_audit_user('create domain','user1,user2','all');
create domain domain_test2 varchar(50) not null;
create domain domain_test2 varchar(50) not null;

create user user01 password 'Hello@1234';
create user user02 password 'Hello@1234';
select count(*) from hg_secsch.pg_audit_log;
select * from hg_secsch.pg_clean_auditlog();
create table student(id int primary key,name varchar,score int);
GRANT SELECT,insert,update ON TABLE student TO user02;
create table student2(id int primary key,name varchar,score int);
GRANT SELECT,insert,update ON TABLE student2 TO user01;
select hg_secsch.pg_audit_user('revoke','user01','fail');
revoke select on student from user03;
revoke select on student from user03;
select count(*) from hg_secsch.pg_audit_log;

select * from hg_secsch.pg_clean_audit_config();
select * from hg_secsch.pg_clean_auditlog();
create table tb_aa(a int primary key,b varchar);
create table tb_bb(a int primary key,b varchar);
create schema sch01;
create table sch01.tb_aa(a int primary key,b varchar);
select hg_secsch.pg_audit_object('insert','table','tb_aa','public','all');
insert into tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into tb_bb values(1,'aa'),(2,'bb'),(3,'cc');
insert into sch01.tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into tb_bb values(1,'aa'),(2,'bb'),(3,'cc');
insert into sch01.tb_aa values(1,'aa'),(2,'bb'),(3,'cc');

select hg_secsch.pg_audit_object('insert','table','tb_aa','sch01');
insert into tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into tb_bb values(1,'aa'),(2,'bb'),(3,'cc');
insert into sch01.tb_aa values(1,'aa'),(2,'bb'),(3,'cc');

select * from hg_secsch.pg_audit_object('insert,update','table','tb_aa','sch01','all');
insert into sch01.tb_aa values(2,'bb'),(3,'cc');
update sch01.tb_aa set b='update_1' where a=1;

create table sch01.tb_bb(a int primary key,b varchar);
insert into sch01.tb_bb values(1,'aa');
select * from hg_secsch.pg_audit_object('insert','table','tb_aa,tb_bb','sch01','all');
insert into sch01.tb_aa values(2,'bb'),(3,'cc');
update sch01.tb_aa set b='update_1' where a=1;
insert into sch01.tb_bb values(2,'bb'),(3,'cc');
update sch01.tb_bb set b='update_1' where a=1;

select hg_secsch.pg_noaudit_object('insert','table','tb_aa','public','all');
insert into tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into sch01.tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
select hg_secsch.pg_noaudit_object('insert','table','tb_aa','sch01');
insert into tb_aa values(1,'aa'),(2,'bb'),(3,'cc');
insert into sch01.tb_aa values(1,'aa'),(2,'bb'),(3,'cc');

select *  from hg_secsch.pg_audit_object('insert','table','tb_aa','public','successful');
select *  from hg_secsch.pg_noaudit_object('insert','table','tb_aa','public','successful');
select hg_secsch.pg_audit_object('insert','table','tb_aa','public','fail');
select hg_secsch.pg_noaudit_object('insert','table','tb_aa','public','successful');
select count(*) from hg_secsch.pg_audit_log;

select * from hg_secsch.pg_clean_auditlog();
select * from hg_secsch.pg_clean_audit_config();
drop table tb_aa;
drop table tb_bb;
drop table sch01.tb_aa;
create table tb_aa(id int primary key,b varchar,c int);
create table sch01.tb_aa(id int primary key,b varchar,c int);
select hg_secsch.pg_audit_object('INSERT','COLUMN','tb_aa.b','public','all');
insert into tb_aa(id,b) values(1,'aa');
insert into tb_aa(id,c) values(1,10);
insert into sch01.tb_aa(id,b) values(1,'aa');

create table tb_test(id int);
create table sch01.tb_test(id int);
create procedure pro_test(a integer, b integer)
language sql
as $$
insert into tb_test values (a);
insert into tb_test values (b);
$$;
create procedure sch01.pro_test(a integer, b integer)
language sql
as $$
insert into sch01.tb_test values (a);
insert into sch01.tb_test values (b);
$$;
select hg_secsch.pg_audit_object('call','procedure','pro_test','public','all');
call sch01.pro_test(1, 2);
call public.pro_test(1, 2);
select count(*) from hg_secsch.pg_audit_log;