-- ============================================================
-- swxa_concurrent.sql — SWXA 并发场景：后台进程加解密 + 主进程 ALTER KEK
-- ============================================================
-- 验证连接级缓存隔离：后台进程（独立连接）缓存 DEK 明文后，主进程
-- ALTER KEK 轮换（重加密 DEK），后台进程继续用缓存的 DEK 明文加解密，
-- 结果正确，互不影响。
--
-- 注意：后台进程需显式 enable_tecc=true。数据获取器创建临时连接时会
-- putenv("PGENABLETECC=false")（且未恢复，见 tecc_data_fetcher.c），
-- 污染主进程环境变量，导致 \! fork 的子进程默认继承 enable_tecc=false。
--
-- 注意：本用例靠 sleep 拉开两个进程的时序，时长按**两驱动最慢的一侧**取。
-- JDBC 侧每段 SQL 由测试 runner 起一个独立 JVM 执行（固定开销实测约 2.5s，
-- 见 t/002_tecc_jdbc_test.pl 的分段执行），主进程"ALTER KEK + 3 条语句"因此
-- 比 libpq 侧（单连接连续执行，约 0.3s）晚约 2s 完成。若按 libpq 侧的节奏
-- 取时长，后台查询会赶在 ALTER KEK 之前发生 —— 用例看似通过，实则没验证到
-- "轮换后用缓存 DEK 解密"这个核心点。故两处等待都留足余量：
--   BGW_SLEEP=8  ：后台缓存 DEK 后等待，确保 ALTER KEK 及其后 INSERT 已完成
--   MAIN_SLEEP=12：主进程等待后台写出 conc_bg.out，确保 cat 时结果完整
-- ============================================================

-- 清理
DROP TABLE IF EXISTS conc_t CASCADE;
DROP DEK IF EXISTS conc_dek CASCADE;
DROP KEK IF EXISTS conc_kek CASCADE;
\! rm -f conc_bg.out

-- 准备
CREATE KEK conc_kek WITH (key_store='swxakms', key_path='hsm:id=1');
CREATE DEK conc_dek WITH VALUES(KEK = conc_kek);
CREATE TABLE conc_t (id SERIAL, c INT4_ENC ENCRYPTED WITH (DEK = conc_dek, SUPPORT_ALGO = 'DET'));
INSERT INTO conc_t (c) VALUES (100), (200);

-- 后台进程：INSERT（缓存 DEK 明文）→ 睡眠等待 ALTER KEK → SELECT（用缓存解密）
\! (psql -X -A -t -U postgres -d "dbname=postgres enable_tecc=true" -c "INSERT INTO conc_t (c) VALUES (300)" -c "SELECT pg_sleep(8)" -c "SELECT 'bg_final', id, c FROM conc_t ORDER BY id" > conc_bg.out 2>&1) &

-- 主进程：等待后台缓存 DEK 明文后，执行 ALTER KEK 轮换
\! sleep 2
ALTER KEK conc_kek WITH (KEY_PATH = 'hsm:id=2');

-- 主进程：ALTER KEK 后继续加解密（用新 KEK 重新解密 DEK）
SELECT 'main_after', id, c FROM conc_t ORDER BY id;
INSERT INTO conc_t (c) VALUES (500);
SELECT 'main_final', id, c FROM conc_t ORDER BY id;

-- 等待后台进程结束（须大于 BGW_SLEEP，见文件头说明）
\! sleep 12

-- 检查后台进程结果
\echo '=== background worker ==='
\! cat conc_bg.out

-- 清理
DROP TABLE conc_t CASCADE;
DROP KEK conc_kek CASCADE;
