# JDBC 与 libpq 同用例输出差异台账

> 生成日期：2026-09-18
> 对应改动：JDBC 回归改用**单范式 + JDBC 自有期望**（见 `t/002_tecc_jdbc_test.pl` 头部说明）
> 用途：逐用例登记 JDBC（`TeccSqlTestRunner --output-format psql`）与 libpq（`psql -X`，
> 由 `shared/expected_common/` 维护）在**同一 SQL 用例**上的输出差异，并给出判定。
> 判定为"固有/工具层"的差异是 golden 文件的正常组成部分；判定为 **C** 的条目是
> **待确认的跨驱动真实差异**，需要 JDBC 侧跟进，未在本轮修改 JDBC 代码。

---

## 一、结论摘要

| 类别 | 数量 | 说明 |
|---|---|---|
| 仅 A/B 类（驱动与工具固有差异） | 22 个用例 | JDBC 输出已作为 golden 落库，回归可稳定检出后续变化 |
| 含 C 类（跨驱动真实差异） | 12 个用例 | 逐条列在 §四，建议按优先级跟进；**C-13 已于 2026-09-21 两侧闭环**（libpq 修 schema/别名传播，JDBC 修表达式结果列的源列登记），该用例从 C 类移除 |
| 环境依赖（E） | 1 个用例 | 原 hsmkms 用例已合并为 swxa 版并按无密码机时的跳过规则处理 |
| 框架无法执行（工具缺陷） | 1 个用例 | `reconnect_schema_dek`，已按 JDBC 侧跳过规则处理 |

> 统计口径含 §八 批次 1 迁移进来的 12 个用例（每例一份 JDBC golden）。
> 2026-09-20 晚更新：`jdbc/` 目录解散，两个全覆盖用例迁入 `shared/`（`localkms_enc_full_coverage`、
> `swxa_enc_full_coverage`，各含一条 C 类差异 → C-12 / C-13），`test_only_insert` 退役
> （只有一张无加密列的普通表，测不到密态能力）。归档见 `archive/jdbc_dir_retired_20260920/`。

回归现状：`make check-tecc-jdbc`（SoftSDF）→ **67/67 通过**（1 个用例显式跳过并打印原因）；
无密码机（`TECC_SDF_LIB=off` 且真机不可达）→ **60/60**，7 个 shared 硬加密用例整批不执行并打印一行说明。
（`view_order_by_ope` 用例见 §三 尾注：它的期望文件由本轮补齐，其视图 ORDER BY 校验的
两侧失败点一致、仅文案不同，属已登记的文案差异类。）

---

## 二、差异分类词表

| 标签 | 含义 |
|---|---|
| A1 | 缺行数页脚：psql `-X` 输出 `(N row)`/`(N rows)`，runner 不输出 |
| A2 | NOTICE/ERROR 的**位置与前缀**差异：libpq 把 psql stderr 追加到整份输出末尾并归一化 `psql:/...:LINE:` 前缀；runner 内联打印，且常出现 `ERROR: ERROR:` 双前缀 |
| A3 | 命令 tag 文本差异：libpq 打服务器 tag（`COPY 3`、`TRUNCATE TABLE`、`ANALYZE`）；runner 用**改写后 SQL 的前两个大写词**伪造（`COPY TEST_COPY_ENC`、`TRUNCATE TEST_COPY_ENC`、`ANALYZE ANA_DET_T`、`CREATE TEMP`、`WITH INS_CTE`） |
| A4 | 值格式差异：布尔 `true/false`(JDBC) vs `t/f`(psql)；浮点 `1.0E10` vs `1e+10`、`-1.0E-10` vs `-1e-10`；NULL 显示；bpchar 尾部空格/列宽填充；EXPLAIN 行前导缩进 |
| B1 | psql 客户端元命令产能差异：runner 只实现 `\c ` 和 `\copy `（`TeccSqlTestRunner.java:2415`），`\echo`(201 处)/`\pset`(4)/`\set`(7) 静默丢弃 |
| B2 | COPY TO STDOUT 数据行未被 runner 捕获 |
| C | 其它/真差异：SQL 语义、报错、结果集、语句成败不同——**需 JDBC 侧确认** |
| E | 环境依赖（无密码机 / vendor 库缺失） |

---

## 三、逐用例台账

规模列为「JDBC 行数 / libpq 行数 / diff 行数」；C 列的编号见 §四。

| 用例 | 规模 | 主要构成 | 判定 |
|---|---|---|---|
| analyze_search_col | 22/27/17 | A1×5, A2×6, A3×4 | 仅 A/B |
| bpchar_compare | 229/275/150 | A1×46, A2×8, A4×96 | 仅 A/B |
| bpchar_trailing_space | 212/357/433 | A1×145, A2×18, A3×2, A4×266 | **含 C-2** |
| cache | 107/157/126 | A1×33, A2×63, A3×18, A4×4, B1×4 | 仅 A/B（C-9 文案） |
| collation | 163/212/107 | A1×32, A2×75 | 仅 A/B |
| copy_test | 34/36/22 | A1×3, A2×8, A3×7, B2×2 | **含 C-10**（另有 splitter 缺陷） |
| crypto_verify | 239/291/156 | A1×54, A2×36, A3×1, A4×60 | **含 C-3** |
| det | 140/192/102 | A1×54, A2×14, A4×26 | **含 C-2** |
| det_distinct | 60/81/39 | A1×24, A2×13, A3×2 | 仅 A/B |
| enc_indexes | 93/118/55 | A1×25, A2×20, A4×10 | 仅 A/B |
| enc_types | 155/254/189 | A1×69, A2×98, A3×2, A4×2 | **含 C-4** |
| express | 251/340/107 | A1×89, A2×12, A3×4, A4×2 | 仅 A/B |
| float_precision | 58/92/50 | A1×34, A2×8, A4×8 | 仅 A/B |
| grammar | 738/997/737 | A1×228, A2×~80, A3×16, A4×6 | **含 C-5（7 条）** |
| he_acl | 144/184/313 | A1, A2, A3, A4 | 仅 A/B（级联已由框架修复，见 §五-5） |
| he_dual_slot_scalar | 55/115/148 | A1×5, A2×8, B1×6（HE 错误文案） | **含 C-23**（2026-09-24 新增） |
| he_kek_privkey | 56/69/63 | A1×12, A2×54, B1×1 | **含 C-7** |
| homomorphic_enc | 634/903/799 | A1×267, A2×~56, A4×2 | 仅 A/B（级联已由框架修复） |
| keyword_colname | 27/45/22 | A1×6, A2×4, B1×12 | 仅 A/B |
| localkms_enc_full_coverage | 450/668/506 | A1–A4、B（未逐条计数） | **含 C-12**（2026-09-20 由 `jdbc/` 迁入 shared） |
| nested_subquery | 28/41/19 | A1×11, A2×8 | 仅 A/B |
| number_order | 181/238/75 | A1×57, A2×18 | 仅 A/B |
| ope | 615/742/211 | A1×122, A2×59, A3×2, A4×28 | 仅 A/B（C-9 文案 ×3） |
| partition_crud_test | 215/303/164 | A1×34, B1×63, A2×51, A4×2 | **含 C-8** |
| partition_stress_test | 156/225/87 | A1×29, B1×40, A2×16, A3×2 | 仅 A/B |
| partition_types_test | 205/278/167 | A1×19, B1×54, A2×94 | 仅 A/B |
| pbe | 163/213/294 | A1×36, A2×56, A3×189, A4×2 | **含 C-1（最重）** |
| query_boundary | 80/104/42 | A1×23, A2×19 | 仅 A/B（C-9 文案） |
| schema_qualified_dml | 37/51/22 | A1×14, A2×8 | 仅 A/B（另有 splitter 缺陷） |
| search | 308/355/390 | A1×47, A2×14, A4×144 | 仅 A/B |
| swxa_enc_full_coverage | 461/689/503 | A1–A4、B（未逐条计数） | **含 C-13**（2026-09-20 由 `jdbc/` 迁入 shared） |
| tecc_full_test_suite | 986/1194/1058 | A1×171, A2×~372, A3×60, A4×37 | **含 C-6（8 条）**（另有 splitter 缺陷） |
| temp_view_reject | 22/22/12 | A1×2, A2×8, A3×2 | 仅 A/B |
| update_delete | 73/95/30 | A1×21, A2×8 | **含 C-11** |
| view_order_by_ope | 38/40/28 | A1/A2 + 拦截文案（同一失败点） | 仅 A/B（C-9 文案，2026-09-20 补 goldens） |
| view_union | 24/27/3 | A1×3 | 仅 A/B |
| reconnect_schema_dek | — | 框架无法执行 | 已跳过（§五-7） |
| ~~test_jdbc_hsm_enc_full_coverage~~ | — | 已退役 | **TZWY 出范围**：已合并为 `swxa_enc_full_coverage.sql`（swxakms，2026-09-20 迁入 shared） |
| ~~test_only_insert~~ | 10/9 | A2×1（首次运行 NOTICE） | **退役**（2026-09-20）：只有一张无加密列的普通表，两个驱动都测不到密态能力 |

> 登记缺口：`shared/sql/swxa_{enc,search,ope,alter_kek,concurrent}.sql` 等 HSM 用例早于本台账
> 建立，未逐条登记（本轮只补了迁入的 `swxa_enc_full_coverage`），后续可择机补齐。

---

## 四、C 类明细（待确认的跨驱动真实差异）

按影响面排序。每条给出：位置 / libpq 侧 / JDBC 侧 / 性质。

### C-1 pbe —— EXECUTE 参数改写**双向不一致**（影响面最大）
| # | 位置 | libpq | JDBC | 性质 |
|---|---|---|---|---|
| 1 | `EXECUTE a1(1,'Alice',50000,'IT')`（参数 int4_enc） | `PREPARE` + 结果 `3a_insert_param\|1\|Alice\|50000` | `ERROR: parameter $3 of type integer cannot be coerced to the expected type int4_enc` → 表空，连带 5 行结果缺失 | 改写缺失→报错 |
| 2 | `EXECUTE c1(55000,1)` | `ERROR: parameter $1 ... int4_enc` | `EXECUTE C1(55000,` 成功 | 反向：libpq 报错而 JDBC 成功 |
| 3 | `EXECUTE d1(70000)` | 同上报错 | 成功，且结果 `3c_delete_param_cnt\|1` vs libpq `\|0` | 报错 + 结果不同 |
| 4 | `PREPARE d1 AS SELECT ... WHERE salary > 50000` | `ERROR: operator does not exist: int4_enc > integer` | `PREPARE D1` 成功 | 报错不同 |
| 5 | §3g/3h、§4 S1（bpchar_enc 参数） | 返回结果 | `parameter $3 ... int4_enc` / `invalid hex string: "CODE001"` | 报错不同 |

