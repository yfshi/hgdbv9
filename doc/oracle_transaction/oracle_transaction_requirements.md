# Oracle 事务兼容需求文档

- 文档版本：1.0
- 日期：2026-11-12
- 状态：设计基线
- 适用版本：PG14 二次开发版本
- 相关文档：
  - `doc/oracle_transaction/oracle_transaction_high_level_design.md`
  - `doc/oracle_transaction/oracle_transaction_detailed_design.md`
  - `doc/oracle_transaction/oracle_transaction_test_spec.md`
  - `doc/oracle_transaction/oracle_transaction_compat_plan.md`（设计演进记录）

---

## 1. 背景

当前项目基于 PostgreSQL 14 二次开发，支持三种兼容模式：

- PG 模式
- MySQL 兼容模式
- Oracle 兼容模式

本项目仅针对 **Oracle 兼容模式** 的事务行为进行增强。

当前 Oracle 模式下的事务内核仍然沿用 PostgreSQL 语义，主要差异包括：

1. 显式事务中某条语句失败后，整个事务进入 aborted 状态；
2. PL/iSQL 异常块会回滚整个块的修改，而不是只回滚失败语句；
3. 未处理异常会回滚整个 PL/SQL 调用或整个事务；
4. 带 `EXCEPTION` 的 PL/iSQL 块内不能正常 `COMMIT` / `ROLLBACK`；
5. PL/iSQL 缺少静态 `SAVEPOINT` / `ROLLBACK TO SAVEPOINT` 语法；
6. 显式事务中的 `CALL` / `DO` 被强制 atomic；
7. 语句级自动 savepoint 可能覆盖用户 savepoint；
8. 自治事务内部仍受异常块子事务限制。

---

## 2. 目标

### 2.1 总体目标

在 Oracle 兼容模式下，使事务行为兼容到以下语义：

> 未处理异常回滚失败的顶层 SQL 语句（匿名块或过程调用）的整个 DML；事务中之前成功的 SQL 语句保留；事务保持可用。
> 异常被 `EXCEPTION` handler 捕获时，只回滚失败的内层 SQL 语句，块内之前成功的语句保留。

### 2.2 具体目标

> 下列第 1、2、3、6 项为 `ivorysql.ora_statement_level_rollback=on` 时的目标；默认 `off` 时保持块级/事务级回滚。

1. 显式事务中语句失败只回滚当前语句；
2. PL/iSQL 异常处理按语句级回滚，而不是块级回滚；
3. 未处理异常不导致整个事务被回滚；
4. PL/iSQL 支持 Oracle 风格事务控制语句；
5. SQL 与 PL/iSQL 共享事务作用域和用户 savepoint 命名空间；
6. 支持 Extended Query Protocol 下的语句级恢复；
7. 保持自治事务独立事务边界；
8. 不影响 PG 模式和 MySQL 模式。

---

## 3. 术语

| 术语 | 说明 |
|---|---|
| Oracle 模式 | `ivorysql.compatible_mode=oracle` |
| 显式事务 | 客户端显式 `BEGIN` / `START TRANSACTION`，或 `ora_autocommit=off` 下的持续事务 |
| 语句级回滚 | 失败语句只回滚自己，不影响之前成功的工作 |
| 事务帧 | Oracle 兼容层维护的逻辑执行帧 |
| PortalFrame | 绑定 Portal 完整生命周期的语句帧 |
| Per-Execute 帧 | 每次 Execute 创建并释放的短生命周期语句帧 |
| 用户 savepoint | 用户显式 `SAVEPOINT` 创建的保存点 |
| 内部语句帧 | 系统为语句级回滚创建的内部帧，对用户不可见 |
| 自治事务 | `PRAGMA AUTONOMOUS_TRANSACTION` 声明的独立事务 |
| Oracle 隐式事务 | `ora_autocommit=off` 下的持续事务兼容策略 |

---

## 4. 范围

### 4.1 本期范围

1. 会话事务模型：
   - `ivorysql.ora_autocommit=on`：模拟客户端 autocommit；
   - `ivorysql.ora_autocommit=off`：IvorySQL Oracle 模式持续事务兼容策略；
   - 显式 `BEGIN` 覆盖自动提交。
