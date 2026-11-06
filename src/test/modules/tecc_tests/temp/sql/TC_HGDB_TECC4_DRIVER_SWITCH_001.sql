-- 清理残留
DROP TABLE IF EXISTS tb1 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;

-- 前置条件
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES (KEK=kek_test);

-- 测试步骤
create table tb1(id int, name text_enc encrypted with (dek = dek_test, support_algo='det,ope,search'));
insert into tb1 values(1,'张三');
select name from tb1 where id=1;

-- 清理
DROP TABLE IF EXISTS tb1 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
