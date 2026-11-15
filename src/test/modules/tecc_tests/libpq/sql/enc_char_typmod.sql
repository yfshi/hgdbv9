--
-- enc_char_typmod.sql — libpq专有测试：字符型密态列的声明长度必须生效（写入+读取）
--
-- 背景（QA 报修 Bug 9327）：
--   建表时声明的长度/精度只落在 pg_attribute.atttypmod 与
--   hg_encrypted_columns.data_type_original_mod 上，写入不含长度校验：
--   `varchar_enc(100)` 列可以插进 150 个字符，`bpchar_enc(50)` 同理，
--   且 information_schema.columns 看不到任何长度/精度（密态类型的 OID 没进
--   information_schema 的 typmod 解码函数），DBA 无从发现列被声明成多长。
--
-- 根因：
--   1. 服务端 varchar_enc/bpchar_enc 的输入函数拿到的是密文（cipherdata_in），
--      无法判断明文长度，长度校验在密态链路上整条缺失——既不在服务端，也不在
--      前端加密前。前端加密之后的比较用的是密文，也无法事后补校验。
--   2. information_schema 的 _pg_char_max_length / _pg_numeric_precision 等
--      函数按基础类型 OID 白名单解码 typmod，密态类型 OID（50000~50007）不在
--      名单里，一律返回 NULL。
--
-- 修复：
--   1. 写入侧（tecc_crypto_scheduler.c 加密前）：按列元数据里的 typmod 校验明文
--      长度，判据与服务端 varchar_input/bpchar_input 完全一致 —— 按【字符】计数，
--      超过 N 个字符时只有超出部分全是空格才接受（服务端会截掉后再存，前端也
--      截断，存的是同一份值），否则报服务端同款错误
--      "value too long for type character varying(N)"/"character(N)"。
--      覆盖 INSERT/UPDATE 字面量、EXECUTE 参数、COPY 等所有写入路径；WHERE
--      比较不套用列的 typmod（与服务端一致：超长常量比较返回空集），但带长度的
--      cast 会先按 cast 声明的长度截断再比较（第 13.9 节）。
--   2. 读取侧（tecc_type_output_data → tecc_type_output_char）：回显时同样按列
--      声明长度定型。只对历史数据（旧版本写进去的超长值）起作用，新写入的值本身
--      就在声明长度内，这里是无操作。与数值类型对称（数值输出也按 typmod 取整）。
--   3. information_schema.sql 的 typmod 解码函数补上密态类型 OID，列声明的
--      长度/精度可以在 information_schema.columns 里读到。
--   4. 带长度的密态 cast（'x'::varchar_enc(N) ENCRYPTED WITH (...)）：前端按原生
--      显式 cast 语义先把明文静默截断到 N 个字符（判据 tecc_char_cast_len_clip），
--      再把【目标列】的 typmod 交给写入校验（赋值语义：超长且超出部分含非空格才
--      拒绝）—— 与原生「显式 cast 静默截断 + 赋值转换严格校验」的分工一致。
--      覆盖 INSERT...VALUES / INSERT...SELECT / 纯 SELECT / WHERE 比较 / CTAS。
--   5. 加密时【目标列还不存在】的写入路径（CTAS、物化视图/视图定义里的 cast、
--      纯 SELECT 里的 cast）取不到列元数据，因而也取不到列排序规则：显式解析
--      "该列将继承的排序规则"（无 COLLATE 的文本类型 = 库默认排序规则的 collcollate）
--      交给加密入口，使写进去的密文与列自身路径一致。加密列的 DEFAULT 走同一机制
--      （resolve_new_column_collation_locale），由 default_value_equality.sql 覆盖。
--   写入侧严格判据 tecc_char_typmod_effective_len 与显式 cast 判据
--   tecc_char_cast_len_clip 共用同一份字符计数实现（tecc_type_output.c）。
--
-- 判定点：
--   第 2 节 声明长度/精度可见；第 3 节 超长被拒、恰好长度成功；第 4 节 尾部
--   空格形态与服务端一致（超出部分是空格 → 截断成声明长度存，可用等值断言验证）；
--   第 5 节 多字节按字符计数（按字节计数会把 2 个汉字误判为超长）；第 6 节
--   text_enc 无长度限制；第 7 节 WHERE 超长常量必须【不】报错（修复不得扩大化）；
--   第 8 节 UPDATE 同样生效；第 9 节 数值精度校验不回退；第 10 节 合法值仍可
--   正常解密回读；第 11 节 EXECUTE 参数同样受约束；第 12 节 历史数据的读取侧定型；
--   第 13 节 带长度的密态 cast（cast 声明长度 + 目标列长度，两条判据）。
--
-- 第 13 节须知（原生对照，实测）：
--   'abcdefghij'::varchar(5)                  -> 存 abcde，不报错（显式 cast 静默截断）
--   'abcdefghij'::varchar(20) 写进 varchar(5) -> 报 value too long（赋值转换）
--   'abcdefghij'::varchar     写进 varchar(5) -> 报 value too long（未声明长度）
--   本节按这三条对照逐项断言，保证密态类型与原生 varchar/bpchar 的分工一致。
--   断言同时用【等值比较】与【回显值】：cast 写进去的值必须能被列自身路径（字面量
--   比较）查到 —— 这要求 cast 路径与列路径用同一套加密参数（DEK/算法/排序规则）。
--   本次修复把目标列的真实表/列名传给加密入口，调度器据此取列的排序规则；修复前
--   cast 路径传占位名（"insert_select"），非 C/POSIX 排序规则的库里不做 ICU 排序键，
--   密文与列路径不等、等值查询静默查不到（改动前后均实测）。
--   同一根因还在所有"加密时目标列还不存在"的非列路径上（CTAS、物化视图/视图定义里的
--   带 ENCRYPTED WITH cast、纯 SELECT 里的 cast）：这些路径改为显式解析该列将继承的
--   排序规则再加密（见第 13.11 节）。
--
-- 第 12 节须知（历史数据的两面性，实测固化）：
--   该节要造出"未经长度校验的存量数据"（等价于修复前的版本写入）。本修复之后所有
--   字面量/参数写入路径都已校验，回归框架里再造存量数据只能用【仍在的密文拷贝
--   路径】：UPDATE ... SET 密态列 = 另一张表更宽声明的密态列 —— 服务端直接拷密文、
--   前端不做长度校验（实测可写入，属另一处未修缺口，见 plan 文档残留；该缺口修掉后
--   本节造法需换成"修复前版本写入的数据"）。读取侧定型后
--   **回显**与服务端一致（第 12.2 节按声明长度显示、第 12.3 节含非空格的值不截断），
--   但存储的密文仍是旧值：这类行参与比较时看的还是实际存储的内容，把回显值再拿去
--   比较不一定命中（实测随列排序规则而异，故本用例不对比较结果作断言）。要彻底收敛
--   到服务端语义需重写一遍：第 12.4 节用回读值重新写入，回显与比较即都回到原生语义
--   （导出后重新导入同理，写入侧现在会自动截断）。新写入的数据没有这一面。
--
-- 归属：长度校验与定型都在前端（服务端拿不到明文），报错文本由 libpq 前端产生，
--   放 libpq/sql。用例本身只用普通 SQL（无 psql 专有呈现），结构上两个驱动都能
--   执行，之所以【暂不】放 shared/ 是因为 Java 驱动尚未做这项校验：实测
--   （2026-09-21，驱动 42.5.1-tecc-4.0.1.0）把本用例喂给 check-tecc-jdbc，第 3.2/
--   3.3/3.5/4.3/5.3 节全部照常 "INSERT 0 1"（超长值被写入），只有第 2 节
--   information_schema 一项因子服务端修复而通过；第 13 节（带长度的 cast）同理，
--   Java 侧既不做 cast 声明长度的截断、也不做目标列长度的校验。此时放进 shared/
--   会把缺陷固化进 shared/expected_jdbc/，等 Java 侧修好后再整体迁到 shared/ 并补
--   三份期望文件。
-- ============================================================

