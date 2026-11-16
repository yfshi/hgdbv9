# Oracle 事务处理与 KingbaseES Oracle 模式兼容程度对比

- 日期：2026-11-13
- 状态：文档对比，非实测报告
- 目的：为当前项目的 Oracle 事务兼容方案提供外部参照
- 参考产品：Oracle Database 19c、KingbaseES V9R1C10（Oracle 模式）

---

## 1. 说明

本文档基于公开官方文档做对比：

- Oracle Database Concepts / SQL Reference / PL/SQL Language Reference
- KingbaseES V9R1C10 官方文档：
  - Oracle 兼容性说明
  - PL/SQL 事务与自治事务
  - SQL 参考（Oracle 模式）
  - 兼容性配置参数

**当前已部署 Oracle Free 23ai 和 KingbaseES V9R1C10 Docker 实例，本文包含文档对比和实测结果。**
第 9 节为实测结果；其余章节保留文档对比背景。

---

## 2. 结论摘要

1. **KingbaseES 的 Oracle 兼容面很广**，包括 SQL 语法、PL/SQL、数据类型、包、触发器等，官方宣称“Oracle 常用能力兼容性已达 100%”。
2. **但事务语义不是默认完全 Oracle 化**：
   - 语句级回滚依赖 `ora_statement_level_rollback`；
   - 该参数默认 `off`；
   - 关闭时，PL/SQL 中出现异常会回滚整个事务；
   - 打开时，只回滚失败的 SQL 语句。
3. **KingbaseES 的 SQL 事务模型仍然是 PostgreSQL 风格**：
   - `SET TRANSACTION` 文档明确要求先 `START TRANSACTION` / `BEGIN`；
   - 没有明确承诺 Oracle 的“第一条可执行 SQL 隐式开启事务、COMMIT 后下一条 SQL 自动开启新事务”模型。
4. **KingbaseES 支持 PL/SQL COMMIT / ROLLBACK / SAVEPOINT**，并支持自治事务。
5. **KingbaseES 自治事务的异常处理比我们当前方案更严格**：
   - 文档说明自治事务中未处理异常会导致事务级回滚；
   - 我们当前方案允许自治事务复用语句级帧，倾向于更宽松。
6. **DDL 隐式提交**：实测 KingbaseES 不隐式提交；Oracle 隐式提交。
7. **`SET TRANSACTION READ ONLY`**：实测 KingbaseES 不是事务级快照；Oracle 是事务级快照。

---

## 3. Oracle 事务处理要点

### 3.1 事务起点与结束

- 事务在第一条可执行 SQL 语句时开始；
- `COMMIT` / `ROLLBACK` 结束事务；
- 结束后下一条可执行 SQL 自动开始新事务；
- 普通 `SELECT` 不产生事务 ID，但属于事务上下文；
- DDL 执行前和执行后隐式提交。

### 3.2 语句级原子性

Oracle 的 statement-level atomicity：

- 失败的 SQL 语句只回滚自己；
- 不回滚该语句之前已经成功的工作；
- 触发器副作用视为语句的一部分；
- 错误后事务保持可用。

### 3.3 PL/SQL 异常

- Oracle 的 statement-level atomicity 以“SQL 语句”为单位；
- 匿名 PL/SQL 块、`CALL` 过程调用本身都是一个顶层 SQL 语句；
- 未处理异常会回滚整个失败的匿名块 / `CALL` 语句；
- 事务中之前成功的 SQL 语句保留；
- 异常被 `EXCEPTION` handler 捕获时，只回滚失败的内层 SQL 语句，块内之前成功的语句保留；
- 异常处理程序自身的异常向外层传播；
- 事务保持可用。

### 3.4 SAVEPOINT

- `SAVEPOINT name` 创建保存点；
- `ROLLBACK TO SAVEPOINT name` 回滚到保存点；
- 目标保存点保留；
- 目标之后的保存点失效；
- 回滚到保存点会释放保存点之后获取的表锁和行锁；
- 保存点是事务作用域的。

### 3.5 自治事务

