# Oracle 事务兼容测试规范与行为基线（阶段 0）

- 日期：2026-11-12
- 对应设计：`doc/oracle_transaction/oracle_transaction_compat_plan.md` 第 17、18、19 节
- 阶段：阶段 0，测试规范与行为基线
- 状态：设计阶段，尚未开始编码

---

## 1. 目的

在编码前固定以下内容：

1. Oracle 事务兼容的目标行为；
2. 当前仓库的已知差异；
3. 可执行的测试用例和预期结果；
4. 测试类型、测试位置和运行方式；
5. 后续实现阶段的验收基线。

本规范不要求当前代码全部通过，而是作为后续实现的验收标准。

---

## 2. 适用范围

### 2.1 覆盖范围

- Oracle 兼容模式：`ivorysql.compatible_mode=oracle`
- 会话事务模型：
  - `ivorysql.ora_autocommit=on`
  - `ivorysql.ora_autocommit=off`
  - 显式 `BEGIN` / `START TRANSACTION`
- SQL 顶层事务：
  - 语句级回滚
  - `COMMIT` / `ROLLBACK`
  - `SAVEPOINT` / `ROLLBACK TO SAVEPOINT`
- PL/iSQL：
  - 异常处理
  - 语句级回滚
  - `COMMIT` / `ROLLBACK`
  - `SAVEPOINT` / `ROLLBACK TO`
  - `CALL` / `DO`
  - `FORALL`（普通模式）
- 自治事务：
  - `PRAGMA AUTONOMOUS_TRANSACTION`
  - 提交、回滚、异常、保存点隔离
- Extended Query Protocol：
  - Portal 生命周期
  - suspended Portal
  - Portal 交错限制
  - MULTI_QUERY 逐语句帧

### 2.2 明确不覆盖

以下行为本期不实现，测试只验证“差异已文档化”：

1. DDL 隐式提交；
2. `TRUNCATE` 隐式提交；
3. `FORALL SAVE EXCEPTIONS`；
4. Oracle `SET TRANSACTION READ ONLY` / `SERIALIZABLE` 特殊语义；
5. `COMMIT WRITE BATCH NOWAIT`、`COMMIT FORCE`、`ROLLBACK FORCE` 的完整语义；
6. 自治事务退出严格报 `ORA-06519`；
7. ORA 错误码全量 Oracle 化。

---

## 3. 测试类型与位置

| 测试类型 | 用途 | 建议位置 |
|---|---|---|
| SQL 回归测试 | 单会话 SQL / PL/iSQL 行为 | `src/oracle_test/regress/sql/ora_tx_*.sql` 与 `expected/` |
| 隔离测试 | 两会话可见性、锁、autocommit | `src/oracle_test/isolation/specs/ora_tx_*.spec` |
| TAP / libpq 测试 | Extended Query、Portal 生命周期、协议状态 | `src/oracle_test/modules/libpq_pipeline/` 或新增 TAP 测试 |
| 手工验证 | 性能、连接池、客户端行为 | 单独记录，不作为 CI 基线 |

### 3.1 运行方式

Oracle 模式回归：

```bash
make -C src/oracle_test/regress oracle-check-tests \
  TESTS='ora_tx_autocommit ora_tx_statement_rollback ora_tx_plisql_exception'
```

隔离测试：

```bash
make -C src/oracle_test/isolation oracle-check-tests \
  TESTS='ora_tx_autocommit_visibility'
```

TAP 测试按新增测试目录的 Makefile 运行。

---

## 4. 测试命名约定

- `T-AUTO-*`：会话事务模型与 autocommit
- `T-SQL-*`：SQL 顶层语句级回滚
- `T-SP-*`：SAVEPOINT / ROLLBACK TO
- `T-PL-*`：PL/iSQL 异常与事务控制
- `T-CALL-*`：CALL / DO
- `T-AX-*`：自治事务
- `T-EXT-*`：Extended Query / Portal
- `T-ERR-*`：错误恢复与状态
- `T-OOS-*`：本期不实现、只验证差异
- `T-PERF-*`：性能与子事务数量

---

## 5. 测试矩阵

> 说明：
> - 默认 `ora_statement_level_rollback=off`；
> - T-SQL-* / T-PL-* 中的严格语句级语义用例默认假设 `ora_statement_level_rollback=on`；
> - 默认 `off` 的行为由 T-OFF-* 覆盖。

### 5.0 默认关闭语句级回滚（off）

