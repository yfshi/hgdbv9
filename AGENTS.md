# AGENTS.md

## 当前工作上下文

- 项目：PG14 二次开发版本，支持 PG / MySQL 兼容 / Oracle 兼容三种模式
- 当前任务：Oracle 事务兼容方案设计与讨论
- 当前阶段：方案讨论 / 架构设计，**尚未开始写代码**
- 当前状态：v2.4 设计决策完成（语句级回滚默认 off）；Oracle 与 KingbaseES 已部署并完成完整事务对比；等待编码指示
- 文档目录：`doc/oracle_transaction/`
- 需求文档：`doc/oracle_transaction/oracle_transaction_requirements.md`
- 概要设计：`doc/oracle_transaction/oracle_transaction_high_level_design.md`
- 详细设计：`doc/oracle_transaction/oracle_transaction_detailed_design.md`
- 测试规范：`doc/oracle_transaction/oracle_transaction_test_spec.md`
- 外部对比：`doc/oracle_transaction/oracle_vs_kingbase_transaction_compare.md`（含实测结果）
- 事务实测报告：`doc/oracle_transaction/oracle_vs_kingbase_transaction_test_results.md`
- 对比测试用例：`doc/oracle_transaction/test_cases/`
- 方案演进记录：`doc/oracle_transaction/oracle_transaction_compat_plan.md`
- 当前有效设计：方案文档第 21 节 v2.4 决策 + 第 20 节 v2.3 修正 + 第 17、18、19 节（第 15、16 节为初版记录）
- 最后更新：2026-11-13

## 已确认的关键决策

1. 适用范围仅针对 Oracle 兼容模式，不影响 PG / MySQL 模式。
2. 会话事务模型采用**方案 B：Oracle 隐式事务模型**，默认 `ivorysql.ora_autocommit=on`：
   - `on`（默认）：服务端模拟客户端 autocommit on，顶层命令结束提交；
   - `off`：严格 Oracle 隐式事务：
     - 第一条可执行 SQL 隐式开启事务；
     - 语句结束不自动提交；
     - `COMMIT` / `ROLLBACK` 后，下一条 SQL 隐式开启新事务；
     - `SAVEPOINT` / `ROLLBACK TO` 不要求 `BEGIN`；
     - PL/iSQL 内部 `COMMIT` 后，剩余语句属于新事务并保持打开。
3. 兼容目标为严格 Oracle 语义：
   - 未处理异常回滚失败的顶层 SQL 语句（匿名块或 `CALL`）；
   - 事务中之前成功的 SQL 语句保留；
   - 事务保持可用；
   - 异常被 handler 捕获时，只回滚失败的内层 SQL 语句，块内之前成功的语句保留。
4. 子事务仍然存在，但粒度改变：
   - 去掉“异常块级子事务”；
   - 引入“语句级内部子事务”；
   - 用户 `SAVEPOINT` 作为独立用户子事务管理。
5. 引入事务作用域帧栈，统一管理：
   - 普通事务语句帧；
   - 用户 savepoint 帧；
   - 自治事务 root；
   - 异常块逻辑帧（不是子事务）。
6. 自治事务保持独立事务边界，但内部复用同一套语句级帧模型。
7. 自治事务退出保持当前宽松策略：未显式提交时可自动提交，不强制 `ORA-06519`。
8. PL/iSQL 需要补齐：
   - `SAVEPOINT name;`
   - `ROLLBACK [WORK] TO [SAVEPOINT] name;`
   - Oracle 风格 `COMMIT` / `ROLLBACK`；
   - `SET TRANSACTION` 视范围决定。
9. 修复显式事务块中的 `CALL` / `DO` 被强制 atomic 的问题。
10. `ivorysql.ora_statement_level_rollback` 默认 `off`；默认不启用语句级回滚，需要严格 Oracle 语义时显式设为 `on`。
11. 本期不实现 Oracle undo/回滚段，不要求 LGWR 细节完全一致。

### 方案 B 已定子项

1. 新增 `ivorysql.ora_autocommit`：
   - 默认 `true`；
   - `true`：服务端模拟客户端 autocommit on，顶层命令结束提交；
   - `false`：严格 Oracle 隐式事务，语句结束不提交。