- 独立事务边界；
- 不共享主事务的锁、资源、提交依赖；
- 提交后对其他事务可见；
- 主事务回滚不影响自治事务；
- 自治事务中未处理异常通常导致该自治事务回滚。

### 3.6 隔离级别

- 默认 READ COMMITTED；
- 支持 SERIALIZABLE；
- 支持 READ ONLY；
- READ ONLY 和 SERIALIZABLE 提供事务级读一致性。

### 3.7 事务控制语法

- `COMMIT [WORK] [COMMENT ...] [WRITE ...] [FORCE ...]`
- `ROLLBACK [WORK] [TO [SAVEPOINT] name] [FORCE ...]`
- `SET TRANSACTION` 必须是事务中的第一条语句。

---

## 4. KingbaseES 文档观察

### 4.1 语句级回滚

KingbaseES 文档“事务与自治事务”章节说明：

> 在运行 INSERT、UPDATE、DELETE 或 MERGE 语句之前，数据库会标记一个隐式保存点（对用户不可用）。如果语句失败，数据库将回滚到保存点。

> KingbaseES 数据库需要通过 `ora_statement_level_rollback` 系统参数来设置是否启动单语句回滚的功能，默认是不启动该功能。

> 当参数未启用时，如果事务中出现了异常，会回滚整个事务。当参数启用时，只是回滚失败的 SQL 语句，而不是整个事务。

兼容性参数文档：

| 参数 | 类型 | 默认值 | 级别 | 说明 |
|---|---|---|---|---|
| `ora_statement_level_rollback` | bool | `off` | USER | 是否启用 PL/SQL 语句级回滚 |

结论：

- KingbaseES 的语句级回滚是**可选能力**，默认关闭；
- 默认行为更接近 PostgreSQL：异常回滚整个事务；
- 只有显式打开参数后，才接近 Oracle 的语句级原子性。

### 4.2 COMMIT / ROLLBACK

KingbaseES SQL 参考中的 `COMMIT` 语法：

```sql
COMMIT [ WORK | TRANSACTION ] [ AND [ NO ] CHAIN ]
```

- 与 PostgreSQL 风格一致；
- 文档中未看到 Oracle 风格的 `COMMIT COMMENT` / `COMMIT WRITE` / `COMMIT FORCE`；
- PL/SQL 章节示例中允许在过程、匿名块中使用 `COMMIT` / `ROLLBACK`。

### 4.3 SAVEPOINT

KingbaseES SQL 参考支持：

- `SAVEPOINT`
- `RELEASE SAVEPOINT`
- `ROLLBACK TO SAVEPOINT`

PL/SQL 章节在描述自治事务时也提到 `SAVEPOINT` 是当前事务可用的控制语句之一。

结论：

- SQL 层明确支持 savepoint；
- PL/SQL 静态语法是否完整支持 `SAVEPOINT` / `ROLLBACK TO`，需要实测确认。

### 4.4 自治事务

KingbaseES 文档说明：

- 自治事务由主事务启动，但独立提交/回滚；
- 不共享主事务锁和资源；
- 主事务回滚不影响自治事务；
- 自治事务提交后对其他事务可见；
- 自治事务中未处理异常会导致**事务级回滚**，而不是语句级回滚；
- 支持 `PRAGMA AUTONOMOUS_TRANSACTION`；
- 支持在包、过程、函数、匿名块中使用；
- 自治事务内可以使用 `COMMIT` / `ROLLBACK` / `SAVEPOINT`。

### 4.5 SET TRANSACTION

PL/SQL 章节描述：

- `SET TRANSACTION READ ONLY` 提供事务级快照；
- 只允许 `SELECT`、`OPEN`、`FETCH`、`CLOSE`、`LOCK TABLE`、`COMMIT`、`ROLLBACK`；
- 只读事务中 `SELECT` 不能使用 `FOR UPDATE`；
- `SET TRANSACTION` 在事务中只能出现一次。

SQL 参考章节描述：

- 仍是 PostgreSQL 风格；
- 如果没有 `START TRANSACTION` / `BEGIN`，会发出警告并且不生效；
- 支持 `READ COMMITTED` / `REPEATABLE READ` / `SERIALIZABLE` / `READ ONLY`；
- `SERIALIZABLE` 使用类似 PostgreSQL 的 serialization failure 机制。

