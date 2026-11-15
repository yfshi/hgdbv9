--
-- pg_dump.sql — pg_dump/pg_restore 密态表导出导入验证
--

-- ============================================================
-- 准备
-- ============================================================
DROP TABLE IF EXISTS t_pgdump_no_ope CASCADE;
DROP TABLE IF EXISTS t_pgdump_ope CASCADE;
DROP TABLE IF EXISTS test_pg_dump CASCADE;
DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek_pgdump CASCADE;
DROP KEK IF EXISTS kek_pgdump CASCADE;

CREATE KEK kek_pgdump WITH (
    key_store = 'localkms',
    key_path = '/tmp/kek_pgdump.key'
);
CREATE DEK dek_pgdump WITH VALUES(KEK = 'kek_pgdump');

-- ============================================================
-- 表1: 无OPE列 (schema-only — SUPPORT_ALGO 子句验证)
-- ============================================================
CREATE TABLE t_pgdump_no_ope (
    id   SERIAL PRIMARY KEY,
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH'),
    val  INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    note TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO t_pgdump_no_ope (name, val, note) VALUES
('Alice', 100, 'first'), ('Bob', 200, 'second'), ('Charlie', 300, 'third');

-- ============================================================
-- 表2: 含OPE列 (schema-only — COLLATE 子句验证)
-- ============================================================
CREATE TABLE t_pgdump_ope (
    id     SERIAL PRIMARY KEY,
    txt    TEXT_ENC        ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    vc     VARCHAR_ENC(50) ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    bp     BPCHAR_ENC      ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE'),
    amount INT4_ENC        ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'OPE')
);
INSERT INTO t_pgdump_ope (txt, vc, bp, amount) VALUES
('apple', 'red', 'A', 10), ('banana', 'yellow', 'B', 20), ('cherry', 'red', 'C', 30);