2. 所有可执行 SQL 都作为事务执行单元，包括 `SELECT`、DML、DDL、`SELECT FOR UPDATE`、`LOCK`、`SET TRANSACTION`、`SAVEPOINT`、`CALL` / `DO`；`off` 时开启并保持打开，`on` 时命令结束提交。
3. 多语句 simple Query：
   - 阶段 1：只回滚失败语句，中止当前 Query 消息后续语句，事务保持可用；
   - 阶段 2：实现逐语句继续执行。
4. `idle_in_transaction_session_timeout` 用户显式设置仍然生效，不静默忽略；默认值 0 时不启用。
5. `BEGIN` / `START TRANSACTION` 作为兼容扩展接受；`AND CHAIN` 等价为普通提交/回滚；无事务时 `COMMIT` / `ROLLBACK` 静默成功。

### 已接受的复核问题处理

1. 显式 `BEGIN` / `START TRANSACTION` 覆盖 `ora_autocommit=on`：
   - 进入显式事务块后暂停自动提交；
   - 直到 `COMMIT` / `ROLLBACK`；
   - 块内 `SAVEPOINT` / `ROLLBACK TO` 跨语句有效。
2. 多语句 simple Query（v2.1 放宽）：
   - 阶段 1 不实现“每条内部语句独立 autocommit”；
   - 一个 Query 消息按一个事务单元处理；
   - 失败的顶层语句只回滚自己，中止后续语句，之前成功的语句保留，事务保持可用；
   - 文档记录与 Oracle 客户端逐语句 autocommit 的差异。
3. `ora_autocommit=on` 下未处理异常：
   - 回滚失败的顶层 SQL 语句（匿名块 / `CALL`）；
   - 保留事务中之前成功的 SQL 语句；
   - 事务保持打开；
   - 失败命令不触发自动提交，下一次成功命令再触发。

### v2.0 修订要点（基于复核意见）

1. SQL 语句失败只能由对应语句执行边界负责回滚；不能全部推到 `PostgresMain`。
2. PL/iSQL 异常传播不等于事务回滚；纯 `RAISE` / 运行时异常不能撤销此前成功的 DML。
3. `ora_txframe` 只管理逻辑帧；物理子事务、ResourceOwner、锁、快照仍由 PG `xact.c` / `SPI` 负责。
4. 语句执行边界使用 `PG_TRY` / `PG_CATCH` 做局部回滚；`PostgresMain` 只处理未被局部恢复的错误和协议恢复。
5. 帧栈与 PG 子事务必须通过统一包装接口同步；xact 子事务回调只做安全网。
6. 不提供通用的 `SPI_commit_allow_subxact()`；改为 Oracle 事务控制器 `OraExecCommit` / `OraExecRollback`，并校验调用上下文。
7. `ora_autocommit=on` 定位为“模拟客户端 autocommit”，不是 Oracle 原生默认行为；显式 `BEGIN` 覆盖自动提交。
8. DDL 隐式提交、`FORALL SAVE EXCEPTIONS`、`SET TRANSACTION` 需要单独明确范围。
9. `idle_in_transaction_session_timeout` 用户显式设置仍然生效，不静默忽略。

### v2.0 修订后的实施顺序

1. 行为定义与测试规范
2. 顶层 SQL 语句级回滚
3. PL/iSQL 语句级回滚
4. PL/iSQL 异常传播（`RAISE` / 未处理异常）
5. 异常块与 `COMMIT` / `ROLLBACK`
6. `SAVEPOINT` 与事务帧
7. `CALL` / `DO` 与事务控制
8. 自治事务
9. Oracle 隐式事务与自动提交
10. DDL 与隔离级别
11. 性能与回归

### v2.1 修订要点

