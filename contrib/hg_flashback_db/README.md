# hg_flashback_db

基于 PITR 和 pgBackRest 的 PostgreSQL 闪回数据库扩展。

## 概述

hg_flashback_db 是一个 PostgreSQL 恢复点元数据管理器，负责管理 restore point 元数据，并通过后台工作进程定期自动创建恢复点，帮助用户把恢复流程组织得更稳定、更可观测。

### 主要特性

- **恢复点管理**：支持创建手动和自动恢复点，可选择是否同时创建备份
- **自动备份**：后台工作进程按配置间隔自动创建恢复点并执行备份
- **自动清理**：自动清理过期的恢复点元数据

## 快速开始

### 1. 配置 PostgreSQL

```sql
-- postgresql.conf 配置
shared_preload_libraries = 'hg_flashback_db'

hg_flashback_db.pgbackrest_stanza = '<stanza_name>'
hg_flashback_db.pgbackrest_config_path = '<pgbackrest.conf>'

wal_level = replica
archive_mode = on
archive_command = 'pgbackrest --config=<pgbackrest.conf> --stanza=<stanza_name> --pg1-path=/home/highgo/data archive-push %p'
```

重启 PostgreSQL 服务使配置生效。

### 2. 安装扩展

```bash
cd contrib/hg_flashback_db
make && make install
```

```sql
CREATE EXTENSION hg_flashback_db;
```

### 3. 创建 pgBackRest Stanza

```bash
pgbackrest stanza-create --stanza=<stanza_name> --config=<pgbackrest.conf>
```

## 配置参数

| 参数名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `hg_flashback_db.db_flashback_logging_interval` | 整数 | 60 | 自动恢复点创建间隔（分钟） |
| `hg_flashback_db.db_flashback_retention_target` | 整数 | 10080 | 恢复点保留时间（分钟），默认 7 天。diff 在该时间后清理，full 在 2x 该时间后清理 |
| `hg_flashback_db.pgbackrest_stanza` | 字符串 | - | pgBackRest stanza 名称（必填） |
| `hg_flashback_db.pgbackrest_path` | 字符串 | pgbackrest | pgBackRest 可执行文件路径 |
| `hg_flashback_db.pgbackrest_config_path` | 字符串 | NULL | pgBackRest 配置文件路径 |
| `hg_flashback_db.pgbackrest_dry_run` | 整数 | 0 | 干运行模式：0=关闭，1=dry-run（仅记录不执行），2=test-bypass（执行但不解析JSON） |
| `hg_flashback_db.db_flashback_enabled` | 布尔 | true | 是否启用后台工作进程 |
| `hg_flashback_db.db_flashback_bgw_database` | 字符串 | postgres | 后台工作进程连接的数据库名 |
| `hg_flashback_db.auto_cleanup` | 布尔 | true | 自动清理孤立恢复点 |

## 使用方法

### 检查系统要求

```sql
SELECT * FROM hg_flashback_db.check_system_requirements();
```

### 创建恢复点

```sql
-- 仅创建恢复点，不创建备份
SELECT hg_flashback_db.create_restore_point('my_restore_point');

-- 创建恢复点并同时执行备份
SELECT hg_flashback_db.create_restore_point('my_restore_point', 'full');
SELECT hg_flashback_db.create_restore_point('my_restore_point', 'diff');
SELECT hg_flashback_db.create_restore_point('my_restore_point', 'incr');
```

- **参数1**：恢复点名称（唯一）
- **参数2**（可选）：备份类型 `full` / `diff` / `incr`，默认为空
- **返回值**：恢复点的 LSN

### 查询恢复点

```sql
SELECT * FROM hg_flashback_db.restore_points_base;
```

### 查询闪回数据库统计信息

```sql
SELECT * FROM hg_flashback_db.v$flashback_database_log;
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `OLDEST_FLASHBACK_SCN` | BIGINT | 数据库可闪回的最早 SCN（暂不支持，返回 NULL） |
| `OLDEST_FLASHBACK_TIME` | TIMESTAMP | 数据库可闪回的最早时间戳 |
| `RETENTION_TARGET` | BIGINT | 配置的闪回保留目标时间（分钟） |
| `FLASHBACK_SIZE` | BIGINT | 当前已使用的闪回日志总大小（字节），从 pgBackRest 备份大小汇总 |
| `ESTIMATED_FLASHBACK_SIZE` | BIGINT | 预估闪回日志大小（字节），计算公式：1个Full + (RETENTION_TARGET / LOGGING_INTERVAL) × Diff平均大小 |
| `CON_ID` | INTEGER | 数据所属容器 ID（始终返回 0） |

### 查询所有恢复点

```sql
SELECT * FROM hg_flashback_db.show_restore_point;
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `SCN` | BIGINT | 恢复点对应的 SCN（暂不支持，返回 NULL） |
| `RSP Time` | TIMESTAMP | SCN 对应的时间戳（暂不支持，返回 NULL） |
| `Type` | VARCHAR(3) | 恢复点类型（始终返回 GUARANTEED） |
| `Time` | TIMESTAMP | 恢复点创建时间 |
| `Name` | TEXT | 恢复点名称 |