> 两侧"成功集合"互补，说明 EXECUTE 参数映射与值加密的**触发条件**在两个驱动上不一致；不是单纯文案差异。

> **2026-09-21 补充（libpq 侧定位 + 修复）**：libpq 参数路径的根因定位到
> `tecc_processor.c` 的 `tecc_processor_encrypt_execute_params()`——参数值**按原文切片、连同 SQL 单引号一起入密文**，
> 且加密入口用"全算法" `tecc_unified_encrypt_string()`，而非字面量路径的 `tecc_unified_encrypt_for_where()`。
> 由此 libpq 侧还存在本表未覆盖的形态：**参数与字面量混用 → 静默 0 行**（不报错、返回空集），
> 且 `guc_string_parsing` 金标曾把带引号的错误值固化为期望（`val = 'exec_off_val'`）。
> **该缺陷已修复**（`8adf6683076` 参数字面量按值加密 + `2de3c2fbbaf` 显式类型声明按列加密，2026-09-21 独立复核通过；
> 金标 `expected_libpq/{guc_string_parsing,pbe}.out` 已同步）。
> **仍未修**：**CTE + 参数**形态（`WITH a AS (…) … FROM a WHERE a.c_det = $1` 四种写法均报 `invalid hex string`，
> 且无用例覆盖）——见 `plan/2026-09-19-回归遗留问题登记-四项待办.md` §13.3 与
> `plan/2026-09-20-批次6-libpq侧缺口差异说明.md` §10.4。

### C-2 密态 bpchar 尾部空格比较语义不一致
- `bpchar_trailing_space.sql` `D12_bp_empty_eq_vc_sp_ope`（OPE 密态 bpchar `''` = OPE 密态 varchar `' '`）：libpq `f` / JDBC `true`
- `det.sql:127` `2e_bpchar_space_enc`（密态 bpchar 与 `'hello '` 比较）：libpq `t|f` / JDBC `true|true`
- 明文对照组（`2d`、`D10/D11/D12_plain`）两侧一致 → 差异只在**密态路径**（space_count/填充语义）

### C-3 crypto_verify —— 非法值被 JDBC 接受
- `crypto_verify.sql:202` `INSERT INTO tb_int4 VALUES (12.34,'float')`
- libpq：`invalid input syntax for type integer: "12.34"`（不插入）
- JDBC：`INSERT 0 1` 且结果含 `12|float`（**未做整数语法校验**）

### C-4 enc_types —— DEFAULT 加密列 INSERT：libpq 侧报错，JDBC 侧接受（**libpq 侧已修复，2026-09-21**）
- `enc_types.sql:99` / `:108` / `:109`
- 修复前 libpq：`column "val" is of type int4_enc but expression is of type integer` /
  `invalid hex string: "only_default"`；JDBC：`INSERT 0 1` + 结果 `1|200|row2`、`100|world`、`99|only_default`
- 这是 C-12 **同一个缺陷**（加密列带 DEFAULT 的表 + 未限定表名 → 会话级负缓存哨兵误固化），
  修 C-12 时一并解决：libpq 两侧现均正常插入并返回 `100|world`、`99|only_default`
- 注意本条**不是**"JDBC 校验更松"的接受类差异：libpq 报错本身即缺陷，修复后两侧一致

### C-5 grammar —— 7 条
1. Test 11 `WITH ins AS (INSERT INTO t_dml VALUES (5,50) RETURNING *) SELECT * FROM ins;`：libpq 该 CTE-INSERT **未生效**（后续结果无 `5|50`），JDBC 生效（3 处）
2. `:763/:764` `WHERE name SIMILAR TO 'hello%'`：libpq `WHERE clause processing failed`（无行），JDBC 返回 `1|hello`
3. §18h 4 条 ILIKE：libpq `operator does not exist: text_enc ~~* text_enc`，JDBC 正常返回（多 6 行）
4. `:863-866` `test_p0_expr_null`：libpq `INSERT 0 3` + `10/99/99`，JDBC `column "x" is of type int4_enc but expression is of type integer`
5. `:869` `WHERE (val,name) = ROW(100,'hello')`：libpq `1`，JDBC **无输出**
6. §15 Indirection 数组查询：libpq `1|{hello}`，JDBC **驱动异常** `Index 0 out of bounds for length 0`
7. §19e ALTER COLLATE：报错文案不同（libpq `cannot alter type of an encrypted column`）

### C-6 tecc_full_test_suite —— 8 条
1. §22.2：libpq `UPDATE 0`/`update_ok|0` vs JDBC `UPDATE 1`/`update_ok|1`
2. `:591` `INSERT INTO b_cast(...)`：libpq 失败（`numeric field overflow`）vs JDBC `INSERT 0 3`
3. `:614-616` `count(*)`/`count(v_det)`/`count(DISTINCT v_det)`：libpq 无输出 vs JDBC `10/9/8`
4. COPY/\copy 段（`:1131-1195`）：JDBC 多 12 行由**注释被当成语句执行**产生的 ERROR，缺 4×`DROP TABLE`、`TRUNCATE TABLE`、`restored|0`
5. §22.5/22.6：libpq `operator does not exist: int4_enc > integer`（无行）vs JDBC 报 OPE 不兼容 / 另一条**成功返回 `2`**
6. §23.1：libpq `CREATE KEK kek2`/`CREATE DEK dek2` **失败**（cache miss）vs JDBC 成功 + `INSERT 0 1`
7. 约 20 处客户端预处理报错**文案**不同（同触发）
8. `:1257` `INSERT INTO edge3_enc(v) SELECT v FROM edge3_plain`：libpq `processor detected encrypted columns but failed to produce rewritten query` vs JDBC `INSERT...SELECT into encrypted column "v" without explicit encryption is not allowed`

### C-7 he_kek_privkey —— 无私钥场景 JDBC 抛异常（健壮性）
- 场景：`\! mv` 把 KEK 私钥文件移走后查询密态列
- libpq：返回**密文文本**（`\x被隐藏的HEX输出`），流程继续
- JDBC：`ERROR: TECC: KEK(sm2) material load failed ...` + `[读取异常: Bad value for type long : \x被隐藏的HEX输出]`
  —— 即在无法解密时把密文按 `long` 解析抛异常，未降级为返回密文

### C-8 partition_crud_test —— 报错口径 + ON_ERROR_STOP 行为
- `:298` `WITH src AS (...) INSERT INTO t_text_main (...) SELECT ...`
  - libpq：`WITH clause (CTE) combined with INSERT...SELECT into encrypted table is not supported in TECC mode.` 且因 `\set ON_ERROR_STOP on` **中止后续语句**
  - JDBC：`INSERT...SELECT into encrypted column "payload" without explicit encryption is not allowed`，**继续执行**（B1：runner 不实现 `\set`）

### C-9 报错文案差异（语义相同，可统一但非缺陷）
- `ope`：`length()` 白名单拦截文案；GREATEST/LEAST 需 OPE 文案；`c_det > 100` 需 OPE 文案
- `cache`：无权限查加密列 → libpq `TECC ERROR: tecc_values_processor_...no key found` vs JDBC `no key found for column 'salary' — current user may lack DEK USAGE privilege`
- `query_boundary`：跨 DEK 比较文案
- `temp_view_reject`：`temporary views with encrypted columns`（libpq）vs `temporary encrypted tables`（JDBC）

### C-10 仅 JDBC 侧输出的 WARNING（C 客户端无对应）
- `copy_test`：`WARNING: TECC COPY OUT: subquery COPY TO STDOUT detected, TECC wrapping skipped.`
- `det` / `det_distinct` / `crypto_verify`：`WARNING: Encrypted column [...] in ORDER BY without OPE: ordering by ciphertext bytes...`
- `collation`：`WARNING: collationLocaleId is null for OPE text column null.null.null`

### C-11 update_delete —— RETURNING 语句缺命令 tag
- `UPDATE ... RETURNING id, salary`：psql 先打 `UPDATE 1` 再打结果；runner 只输出结果行（不输出 tag）

### C-12 localkms_enc_full_coverage —— DEFAULT 值表子场景：libpq 侧 4 条语句报错，JDBC 侧全过（**已修复，2026-09-21**）
（原 C-12 条目「test_jdbc_soft_enc_full_coverage」，用例 2026-09-20 迁入 shared 并改名）

- 位置：Part 3.8 两条给出加密列显式字面量的 INSERT、Part 4.17 的 `WHERE def_i4 = 100` 与
  `WHERE def_txt LIKE '%default%'`
- libpq 侧（修复前）：4 条全错（`column "def_i4" is of type int4_enc but expression is of type integer`、
  `invalid hex string: "..."`、`operator does not exist: int4_enc = integer`）
- JDBC 侧：**全部通过**（3 行都正确插入、默认值 `100/hello_default` 正确、WHERE/LIKE 返回正常）
- 判定：**libpq（C 客户端）侧缺陷**，非驱动固有差异
- **根因（2026-09-21 定位并修复）**：客户端加密列元数据负缓存哨兵被错误固化。
  `CREATE TABLE` 处理加密列 DEFAULT 值时需按名回查该列元数据，而此刻表尚未创建 →
  数据获取器返回"0 行" → 缓存层把"0 行"当成"该表存在且无加密列"，
  插入 `TECC_CACHE_STATUS_NOT_ENCRYPTED` 表级哨兵。该哨兵是**会话级**的，
  于是同会话后续对该表名的所有按名查找都在 `tecc_cache_get_encrypted_column_by_name`
  第 5 步短路返回 NULL → INSERT/WHERE 不再被改写 → 明文直送服务端。
  触发需同时满足"表有带 DEFAULT 的加密列"（才会在 CREATE 期按名回查）与
  "SQL 用未限定表名"（限定名查的是别的 key，哨兵不命中）——HSM 孪生用例正是
  因为用了限定名 `swxa_full_test.enc_default` 才没暴露，与密码机无关。
  修复：区分"关系不存在"与"关系存在但无加密列"，前者不再固化负缓存
  （`tecc_column_cache.c` / `tecc_data_fetcher_adapter.c` / `tecc_data_fetcher.c`）。
- `enc_types.sql`（C-4）是**同一缺陷**的另一个用例，金标同样固化着这 3 条 ERROR；两份金标
  已一并重生成，两侧驱动输出现已一致
- 台账：`plan/2026-09-20-加密列带DEFAULT的软加密表libpq入参加密失效.md`（已更新为已修复）
- 与 `plan/2026-09-20-批次6-libpq侧缺口差异说明.md` 的缺口 A/B/C 同属"libpq 侧字面量未加密、
  明文外发 → `invalid hex string`"家族；那条是否同因**尚未验证**，建议按同法排查