1. Extended Query 的语句帧绑定到 Portal 完整生命周期，不只绑定单次 `PortalRun()`。
2. `OraLocalErrorRecovered` 改为带 `error_generation` 的 `OraErrorState`，防止嵌套异常复用旧状态。
3. `OraExecCommit` / `OraExecRollback` 通过 SPI 内部受控接口完成资源处理，不直接从 `pl_exec.c` 调裸 xact API。
4. 新帧模型与旧 `SetAutoSavepoint()` 明确互斥：ORA 新模式禁用旧自动 savepoint。
5. 多语句 simple Query 阶段 1 接受“整个消息一个事务单元”，不实现逐语句独立提交。
6. 增加 PL/iSQL 语句帧分类表；`FORALL` 使用 `in_forall` 防止内部重复建帧。
7. `CALL` / `DO` atomic 判定按 procedure/function/trigger/package 矩阵处理，不只按 ORA 模式放开。
8. `ora_autocommit=off` 改称“IvorySQL Oracle 模式持续事务兼容策略”，不称严格 Oracle 原生行为。

### v2.1 实施顺序

1. 行为规范与测试基线
2. 错误状态 generation + 帧栈骨架
3. 顶层 SQL 语句帧 + Simple Query 多语句限制
4. Extended Query Portal 语句帧
5. PL/iSQL 叶子语句帧 + 异常传播
6. 事务控制器 + SPI 受控接口
7. `SAVEPOINT` / `ROLLBACK TO`
8. `CALL` / `DO` + atomic 矩阵
9. 自治事务
10. `ora_autocommit` / 隐式事务
11. 回归、游标、Portal、性能

### v2.2 修订要点（执行边界最后收紧）

1. Portal 语句帧分两类：
   - PortalFrame：DML RETURNING / `SELECT FOR UPDATE` / 含 DML 的 MULTI_QUERY，绑定 Portal 完整生命周期；
   - Per-Execute 短帧：只读 SELECT，每次 Execute 创建并释放。
2. 同一会话同时只允许一个活动 PortalFrame Portal；交错执行需明确拒绝，避免子事务嵌套顺序错误。
3. 只读 SQL 也必须具备语句级错误恢复：Simple Query 建帧，Extended Query 只读 Portal 建 Per-Execute 短帧。
4. `PORTAL_MULTI_QUERY` 必须逐内部语句建帧，不能只用一个 Portal 帧。
5. 错误 generation 改为 `OraErrorScope` 边界栈：每个错误传播边界拥有独立 generation，handler 只消费当前 scope。
6. v2.2 完成后冻结架构，进入阶段 0：测试规范与行为基线。

### v2.2 后的下一阶段

阶段 0：测试规范与行为基线。  
需求、概要设计、详细设计、测试规范均已形成：

- `doc/oracle_transaction/oracle_transaction_requirements.md`
- `doc/oracle_transaction/oracle_transaction_high_level_design.md`
- `doc/oracle_transaction/oracle_transaction_detailed_design.md`
- `doc/oracle_transaction/oracle_transaction_test_spec.md`

下一步：
1. 评审需求、概要设计、详细设计；
2. 评审测试矩阵和关键用例；
3. 确认每个测试 ID 的落地位置（regress / isolation / TAP）；
4. 采集当前行为基线；
5. 评审通过后再进入编码。

在用户明确指示前不开始编码。

### KingbaseES 实测结论（V9R1C10 Docker）

已部署 KingbaseES V9R1C10 Docker（`database_mode=oracle`，开发版 license），实测结果：

1. `ora_statement_level_rollback` 默认 `off`。
2. 该参数对顶层 SQL 显式事务无效；失败语句仍导致事务 aborted。
3. 对 PL/SQL 异常块有效：`on` 时只回滚失败语句，`off` 时回滚整块。
4. 未处理 `RAISE` / 未处理 SQL 错误：即使 `on`，整个匿名块仍回滚，之前 DML 不保留。
5. PL/SQL 异常块内 `COMMIT` 可正常工作。
6. PL/SQL 顶层 `SAVEPOINT` / `ROLLBACK TO` 可用。
7. SQL `SAVEPOINT` 无 `BEGIN` 直接报错。
8. DDL / `TRUNCATE` 未观察到隐式提交，均可被 `ROLLBACK` 回滚。
9. `SET TRANSACTION READ ONLY` 不是 Oracle 事务级快照。
10. 自治事务独立提交/回滚；自治事务未处理异常整体回滚。
11. 显式事务中 SQL savepoint + PL/SQL `ROLLBACK TO` 报 `invalid transaction termination`。
12. PL/SQL savepoint 在过程返回后不可见。