| ID | 场景 | 预期 |
|---|---|---|
| T-OFF-01 | 显式事务中失败 DML | 整个事务 aborted，后续语句不可执行 |
| T-OFF-02 | 显式事务中 `SELECT 1/0` | 整个事务 aborted |
| T-OFF-03 | PL/iSQL 异常块（无开关） | 整块回滚，只保留 handler 中 DML |
| T-OFF-04 | 未处理 `RAISE` | 事务级回滚 |
| T-OFF-05 | 未处理 SQL 错误 | 事务级回滚 |
| T-OFF-06 | 用户 savepoint | savepoint 本身可用；异常仍按块级/事务级回滚 |
| T-OFF-07 | `ora_autocommit=on` + 失败顶层命令 | 失败命令不提交，事务行为按 off 模式 |

### 5.1 会话事务模型

| ID | 场景 | 预期 |
|---|---|---|
| T-AUTO-01 | `ora_autocommit=on`，单条 `INSERT` | 命令结束提交，另一会话可见 |
| T-AUTO-02 | `ora_autocommit=off`，单条 `INSERT` | 事务保持打开，另一会话不可见，直到 `COMMIT` |
| T-AUTO-03 | `ora_autocommit=off`，`SELECT` 后 `INSERT` | 同一事务内执行，`COMMIT` 后可见 |
| T-AUTO-04 | `ora_autocommit=on` + 显式 `BEGIN` | 暂停自动提交，直到 `COMMIT` / `ROLLBACK` |
| T-AUTO-05 | 无事务时 `COMMIT` | 静默成功，不输出 WARNING |
| T-AUTO-06 | 无事务时 `ROLLBACK` | 静默成功，不输出 WARNING |
| T-AUTO-07 | `off` 下 `SAVEPOINT` 无 `BEGIN` | 允许，事务保持打开 |
| T-AUTO-08 | `on` 下 `SAVEPOINT` 单语句 | 语句结束提交，savepoint 不跨语句 |
| T-AUTO-09 | `COMMIT AND CHAIN` | 等价普通 `COMMIT`，下一条语句隐式开启新事务 |
| T-AUTO-10 | `ROLLBACK AND CHAIN` | 等价普通 `ROLLBACK` |

### 5.2 SQL 顶层语句级回滚

| ID | 场景 | 预期 |
|---|---|---|
| T-SQL-01 | 显式事务：`INSERT 1`、失败、`INSERT 2`、`COMMIT` | 1、2 保留 |
| T-SQL-02 | `off`：同上 | 1、2 保留，事务可用 |
| T-SQL-03 | 显式事务：`INSERT 1`、`SELECT 1/0`、`INSERT 2`、`COMMIT` | 1、2 保留，SELECT 只影响自己 |
| T-SQL-04 | `on` 无 `BEGIN`：`SELECT 1/0` 后继续执行 | 会话继续可用 |
| T-SQL-05 | Simple Query 多语句：`INSERT 1`、失败、`INSERT 3` | 1 保留，失败语句回滚，3 不执行，事务可用 |
| T-SQL-06 | 多语句 simple Query 不要求逐语句独立提交 | 文档记录为差异，不作为失败 |
| T-SQL-07 | `SELECT FOR UPDATE` 失败 | 只回滚当前语句，事务可用 |
| T-SQL-08 | 顶层普通 SQL 失败后下一条语句成功 | 下一条语句可执行，最终可 `COMMIT` |

### 5.3 SAVEPOINT / ROLLBACK TO

| ID | 场景 | 预期 |
|---|---|---|
| T-SP-01 | `off` 下无 `BEGIN`：`SAVEPOINT s1`、DML、`ROLLBACK TO s1` | 回滚到 s1，事务继续 |
| T-SP-02 | `ROLLBACK TO` 后目标 savepoint 保留 | 可再次 `ROLLBACK TO` 同一 savepoint |
| T-SP-03 | 目标之后的 savepoint 失效 | `ROLLBACK TO` 后续 savepoint 报错 |
| T-SP-04 | 同名 savepoint 重复创建 | 最近创建的优先 |
| T-SP-05 | 用户 savepoint 与内部语句帧交错 | 用户 savepoint 不被内部帧误 release |
| T-SP-06 | SQL `SAVEPOINT`，PL/iSQL 内 `ROLLBACK TO` | 跨层回滚成功 |
| T-SP-07 | PL/iSQL 内 `SAVEPOINT`，过程返回后 SQL `ROLLBACK TO` | 跨层回滚成功 |
| T-SP-08 | `ROLLBACK TO` 时有打开游标 | 游标状态和资源清理可预期 |
| T-SP-09 | 自治事务内 savepoint | 只能回滚自治事务作用域 |

### 5.4 PL/iSQL 异常与事务控制

