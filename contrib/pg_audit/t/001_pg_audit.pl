
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More tests => 100;

my $node = get_new_node('test');
$node->init;

#$node->append_conf('sec_pg.conf', 'audit.enable = on');
$node->start;

$node->safe_psql('postgres', "alter system set audit.enable = on;");

$node->stop;
$node->start;

# select table.
my $set_audit_privilege_create = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_privilege('create role');");
is($set_audit_privilege_create, 'Successfully configured privilege audit policy!', 'pg_audit_privilege');
$node->safe_psql('postgres', "create role test login;");
my $count_pg_audit_log_create_role = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE ROLE';");
is($count_pg_audit_log_create_role, '1', 'count_pg_audit_log');

my $set_noaudit_privilege_create = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_privilege('create role');");
is($set_noaudit_privilege_create, 'Successfully canceled privilege audit policy!', 'pg_noaudit_privilege');
$node->safe_psql('postgres', "create role test1;");
my $count_pg_audit_log_create_role1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE ROLE';");
is($count_pg_audit_log_create_role1, '1', 'count_pg_audit_log');

my $set_audit_privilege_grant = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_privilege('grant role');");
is($set_audit_privilege_grant, 'Successfully configured privilege audit policy!', 'pg_audit_privilege');
$node->safe_psql('postgres', "grant connect on database postgres to test;");
my $count_pg_audit_log_grant_role = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'GRANT';");
is($count_pg_audit_log_grant_role, '1', 'count_pg_audit_log');

my $set_noaudit_privilege_grant = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_privilege('grant role');");
is($set_noaudit_privilege_grant, 'Successfully canceled privilege audit policy!', 'pg_noaudit_privilege');
$node->safe_psql('postgres', "grant connect on database postgres to test;");
my $count_pg_audit_log_grant_role1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'GRANT';");
is($count_pg_audit_log_grant_role1, '1', 'count_pg_audit_log');


my $set_audit_privilege_revoke = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_privilege('revoke role');");
is($set_audit_privilege_revoke, 'Successfully configured privilege audit policy!', 'pg_audit_privilege');
$node->safe_psql('postgres', "revoke connect on database postgres from test;");
my $count_pg_audit_log_revoke = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'REVOKE';");
is($count_pg_audit_log_revoke, '1', 'count_pg_audit_log');

my $set_noaudit_privilege_revoke = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_privilege('revoke role');");
is($set_noaudit_privilege_revoke, 'Successfully canceled privilege audit policy!', 'pg_noaudit_privilege');
$node->safe_psql('postgres', "revoke connect on database postgres from test;");
my $count_pg_audit_log_revoke1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'REVOKE';");
is($count_pg_audit_log_revoke1, '1', 'count_pg_audit_log');


my $set_audit_privilege_drop = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_privilege('drop role');");
is($set_audit_privilege_drop, 'Successfully configured privilege audit policy!', 'pg_audit_privilege');
$node->safe_psql('postgres', "drop role test1;");
my $count_pg_audit_log_drop = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP ROLE';");
is($count_pg_audit_log_drop, '1', 'count_pg_audit_log');

my $set_noaudit_privilege_drop = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_privilege('drop role');");
is($set_noaudit_privilege_drop, 'Successfully canceled privilege audit policy!', 'pg_noaudit_privilege');
$node->safe_psql('postgres', "create role test1;");
$node->safe_psql('postgres', "drop role test1;");
my $count_pg_audit_log_drop1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP ROLE';");
is($count_pg_audit_log_drop1, '1', 'count_pg_audit_log');


my $set_audit_privilege_alter = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_privilege('alter role');");
is($set_audit_privilege_alter, 'Successfully configured privilege audit policy!', 'pg_audit_privilege');
$node->safe_psql('postgres', "alter role test with password '123';");
my $count_pg_audit_log_alter = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER ROLE';");
is($count_pg_audit_log_alter, '1', 'count_pg_audit_log');

