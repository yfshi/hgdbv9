-- ============================================================
-- kek_dup_check.sql — CREATE KEK 重复预检（前端英文报错）回归测试
-- 背景（QA 报修）：用已登记 key 文件再建第二个 KEK 时，后端 createkek.c 的
-- 查重报错在 lc_messages=zh_CN 服务器上带中文本地化标签（"错误:"/"描述:"），
-- 而同会话其余 TECC CREATE KEK 校验（缺文件等）是前端硬编码英文（"ERROR:"）。
-- 修复：前端发 SQL 前镜像后端查重规则并以前端英文风格拦截（stmt_processor.c
-- tecc_create_kek_check_duplicate），后端查重保留作权威兜底。
-- 此处验证：重复 key 文件 / 重复 KEK 名报英文错误且不落库；
--           IF NOT EXISTS + 同名 仍由后端 NOTICE 跳过（前端预检放行）。
-- 注意：key_path 用固定 /tmp 字面量（C 层经 TECC_KEK_PATH 覆盖到本回归 key 目录，
-- 存储/比对的 KEY_PATH 即该字面量），保证错误文本跨机器稳定。
-- ============================================================

-- 0. 幂等清理
DROP KEK IF EXISTS kekdup_a CASCADE;
DROP KEK IF EXISTS kekdup_a2 CASCADE;
DROP KEK IF EXISTS kekdup_b CASCADE;

-- 1. 基线：文件 A → KEK kekdup_a（密钥文件由框架预生成到 TECC_KEK_PATH）
CREATE KEK kekdup_a WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kekdup_a_regress.key'
);

-- 2. 同一 key 文件，不同 KEK 名 → 前端英文重复文件错误（QA 复现场景）
CREATE KEK kekdup_a2 WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kekdup_a_regress.key'
);

-- 3. 同一 KEK 名，不同文件 → 前端英文重复名错误
CREATE KEK kekdup_a WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kekdup_b_regress.key'
);

-- 4. IF NOT EXISTS + 同名：预检放行，后端 NOTICE 跳过（不报错）
CREATE KEK IF NOT EXISTS kekdup_a WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kekdup_b_regress.key'
);

-- 5. 独立文件+名字 → 正常创建
CREATE KEK kekdup_b WITH (
    key_store = 'localkms',
    key_algo = 'sm4',
    key_path = '/tmp/kekdup_b_regress.key'
);

-- 6. 校验落库结果：kekdup_a2 未创建
SELECT kek_name FROM hg_kek
WHERE kek_name IN ('kekdup_a', 'kekdup_a2', 'kekdup_b') ORDER BY 1;

-- 7. 清理
DROP KEK kekdup_a CASCADE;
DROP KEK kekdup_b CASCADE;