结论：

- KingbaseES 文档内部存在不一致；
- 需要实测确认 Oracle 模式下 `SET TRANSACTION READ ONLY` 的实际行为。

### 4.6 Portal / autocommit 相关参数

KingbaseES 提供：

| 参数 | 默认值 | 说明 |
|---|---|---|
| `enable_autocommit_fetch` | off | 客户端 autocommit=true 时，是否按 fetchsize 按需执行查询 |
| `enable_autocommit_crossquery` | off | 客户端语句交叉执行时，前语句是否通过 HoldablePortal 继续执行 |

参考意义：

- KingbaseES 也关注 Extended Query / JDBC fetchsize / 语句交叉执行；
- 默认关闭 crossquery，说明默认不允许前语句在后续语句执行后继续；
- 这与我们对“两个 PortalFrame Portal 交错执行”的限制方向一致。

---

## 5. 对比矩阵

| 场景 | Oracle | KingbaseES（文档） | 当前仓库 / 目标 | 备注 |
|---|---|---|---|---|
| 事务起点 | 第一条可执行 SQL | SQL 文档仍要求 `BEGIN` | 目标：`ora_autocommit=off` 隐式开启 | Kingbase 未明确承诺 Oracle 隐式事务 |
| 默认 autocommit | 客户端控制 | PG 风格 | 目标：`ora_autocommit=on` 默认 | 产品策略不同 |
| 语句级回滚 | 默认支持 | `ora_statement_level_rollback` 默认 off | 目标：默认 off，严格语义 opt-in | 与 Kingbase 默认策略一致 |
| 显式事务中失败语句 | 只回滚失败语句 | 参数 on 时同样 | 目标一致 | 需实测 |
| 未处理异常 | 回滚失败的顶层块/调用，事务中之前 SQL 保留 | 回滚整个事务或整个块 | 目标：同 Oracle | Kingbase 实测不保留之前事务语句 |
| PL/SQL 异常块（handler 捕获） | 只回滚失败的内层语句 | 参数 on 时接近；off 时整事务 | 目标一致 | 需实测 |
| PL/SQL COMMIT/ROLLBACK | 支持 | 文档示例支持 | 目标支持 | 当前仓库缺少块内 COMMIT |
| PL/SQL SAVEPOINT | 支持 | 文档显示支持 | 目标支持 | 当前仓库缺少静态语法 |
| 用户 savepoint 作用域 | 事务级 | 事务级 | 目标一致 | 需实测跨过程行为 |
| ROLLBACK TO 保留目标 | 是 | 文档支持 | 目标一致 | 需实测 |
| 自治事务 | 独立事务 | 支持 | 目标支持 | 当前已有基础 |
| 自治事务未处理异常 | 回滚自治事务 | 文档：事务级回滚 | 当前目标偏宽松 | 需要决定是否对齐 |
| DDL 隐式提交 | 是 | 实测：不隐式提交 | 本期不做 | Kingbase 与 Oracle 不同 |
| TRUNCATE 隐式提交 | 是 | 实测：不隐式提交 | 本期不做 | Kingbase 与 Oracle 不同 |
| SET TRANSACTION READ ONLY | 事务级快照 | 实测：不是事务级快照 | 本期保持 PG | Kingbase 与 Oracle 不同 |
| SQL SAVEPOINT 无 BEGIN | 可用 | 实测：报错 | 目标支持 | Kingbase 不支持 |
| SQL/PL/iSQL 跨层 savepoint | 可用 | 实测：受限 | 目标支持 | Kingbase 不完整 |
| SERIALIZABLE | Oracle 快照 + ORA-08177 | SQL 文档：PG SSI 风格 | 本期保持 PG | 需实测 |
| COMMIT 语法 | COMMENT/WRITE/FORCE | PG 风格 + AND CHAIN | 目标接受基础语法 | Kingbase 也未完整实现 Oracle 语法 |
| 多语句独立提交 | 客户端逐语句 | 未明确 | 本期接受一个消息一个事务单元 | 主动放宽 |
| Portal 交错 | 语义取决于客户端/游标 | `enable_autocommit_crossquery` 默认 off | 目标限制两个 PortalFrame 交错 | 方向接近 |

