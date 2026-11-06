-- 使用 enable_tecc=0 连接，查询加密数据应为密文
select * from example_table;

-- 清理
DROP TABLE IF EXISTS example_table CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