2. 顶层 SQL 语句级回滚。
3. PL/iSQL 语句级回滚与异常传播。
4. PL/iSQL `COMMIT` / `ROLLBACK` / `SAVEPOINT` / `ROLLBACK TO`。
5. SQL `SAVEPOINT` / `ROLLBACK TO SAVEPOINT`。
6. `CALL` / `DO` 的事务控制权限。
7. 自治事务内部复用语句级帧模型。
8. Extended Query Protocol 的 Portal 生命周期和语句级恢复。
9. Simple Query 多语句的失败隔离。
10. 新旧自动 savepoint 机制互斥。
11. 配置参数、错误恢复状态、测试基线。

### 4.2 本期不做

1. DDL 隐式提交；
2. `TRUNCATE` 隐式提交；
3. `FORALL SAVE EXCEPTIONS`；
4. Oracle `SET TRANSACTION READ ONLY` / `SERIALIZABLE` 特殊语义；
5. `COMMIT WRITE BATCH NOWAIT`、`COMMIT FORCE`、`ROLLBACK FORCE` 完整语义；
6. Oracle undo / 回滚段；
7. LGWR 细节完全一致；
8. 自治事务退出严格 `ORA-06519`；
9. ORA 错误码全量 Oracle 化；
10. 多语句 Simple Query 的逐语句独立 autocommit。

上述差异必须在用户文档和测试基线中明确记录。

---

## 5. 现状与差距

> Oracle / KingbaseES / HGDB（规划）三方能力对比表见 `doc/oracle_transaction/oracle_vs_kingbase_transaction_compare.md` 第 10 节。

| 场景 | 当前行为 | 目标行为 |
|---|---|---|
| 显式事务中语句失败 | 整个事务 aborted | 只回滚失败语句，事务可用 |
| PL/iSQL 异常块失败 | 回滚整个块 | 只回滚失败语句 |
| 未处理异常 | 回滚整个 PL/SQL 调用或整个事务 | 回滚失败的顶层 SQL 语句，保留事务中之前成功的 SQL 语句 |
| 异常块内 `COMMIT` | 被 `IsSubTransaction()` 阻止 | 允许 |
| 显式事务中 `CALL` / `DO` | 强制 atomic | 按过程/函数/触发器矩阵判定 |
| PL/iSQL `SAVEPOINT` | 不支持静态语法 | 支持 |
| 用户 savepoint | 可能被自动 savepoint 覆盖 | 与内部帧隔离 |
| 自治事务异常块内 `COMMIT` | 仍被块级子事务阻止 | 允许 |
| 多语句 Simple Query | PG 隐式事务块整体处理 | 一个事务单元，但逐语句帧失败隔离 |

---

## 6. 功能性需求

### REQ-TX-001 会话事务模型

- `ivorysql.ora_autocommit` 默认 `true`；
- `true`：顶层命令成功结束后提交；
- `false`：事务持续打开，直到 `COMMIT` / `ROLLBACK`；
- 显式 `BEGIN` / `START TRANSACTION` 覆盖自动提交；
- 无事务时 `COMMIT` / `ROLLBACK` 静默成功；
- 参数仅影响 Oracle 模式。

### REQ-TX-002 顶层 SQL 语句级回滚（`ora_statement_level_rollback=on`）

- 默认 `off`，不启用本需求；
- 显式事务或 `ora_autocommit=off` 下，普通 SQL 语句失败只回滚当前语句；
- 事务保持可用；
- 后续语句可继续执行；
- 包括只读 `SELECT` 和 `SELECT FOR UPDATE`。

### REQ-TX-003 PL/iSQL 语句级回滚（`ora_statement_level_rollback=on`）

- 默认 `off`，不启用本需求；
- 在异常被 `EXCEPTION` handler 捕获时，失败的内层 SQL 语句只回滚自己；
- 块内之前成功的语句保留；
- 控制结构本身不单独建帧；
- `FORALL` 整体一个帧。

### REQ-TX-004 未处理异常语义（`ora_statement_level_rollback=on`）

- 默认 `off`，不启用本需求；
- 未处理异常回滚失败的顶层 SQL 语句（匿名块、过程调用）的整个 DML；
- 事务中之前成功的 SQL 语句保留；
- 事务保持可用；
- 错误返回客户端。

### REQ-TX-005 异常块 handler

