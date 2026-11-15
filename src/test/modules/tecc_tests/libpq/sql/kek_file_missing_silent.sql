-- ============================================================
-- kek_file_missing_silent.sql — KEK 密钥文件被删后重连，SELECT 应静默密文
-- 背景（Bug 记录 L650）：tecc_ktool delete 掉 KEK 的 .key 后【重连】（连接级
-- KEK 明文缓存被清空 → 重新读文件失败），SELECT 解密路径本应被调度器归类为
-- skipped_decrypt 静默返回密文；但 high_level_api 的
-- "[KEK ERROR] local_kms_load_key FAILED" 与 adapter 两个 get_kek==NULL 分支的
-- "Failed to get KEK with state"（无负缓存）会先按 列×行 刷屏。
-- 修复：文件加载失败分支 ERROR→DEBUG；get_kek==NULL 分支加负缓存短路。
-- 断言：删 .key → 重连后 SELECT 返回密文且【零 stderr 诊断行】；同连接再
--       SELECT 仍静默（负缓存短路，不重复尝试）；INSERT（写路径）硬报错。
--       （stderr 由回归框架并入 .out —— 任何重新引入的诊断 ERROR 都会使输出
--        与期望不符，即回归守卫。）
-- 注（Bug 9256）：KEK 解析改为按 OID 定位后，加载明文用的是限定名
--   "schema.name"，故第 6 步的硬报错里 KEK 标识为 'public.kek_fm' 而非裸名
--   'kek_fm'。限定名可区分分处不同 schema 的同名 KEK，是期望的形式。
-- 放 libpq/sql：读路径的静默行为只在 libpq 客户端体现（psql 会打印 stderr）。
-- ============================================================

-- 0. 幂等清理 + 自生成 sm4 对称 KEK
DROP TABLE IF EXISTS t_fm CASCADE;
DROP DEK IF EXISTS dek_fm CASCADE;
DROP KEK IF EXISTS kek_fm CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_fm.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_fm --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK kek_fm WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_fm.key'
);
CREATE DEK dek_fm WITH VALUES (KEK = 'kek_fm');

-- 1. 基线：建表 + 插入 + 明文 SELECT（文件在场，解密正常）
CREATE TABLE t_fm (
    id INT,
    a  INT4_ENC ENCRYPTED WITH (DEK = 'dek_fm', SUPPORT_ALGO = 'det'),
    b  TEXT_ENC ENCRYPTED WITH (DEK = 'dek_fm', SUPPORT_ALGO = 'det')
);
INSERT INTO t_fm VALUES (1, 10, 'alpha'), (2, 20, 'bravo');
SELECT 'baseline plaintext', id, a, b FROM t_fm ORDER BY id;

-- 2. tecc_ktool delete 销毁 .key 文件（预置断言：文件确已消失）
\! tecc_ktool delete --key-path "$TECC_KEK_PATH/kek_fm.key" >/dev/null 2>&1
\! test ! -f "$TECC_KEK_PATH/kek_fm.key" && echo 'key file gone'

-- 3. 重连（清空连接级 KEK 明文缓存 → 冷缓存重新读文件，必失败）
\c postgres postgres

-- 4. 缺失文件 SELECT → 静默返回密文（修复前会向 stderr 刷 KEK ERROR）
SELECT 'after delete select1', id, a, b FROM t_fm ORDER BY id;

-- 5. 同连接再 SELECT → 仍静默（负缓存短路，不重复尝试/不重复打日志）
SELECT 'after delete select2', id, a, b FROM t_fm ORDER BY id;

-- 6. 写路径必须硬报错（INSERT 加密需要 DEK → 加载 KEK 失败）
INSERT INTO t_fm VALUES (3, 30, 'charlie');

-- 7. 清理
DROP TABLE t_fm CASCADE;
DROP DEK dek_fm CASCADE;
DROP KEK kek_fm CASCADE;
