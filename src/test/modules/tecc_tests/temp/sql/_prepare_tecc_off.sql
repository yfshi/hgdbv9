-- 为TC_HGDB_TECC4_DRIVER_SWITCH_002/007准备数据
-- 注意：KEK/DEK需已存在（由tecc_on测试创建）
create table tb1_002(id int, name text_enc encrypted with (dek = dek_test, support_algo='det'));
insert into tb1_002 values(1,'张三');