- handler 只捕获当前块体中的异常；
- handler 自身异常向外层传播；
- handler 中 SQL 语句失败只回滚该语句；
- 嵌套 handler 的错误状态互不污染。

### REQ-TX-006 PL/iSQL 事务控制

- 支持 `COMMIT` / `ROLLBACK`；
- 支持在带 `EXCEPTION` 的块内使用；
- 过程、匿名块允许；
- 函数、触发器禁止；
- 提交/回滚后进入新事务并继续执行。

### REQ-TX-007 SAVEPOINT / ROLLBACK TO

- SQL 和 PL/iSQL 都支持 `SAVEPOINT`；
- 支持 `ROLLBACK TO [SAVEPOINT] name`；
- 目标 savepoint 保留；
- 目标之后的 savepoint 失效；
- 无显式事务时按会话事务模型处理。

### REQ-TX-008 savepoint 作用域

- 用户 savepoint 是事务作用域；
- SQL 与 PL/iSQL 共享命名空间；
- 过程返回后用户 savepoint 仍可被外层回滚；
- 内部语句帧对用户不可见；
- 自治事务与主事务 savepoint 隔离。

### REQ-TX-009 CALL / DO 权限

- Oracle 模式下，过程调用和匿名块允许事务控制；
- 函数、触发器禁止；
- 嵌套 CALL 遵循 atomic context 传播；
- package / SECURITY DEFINER 沿用现有判定；
- 不因 ORA 模式无条件放开。

### REQ-TX-010 自治事务

- 自治事务保持独立事务边界；
- 内部复用语句级帧模型；
- 自治事务提交/回滚不影响主事务；
- savepoint 作用域隔离；
- 退出未显式提交时保持当前宽松自动提交策略。

### REQ-TX-011 Extended Query Portal

- 语句帧绑定 Portal 完整生命周期；
- 只读 SELECT 使用 Per-Execute 短帧；
- DML RETURNING / `SELECT FOR UPDATE` / 含 DML 的 MULTI_QUERY 使用 PortalFrame；
- 同一会话同时只允许一个活动 PortalFrame Portal；
- 交错执行需要 PortalFrame 的 Portal 时明确报错；
- PortalDrop / 事务结束 / 取消请求时正确清理。

### REQ-TX-012 MULTI_QUERY 逐语句帧

- `PORTAL_MULTI_QUERY` 不能只使用一个 Portal 级语句帧；
- 内部每个实际 SQL 执行单元建立独立语句帧；
- 失败语句回滚自己；
- 之前成功语句保留；
- 后续语句停止执行。

### REQ-TX-013 错误恢复状态

- 错误恢复状态必须带 generation / scope；
- 嵌套 handler 中错误状态互不影响；
- handler 成功处理后消费当前 scope；
- 错误状态不跨请求保留；
- `PostgresMain` 只消费顶层恢复结果。

### REQ-TX-014 新旧 savepoint 互斥

- ORA 新帧模型启用时，旧 `SetAutoSavepoint` / `RollbackToAutoSavepoint` 禁用；
- PG / MySQL 模式保持原行为；
- 旧 GUC 仅作为回退开关；
- 不允许两套机制同时负责同一语句。

### REQ-TX-015 配置参数

- 新增 `ivorysql.ora_autocommit`；
- 默认 `true`；
- `PGC_USERSET`；
- 仅 ORA 模式生效；
- 中途修改策略需明确。

### REQ-TX-016 测试基线

- 提供 Oracle / PG 行为差异测试矩阵；
- 提供关键 SQL 用例；
- 提供 Extended Query / Portal 测试；
- 提供本期不做项的差异验证；
- 提供性能基线。

### REQ-TX-017 性能

- 不允许出现不可接受的性能退化；
- 语句帧只在需要时创建；
- `FORALL` 不按迭代建帧；
- 长事务帧节点可回收；
- 提供性能基线测试。

### REQ-TX-018 语句级回滚默认关闭

- `ivorysql.ora_statement_level_rollback` 默认 `off`；
- `off` 时：
  - 顶层 SQL 失败仍导致事务 aborted；
  - PL/iSQL 异常块按块级回滚；
  - 未处理异常按事务级回滚；
  - 不启用新的语句级帧恢复路径。