结论：KingbaseES 可作为 Oracle 兼容下限参考，但未完整实现：
- Oracle 的未处理异常语句级语义（失败顶层语句回滚，之前事务语句保留）；
- 顶层 SQL 语句级回滚；
- Oracle 隐式事务模型；
- DDL/TRUNCATE 隐式提交；
- SQL/PL/iSQL 跨层 savepoint。

详细结果见 `doc/oracle_transaction/oracle_vs_kingbase_transaction_compare.md` 第 9 节和 `doc/oracle_transaction/oracle_vs_kingbase_transaction_test_results.md`。

### v2.3 实测修正（Oracle / Kingbase 对比后）

实测 Oracle Free 23ai 后，修正此前对 Oracle 未处理异常的理解：

1. Oracle 的原子单位是顶层 SQL 语句：
   - 匿名块、`CALL` / `DO` 都是一个顶层 SQL 语句；
   - 未处理异常回滚整个失败的顶层语句；
   - 事务中之前成功的 SQL 语句保留；
   - 事务保持可用。
2. 异常被 `EXCEPTION` handler 捕获时：
   - 只回滚失败的内层 SQL 语句；
   - 块内之前成功的语句保留。
3. 对设计的修正：
   - 顶层语句帧必须覆盖整个顶层 SQL 语句，包括 `CALL` / `DO` / 匿名块；
   - 叶子语句帧只用于 handler 捕获场景；
   - 未处理异常不能只回滚叶子语句，必须回滚顶层语句帧。
4. KingbaseES 实测：
   - 未处理异常会让整个事务 aborted，之前事务语句丢失；
   - 顶层 SQL 失败即使 `ora_statement_level_rollback=on` 也不能保持事务可用；
   - DDL / TRUNCATE 不隐式提交；
   - `SET TRANSACTION READ ONLY` 不是事务级快照；
   - SQL SAVEPOINT 无 `BEGIN` 报错；
   - 跨 SQL/PL/iSQL savepoint 受限。
5. 非自治嵌套存储过程共享调用者事务；异常被 handler 捕获时只回滚失败的内层 SQL 语句，callee 之前成功 DML 保留；未处理到顶层时回滚整个顶层语句。`CALL` 本身不单独建语句帧，帧加在内部实际 SQL 语句上。
6. 详细实测见 `doc/oracle_transaction/oracle_vs_kingbase_transaction_compare.md` 第 9 节。

### v2.4 设计决策：默认关闭语句级回滚

1. `ivorysql.ora_statement_level_rollback` 默认 `off`。
2. `off` 时：
   - 顶层 SQL 失败仍导致事务 aborted；
   - PL/iSQL 异常块按块级回滚；
   - 未处理异常按事务级回滚；
   - 不启用新的语句级帧恢复路径。
3. `on` 时：
   - 启用 v2.3 定义的语句级帧模型；
   - 顶层 SQL 失败只回滚失败语句；
   - handler 捕获时只回滚失败内层语句；
   - 未处理异常回滚失败顶层语句。
4. `ora_autocommit` 默认 `on` 保持不变；它和语句级回滚开关相互独立。
5. 严格 Oracle 事务语义需要：
   - `ora_statement_level_rollback=on`；
   - 根据会话模型选择 `ora_autocommit=on/off`。

## 关键结论备忘

- PG 中唯一的部分回滚机制是子事务，因此不能“完全不用子事务”。
- 当前语句级 savepoint 不是“语句前创建、语句后立即释放”，而是“语句成功后创建、下一条成功后释放”，且按名字搜索 release，可能覆盖用户 savepoint。
- 当前异常块是整个块一个子事务，因此块内 `COMMIT` / `ROLLBACK` 会被 `IsSubTransaction()` 阻止。
- 未处理异常要只回滚失败的顶层语句并保持事务可用，必须改造顶层错误处理，不能直接 `AbortCurrentTransaction()`。
- `FORALL` 按 Oracle 语义应整体回滚，不能按每次迭代建帧。

