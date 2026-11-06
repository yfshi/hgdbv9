-- 清理
DROP TABLE IF EXISTS test CASCADE;

-- 测试步骤（enable_tecc=1连接TECC数据库）
create table test(id int);
insert into test values(1);
select * from test;

-- 清理
DROP TABLE IF EXISTS test CASCADE;
