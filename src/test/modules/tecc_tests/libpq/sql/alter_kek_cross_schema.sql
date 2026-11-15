--
-- alter_kek_cross_schema.sql — libpq专有测试：跨模式 ALTER KEK（限定名）
--
-- Bug 9330：ALTER KEK sch1.kek_sm4 这类限定名，前端只取名字列表的末元素
-- （裸名），模式被丢弃，随后所有 KEK 查找都退回 search_path；当 KEK 所在
-- 模式不在 search_path 上时必然报 "Failed to load old KEK"，跨模式轮换
-- 完全不可用（且报错文案把"跨模式"伪装成"KEK 不存在"）。
--
-- 本例覆盖：
--   1. 限定名跨模式轮换（sch_rot 不在 search_path 上）—— 原报错场景
--   2. 轮换后旧数据仍能解出明文（证明 DEK 真的按新 KEK 重加密过）
--   3. 轮换回原密钥文件
--   4. 另一个模式里有同名 KEK 且 search_path 指向它时，限定名必须精确作用
--      于指定模式的那把 KEK，不得错绑到同名 KEK（错绑会按错误的 OID
--      重加密另一批 DEK，属于静默数据错乱，比报错更危险）
--   5. 反面：**未限定名**时必须与"旧 KEK 明文实际来自哪把"用同一身份。
--      两把同名 KEK 材料相同时，按名字另选一把不会报错（解密守卫失效），
--      会把另一个模式的 DEK 用错 KEK 重加密 —— 静默且不可逆
--
-- 注：新密钥文件沿用 shared/sql/alter_kek.sql 的既有做法（copy 旧文件），
-- 因为 ALTER 的 KEY_PATH 不在测试框架的 KEK 预生成范围内。
--

DROP TABLE IF EXISTS rot_cs_test CASCADE;
DROP TABLE IF EXISTS rot_cs_other CASCADE;
DROP DEK IF EXISTS dek_rot_cs CASCADE;
DROP DEK IF EXISTS dek_rot_cs_other CASCADE;
DROP KEK IF EXISTS sch_rot.kek_rot_cs CASCADE;
DROP KEK IF EXISTS sch_rot_other.kek_rot_cs CASCADE;
DROP SCHEMA IF EXISTS sch_rot CASCADE;
DROP SCHEMA IF EXISTS sch_rot_other CASCADE;

CREATE SCHEMA sch_rot;
CREATE SCHEMA sch_rot_other;

\! rm -f "$TECC_KEK_PATH/kek_rot_cs_new.key" 2>/dev/null

-- ============================================================
-- 1. KEK / DEK / 加密表都在 sch_rot（DEK 放 public，与报障场景一致）
-- ============================================================
CREATE KEK sch_rot.kek_rot_cs WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_rot_cs_old.key'
);

\! cp "$TECC_KEK_PATH/kek_rot_cs_old.key" "$TECC_KEK_PATH/kek_rot_cs_new.key"

CREATE DEK dek_rot_cs WITH VALUES(KEK = 'sch_rot.kek_rot_cs', KEY_ALGO = 'sm4');

CREATE TABLE rot_cs_test (
    id    INT4,
    name  TEXT_ENC ENCRYPTED WITH (DEK = 'dek_rot_cs', SUPPORT_ALGO = 'DET')
);

INSERT INTO rot_cs_test VALUES (1, 'alpha'), (2, 'beta'), (3, 'gamma');

SELECT '1_before' AS tag, id, name FROM rot_cs_test ORDER BY id;

-- DEK 的 encrypted_value（旧 KEK 信封）快照：轮换是否真的落到数据上，
-- 用它对比。只看"轮换后还能查出明文"是不够的 —— 轮换失败时旧 KEK 原封不动，
-- 数据照样能解出来。
CREATE TABLE rot_cs_snap AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value';

-- ============================================================
-- 2. 跨模式轮换：sch_rot 不在 search_path 上（原报错场景）
-- ============================================================
SELECT '2_alter_cross_schema' AS tag;
ALTER KEK sch_rot.kek_rot_cs WITH (KEY_PATH = '/tmp/kek_rot_cs_new.key');

SELECT '2_dek_reencrypted' AS tag,
       (SELECT ev FROM rot_cs_snap) IS DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value') AS dek_reencrypted;

SELECT '2_after_rotate' AS tag, id, name FROM rot_cs_test ORDER BY id;

-- ============================================================
-- 3. 再按限定名轮换回原密钥文件
-- ============================================================
DROP TABLE rot_cs_snap;
CREATE TABLE rot_cs_snap AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value';

SELECT '3_alter_back' AS tag;
ALTER KEK sch_rot.kek_rot_cs WITH (KEY_PATH = '/tmp/kek_rot_cs_old.key');

SELECT '3_dek_reencrypted' AS tag,
       (SELECT ev FROM rot_cs_snap) IS DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value') AS dek_reencrypted;

SELECT '3_after_rotate_back' AS tag, id, name FROM rot_cs_test ORDER BY id;

-- ============================================================
-- 4. 同名 KEK 在另一个模式：限定名必须精确命中
-- ============================================================
-- 让这另一把同名 KEK 与 sch_rot 的那把**密钥材料相同**（拷贝到不同路径，
-- 以通过 KEY_PATH 唯一性校验）。材料相同是第 5 节的前提：材料不同时"旧 KEK
-- 明文"取错一把必然解密失败，"解密守卫"会响亮的 abort；材料相同才会静默走完。
\! cp "$TECC_KEK_PATH/kek_rot_cs_old.key" "$TECC_KEK_PATH/kek_rot_cs_other.key"