- `on` 时：
  - 启用顶层 SQL 与 PL/iSQL 语句级回滚；
  - 启用 v2.3 定义的顶层语句帧和叶子语句帧；
  - 启用严格 Oracle 未处理异常语义。
- `ora_autocommit` 与该开关相互独立；
- 严格 Oracle 事务语义需要显式设置 `ora_statement_level_rollback=on`。

---

## 7. 非功能性需求

| 类别 | 要求 |
|---|---|
| 正确性 | 事务状态、子事务、ResourceOwner、Snapshot、Portal 状态一致 |
| 隔离性 | 不改变 PG / MySQL 模式行为 |
| 并发性 | 不引入新的死锁或锁顺序问题 |
| 内存 | 帧节点可回收，不随长事务无限增长 |
| 可观测性 | 关键错误和限制有明确错误信息 |
| 可维护性 | 帧栈与 PG 物理事务职责分离 |
| 兼容性 | 文档记录与 Oracle 的剩余差异 |
| 安全性 | 不绕过现有权限、审计、SECURITY DEFINER 规则 |

---

## 8. 约束与假设

1. PostgreSQL 中部分回滚只能依赖子事务；
2. 物理事务状态以 `xact.c` 为权威；
3. Oracle 模式与 PG / MySQL 模式共用内核，改动必须模式隔离；
4. 现有 `ivorysql.ora_statement_level_rollback` 过渡期保留；
5. 测试基线需要覆盖 `ora_autocommit=on/off` 两种模式。

---

## 9. 验收标准

| 需求 | 验收方式 |
|---|---|
| REQ-TX-001 | T-AUTO-* |
| REQ-TX-002 | T-SQL-* |
| REQ-TX-003 | T-PL-01/02/11/12 |
| REQ-TX-004 | T-PL-02/03 |
| REQ-TX-005 | T-PL-04/05 |
| REQ-TX-006 | T-PL-06/07/08/09/10 |
| REQ-TX-007 | T-SP-* |
| REQ-TX-008 | T-SP-05/06/07/09 |
| REQ-TX-009 | T-CALL-* |
| REQ-TX-010 | T-AX-* |
| REQ-TX-011 | T-EXT-* |
| REQ-TX-012 | T-EXT-09 |
| REQ-TX-013 | T-ERR-* |
| REQ-TX-014 | T-AUTO-08、T-SP-05 |
| REQ-TX-015 | T-AUTO-01/02/04 |
| REQ-TX-016 | 阶段 0 测试规范 |
| REQ-TX-017 | T-PERF-* |
| REQ-TX-018 | T-AUTO-*、T-SQL-*、T-PL-* 的 on/off 变体 |

---

## 10. 风险

1. 顶层错误恢复改造影响面大；
2. Portal 生命周期和物理子事务同步复杂；
3. SPI 事务控制容易与现有资源管理冲突；
4. 新旧 savepoint 机制并存期容易出现双重回滚；
5. 子事务开销影响批量 DML；
6. 现有 Oracle 模式测试中包含 PG 语义，需要拆分基线。

---

## 11. 需求与测试追踪

完整测试矩阵见 `doc/oracle_transaction/oracle_transaction_test_spec.md`。

| 测试组 | 对应需求 |
|---|---|
| T-AUTO | REQ-TX-001、REQ-TX-014、REQ-TX-015 |
| T-SQL | REQ-TX-002 |
| T-SP | REQ-TX-007、REQ-TX-008 |
| T-PL | REQ-TX-003、REQ-TX-004、REQ-TX-005、REQ-TX-006 |
| T-CALL | REQ-TX-009 |
| T-AX | REQ-TX-010 |
| T-EXT | REQ-TX-011、REQ-TX-012 |
| T-ERR | REQ-TX-013 |
| T-OOS | 4.2 本期不做 |
| T-PERF | REQ-TX-017 |

---

## 12. 文档关系

- 需求文档：本文档
- 概要设计：`doc/oracle_transaction/oracle_transaction_high_level_design.md`
- 详细设计：`doc/oracle_transaction/oracle_transaction_detailed_design.md`
- 测试规范：`doc/oracle_transaction/oracle_transaction_test_spec.md`
- 设计演进记录：`doc/oracle_transaction/oracle_transaction_compat_plan.md`