---

## 6. 对当前方案的影响

### 6.1 KingbaseES 可以作为“下限参照”，但不能作为“Oracle 语义证明”

KingbaseES 的 Oracle 兼容性更多体现在：

- 语法；
- 数据类型；
- PL/SQL 结构；
- 包、触发器；
- 常用函数和视图。

事务语义方面：

- 语句级回滚是可选参数；
- 默认仍是 PostgreSQL 风格；
- SQL 事务模型没有明显改成 Oracle 隐式事务模型。

因此：

> 不能因为 KingbaseES 宣称 100% Oracle 兼容，就认为其事务语义与 Oracle 完全一致。

### 6.2 当前方案与 Kingbase 默认行为的关系

当前方案目标：

- `ora_statement_level_rollback` 默认 `off`，与 Kingbase 默认策略一致；
- 显式设置 `on` 后启用严格语句级回滚、未处理异常顶层语句回滚、叶子语句帧；
- `ora_autocommit` 支持 Oracle 风格持续事务；
- PL/SQL SAVEPOINT / COMMIT / ROLLBACK；
- PortalFrame 生命周期管理。

因此：

- 默认配置下，事务回滚行为接近 Kingbase/PG；
- 严格 Oracle 语义需要显式开启 `ora_statement_level_rollback=on`。

### 6.3 需要重新评估的点

1. **自治事务未处理异常**
   - KingbaseES：事务级回滚；
   - 当前方案：语句级帧，偏宽松；
   - 是否需要对齐 Kingbase/Oracle，需要产品确认。

2. **SET TRANSACTION READ ONLY**
   - KingbaseES PL/SQL 文档声称事务级快照；
   - 当前方案保持 PG 行为；
   - 如果产品要求对标 Kingbase，需要实测并评估是否实现事务级只读快照。

3. **DDL 隐式提交**
   - KingbaseES 文档未明确；
   - 当前方案本期不做；
   - 需要实测 Kingbase 行为，作为后续版本参考。

4. **多语句独立提交**
   - 当前方案接受“一个 Query 消息一个事务单元”；
   - 与 Oracle 客户端逐语句 autocommit 有差异；
   - 已确认作为本期限制。

---

## 7. 后续实测清单

环境可用后，建议优先测试以下用例：

1. `ora_statement_level_rollback=off/on` 下：
   - 显式事务中 `INSERT 1`、失败、`INSERT 2`；
   - PL/SQL 异常块；
   - 未处理异常。
2. `COMMIT` / `ROLLBACK` 在带 `EXCEPTION` 的 PL/SQL 块内是否可用。
3. PL/SQL 静态 `SAVEPOINT` / `ROLLBACK TO SAVEPOINT` 是否支持。
4. 用户 savepoint 在过程返回后是否仍可回滚。
5. 自治事务未处理异常是语句级还是事务级回滚。
6. `SET TRANSACTION READ ONLY` 是否提供事务级快照。
7. `SERIALIZABLE` 冲突行为。
8. DDL / `TRUNCATE` 是否隐式提交。
9. Simple Query 多语句失败后的提交/回滚行为。
10. Extended Query `Execute(max_rows)` 分批执行和交叉执行。

测试 SQL 可参考：

- `doc/oracle_transaction/oracle_transaction_test_spec.md`
- 本文第 3、4 节的行为描述

---

## 8. 参考链接

Oracle：

- Oracle Database Concepts: Transactions
- Oracle Database Concepts: Data Concurrency and Consistency
- Oracle Database SQL Language Reference: COMMIT / ROLLBACK / SAVEPOINT
- Oracle Database PL/SQL Language Reference: Exception Handling

KingbaseES V9R1C10：