### C-13 swxa_enc_full_coverage —— HSM 下 OPE 列 min/max（**两侧均已修复：libpq 2026-09-21 / JDBC 2026-09-21**）
- 位置：Part 4.20 `SELECT min(ope_i4), max(ope_i4) FROM swxa_full_test.enc_ope`
- 修复前 libpq：正文无结果，stderr 报 `TECC result processing error: Batch decrypt: item 0 HSM not available`
- 修复前 JDBC：`[读取异常: Bad value for type int : \x...]|[读取异常: ...]`（驱动按 int 读失败后替换为标记文本）
- **libpq 侧根因**：聚合/表达式结果列在 PG 协议里没有 `table_oid`，客户端必须**按名**回查列元数据
  恢复；`tecc_collect_expr_target_column()`（`stmt_processor.c`）对未带 schema 的列引用**硬编码
  `"public"`**，于是非 public schema 的表（本用例用 `swxa_full_test`）查不到列元数据 →
  批量解密拿不到密钥材料。又因 `tecc_crypto_context_reset()` 未清 `hsm_kms_type`，
  上一条语句残留的 HSM 标记让失败形态表现为 "HSM not available"，掩盖了真实原因。
  修复：把 FROM 单表的 schema/别名一路传到列元数据回查；并补上 `hsm_kms_type` 复位。
  修复后 libpq 返回 `-500 | 500`。
- **触发条件与密码机无关**：是"表不在 public schema（或用别名、或用 `schema.table` 限定 FROM）"
  触发；`min(schema.table.col)` 限定列引用与 public schema 表本来就正常。软加密之所以"正常"，
  是因为该对拍用例把表建在 `public` —— 原台账"HSM 特有"的结论**是 schema 混淆导致的误判**。
- **JDBC 侧根因（独立，不是同一处代码）**：`tableOid==0` 的聚合结果列在 JDBC 里**从未做按名恢复**
  （`PgResultSetMetaData.java` 只在有 OID 时填 metadata），于是落到 `decryptColumnByDistOid`
  回退路径（`TeccResultDecryptor.java:1852` → `DefaultTeccJdbcService.java:3308`）——它能按
  dist_oid 找到 DEK，却把 HSM 列密文交给软件 RND 解析器，失败被吞掉、密文直达 `getInt`。
  与 public/非 public、别名、限定名**都无关**（JDBC 侧全部失败）。
  详见 `plan/2026-09-20-HSM下OPE列min_max聚合两侧表现不一致.md`
- **JDBC 侧修复（2026-09-21，已在 pgjdbc 仓落地并真机验证）**：按本台账建议的方向实施——
  1. **改写期登记**：新增通用表达式解析（`NestedSelectExpressionRewrite.resolveExprSourceColumn()`，
     递归覆盖函数参数 / 二元运算 / 带符号 / CAST / CASE，每项只取第一个命中加密列；`array_agg`、
     `ARRAY[]`、窗口函数、`SELECT *` 之后不登记；递归深度上限 16 超限即放弃），把表达式结果列的
     `(schema, table, column)` 登记进既有"结果列 → 源加密列"通道（该通道已按语义中性化更名为
     `TeccResultSourceMapping`）。
  2. **读取期类型门**：只有结果列**声明类型**属于密态标量区间（新增
     `TeccConnectionBootstrap.isEncScalarOid()`）才按源列解密——对齐 C 侧按 `PQftype` 判定的口径，
     并据此挡住 `COUNT(加密列)` 这类"登记了源列、但结果本身是明文类型（普通 int8）"的形态；
     **不需要**把映射分支改成"解密失败即降级"，明文结果根本不会进入该分支。
  3. **定型口径**：表达式结果列解密后按**结果列 OID/typmod** 定型（新增
     `DefaultTeccJdbcService.mapEncScalarOidToBaseOid()`），与 C 侧 `PQftype`/`PQfmod` 判据一致。
  **真机证据**（硬加密 SWXA 192.168.70.71:8008，`min/max(ope_i4)` 落在非 public schema 的表上）：
  修复前 `[读取异常: Bad value for type int : \x3001…]` → 修复后 **`-500 | 500`**；
  `greatest(int2_enc, int8_enc)` 由读取异常 → **`9`**；负例 `count(ope_i4)` 前后均为 `3`（无回归）。
  回归：`TeccBuiltinFunctionIntegrationTest` 25/25、`parser.*` + `expression.*` 1015/1015（0 failed / 0 skipped）；
  改名后合并重跑 1040/1040。
  详见 pgjdbc 仓 `pgjdbc/doc/401/bug修复方案/2026-09-21-A4-*`（实施方案 / 方案审查报告 / 实施记录 / 实施代码 REVIEW）。
- **已知跨驱动差异（本次新增登记）**：JDBC 侧覆盖面**宽于** libpq —— JSqlParser 把
  `COALESCE/GREATEST/LEAST` 解析为 `Function`，故 `COALESCE(加密列, 常量)`、`GREATEST(...)` 等
  也会被登记并解密；而 libpq 的 `tecc_collect_expr_target_column()` 的 `switch` **无 `CoalesceExpr`
  分支**（`default: break`），同一 SQL 在 libpq 侧可能保持密文。属**有意为之**（pgjdbc 既有
  `COALESCE(MIN(v), -1)` 明文断言依赖它），此处登记以免被误判为 JDBC 缺陷。
- 对照：软加密同查询两侧都正常（`-500|500`）；同用例其它 DET/OPE/SEARCH 查询两侧都有明文结果

---

## 五、框架/工具层问题（非驱动语义差异）

### 1. runner `splitStatements` 反斜杠不区分注释（**影响 6 个用例**）
`TeccSqlTestRunner.java:2287` 的切分逻辑遇到**任意反斜杠**就把缓冲作为独立语句提交，未跟踪 `--` 行注释 / `/* */` 块注释。于是注释文本被伪造成语句：

- `reconnect_schema_dek.sql` 头部注释含 `\c 重连(PQfinish→tecc_state_cleanup)...` → 被当成重连命令，`parseConnectUser` 取到第 2 个词 `DEK` → `FATAL: role "DEK" does not exist` → **整个文件中止、零输出**
  （复现：用同一算法模拟切分，该文件产出 25 条"语句"，其中 6 条以 `\c` 开头，`user` 分别为 `DEK`/`<fallback>`/`DROP`/`触发旧连接`/`test_reconn`/`postgres`）
- `tecc_full_test_suite.sql` 注释含 `-- 原 \copy 命令: COPY ...` → 产生 12 行 `No such file or directory` ERROR
- 另涉及 `copy_test.sql`、`homomorphic_enc.sql`、`schema_qualified_dml.sql`、`swxa_concurrent.sql`（注释中含反斜杠的行数：13/1/1/1/1/5）

**变体（批次 1 迁移时新发现，`search_path_sync`）**：注释里的**分号**同样致命——
切分先按 `;` 断句，而 `stripComments` 只丢弃**以 `--` 开头**的片段，于是 `-- 5. …：BEGIN; SET; ROLLBACK;`
这类注释会产出真实语句（该用例产生 3 条虚假语句：`SET` 语法错误、`NOTICE: there is no transaction in progress`、
多一个 `SET SEARCH_PATH`），golden 被污染（语义无害，但每次比对都带着假行）。

**变体 2（2026-09-20 实测，迁 `localkms_enc_full_coverage` 时踩到）**：注释里的 **美元引用**
最致命——`findDollarQuoteTag` 把注释中的 `$$` 当成美元引用开始符（空 tag），此后**整个文件剩余
内容被吞成同一条语句**，直到再出现 `$$` 才闭合。当时的用例头注释里正好写了「无 `$$`」这个短语，
症状是：服务端一条 DDL 都没收到、runner 只报出其中一个 `CREATE DEK` 的客户端错误
（`TECC CREATE DEK ... referenced KEK ... does not exist in key manager`）——表象指向"KEK 没建"，
与真实原因（拆句吞文件）毫无关系，极易误判成 KEK/密钥文件问题。规避：别在注释里写 `$$`
（用"美元引用"文字描述）。**这条与 `\c`/分号两个变体同源：`splitStatements` 不识别注释。**

