--
-- kek_restore_missing_pub.sql — pg_dump 还原非对称 KEK：目标机缺 .pub 必须失败
--
-- 背景（Bug 记录 L1317 / QA 用例 TC_HGDB_TECC4_TOOL_DUMP_012）：
--   产品概要设计 6.2.3/6.2.4 要求 localkms 还原前须在目标机放好密钥文件
--   （对称 .key；非对称 .priv + .pub 且同目录），缺失即还原报错。
--   实际恢复链路曾有两条各自独立的缺口：
--     A. pg_dump 输出的是规范父括号形式 `CREATE KEK schema.name (`（不带 WITH），
--        前端 tecc 语法解析不出 parse tree → 走 parse_tree=NULL 的透传分支，
--        而该分支不做任何 key 文件在位校验；
--     B. 实时 CREATE KEK 的 .pub 缺失原先只是 WARNING，KEK 照样建成功，
--        只是 public_key 为空。
--   于是"只放 .priv、不放 .pub"的还原会【静默成功】（退出 0），建出来的 KEK
--   公钥为空 —— 后续 DET/HE 加解密全部不可用，且失败点远离现场。
--
-- 判定点：
--   第 1 段只放 .priv → 还原必须失败：ERROR + psql 退出非 0 + 目标库无 KEK 元数据；
--   第 2 段 .priv/.pub 齐备 → 还原成功并建出 KEK（对照组，证明第 1 段失败确实
--   来自缺 .pub，而不是路径写错或环境污染）。
--
-- 注：\! 子进程继承的是主 psql 被污染的 PGENABLETECC=false（数据获取器
-- putenv 后未恢复），故凡在 \! 里新起的 psql/pg_dump 都要显式打开 TECC
-- （psql 用 -k on，pg_dump 用 PGENABLETECC=true）。还原必须走前端校验，
-- 少了这个开关整条用例会退化成"不做校验也通过"的假阳性。
-- ============================================================

SELECT '=== 0 准备：sm2 KEK + he-add DEK + 纯 HE 表，导出还原脚本 ===';

\! dropdb --if-exists tecc_rp_src 2>&1
\! dropdb --if-exists tecc_rp_dst_nopub 2>&1
\! dropdb --if-exists tecc_rp_dst_ok 2>&1
\! rm -f "$TECC_KEK_PATH/kek_rp.priv" "$TECC_KEK_PATH/kek_rp.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_rp --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! createdb tecc_rp_src 2>&1
\! psql -q -k on -d tecc_rp_src -c "CREATE KEK kek_rp WITH (key_store='localkms', key_algo='sm2', key_path='%KEKPATH%/kek_rp.priv');" 2>&1
\! psql -q -k on -d tecc_rp_src -c "CREATE DEK dek_rp WITH VALUES(KEK='kek_rp', KEY_ALGO='he-add');" 2>&1
\! psql -q -k on -d tecc_rp_src -c "CREATE TABLE t_rp (id int, a int4_enc ENCRYPTED WITH (SUPPORT_ALGO='he-add', HE_ADD_DEK='public.dek_rp'));" 2>&1
\! psql -q -k on -d tecc_rp_src -c "INSERT INTO t_rp VALUES (1,10),(2,20);" 2>&1
\! PGENABLETECC=true pg_dump -d tecc_rp_src --schema-only --no-owner -f tecc_rp_dump.sql 2>&1
\! dropdb --if-exists tecc_rp_src 2>&1

SELECT '=== 1 目标目录只有 .priv → 还原必须失败 ===';

\! rm -rf "$TECC_KEK_PATH/nopub"; mkdir -p "$TECC_KEK_PATH/nopub"; cp "$TECC_KEK_PATH/kek_rp.priv" "$TECC_KEK_PATH/nopub/"
\! createdb tecc_rp_dst_nopub 2>&1
\! TECC_KEK_PATH="$TECC_KEK_PATH/nopub" psql -k on -v ON_ERROR_STOP=1 -d tecc_rp_dst_nopub -f tecc_rp_dump.sql 2>&1; echo "psql rc=$?"
\echo '目标库里没有建出 KEK（0 = 正确）：'
\! psql -d tecc_rp_dst_nopub -At -c "SELECT count(*) FROM hg_kek WHERE kek_name='kek_rp'" 2>&1

SELECT '=== 2 对照组：.priv + .pub 齐备 → 还原成功 ===';

\! createdb tecc_rp_dst_ok 2>&1
\! psql -k on -v ON_ERROR_STOP=1 -q -d tecc_rp_dst_ok -f tecc_rp_dump.sql 2>&1; echo "psql rc=$?"
\echo '目标库里 KEK 已建出（1 = 正确）：'
\! psql -d tecc_rp_dst_ok -At -c "SELECT count(*) FROM hg_kek WHERE kek_name='kek_rp'" 2>&1

SELECT '=== 99 CLEANUP ===';

\! dropdb --if-exists tecc_rp_dst_nopub 2>&1
\! dropdb --if-exists tecc_rp_dst_ok 2>&1
\! rm -rf "$TECC_KEK_PATH/nopub"
\! rm -f tecc_rp_dump.sql
\! rm -f "$TECC_KEK_PATH/kek_rp.priv" "$TECC_KEK_PATH/kek_rp.pub" 2>/dev/null
