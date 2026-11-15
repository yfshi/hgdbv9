# JDBC 驱动与 libpq 的 common 输出差异

## 验证结论

search.sql 的 common 比较中，前 181 行数据内容 libpq 与 JDBC 完全一致。
diff 文件：`/tmp/jdbc_vs_libpq_diff.txt`（1226行）

## 待解决问题

### #1 JDBC 端缺少 NOTICE 输出

**现象**：libpq（psql stderr）输出 DROP IF EXISTS 的 NOTICE，JDBC 端不产生。

```
< psql:/.../search.sql:11: NOTICE:  key encryption key "kek_search" does not exist, skipping
< psql:/.../search.sql:24: NOTICE:  table "tecc_like_test" does not exist, skipping
< psql:/.../search.sql:78: NOTICE:  table "tecc_not_like_test" does not exist, skipping
< psql:/.../search.sql:118: NOTICE:  table "tecc_like_combined" does not exist, skipping
< psql:/.../search.sql:157: NOTICE:  table "tecc_empty_search" does not exist, skipping
< psql:/.../search.sql:183: NOTICE:  drop cascades to data encryption key dek_search
```

**分析**：JDBC 驱动通过 TCP 直连 PG，没有 psql 的 NOTICE 输出机制。 
PG JDBC 驱动收到 NOTICE 后通过 `PGConnection.getNotifications()` 获取，但
`TeccSqlTestRunner` 当前没有捕获并输出 NOTICE。

**复现**：`TECC_JDBC_HOME=... make check-tecc-jdbc-common TECC_TEST=search.sql`

### #2 TeccSqlTestRunner 输出框架噪音

**现象**：JDBC runner 输出了汇总行、WARNING、异常堆栈，这些不属于数据输出。

```
>     语句总数: 48, 通过率: 100%
> 全部完成: 1 个文件, 100% 通过率
> 报告已生成: ...
> WARNING: Failed to read SymAlgFlag.SGD_SM4_ECB via reflection
> WARNING: fetchColumnsByTable failed for .
> FetcherException{...schemaName must be explicitly provided...}
```

**分析**：`TeccSqlTestRunner.run()` 方法在 psql 输出模式下仍然输出了框架汇总
信息。需在 psql 模式下抑制这些 System.out 输出。

**涉及文件**：`TeccSqlTestRunner.java` — `outputPsqlResult()` 方法已实现，
但 `run()` 中的进度/汇总输出未针对 psql 模式屏蔽。

### #3 FetcherException: schemaName 为空

**现象**：每取一条结果集数据，都报一次 `fetchColumnsByTable failed for .`，
schema 为空。

```
5月 24, 2026 3:46:18 ... MetadataCacheManager lambda$getColumnEncryptionPolicy$8
WARNING: fetchColumnsByTable failed for .
FetcherException{code=7, message='schemaName must be explicitly provided ...'}
```

**分析**：`DefaultMetadataFetcher.fetchColumnsByTable()` 拒绝空 schema 参数。
`MetadataCacheManager.getColumnEncryptionPolicy()` 调用时传入了空 schema，
可能是连接串中未设置 `currentSchema`，或 TeccSqlTestRunner 未配置 schema
参数。

**影响**：WARNING 级别，不阻断执行，数据仍能正常返回。但说明 JDBC 驱动的元
数据获取路径有配置缺失，可能导致某些场景下加密列元数据查找失败。

**复现**：`TECC_JDBC_HOME=... make check-tecc-jdbc-common TECC_TEST=search.sql`

### #4 HSM 反射警告

**现象**：启动时反射读取 `SymAlgFlag` 失败，使用 fallback 值。

```
WARNING: Failed to read SymAlgFlag.SGD_SM4_ECB via reflection, using fallback=1025
WARNING: Failed to read SymAlgFlag.SGD_SM4_CBC via reflection, using fallback=1026
```

**分析**：环境没有 HSM 密码机，JDBC 驱动启动时尝试加载 HSM 厂商类失败，
回退到默认值。不影响功能。

**解决方案**：有 HSM 环境的机器上不会出现此警告；无 HSM 时可忽略或配置
`tecc_config` 跳过 HSM 初始化。


## ope.sql 对比发现的差异