### 删除恢复点

```sql
-- 删除恢复点及关联的备份数据
SELECT hg_flashback_db.drop_restore_point('my_restore_point');

-- 仅删除恢复点元数据，保留备份
SELECT hg_flashback_db.drop_restore_point('my_restore_point', true);
```

### 清理孤立恢复点

当 pgbackrest expire 删除备份时，关联的恢复点元数据会变成孤立状态。

```sql
-- 手动清理孤立恢复点
SELECT hg_flashback_db.cleanup_orphan_restore_points();
```

## 备份策略

后台工作进程自动管理备份创建时机：

1. **首次启动**：没有任何备份时，创建全量备份（full）
2. **已超出保留期**：当最新的全量备份已超出 retention_target 时间，创建新的全量备份
3. **常规情况**：创建差异备份（diff）

自动恢复点命名格式：`auto_YYYYMMDD_HHMMSS_MMM`

## pgBackRest 配置示例

```ini
[global]
repo1-path=/home/highgo/pgbackrestdata
log-path=/home/highgo/pgbackrestlog
log-level-console=info
log-level-file=detail
compress-type=zst
process-max=2
start-fast=y

[<stanza_name>]
pg1-path=/home/highgo/data
pg1-port=5432
pg1-user=highgo
pg1-database=postgres
pg1-host-user=highgo
pg1-socket-path=/tmp
```

### 常用命令

```bash
# 查看备份信息
pgbackrest info --config=<pgbackrest.conf> --stanza=<stanza_name>

# 恢复到最新备份点
pgbackrest restore --config=<pgbackrest.conf> --stanza=<stanza_name> --type=immediate

# 恢复到指定时间点
pgbackrest restore --config=<pgbackrest.conf> --stanza=<stanza_name> --type=time --target='2024-01-15 10:00:00'

# 停止 stanza
pgbackrest --config=<pgbackrest.conf> --stanza=<stanza_name> stop

# 删除 stanza
pgbackrest --config=<pgbackrest.conf> --stanza=<stanza_name> stanza-delete
```

## 恢复流程

### 首次配置与启动

1. **配置 pgBackRest**：编辑 pgbackrest.conf 文件
2. **启动数据库**
3. **安装扩展**：
   ```sql
   CREATE EXTENSION hg_flashback_db;
   ```
4. **创建 Stanza**：
   ```bash
   pgbackrest stanza-create --stanza=<stanza_name> --config=<pgbackrest_config_path>
   ```
5. **配置 PostgreSQL 参数**（postgresql.conf）：
   ```properties
   shared_preload_libraries = 'hg_flashback_db'
   hg_flashback_db.pgbackrest_stanza = '<stanza_name>'
   hg_flashback_db.pgbackrest_config_path = '<pgbackrest_config_path>'
   wal_level = replica
   archive_mode = on
   archive_command = 'pgbackrest --config=<pgbackrest_config_path> --stanza=<stanza_name> --pg1-path=<pgdata_path> archive-push %p'
   ```
6. **重启数据库使配置生效**

配置完成后，后台工作进程将自动按配置的间隔创建恢复点。

### 恢复步骤



1. **停止数据库**
2. **执行恢复**：
   ```bash
   # 按时间点恢复
   pgbackrest restore \
      --config=<pgbackrest_config_path> \
      --stanza=<stanza_name> \
      --pg1-path=<pgdata_path> \
      --delta \
      --type=time \
      --target="<restore_time>"

   # 按备份label恢复（停留在备份时刻）
   pgbackrest restore \
      --config=<pgbackrest_config_path> \
      --stanza=<stanza_name> \
      --pg1-path=<pgdata_path> \
      --delta \
      --set=<backup_label> \
      --type=immediate
   ```
3. **清理 WAL 日志**（必须，否则不会重放）：
   ```bash
   rm -f <pgdata_path>/pg_wal/*
   ```
4. **启动数据库**


### 恢复后配置

恢复完成后，需要创建新的 stanza 并更新归档命令：
```bash
# 创建新 stanza
pgbackrest stanza-create --stanza=<new_stanza_name> --config=<pgbackrest_config_path>

# 更新 postgresql.conf
hg_flashback_db.pgbackrest_stanza = '<new_stanza_name>'
archive_command = 'pgbackrest --stanza=<new_stanza_name> archive-push %p'

# 重载配置
pg_ctl reload -D <pgdata_path>
```

### 获取 backup_label

```sql
SELECT * FROM hg_flashback_db.show_restore_point;
pgbackrest info --stanza=<stanza_name>
```

## 注意事项

1. **备份存储**：确保有足够的存储空间
2. **性能影响**：后台工作进程会定期执行备份任务
3. **恢复测试**：定期测试恢复流程确保备份可用
4. **监控**：建议监控后台工作进程的运行状态和错误日志
