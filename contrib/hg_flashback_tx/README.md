# HG Flashback TX 插件

## 概述

HG Flashback TX 是 PostgreSQL 的一个扩展插件，提供了类似 Oracle Flashback 的事务回溯查询功能。该插件能够记录数据库中的所有数据变更操作（DML），并生成对应的撤销 SQL 语句，使用户能够方便地查询历史数据变更并进行回溯操作。

## 功能特性

### 1. 事务回溯查询
- 记录所有 INSERT、UPDATE、DELETE 操作
- 自动生成对应的撤销 SQL 语句（Undo SQL）
- 支持含ROWID的表
- 支持单列主键表
- 支持复合主键表
- 支持无主键表（使用完整行作为行定位器）

### 2. 逻辑解码
- 基于 PostgreSQL 逻辑解码（Logical Decoding）机制
- 使用 JSON 格式存储变更元数据
- 支持事务级别的变更追踪

### 3. 后台工作者
- 自动创建后台工作者（Background Worker）
- 持续捕获数据库变更
- 支持多数据库监控

### 4. 数据管理
- 可配置的数据保留策略
- 自动清理过期数据
- 支持手动清理数据

### 5. 安全特性
- 自动过滤敏感 schema（如 hg_security）
- 支持自动设置表 replica identity

## 架构设计

### 核心组件

```
┌─────────────────────────────────────────────────────────────┐
│                    PostgreSQL Server                         │
│                                                              │
│  ┌─────────────────┐    ┌────────────────────────────────┐ │
│  │  Background     │    │  hg_flashback_tx Schema        │ │
│  │  Worker         │───▶│  ┌──────────────────────────┐   │ │
│  │                 │    │  │flashback_transaction_query │  │ │
│  │  - 监控数据库    │    │  │ (元数据表)                │   │ │
│  │  - 捕获变更      │    │  └──────────────────────────┘   │ │
│  │  - 生成Undo SQL │    │                                │ │
│  └─────────────────┘    └────────────────────────────────┘ │
│           │                                                   │
│           ▼                                                   │
│  ┌─────────────────┐                                        │
│  │  Logical        │                                        │
│  │  Decoding       │                                        │
│  │  (flashback_    │                                        │
│  │   decoder)      │                                        │
│  └─────────────────┘                                        │
└─────────────────────────────────────────────────────────────┘
```

### 数据流

1. **变更捕获阶段**
   - 后台工作者启动逻辑解码
   - 监听 WAL 日志变更
   - 使用 flashback_decoder 输出插件解析变更

2. **数据处理阶段**
   - 解析 JSON 格式的变更数据
   - 生成对应操作的撤销 SQL
   - 构建行定位器（基于ROWID或主键或完整行）

3. **数据存储阶段**
   - 将变更记录写入 `hg_flashback_tx.flashback_transaction_query` 表
   - 记录事务时间戳、操作类型、表信息、撤销 SQL 等

4. **数据清理阶段**
   - 根据 `undo_retention` 配置清理过期数据
   - 支持自动清理和手动清理

### 元数据表结构

```sql
CREATE TABLE hg_flashback_tx.flashback_transaction_query
(
    xid                 bigint,          -- 事务ID
    start_scn           numeric,         -- 事务开始SCN
    start_timestamp     timestamp,       -- 事务开始时间
    commit_scn          numeric,         -- 事务提交SCN
    commit_timestamp    timestamp,       -- 事务提交时间
    logon_user          varchar(128),    -- 操作用户
    "undo_change#"      numeric,         -- 变更编号
    operation           varchar(32),    -- 操作类型 (I/U/D)
    table_name          varchar(256),   -- 表名
    table_owner         varchar(386),   -- 表所有者
    row_id              varchar(128),   -- 行标识符
    undo_sql            varchar(4000)   -- 撤销SQL语句
);
```

### GUC 配置参数

