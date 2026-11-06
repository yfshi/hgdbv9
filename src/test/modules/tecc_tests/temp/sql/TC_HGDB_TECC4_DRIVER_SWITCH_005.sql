-- 清理
DROP TABLE IF EXISTS tb1_005 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;

-- 前置条件
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES (KEK=kek_test);

-- 测试步骤
create table tb1_005(id int, name text_enc encrypted with (dek = dek_test));
insert into tb1_005 values(2,'李四');
update tb1_005 set name='王五' where id=2;
select name from tb1_005 where id=2;
delete from tb1_005 where id=2;

-- 清理
DROP TABLE IF EXISTS tb1_005 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