**建议**（JDBC 侧，一行级思路）：在切分循环里维护注释状态（`--` 行注释 + `/* */` 块注释），
或仅在**行首**（前导空白后）遇到 `\` 才作为元命令切分，且在切分前先整体移除注释。

### 2. runner 只实现 `\c` / `\copy`
`\echo`(201 处)、`\pset`(4)、`\set`(7)、`\!`(12) 被静默丢弃 →
- 本轮框架已**代 runner 执行 `\!`**（见 §六-6），he_kek_privkey 的"无私钥"场景因此恢复
- `\echo` 标题、`\pset null` 回显、`\set ON_ERROR_STOP` 语义仍是差异（B1）
- `\c` 的回显为 `\C POSTGRES`，psql 打印 `You are now connected to database "postgres" as user "...".`

### 3. 输出保真度（A1–A3）
行数页脚缺失、NOTICE 流位置、命令 tag 伪造。若要让 JDBC 输出与 psql 逐字节可比，需要 runner 侧改造（`TeccSqlTestRunner.java:3197-3221`、`:3267-3288`）。

### 4. COPY TO STDOUT 数据未捕获（B2，copy_test）

### 5. hsmkms 环境（E）
**已处置（2026-09-18，D1 决策）**：TZWY/hsmkms 整体出范围。原 `jdbc/sql/test_jdbc_hsm_enc_full_coverage.sql`（`key_store='hsmkms'`）
已合并为 `shared/sql/swxa_enc_full_coverage.sql`（`key_store='swxakms'`、`key_path='hsm:id=1'`，复用框架预置的 kek-1；2026-09-20 由 `jdbc/` 迁入 shared），
由无密码机时的跳过规则统一控制（`TECC_SDF_LIB=off` 且真实 swsds.ini 可达时才执行，否则不参与本轮执行）；
`shared/sql/tzhsm_enc.sql` 因与 `swxa_enc.sql` 首表逐列重复已归档到 `archive/hsm_tzwy_cases_20260918/`。
详见 `plan/softsdf-jdbc-hsm-适配方案.md`（v2）。

---

## 六、本轮已修的框架缺陷（C 侧，供回溯）

| # | 问题 | 修法 |
|---|---|---|
| 1 | pgjdbc 仓库 `gradlew` 无执行位（git 100644）→ `make check-tecc-jdbc` 第一步 Permission denied | `Makefile` 改用 `sh ./gradlew`；`JAVA_HOME_JDBC` 回退到 `JAVA_HOME`/从 PATH 反推 |
| 2 | JDBC 与 libpq 共用 `expected_common` 不可行（布尔/浮点/NULL/页脚/tag 固有差异） | 改为单范式 + JDBC 自有 `shared/expected_jdbc/`（32 份重建） |
| 3 | `%PGDATA%`/`%KEKPATH%` 未替换、CWD 不在 `tmp_check/tmp` → copy_test 报 `could not open file "%PGDATA%/..."` | 与 libpq 侧对齐：占位符替换 + 切 CWD |
| 4 | JUL 日志带本地化时间戳 → 输出不可比对 | runner 加 `-Djava.util.logging.SimpleFormatter.format=%4$s: %5$s%n` |
| 5 | SQL 自带的 `\! tecc_ktool generate` 被 runner 丢弃 → he_acl / he_kek_privkey / homomorphic_enc **整例级联失败** | `pregen_localkms_kek_files(..., 1)` 强制预生成（并对"已存在"容忍） |
| 6 | `\!` 语义（mv 私钥等）丢失 → he_kek_privkey "无私钥"场景不可测 | 框架按 `\!` 切段执行：段间代执行 shell 命令，输出按序拼接 |
| 7 | 期望文件缺失时与空串比较 → `reconnect_schema_dek` "空 == 空"**假通过** | 缺期望显式失败；归一化后无内容时落盘 `.raw` 原始输出并提示 |
| 8 | `DROP DEK` 依赖校验的 DETAIL 行（依赖列名 + 表 OID）非确定性 | `normalize_common_output` 归一化为 `Encrypted column "COL" in table OID OID` |

---

## 七、建议后续动作

1. **P0**：JDBC 侧修 `splitStatements` 注释态（§五-1）——修完 6 个用例的 golden 需重生成；`reconnect_schema_dek` 可解除跳过。
2. **P0**：`pbe` EXECUTE 参数改写双向不一致（C-1）——建议与 libpq 逐条对拍。
3. **P1**：密态 bpchar 空格比较（C-2）、非法值/ DEFAULT 加密列被接受（C-3/C-4）。
4. **P1**：无法解密时应降级返回密文而非抛 `Bad value for type long`（C-7）。
5. **P2**：报错文案统一（C-9）、RETURNING tag（C-11）、`\echo`/`\pset`/`\set` 与输出保真度（§五-2/3）。
6. **待决策**：HSM 适配（`plan/softsdf-jdbc-hsm-适配方案.md` 的 D1/D2/D3）。
7. ~~建议同步 C 仓库落后的 2 个提交~~ **已同步（rebase 至 origin，含 `357187bc4ca` 等）**：
   `pg_dump` 的 HE 失败已转绿；`tecc_admin_funcs` 仍失败，根因待查（见 §十二）。
8. **已修复（2026-09-21 复核）**：EXECUTE 参数连 SQL 引号一起入密文 → 与字面量/存储路径不匹配 →
   静默 0 行（C-1 的 libpq 侧根因）。由 `8adf6683076` + `2de3c2fbbaf` 修复，独立复核通过（详见
   `plan/2026-09-21-libpq-EXECUTE参数引号入密文与静默零行.md` §8）。
9. **待办（2026-09-21 复核，libpq 侧）**：批次 6 三项缺口中，CTAS 丢内层查询与多 CTE 已修复并复核通过；
   **仅"CTE + 参数"形态仍复现**（四种写法全失败，且无用例覆盖）。详见
   `plan/2026-09-19-回归遗留问题登记-四项待办.md` §13.3 与
   `plan/2026-09-20-批次6-libpq侧缺口差异说明.md` §9 + §10（含判据、代码定位、复现配方）。

---

## 八、libpq 独有用例迁移到 shared（批次 1，2026-09-18）

**动机**：JDBC 侧只覆盖 32 个 shared 用例（单范式），libpq 侧 104 个——差距主要在 `libpq/sql/` 的独有用例。

**核查口径**：一个用例只有能**同时**跑 psql 与 JVM runner 才算"驱动无关"。40 个 `libpq/sql` 用例逐个核查后分布如下（远小于最初按文件名粗估的 36）：

| 分类 | 数量 | 说明 |
|---|---|---|
| 可直搬 | **12** | SQL 零改动即可两侧运行 |
| 适配后可搬 | 11 | 仅需清注释中的反斜杠 / `\connect`→`\c` / 或仅需补期望文件 |
| 留 libpq-only | 16 | 断言依赖 C 前端专有校验、libpq 专有失败语义、客户端工具链，或 JDBC 缺对应能力 |
| 配对链（整链保留） | 1 | `kek_overwrite_1/2/3` 中 2、3 不可搬，整链必须一起留 libpq |

### 批次 1 已搬入 `shared/sql/` 的 12 个用例

`aggregate_func`、`alter_kek`、`cross_dek_error`、`dek_key_algo_sm4`、`det_join_plan`、`det_setop_plan`、
`empty_target_list`、`encrypted_view`、`innherits_encrypted`、`search_path_sync`、`temp_table_reject`、
`unqualified_table_schema`

- 每例新增 3 份期望：`shared/expected_libpq/<case>.out`（`psql -X` raw）、
  `shared/expected_common/<case>.out`（归一化）、`shared/expected_jdbc/<case>.out`（JDBC 自有）
- 原 `libpq/expected/<case>.out`（`-X -A -t` 形态，不可直接当 raw 期望）已移出，留存于 `/tmp/moved_libpq_expected/`
- 验证：**JDBC 46/46 全绿**；**libpq 115/116**（剩余 1 个为 `tecc_admin_funcs`，由 C 仓库落后 2 个提交导致，见 §七-7）
- 迁移未引入任何既有用例回归

### 批次 2（待批准）

**11 个"适配后可搬"**：
- 仅需清 `--` 注释中的反斜杠（触发 runner `splitStatements` 缺陷）：`guc_backslash`、`guc_string_parsing`
- 需把裸 `\connect` 改为 `\c postgres postgres`（runner 只认 `\c `）：`kek_dek_cross_schema`、`kek_dek_recreate_after_drop`、`he_rotate_keep_ciphertext`
- 仅需补三份期望（SQL 零改动）：`kek_dek_drop_owned`、`kek_upper_options`、`scalar_func`、
  `scalar_func_arg_combos`、`scalar_func_arg_combos2`、`scalar_func_crosstype`

### 16 个留 libpq-only 的原因分类

| 原因 | 用例 |
|---|---|
| C 前端专有校验（JDBC 无对应实现） | `kek_dup_check`、`kek_key_file_ext_check`、`dek_pubfile_gate` |
| libpq 专有失败语义（解密失败静默回退密文；JDBC 为 fail-closed，见 §四 C-7） | `kek_file_missing_silent`、`kek_overwrite_2_verify`、`kek_overwrite_3_restore` |
| 客户端工具链 / psql 元命令语义 | `pg_dump`、`psql_tecc_help`、`psql_tecc_mode`、`copy_extended`、`nested_batch_rewrite`、`kek_restore_missing_pub` |
| JDBC 侧缺对应能力（待补齐后才可能搬） | `like_escape`（JDBC 无 `ESCAPE` 入参）、`tecc_admin_funcs`（`\gset` + `hg_tecc_mode` 会话开关两驱动无通用写法）、`tecc_decrypt_raw_he`（JDBC 未实现前端 `tecc_decrypt_raw`）、`he_dek_recreate_after_drop`（C 前端进程级 HE 缓存 + `psql -k on`） |

---

## 九、批次 1 迁移用例的逐例台账（§八 的 12 个）

规模 = 「JDBC / libpq / diff」行数。

| 用例 | 规模 | 主要构成 | 判定 |
|---|---|---|---|
| aggregate_func | 263/381/362 | A1×89, A2×227, B1×25 | **含 C-13（4 条）** |
| alter_kek | 116/148/92 | A1×36, A2×17, A3×23, A4×2 | **含 C-14（3 条）** |
| cross_dek_error | 51/70/65 | A1×7, A2×56 | 仅 A/B（C-9 文案 ×14） |
| dek_key_algo_sm4 | 10/11/9 | A1×1, A2×6 | 仅 A/B（C-9 文案 ×1） |
| det_join_plan | 136/183/133 | A1×39, A2×62, A4×32 | 仅 A/B（C-9 文案；C-10 类 1 条） |
| det_setop_plan | 161/205/246 | A1×45, A2×107, A3×8 | **含 C-15（1 条，严重）** |
| empty_target_list | 32/38/36 | A1×12, A2×3 | **含 C-16（4 条）** |
| encrypted_view | 96/138/96 | A1×33, A2×11, A3×8 | **含 C-17（6 条）** |
| innherits_encrypted | 12/12/10 | A2×3 | 仅 A/B |
| search_path_sync | 59/61/60 | A1×1, A2×11, A3×13 | 仅 A/B（含 splitter 新变体，见 §五-1） |
| temp_table_reject | 19/21/18 | A1×2, A2×3, A3×2 | **含 C-18（2 条）** |
| unqualified_table_schema | 49/58/43 | A1×5, A2×8, A3×9 | 仅 A/B |

### 批次 1 新增 C 类明细

#### C-13 aggregate_func —— 聚合拦截口径不一致（4 条）
1. `sql:571/574/577/580/590/593/596`（7 条 `array_agg(密态列)`）：libpq **前端拦截**无结果行（`aggregate function "array_agg" is not supported on encrypted column "c_int2"`）／JDBC **成功**返回解密数组（`array_agg(c_int2)-OPE|{"100","-50","0",NULL,"200"}`）
2. `sql:640` WF-04 `array_agg(...) OVER (...)`：libpq 拦截（`inside window function is not supported`）／JDBC 返回 5 行
3. `sql:764` `COUNT(DISTINCT CASE WHEN c_int4 > 0 THEN c_text ELSE NULL END)`：libpq `operator does not exist: int4_enc > integer`／JDBC `COUNT(DISTINCT CASE)-OPE|2`
4. `sql:817` 子查询 array_agg：libpq 4 行 `{"ERROR"}`（前端占位）／JDBC 4 行 `{"\x<hex>"}`（未解密密文）

#### C-14 alter_kek —— 轮换链 3 处行为分叉
1. `sql:139` 信封长度断言：libpq `t`／JDBC `ERROR: TECC SQL parse failed (tecc.rewrite.fail_closed=true)`
2. `sql:228` 无 USAGE 角色 `ALTER KEK`：libpq `ERROR: permission denied for key encryption key kek_rotate`／JDBC `ERROR: TECC ALTER KEK: KEK (oid=16480) not found in database`（**权限不足被伪装成 KEK 不存在**）
3. `sql:236` GRANT USAGE 后 `SELECT id,name,age FROM rot_test`：libpq 明文 5 行（`1|Alice|30`…）／JDBC `1|\x被隐藏的HEX输出|0`…（**未解密，age 退化为 0**）

#### C-15 det_setop_plan —— EXPLAIN 计划被当成密文解密（严重）
`sql:95/128/129/132/155/175/182/186/199`（8 处 `EXPLAIN (COSTS OFF)`，UNION/INTERSECT/EXCEPT）：
libpq 输出正常计划共 86 行（`HashAggregate`／`Group Key: ds_t.a, ds_t.d`／`HashSetOp Intersect`）；
JDBC **计划行全部缺失**，被 86 行驱动异常替代：
`[读取异常: TECC: ciphertext too short or truncated for algorithm RND (payload len=13, min=72)]`
（`payload len` 恰等于各段计划文本长度 → 驱动把 EXPLAIN 结果行拿去解密）。**建议最高优先级跟进。**

#### C-16 empty_target_list —— 不支持空目标列表（4 条）

> **2026-09-21 补充（libpq 侧复核）**：libpq 侧"空目标列表"本身在**顶层 / 派生表 / CTE 三形态均正常**
> （已用服务端收到的 SQL 正向验证：三形态均改写为 `…'2500000002…'::text_enc`），与本条"libpq 正常 / JDBC 报错"一致。
> 但同一家族另有一个 **libpq 侧缺口**：`CREATE TABLE … AS SELECT … WHERE <密态列>='x'`（CTAS，**与目标列表是否为空无关**）
> 的常量不被改写 → 明文外发。**该缺口已修复**（`97cdeecf12f`，2026-09-21 独立复核通过，新用例
> `libpq/sql/ctas_select_where.sql`）；记录见 `plan/2026-09-20-批次6-libpq侧缺口差异说明.md` §9.2 + §10.3 与
> `plan/2026-09-19-回归遗留问题登记-四项待办.md` §13.1。
`sql:42/45/48/56`（`SELECT FROM t` 之类空目标列表）：
libpq 正常计数（`1a_empty_tlist_count|2` 等）／JDBC `ERROR: invalid hex string: "Alice"`、
`operator does not exist: int4_enc > integer`（密态常量未改写、明文下发）。
对照组 `2b/3a/3b` 两侧一致。

#### C-17 encrypted_view —— 多对象 DDL 与 SELECT INTO TEMP 不支持（6 条）
1. `sql:14` `DROP VIEW IF EXISTS v_enc, v_enc2, v_plain CASCADE`：libpq `DROP VIEW`／JDBC `ERROR: TECC SQL parse failed (tecc.rewrite.fail_closed=true)`
2. `sql:15` `DROP MATERIALIZED VIEW IF EXISTS ...`：同上
3. `sql:136` `SELECT id, name FROM mv_enc`：libpq `1|Alice`／JDBC 0 行
4. `sql:173` `SELECT oid INTO TEMP ev_multi_colid ...`：libpq `SELECT 1`／JDBC parse failed
5. `sql:178` `... WHERE column_id IN (SELECT oid FROM ev_multi_colid)`：libpq `0`／JDBC `relation "ev_multi_colid" does not exist`（第 4 条连带）
6. `sql:235` `DROP MATERIALIZED VIEW mv_enc, mv_enc2, mv_empty CASCADE`：同上

#### C-18 temp_table_reject —— 会话临时表未遮蔽同名加密表（2 条）
1. `sql:45` `INSERT INTO t3 VALUES (1, 10)`（会话 TEMP `t3` 应遮蔽加密 `public.t3`）：libpq `INSERT 0 1`／JDBC `ERROR: invalid input syntax for type integer: "\x被隐藏的HEX输出"`（未限定 `t3` 被解析到加密 `public.t3`，密文写入 int 列）
2. `sql:46` `SELECT * FROM t3 ORDER BY id`：libpq `1|10`／JDBC 0 行（连带）

### 批次 2 已搬入（11 个，2026-09-18）

`guc_backslash`、`guc_string_parsing`、`kek_dek_cross_schema`、`kek_dek_recreate_after_drop`、
`he_rotate_keep_ciphertext`、`kek_dek_drop_owned`、`kek_upper_options`、`scalar_func`、
`scalar_func_arg_combos`、`scalar_func_arg_combos2`、`scalar_func_crosstype`

搬移前做的适配（**只动注释与两处元命令**，SQL 语句语义不变）：
1. `--` 注释里的 ASCII 反斜杠一律改写为全角 `＼`（U+FF3C）——runner `splitStatements` 对注释中的反斜杠也会切句并伪造语句
   （涉及 `guc_backslash`、`guc_string_parsing`、`kek_dek_cross_schema`、`kek_dek_recreate_after_drop`、`he_rotate_keep_ciphertext`、`kek_dek_drop_owned`）
2. `--` 注释里的分号改写为全角 `；`——同一缺陷的 `;` 变体（`scalar_func_crosstype` 4 处）
3. 两处真实元命令 `\connect` → `\c postgres postgres`（`kek_dek_recreate_after_drop:65`、`he_rotate_keep_ciphertext:108`）

> **关于 `\c` 的定性**：`\c`/`\connect` 是 **psql 客户端元命令**，JDBC **驱动本身不支持任何元命令**。
> 之所以能搬，是因为 JDBC 侧的 SQL 是由测试工具 `TeccSqlTestRunner` 执行的，它**自己实现**了 `\c `
> （`TeccSqlTestRunner.java:681`：`parseConnectUser/parseConnectDatabase` + 关闭并重建 `Connection`），
> 而 `:2415` 只放行 `\c ` 与 `\copy `——裸 `\connect` 会被静默丢弃，故必须改写成带空格的 `\c `。
> 即：**这是测试工具的兼容能力，不是驱动能力**；对最终用户而言 JDBC 下没有 `\c` 这种东西。

**验证**：JDBC **57/57 全绿**（55 shared + 2 jdbc 专有）；libpq **125/127**
（2 个失败 = `pg_dump`、`tecc_admin_funcs`，均由 C 仓库落后 2 个提交导致，见 §七-7）。

### 观察：HE 乘法偶发失败（待 C 侧确认）

同一批用例暴露了一个**间歇性**现象，同一断言在不同运行间结果不同：

| 用例 | 断言点 | 现象 |
|---|---|---|
| `homomorphic_enc` | `sql:187` | 一次运行 `ERROR: homomorphic multiplication failed`（`id\|mul3` 4 行缺失），另一运行（含单跑）完全正常 |
| `he_rotate_keep_ciphertext` | `he-mul before` | 一次运行报 `ERROR: ElGamal scalar multiplication failed` 且无 `he-mul before\|40`，另一运行正常 |
| `pg_dump` | `he_scalar_mul` | **稳定**失败：`ERROR: homomorphic multiplication failed` |

三者同族（标量乘 / HE 乘法）。`pg_dump` 的稳定失败 + 待拉提交 `357187bc4ca`
（`fixbug:DET 密文段需对齐已有格式`，改的就是 `src/interfaces/libpq/tecc_scheduler/tecc_crypto_scheduler.c`
的 DET/HE 解密段）高度吻合——**建议先拉取该提交再复跑**，再判断是否仍有间歇性。
本台账按"正确行为"（无错误行）安装了 `he_rotate_keep_ciphertext` 的期望：若该故障复现，用例会如实报红。

---

## 十、批次 2 迁移用例的逐例台账（11 个）与后记

| 用例 | 规模(jdbc/libpq/diff) | 主要构成 | 判定 |
|---|---|---|---|
| guc_backslash | 36/47/81 | A1×5, A2×3 | **含 C-19** |
| guc_string_parsing | 24/121/131 | A1×13, A2×4 | **含 C-20** |
| kek_dek_cross_schema | 56/68/42 | A1×9, A2×10, A3×5 | 仅 A/B |
| kek_dek_recreate_after_drop | 28/34/16 | A1×7, A2×3, A3×1, B1×1 | 仅 A/B |
| he_rotate_keep_ciphertext | 38/57/35 | A1×18, A2×6, A3×2, A4×2, B1×1 | 仅 A/B（HE 偶发另见 §十一） |
| kek_dek_drop_owned | 16/19/27 | A1×3, A2×3, A3×6, A4×2 | 仅 A/B |
| kek_upper_options | 20/23/27 | A1×3, A2×4, A4×1 | 仅 A/B（HE 偶发另见 §十一） |
| scalar_func | 337/408/244 | A1×52, A2×13组, A3×2, B1×15 | 仅 A/B（C-9 文案 ×22） |
| scalar_func_arg_combos | 72/131/176 | A1×9, A2×46, B1×50 | 仅 A/B |
| scalar_func_arg_combos2 | 39/64/94 | A1×2, A2×25, B1×22 | 仅 A/B |
| scalar_func_crosstype | 93/136/133 | A1×32, A2×21, B1×13 | **含 C-21** |

### C-19 / C-20 —— runner 拆句器把注释里的 ASCII 单引号当字符串起点（`guc_backslash`、`guc_string_parsing`）

**机理**（`TeccSqlTestRunner.splitStatements:2287`）：先按引号/美元引用状态扫描，**之后**才做 `stripComments`。
注释里的 ASCII `'` 会开启字符串状态，把后续多条真语句吞进同一个"语句"，形成多语句 blob。