- Oracle 兼容性总览：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/kes-vs-oracle/overview`
- 事务与自治事务：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/reference/oracle/plsql/plsql-transacion-autonomous-transaction`
- 兼容性配置参数：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/administration/Config_Mgmt/runtime-config-compatible`
- SET TRANSACTION：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/reference/oracle/sql/statement/set_transaction`
- COMMIT：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/reference/oracle/sql/statement/commit`
- SAVEPOINT：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/reference/oracle/sql/statement/savepoint`
- ROLLBACK TO SAVEPOINT：
  `https://docs.kingbase.com.cn/cn/KES-V9R1C10/application/application-develop-guide/reference/oracle/sql/statement/rollback_to_savepoint`

---

## 9. KingbaseES 实测结果（V9R1C10 Docker）

### 9.1 环境

- 产品：KingbaseES V009R001C010
- 部署方式：官方 Docker 镜像
- `database_mode`：`oracle`
- 授权：开发版 365 天 license
- 默认 `ora_statement_level_rollback`：`off`
- 访问方式：`ksql` / `psycopg2`

> Oracle 环境仍在部署中；以下为 KingbaseES 实测结果。

### 9.2 实测矩阵

| 编号 | 场景 | 实测结果 |
|---|---|---|
| KB-01 | 显式事务中失败 DML，`ora_statement_level_rollback=off` | 事务 aborted，后续语句不可执行 |
| KB-02 | 显式事务中失败 DML，`ora_statement_level_rollback=on` | 仍然 aborted；该参数对顶层 SQL 无效 |
| KB-03 | PL/SQL 异常块，`off` | 回滚整个块，只保留 handler 中的 DML |
| KB-04 | PL/SQL 异常块，`on` | 保留失败语句之前的 DML，只回滚失败语句 |
| KB-05 | 未处理 `RAISE`，`off` | 整个匿名块回滚 |
| KB-06 | 未处理 `RAISE`，`on` | 仍然整个匿名块回滚，之前 DML 不保留 |
| KB-07 | 未处理 SQL 错误，`on` | 整个匿名块回滚，之前 DML 不保留 |
| KB-08 | 嵌套异常块，`on` | 1、2、3 保留 |
| KB-09 | handler 自身抛异常，外层 handler，`on` | 2、3 保留 |
| KB-10 | 异常块内 `COMMIT`，`on` | 提交成功，1、2 保留 |
| KB-11 | PL/SQL 顶层 `SAVEPOINT` / `ROLLBACK TO` | 成功，1、3 保留 |
| KB-12 | SQL `SAVEPOINT` 无 `BEGIN` | 报错：`SAVEPOINT can only be used in transaction blocks` |
| KB-13 | SQL `SAVEPOINT` 后再 `INSERT` | `INSERT` 自动提交 |
| KB-14 | DDL 隐式提交 | 未观察到；`CREATE TABLE` 可被 `ROLLBACK` 回滚 |
| KB-15 | `TRUNCATE` 隐式提交 | 未观察到；`ROLLBACK` 可恢复数据 |
| KB-16 | `SET TRANSACTION READ ONLY` | 第二次查询能看到其他会话新提交的行，不是事务级快照 |
| KB-17 | `SET TRANSACTION ISOLATION LEVEL SERIALIZABLE` | 事务内两次查询看到同一快照 |
| KB-18 | 自治事务 `COMMIT` | 独立提交，主事务 `ROLLBACK` 不影响 |
| KB-19 | 自治事务未处理异常 | 自治事务整体回滚，主事务 handler 正常执行 |
| KB-20 | 显式事务中 `CALL` 过程，过程内 `COMMIT` | 成功 |
| KB-21 | 过程内异常块 + handler `COMMIT` | 成功，1、2、3 保留 |
| KB-22 | 过程体 `COMMIT` + 异常段 | 成功，4 保留 |
| KB-23 | 显式事务中 SQL savepoint + PL/SQL `ROLLBACK TO` | 报错：`invalid transaction termination` |
| KB-24 | PL/SQL savepoint + 过程返回后 SQL `ROLLBACK TO` | 报错：`savepoint "psp" does not exist` |
| KB-25 | 多语句 simple Query | 整体作为一个事务，失败时全部回滚 |
| KB-26 | 单条语句失败，autocommit | 之前语句已提交，失败语句回滚，后续不执行 |

### 9.3 Oracle 实测结果

环境：

