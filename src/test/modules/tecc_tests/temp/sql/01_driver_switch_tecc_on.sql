-- ============================================================
-- TECC 4.0.0.0 安装测试 - 功能开关（enable_tecc=1）
-- 运行方式: --modes simple --enable-tecc 1
-- ============================================================

-- 前置条件：创建测试密钥（补充完整WITH语法）
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES (KEK=kek_test);

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_001: JDBC默认开启密态功能（enable_tecc=1）
-- 预期: 建表成功，插入成功，查询返回明文'张三'
-- ============================================================
create table tb1_001(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_001 values(1,'张三');
select name from tb1_001 where id=1;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_004: 连接密态数据库，不关闭密态（默认enable_tecc=1）
-- 预期: 连接成功，普通增删改查正常执行
-- ============================================================
create table tb_004(id int);
insert into tb_004 values(1);
select * from tb_004;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_005: JDBC密态DML功能验证
-- 预期: INSERT/UPDATE/SELECT/DELETE全部成功，查询返回明文
-- ============================================================
create table tb1_005(id int, name text_enc encrypted with (dek = dek_test));
insert into tb1_005 values(2,'李四');
update tb1_005 set name='王五' where id=2;
select name from tb1_005 where id=2;
delete from tb1_005 where id=2;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_006: libpq接口（SQL同JDBC）
-- 预期: 建表成功，插入成功，查询返回明文'张三'
-- ============================================================
create table tb1_006(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_006 values(1,'张三');
select name from tb1_006 where id=1;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_009: psql无额外参数（SQL部分可执行）
-- 预期: 查询返回明文'张三'
-- ============================================================
create table tb1_009(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_009 values(1,'张三');
select name from tb1_009 where id=1;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_010: psql -C off（SQL部分可执行）
-- 预期: 查询返回明文'张三'
-- ============================================================
create table tb1_010(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_010 values(1,'张三');
select name from tb1_010 where id=1;

-- ============================================================
-- TC_HGDB_TECC4_DRIVER_SWITCH_011: psql密态开关切换（SQL部分可执行）
-- 预期: INSERT/SELECT操作正常
-- ============================================================
create table tb1_011(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1_011 values(1,'张三');
select name from tb1_011 where id=1;
