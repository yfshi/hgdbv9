SELECT hg_security.sql_firewall_stat_reset();

-- Protected firewall relations and views are superuser-only.
CREATE ROLE sql_firewall_permission_test;
GRANT USAGE ON SCHEMA hg_security TO sql_firewall_permission_test;
SET ROLE sql_firewall_permission_test;
SELECT * FROM hg_security.sql_firewall_rec;
SELECT * FROM hg_security.sql_firewall_stat_rec;
SELECT * FROM hg_security.sql_high_risk_rec;
SELECT * FROM hg_security.sql_firewall_black_list;
SELECT * FROM hg_security.sql_firewall_stat;
SELECT * FROM hg_security.sql_firewall_statements;
SELECT * FROM hg_security.sql_firewall_white_list;
RESET ROLE;
REVOKE USAGE ON SCHEMA hg_security FROM sql_firewall_permission_test;
DROP ROLE sql_firewall_permission_test;
SELECT count(*) from hg_security.sql_firewall_statements;
SELECT hg_security.import_white_list_for_user('sqlfirewallsuperuser'::name, 'CREATE ROLE case_owner NOLOGIN;');

select count(*) from hg_security.sql_firewall_white_list(true);
select count(*) from hg_security.sql_firewall_white_list(false);
select count(*) from hg_security.sql_firewall_white_list;

SELECT hg_security.dispaly_single_sql(queryid) from hg_security.sql_firewall_statements;

ALTER SYSTEM SET sql_firewall.firewall to 'disabled';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);

ALTER SYSTEM SET sql_firewall.firewall to 'permissive';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT 1;

SELECT hg_security.sql_firewall_stat_error_count(); --0
SELECT hg_security.sql_firewall_stat_warning_count(); --1


ALTER SYSTEM SET sql_firewall.firewall to 'enforcing';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT 1;


SELECT hg_security.sql_firewall_stat_error_count(); --1
SELECT hg_security.sql_firewall_stat_warning_count(); --1

select  * from hg_security.sql_firewall_stat;

SELECT hg_security.sql_firewall_reset();
SELECT hg_security.sql_firewall_stat_reset();

select count(*) from hg_security.sql_firewall_black_list(true);
select count(*) from hg_security.sql_firewall_black_list(false);
SELECT hg_security.import_black_list_for_user('sqlfirewallsuperuser'::name, 'select ''aaa'';');
select count(*) from hg_security.sql_firewall_black_list(true);
select count(*) from hg_security.sql_firewall_black_list(false);

select count(*) from hg_security.sql_firewall_rec;
select count(*) from hg_security.sql_firewall_black_list;
select count(*) from hg_security.sql_firewall_statements;

SELECT hg_security.sql_firewall_reset();
SELECT hg_security.sql_firewall_stat_reset();

-- test high risk statement
select * from hg_security.sql_high_risk_rec;

select hg_security.import_high_risk_statement('unsafealter');
select hg_security.import_high_risk_statement('unsafedroptable');
select hg_security.import_high_risk_statement('unsafetruncate');
select hg_security.import_high_risk_statement('unsafedropdatabase');
select hg_security.import_high_risk_statement('unsafedropschema');
select * from hg_security.sql_high_risk_rec;

ALTER SYSTEM SET sql_firewall.firewall to 'permissive';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);

create table test(id int, name text);
alter table test add column address text;
update test set id = 1;
delete from test;
truncate test;
drop table test;

create schema aaa;
drop schema aaa;

create database sqlfiretestdb;
drop  database sqlfiretestdb;

select hg_security.remove_high_risk_statement('unsafealter');
select hg_security.remove_high_risk_statement('unsafedroptable');
select hg_security.remove_high_risk_statement('unsafetruncate');
select hg_security.remove_high_risk_statement('unsafedropdatabase');
select hg_security.remove_high_risk_statement('unsafedropschema');
select * from hg_security.sql_high_risk_rec;

-- Formatting-only whitespace and a trailing semicolon do not change queryid.
ALTER SYSTEM SET sql_firewall.firewall TO 'disabled';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);
CREATE TEMP TABLE sql_firewall_spacing_test(id integer);
SELECT hg_security.import_white_list(
    'SELECT * FROM sql_firewall_spacing_test WHERE id = 1');
SELECT hg_security.import_black_list(
    'SELECT * FROM sql_firewall_spacing_test WHERE id = 1');
SELECT hg_security.delete_white_list(
    'SELECT * FROM sql_firewall_spacing_test WHERE id = 1');
SELECT hg_security.delete_black_list(
    'SELECT * FROM sql_firewall_spacing_test WHERE id = 1');
ALTER SYSTEM SET sql_firewall.firewall TO 'permissive';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.import_white_list(
    'SELECT * FROM sql_firewall_spacing_test WHERE id = 1;');
SELECT *FROM sql_firewall_spacing_test WHERE id= 1;
SELECT * FROM sql_firewall_spacing_test WHERE id=1;
SELECT hg_security.import_white_list(
    'CREATE TABLE sql_firewall_utility_spacing(id int, name text);');
CREATE TABLE sql_firewall_utility_spacing(id int,          name text);
ALTER SYSTEM SET sql_firewall.firewall TO 'disabled';
SELECT hg_security.import_white_list('SELECT pg_reload_conf();');
SELECT hg_security.import_white_list('SELECT pg_sleep(1);');
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.sql_firewall_reset();
DROP TABLE sql_firewall_spacing_test;
DROP TABLE sql_firewall_utility_spacing;
-- end of other-functions
