# Oracle 事务兼容详细设计

- 文档版本：1.0
- 日期：2026-11-12
- 状态：设计基线
- 适用版本：PG14 二次开发版本
- 相关文档：
  - `doc/oracle_transaction/oracle_transaction_requirements.md`
  - `doc/oracle_transaction/oracle_transaction_high_level_design.md`
  - `doc/oracle_transaction/oracle_transaction_test_spec.md`
  - `doc/oracle_transaction/oracle_transaction_compat_plan.md`（设计演进记录）

---

## 1. 设计范围

本文档描述 Oracle 事务兼容的实现级设计，包括：

- 数据结构；
- 模块 API；
- 帧与 PG 子事务同步；
- 错误恢复算法；
- Portal 生命周期；
- Simple Query 多语句；
- PL/iSQL 执行路径；
- 事务控制器与 SPI；
- 语法和 AST；
- GUC；
- 文件级和函数级改动；
- 实施顺序与验收。

---

## 2. 数据结构

### 2.1 事务帧

```c
typedef enum OraTxFrameKind
{
    ORA_TXFRAME_TOP,            /* 顶层事务 root */
    ORA_TXFRAME_AUTON_ROOT,     /* 自治事务 root */
    ORA_TXFRAME_STATEMENT,      /* 内部语句帧 */
    ORA_TXFRAME_USER_SAVEPOINT, /* 用户 SAVEPOINT */
    ORA_TXFRAME_BLOCK           /* PL/iSQL 异常块逻辑帧 */
} OraTxFrameKind;

typedef struct OraTxFrame
{
    struct OraTxFrame *parent;
    OraTxFrameKind     kind;
    char              *name;          /* 用户 savepoint 名，内部帧可为 NULL */
    SubTransactionId   subxid;        /* 对应 PG 子事务 */
    bool               subxact_alive;
    bool               has_handler;   /* BLOCK 帧使用 */
    bool               in_handler;    /* BLOCK 帧使用 */
    uint64             epoch;
    void              *owner;         /* PLiSQL_execstate 或 NULL */
} OraTxFrame;
```

### 2.2 事务 root

```c
typedef struct OraTxFrameRoot
{
    MemoryContext          context;
    OraTxFrame            *root;
    OraTxFrame            *current;
    uint64                 epoch;
    struct OraTxFrameRoot *parent;    /* 自治事务使用 */
} OraTxFrameRoot;
```

### 2.3 错误来源与错误状态

```c
typedef enum OraErrorOrigin
{
    ORA_ERR_NONE = 0,
    ORA_ERR_SQL_STATEMENT,
    ORA_ERR_PLSQL_RUNTIME,
    ORA_ERR_TX_CONTROL,
    ORA_ERR_INTERNAL
} OraErrorOrigin;

typedef struct OraErrorScope
{
    struct OraErrorScope *parent;
    uint64                generation;
    OraErrorOrigin        origin;
    SubTransactionId      failed_subxid;
    uint64                failed_epoch;
    bool                  recovered;
} OraErrorScope;

typedef struct OraErrorState
{
    OraErrorScope *current;
    uint64         generation;
} OraErrorState;
```

### 2.4 会话状态

```c
typedef struct OraTxSessionState
{
    bool   autocommit;        /* ivorysql.ora_autocommit */
    bool   explicit_txn;      /* 显式 BEGIN 后为 true */
    bool   use_new_frames;    /* ORA 新帧模型 */
    uint64 epoch;
} OraTxSessionState;
```

### 2.5 Portal 扩展

```c
/* src/include/utils/portal.h */
struct OraTxFrame *ora_stmt_frame;   /* 仅 PortalFrame 使用 */
```

### 2.6 PL/iSQL AST