**证据**：
- `guc_backslash`：用同一算法复算，24 条真语句只剩 **19 条**（第 1 条把 DROP TABLE/DROP DEK/DROP KEK/CREATE KEK/CREATE DEK/CREATE TABLE 全并成一条）
  → JDBC `ERROR: TECC CREATE DEK 'dek_bs': referenced KEK 'kek_bs' does not exist in key manager`，建表段整体分叉 + 15 条 `relation "t_backslash" does not exist`，**全部断言失效**；
  libpq 侧 `DROP TABLE→…→CREATE TABLE` 全成功，3/6/10 行结果正常。
- `guc_string_parsing`：pgjdbc `Parser.java:1288` `Unterminated string literal started at position 124/324/32` + `Index 0 out of bounds for length 0`；
  libpq 侧 scs on/off 往返、`path\C:\file`、`escaped\\double`、`bq_on/bq_safe`、`PREPARE/EXECUTE`、LIKE 等断言全部通过，JDBC 一条未跑到。

**修法（测试资产侧，待做）**：把这两个文件注释里的 ASCII 单引号也全角化（与反斜杠/分号同一处理），
或让 runner 在拆句前**先整体移除注释**（推荐，一次性消掉 C-19/C-20 与 §五-1 全部变体）。
修完需重生成两例的三份 golden，再判断驱动侧是否仍有独立分歧。

### C-21 `GREATEST` 对 `::varchar` 常量与加密列的混合参数失败（`scalar_func_crosstype`）

