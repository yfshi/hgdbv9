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