\set ON_ERROR_STOP off

-- ============================================================
-- 0. 幂等清理 + 建密钥
-- ============================================================
DROP TABLE IF EXISTS ct_t CASCADE;
DROP DEK IF EXISTS ct_d CASCADE;
DROP KEK IF EXISTS ct_k CASCADE;

\! rm -f "$TECC_KEK_PATH/ct_k.key" 2>/dev/null
\! tecc_ktool generate --key-name ct_k --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null 2>&1

CREATE KEK ct_k WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '%KEKPATH%/ct_k.key'
);
CREATE DEK ct_d WITH VALUES(KEK = ct_k);

-- ============================================================
-- 1. 建表：带长度/精度的密态类型（含报修用例的 varchar_enc(100)/bpchar_enc(50)）
-- ============================================================
SELECT '=== 1 建表 ===' AS stage;
CREATE TABLE ct_t (
    id   int,
    v5   varchar_enc(5)   ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    v100 varchar_enc(100) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    b3   bpchar_enc(3)    ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    n52  numeric_enc(5,2) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    t    text_enc         ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
);

-- 列声明类型（用 format_type 而非 \d：\d 会带出本机默认排序规则名，golden 不可移植）
SELECT '=== 1b 列声明类型（含长度/精度）===' AS stage;
SELECT attnum, attname, format_type(atttypid, atttypmod) AS decl_type
FROM pg_attribute
WHERE attrelid = 'ct_t'::regclass AND attnum > 0
ORDER BY attnum;