- Oracle AI Database 26ai Free Release 23.26.3.0.0
- 镜像：`gvenzl/oracle-free:23-slim`
- PDB：`FREEPDB1`
- 端口：`1522`
- 用户：`test`

| 编号 | 场景 | 实测结果 |
|---|---|---|
| OR-01 | 显式事务中失败 DML，随后继续 `INSERT` 并 `COMMIT` | 失败语句回滚，前后语句保留 |
| OR-02 | 显式事务中 `SELECT 1/0`，随后继续 DML | 失败 SELECT 回滚，事务可用 |
| OR-03 | 匿名块内异常被 handler 捕获 | 只回滚失败的内层语句，块内之前 DML 保留 |
| OR-04 | 匿名块内未处理 `RAISE` | 整个匿名块回滚 |
| OR-05 | 事务中先 `INSERT 1`，再执行失败匿名块，再 `INSERT 3`、`COMMIT` | `1`、`3` 保留，匿名块内 DML 回滚 |
| OR-06 | 存储过程未处理异常，调用前已有事务语句 | 整个 `CALL` 语句回滚，之前事务语句保留 |
| OR-07 | 嵌套异常块 | 1、2、3 保留 |
| OR-08 | handler 自身抛异常，外层 handler | 2、3 保留 |
| OR-09 | 异常块内 `COMMIT` | 提交成功 |
| OR-10 | PL/SQL `SAVEPOINT` / `ROLLBACK TO` | 成功 |
| OR-11 | SQL `SAVEPOINT` 无 `BEGIN` | 可用，隐式开启事务 |
| OR-12 | DDL 隐式提交 | `CREATE TABLE` 前的 DML 被提交，表保留 |
| OR-13 | `TRUNCATE` 隐式提交 | `TRUNCATE` 不被 `ROLLBACK` 撤销 |
| OR-14 | `SET TRANSACTION READ ONLY` | 事务级快照，第二次查询看不到其他会话新提交的行 |
| OR-15 | `SET TRANSACTION ISOLATION LEVEL SERIALIZABLE` | 事务级快照 |
| OR-16 | 自治事务提交 | 独立提交，主事务回滚不影响 |
| OR-17 | 自治事务未处理异常 | 自治事务整体回滚，主事务继续 |
| OR-18 | SQL savepoint + PL/SQL `ROLLBACK TO` | 成功 |
| OR-19 | PL/SQL savepoint + 过程返回后 SQL `ROLLBACK TO` | 成功 |
| OR-20 | 多语句 simple Query | Oracle 协议层不适用 |

### 9.4 Oracle vs Kingbase 关键差异

| 场景 | Oracle | KingbaseES | 说明 |
|---|---|---|---|
| 顶层 SQL 失败 | 只回滚失败语句，事务可用 | 事务 aborted | Kingbase 的 `ora_statement_level_rollback` 对顶层 SQL 无效 |
| 匿名块 handler 捕获 | 只回滚失败内层语句 | 参数 `on` 时接近 | Kingbase 默认 `off` 时回滚整块 |
| 匿名块未处理异常 | 回滚整个匿名块，保留事务中之前 SQL | 回滚整个事务，之前 SQL 丢失 | 核心差异 |
| 存储过程未处理异常 | 回滚整个 `CALL` 语句，保留事务中之前 SQL | 回滚整个事务或块 | 核心差异 |
| SQL SAVEPOINT 无 `BEGIN` | 可用 | 报错 | Kingbase 没有 Oracle 隐式事务 |
| DDL 隐式提交 | 是 | 否 | 实测差异 |
| TRUNCATE 隐式提交 | 是 | 否 | 实测差异 |
| `SET TRANSACTION READ ONLY` | 事务级快照 | 不是事务级快照 | 实测差异 |
| 跨 SQL/PL/iSQL savepoint | 可用 | 受限 | Kingbase 不完整 |
| 自治事务未处理异常 | 回滚自治事务 | 回滚自治事务 | 行为接近 |
| 多语句 simple Query | 协议层不适用 | 整体回滚 | Kingbase 仍是 PG 风格 |

### 9.5 对当前方案的启示