-- ============================================================
-- 表3: DET + SEARCH (data-only + restore, from inline test)
-- ============================================================
CREATE TABLE test_pg_dump (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO test_pg_dump VALUES (1, 'Alice'), (2, 'Bob'), (3, 'Charlie');

-- ============================================================
-- 表4: 全9种加密类型 (data-only + restore, Bug 8047)
-- ============================================================
CREATE TABLE tb_all (
    id          INT,
    col_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO tb_all VALUES (1, 'text_value', 'varchar_value', 'bpchar_val',
    123::INT2, 456789::INT4, 9876543210::INT8, 12345.67::NUMERIC,
    3.14::FLOAT4, 2.718281828::FLOAT8);

-- ============================================================
-- schema-only
-- ============================================================
\echo '=== 1 schema-only no OPE ==='
\! pg_dump -t t_pgdump_no_ope --schema-only --no-owner 2>&1

\echo '=== 2 schema-only OPE ==='
\! pg_dump -t t_pgdump_ope --schema-only --no-owner 2>&1

-- ============================================================
-- data-only + restore
-- ============================================================
\echo '=== 3 data-only test_pg_dump ==='
\! pg_dump -t test_pg_dump --data-only 2>&1
\! pg_dump -t test_pg_dump --data-only -f tecc_pgdump_data.sql 2>&1
TRUNCATE test_pg_dump;
\i tecc_pgdump_data.sql
\echo 'restore test_pg_dump:'
SELECT id, name FROM test_pg_dump ORDER BY id;

\echo '=== 4 data-only tb_all (9 types) ==='
\! pg_dump -t tb_all --data-only 2>&1
\! pg_dump -t tb_all --data-only -f tecc_pgdump_9col.sql 2>&1
DELETE FROM tb_all;
\i tecc_pgdump_9col.sql
\echo 'restore tb_all:'
SELECT * FROM tb_all;

-- ============================================================
-- pg_dump -Fc (custom format) + pg_restore
-- ============================================================

-- pg_dump/pg_restore 通过 psql -At 获取数据库名（\! 中 :variable 不展开）

-- ============================================================
-- -Fc (custom format) + pg_restore 测试
-- 每个子节自包含：建表→dump→drop→restore→验证→清理
-- NOTE: \i 导入 pg_dump 输出会把 search_path 清空，需手动恢复
-- ============================================================
SET search_path TO public;

-- 5a: -Fc data-only (验证 COPY 加密走 PQputCopyData)
\echo '=== 5a pg_dump -Fc --data-only + pg_restore --data-only ==='
DROP TABLE IF EXISTS t_fc_data CASCADE;
CREATE TABLE public.t_fc_data (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_fc_data VALUES (10, 'Alice'), (20, 'Bob');
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_data --data-only -Fc -f tecc_fc_data.dump 2>&1
TRUNCATE t_fc_data;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" --data-only tecc_fc_data.dump 2>&1
\echo 'restore:'
SELECT id, name FROM t_fc_data;
DROP TABLE t_fc_data CASCADE;

-- 5b: -Fc full (schema+data, 验证 search_path 空时 DEK 查找)
\echo '=== 5b pg_dump -Fc full + pg_restore full ==='
DROP TABLE IF EXISTS t_fc_full CASCADE;
CREATE TABLE public.t_fc_full (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_fc_full VALUES (100, 'Alice'), (200, 'Bob');
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_full -Fc -f tecc_fc_full.dump 2>&1
DROP TABLE t_fc_full CASCADE;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" tecc_fc_full.dump 2>&1
\echo 'restore:'
SELECT id, name FROM t_fc_full;
DROP TABLE t_fc_full CASCADE;

-- 5c: -Fc full + 全9种加密类型
\echo '=== 5c pg_dump -Fc full 9 types + pg_restore ==='
DROP TABLE IF EXISTS t_fc_9col CASCADE;
CREATE TABLE public.t_fc_9col (
    id          INT,
    col_text    TEXT_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_varchar VARCHAR_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_bpchar  BPCHAR_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int2    INT2_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int4    INT4_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_int8    INT8_ENC    ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_numeric NUMERIC_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float4  FLOAT4_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump'),
    col_float8  FLOAT8_ENC  ENCRYPTED WITH (DEK = 'public.dek_pgdump')
);
INSERT INTO t_fc_9col VALUES (1, 'text_value', 'varchar_value', 'bpchar_val',
    123::INT2, 456789::INT4, 9876543210::INT8, 12345.67::NUMERIC,
    3.14::FLOAT4, 2.718281828::FLOAT8);
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -t t_fc_9col -Fc -f tecc_fc_9col.dump 2>&1
DROP TABLE t_fc_9col CASCADE;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" tecc_fc_9col.dump 2>&1
\echo 'restore:'
SELECT * FROM t_fc_9col;
DROP TABLE t_fc_9col CASCADE;

-- 5d: -1 单事务恢复（CREATE KEK/DEK/TABLE 与 COPY 数据同处一个未提交事务）
--     QA 用例 TC_HGDB_TECC4_TOOL_DUMP_007（sym det）/ _008（comb: det+he-add+he-mul）。
--     修复前 COPY 行取键失败——前端缓存里 KEK/DEK 只有 PENDING 状态（同事务
--     内尚未提交），报 "error returned by PQputCopyData" 且整个事务回滚。
--     上面 5a/5b/5c 都是多事务恢复（pg_restore 每条 CREATE 各自提交，缓存
--     已刷新），覆盖不到这条路径。
--     用独立源库：归档必须自带 CREATE KEK/DEK 才会走这段代码，而 -t 单表
--     dump 不含依赖的 KEK/DEK（见第 9 节的 schema-only dump）。
--     注：\! 子进程继承的是主 psql 被污染的 PGENABLETECC=false（数据获取器
--     putenv 后未恢复），故凡在 \! 里新起的 psql/pg_dump/pg_restore 都要显式
--     打开 TECC（psql 用 -k on，pg_dump/pg_restore 用 PGENABLETECC=true），
--     否则它们跑的是非密态语义。
\echo '=== 5d pg_restore -1 single transaction ==='
\! dropdb --if-exists tecc_1tx_src 2>&1
\! dropdb --if-exists tecc_1tx_dst 2>&1
\! createdb tecc_1tx_src 2>&1
\! rm -f "$TECC_KEK_PATH/kek_1tx.key" 2>/dev/null
\! tecc_ktool generate --key-name kek_1tx --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! psql -q -k on -d tecc_1tx_src -c "CREATE KEK kek_1tx WITH (key_store='localkms', key_path='%KEKPATH%/kek_1tx.key');" 2>&1
\! psql -q -k on -d tecc_1tx_src -c "CREATE DEK dek_1tx WITH VALUES(KEK='kek_1tx');" 2>&1
\! psql -q -k on -d tecc_1tx_src -c "CREATE TABLE t_1tx (id int, a int4_enc ENCRYPTED WITH (DEK='public.dek_1tx', SUPPORT_ALGO='DET'), b text_enc ENCRYPTED WITH (DEK='public.dek_1tx', SUPPORT_ALGO='DET'));" 2>&1
\! psql -q -k on -d tecc_1tx_src -c "INSERT INTO t_1tx VALUES (1,10,'alpha'),(2,20,'beta');" 2>&1
\! PGENABLETECC=true pg_dump -d tecc_1tx_src -Fc -f tecc_1tx.dump 2>&1
\! dropdb --if-exists tecc_1tx_src 2>&1
\! createdb tecc_1tx_dst 2>&1
-- 单事务：归档里的 CREATE KEK/DEK/TABLE 与 COPY 数据一次提交
\! PGENABLETECC=true pg_restore -d tecc_1tx_dst -1 tecc_1tx.dump 2>&1
\echo 'restored (single transaction):'
\! psql -k on -d tecc_1tx_dst -At -c "SELECT id, a, b FROM t_1tx ORDER BY id" 2>&1
\! psql -k on -d tecc_1tx_dst -At -c "SELECT 'det_eq', count(*) FROM t_1tx WHERE b = 'beta'" 2>&1
\! dropdb --if-exists tecc_1tx_dst 2>&1
\! rm -f tecc_1tx.dump

-- ============================================================
-- pg_restore 权限验证（DEK USAGE 不足时拒绝）
-- ============================================================
\echo '=== 6 pg_restore ACL: DEK permission denied ==='
DROP TABLE IF EXISTS t_acl CASCADE;
CREATE TABLE public.t_acl (
    id   INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET'),
    name TEXT_ENC ENCRYPTED WITH (DEK = 'public.dek_pgdump', SUPPORT_ALGO = 'DET,SEARCH')
);
INSERT INTO t_acl VALUES (1, 'Alice'), (2, 'Bob');

DROP USER IF EXISTS regress_pgdump_user;
CREATE USER regress_pgdump_user;
GRANT SELECT ON t_acl TO regress_pgdump_user;
-- 撤销 DEK 权限
REVOKE ALL ON DEK dek_pgdump FROM regress_pgdump_user;
REVOKE ALL ON KEK kek_pgdump FROM regress_pgdump_user;
-- dump
\! DB=$(psql -At -c "SELECT current_database()"); pg_dump -d "$DB" -U regress_pgdump_user -t t_acl -Fc -f tecc_acl.dump 2>&1
DROP TABLE t_acl CASCADE;
-- restore（无 DEK 权限，应报 permission denied）
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" -U regress_pgdump_user tecc_acl.dump 2>&1
\echo '--- grant DEK USAGE ---'
GRANT USAGE ON DEK dek_pgdump TO regress_pgdump_user;
GRANT USAGE ON KEK kek_pgdump TO regress_pgdump_user;
GRANT ALL ON SCHEMA public TO regress_pgdump_user;
\! DB=$(psql -At -c "SELECT current_database()"); pg_restore -d "$DB" -U regress_pgdump_user tecc_acl.dump 2>&1
\echo 'restore with ACL:'
SELECT id, name FROM t_acl;
DROP TABLE IF EXISTS t_acl CASCADE;
DROP USER IF EXISTS regress_pgdump_user;

-- ============================================================
-- 7. pg_dump KEK/DEK SQL 语句导出验证
-- ============================================================
\echo '=== 7 pg_dump KEK/DEK ==='
\! pg_dump --schema-only --no-owner -f _pgdump_kek_dek.sql 2>/dev/null; grep -E "CREATE KEK .*kek_pgdump" _pgdump_kek_dek.sql 2>/dev/null
\! grep -E "CREATE DEK .*dek_pgdump" _pgdump_kek_dek.sql 2>/dev/null

-- ============================================================
-- 8. sm2（非对称）KEK 的 pg_dump/restore round-trip
--    覆盖 public_key 转义不一致（dump 用 encode(value,'escape') 输出
--    转义文本，restore 端 createkek.c 按 \x<hex> 解析 → 恢复失败）
--    用独立源库隔离：全库 dump 会带出其他用例（swxa 失败残留等）
--    对象，使 expected 依赖测试顺序/环境，故在专用源库建 KEK 后 dump。
-- ============================================================
\echo '=== 8 sm2 KEK pg_dump/restore ==='
DROP KEK IF EXISTS kek_pgdump_sm2 CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_pgdump_sm2.priv" "$TECC_KEK_PATH/kek_pgdump_sm2.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_pgdump_sm2 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
-- 主库原始 public_key 结构校验（65 字节未压缩点：04 前缀 + hex 130）
CREATE KEK kek_pgdump_sm2 WITH (
    key_store = 'localkms',
    key_algo = 'sm2',
    key_path = '%KEKPATH%/kek_pgdump_sm2.priv'
);
SELECT '8a_orig_pub_ok', length(encode(value,'hex')) = 130 AND encode(value,'hex') LIKE '04%' AS pub_ok
FROM hg_kek_args WHERE kek_id=(SELECT oid FROM hg_kek WHERE kek_name='kek_pgdump_sm2') AND upper(key)='PUBLIC_KEY';
DROP KEK kek_pgdump_sm2 CASCADE;
-- DROP KEK CASCADE 会删除密钥文件，源库需重新生成密钥对
\! rm -f "$TECC_KEK_PATH/kek_pgdump_sm2.priv" "$TECC_KEK_PATH/kek_pgdump_sm2.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_pgdump_sm2 --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
-- 独立源库建 KEK → dump → 还原新库
\! dropdb --if-exists tecc_sm2_src 2>&1
\! createdb tecc_sm2_src 2>&1
\! psql -k on -d tecc_sm2_src -q -c "CREATE KEK kek_pgdump_sm2 WITH (key_store = 'localkms', key_algo = 'sm2', key_path = '%KEKPATH%/kek_pgdump_sm2.priv');" 2>&1
-- Bug 9270：4.0.0→4.0.1 迁移库里的 PUBLIC_KEY 行可能不是全大写写法，而
-- getKekInfo 曾用大小写敏感的 ka.key = 'PUBLIC_KEY' 判定，不匹配就落到
-- encode(value,'escape')，把 65 字节公钥点写成转义二进制 → restore 报
-- invalid hexadecimal digit: ""（无上下文）。此处把数据字典里该行的 key 改成
-- 小写，模拟迁移库，断言 dump 仍按 hex 输出、且下方 restore 能成功。
\! psql -k on -d tecc_sm2_src -q -c "UPDATE hg_kek_args SET key = 'public_key' WHERE upper(key) = 'PUBLIC_KEY' AND kek_id = (SELECT oid FROM hg_kek WHERE kek_name = 'kek_pgdump_sm2');" 2>&1
\echo '--- 8b dump public_key 须为 130 位 hex 且 04 开头 ---'
\! pg_dump -d tecc_sm2_src --schema-only --no-owner -f tecc_sm2_kek.sql 2>&1
\! grep -qE "public_key = '[0-9a-f]{130}'" tecc_sm2_kek.sql && echo '8b_dump_pub_hex ok' || echo '8b_dump_pub_hex BAD'
\! dropdb --if-exists tecc_sm2_src 2>&1
\! dropdb --if-exists tecc_restore_sm2 2>&1
\! createdb tecc_restore_sm2 2>&1
\echo '--- restore into fresh db ---'
\! psql -d tecc_restore_sm2 -f tecc_sm2_kek.sql 2>&1
\echo 'restored sm2 KEK public_key:'
\! psql -d tecc_restore_sm2 -At -c "SELECT length(encode(value,'hex')) = 130 AND encode(value,'hex') LIKE '04%' FROM hg_kek_args WHERE kek_id=(SELECT oid FROM hg_kek WHERE kek_name='kek_pgdump_sm2') AND upper(key)='PUBLIC_KEY'" 2>&1
\! dropdb --if-exists tecc_restore_sm2 2>&1

-- ============================================================
-- 清理
-- ============================================================
DROP TABLE IF EXISTS t_pgdump_no_ope CASCADE;
DROP TABLE IF EXISTS t_pgdump_ope CASCADE;
DROP TABLE IF EXISTS test_pg_dump CASCADE;
DROP TABLE IF EXISTS tb_all CASCADE;
DROP DEK IF EXISTS dek_pgdump CASCADE;
DROP KEK IF EXISTS kek_pgdump CASCADE;

-- ============================================================
-- 9. HE (he-add/he-mul) 绑定导出 round-trip — Bug 记录 L471
--    pg_dump 曾只 LEFT JOIN hg_encrypted_columns→hg_dek（主对称 DEK），从不读
--    hg_encrypted_column_args → HE_ADD_DEK/HE_MUL_DEK 丢失；纯 HE 列还会输出
--    损坏的 DEK = ""。修复后三种列形态均须带 HE 绑定导出、restore 后绑定还原。
--    断言载体：下方 inline schema-only dump 全文进 expected —— HE 参数一旦再
--    丢失输出即与期望不符；restore 后 明文读/同态加/标量乘 再证绑定已还原。
--    本节自包含（自带 KEK/DEK/表 + 清理），放在文件末尾，不影响既有各节。
-- ============================================================
SET search_path TO public;
DROP TABLE IF EXISTS he_dump_t CASCADE;
DROP DEK IF EXISTS dek_he_s CASCADE;
DROP DEK IF EXISTS dek_he_add CASCADE;
DROP DEK IF EXISTS dek_he_mul CASCADE;
DROP KEK IF EXISTS kek_he_s CASCADE;
DROP KEK IF EXISTS kek_he_a CASCADE;
\! rm -f "$TECC_KEK_PATH/kek_he_s.key" "$TECC_KEK_PATH/kek_he_a.priv" "$TECC_KEK_PATH/kek_he_a.pub" 2>/dev/null
\! tecc_ktool generate --key-name kek_he_s --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1
\! tecc_ktool generate --key-name kek_he_a --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK kek_he_s WITH (key_store = 'localkms', key_path = '/tmp/kek_he_s.key');
CREATE DEK dek_he_s WITH VALUES(KEK = 'kek_he_s');
CREATE KEK kek_he_a WITH (key_store = 'localkms', key_algo = 'sm2', key_path = '%KEKPATH%/kek_he_a.priv');
CREATE DEK dek_he_add WITH VALUES(KEK = 'kek_he_a', KEY_ALGO = 'he-add');
CREATE DEK dek_he_mul WITH VALUES(KEK = 'kek_he_a', KEY_ALGO = 'he-mul');

-- 三种列形态：多 DEK 绑定列 / 纯 he-add 列 / 纯 he-mul 列
CREATE TABLE public.he_dump_t (
    id     INT,
    c_full INT4_ENC ENCRYPTED WITH (DEK = 'public.dek_he_s', SUPPORT_ALGO = 'det,he-add,he-mul', HE_ADD_DEK = 'public.dek_he_add', HE_MUL_DEK = 'public.dek_he_mul'),
    c_add  INT4_ENC ENCRYPTED WITH (SUPPORT_ALGO = 'he-add', HE_ADD_DEK = 'public.dek_he_add'),
    c_mul  INT4_ENC ENCRYPTED WITH (SUPPORT_ALGO = 'he-mul', HE_MUL_DEK = 'public.dek_he_mul')
);
INSERT INTO he_dump_t VALUES (1, 10, 100, 3), (2, 20, 200, 5);

\echo '=== 9 HE schema-only dump ==='
\! pg_dump -t he_dump_t --schema-only --no-owner 2>&1
\! pg_dump -t he_dump_t --schema-only --no-owner -f tecc_he_dump.sql 2>&1

-- DROP → restore → 绑定还原（\i 会清空 search_path，随后显式恢复）
DROP TABLE he_dump_t CASCADE;
\i tecc_he_dump.sql
SET search_path TO public;
INSERT INTO he_dump_t VALUES (3, 30, 300, 7);
\echo 'restore he_dump_t:'
SELECT id, c_full, c_add, c_mul FROM he_dump_t ORDER BY id;
\echo 'HE binding survived restore:'
SELECT 'he_sum', SUM(c_add) FROM he_dump_t;
SELECT 'he_scalar_mul', c_mul * 2 FROM he_dump_t ORDER BY id;

-- ============================================================
-- 9b. 纯 HE 列的数据段 round-trip — QA 用例 TC_HGDB_TECC4_TOOL_DUMP_010
--     dump 的数据段走【单值解密】路径，而 SELECT 走【批量解密】，两者对纯 HE
--     列（无对称 DEK，明文在 EG_ADD/EG_MUL 段）的处理曾不一致：单值解密把值
--     导出成 \N，恢复端 COPY 又因纯 HE 列取不到对称键名整行失败
--     （"Failed to process COPY data row"）。
--     第 9 节是 --schema-only，只覆盖 schema 绑定，覆盖不到数据段。
-- ============================================================
\echo '=== 9b HE full dump (schema+data) round-trip ==='
-- 先把数据补到 3 行：纯 he-add / 纯 he-mul 两列都要有非 NULL 值参与往返
INSERT INTO he_dump_t VALUES (4, 40, 400, 9), (5, 50, 500, 11);
\! PGENABLETECC=true pg_dump -t he_dump_t --no-owner -f tecc_he_full.sql 2>&1
\! echo -n 'data-segment NULL markers in dump (expect 0): '; grep -c '\\N' tecc_he_full.sql || true
DROP TABLE he_dump_t CASCADE;
\i tecc_he_full.sql
SET search_path TO public;
\echo 'restored from full dump:'
SELECT id, c_full, c_add, c_mul FROM he_dump_t ORDER BY id;
\echo 'HE ops on restored data:'
SELECT 'he_sum', SUM(c_add) FROM he_dump_t;
SELECT 'he_scalar_mul', c_mul * 2 FROM he_dump_t ORDER BY id;

-- 清理
DROP TABLE he_dump_t CASCADE;
DROP DEK dek_he_s CASCADE;
DROP DEK dek_he_add CASCADE;
DROP DEK dek_he_mul CASCADE;
DROP KEK kek_he_s CASCADE;
DROP KEK kek_he_a CASCADE;