-- ============================================================
-- 2. 声明长度/精度必须可见（Bug 9327 期望 2）
-- ============================================================
SELECT '=== 2 information_schema 显示长度/精度 ===' AS stage;
SELECT column_name, data_type,
       character_maximum_length, character_octet_length,
       numeric_precision, numeric_precision_radix, numeric_scale
FROM information_schema.columns
WHERE table_name = 'ct_t'
ORDER BY ordinal_position;

SELECT '=== 2b atttypmod 原值（104/54 等 = 声明长度 + 4）===' AS stage;
SELECT attname, atttypid::regtype AS typ, atttypmod
FROM pg_attribute
WHERE attrelid = 'ct_t'::regclass AND attnum > 0
ORDER BY attnum;

-- ============================================================
-- 3. 恰好长度放行、超长拒绝（varchar 与 bpchar）
-- ============================================================
SELECT '=== 3.1 varchar_enc(5) 恰好 5 字符 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (1, 'abcde');

SELECT '=== 3.2 varchar_enc(5) 6 字符 -> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (2, 'abcdef');

SELECT '=== 3.3 varchar_enc(5) 20 字符 -> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (3, 'abcdefghijklmnopqrst');

SELECT '=== 3.4 bpchar_enc(3) 恰好 3 字符 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (4, 'abc');

SELECT '=== 3.5 bpchar_enc(3) 4 字符 -> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (5, 'abcd');

-- ============================================================
-- 4. 尾部空格形态：与服务端一致 —— 超出部分是空格时截断到声明长度再存
-- ============================================================
SELECT '=== 4.1 varchar_enc(5) ''ab      ''（8 字符，超出全是空格）-> 存成 5 字符 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (6, 'ab      ');
SELECT '=== 4.1b 存的是服务端那份值（5 字符为真 / 8 字符为假）===' AS stage;
SELECT id, v5 = 'ab   ' AS eq_native5, v5 = 'ab      ' AS eq_old8
FROM ct_t WHERE id = 6;

SELECT '=== 4.2 bpchar_enc(3) ''ab   ''（5 字符，超出全是空格）-> 存成 3 字符 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (7, 'ab   ');

SELECT '=== 4.3 bpchar_enc(3) ''ab  d''（超限位是非空格）-> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (8, 'ab  d');

-- ============================================================
-- 5. 多字节按字符计数（按字节计数会把 2 个汉字误判为超长）
-- ============================================================
SELECT '=== 5.1 varchar_enc(5) ''中文''（2 字符 / 6 字节）-> 放行 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (9, '中文');

SELECT '=== 5.2 varchar_enc(5) ''中文字符串''（5 字符 / 15 字节）-> 放行 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (10, '中文字符串');

SELECT '=== 5.3 varchar_enc(5) ''中文字符串长''（6 字符）-> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (11, '中文字符串长');

SELECT '=== 5.4 bpchar_enc(3) ''中文''（2 字符 / 6 字节）-> 放行 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (12, '中文');