```
位置: shared/sql/scalar_func_crosstype.sql:190-191
SQL : GREATEST(c_text, 'cherry-ish'::varchar), LEAST(...), GREATEST(n_text, ...), LEAST(n_text, ...) 
libpq: 成功 2 行 → XG-cast-varchar-ontext|cherry-ish|apple|cherry-ish|apple / …|zebra|cherry-ish|zebra|cherry-ish
JDBC : ERROR: ERROR: GREATEST types text_enc and character varying cannot be matched  (Position: 51)
```
性质：**语句成败 + 结果集不同**。JDBC 侧未处理 `::varchar` cast 后的类型匹配（改写后 SQL 直达服务端触发类型解析报错）。
另 `sql:162/163` 跨 DEK 校验两侧同失败、仅文案不同（C-9）。

### C-22 CREATE DEK 显式 `KEY_ALGO='sm4'`（**已对齐**，2026-09-20）

```
位置: shared/sql/dek_key_algo_sm4.sql:28（he_rotate_keep_ciphertext、kek_dek_cross_schema 同形态级联）
SQL : CREATE DEK dek_algo_sm4 WITH VALUES(KEK = kek_algo_sm4, KEY_ALGO = 'sm4');
libpq: CREATE DEK（放行 —— 'sm4' 等价于省略，DEK 的 key_algo 域为 sm4/he-add/he-mul）
JDBC : ERROR: TECC CREATE DEK: invalid KEY_ALGO 'sm4' for DEK '...' (expected he-add or he-mul for HE DEK; omit for symmetric DEK)
```
性质：**语句成败不同**。C 侧 `stmt_processor.c` 的 CREATE DEK 校验已按 V4.0.1 概要设计 §7.1
放行显式 `'sm4'`（`is_he_dek=0`，等价省略），JDBC 侧照搬了修复前的口径。golden 按正确行为
安装，故 JDBC 侧如实报红。

**已对齐**：pgjdbc `42198da1` 同步放行（并把取值改为严格小写匹配、`KEY_ALGO=''` 由"静默按未
声明"改为报错、报错文案列出全部合法取值）；驱动 JAR 重建后三例转绿，实测见登记册 §7。

### C-23 he_dual_slot_scalar —— `he-add 列 * 常量/列` 赋回本列：libpq 放行，JDBC 前端拦截（2026-09-24）

```
位置: shared/sql/he_dual_slot_scalar.sql:81（B4）
SQL : UPDATE he_dss_sc SET a = a * k;      -- a 声明 he-add，k 为明文 int4 列
libpq: UPDATE 1（服务端 he_mul_scalar_core 走 EC-ElGamal 标量倍点，结果 EG_ADD，与目标列匹配）
JDBC : ERROR: TECC strict: encrypted column 'public.he_dss_sc.a' lacks he-mul segment capability
       for homomorphic assignment to 'a' in SET clause
```

性质：**语句成败不同**，属 JDBC 前端严格规则（对 `*` 赋值要求目标列具备 he-mul 能力）与 libpq
（把 `*` 交给服务端按槽位裁决）的口径差异，**不是本次服务端改动引入**：同一现象已存在于既有
用例 `homomorphic_enc.sql` 的 `19 UPDATE mul neg scalar (he-add)`（JDBC golden 同样报该错，
libpq 侧为 `UPDATE 1`），只是当时未登记。

该断言在用例中的作用是**锁定"仅 he-add 列仍可标量乘"（EC 倍点路径未被双槽改法误伤）**，
故保留并按各自实际行为固化两侧 golden，不按"正确行为"单边安装。若日后统一口径：
libpq 侧收紧、或 JDBC 侧放行，届时同步改本行与两侧期望。

---

## 十一、HE 乘法族"游走式偶发失败"（重点观察项）

同一批用例在**同一份代码**上的不同运行之间，HE 乘法/标量乘断言会随机失败，且**每次都换一个用例**：

| 运行 | 代码基线 | 失败用例 | 现象 |
|---|---|---|---|
| 1（迁移批次 1 前） | 旧 | `homomorphic_enc` | `sql:187 ERROR: homomorphic multiplication failed`（`id\|mul3` 4 行缺失） |
| 2（单跑该例） | 旧 | — | 通过 |
| 3 | 旧 | `he_rotate_keep_ciphertext` | `ERROR: ElGamal scalar multiplication failed`、`he-mul before\|40` 缺失 |
| 4 | 旧 | `pg_dump` | **稳定**失败：`he_scalar_mul\|14` 缺失 |
| 5（rebase 后） | 新 | `kek_upper_options`（libpq） | `sql:48 ERROR: homomorphic multiplication failed` |
| 5（rebase 后） | 新 | `scalar_func`（JDBC） | `ERROR: ERROR: homomorphic power failed`（`power(m_int2,3)` 5 行全缺） |
| 6（rebase 后） | 新 | — | 上述两例均通过 |
| 7（rebase 后） | 新 | `he_rotate_keep_ciphertext`（JDBC） | 同上 HE 族 |

观察：拉取 `357187bc4ca`（DET 密文段对齐）后 `pg_dump` 的稳定失败**已转绿**，但 HE 族的偶发失败**仍在**，
且失败点随运行漂移（不同用例、不同断言）。这些用例的 HE 密钥由 `\! tecc_ktool generate --key-algo sm2`
在**运行期随机生成**，因此"是否命中可完成标量乘/乘法的密钥对"很可能是随机量。

**建议**：优先排查 SM2 密钥生成 → HE 标量乘（`ElGamal scalar multiplication failed` /
`homomorphic multiplication failed` / `homomorphic power failed`）这条链，确认是否存在
"部分密钥对不可用于 HE 运算"或"密钥文件写入/加载偶发不一致"。在定位前，本台账对这 5 个用例
按**正确行为**（无错误行）安装期望：故障复现时用例会如实报红。

### 已定位并修复（2026-09-19）

**根因不在密钥材料**，而在服务端 HE 公钥缓存的**填充顺序**：`he_get_pubkey()`
（`src/backend/utils/tecc/he_operators.c`）旧实现先把空条目插入缓存、再去读 `hg_dek`/`hg_dek_args`；
读目录期间 syscache 未命中会调用 `AcceptInvalidationMessages()`，从而触发
`he_pubkey_cache_relcallback()` → `he_pubkey_cache_invalidate()`，把**正在填充的条目**连同其中的
BIGNUM/EC_POINT 一起释放并 `hash_destroy`，函数随后返回已释放（实测为 NULL）的 `p/q/g/h` 指针 ⇒
`tecc_elgamal_mul()` 报 `homomorphic multiplication failed`。

- 证据：临时探针在每次失败时都记录 `htab destroyed during he_load_real_pubkey(FF) ... ff_p=(nil)`；
- 复现率：独立复现循环（同会话内新建 KEK/DEK/表后立刻做乘法）**修复前 6/240 命中，修复后 0/300**；
- 修法与验证详见 `plan/2026-09-19-HE乘法偶发失败-根因与修复.md`。

---

## 十二、rebase 到 origin 后的复验与新增归一化

本仓库落后 origin 2 个提交，提交后 rebase 并复验（新代码基线不含我本轮改动）：

- **`tecc_full_test_suite`（JDBC）** 暴露 `ALTER TYPE` 的两个**合法但互斥**的依赖报错分支，
  报哪个取决于目录扫描顺序（运行间不确定）：
  `cannot alter type of a column used by a view or rule` + `Detail: rule _RETURN on view edge_view depends on column "v"`
  ↔ `cannot alter type of an encrypted column` + `Detail: encrypted column edge2.v depends on column "v"`。
  已在 `normalize_common_output` 归一化为 `cannot alter type of a column` + `DEPENDENCY depends on column`
  （与既有 DROP DEK DETAIL 归一化同一处理思路），两侧对称生效。
- 复验结果：JDBC **56/57**（1 例为 §十一 的 HE 偶发）；libpq **130/131**。
- 更新 §七-7 的结论：拉取落后提交后，`pg_dump` 的稳定失败（`he_scalar_mul` 缺失 +
  `homomorphic multiplication failed`）**已转绿**，证实其归因正确；但 `tecc_admin_funcs`
  **拉取后仍失败**，故其根因**尚未确定**（不是本仓库改动引入：本轮改动前它就在失败）。
  现象：实际输出在 `hg_tecc.mode is enabled.` 之后提前结束，缺失尾部 `t`、重连横幅、
  `REVOKE`×2/`DROP ROLE`/`DROP TABLE`/`DROP DEK`/`DROP KEK` 与结束语，同时尾部出现
  `sql:74/107/124` 三处错误行。需另立项排查（涉及 `\enable hg_tecc.mode`、角色与
  `tecc_decrypt_raw`），**不应由本仓库改期望掩盖**。

## 十三、JDBC 侧硬加密（HSM）用例打通（D2，2026-09-18）

### 背景与做法

此前 HSM 用例（`shared/sql/swxa_*.sql` 等 7 个）只在 libpq 侧能跑：libpq 通过
`TECC_SDF_LIB=<libteccsdf.so>` 把 swxa 适配器的设备调用切到软件密码机 SoftSDF；
而 **JDBC 走 JCE Provider 路线**（反射加载 `com.sansec.jce.provider.SwxaProvider`），
SoftSDF 插不进去，于是 JDBC 回归里这批用例长期是靠门禁跳过的。

D2 补上这一层：新增 `tecc_kms/jce/`（SoftSDF 的 JCE 门面，drop-in Provider，
后端读 SoftSDF 的 `kek-<index>.key` / `sm2enc-<index>.pem` 做标准 SM4-CBC / SM2，
已与 native SoftSDF **逐字节核对**），**零 pgjdbc 产品代码改动**。
细节见 `tecc_kms/jce/README.md`。

接入方式（仅在设了 `TECC_SDF_LIB` 时生效）：
把门面 JAR 放到 runner classpath 首位、由 `setup_swsds_ini` 生成 `hsm.properties`
（`swxa.ini.path` 仅做存在性校验）并传 `--tecc-config`。

### 本轮同时修掉的两个框架缺陷（都在 C 侧 harness）

1. **`\!` 段没有注入节点连接信息**：`002` 代 runner 执行 `\!` 用的是裸 `system()`，
   而 psql 的 `\!` 子进程只继承环境、拿不到 psql 命令行的 `-h/-p`。节点用的是自定义
   端口，于是 `swxa_concurrent` 里 `\! psql ...` 起的后台进程**连到了默认 5432 端口**
   （`conc_bg.out` 里是 `connection ... failed`）——即该用例在 JDBC 路径上**后台进程从未运行**，
   却因为 golden 与"实际"同样缺那部分而看起来通过。
   修法：`\!` 段用 `local %ENV` 注入 `PGHOST`/`PGPORT`（与 `$node->psql` 口径一致）。