CREATE KEK sch_rot_other.kek_rot_cs WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_rot_cs_other.key'
);
CREATE DEK dek_rot_cs_other WITH VALUES(KEK = 'sch_rot_other.kek_rot_cs', KEY_ALGO = 'sm4');
CREATE TABLE rot_cs_other (
    id    INT4,
    name  TEXT_ENC ENCRYPTED WITH (DEK = 'dek_rot_cs_other', SUPPORT_ALGO = 'DET')
);
INSERT INTO rot_cs_other VALUES (9, 'zzz');

\! rm -f "$TECC_KEK_PATH/kek_rot_cs_new2.key" 2>/dev/null
\! cp "$TECC_KEK_PATH/kek_rot_cs_old.key" "$TECC_KEK_PATH/kek_rot_cs_new2.key"

-- 另一把同名 KEK 的 DEK 信封快照：本次轮换必须完全不碰它
CREATE TABLE rot_cs_snap_other AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_rot_cs_other' AND a.key = 'encrypted_value';

-- search_path 指向另一个模式：裸名查找会命中同名的那把 KEK，
-- 限定名必须仍然只作用于 sch_rot.kek_rot_cs
SET search_path = sch_rot_other, public;
SELECT '4_alter_with_same_name_on_path' AS tag;
ALTER KEK sch_rot.kek_rot_cs WITH (KEY_PATH = '/tmp/kek_rot_cs_new2.key');
RESET search_path;

SELECT '4_target_table' AS tag, id, name FROM rot_cs_test ORDER BY id;
SELECT '4_other_table_intact' AS tag, id, name FROM rot_cs_other ORDER BY id;
SELECT '4_other_dek_untouched' AS tag,
       (SELECT ev FROM rot_cs_snap_other) IS NOT DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_rot_cs_other' AND a.key = 'encrypted_value') AS other_dek_untouched;

-- ============================================================
-- 5. 未限定名：身份必须与"旧 KEK 明文实际来自哪把"一致
-- ============================================================
-- search_path 指向 sch_rot_other（它有同名 KEK），用**裸名**轮换。
-- 旧 KEK 明文按 search_path 解析到 sch_rot_other 那把（材料与 sch_rot 相同，
-- 所以能解开 dek_rot_cs 的信封）。因此本次轮换只应作用于 sch_rot_other 那把，
-- 其名下的 dek_rot_cs_other 被重加密，而 sch_rot 的 dek_rot_cs 必须原封不动。
--
-- 若 3a 另行按名字随便命中一把（无 ORDER BY），就会出现"明文来自 A、OID/DEK
-- 列表来自 B"的分裂：dek_rot_cs 的信封被 sch_rot_other 的新密钥重加密，而
-- hg_dek.kek_id 仍指向 sch_rot，此后该 DEK 永久解不开 —— 静默、不可逆。
DROP TABLE rot_cs_snap;
DROP TABLE rot_cs_snap_other;
CREATE TABLE rot_cs_snap AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value';
CREATE TABLE rot_cs_snap_other AS
SELECT encode(a.value, 'hex') AS ev
FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
WHERE d.dek_name = 'dek_rot_cs_other' AND a.key = 'encrypted_value';

SET search_path = sch_rot_other, public;
SELECT '5_bare_name_on_other_schema' AS tag;
-- 新密钥文件与当前材料不同，保证"被重加密"这一断言是确定性的（同材料时
-- 重加密结果是否变化取决于 IV 是否随机，会变成不稳定断言）
\! tecc_ktool generate --key-name kek_rot_cs_mate --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
ALTER KEK kek_rot_cs WITH (KEY_PATH = '/tmp/kek_rot_cs_mate.key');
RESET search_path;

-- 命中的应是 search_path 那把：它名下的 DEK 被重加密
SELECT '5_search_path_dek_rotated' AS tag,
       (SELECT ev FROM rot_cs_snap_other) IS DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_rot_cs_other' AND a.key = 'encrypted_value') AS rotated;

-- 另一个模式的 DEK 不得被碰
SELECT '5_other_schema_dek_untouched' AS tag,
       (SELECT ev FROM rot_cs_snap) IS NOT DISTINCT FROM
       (SELECT encode(a.value, 'hex') FROM hg_dek_args a JOIN hg_dek d ON a.dek_id = d.oid
         WHERE d.dek_name = 'dek_rot_cs' AND a.key = 'encrypted_value') AS untouched;

-- 两张表都必须还能解出明文（分裂时 rot_cs_test 会解不开）
SELECT '5_target_table' AS tag, id, name FROM rot_cs_test ORDER BY id;
SELECT '5_other_table' AS tag, id, name FROM rot_cs_other ORDER BY id;

-- ============================================================
-- 清理
-- ============================================================
DROP TABLE rot_cs_test;
DROP TABLE rot_cs_other;
DROP TABLE rot_cs_snap;
DROP TABLE rot_cs_snap_other;
DROP DEK dek_rot_cs;
DROP DEK dek_rot_cs_other;
DROP KEK sch_rot.kek_rot_cs;
DROP KEK sch_rot_other.kek_rot_cs;
DROP SCHEMA sch_rot;
DROP SCHEMA sch_rot_other;
SELECT 'cleanup_done' AS tag;