-- ============================================================
-- 6. text_enc 无长度限制：比其它列都长的值照常写入
-- ============================================================
SELECT '=== 6 text_enc 无 typmod，长值放行 ===' AS stage;
INSERT INTO ct_t(id, t) VALUES (13, 'abcdefghijklmnopqrst');

-- ============================================================
-- 7. WHERE 超长常量必须不报错（与服务端一致：返回空集）
-- ============================================================
SELECT '=== 7 WHERE 超长常量 -> 0 行，不报错 ===' AS stage;
SELECT count(*) AS v5_match FROM ct_t WHERE v5 = 'abcdefghijklmnopqrst';

SELECT '=== 7b 超大 WHERE 常量同样不报错 ===' AS stage;
SELECT count(*) AS b3_match FROM ct_t WHERE b3 = 'abcdefghijklmnopqrst';

-- ============================================================
-- 8. UPDATE 同样生效
-- ============================================================
SELECT '=== 8.1 UPDATE 超长 -> 拒绝 ===' AS stage;
UPDATE ct_t SET v5 = 'abcdef' WHERE id = 1;

SELECT '=== 8.2 UPDATE 合法 -> 成功 ===' AS stage;
UPDATE ct_t SET v5 = 'ok' WHERE id = 1;

-- ============================================================
-- 9. 数值精度校验不回退（既有行为）
-- ============================================================
SELECT '=== 9.1 numeric_enc(5,2) 123.45 -> 放行 ===' AS stage;
INSERT INTO ct_t(id, n52) VALUES (14, 123.45);

SELECT '=== 9.2 numeric_enc(5,2) 1234.56（超出精度）-> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, n52) VALUES (15, 1234.56);

-- ============================================================
-- 10. 合法值仍可正常解密回读
-- ============================================================
SELECT '=== 10 回读 ===' AS stage;
SELECT id, v5, b3, n52, t FROM ct_t ORDER BY id;

-- ============================================================
-- 11. EXECUTE 参数路径同样受约束
-- ============================================================
SELECT '=== 11 EXECUTE 参数：超长拒绝 / 合法放行 ===' AS stage;
PREPARE ct_p AS INSERT INTO ct_t(id, v5) VALUES ($1, $2);
EXECUTE ct_p(16, 'abcdef');
EXECUTE ct_p(17, 'ok17');
DEALLOCATE ct_p;

-- ============================================================
-- 12. 历史数据（修复前写入的超长值）在读取侧同样按声明长度定型
--     造法：用「值自带 ENCRYPTED WITH」的形态写入——该路径目前不做长度校验
--     （等价于修复前的版本写入；它本身是另一处未修缺口，见 plan 文档）。
--     注意比较语义见用例头的「第 12 节须知」：回显已定型，比较仍看实际密文。
-- ============================================================
SELECT '=== 12.1 造存量数据：宽声明列(20/20) 拷进窄声明列(5/3) ===' AS stage;
DROP TABLE IF EXISTS ct_legacy CASCADE;
DROP TABLE IF EXISTS ct_wide CASCADE;
CREATE TABLE ct_legacy(id int,
    v5 varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    b3 bpchar_enc(3)  ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
);
CREATE TABLE ct_wide(id int,
    w20  varchar_enc(20) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
    wb20 bpchar_enc(20)  ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
);
INSERT INTO ct_legacy(id) VALUES (1), (2);
INSERT INTO ct_wide VALUES (1, 'ab      ', 'ab   ');
INSERT INTO ct_wide VALUES (2, 'abcdefghij', 'wxyz');

-- 造法变更（2026-09-22）：原先用「窄列 = 宽列」的密文拷贝造这批数据，理由是前端当时
-- 不做长度校验；现在前端按声明长度保守拒绝这条路径（见第 14 节），所以要造出"修复前
-- 版本写入的存量数据"，只能像旧版本那样绕开前端——关掉客户端加密，让服务端直接搬密文。
SELECT '=== 12.2 造存量数据：绕过前端（关闭客户端加密）让服务端直接拷密文 ===' AS stage;
\! PGENABLETECC=0 psql -X -A -t -c "UPDATE ct_legacy l SET v5 = w.w20, b3 = w.wb20 FROM ct_wide w WHERE w.id = l.id" 2>&1