## 当前仓库关键位置

- 兼容模式：`src/include/utils/ora_compatible.h:32`
- 语句级回滚 GUC：`src/backend/utils/misc/ivy_oracle_guc.c:1015`
- 自动 savepoint：`src/backend/access/transam/xact.c:6694`
- SQL 主循环设置 savepoint：`src/backend/tcop/postgres.c:5856`
- 顶层错误回滚：`src/backend/tcop/postgres.c:5113`
- PL/iSQL 异常块子事务：`src/pl/plisql/src/pl_exec.c:2394`
- PL/iSQL 语句 savepoint：`src/pl/plisql/src/pl_exec.c:2993`
- PL/iSQL COMMIT / ROLLBACK：`src/pl/plisql/src/pl_exec.c:6917`、`:6964`
- PL/iSQL 语法：`src/pl/plisql/src/pl_gram.y:4197`、`:4212`
- SPI 子事务限制：`src/backend/executor/spi.c:274`、`:370`
- CALL / DO atomic 判定：`src/backend/tcop/utility.c:661`
- 自治事务：`src/backend/access/transam/xact.c:7635`、`:7700`、`:7728`
- 自治事务 PL/iSQL 入口与退出：`src/pl/plisql/src/pl_handler.c:410`、`:524`

## 下次继续讨论的切入点

v2.0 修订已写入方案文档第 17 节。编码前仍需确认：

### v2.0 最终范围决策

1. DDL 隐式提交：本期不实现，文档记录与 Oracle 的差异。
2. `idle_in_transaction_session_timeout`：用户显式设置仍然生效，不静默忽略；默认值 0 时不启用。
3. `SET TRANSACTION READ ONLY` / `SERIALIZABLE`：保持 PG 行为，不映射 Oracle 特殊语义，文档说明差异。
4. `FORALL SAVE EXCEPTIONS`：本期不实现，预留 AST 和测试标志。

### 已确认的错误与帧栈实现方案

1. 错误来源使用内部 `OraErrorState` / `OraErrorOrigin`，不导出公共 ABI；通过 `OraErrorMarkStatement` / `OraErrorMarkRuntime` / `OraErrorMarkRecovered` 等内部函数管理。
2. 帧栈 API 使用 `ora_txframe_*` snake_case 命名；使用 `OraTxFrameRoot` 管理 `MemoryContext`、root、current、epoch 和自治事务 parent。
3. 顶层 `OraTxFrameContext` 挂在 `TopMemoryContext` 下；自治事务使用独立子上下文，退出时恢复外层 root。

### 已确认的编码前决策

1. 帧栈采用“独立全局对象 + 自治事务 root 保存/恢复”，不直接挂 `TransactionStateData`；实现放在 `ora_txframe.c`。
2. `ora_autocommit` 阶段 0–4 先保持 `false`，阶段 5 完成后再改为默认 `true`。
3. `ROLLBACK TO` 参考 `_SPI_commit` / `_SPI_rollback` 清理 pinned portal / portal snapshot，并专门测试游标、`sys_refcursor`、holdable cursor。
4. `ivorysql.ora_statement_level_rollback` 默认 `off`：默认保持现有块级/事务级回滚；设为 `on` 时才启用新语句帧模型和严格 Oracle 语句级语义。
5. `idle_in_transaction_session_timeout` 保持用户显式设置生效，不静默跳过。
6. 错误来源与本地恢复使用内部 `OraErrorState`，`PostgresMain` 通过 `locally_recovered` 决定是否跳过 `AbortCurrentTransaction()`。
7. 帧栈使用 `OraTxFrameRoot` + `OraTxFrameContext`，弹出帧时 `pfree`，事务结束/自治事务结束 reset 并 `epoch++`。

> 所有编码前设计决策已确认。当前阶段仍只做设计，不开始编码；需要用户明确指示后才进入实现。

## 协作约定

- 默认使用简体中文交流。
- 未经明确要求，不修改代码、不提交 Git。
- 修改代码前先阅读 `doc/oracle_transaction/oracle_transaction_compat_plan.md`。
- 涉及删除、覆盖、重置、数据库变更等不可逆操作时先确认。