```c
typedef enum PLiSQL_txn_stmt_kind
{
    PLISQL_TXN_COMMIT,
    PLISQL_TXN_ROLLBACK,
    PLISQL_TXN_SAVEPOINT,
    PLISQL_TXN_ROLLBACK_TO,
    PLISQL_TXN_SET_TRANSACTION
} PLiSQL_txn_stmt_kind;

typedef struct PLiSQL_stmt_txn
{
    PLiSQL_stmt_type      cmd_type;   /* PLISQL_STMT_TXN */
    int                   lineno;
    unsigned int          stmtid;
    PLiSQL_txn_stmt_kind  kind;
    char                 *savepoint_name;
    bool                  chain;
    char                 *comment;
    int                   write_mode;
    bool                  nowait;
    char                 *force_gid;
    List                 *set_options;
} PLiSQL_stmt_txn;
```

---

## 3. 新增模块 `ora_txframe`

### 3.1 文件

- `src/include/access/ora_txframe.h`
- `src/backend/access/transam/ora_txframe.c`

### 3.2 API

```c
void ora_txframe_init(void);
void ora_txframe_reset_top(void);

void ora_txframe_begin_auton(void);
void ora_txframe_end_auton(bool isCommit);

void ora_txframe_begin_statement(void);
void ora_txframe_end_statement_success(void);
void ora_txframe_end_statement_error(void);

void ora_txframe_define_user_savepoint(const char *name);
void ora_txframe_rollback_to_user_savepoint(const char *name);

void ora_txframe_push_block(void *owner, bool has_handler);
void ora_txframe_pop_block(void);

OraTxFrame *ora_txframe_current(void);
OraTxFrame *ora_txframe_find_user_savepoint(const char *name);
bool ora_txframe_is_valid(const OraTxFrame *frame);
bool ora_txframe_has_active_handler(void);
uint64 ora_txframe_epoch(void);
```

### 3.3 错误 API

```c
void OraErrorStateReset(void);

void OraErrorScopePush(OraErrorOrigin origin);
void OraErrorScopePop(void);

void OraErrorMarkStatement(SubTransactionId subid, uint64 epoch);
void OraErrorMarkRuntime(void);
void OraErrorMarkRecovered(void);

bool OraErrorIsRecovered(void);
bool OraErrorConsumeRecovered(void);
OraErrorOrigin OraErrorGetOrigin(void);
uint64 OraErrorGetGeneration(void);
```

### 3.4 内存上下文

- `OraTxFrameRoot.context` 为独立 `MemoryContext`；
- 顶层 root 的父上下文为 `TopMemoryContext`；
- 自治事务 root 使用独立子上下文；
- 事务结束/自治事务结束时 `MemoryContextReset` 并 `epoch++`；
- 帧节点弹出时 `pfree`；
- 不把全局指针指向已销毁上下文。

---

## 4. 帧与 PG 子事务同步

### 4.1 物理权威

- `TransactionStateData` / `SubTransactionId` 是物理权威；
- `OraTxFrame` 只记录逻辑关系；
- 每次物理操作前后校验 `subxid`；
- epoch 不匹配的帧视为失效。

### 4.2 统一包装

```c
void ora_txframe_begin_statement(void)
{
    BeginInternalSubTransaction(NULL);
    push_statement_frame(GetCurrentSubTransactionId());
}

void ora_txframe_end_statement_success(void)
{
    verify_current_frame();
    ReleaseCurrentSubTransaction();
    pop_and_free_frame();
}

void ora_txframe_end_statement_error(void)
{
    verify_current_frame();
    RollbackAndReleaseCurrentSubTransaction();
    pop_and_free_frame();
}
```

### 4.3 状态转换表

