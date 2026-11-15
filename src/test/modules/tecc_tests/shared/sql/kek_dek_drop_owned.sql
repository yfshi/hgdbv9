-- ============================================================
-- kek_dek_drop_owned.sql — DROP OWNED BY 清理持有 KEK/DEK 授权的角色
-- 背景（QA "步骤8清理宕库" 场景排查中发现的真实缺陷）：GRANT USAGE ON KEK/DEK
-- 授权给某角色后，用 DROP OWNED BY <role> CASCADE 清理该角色（须从对象 ACL 中
-- 剥离其授权）时，aclchk.c RemoveRoleFromObjectACL() 的 classid→objtype 分派
-- 遗漏 HgKekRelationId/HgDekRelationId → elog("unexpected object class 11000")，
-- 角色清理中途报错中断（后续 DROP USER 也连带失败）。
-- 修复：RemoveRoleFromObjectACL 补 KEK/DEK 两分支 → OBJECT_KEK/OBJECT_DEK。
-- 本用例证明：DROP OWNED BY 能剥离角色对 KEK/DEK 的 USAGE，随后 DROP USER 成功。
-- 属后端 ACL 行为，但因建 KEK/DEK 走 libpq C 前端（create_kek_processor），放 libpq/sql。
-- 注意：key_path 用固定 /tmp 字面量（C 层经 TECC_KEK_PATH 覆盖到本回归 key 目录），
-- 密钥文件由 ＼! tecc_ktool generate 自生成到 $TECC_KEK_PATH（框架预生成同一 key-name 即跳过）。
-- ============================================================

-- 0. 幂等清理（do_u1 若残留：先 DROP OWNED 剥离其授权/清其属物，再删角色）
DROP KEK IF EXISTS kek_do CASCADE;
DROP DEK IF EXISTS dek_do CASCADE;
DO $$
BEGIN
   IF EXISTS (SELECT 1 FROM pg_roles WHERE rolname = 'do_u1') THEN
      DROP OWNED BY do_u1 CASCADE;
   END IF;
END $$;
DROP USER IF EXISTS do_u1;

\! rm -f "$TECC_KEK_PATH/kek_do.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_do --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

-- 1. owner（当前超级用户）建 KEK/DEK，仅向 do_u1 授权，不转交属权
CREATE USER do_u1;
CREATE KEK kek_do WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_do.key'
);
CREATE DEK dek_do WITH VALUES (KEK = kek_do);
GRANT USAGE ON KEK kek_do TO do_u1;
GRANT USAGE ON DEK dek_do TO do_u1;

-- 2. 授权已生效（基线，期待 t/t）
SELECT has_kek_privilege('do_u1', 'kek_do', 'USAGE') AS kek_granted,
       has_dek_privilege('do_u1', 'dek_do', 'USAGE') AS dek_granted;

-- 3. 修复前：DROP OWNED BY do_u1 → "ERROR: unexpected object class 11000"
--    修复后：成功剥离 do_u1 对 KEK/DEK 的授权（回归守卫：无 ERROR 输出）
DROP OWNED BY do_u1 CASCADE;

-- 4. 授权已剥离（期待 f/f）——证明 RemoveRoleFromObjectACL 确实清理了 KEK/DEK ACL
SELECT has_kek_privilege('do_u1', 'kek_do', 'USAGE') AS kek_granted,
       has_dek_privilege('do_u1', 'dek_do', 'USAGE') AS dek_granted;

-- 5. 角色可正常删除（修复前 DROP OWNED 中途 ERROR 后此处不可达）
DROP USER do_u1;
SELECT 'drop-owned role cleanup ok' AS status;