1. **之前对 Oracle 未处理异常的理解需要修正**
   - Oracle 未处理异常回滚的是“失败的顶层 SQL 语句”，包括匿名块或 `CALL`；
   - 它不会保留失败匿名块内部的 DML；
   - 它保留的是事务中在该失败语句之前已经成功执行的 SQL 语句。

2. **顶层语句帧必须覆盖整个顶层 SQL 语句**
   - 包括 `CALL` / `DO` / 匿名块；
   - 不能只给叶子 SQL 建帧，否则未处理异常时无法完整回滚顶层块。

3. **叶子语句帧只用于 handler 捕获场景**
   - 当异常被 `EXCEPTION` handler 捕获时，失败的内层 SQL 语句只回滚自己；
   - 块内之前成功的语句保留。

4. **KingbaseES 是下限参考，不是 Oracle 语义证明**
   - Kingbase 在未处理异常、DDL/TRUNCATE 隐式提交、READ ONLY 快照、跨层 savepoint 上均与 Oracle 不同；
   - 当前方案的部分目标需要按 Oracle 实测结果修正，而不是按 Kingbase 文档或早期 Oracle 文档片段推断。

---

## 10. Oracle / KingbaseES / HGDB（规划）能力对比

> HGDB 列对应当前设计 v2.4：`ora_statement_level_rollback` 默认 `off`，严格语句级语义需要显式设为 `on`；`ora_autocommit` 默认 `on`。