| 操作 | 物理动作 | 逻辑动作 |
|---|---|---|
| 语句开始 | `BeginInternalSubTransaction` | push `STATEMENT` |
| 语句成功 | `ReleaseCurrentSubTransaction` | pop `STATEMENT` |
| 语句失败 | `RollbackAndReleaseCurrentSubTransaction` | pop `STATEMENT` |
| `SAVEPOINT` | `DefineSavepoint` + `CommitTransactionCommand` | push `USER_SAVEPOINT` |
| `ROLLBACK TO` | `RollbackToSavepoint` + `CommitTransactionCommand` | 删除目标之上的帧，保留并更新目标 |
| `COMMIT` | `EndTransactionBlock` + `CommitTransactionCommand` | 清空当前 root 下所有帧 |
| `ROLLBACK` | `UserAbortTransactionBlock` + `CommitTransactionCommand` | 清空当前 root 下所有帧 |
| 自治事务进入 | `BeginInternalAutonTransaction` | 保存外层 root，push `AUTON_ROOT` |
| 自治事务退出 | commit/abort | pop `AUTON_ROOT`，恢复外层 root |

### 4.4 xact 回调

- `OraTxFrameOnSubCommit(subid)` / `OraTxFrameOnSubAbort(subid)` 只做安全网；
- 不重复释放已由包装接口 pop 的帧；
- 发现残留帧时按 `subxid` 清理并修正 current；
- `ora_txframe_reset_top()` 只重置顶层 root；
- `ora_txframe_end_auton()` 只重置自治子树并恢复外层 root。

---

## 5. 错误恢复设计

### 5.1 顶层 SQL 语句边界

> 仅当 `ivorysql.ora_statement_level_rollback=on` 时启用本节的顶层语句帧和错误恢复路径。  
> 默认 `off` 时沿用现有事务/块级回滚行为。

位置：

- `exec_simple_query` 每条 parsetree；
- `exec_execute_message` 的 Portal 执行边界。

顶层语句帧覆盖整个顶层 SQL 语句，包括：

- 普通 DML / DDL / SELECT；
- `CALL` / `DO`；
- 匿名 PL/iSQL 块。

未处理异常导致整个顶层语句帧回滚；事务中之前成功的 SQL 语句保留。

伪代码：

```c
ora_error_scope_push(ORA_ERR_SQL_STATEMENT);
ora_txframe_begin_statement();

PG_TRY();
{
    PortalRun(...);

    ora_txframe_end_statement_success();
    ora_error_scope_pop();
}
PG_CATCH();
{
    ErrorData *edata = CopyErrorData();

    FlushErrorState();
    ora_txframe_end_statement_error();
    OraErrorMarkRecovered();

    ReThrowError(edata);
}
PG_END_TRY();
```

### 5.2 PL/iSQL 叶子语句边界

```c
if (plisql_stmt_needs_statement_frame(stmt))
{
    ora_error_scope_push(ORA_ERR_SQL_STATEMENT);
    ora_txframe_begin_statement();
}

PG_TRY();
{
    rc = exec_statement(...);

    if (frame_created)
    {
        ora_txframe_end_statement_success();
        ora_error_scope_pop();
    }
}
PG_CATCH();
{
    ErrorData *edata = CopyErrorData();

    FlushErrorState();

    if (frame_created)
    {
        /* 只回滚失败的内层语句，供 handler 匹配使用 */
        ora_txframe_end_statement_error();
    }

    /* 是否最终保持事务可用由顶层语句边界决定 */
    ReThrowError(edata);
}
PG_END_TRY();
```

### 5.3 异常块 handler 边界

`exec_stmt_block`：

- 不再 `BeginInternalSubTransaction`；
- push BLOCK 逻辑帧；
- `PG_CATCH` 中：
  - 接收叶子语句 rethrow；
  - 不重复回滚已回滚的语句帧；
  - 匹配 handler；
  - handler 执行期间 `in_handler = true`；
  - handler 成功后 pop BLOCK 帧并消费错误 scope；
  - handler 失败时新错误使用新 scope。

### 5.4 纯 PL/iSQL 异常

规则：

- 未处理 `RAISE` / 运行时异常向上传播到顶层语句边界；
- 顶层语句边界回滚整个顶层语句帧（匿名块 / `CALL` / `DO`）；
- 失败块内部的 DML 全部回滚；
- 事务中之前成功的 SQL 语句保留；
- 顶层边界标记 `locally_recovered`；
- `PostgresMain` 跳过 `AbortCurrentTransaction()`；
- 事务保持打开。