| ID | 场景 | 预期 |
|---|---|---|
| T-PL-01 | 异常块：`INSERT 1`、失败、`INSERT 2`；handler `INSERT 3`、`COMMIT` | 1、3 保留 |
| T-PL-02 | 匿名块内未处理 SQL 失败（开关 on），块内之前 `INSERT 1` 成功 | 整个块回滚（1 回滚），事务可用 |
| T-PL-03 | 匿名块内纯 `RAISE`（开关 on），块内之前 `INSERT 1` 成功 | 整个块回滚（1 回滚），事务可用 |
| T-PL-03b | 事务中先 `INSERT 1`，再执行失败匿名块（块内 `INSERT 2` 后报错），再 `INSERT 3`、`COMMIT` | 1、3 保留，2 回滚 |
| T-PL-04 | 嵌套异常块 | 1、2、3 保留 |
| T-PL-05 | handler 自身抛异常，外层 handler 捕获 | 内层 handler 之前成功的语句保留 |
| T-PL-06 | 异常块内 `COMMIT` | 提交成功，后续语句进入新事务 |
| T-PL-07 | 异常块内 `ROLLBACK` | 回滚成功，后续语句进入新事务 |
| T-PL-08 | 函数内 `COMMIT` | 报错，事务状态符合预期 |
| T-PL-09 | 触发器内 `COMMIT` | 报错 |
| T-PL-10 | 过程内 `COMMIT`，调用者在显式事务中 | 提交成功 |
| T-PL-11 | 调用者 handler 捕获，嵌套过程未处理异常 | 只回滚失败的内层 SQL 语句；callee 之前成功 DML 保留；handler 执行 |
| T-PL-11b | 无 handler，异常传播出顶层块 / `CALL` | 回滚整个顶层语句；事务中之前成功的 SQL 保留 |
| T-PL-12 | 普通 `FORALL` 中途失败 | 整个 `FORALL` 回滚 |
| T-PL-13 | `FORALL SAVE EXCEPTIONS` | 本期不实现，验证文档化差异 |

### 5.5 CALL / DO

| ID | 场景 | 预期 |
|---|---|---|
| T-CALL-01 | 显式事务中 `CALL procedure`，过程内 `COMMIT` | 提交成功 |
| T-CALL-02 | 嵌套 `CALL procedure`，内层 `COMMIT` | 提交成功 |
| T-CALL-03 | `CALL` 函数，函数内 `COMMIT` | 报错 |
| T-CALL-04 | 顶层 `DO` 匿名块内 `COMMIT` | 提交成功 |
| T-CALL-05 | `DO` 内调用函数，函数尝试 `COMMIT` | 报错 |
| T-CALL-06 | package SECURITY DEFINER / AUTHID 场景 | 按现有规则判定 |

### 5.6 自治事务

| ID | 场景 | 预期 |
|---|---|---|
| T-AX-01 | 自治事务提交 | 独立于主事务提交 |
| T-AX-02 | 自治事务回滚 | 不影响主事务 |
| T-AX-03 | 自治事务未显式提交退出 | 保持当前宽松策略，自动提交 |
| T-AX-04 | 嵌套自治事务 | 按嵌套层级提交/回滚 |
| T-AX-05 | 主事务 savepoint 与自治事务 | 互相隔离 |
| T-AX-06 | 自治事务内 `EXCEPTION` + `COMMIT` | 提交成功 |
| T-AX-07 | 审计日志包场景 | 不再出现 `cannot commit while a subtransaction is active` |

### 5.7 Extended Query / Portal

| ID | 场景 | 预期 |
|---|---|---|
| T-EXT-01 | `Execute(max_rows=10)` 分批读取只读 SELECT | 正常返回 |
| T-EXT-02 | suspended 只读 SELECT 中途错误 | 事务可用 |
| T-EXT-03 | DML RETURNING suspended 后错误 | 整个 DML 语句回滚 |
| T-EXT-04 | 两个 PortalFrame Portal 交错 | 明确报错 |
| T-EXT-05 | 只读短帧 Portal 与一个 PortalFrame Portal 交错 | 允许 |
| T-EXT-06 | PortalDrop 时仍有语句帧 | 正确清理 |
| T-EXT-07 | suspended Portal 期间 `COMMIT` / `ROLLBACK` | 正确清理并结束事务 |
| T-EXT-08 | suspended Portal 期间取消请求 | 正确清理 |
| T-EXT-09 | `PORTAL_MULTI_QUERY` 中间语句失败 | 之前成功语句保留，后续不执行 |
| T-EXT-10 | named prepared statement 重复 Execute | 帧状态正确 |