测试脚本：`shared/sql/ope.sql`（437 行，覆盖 OPE 范围查询、ORDER BY、COLLATE 等）
diff 文件：见 `/home/wxp/.claude/projects/.../tool-results/bo5fwlmvs.txt`（411KB）

### #5 浮点数格式不一致

**现象**：psql 与 JDBC 对 float4/float8/numeric 的输出格式不同。

```
libpq:  1e+06       0          1e-07         0.0000001
JDBC:   1000000.0   0.0        1.0E-7        1E-7
libpq:  5           10
JDBC:   5.0         10.0
```

- psql 对整数型 float 输出 `0` `5`（不带 `.0`）
- JDBC `ResultSet.getString()` 输出 `0.0` `5.0`（带 `.0`）
- 科学记数法格式不同：`1e+06` vs `1000000.0`，`1e-07` vs `1.0E-7`

**影响**：排序结果本身正确，但输出格式差异导致 common 比较失败。

**复现**：`TECC_JDBC_HOME=... make check-tecc-jdbc-common TECC_TEST=ope.sql`

### #6 排序结果不一致（COLLATE 场景）

**现象**：libpq 和 JDBC 对中文字符和 NULL 的排序位置不同。

```
=== COLLATE "C" ORDER BY text_asc ===
libpq:  ABC → Banana → Z → 测试 → (空串)
JDBC:   测试 → ABC → Banana → Z → NULL
```

- 中文字符 `测试` 在 libpq 中排在字母之后，JDBC 中排在最前
- NULL 值显示：libpq 输出空串 `|`，JDBC 输出 `|NULL`

**分析**：COLLATE "C" 下中文字符的排序行为可能由 libpq 和 JDBC 的本地化
设置差异导致。NULL 的字符串表示不同（psql 输出空串，JDBC 输出 "NULL"）。

**复现**：`TECC_JDBC_HOME=... make check-tecc-jdbc-common TECC_TEST=ope.sql`

### #7 JDBC 报告 extra ERROR

**现象**：JDBC 端多出一条 libpq 没有的报错。

```
ERROR: cannot cast type varchar_enc to text
  Position: 43
```

**分析**：ope.sql 第 7 节 ORDER BY COLLATE 测试中，JDBC 驱动报告
`varchar_enc` 转 `text` 类型转换失败，但 libpq（psql -C）正常执行。
可能是 JDBC 驱动的类型映射缺失 `varchar_enc → text` 的隐式转换路径。

**复现**：`TECC_JDBC_HOME=... make check-tecc-jdbc-common TECC_TEST=ope.sql`

---

# 跨驱动行为差异（C 类）——2026-09-18，批次 1 迁移用例暴露

> 来源：12 个 libpq 独有用例迁入 `shared/sql/` 后，同一用例在 psql 与 JDBC 上的行为对拍。
> 逐条证据与差异分布见 `shared/jdbc_vs_libpq_diff.md` §九；此处按**问题**组织，供 JDBC 侧跟进。
> 编号 C-13…C-18 与台账一致。

## 复现方式（任一条通用）

```bash
cd src/test/modules/tecc_tests
TECC_JDBC_HOME=~/pg14_tecc_dev/pgjdbc-release-42.5.x TECC_TEST=<case>.sql make check-tecc-jdbc   # JDBC 侧
TECC_TEST=<case>.sql make check-tecc-libpq-common                                                 # libpq 侧
# 两侧期望文件即为对拍基准：
#   JDBC : shared/expected_jdbc/<case>.out
#   libpq: shared/expected_common/<case>.out
```

---

### C-15 【严重】EXPLAIN 计划输出被驱动当作密文解密（`det_setop_plan`）

**现象**：8 处 `EXPLAIN (COSTS OFF)`（UNION/INTERSECT/EXCEPT 集合操作）在 JDBC 侧计划行**全部缺失**，
被等量的驱动异常替代；补丁的 `payload len` 恰好等于各段计划文本长度 → 计划文本被送入了解密路径。

**证据**：
```
libpq（正常）: HashAggregate / Group Key: ds_t.a, ds_t.d / HashSetOp Intersect / Filter: (a = '\x…'::int4_enc)
JDBC        : [读取异常: TECC: ciphertext too short or truncated for algorithm RND (payload len=13, min=72)]
```