说明：

- 如果同一事务中先执行了 `INSERT 1`，再执行一个失败的匿名块（块内 `INSERT 2` 后报错），Oracle 的结果是保留 `1`、回滚 `2`；
- 不是“保留块内 `2` 之前的所有内层 DML”。

### 5.5 错误 scope 归属

- 每个错误传播边界 push 一个 `OraErrorScope`；
- generation 全局递增，但归属当前 scope；
- handler 成功处理后 pop 当前 scope；
- 外层 handler 状态保存在父 scope；
- `PostgresMain` 只消费顶层 scope；
- 错误状态不跨请求保留；
- Portal suspended 不保留错误状态。

---

## 6. Portal 生命周期设计

### 6.1 两类帧

| 类型 | 场景 | 生命周期 |
|---|---|---|
| PortalFrame | DML RETURNING、`SELECT FOR UPDATE`、含 DML 的 MULTI_QUERY | Portal 完整生命周期 |
| Per-Execute 帧 | 只读 SELECT | 每次 Execute |

### 6.2 Portal 字段

```c
struct OraTxFrame *ora_stmt_frame;
```

只用于 PortalFrame；Per-Execute 短帧由执行边界局部管理。

### 6.3 执行流程

```text
首次 Execute：
  if (needs_portal_frame(portal) && portal->ora_stmt_frame == NULL)
      ora_txframe_begin_statement();
      portal->ora_stmt_frame = current;

Execute 成功：
  if (portal 完成)
      ora_txframe_end_statement_success();
      portal->ora_stmt_frame = NULL;

Execute 失败：
  ora_txframe_end_statement_error();
  portal->ora_stmt_frame = NULL;
  MarkPortalFailed();

PortalDrop / 事务 abort：
  if (portal->ora_stmt_frame != NULL)
      回滚/释放并清空；
```

### 6.4 交错限制

- 同时只允许一个活动 PortalFrame Portal；
- 两个 PortalFrame Portal 交错执行时报错：
  `cannot execute interleaved Oracle statement frames`；
- 只读 Per-Execute 短帧允许嵌套在一个 PortalFrame 下；
- 限制写入用户文档和测试基线。

### 6.5 MULTI_QUERY

- `PORTAL_MULTI_QUERY` 不建一个覆盖整个 Portal 的帧；
- `PortalRunMulti` 内部逐 planned statement 建帧；
- 成功释放，失败回滚；
- 失败后停止后续语句。

---

## 7. Simple Query 多语句

### 7.1 行为

- 一个 Query 消息仍然是一个事务单元；
- 不实现逐语句独立 autocommit；
- 每个 parsetree 建独立语句帧；
- 失败语句回滚自己；
- 后续语句停止；
- 之前成功语句保留；
- 事务保持可用。

### 7.2 实现

`exec_simple_query`：

- 在 ORA 模式下禁用 per-statement commit；
- 每个 parsetree 调用语句边界包装；
- 失败后中止当前消息；
- 由顶层恢复逻辑保持事务可用。

---

## 8. PL/iSQL 执行路径

### 8.1 语句帧分类

> 仅当 `ivorysql.ora_statement_level_rollback=on` 时启用下列语句帧。  
> 默认 `off` 时不创建这些内部语句帧，异常按块级/事务级回滚。

| 语句 | 建帧 |
|---|---|
| `EXECSQL` | 是 |
| `SELECT INTO` | 是 |
| `DYNEXECUTE` | 是 |
| `CALL` | 否（内部实际 SQL 语句建帧） |
| `OPEN cursor` | 暂不建帧 |
| `FETCH` | 暂不建帧 |
| `CLOSE cursor` | 否 |
| `FORALL` | 整体一个 |
| `RAISE` | 否 |
| 变量赋值 | 否 |
| `IF` / `LOOP` | 否 |

### 8.2 `exec_stmt_block`

