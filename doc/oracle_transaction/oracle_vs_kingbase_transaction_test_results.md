# Oracle vs KingbaseES 事务兼容实测对比报告

- 日期：2026-11-13
- 状态：实测完成
- 目的：按事务语义逐项对比 Oracle 与 KingbaseES Oracle 模式
- 相关文档：
  - `doc/oracle_transaction/oracle_vs_kingbase_transaction_compare.md`
  - `doc/oracle_transaction/oracle_transaction_test_spec.md`
  - `doc/oracle_transaction/oracle_transaction_compat_plan.md` 第 20 节

---

## 1. 环境

| 项目 | Oracle | KingbaseES |
|---|---|---|
| 版本 | Oracle AI Database 26ai Free 23.26.3.0.0 | KingbaseES V009R001C010 |
| 部署 | Docker `gvenzl/oracle-free:23-slim` | 官方 Docker 镜像 |
| 端口 | 1522 | 54321 |
| 数据库 | `FREEPDB1` | `kingbase` |
| 用户 | `test` | `kingbase` |
| 兼容模式 | Oracle | `database_mode=oracle` |
| 驱动 | `python-oracledb` | `psycopg2` |

说明：

- Oracle 实测版本是 26ai Free，不是 19c；事务语义在本次测试范围内与 Oracle 文档一致。
- Kingbase 主测试集使用 `ora_statement_level_rollback=on`；另单独测试默认 `off`。
- DB-API 驱动会自动开始事务，涉及“无 `BEGIN`”的服务器语义时，另用 `ksql` 验证。

---

## 2. 测试结果总表

| ID | 场景 | Oracle | KingbaseES | 结论 |
|---|---|---|---|---|
| T01 | autocommit on，其他会话可见性 | 可见 [1] | 可见 [1] | 相同 |
| T02 | autocommit off，提交前/后可见性 | 前 [] / 后 [1] | 前 [] / 后 [1] | 相同 |
| T03 | 无事务 `COMMIT` / `ROLLBACK` | 无异常 | 无异常 | 相同（DB-API 自动 BEGIN） |
| T04 | 显式事务中失败 DML，继续插入并提交 | 行 [1,2] | 事务 aborted，行 [] | **不同** |
| T05 | 显式事务中 `SELECT 1/0`，继续 DML | 行 [1,2] | 事务 aborted，行 [] | **不同** |
| T06 | 事务中先成功 SQL，再失败匿名块（RAISE），再成功 SQL | 行 [1,3] | 事务 aborted，行 [] | **不同** |
| T07 | 事务中先成功 SQL，再失败匿名块（SQL 错误），再成功 SQL | 行 [1,3] | 事务 aborted，行 [] | **不同** |
| T08 | handler 捕获异常（Kingbase `on`） | 行 [1,3] | 行 [1,3] | 相同 |
| T08O | handler 捕获异常（Kingbase 默认 `off`） | — | 行 [3] | Kingbase 默认整块回滚 |
| T09 | 嵌套异常块 | 行 [1,2,3] | 行 [1,2,3] | 相同 |
| T10 | handler 自身抛异常，外层 handler | 行 [2,3] | 行 [2,3] | 相同 |
| T11 | 异常块内 `COMMIT` | 行 [1,2] | 行 [1,2] | 相同 |
| T12 | 异常块内 `ROLLBACK` | 行 [2] | 行 [2] | 相同 |
| T13 | 过程内异常块 + handler `COMMIT` | 行 [1,2,3] | 行 [1,2,3] | 相同 |
| T14 | 过程体 `COMMIT` + 异常段 | 行 [4] | 行 [4] | 相同 |
| T15 | 事务中先成功 SQL，再调用失败过程，再成功 SQL | 行 [1,3] | 事务 aborted，行 [] | **不同** |
| T16 | PL/SQL `SAVEPOINT` / `ROLLBACK TO` | 行 [1,3] | 行 [1,3] | 相同 |
| T17 | SQL `SAVEPOINT` 无 `BEGIN` | 可用，行 [] | DB-API 自动 BEGIN；原生 `ksql` 报错 | **不同** |
| T18 | 显式事务中 SQL savepoint + PL/SQL `ROLLBACK TO` | 行 [1,3] | `invalid transaction termination`，行 [] | **不同** |
| T19 | PL/SQL savepoint + 过程返回后 SQL `ROLLBACK TO` | 行 [1,3] | `savepoint does not exist`，行 [] | **不同** |
| T20 | DDL 隐式提交 | 行 [5]，表存在 | 行 []，表不存在 | **不同** |
| T21 | TRUNCATE 隐式提交 | 行 [] | 行 [1,2] | **不同** |
| T22 | `SET TRANSACTION READ ONLY` 快照 | first=1 / second=1 / id2=0 | first=1 / second=2 / id2=1 | **不同** |
| T23 | `SET TRANSACTION ISOLATION LEVEL SERIALIZABLE` | first=1 / second=1 | first=1 / second=1 | 相同（快照读） |
| T24 | 自治事务提交，主事务回滚 | main [] / log [1] | main [] / log [1] | 相同 |
| T25 | 自治事务未处理异常 | main [1,3] / log [] | main [] / log [] | **不同** |
| T26 | 调用者 handler 捕获嵌套过程异常 | 行 [1,2,3,4] | 行 [1,2,3,4] | 相同（Kingbase `on`） |