| 能力 / 场景 | Oracle | KingbaseES V9R1C10 | HGDB（规划 v2.4） | 说明 |
|---|---|---|---|---|
| 事务起点 | 第一条可执行 SQL 隐式开启 | 需要显式 `BEGIN` / `START TRANSACTION` | `ora_autocommit=off` 时第一条可执行 SQL 隐式开启 | HGDB 补齐 Oracle 隐式事务 |
| 默认 autocommit | 客户端控制 | PG 风格 | `ora_autocommit=on` 模拟客户端 autocommit | 三者定位不同 |
| 显式 `BEGIN` | 不作为 SQL 事务控制 | 支持 | 兼容接受，并覆盖自动提交 | HGDB 保留 PG 客户端兼容 |
| 顶层 SQL 失败 | 只回滚失败语句，事务可用 | 事务 aborted | `off`：事务 aborted；`on`：只回滚失败语句 | HGDB 严格语义需 `on` |
| `SELECT 1/0` 失败 | 只回滚当前语句，事务可用 | 事务 aborted | `off`：事务 aborted；`on`：只回滚当前语句 | 同上 |
| PL/SQL handler 捕获异常 | 只回滚失败内层语句 | `off`：整块回滚；`on`：只回滚失败内层语句 | `off`：整块回滚；`on`：只回滚失败内层语句 | HGDB 与 Kingbase 接近 |
| 未处理异常 | 回滚失败的顶层语句，事务中之前 SQL 保留，事务可用 | 事务 aborted，之前事务 SQL 丢失 | `off`：事务 aborted；`on`：回滚失败顶层语句，之前事务 SQL 保留 | HGDB 多出严格语义 |
| 嵌套过程未处理异常 | 失败顶层语句回滚，之前事务 SQL 保留 | 事务 aborted | `on` 时同 Oracle | HGDB 多出 |
| 嵌套过程被 handler 捕获 | 只回滚失败内层语句，callee 之前 DML 保留 | `on` 时同 Oracle | `on` 时同 Oracle | 基本一致 |
| PL/SQL `COMMIT` / `ROLLBACK` | 支持 | 支持 | 支持 | 基本一致 |
| 异常块内 `COMMIT` | 支持 | 支持 | 支持 | Kingbase 已放宽 |
| 函数 / 触发器内 `COMMIT` | 禁止 | 禁止 | 禁止 | 基本一致 |
| SQL `SAVEPOINT` 无 `BEGIN` | 支持 | 报错 | 支持 | HGDB 多出 |
| PL/SQL `SAVEPOINT` / `ROLLBACK TO` | 支持 | 支持 | 支持 | 基本一致 |
| SQL savepoint + PL/SQL `ROLLBACK TO` | 支持 | 报 `invalid transaction termination` | 支持 | HGDB 多出 |
| PL/SQL savepoint + 过程返回后 SQL `ROLLBACK TO` | 支持 | savepoint 不存在 | 支持 | HGDB 多出 |
| 用户 savepoint 生命周期 | 事务作用域 | 事务作用域，但跨层受限 | 事务作用域，SQL/PLiSQL 共享 | HGDB 更完整 |
| 用户 savepoint 与内部帧隔离 | 用户不可见内部机制 | 使用隐式 savepoint，未明确隔离 | 显式帧栈，用户帧/内部帧分离 | HGDB 设计更严谨 |
| 自治事务 | 支持 | 支持 | 支持，统一帧 root | 基础能力相当 |
| 自治事务未处理异常 | 回滚自治事务，主事务可用性由调用者决定 | 回滚自治事务；主事务可用性受传播影响 | 回滚自治事务，主事务按顶层语句规则处理 | 设计目标接近 |
| DDL 隐式提交 | 支持 | 不支持 | 本期不实现 | 与 Kingbase 相同 |
| `TRUNCATE` 隐式提交 | 支持 | 不支持 | 本期不实现 | 与 Kingbase 相同 |
| `SET TRANSACTION READ ONLY` | 事务级快照 | 非事务级快照 | 保持 PG 行为 | 三者不同 |
| `SERIALIZABLE` | 事务级快照 + ORA-08177 | PG SSI 风格 | 保持 PG SSI | 语义不同 |
| `COMMIT AND CHAIN` | 不支持 | 支持 | 作为兼容扩展支持 | 与 Oracle 不同 |
| `COMMIT COMMENT` / `WRITE` / `FORCE` | 支持 | 不支持 | 部分语法可接受，语义不完整 | HGDB 语法面稍宽 |
| 多语句 Simple Query | 协议层不适用 | 整体事务，全成全败 | 一个事务单元 + 逐语句失败隔离（`on` 时） | HGDB 不实现逐语句提交 |
| Extended Query / Portal 语句帧 | 协议层不适用 | 有 fetch/crossquery 参数，无统一语句帧模型 | PortalFrame + Per-Execute 帧 | HGDB 多出 |
| `PORTAL_MULTI_QUERY` 逐语句帧 | 协议层不适用 | 未明确 | 支持 | HGDB 多出 |
| `FORALL` 整体回滚 | 支持 | 支持（普通 FORALL） | 规划支持 | 基本一致 |
| `FORALL SAVE EXCEPTIONS` | 支持 | 未完整验证 | 本期不实现 | 需后续补充 |
| 默认语句级回滚开关 | 原生语句级原子性 | `ora_statement_level_rollback=off` | `ora_statement_level_rollback=off` | 默认行为接近 |
| 严格 Oracle 事务语义 | 默认 | 需显式 `on`，且不覆盖顶层 SQL | 需显式 `on`，覆盖顶层 SQL/PLiSQL | HGDB 严格模式范围更大 |

### 10.1 关键差异总结

1. **默认行为**
   - Oracle：原生语句级原子性；
   - Kingbase：默认 `off`，接近 PG；
   - HGDB：默认 `off`，接近 Kingbase/PG；开启 `on` 后覆盖范围更广。

2. **HGDB 相对 Kingbase 多出的能力**
   - Oracle 隐式事务模型（`ora_autocommit=off`）；
   - 顶层 SQL 语句级回滚（`on`）；
   - 未处理异常保留事务中之前 SQL（`on`）；
   - SQL / PLiSQL 跨层 savepoint；
   - 用户 savepoint 与内部帧隔离；
   - Extended Query / Portal 语句帧；
   - MULTI_QUERY 逐语句失败隔离。

3. **本期仍未实现、与 Oracle 有差异**
   - DDL / TRUNCATE 隐式提交；
   - `SET TRANSACTION READ ONLY` 事务级快照；
   - Oracle `SERIALIZABLE` 的 ORA-08177 语义；
   - `COMMIT COMMENT/WRITE/FORCE` 完整语义；
   - `FORALL SAVE EXCEPTIONS`；
   - 多语句 Simple Query 逐语句独立提交。