| 参数名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `hg_flashback_tx.enable_flashback_transaction_query` | boolean | false | 启用事务回溯查询功能 |
| `hg_flashback_tx.enable_flashback_transaction_query_auto_purge` | boolean | false | 启用自动清理过期数据 |
| `hg_flashback_tx.worker_poll_interval` | integer | 1 | 后台工作者轮询间隔（秒） |
| `hg_flashback_tx.undo_retention` | integer | 0 | 撤销数据保留时间（秒），0 表示不自动清理 |
| `hg_flashback_tx.databases` | string | postgres | 要监控的数据库列表（逗号分隔） |
| `hg_flashback_tx.auto_replica_identity` | boolean | false | 自动为所有表设置 REPLICA IDENTITY FULL |

## 使用方法

### 安装步骤

1. **编译插件**

   ```bash
   cd contrib/hg_flashback_tx
   make
   make install
   ```

2. **配置 postgresql.conf**

   在 `postgresql.conf` 中添加以下配置：

   ```conf
   shared_preload_libraries = 'hg_flashback_tx'
   wal_level = logical
   max_replication_slots = 4
   max_wal_senders = 4
   ```

3. **创建扩展**

   ```sql
   CREATE EXTENSION hg_flashback_tx;
   ```

4. **启用功能**

   ```sql
   ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query = on;
   SELECT pg_reload_conf();
   ```

### 配置示例

**基本配置：**

```sql
-- 设置保留7天的撤销数据
ALTER SYSTEM SET hg_flashback_tx.undo_retention = 604800;

-- 启用自动清理
ALTER SYSTEM SET hg_flashback_tx.enable_flashback_transaction_query_auto_purge = on;

-- 监控多个数据库
ALTER SYSTEM SET hg_flashback_tx.databases = 'postgres, mydb';

-- 重新加载配置
SELECT pg_reload_conf();
```

**自动设置 replica identity（可选）：**

```sql
ALTER SYSTEM SET hg_flashback_tx.auto_replica_identity = on;
SELECT pg_reload_conf();
```

### 表的准备工作

为了让 flashback 功能正常工作，需要为表设置合适的 replica identity：

**方式一：使用自动设置功能（推荐）**

启用 `hg_flashback_tx.auto_replica_identity` 参数后，插件会自动为所有新建的表设置 `REPLICA IDENTITY FULL`：

```sql
ALTER SYSTEM SET hg_flashback_tx.auto_replica_identity = on;
SELECT pg_reload_conf();
```

**方式二：手动设置**

```sql
-- 为表设置完整行作为行定位器
ALTER TABLE my_table REPLICA IDENTITY FULL;

-- 或者使用默认值（主键）
ALTER TABLE my_table REPLICA IDENTITY DEFAULT;

-- 查看当前表的 replica identity
SELECT relname, relreplident FROM pg_class WHERE relkind = 'r';
```

### 查询历史变更

**查询指定时间范围内的事务：**

```sql
SELECT xid, commit_timestamp, operation, table_name, undo_sql
FROM hg_flashback_tx.flashback_transaction_query
WHERE commit_timestamp BETWEEN '2024-01-01 00:00:00' AND '2024-01-02 00:00:00'
ORDER BY commit_timestamp DESC;
```

**查询特定表的所有变更：**

```sql
SELECT *
FROM hg_flashback_tx.flashback_transaction_query
WHERE table_name = 'my_table'
ORDER BY commit_timestamp DESC;
```

**查询特定事务的变更：**

```sql
SELECT *
FROM hg_flashback_tx.flashback_transaction_query
WHERE xid = 12345;
```

### 执行撤销操作

根据 `undo_sql` 列生成的撤销语句，可以执行以下操作：

**撤销 DELETE 操作（恢复被删除的数据）：**

```sql
-- 查询 DELETE 操作的 undo_sql
SELECT undo_sql
FROM hg_flashback_tx.flashback_transaction_query
WHERE operation = 'D' AND table_name = 'my_table'
ORDER BY commit_timestamp DESC
LIMIT 1;

-- 执行撤销（示例）
-- undo_sql: INSERT INTO my_table (id, name) VALUES (1, 'test')
```

**撤销 INSERT 操作（删除错误插入的数据）：**