SELECT '=== 12.3 回读：尾部空格形态定型成 5 字符 / 3 字符（''|'' 之后即尾部空格），含非空格的超长值不截断 ===' AS stage;
SELECT id, v5, b3 FROM ct_legacy ORDER BY id;

SELECT '=== 12.4 重写一遍即收敛：用回读值重新写入，回显与比较都回到原生语义 ===' AS stage;
DROP TABLE IF EXISTS ct_rewrite CASCADE;
CREATE TABLE ct_rewrite(id int,
    v5 varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
);
INSERT INTO ct_rewrite VALUES (1, 'ab   ');
SELECT id, v5 = 'ab   ' AS eq_native5, v5 = 'ab      ' AS eq_old8 FROM ct_rewrite;

DROP TABLE IF EXISTS ct_legacy CASCADE;
DROP TABLE IF EXISTS ct_wide CASCADE;
DROP TABLE IF EXISTS ct_rewrite CASCADE;

-- ============================================================
-- 13. 带长度的密态 cast：cast 声明的长度必须生效（Bug 9327 残留缺口）
--     原生对照见用例头「第 13 节须知」。判定用等值断言（截断后是哪个值），
--     不依赖尾部空格的呈现。
-- ============================================================
SELECT '=== 13.1 ::varchar_enc(5) 10 字符 -> 截断成 5 字符（显式 cast 不报错）===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (20,
    'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
SELECT id, v5, v5 = 'abcde' AS eq5, v5 = 'abcdefghij' AS eq10 FROM ct_t WHERE id = 20;

SELECT '=== 13.2 ::varchar_enc(20) 10 字符 写进 varchar_enc(5) -> 拒绝（赋值转换）===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (21,
    'abcdefghij'::varchar_enc(20) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));

SELECT '=== 13.3 ::varchar_enc 未声明长度 10 字符 写进 varchar_enc(5) -> 拒绝 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (22,
    'abcdefghij'::varchar_enc ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));