**位置**：`shared/sql/det_setop_plan.sql:95/128/129/132/155/175/182/186/199`
**影响**：EXPLAIN 类断言在 JDBC 下完全失效（探针类用例尤其依赖它）；也可能影响用户用 EXPLAIN 排查。
**建议**：优先排查结果集取值路径对 EXPLAIN（`*plan*` 文本列）的解密门控。

### C-13 聚合拦截口径不一致（`aggregate_func`）

**现象**：同一批聚合语句，libpq **前端拦截**，JDBC **放行**并返回解密结果（或两侧都退化但退化方式不同）。

| # | 位置 | libpq | JDBC |
|---|---|---|---|
| 1 | `sql:571/574/577/580/590/593/596`（7 条 `array_agg(密态列)`） | 拦截、无结果行（`aggregate function "array_agg" is not supported on encrypted column "c_int2"`） | 成功：`array_agg(c_int2)-OPE\|{"100","-50","0",NULL,"200"}` |
| 2 | `sql:640` `array_agg(...) OVER (...)` | 拦截（`inside window function is not supported`） | 返回 5 行 |
| 3 | `sql:764` `COUNT(DISTINCT CASE WHEN c_int4 > 0 THEN c_text ELSE NULL END)` | `ERROR: operator does not exist: int4_enc > integer` | `COUNT(DISTINCT CASE)-OPE\|2` |
| 4 | `sql:817` 子查询 array_agg | 4 行占位 `{"ERROR"}` | 4 行 `{"\x<hex>"}`（未解密） |

**建议**：先判定哪一侧是期望语义（拦截 or 支持），再统一；`COUNT(DISTINCT CASE)` 的语句成败差异（第 3 条）优先级更高。

### C-14 ALTER KEK 轮换链 3 处分叉（`alter_kek`）

| # | 位置 | libpq | JDBC |
|---|---|---|---|
| 1 | `sql:139` 信封长度断言 | `t`（成功） | `ERROR: TECC SQL parse failed (tecc.rewrite.fail_closed=true)` |
| 2 | `sql:228` 无 USAGE 角色 `ALTER KEK` | `ERROR: permission denied for key encryption key kek_rotate`（Bug 9255 修复后口径） | `ERROR: TECC ALTER KEK: KEK (oid=16480) not found in database` |
| 3 | `sql:236` GRANT USAGE 后 `SELECT id,name,age FROM rot_test` | 明文 5 行（`1\|Alice\|30`…） | `1\|\x被隐藏的HEX输出\|0`…（**未解密，age 退化为 0**） |

**建议**：第 2 条属**权限不足被伪装成对象不存在**（安全/诊断口径问题）；第 3 条属轮换后解密失效，优先排查。

### C-16 不支持空目标列表（`empty_target_list`）

**现象**：`SELECT FROM t`（空目标列表）及其子查询形式在 JDBC 侧不解析 → 密态常量未改写、明文下发。
**证据**：`sql:42/45/48/56` 四处
```
libpq: 1a_empty_tlist_count|2 / 1b_empty_tlist_nomatch_count|1 / 1c_empty_tlist_ope_count|3 / 2a_subquery_empty_tlist_count|2
JDBC : ERROR: invalid hex string: "Alice" ／ ERROR: operator does not exist: int4_enc > integer
```
对照组 `2b/3a/3b` 两侧一致 → 问题局限在空目标列表形态。

### C-17 多对象 DDL 与 SELECT INTO TEMP 不支持（`encrypted_view`）

| # | 位置 | libpq | JDBC |
|---|---|---|---|
| 1 | `sql:14` `DROP VIEW IF EXISTS v_enc, v_enc2, v_plain CASCADE` | `DROP VIEW` | `ERROR: TECC SQL parse failed (tecc.rewrite.fail_closed=true)` |
| 2 | `sql:15` `DROP MATERIALIZED VIEW IF EXISTS …` | `DROP MATERIALIZED VIEW` | 同上 parse failed |
| 3 | `sql:173` `SELECT oid INTO TEMP ev_multi_colid …` | `SELECT 1` | parse failed |
| 4 | `sql:178` `… IN (SELECT oid FROM ev_multi_colid)` | `0` | `relation "ev_multi_colid" does not exist`（第 3 条连带） |
| 5 | `sql:136` `SELECT id, name FROM mv_enc` | `1\|Alice` | 0 行 |
| 6 | `sql:235` `DROP MATERIALIZED VIEW mv_enc, mv_enc2, mv_empty CASCADE` | `DROP MATERIALIZED VIEW` | parse failed |