```sql
-- 查询 INSERT 操作的 undo_sql
SELECT undo_sql
FROM hg_flashback_tx.flashback_transaction_query
WHERE operation = 'I' AND table_name = 'my_table'
ORDER BY commit_timestamp DESC
LIMIT 1;

-- 执行撤销（示例）
-- undo_sql: DELETE FROM my_table WHERE id = 1
```

**撤销 UPDATE 操作（恢复旧值）：**

```sql
-- 查询 UPDATE 操作的 undo_sql
SELECT undo_sql
FROM hg_flashback_tx.flashback_transaction_query
WHERE operation = 'U' AND table_name = 'my_table'
ORDER BY commit_timestamp DESC
LIMIT 1;

-- 执行撤销（示例）
-- undo_sql: UPDATE my_table SET id=1, name='old_value' WHERE id = 1
```

### 手动清理数据

**清理所有历史数据：**

```sql
TRUNCATE hg_flashback_tx.flashback_transaction_query;
```

**清理特定时间之前的数据：**

```sql
DELETE FROM hg_flashback_tx.flashback_transaction_query
WHERE commit_timestamp < NOW() - INTERVAL '7 days';
```

### 监控和维护

**查看复制槽：**

```sql
SELECT * FROM pg_replication_slots WHERE slot_name LIKE 'hg_flashback_tx%';
```

**查看后台工作者状态：**

```sql
SELECT * FROM pg_stat_background_workers WHERE worker LIKE 'hg_flashback_tx%';
```

**查看扩展状态：**

```sql
SELECT * FROM pg_extension WHERE extname = 'hg_flashback_tx';
```

## 限制和注意事项

### 限制

1. **需要逻辑解码**：必须启用 `wal_level = logical`
2. **REPLICA IDENTITY 要求**：目标表必须设置合适的 replica identity
3. **系统表限制**：无法回溯系统表（如 pg_class）的变更
4. **DDL 不支持**：不支持回溯 CREATE、DROP、ALTER 等 DDL 操作
5. **大事务处理**：大事务可能产生大量撤销数据，需要注意存储空间

### 性能影响

1. 启用功能后会持续捕获 WAL 变更，对性能有一定影响
2. 撤销数据会写入元数据表，需要定期清理
3. 建议根据业务需求设置合理的 `undo_retention` 值

### 安全考虑

1. 撤销 SQL 包含敏感数据，应控制访问权限
2. 建议只对需要回溯功能的表启用监控
3. 定期清理不再需要的撤销数据

## 常见问题

**Q: 为什么某些表的变更没有被记录？**

A: 请检查以下几点：
1. 表是否设置了 `REPLICA IDENTITY`
2. `hg_flashback_tx.enable_flashback_transaction_query` 是否已启用
3. 表是否在敏感 schema 中（如 hg_security）

**Q: 如何提高撤销 SQL 的准确性？**

A: 确保表有主键或唯一索引，这样可以使用主键列作为行定位器，生成更精确的撤销 SQL。

**Q: 存储空间增长太快怎么办？**

A: 可以采取以下措施：
1. 减小 `undo_retention` 值
2. 启用 `enable_flashback_transaction_query_auto_purge`
3. 定期手动清理历史数据

**Q: 后台工作者没有启动怎么办？**

A: 检查以下几点：
1. `shared_preload_libraries` 是否包含 `hg_flashback_tx`
2. WAL 级别是否设置为 `logical`
3. 查看 PostgreSQL 日志中的错误信息

## 相关函数和视图

### 系统视图

- `pg_replication_slots`：查看创建的逻辑复制槽
- `pg_stat_background_workers`：查看后台工作者状态
- `pg_extension`：查看已安装的扩展

### 数据表

- `hg_flashback_tx.flashback_transaction_query`：存储所有变更的元数据

## 开发者信息

### 代码结构

- `hg_flashback_tx.c`：主插件代码，包含后台工作者、逻辑解码集成、元数据管理
- `flashback_decoder.c`：逻辑解码输出插件，负责解析 WAL 变更
- `hg_flashback_tx--1.0.sql`：扩展安装脚本，创建元数据表和索引