my $set_noaudit_privilege_alter = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_privilege('alter role');");
is($set_noaudit_privilege_alter, 'Successfully canceled privilege audit policy!', 'pg_noaudit_privilege');
$node->safe_psql('postgres', "alter role test with password '123';");
my $count_pg_audit_log_alter1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER ROLE';");
is($count_pg_audit_log_alter1, '1', 'count_pg_audit_log');


my $set_audit_stmt_create_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('create table');");
is($set_audit_stmt_create_table, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "create table test(id int);");
my $count_pg_audit_log_create_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE TABLE';");
is($count_pg_audit_log_create_table, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_create_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('create table');");
is($set_noaudit_stmt_create_table, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "create table test1(id int);");
my $count_pg_audit_log_create_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE TABLE';");
is($count_pg_audit_log_create_table1, '1', 'count_pg_audit_log');

my $set_audit_stmt_create_index = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('create index');");
is($set_audit_stmt_create_index, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "create index idx_id on test(id);");
my $count_pg_audit_log_create_index = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE INDEX';");
is($count_pg_audit_log_create_index, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_create_index = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('create index');");
is($set_noaudit_stmt_create_index, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "create index idx_id1 on test1(id);");
my $count_pg_audit_log_create_index1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE INDEX';");
is($count_pg_audit_log_create_index1, '1', 'count_pg_audit_log');

my $set_audit_stmt_drop_index = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('drop index');");
is($set_audit_stmt_drop_index, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "drop index idx_id;");
my $count_pg_audit_log_drop_index = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP INDEX';");
is($count_pg_audit_log_drop_index, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_drop_index = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('drop index');");
is($set_noaudit_stmt_drop_index, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "drop index idx_id1;");
my $count_pg_audit_log_drop_index1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP INDEX';");
is($count_pg_audit_log_drop_index1, '1', 'count_pg_audit_log');

my $set_audit_stmt_drop_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('drop table');");
is($set_audit_stmt_drop_table, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "drop table test;");
my $count_pg_audit_log_drop_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP TABLE';");
is($count_pg_audit_log_drop_table, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_drop_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('drop table');");
is($set_noaudit_stmt_drop_table, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "drop table test1;");
my $count_pg_audit_log_drop_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP TABLE';");
is($count_pg_audit_log_drop_table1, '1', 'count_pg_audit_log');


my $set_audit_stmt_create_table_as = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('create table as');");
is($set_audit_stmt_create_table_as, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "create table test(id int);");
$node->safe_psql('postgres', "create table tt as select * from test;");
my $count_pg_audit_log_create_table_as = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE TABLE AS';");
is($count_pg_audit_log_create_table_as, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_create_table_as = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('create table as');");
is($set_noaudit_stmt_create_table_as, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "create table tt1 as select * from test;");
my $count_pg_audit_log_create_table_as1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE TABLE AS';");
is($count_pg_audit_log_create_table_as1, '1', 'count_pg_audit_log');

my $set_audit_stmt_create_view = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('create view');");
is($set_audit_stmt_create_view, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "create view v1 as select * from test;");
my $count_pg_audit_log_create_view = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE VIEW';");
is($count_pg_audit_log_create_view, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_create_view = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('create view');");
is($set_noaudit_stmt_create_view, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "create view v2 as select * from test;");
my $count_pg_audit_log_create_view1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE VIEW';");
is($count_pg_audit_log_create_view1, '1', 'count_pg_audit_log');

my $set_audit_stmt_drop_view = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('drop view');");
is($set_audit_stmt_drop_view, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "drop view v1;");
my $count_pg_audit_log_drop_view = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP VIEW';");
is($count_pg_audit_log_drop_view, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_drop_view = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('drop view');");
is($set_noaudit_stmt_drop_view, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "drop view v2;");
my $count_pg_audit_log_drop_view1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP VIEW';");
is($count_pg_audit_log_drop_view1, '1', 'count_pg_audit_log');

my $set_audit_stmt_create_domain = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('create domain');");
is($set_audit_stmt_create_domain, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "create domain domain_test varchar(50) not null;");
my $count_pg_audit_log_create_domain = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE DOMAIN';");
is($count_pg_audit_log_create_domain, '1', 'count_pg_audit_log');


my $set_noaudit_stmt_create_domain = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('create domain');");
is($set_noaudit_stmt_create_domain, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "create domain domain_test1 varchar(50) not null;");
my $count_pg_audit_log_create_domain1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'CREATE DOMAIN';");
is($count_pg_audit_log_create_domain1, '1', 'count_pg_audit_log');

my $set_audit_stmt_drop_domain = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_stmt('drop domain');");
is($set_audit_stmt_drop_domain, 'Successfully configured statement audit policy!', 'pg_audit_stmt');
$node->safe_psql('postgres', "drop domain domain_test");
my $count_pg_audit_log_drop_domain = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP DOMAIN';");
is($count_pg_audit_log_drop_domain, '1', 'count_pg_audit_log');

my $set_noaudit_stmt_drop_domain = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_stmt('drop domain');");
is($set_noaudit_stmt_drop_domain, 'Successfully canceled statement audit policy!', 'pg_noaudit_stmt');
$node->safe_psql('postgres', "drop domain domain_test1;");
my $count_pg_audit_log_drop_domain1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP DOMAIN';");
is($count_pg_audit_log_drop_domain1, '1', 'count_pg_audit_log');

my $set_audit_object_insert_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('insert','table','test');");
is($set_audit_object_insert_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "insert into test values(1);");
my $count_pg_audit_insert_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'INSERT';");
is($count_pg_audit_insert_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_insert_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('insert','table','test');");
is($set_noaudit_object_insert_table, 'Successfully canceled object audit policy!', 'pg_noaudit_object');
$node->safe_psql('postgres', "insert into test values(1);");
my $count_pg_audit_insert_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'INSERT';");
is($count_pg_audit_insert_table1, '1', 'count_pg_audit_log');

my $set_audit_object_update_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('update','table','test');");
is($set_audit_object_update_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "update test set id = 2 where id = 1;");
my $count_pg_audit_update_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'UPDATE';");
is($count_pg_audit_update_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_update_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('update','table','test');");
is($set_noaudit_object_update_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "update test set id = 1 where id = 2;");
my $count_pg_audit_update_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'UPDATE';");
is($count_pg_audit_update_table1, '1', 'count_pg_audit_log');

my $set_audit_object_delete_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('delete','table','test');");
is($set_audit_object_delete_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "delete from test;");
my $count_pg_audit_delete_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DELETE';");
is($count_pg_audit_delete_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_delete_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('delete','table','test');");
is($set_noaudit_object_delete_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "delete from test;");
my $count_pg_audit_delete_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DELETE';");
is($count_pg_audit_delete_table1, '1', 'count_pg_audit_log');

my $set_audit_object_select_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('select','table','test');");
is($set_audit_object_select_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "select * from test;");
my $count_pg_audit_select_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'SELECT' and audittype = 'object';");
is($count_pg_audit_select_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_select_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('select','table','test');");
is($set_noaudit_object_select_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "select * from test;");
my $count_pg_audit_select_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'SELECT' and audittype = 'object';");
is($count_pg_audit_select_table1, '1', 'count_pg_audit_log');

my $set_audit_object_lock_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('lock','table','test');");
is($set_audit_object_lock_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "begin; lock table test in access exclusive mode; end;");
my $count_pg_audit_lock_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'LOCK' and audittype = 'object';");
is($count_pg_audit_lock_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_lock_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('lock','table','test');");
is($set_noaudit_object_lock_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "begin; lock table test in access exclusive mode; end;");
my $count_pg_audit_lock_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'LOCK' and audittype = 'object';");
is($count_pg_audit_lock_table1, '1', 'count_pg_audit_log');

my $set_audit_object_alter_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('alter','table','test');");
is($set_audit_object_alter_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "alter table test add column name text;");
my $count_pg_audit_alter_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER TABLE' and audittype = 'object';");
is($count_pg_audit_alter_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_alter_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('alter','table','test');");
is($set_noaudit_object_alter_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "alter table test add column address text;");
my $count_pg_audit_alter_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER TABLE' and audittype = 'object';");
is($count_pg_audit_alter_table1, '1', 'count_pg_audit_log');

my $set_audit_object_comment_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('comment','table','test');");
is($set_audit_object_comment_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "comment on table test is 'highgo';");
my $count_pg_audit_comment_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'COMMENT' and audittype = 'object';");
is($count_pg_audit_comment_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_comment_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('comment','table','test');");
is($set_noaudit_object_comment_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "comment on table test is 'highgo';");
my $count_pg_audit_comment_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'COMMENT' and audittype = 'object';");
is($count_pg_audit_comment_table1, '1', 'count_pg_audit_log');

my $set_audit_object_truncate_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('truncate','table','test');");
is($set_audit_object_truncate_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "truncate table test;");
my $count_pg_audit_truncate_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'TRUNCATE' and audittype = 'object';");
is($count_pg_audit_truncate_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_truncate_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('truncate','table','test');");
is($set_noaudit_object_comment_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "truncate table test;");
my $count_pg_audit_truncate_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'TRUNCATE' and audittype = 'object';");
is($count_pg_audit_truncate_table1, '1', 'count_pg_audit_log');

my $set_audit_object_alterindex_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('alter index','table','test');");
is($set_audit_object_alterindex_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "create index idx1 on test(id);");
$node->safe_psql('postgres', "alter index idx1 rename to idx11;");
my $count_pg_audit_alterindex_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER INDEX' and audittype = 'object';");
is($count_pg_audit_alterindex_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_alterindex_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('alter index','table','test');");
is($set_noaudit_object_alterindex_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "alter index idx11 rename to idx1;");
my $count_pg_audit_alterindex_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'ALTER INDEX' and audittype = 'object';");
is($count_pg_audit_alterindex_table1, '1', 'count_pg_audit_log');

my $set_audit_object_dropindex_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_audit_object('drop index','table','test');");
is($set_audit_object_dropindex_table, 'Successfully configured object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "drop index idx1;");
my $count_pg_audit_dropindex_table = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP INDEX' and audittype = 'object';");
is($count_pg_audit_dropindex_table, '1', 'count_pg_audit_log');

my $set_noaudit_object_dropindex_table = $node->safe_psql('postgres', "select * from hg_secsch.pg_noaudit_object('drop index','table','test');");
is($set_noaudit_object_dropindex_table, 'Successfully canceled object audit policy!', 'pg_audit_object');
$node->safe_psql('postgres', "create index idx1 on test(id);");
$node->safe_psql('postgres', "drop index idx1;");
my $count_pg_audit_dropindex_table1 = $node->safe_psql('postgres', "select count(*) from hg_secsch.pg_audit_log where actionname = 'DROP INDEX' and audittype = 'object';");
is($count_pg_audit_dropindex_table1, '1', 'count_pg_audit_log');

my $clean_auditlog = $node->safe_psql('postgres', "select * from hg_secsch.pg_clean_auditlog();");
is($clean_auditlog, 't', 'pg_clean_auditlog');
my $delete_auditlog7 = $node->safe_psql('postgres', "select * from hg_secsch.pg_delete_auditlog('2025-06-01 12:00:00');");
is($delete_auditlog7, 'f', 'pg_delete_auditlog');

my $clean_audit_config = $node->safe_psql('postgres', "select * from hg_secsch.pg_clean_audit_config();");
is($clean_audit_config, 't', 'pg_clean_audit_config');
my $delete_audit_config = $node->safe_psql('postgres', "select * from hg_secsch.pg_delete_audit_config(1);");
is($delete_audit_config, 'f', 'pg_delete_audit_config');

$node->stop;