---

## 3. 补充测试

| ID | 场景 | Oracle | KingbaseES | 结论 |
|---|---|---|---|---|
| S1 | `ROLLBACK TO` 后目标 savepoint 可再次使用 | [1,3] | [1,3] | 相同 |
| S2 | 同名 savepoint | [1,2,3] | [1,2,3] | 相同 |
| S3 | `COMMIT AND CHAIN` | ORA-02185 语法错误 | [1,2] 成功 | **不同** |
| S4 | 函数内 `COMMIT` | ORA-14551 | `invalid transaction termination` | 语义相同，错误码不同 |
| S5 | 触发器内 `COMMIT` | ORA-04092 | `invalid transaction termination` | 语义相同，错误码不同 |
| S6 | 普通 `FORALL` 中途失败（BULK In-BIND） | ORA-00001，整体回滚，行 [] | 唯一键冲突，事务 aborted，行 [] | 整体回滚语义相同 |
| S7 | SQL `SAVEPOINT` 无 `BEGIN`（原生 `ksql`） | 可用 | 报错 `SAVEPOINT can only be used in transaction blocks` | **不同** |
| S8 | Simple Query 多语句失败 | 协议层不适用 | 整体回滚 | Kingbase 为 PG 风格 |

---

## 4. 关键结论

### 4.1 Oracle 语义基线

1. **顶层 SQL 语句原子性**
   - 失败 SQL 语句只回滚自己；
   - 之前成功的 SQL 语句保留；
   - 事务保持可用。

2. **匿名块 / 顶层 `CALL` 是顶层 SQL 语句**
   - 未处理异常回滚整个失败的匿名块 / 顶层 `CALL`；
   - 事务中之前成功的 SQL 语句保留；
   - 不是“保留块内失败语句之前的所有内层 DML”。
   - 异常被调用者 handler 捕获时，只回滚失败的内层 SQL 语句；callee 之前成功 DML 保留。

3. **handler 捕获异常**
   - 只回滚失败的内层 SQL 语句；
   - 块内之前成功的语句保留。

4. **DDL / TRUNCATE 隐式提交**
   - DDL 前/后隐式提交；
   - TRUNCATE 不受后续 ROLLBACK 影响。

5. **事务级快照**
   - `SET TRANSACTION READ ONLY` 和 `SERIALIZABLE` 提供事务级读一致性。

6. **跨层 savepoint**
   - SQL 和 PL/SQL 共享事务作用域；
   - savepoint 可跨层使用。

7. **自治事务**
   - 独立提交/回滚；
   - 未处理异常回滚自治事务；
   - 主事务保持可用。

### 4.2 KingbaseES 主要差距

1. 顶层 SQL 失败会导致整个事务 aborted；
2. `ora_statement_level_rollback=on` 只对 PL/SQL handler 捕获场景有效；
3. 未处理异常会导致整个事务 aborted，之前事务语句丢失；
4. DDL / TRUNCATE 不隐式提交；
5. `SET TRANSACTION READ ONLY` 不是事务级快照；
6. SQL `SAVEPOINT` 无 `BEGIN` 报错；
7. SQL / PL/iSQL 跨层 savepoint 受限；
8. 多语句 Simple Query 仍是 PG 风格整体事务。

### 4.3 与我们方案的关系

此前设计中的一个理解需要修正：

> 不是“未处理异常只回滚失败的内层语句，块内之前 DML 保留”。

正确目标：

> 未处理异常回滚失败的顶层 SQL 语句（匿名块 / `CALL` / `DO`）；  
> 事务中之前成功的 SQL 语句保留；  
> 只有 handler 捕获时，才只回滚失败的内层 SQL 语句。

对应设计调整：

- 顶层语句帧覆盖整个顶层 SQL 语句；
- 叶子语句帧只用于 handler 捕获场景；
- 未处理异常回滚顶层语句帧，保留事务中之前 SQL；
- KingbaseES 作为下限参考，不作为 Oracle 语义证明。

> v2.4 设计决策：上述严格语句级语义默认关闭；需要 `ivorysql.ora_statement_level_rollback=on` 才启用。默认 `off` 时行为接近 Kingbase/PG。

---

## 5. 限制与后续

1. Oracle 实测版本为 26ai Free，不是 19c；如需 19c 精确基线，需要重新部署 19c。
2. DB-API 驱动会自动开始事务，涉及“无 `BEGIN`”的测试需使用原生客户端。
3. Oracle `FORALL` 已改用 BULK In-BIND 集合测试，结果见补充测试 S6。
4. Extended Query / Portal 交错主要适用于 Kingbase；Oracle 协议层不适用。
5. 性能、连接池和 JDBC fetchsize 未在本次报告中覆盖。
6. 后续应按 `doc/oracle_transaction/oracle_transaction_test_spec.md` 固化回归测试，并将本次结果作为行为基线。

---

## 6. 原始结果

- 主测试集：`/tmp/tx_compare_results.json`
- 补充测试：`/tmp/tx_supp_results.json`
- 运行日志：`/tmp/tx_compare_run2.log`、`/tmp/tx_supp_run.log`