- 去掉块级 `BeginInternalSubTransaction`；
- push `BLOCK` 逻辑帧；
- 顶层匿名块由 `postgres.c` 的顶层语句帧覆盖；
- handler 只做匹配和执行；
- handler 内语句仍然走叶子边界；
- 未处理异常由顶层语句边界回滚整个顶层帧。

### 8.3 `exec_stmts_ext`

- 只在存在活动 `EXCEPTION` handler 时，为需要原子性的内层语句创建语句帧；
- `CALL` / 嵌套块本身不建帧；其内部实际 SQL 语句建帧；
- 需要则 `ora_txframe_begin_statement`；
- 成功 `ora_txframe_end_statement_success`；
- 失败由 catch 回滚该内层语句，然后交给 handler 或顶层边界。

### 8.4 `FORALL`

- `estate->in_forall = true`；
- 整个 FORALL 一个语句帧；
- 内部 DML 不再建帧；
- 失败回滚整个 FORALL 帧。

### 8.5 `CALL` / `DO`

- 顶层 `CALL procedure`：由顶层语句帧覆盖；允许事务控制；
- 嵌套 `CALL procedure`：本身不单独建帧；内部实际 SQL 语句建帧；允许事务控制；
- `DO` 匿名块：由顶层语句帧覆盖；允许事务控制；
- 函数、触发器：禁止；
- package / SECURITY DEFINER 沿用现有判定；
- 不因 ORA 模式无条件放开。

---

## 9. 事务控制器与 SPI

### 9.1 控制器 API

```c
int OraExecCommit(PLiSQL_execstate *estate, bool chain);
int OraExecRollback(PLiSQL_execstate *estate, bool chain);
int OraExecSavepoint(PLiSQL_execstate *estate, const char *name);
int OraExecRollbackTo(PLiSQL_execstate *estate, const char *name);
```

### 9.2 校验矩阵

| 条件 | 要求 |
|---|---|
| ORA 模式 | 必须 |
| PL/iSQL 非 atomic 上下文 | 必须 |
| 函数 / 触发器 | 禁止 |
| 活动 STATEMENT 帧 | 不允许 |
| 自治事务 | 只能操作当前 root |
| SPI 上下文 | 必须通过受控接口 |

### 9.3 SPI 内部接口

```c
int SPI_ora_commit(bool chain);
int SPI_ora_rollback(bool chain);
int SPI_ora_savepoint(const char *name);
int SPI_ora_rollback_to(const char *name);
```

要求：

- 复用 `_SPI_commit` / `_SPI_rollback` 的 Portal、Snapshot、ResourceOwner 处理；
- 保护当前 SPI 栈顶 `internal_xact`；
- 提交/回滚后 `StartTransactionCommand`；
- 重置/重建 PL/iSQL eval context；
- 不提供通用 `SPI_commit_allow_subxact()`。

---

## 10. 语法与 AST

### 10.1 新增关键字

- `savepoint`
- `work`
- `comment`
- `write`
- `wait`
- `nowait`
- `batch`
- `force`
- `set`
- `transaction`

建议按 unreserved 增加。

### 10.2 PL/iSQL 语法

```yacc
stmt_savepoint:
    opt_stmt_label K_SAVEPOINT ColId ';'

stmt_rollback:
    opt_stmt_label K_ROLLBACK opt_transaction_chain ';'
    | opt_stmt_label K_ROLLBACK opt_transaction K_TO opt_savepoint ColId ';'

stmt_commit:
    opt_stmt_label K_COMMIT opt_commit_options ';'
```

### 10.3 AST 处理

需要修改：

- `pl_gram.y`
- `plisql.h`
- `pl_exec.c`
- `pl_funcs.c`
- `pl_unreserved_kwlist.h`

`pl_funcs.c` 需要覆盖：

- `plisql_stmt_type_name`
- `free_stmt`
- `dump_stmt`

---

## 11. GUC

### 11.1 `ivorysql.ora_autocommit`

```c
bool ora_autocommit = true;
```

