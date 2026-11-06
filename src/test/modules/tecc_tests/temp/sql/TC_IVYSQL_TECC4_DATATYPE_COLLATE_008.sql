-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_test_tecc(id SERIAL PRIMARY KEY, col_text text_enc encrypted with (dek = dek_t1));

alter table tb_test_tecc alter column col_text set data type text_enc encrypted with (dek = dek_t2);

-- 清理
DROP TABLE IF EXISTS tb_test_tecc CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP DEK IF EXISTS dek_t2 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