**建议**：多对象 DDL 的解析支持（1/2/6）+ `SELECT … INTO TEMP`（3）为同一类前端缺口；第 5 条（MV 过滤未生效）需单独看。

### C-18 会话临时表未遮蔽同名加密表（`temp_table_reject`）

**现象**：会话 TEMP 表 `t3` 应遮蔽同名加密表 `public.t3`，JDBC 侧未遮蔽 → 未限定名被解析到加密表，密文写入 int 列。
```
sql:45  libpq: INSERT 0 1                                   JDBC: ERROR: invalid input syntax for type integer: "\x被隐藏的HEX输出"
sql:46  libpq: 1|10                                         JDBC: 0 行
```
**对照**：`sql:31` `CREATE TEMP TABLE t1(加密列)` 两侧同样拒绝（仅文案不同）→ 说明 JDBC 侧已有"临时表"概念，但**查表清单未参与未限定名解析**。
**建议**：核对 JDBC 的 `SchemaResolver` 临时表清单是否在 INSERT/SELECT 未限定名解析前生效。

---

### C-19 / C-20 runner 拆句器把注释里的单引号当字符串起点（`guc_backslash`、`guc_string_parsing`）

**现象**：迁移过来的两个用例在 JDBC 侧**整体分叉**——建表/建 DEK 段报错、后续全部断言无输出；
libpq 侧完全正常。
**根因**（测试工具侧）：`TeccSqlTestRunner.splitStatements:2287` 先按引号/美元引用状态扫描，
**之后**才 `stripComments`；注释里的 ASCII `'` 会开启字符串状态，把后续多条真语句吞成一个多语句 blob。
**证据**：`guc_backslash` 复算 24 条真语句只剩 19 条（第 1 条把 6 条 DDL 并成一条）→
`TECC CREATE DEK 'dek_bs': referenced KEK 'kek_bs' does not exist in key manager` + 15 条
`relation "t_backslash" does not exist`；`guc_string_parsing` → pgjdbc
`Unterminated string literal started at position 124/324/32` + `Index 0 out of bounds for length 0`。
**修法**：① 测试资产侧把这两例注释里的 ASCII 单引号全角化（与已做的反斜杠/分号处理一致）；
② 或在 runner 里**先去注释再拆句**（推荐，可一次性消掉本类与注释反斜杠/分号两个变体）。
**复现**：`TECC_JDBC_HOME=… TECC_TEST=guc_backslash.sql make check-tecc-jdbc`

### C-21 `GREATEST` 混合参数（`::varchar` 常量 × 加密列）失败（`scalar_func_crosstype`）

```
sql:190-191  GREATEST(c_text, 'cherry-ish'::varchar), LEAST(...), GREATEST(n_text, ...), LEAST(n_text, ...)
libpq: 成功 2 行（XG-cast-varchar-ontext|cherry-ish|apple|cherry-ish|apple / …|zebra|…）
JDBC : ERROR: ERROR: GREATEST types text_enc and character varying cannot be matched   (Position: 51)
```
**性质**：语句成败 + 结果集不同；JDBC 侧未处理 `::varchar` cast 后的类型匹配。
**复现**：`TECC_JDBC_HOME=… TECC_TEST=scalar_func_crosstype.sql make check-tecc-jdbc`

---

## 观察项：HE 乘法族"游走式偶发失败"（2026-09-18）

同一份代码、不同运行之间，HE 乘法/标量乘断言随机失败，且**每次换用例**：
`homomorphic_enc`(sql:187)、`he_rotate_keep_ciphertext`、`kek_upper_options`(sql:48)、
`scalar_func`（`power(m_int2,3)`）、`pg_dump`（稳定，已随上游 `357187bc4ca` 转绿）。

报错形态：`homomorphic multiplication failed` / `homomorphic power failed` /
`ElGamal scalar multiplication failed`；断言结果行缺失。

**可疑点**：这些用例的 HE 密钥由 `\! tecc_ktool generate --key-algo sm2` **运行期随机生成**，
怀疑存在"部分 SM2 密钥对无法完成 HE 标量乘/乘法"或"密钥材料写入/加载偶发不一致"。
建议优先排查 SM2 生成 → HE 运算这条链。详细证据表见 `shared/jdbc_vs_libpq_diff.md` §十一。