### 5.8 错误恢复与状态

| ID | 场景 | 预期 |
|---|---|---|
| T-ERR-01 | 嵌套 handler 中 generation 归属 | 内层恢复不影响外层 |
| T-ERR-02 | handler 内再次失败，外层 handler 捕获 | 外层 scope 消费新错误 |
| T-ERR-03 | 局部恢复后 `PostgresMain` 跳过 `AbortCurrentTransaction` | 事务保持可用 |
| T-ERR-04 | 错误状态不跨请求保留 | 下一条命令状态干净 |
| T-ERR-05 | 失败语句后继续执行并 `COMMIT` | 成功 |
| T-ERR-06 | `ReadyForQuery` 状态 | `I` / `T` / `E` 符合预期 |

### 5.9 本期不实现（差异验证）

| ID | 场景 | 预期 |
|---|---|---|
| T-OOS-01 | `CREATE TABLE` 后 `ROLLBACK` | 保持 PG 事务 DDL 行为，文档记录差异 |
| T-OOS-02 | `TRUNCATE` 后 `ROLLBACK` | 保持 PG 行为，文档记录差异 |
| T-OOS-03 | `SET TRANSACTION READ ONLY` | 保持 PG 行为，文档记录差异 |
| T-OOS-04 | `SET TRANSACTION ISOLATION LEVEL SERIALIZABLE` | 保持 PG SSI 行为 |
| T-OOS-05 | `FORALL SAVE EXCEPTIONS` | 报不支持或按当前行为，文档记录 |

### 5.10 性能基线

| ID | 场景 | 指标 |
|---|---|---|
| T-PERF-01 | PL/iSQL 循环 10k 次单行 DML | 子事务数量与耗时 |
| T-PERF-02 | `FORALL` 10k 行 | 确认不按迭代建帧 |
| T-PERF-03 | 大量 SAVEPOINT / ROLLBACK TO | 子事务数量与内存 |
| T-PERF-04 | 长事务中大量语句帧 | 帧节点内存可回收 |

---

## 6. 关键用例详细定义

### 6.1 T-SQL-01：显式事务语句级回滚

```sql
CREATE TABLE tx_test(id int);

BEGIN;
INSERT INTO tx_test VALUES (1);
INSERT INTO tx_test VALUES ('bad');
INSERT INTO tx_test VALUES (2);
COMMIT;

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
1
2
```

### 6.2 T-SQL-03：只读 SELECT 错误不影响事务

```sql
BEGIN;
INSERT INTO tx_test VALUES (1);
SELECT 1 / 0;
INSERT INTO tx_test VALUES (2);
COMMIT;

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
1
2
```

### 6.3 T-PL-03：未处理纯 PL/iSQL 异常

```sql
BEGIN
  INSERT INTO tx_test VALUES (1);
  RAISE_APPLICATION_ERROR(-20001, 'test error');
END;
/

SELECT * FROM tx_test ORDER BY 1;
```

预期：

- 客户端收到错误；
- 匿名块整体回滚，`1` 不保留；
- 事务可用；
- 可以继续执行新的 SQL 并 `COMMIT`。

补充用例 T-PL-03b：

```sql
INSERT INTO tx_test VALUES (1);

BEGIN
  INSERT INTO tx_test VALUES (2);
  RAISE_APPLICATION_ERROR(-20001, 'test error');
END;
/

INSERT INTO tx_test VALUES (3);
COMMIT;

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
1
3
```

说明：

- 匿名块是顶层 SQL 语句，未处理异常回滚整个块；
- 事务中之前成功的 SQL 语句保留。

### 6.4 T-PL-04：嵌套异常块

```sql
BEGIN
  INSERT INTO tx_test VALUES (1);

  BEGIN
    INSERT INTO tx_test VALUES ('bad');
  EXCEPTION
    WHEN OTHERS THEN
      INSERT INTO tx_test VALUES (2);
  END;

  INSERT INTO tx_test VALUES (3);
END;
/

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
1
2
3
```

### 6.5 T-PL-05：handler 自身抛异常

```sql
BEGIN
  BEGIN
    INSERT INTO tx_test VALUES ('bad');
  EXCEPTION
    WHEN OTHERS THEN
      INSERT INTO tx_test VALUES (2);
      RAISE_APPLICATION_ERROR(-20002, 'handler error');
  END;
EXCEPTION
  WHEN OTHERS THEN
    INSERT INTO tx_test VALUES (3);
END;
/

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
2
3
```

### 6.6 T-PL-06：异常块内 COMMIT