注册：

```c
{"ivorysql.ora_autocommit", PGC_USERSET, COMPAT_ORACLE_OPTIONS, ...}
```

要求：

- 默认 `true`；
- 仅 ORA 模式生效；
- 中途修改策略在实现阶段确认；
- 用户文档明确说明不是完整 JDBC autocommit。

### 11.2 `ivorysql.ora_statement_level_rollback`

```c
bool enable_ora_statement_level_rollback = false;  /* 默认 off */
```

注册：

```c
{"ivorysql.ora_statement_level_rollback", PGC_USERSET, COMPAT_ORACLE_OPTIONS, ...}
```

要求：

- 默认 `false`；
- `off`：
  - 不启用新的语句级帧模型；
  - 顶层 SQL 失败仍导致事务 aborted；
  - PL/iSQL 异常块按块级回滚；
  - 未处理异常按事务级回滚；
  - 旧 `SetAutoSavepoint` 机制也禁用。
- `on`：
  - 启用 v2.3 定义的顶层语句帧、叶子语句帧；
  - 启用严格 Oracle 语句级回滚语义；
  - 旧 `SetAutoSavepoint` 机制禁用。
- 仅 ORA 模式生效；
- 与 `ora_autocommit` 相互独立。

### 11.3 会话状态

```c
typedef struct OraTxSessionState
{
    bool   autocommit;
    bool   explicit_txn;
    bool   use_statement_frames;  /* = enable_ora_statement_level_rollback */
    uint64 epoch;
} OraTxSessionState;
```

---

## 12. 新旧 savepoint 互斥

| 模式 | 旧自动 savepoint | 新事务帧 |
|---|---|---|
| PG | 保持 | 不启用 |
| MySQL | 保持 | 不启用 |
| ORA + `ora_statement_level_rollback=off`（默认） | 禁用 | 不启用 |
| ORA + `ora_statement_level_rollback=on` | 禁用 | 启用 |

实现：

- `SetAutoSavepoint()` / `RollbackToAutoSavepoint()` 在 ORA 模式下一律禁用（no-op 或 assert）；
- `postgres.c` 主循环和错误路径检查 `ora_statement_level_rollback`；
- `pl_exec.c` 旧 `set_savepoint` / `has_savepoint` 逻辑禁用；
- `ora_statement_level_rollback` 是新帧模型和旧自动 savepoint 的统一开关。

---

## 13. 文件级改动

| 文件 | 改动 |
|---|---|
| `src/include/access/ora_txframe.h` | 新增 |
| `src/backend/access/transam/ora_txframe.c` | 新增 |
| `src/include/access/xact.h` | 事务回调、epoch 相关声明 |
| `src/backend/access/transam/xact.c` | 事务/子事务/自治事务挂接 |
| `src/include/utils/portal.h` | `ora_stmt_frame` 字段 |
| `src/backend/utils/mmgr/portalmem.c` | PortalDrop / abort 清理 |
| `src/backend/tcop/postgres.c` | 顶层边界、错误恢复、autocommit |
| `src/backend/tcop/pquery.c` | PortalRunMulti 逐语句帧 |
| `src/backend/executor/spi.c` | `SPI_ora_*` 内部接口 |
| `src/include/executor/spi.h` | 内部接口声明 |
| `src/backend/tcop/utility.c` | CALL / DO atomic 判定 |
| `src/backend/utils/misc/ivy_oracle_guc.c` | `ora_autocommit` |
| `src/pl/plisql/src/pl_exec.c` | 语句帧、异常、事务控制 |
| `src/pl/plisql/src/pl_gram.y` | 事务语法 |
| `src/pl/plisql/src/plisql.h` | AST |
| `src/pl/plisql/src/pl_funcs.c` | free / dump / name |
| `src/pl/plisql/src/pl_handler.c` | 自治事务与帧 root |
| `src/oracle_test/regress/sql/ora_tx_*.sql` | 新增测试 |
| `src/oracle_test/isolation/specs/ora_tx_*.spec` | 新增测试 |
| `src/oracle_test/modules/libpq_pipeline` 或新增 TAP | Extended Query 测试 |

