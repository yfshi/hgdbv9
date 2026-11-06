-- 验证：enable_tecc=false下查询加密数据应返回16进制密文
select name from tb1 where id=1;

-- 清理
DROP TABLE IF EXISTS tb1 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