```sql
BEGIN
  INSERT INTO tx_test VALUES (1);

  BEGIN
    INSERT INTO tx_test VALUES ('bad');
  EXCEPTION
    WHEN OTHERS THEN
      COMMIT;
  END;

  INSERT INTO tx_test VALUES (2);
END;
/
```

预期：

- `1` 已提交；
- `2` 在新事务中；
- 后续 `COMMIT` 后 `1`、`2` 都可见；
- 若 `ora_autocommit=on`，成功命令结束后 `2` 也可能被提交，需按模式分别记录基线。

### 6.7 T-SP-01：savepoint 与内部语句帧交错

```sql
BEGIN;
INSERT INTO tx_test VALUES (1);
SAVEPOINT s1;
INSERT INTO tx_test VALUES (2);
INSERT INTO tx_test VALUES ('bad');
ROLLBACK TO SAVEPOINT s1;
INSERT INTO tx_test VALUES (3);
COMMIT;

SELECT * FROM tx_test ORDER BY 1;
```

预期：

```text
1
3
```

### 6.8 T-EXT-04：两个 PortalFrame 交错

流程：

```text
Parse/Bind Portal A: INSERT ... RETURNING
Execute A(max_rows=1) -> suspended
Parse/Bind Portal B: INSERT ... RETURNING
Execute B -> 预期报错：cannot execute interleaved Oracle statement frames
```

预期：

- B 的执行被拒绝；
- A 的语句帧仍保持；
- 关闭 A 或完成 A 后状态恢复。

### 6.9 T-EXT-09：MULTI_QUERY 逐语句帧

```sql
INSERT INTO tx_test VALUES (1);
INSERT INTO tx_test VALUES ('bad');
INSERT INTO tx_test VALUES (3);
```

预期：

- 1 保留；
- bad 回滚；
- 3 不执行；
- 事务可用。

### 6.10 T-PL-11：嵌套存储过程与 handler

```sql
CREATE OR REPLACE PROCEDURE q_test AS
BEGIN
  INSERT INTO tx_test VALUES (3);
  INSERT INTO tx_test VALUES ('bad');
END;
/

CREATE OR REPLACE PROCEDURE p_test AS
BEGIN
  INSERT INTO tx_test VALUES (2);
  q_test;
END;
/

BEGIN
  INSERT INTO tx_test VALUES (1);
  p_test;
EXCEPTION
  WHEN OTHERS THEN
    INSERT INTO tx_test VALUES (4);
END;
/

SELECT * FROM tx_test ORDER BY 1;
```

Oracle 预期：

```text
1
2
3
4
```

说明：

- 异常被调用者 handler 捕获；
- 只回滚失败的内层 `INSERT 'bad'`；
- `p_test` 和 `q_test` 中之前成功的 DML 保留；
- 不是回滚整个 `p_test` / `q_test` 调用；
- 存储过程调用边界本身不是回滚单位。

如果去掉调用者的 `EXCEPTION`，让异常传播出整个匿名块，则整个匿名块作为一个顶层 SQL 语句回滚，事务中之前成功的 SQL 语句保留。

---

## 7. 基线采集流程

1. 使用当前代码在 Oracle 模式下运行测试；
2. 记录实际输出、事务状态和错误信息；
3. 标记：
   - 已符合目标；
   - 与目标不同但本期不实现；
   - 需要后续实现修复；
4. 将目标行为写入 expected 文件前，必须与方案文档第 17、18、19 节对照；
5. 对本期不实现的差异，使用注释或文档说明，不写成“通过”。

---

## 8. 验收标准

阶段 0 完成条件：

1. 本文档测试矩阵通过评审；
2. 每个测试 ID 有明确的测试位置或待新增位置；
3. 关键用例的期望结果明确；
4. 本期不实现的差异有文档记录；
5. 基线采集方式明确；
6. 后续实现阶段可以直接按测试 ID 验收；
7. `ora_statement_level_rollback` 默认 `off` 的行为由 T-OFF-* 覆盖。

---

## 9. 与设计文档的对应关系

| 测试组 | 设计文档章节 |
|---|---|
| T-AUTO | 3.0、17.8、18.2 |
| T-SQL | 17.4、18.2、19.3 |
| T-SP | 15.6、18.5、19.4 |
| T-PL | 17.4、17.5、18.7、18.8、19.5 |
| T-CALL | 17.7、18.10、19.6 |
| T-AX | 15.5、16.9、17.9 |
| T-EXT | 18.3、19.2、19.4 |
| T-ERR | 17.4、18.4、19.5 |
| T-OOS | 17.9、19.1 |
| T-PERF | 16.10、19.6 |
| T-OFF | REQ-TX-018 |