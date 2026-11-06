-- 清理全局残留
DROP TABLE IF EXISTS tb1_001, tb_004, tb1_005, tb1_006, tb1_009, tb1_010, tb1_011 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;

-- 前置条件（DML测试共用KEK/DEK）
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES (KEK=kek_test);

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_001: JDBC默认开启密态
-- ============================================================
create table tb1_001(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_001 values(1,'张三');
select name from tb1_001 where id=1;
DROP TABLE IF EXISTS tb1_001 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_004: 连接密态数据库不关闭密态
-- ============================================================
create table tb_004(id int);
insert into tb_004 values(1);
select * from tb_004;
DROP TABLE IF EXISTS tb_004 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_005: JDBC密态DML功能验证
-- ============================================================
create table tb1_005(id int, name text_enc encrypted with (dek = dek_test));
insert into tb1_005 values(2,'李四');
update tb1_005 set name='王五' where id=2;
select name from tb1_005 where id=2;
delete from tb1_005 where id=2;
DROP TABLE IF EXISTS tb1_005 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_006: libpq接口（SQL同JDBC）
-- ============================================================
create table tb1_006(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_006 values(1,'张三');
select name from tb1_006 where id=1;
DROP TABLE IF EXISTS tb1_006 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_009: psql无额外参数（SQL部分）
-- ============================================================
create table tb1_009(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_009 values(1,'张三');
select name from tb1_009 where id=1;
DROP TABLE IF EXISTS tb1_009 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_010: psql -C off（SQL部分）
-- ============================================================
create table tb1_010(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_010 values(1,'张三');
select name from tb1_010 where id=1;
DROP TABLE IF EXISTS tb1_010 CASCADE;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_011: psql密态开关切换（SQL部分）
-- ============================================================
create table tb1_011(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_011 values(1,'张三');
select name from tb1_011 where id=1;
DROP TABLE IF EXISTS tb1_011 CASCADE;

-- 清理全局KEK/DEK
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
