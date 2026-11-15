-- ============================================================
-- swxa_he_parallel.sql — SWXA硬加密 同态字段级并行 INSERT 回归
-- ============================================================
-- 覆盖: 单语句内同态值达到 TECC_HE_PARALLEL_MIN_FIELDS（64）时触发的
--       字段级并行路径（values_processor 的 tecc_process_values_list_parallel），
--       以及同语句中对称列由主线程串行处理的混排情形。
--       两侧边界都取: 第 1 段整 64 个同态值（进并行），第 2 段 8 个（走串行）。
--
-- 为什么需要本用例: 同态字段级并行按设计只把同态字段分给 worker 线程，
--       对称列留在主线程串行（避免在 worker 线程里调用加密机 SDF）。
--       worker 内首次初始化 HSM 上下文若不串行，多个线程会各自 init 并
--       各开一个设备会话，而连接级缓存只有一份、只留最后一个写入者，
--       其余会话直到进程退出才释放。该缺陷在 SQL 层不可见（会话数差异需
--       用 gdb 断点 SDF_OpenSession 观测），本用例锁定的是并行路径的行为
--       正确性与覆盖: 此前测试集中没有任何用例单语句超过 7 个同态值，
--       该路径从未被执行过。
--
-- 数据: a 为 DET+HE-ADD 双算法列（同态值 + 对称值混在同一语句），
--       c 为纯 HE-ADD 列。
-- ============================================================

-- 清理
DROP TABLE IF EXISTS swxa_he_par_t CASCADE;
DROP DEK IF EXISTS swxa_he_par_add CASCADE;
DROP DEK IF EXISTS swxa_he_par_sym CASCADE;
DROP KEK IF EXISTS swxa_he_par_kek CASCADE;

-- 硬加密 KEK: 同态需要设备内 SM2 加密密钥对（预置索引 3）
CREATE KEK swxa_he_par_kek WITH (
    key_store = 'swxakms',
    key_algo  = 'sm2',
    key_path  = 'hsm:id=3'
);

CREATE DEK swxa_he_par_add WITH VALUES(KEK = swxa_he_par_kek, KEY_ALGO = 'he-add');
CREATE DEK swxa_he_par_sym WITH VALUES(KEK = swxa_he_par_kek, KEY_ALGO = 'sm4');

CREATE TABLE swxa_he_par_t (
    id SERIAL,
    a  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_par_sym, HE_ADD_DEK = swxa_he_par_add,
                                SUPPORT_ALGO = 'det, he-add'),
    c  INT4_ENC ENCRYPTED WITH (DEK = swxa_he_par_sym, HE_ADD_DEK = swxa_he_par_add,
                                SUPPORT_ALGO = 'he-add')
);

SELECT '=== SWXA-HE-PAR: 1. 整 64 个同态值（阈值边界，走字段级并行） ===' AS stage;
INSERT INTO swxa_he_par_t (a, c) VALUES
(1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),
(9,9),(10,10),(11,11),(12,12),(13,13),(14,14),(15,15),(16,16),
(17,17),(18,18),(19,19),(20,20),(21,21),(22,22),(23,23),(24,24),
(25,25),(26,26),(27,27),(28,28),(29,29),(30,30),(31,31),(32,32),
(33,33),(34,34),(35,35),(36,36),(37,37),(38,38),(39,39),(40,40),
(41,41),(42,42),(43,43),(44,44),(45,45),(46,46),(47,47),(48,48),
(49,49),(50,50),(51,51),(52,52),(53,53),(54,54),(55,55),(56,56),
(57,57),(58,58),(59,59),(60,60),(61,61),(62,62),(63,63),(64,64);
SELECT count(*) AS cnt, sum(a) AS sum_a, sum(c) AS sum_c FROM swxa_he_par_t;

SELECT '=== SWXA-HE-PAR: 2. 并行写入的明文回读（首/中/末行） ===' AS stage;
SELECT id, a, c FROM swxa_he_par_t WHERE id IN (1, 32, 64) ORDER BY id;

SELECT '=== SWXA-HE-PAR: 3. 并行写入后的同态运算 ===' AS stage;
UPDATE swxa_he_par_t SET c = a + c WHERE id <= 8;
SELECT id, a, c FROM swxa_he_par_t WHERE id IN (1, 4, 8) ORDER BY id;

SELECT '=== SWXA-HE-PAR: 4. 阈值之下 8 个同态值（串行路径，对照） ===' AS stage;
INSERT INTO swxa_he_par_t (a, c) VALUES
(65,65),(66,66),(67,67),(68,68),(69,69),(70,70),(71,71),(72,72);
SELECT count(*) AS cnt, sum(a) AS sum_a FROM swxa_he_par_t;

SELECT '=== SWXA-HE-PAR: 5. 同态条件表达式（并行写入数据） ===' AS stage;
SELECT id, a + c AS a_plus_c FROM swxa_he_par_t WHERE id IN (2, 16) ORDER BY id;