---

## 14. 函数级改动

| 函数 | 改动 |
|---|---|
| `exec_simple_query` | 顶层语句帧、错误恢复、多语句限制 |
| `exec_execute_message` | Portal 边界、Per-Execute 帧 |
| `PortalRunMulti` | MULTI_QUERY 逐语句帧 |
| `PostgresMain` | 局部恢复消费、跳过 Abort |
| `StartTransactionCommand` | ORA 会话状态 |
| `CommitTransaction` / `AbortTransaction` | 帧 reset / epoch |
| `CommitSubTransaction` / `AbortSubTransaction` | 帧安全网 |
| `BeginInternalAutonTransaction` | 自治 root |
| `ReleaseCurrentAutonTransaction` | 自治 root 清理 |
| `exec_stmt_block` | 去掉块级子事务，逻辑帧 |
| `exec_stmts_ext` | 叶子语句帧 |
| `exec_stmt_commit` / `exec_stmt_rollback` | 事务控制器 |
| `exec_stmt_forall` | in_forall 防重复建帧 |
| `standard_ProcessUtility` | CALL / DO atomic |
| `_SPI_commit` / `_SPI_rollback` | 复用资源处理，内部 ORA 接口 |
| `SetAutoSavepoint` / `RollbackToAutoSavepoint` | 新模式互斥 |
| `plisql_call_handler` / `plisql_inline_handler` | 自治事务与帧 root |
| `free_stmt` / `dump_stmt` | 新 AST 节点支持 |

---

## 15. 实施顺序与验收

| 阶段 | 内容 | 验收 |
|---|---|---|
| 0 | 测试规范与行为基线 | 测试矩阵评审通过 |
| 1 | `ora_txframe` 骨架 + 错误 scope | 编译通过，现有测试不回归 |
| 2 | 顶层 SQL 语句帧 + Simple Query 限制 | T-SQL-* 通过 |
| 3 | Extended Query Portal 帧 | T-EXT-* 通过 |
| 4 | PL/iSQL 叶子帧 + 异常传播 | T-PL-* 通过 |
| 5 | 事务控制器 + SPI 接口 | T-PL-06/07、T-CALL-* 通过 |
| 6 | SAVEPOINT / ROLLBACK TO | T-SP-* 通过 |
| 7 | CALL / DO atomic 矩阵 | T-CALL-* 通过 |
| 8 | 自治事务 | T-AX-* 通过 |
| 9 | `ora_autocommit` / 隐式事务 | T-AUTO-* 通过 |
| 10 | 回归、游标、Portal、性能 | T-ERR-*、T-PERF-* 通过 |

---

## 16. 编码检查清单

- [ ] 不影响 PG / MySQL 模式
- [ ] ORA 新模式禁用旧自动 savepoint
- [ ] 所有物理子事务操作通过统一包装
- [ ] 帧 subxid / epoch 校验
- [ ] 错误 scope 不跨请求
- [ ] 纯 RAISE 不回滚此前 DML
- [ ] 只读 SELECT 有语句级错误恢复
- [ ] MULTI_QUERY 逐语句帧
- [ ] PortalFrame 交错限制
- [ ] 函数 / 触发器禁止事务控制
- [ ] 自治事务 savepoint 隔离
- [ ] 帧节点内存可回收
- [ ] 测试基线覆盖 on/off
- [ ] 本期不做项有文档和测试记录

---

## 17. 文档关系

- 需求：`doc/oracle_transaction/oracle_transaction_requirements.md`
- 概要设计：`doc/oracle_transaction/oracle_transaction_high_level_design.md`
- 详细设计：本文档
- 测试规范：`doc/oracle_transaction/oracle_transaction_test_spec.md`
- 设计演进记录：`doc/oracle_transaction/oracle_transaction_compat_plan.md`