2. **`\!` 段的 stdout 没有拼回用例输出**：psql 会把 `\!` 命令的 stdout 直接写进自己的
   stdout（与 SQL 结果按序交错），而 runner 丢弃 `\!`。修法：框架代执行时把 shell 段
   stdout 按序拼回用例输出。除 `swxa_concurrent` 的 `\! cat conc_bg.out` 外，
   本仓库其余 `\!` 命令均静默或已重定向 → **不影响任何既有 golden**（已用全量回归确认）。

另修一处**新写的归一化规则自身缺陷**（本轮引入、本轮修掉）：`normalize_jdbc_runner_output`
里剔除驱动 `HsmVendorClassLoader` INFO 行时用了 `^[^:]*:?...`——Perl 的否定字符类
**会匹配换行**，导致规则从更早的某一行一路吃到 INFO 行，把中间整块输出删掉
（表现为 5 个 swxa 用例的 golden 全部"尾部缺失"）。已改为不跨行的 `^.*...`，
并在注释里写明这个坑。

### 新增 4 个 `shared/expected_jdbc/swxa_*.out`（JDBC 自有 golden）

`swxa_enc` / `swxa_search` / `swxa_ope` / `swxa_alter_kek` 四例在 JDBC 路径跑通并生成
自有期望。与 libpq 侧同用例归一化输出的差异**全部落在已知类别**：

| 差异 | 类别 | 说明 |
|---|---|---|
| libpq 有 `(N row)` 页脚行 | A1 | psql 特有，runner 不产 |
| `NOTICE` 流位置不同（JDBC 前置、libpq 后置） | A2 | 已知 |
| `Setting up SWXA KEK: <kek> (key index: N)` / `SUCCESS: Key found in SWXA password machine at index N` | E | **libpq C 客户端进度行**（JDBC 侧等价信息走 FINE 日志，默认不输出） |
| `bpchar_enc` 尾部空格：libpq 归一化后无填充、JDBC 保留填充 | A4 | `bpchar` 输出填充差异，既有 `bpchar_*` 用例同一情况 |
| `ALTER KEK` 命令 tag：JDBC 打印 `ALTER KEK`，libpq 不打印 | A3 | libpq 前端把 `ALTER KEK` 重写为辅助函数调用（输出的是空列结果集），**既有 `alter_kek` 用例已是同一差异**，非本轮引入 |

**未发现新的 C 类（真语义）差异。**

### `swxa_concurrent`：JDBC 路径已启用（2026-09-19）

框架修好后该用例的后台进程首次真正运行，随即**稳定复现一个 C 侧真缺陷**：
多 KEK 时 `tecc_parse_kek_list` 未按 `kek_oid` 分组合并 args 行，导致**首个 KEK** 的
`KEY_STORE` 被后续 KEK 的值覆盖 → `kms_type` 误判 → HSM KEK 被当本地文件去
`$TECC_KEK_PATH/hsm:id=1` 加载而失败（其余 KEK 还会在列表里整体丢失）。

- 缺陷详情 + 纯 libpq 最小复现 + 修复与验证：
  `plan/2026-09-18-KEK元数据多KEK合并-首个KEK-kms_type被覆盖.md`
- 修复前处置：JDBC 路径在 `skip_jdbc_unsupported_cases` 中跳过并打印原因，**不把缺陷写进期望**。
- **修复后**（2026-09-19）：KEK 列表解析改为逐行按 `kek_oid` 分组，缺陷消除；该跳过项已删除，
  用例在 JDBC 路径启用并新增期望 `shared/expected_jdbc/swxa_concurrent.out`。
- 启用时同时调整了该用例的等待时长：它靠 sleep 卡两进程时序，而 JDBC 侧每段 SQL 由 runner
  起**独立 JVM**执行（实测固定开销约 2.5s，libpq 单连接仅约 0.3s），原时长下后台查询会赶在
  `ALTER KEK` 之前发生（实测输出只有 3 行、缺轮换后插入的 500）——即用例通过却没验证到
  "轮换后用缓存 DEK 解密"。改为 后台 `pg_sleep` 8s、主进程 `sleep` 12s 后，后台查询结果回到
  4 行。libpq 侧 raw 期望仅 psql NOTICE 的行号变化，common 期望不变。
- ⚠ 注意：JDBC 跑不了的用例（`%JDBC_UNSUPPORTED_CASE`）由 002 在读用例清单处剔除，
  与硬加密用例的剔除（`exclude_hsm_cases`）在同一处生效——读清单之前不要按"少了几条"下结论。

### 本轮结果

| 套件 | 命令 | 结果 |
|---|---|---|
| JDBC（SoftSDF 模式） | `TECC_SDF_LIB=... make check-tecc-jdbc` | **63/63**（偶发会命中 §十一 的 HE 族问题） |
| JDBC（默认，无加密机） | `make check-tecc-jdbc` | **57/57** |
| libpq | `TECC_SDF_LIB=... make check-tecc-libpq` | **141/142**（唯一失败为既有 `tecc_admin_funcs`，见 §十二；HE 族偶发见 §十一） |

> 上表为 2026-09-19 修复"多 KEK 时首个 KEK 的 kms_type 被覆盖"缺陷后的复测结果：
> 该缺陷修好后 `swxa_concurrent` 在 JDBC 侧启用（用例数 62→63），并新增常驻用例
> `libpq/sql/swxa_multi_kek.sql` 直接守该缺陷（libpq 用例数 141→142）。

### 真机模式的厂商 JAR 挂载（补充）

`002` 给 runner 拼的 classpath 原先只有"测试类目录 + 驱动 jar"，**不含厂商 HSM 的 JAR**，
于是真机环境下那 4 个 HSM 用例会以
`SWXA JCE provider loaded from pgjdbc stub — vendor JAR (swxajce) not found on classpath`
失败（改动前它们同样跑不了，只是失败原因是"缺 expected_jdbc 期望文件"）。这是**回归脚手架
漏配**，与产品正常运行无关：部署时厂商 JAR 本来就要求放进应用 classpath，驱动侧
`HsmVendorClassLoader` 会按"非驱动 `.jar`"扫 `java.class.path`、再试线程上下文类加载器
（DBeaver/OSGi 场景），最后才回退驱动内的空壳 stub 并抛错。

处置：**不加模式开关**（`TECC_SDF_LIB` 已承担 SoftSDF / 真机 的选择），未设它时自动加
`--driver-dir`——默认 `$TECC_JDBC_HOME/pgjdbc/libs`（与 runner 用法示例、gradle
`hsmVendorDir` 兜底一致），厂商 JAR 在仓库外时用 `TECC_HSM_VENDOR_DIR` 覆盖目录。
厂商身份由 KEK 的 `key_store` 决定、**不靠目录区分**，故不同厂商 JAR 可共处同一目录。
SoftSDF 模式**绝不传**该参数（门面与厂商同名类，同时在场时谁先被扫到不确定）。
运行日志会打印 `# 真机模式: 厂商 JAR 目录 -> <dir>` 便于排查。

### ~~遗留：`check-tecc-cross`（003）在 SoftSDF 模式下仍不可用~~ —— ✅ 已修复（2026-09-19）

原缺口：`cross/sql/005_swxa_cross.sql` 也受 `TECC_SDF_LIB` 门禁启用，但 `003` 脚本
既没有切 CWD 到 `tmp_check/tmp`（SoftSDF 按**相对 CWD** 读 `kek-<n>.key`），
也没给 runner 加 JCE 门面 JAR 与 `--tecc-config` → libpq 段找不到 KEK 文件、JVM 段
拿不到 HSM 通道；真机模式下同样还缺 `--driver-dir`。

已按 `002` 的做法补齐（JCE 门面 + `--tecc-config` + `--driver-dir` + 子进程 CWD），
并顺带修掉两处会让整套用例必然报红的问题：**用例清单读在门禁之前**（门禁已把
`005_swxa_cross.sql` 改名 `.skip`，却仍被当用例执行 → 读文件不存在）、以及
**localkms KEK 文件未预生成**（`pregen_localkms_kek_files` 未调用 → 001~004 的
`CREATE KEK` 直接 `ERROR: key file not found: kek_cross.key`，其余用例级联失败）。
期望文件随实测重生成（002~005 的两驱动归一化输出逐字节一致后落盘），
并把 libpq 前端 HSM 进度行与 JCE 工具噪音纳入归一化。详见
`plan/2026-09-19-回归遗留问题登记-四项待办.md` §2。

---

---

## 十四、JDBC runner 会挑到过期驱动 JAR —— 一次误判的复盘（2026-09-19，**已修复**）

> **已修复**（2026-09-19，本节保留复盘过程）：驱动 JAR 现由 `t/tecc_jdbc_jar.pm` 统一选取
> ——排除 `-sources/-javadoc` 旁挂产物、优先 `-all.jar`、同组内按版本号数字段逐段比较取最高；
> 五个 JDBC 入口（`002`/`003`/`005`/`006`/`007`）与 `Makefile` 的 `tecc-sdf-jce-jar`
> 共用该规则，并把实际选用项与**被忽略的旧版本**打进 TAP 输出。
> 故下文"正确的跑法/临时绕行"一节不再需要。登记与验证见
> `plan/2026-09-19-回归遗留问题登记-四项待办.md` §5。

### 坑在哪

`t/002_tecc_jdbc_test.pl` 取驱动 JAR 的方式**（修复前）**是：

```perl
my @jars = glob("$jdbc_home/pgjdbc/build/libs/postgresql*.jar");
my $jdbc_jar = $jars[0];
```

`glob` 按**字典序**返回，无版本/mtime 语义。`pgjdbc/build/libs/` 下若同时留着
`...-4.0.0.0-SNAPSHOT-all.jar`（旧）与 `...-4.0.1.0-SNAPSHOT-all.jar`（当前），
`$jars[0]` 恒取 **4.0.0.0**，于是"测试类（当前源码编译） + 驱动 JAR（旧版本）"
混搭运行。`tecc-sdf-jce-jar` 目标（`Makefile`）用同一写法取 JAR，会直接编译失败。

### 现象（本次踩坑记录）

用旧 JAR 跑全量 JDBC 回归，表面是"**56 例失败 + `pg_ctl stop failed`**"，
很容易误判成"JDBC 侧大批用例本来就不过"。实际是两件事被放大：

1. 旧驱动的密文格式与当前服务端不匹配 → 大量 `invalid CipherDataType structure`；
2. 其中一条查询把后端打崩：`double free or corruption (!prev)` → signal 6 Abort，
   节点 `restart_after_crash = off` → 实例 shutdown → 崩溃点之后的用例全部连带失败
   （这才是"56 例失败"的来源，并非 56 个独立缺陷），收尾时 `pg_ctl stop` 自然也失败。

换用与当前源码匹配的驱动 JAR 后同一份代码 **52/57 通过**，且不再出现崩溃与级联。
**结论：JDBC 回归结果对"选到哪个 JAR"高度敏感，遇到大面积红先确认 JAR 版本，别急着归因到用例。**

