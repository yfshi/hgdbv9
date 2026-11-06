-- 清理
DROP TABLE IF EXISTS tb_float4 CASCADE;
DROP TABLE IF EXISTS tb_float8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);
CREATE DEK dek_t2 WITH VALUES (KEK=kek_t1);
create table tb_float4 (id float4_enc encrypted with (dek = dek_t1));
create table tb_float8 (id float8_enc encrypted with (dek = dek_t1));
insert into tb_float4(id) select 1::int2_enc encrypted with (dek = dek_t2);
insert into tb_float8(id) select 1::int2_enc encrypted with (dek = dek_t2);
insert into tb_float8(id) select 1::int4_enc encrypted with (dek = dek_t2);
DROP TABLE IF EXISTS tb_float4 CASCADE;
DROP TABLE IF EXISTS tb_float8 CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
