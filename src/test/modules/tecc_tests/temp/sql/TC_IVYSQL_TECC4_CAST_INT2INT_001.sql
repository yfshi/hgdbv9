-- 清理
DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP TABLE IF EXISTS tb_int8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_int4 (id int4_enc encrypted with (dek = dek_t1));
create table tb_int8 (id int8_enc encrypted with (dek = dek_t1));

insert into tb_int4(id) select 1::int2_enc encrypted with (dek = dek_t1);
insert into tb_int8(id) select 1::int2_enc encrypted with (dek = dek_t1);
insert into tb_int8(id) select 1::int4_enc encrypted with (dek = dek_t1);

-- 清理
DROP TABLE IF EXISTS tb_int4 CASCADE;
DROP TABLE IF EXISTS tb_int8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