### 原本的绕行跑法（历史，已不需要）

修复前把驱动 JAR 单独摆一个 `TECC_JDBC_HOME`、只放当前版本那一个：

```bash
JH=~/pg14_tecc_dev/pgjdbc-release-42.5.x
mkdir -p /tmp/tecc_jdbc_home/pgjdbc/build/libs
ln -sfn $JH/pgjdbc/build/classes /tmp/tecc_jdbc_home/pgjdbc/build/classes
ln -sfn $JH/pgjdbc/libs          /tmp/tecc_jdbc_home/pgjdbc/libs
ln -sfn $JH/pgjdbc/build/libs/postgresql-42.5.1-tecc-4.0.1.0-SNAPSHOT-all.jar \
        /tmp/tecc_jdbc_home/pgjdbc/build/libs/
cd src/test/modules/tecc_tests
TECC_JDBC_HOME=/tmp/tecc_jdbc_home make check PROVE_TESTS=t/002_tecc_jdbc_test.pl
```

（或直接把 `pgjdbc/build/libs/` 下的旧 JAR 清掉；`build/` 是生成物，不入库。）

**已按此建议修复**：改为 `t/tecc_jdbc_jar.pm` 统一按版本选取（优先 `-all.jar`，
数字段逐段比较取最高；兼作 `Makefile` 的 `tecc-sdf-jce-jar` 取 JAR 的唯一规则），
002/003/005 还会把选中的 JAR 与被忽略的旧版本打印到 TAP 输出 —— 今后"用了哪个驱动"
在日志里一目了然，不必再从失败形态反推。本机旧 JAR 亦已删除。

### 本轮遗留的 5 例失败（未修，需确认是"golden 需重生成"还是"环境缺 SoftSDF"）

用正确 JAR 后仍有 5 例红：`alter_kek`、`dek_key_algo_sm4`、`he_kek_privkey`、
`he_rotate_keep_ciphertext`、`kek_dek_cross_schema`。差异形态都是**期望文件与当前
驱动/环境不一致**，与本次 C 侧改动无关：

- `he_kek_privkey` 的 golden 里**泄漏了绝对路径** `.../home/wxp2/pg14_tecc_dev/...`，
  即该 golden 是在另一个用户环境下生成的，换用户必然红（归一化未覆盖此处）；
- 其余几例是 DEK `KEY_ALGO` 校验文案差异（golden 为 `invalid KEY_ALGO 'rsa'`，
  实际为 `invalid KEY_ALGO 'sm4' ... (expected he-add or he-mul for HE DEK)`）
  以及 `permission denied for table hg_dek_args`，属 golden 生成时的驱动/权限环境与当前不同。

另：本轮未设 `TECC_SDF_LIB`（SoftSDF 软件密码机未构建，`tecc_kms/bin/` 仅有 `.gitkeep`），
HSM 用例按门禁跳过；上述 5 例中有几例可能与 SoftSDF 通道有关，需在 SoftSDF 模式下复验。

> ⚠ 上述 5 例的红已在 2026-09-19 全部定性完毕，本节记录的当日状态已过期：
>
> - `he_kek_privkey`：golden 里泄漏绝对路径 → 已在归一化层修掉（登记 §8）；
> - `dek_key_algo_sm4` / `he_rotate_keep_ciphertext` / `kek_dek_cross_schema`：
>   pgjdbc 对显式 `KEY_ALGO='sm4'` 仍 fail-closed，C 侧已放宽（登记 §7）；
> - `alter_kek`：JDBC 侧 ALTER KEK 的写回路径缺陷（见 §十五，登记 §10）。

---

## 十五、ALTER KEK 在非超级用户角色下：JDBC 裸 UPDATE 系统表被 ACL 挡（2026-09-19 定性，2026-09-20 **已修复**）

### 现象

`shared/sql/alter_kek.sql` 17c：非超级用户角色 `u_rot` 作为 KEK `kek_rot_own` 的**属主**
执行 `ALTER KEK kek_rot_own WITH (KEY_PATH = ...)`：

| 驱动 | 结果 |
|---|---|
| libpq | `ALTER KEK`（成功，无需额外 GRANT） |
| JDBC | `ERROR: ERROR: permission denied for table hg_dek_args` |

同一用例的 17a（**无** USAGE 时须给出明确的权限不足）两侧都拒绝，但文案不同（见文末"附带"）。

### 根因：两侧"写回密钥材料"的落点不同

ALTER KEK 轮换要把重加密后的密钥材料写回系统表，两个驱动的做法不一致：

- **C（libpq）**：Bug 9255 已改走 **SECURITY DEFINER 的受控入口** ——
  `SELECT hg_kek_rotate_dek_arg(<dek_oid>, 'encrypted_value', '<hex>')`
  （`stmt_processor.c:8733`）、HE 私钥 `he_ec_privkey/he_eg_privkey` 同函数（`:8908`）、
  KEK 公钥 `SELECT hg_kek_rotate_kek_arg(<kek_oid>, 'public_key', E'\\x<hex>')`（`:8934`）。
  函数内部按 `has_kek_privilege(session_user, kek_id, 'USAGE')` 自校验，口径与后端
  `AlterKekCmd` 一致：有 USAGE 的普通角色能完成轮换，无 USAGE 的得到"权限不足"。
- **Java（pgjdbc）**：`AlterKekPreProcessor.appendDekUpdate/appendHeKeyUpdate` 直接拼
  `UPDATE hg_dek_args SET value = convert_to('<hex>','UTF8') WHERE dek_id = ...`
  （`AlterKekPreProcessor.java:690`、`:698`），`appendPublicKeyUpdate` 同样裸拼
  `UPDATE hg_kek_args ...`（`:709`）。该表的 UPDATE 权限不开放给普通用户，
  在 `SET ROLE u_rot` 下必然被 ACL 挡住。

服务端日志（`tmp_check/log/002_tecc_jdbc_test_tecc_jdbc.log`）直接坐实是**客户端拼的裸 UPDATE**：

```
LOG:  statement: UPDATE hg_dek_args SET value = convert_to('830bee...','UTF8')
      WHERE dek_id = 16440 AND upper(key) = upper('encrypted_value');
ERROR:  permission denied for table hg_dek_args
```

### 性质与处理

**JDBC 侧缺陷**（不是 golden 缺陷，也不是环境 ACL 问题）。`expected_jdbc/alter_kek.out` 中 17c
期望 `ALTER KEK` 属**正确行为**，故该行如实报红，待 pgjdbc 侧改用同样的受控入口后转绿。

**附带（同一处，一并待修）**：17a 无 USAGE 时 libpq 给
`ERROR: permission denied for key encryption key kek_rotate`，JDBC 给
`ERROR: TECC ALTER KEK: KEK (oid=N) not found in database` —— 用例头注明确要求
"须给出明确的权限不足（而非'加载旧 KEK 失败'）"，JDBC 文案不符合该意图。
goldens 目前记录 JDBC 既有行为（OID 已在归一化层处理为 `OID`），pgjdbc 修文案后需同步
`expected_jdbc/alter_kek.out` 该行。

**已修复（2026-09-20）**：pgjdbc 侧 `AlterKekPreProcessor` 已改为调用 SECURITY DEFINER 受控入口
`hg_kek_rotate_kek_arg()` / `hg_kek_rotate_dek_arg()`（`AlterKekPreProcessor.java:1142` 起，带
`to_regprocedure(...)` 能力探测），与 C 侧口径一致。本机复跑：`make check-tecc-jdbc` 58/58、
SoftSDF **64/64**，17c 转绿。

**附带项（仍未修）**：17a 无 USAGE 时的 JDBC 文案照旧，仍是
`TECC ALTER KEK: KEK (oid=OID) not found in database`，未对齐 libpq 的
`permission denied for key encryption key kek_rotate`；golden 该行继续保留 JDBC 既有行为，
待 pgjdbc 侧改文案后再同步。

---

## 十六、JDBC 侧硬加密能力缺口（随驱动 JAR 版本修正而暴露，2026-09-19，2026-09-20 **已修复**）

修好驱动 JAR 选取（§十四）后，JDBC 才真正用上**与服务端匹配**的驱动；此前用旧驱动
"通过"的 HSM 用例，在新驱动下如实报红。SoftSDF 模式下全量 JDBC 为 **57/64**，
其中 4 例是 §十/§十五 已登记的驱动侧差异，另 **3 例为本节新增的硬加密能力缺口**：

| 用例 | libpq（C） | JDBC（Java） | 性质 |
|---|---|---|---|
| `swxa_enc` | 通过 | `ERROR: TECC CREATE DEK: invalid KEY_ALGO 'sm4' for DEK 'swxa_he_kek' (expected he-add or he-mul for HE DEK; omit for symmetric DEK)` → `swxa_he_sym` 建不出、`swxa_he_t` 不存在，后续全红 | 与登记 §7 同源（pgjdbc 未同步 C 侧"显式 `'sm4'` 等价省略"） |
| `swxa_alter_kek` | 通过（`ALTER KEK` 成功） | `ERROR: TECC ALTER KEK: KEK 'swxa_rot_kek' key material is unavailable (HSM-hosted KEK is not supported for ALTER KEK)` | Java 驱动不支持**HSM 托管 KEK** 的轮换 |
| `swxa_concurrent` | 通过 | 同上形态（`ALTER KEK conc_kek`） | 同上 |

即：C 侧通过 SDF 的密钥索引/导出通道支持 HSM 托管 KEK 的 `ALTER KEK`（见
`stmt_processor.c` 的 `export_public_key` 分支与硬加密轮换处理），Java 侧直接拒绝该场景。

**处置**：三例的 golden 按 libpq/C 侧的**正确行为**安装，JDBC 侧如实报红（不得改期望掩盖）；
与 §7（`KEY_ALGO='sm4'`）、§十五（ALTER KEK 写回被 ACL 挡）同属 pgjdbc 仓库，
建议同批处理。修复后复跑
`TECC_SDF_LIB=$PWD/tecc_kms/bin/libteccsdf.so make check-tecc-jdbc` 应转绿。

**已修复（2026-09-20）**：pgjdbc `8edd4201`（"ALTER KEK 支持硬加密（HSM）轮换——设备解包/重封装
+ 写回函数兼容"）补齐了 HSM 托管 KEK 的轮换能力，`42198da1` 解决 `KEY_ALGO='sm4'` 级联。
按上述命令复跑（驱动由 `42198da1`+`8edd4201` 后的检出重建）：

```
64 of 64 tests passed, 0 failed
ok - jdbc: swxa_enc ... ok
ok - jdbc: swxa_alter_kek ... ok
ok - jdbc: swxa_concurrent ... ok
```

三例 golden 无需改动（原本就按 libpq/C 侧正确行为安装）。