SELECT '=== 13.4 ::bpchar_enc(3) 4 字符 -> 截断成 3 字符 ===' AS stage;
INSERT INTO ct_t(id, b3) VALUES (23,
    'abcd'::bpchar_enc(3) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
SELECT id, b3, b3 = 'abc' AS eq3, b3 = 'abcd' AS eq4 FROM ct_t WHERE id = 23;

SELECT '=== 13.5 多行 VALUES：第二行超出目标列 -> 整条语句拒绝 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES
    (24, 'abcde'::varchar_enc(5)      ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')),
    (25, 'klmnopqrst'::varchar_enc(20) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));

SELECT '=== 13.6 多行 VALUES：都按 cast 声明长度截断 -> 放行 ===' AS stage;
INSERT INTO ct_t(id, v5) VALUES
    (26, 'abcde'::varchar_enc(5)      ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')),
    (27, 'fghijklmno'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
SELECT id, v5, v5 = 'abcde' AS eq5a, v5 = 'fghij' AS eq5b
FROM ct_t WHERE id IN (26, 27) ORDER BY id;

SELECT '=== 13.7 INSERT ... SELECT 的带长度 cast -> 同样截断 ===' AS stage;
INSERT INTO ct_t(id, v5) SELECT 28,
    'uvwxyzabcd'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det');
SELECT id, v5, v5 = 'uvwxy' AS eq5 FROM ct_t WHERE id = 28;

SELECT '=== 13.8 纯 SELECT 的带长度 cast -> 按声明长度截断输出 ===' AS stage;
SELECT 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det') AS v;

-- 比较两侧都走【列元数据】路径时，断言与集群排序规则无关：id=30 用字面量写入
-- （列自身路径），比较值用带长度 cast（WHERE 路径按列元数据加密）。截断生效则命中 1，
-- 不截断（按 10 字符比较）则 0。第二个断言是对照——裸字面量不做 cast 截断，
-- 超长常量比较不命中（第 7 节的既有语义）。
SELECT '=== 13.9 WHERE 带长度 cast -> 按截断后的值命中（不是按完整值）===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (30, 'abcde');
SELECT count(*) AS hit_truncated FROM ct_t
WHERE id = 30 AND v5 = 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det');
SELECT count(*) AS hit_bare_literal FROM ct_t
WHERE id = 30 AND v5 = 'abcdefghij';

SELECT '=== 13.10 多字节按字符截断（::varchar_enc(3) 5 个汉字）===' AS stage;
INSERT INTO ct_t(id, v5) VALUES (29,
    '中文字符串'::varchar_enc(3) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
SELECT id, v5, v5 = '中文字' AS eq3 FROM ct_t WHERE id = 29;

-- 本节断言三件事：列按 cast 声明长度建、值按显式 cast 语义截断、且**密文与列自身
-- 路径一致**（v = 'abc' 命中、v = 完整值 不命中）。
-- 修复前 CTAS 的密文与列路径不一致：CTAS 在客户端加密时新表还不存在，调度器按
-- 表列取不到排序规则（密态类型 varchar_enc/bpchar_enc/text_enc 的 typcollation 是
-- DEFAULT_COLLATION_OID，服务端登记该列元数据时解析成库默认排序规则），于是 DET
-- 不算 ICU 排序键；本用例所在集群是 zh_CN.UTF-8（非 C/POSIX），等值比较却按列元数据
-- 的排序规则算 ICU 排序键，两边密文不相等 —— 等值查询静默查不到。
-- 修复：加密时目标列还不存在 → 显式解析"该列将继承的排序规则"（库默认排序规则的
-- collcollate）交给加密入口，与列自身写入/比较路径一致。同一机制覆盖纯 SELECT 的
-- cast 与物化视图/视图定义里的 cast；加密列 DEFAULT 由 default_value_equality.sql 覆盖。
SELECT '=== 13.11 CTAS 的带长度 cast：列按声明长度建、值被截断、密文与列路径一致 ===' AS stage;
DROP TABLE IF EXISTS ct_ctas CASCADE;
CREATE TABLE ct_ctas AS
    SELECT 'abcdefghij'::varchar_enc(3) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det') AS v;
SELECT format_type(atttypid, atttypmod) AS decl_type
FROM pg_attribute WHERE attrelid = 'ct_ctas'::regclass AND attnum > 0;
SELECT v, v = 'abc' AS eq3, v = 'abcdefghij' AS eq10 FROM ct_ctas;

-- CTAS 的结果表列由 cast 的 ENCRYPTED WITH 派生，算法必须与同一份声明在普通写入
-- 路径上解析出的一致。修复前 CTAS 的加密调用写死"只支持 DET"：'det,search' 的列
-- 在 CTAS 建出后缺少 SEARCH 密文，之后该列上的 LIKE 会报
-- "cannot perform LIKE matching on encrypted column without SEARCH ciphertext"，
-- 而同一列若用普通 INSERT 写入则正常。下面两小节是同一声明在两条路径上的对照。
SELECT '=== 13.12 CTAS 尊重显式 SUPPORT_ALGO：DET 等值 + SEARCH 模糊匹配 ===' AS stage;
DROP TABLE IF EXISTS ct_ctas_sup CASCADE;
CREATE TABLE ct_ctas_sup AS
    SELECT 'hello'::text_enc ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det,search') AS v;
SELECT v, v = 'hello' AS eq FROM ct_ctas_sup;
SELECT v FROM ct_ctas_sup WHERE v LIKE '%ell%';

SELECT '=== 13.12b 对照：同一份声明的普通列（应完全相同）===' AS stage;
DROP TABLE IF EXISTS ct_plain_sup CASCADE;
CREATE TABLE ct_plain_sup (v text_enc ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det,search'));
INSERT INTO ct_plain_sup VALUES ('hello');
SELECT v, v = 'hello' AS eq FROM ct_plain_sup;
SELECT v FROM ct_plain_sup WHERE v LIKE '%ell%';

-- 加密列的 DEFAULT 值由客户端在 CREATE TABLE 时加密，长度校验必须在那一刻做：
-- 服务端只拿到密文，无从判断明文长度，不校验就是静默接受超长默认值。
-- 报错文本与服务端赋值转换一致（值超长在服务端是 INSERT 时才报，本路径提前到
-- CREATE TABLE，因为密文就在这里产生）。
SELECT '=== 13.13 加密列 DEFAULT 超长 -> 按声明长度拒绝 ===' AS stage;
DROP TABLE IF EXISTS ct_dflt CASCADE;
CREATE TABLE ct_dflt (
    a varchar_enc(3) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det') DEFAULT 'abcdefghij'
);

SELECT '=== 13.13b 对照：普通 varchar(3) 的同一 DEFAULT 在 CREATE TABLE 不报错，INSERT 才报 ===' AS stage;
DROP TABLE IF EXISTS ct_plain_dflt CASCADE;
CREATE TABLE ct_plain_dflt (a varchar(3) DEFAULT 'abcdefghij');
INSERT INTO ct_plain_dflt DEFAULT VALUES;

SELECT '=== 13.13c 恰好长度的加密 DEFAULT -> 建表可用，且 DEFAULT 行等值命中 ===' AS stage;
DROP TABLE IF EXISTS ct_dflt CASCADE;
CREATE TABLE ct_dflt (
    a varchar_enc(3) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det') DEFAULT 'abc'
);
INSERT INTO ct_dflt DEFAULT VALUES;
SELECT a, a = 'abc' AS eq FROM ct_dflt;

-- UPDATE 的 SET 原先只认常量与一元负号：带 ENCRYPTED WITH 的 cast 既不是常量、也没落进
-- 其它分支，于是既不加密也不报错，原样发往服务端，服务端按密态类型输入函数解析明文报
-- invalid hex string。现在按与 INSERT 的 cast 路径一致的语义处理：先按 cast 声明长度
-- 静默截断，再按目标列的声明长度做赋值语义校验。
SELECT '=== 13.14 UPDATE SET 的带 ENCRYPTED WITH cast：cast 长度截断 + 目标列长度拒绝 ===' AS stage;
UPDATE ct_t SET v5 = 'abcdefghij'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
WHERE id = 30;
SELECT id, v5, v5 = 'abcde' AS eq5, v5 = 'abcdefghij' AS eq10 FROM ct_t WHERE id = 30;

SELECT '=== 13.14b cast 声明 20、值 10 字符写入 varchar_enc(5) -> 按目标列长度拒绝 ===' AS stage;
UPDATE ct_t SET v5 = 'abcdefghij'::varchar_enc(20) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
WHERE id = 30;

-- 同一句里既有带 ENCRYPTED WITH 的 cast，又有写进另一个加密列的普通常量：
-- 两者必须都被加密（原实现会整体跳过常量的加密，语句报 invalid hex string）。
SELECT '=== 13.14c 同一句混合 cast 与普通常量 -> 两者都加密 ===' AS stage;
UPDATE ct_t SET v5 = 'vwxyz'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'),
                v100 = 'qwerty'
WHERE id = 30;
SELECT id, v5, v5 = 'vwxyz' AS eq5, v100, v100 = 'qwerty' AS eq100
FROM ct_t WHERE id = 30;

-- 同一句里带 ENCRYPTED WITH 的 cast 与写进另一个加密列的普通常量：UPDATE 形态见
-- 13.14c，下面的 WHERE / 纯 SELECT 形态是同一根因的另一面——原实现里这条 cast 自己
-- 拼一份改写结果（rewritten_query），而 WHERE 的改写会覆盖它，于是 cast 保持明文发往
-- 服务端报 invalid hex string；现在 cast 统一登记成 is_sql_expression 的 raw value，
-- 与同句的普通常量一次从左到右改写完成。
-- 前置：13.14c 已把 id=30 写成 v5='vwxyz'、v100='qwerty'（本节沿用该状态）。
SELECT '=== 13.15 WHERE 同句混合：cast 比较值与裸常量比较值都要加密 ===' AS stage;
SELECT count(*) AS hit_cast FROM ct_t
WHERE id = 30 AND v5 = 'vwxyz'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det');
SELECT count(*) AS hit_mix FROM ct_t
WHERE id = 30 AND v5 = 'vwxyz'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det')
  AND v100 = 'qwerty';

-- 纯 SELECT（无目标表）的 cast 与 WHERE 同句：cast 仍须被加密，且密文与列自身写入
-- 口径一致（同一列上的等值断言，与集群排序规则无关）。
SELECT '=== 13.16 纯 SELECT 的 cast 与 WHERE 同句 -> 仍被加密、与列密文一致 ===' AS stage;
SELECT 'vwxyz'::varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det') = v5 AS eq
FROM ct_t WHERE id = 30;

-- ============================================================
-- 14. 密文拷贝的声明长度校验（保守版）
--     SET 密态列 = 另一个密态列的赋值由服务端搬密文完成，前端拿不到源值的明文长度
--     （长度只存在于密文里，服务端也没有解密能力、密态类型的 typmod 校验是空操作），
--     因此按【声明长度】判断：源列声明宽于目标列时无法保证值放得下，直接拒绝。
--     代价：原生允许的"值恰好放得下"也会被拒（14.3 固化这一差异，原生对照见 14.4）。
-- ============================================================
SELECT '=== 14.1 源列声明 20 > 目标列声明 5 -> 拒绝（值实际超长）===' AS stage;
DROP TABLE IF EXISTS ct_c14 CASCADE;
DROP TABLE IF EXISTS ct_c20 CASCADE;
CREATE TABLE ct_c14(id int,
    a varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
CREATE TABLE ct_c20(id int,
    w varchar_enc(20) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
INSERT INTO ct_c14 VALUES (1, 'abcde');
INSERT INTO ct_c20 VALUES (1, 'abcdefghij'), (2, 'abc');
UPDATE ct_c14 n SET a = w.w FROM ct_c20 w WHERE w.id = 1;
SELECT id, a, a = 'abcdefghij' AS eq10, a = 'abcde' AS eq5 FROM ct_c14 WHERE id = 1;

SELECT '=== 14.2 源列声明不超过目标列 -> 放行（写入侧已校验过该列的值）===' AS stage;
DROP TABLE IF EXISTS ct_c5 CASCADE;
CREATE TABLE ct_c5(id int,
    s varchar_enc(5) ENCRYPTED WITH (DEK = 'ct_d', SUPPORT_ALGO = 'det'));
INSERT INTO ct_c5 VALUES (1, 'vwxyz');
UPDATE ct_c14 n SET a = s.s FROM ct_c5 s WHERE n.id = 1 AND s.id = 1;
SELECT id, a, a = 'vwxyz' AS eq FROM ct_c14 WHERE id = 1;

SELECT '=== 14.3 已知代价：源列声明更宽、值其实放得下，也一并被拒（与原生不同）===' AS stage;
UPDATE ct_c14 n SET a = w.w FROM ct_c20 w WHERE w.id = 2;

SELECT '=== 14.4 原生对照：同一形态下原生按值判断，放得下的放行、超长的拒绝 ===' AS stage;
DROP TABLE IF EXISTS np_c5 CASCADE;
DROP TABLE IF EXISTS np_c20 CASCADE;
CREATE TABLE np_c5 (id int, a varchar(5));
CREATE TABLE np_c20(id int, w varchar(20));
INSERT INTO np_c5 VALUES (1, 'abcde');
INSERT INTO np_c20 VALUES (1, 'abcdefghij'), (2, 'abc');
UPDATE np_c5 n SET a = w.w FROM np_c20 w WHERE w.id = 1;
SELECT id, a FROM np_c5 WHERE id = 1;
UPDATE np_c5 n SET a = w.w FROM np_c20 w WHERE w.id = 2;
SELECT id, a FROM np_c5 WHERE id = 2;

DROP TABLE IF EXISTS ct_c14 CASCADE;
DROP TABLE IF EXISTS ct_c20 CASCADE;
DROP TABLE IF EXISTS ct_c5 CASCADE;
DROP TABLE IF EXISTS np_c5 CASCADE;
DROP TABLE IF EXISTS np_c20 CASCADE;

-- 清理
DROP TABLE IF EXISTS ct_dflt CASCADE;
DROP TABLE IF EXISTS ct_plain_dflt CASCADE;
DROP TABLE IF EXISTS ct_ctas_sup CASCADE;
DROP TABLE IF EXISTS ct_plain_sup CASCADE;
DROP TABLE IF EXISTS ct_ctas CASCADE;
DROP TABLE IF EXISTS ct_t CASCADE;
DROP DEK IF EXISTS ct_d CASCADE;
DROP KEK IF EXISTS ct_k CASCADE;
\! rm -f "$TECC_KEK_PATH/ct_k.key" 2>/dev/null
