# Oracle vs KingbaseES 事务对比测试用例

本目录包含用于对比 Oracle 与 KingbaseES Oracle 模式事务语义的 Python 测试脚本。

## 环境要求

- Python 3.9+
- `oracledb`
- `psycopg2-binary`

安装：

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
```

## 默认连接参数

脚本支持环境变量覆盖：

### Oracle

| 变量 | 默认值 |
|---|---|
| `ORACLE_USER` | `test` |
| `ORACLE_PASSWORD` | `test` |
| `ORACLE_DSN` | `localhost:1522/FREEPDB1` |

### KingbaseES

| 变量 | 默认值 |
|---|---|
| `KINGBASE_HOST` | `127.0.0.1` |
| `KINGBASE_PORT` | `54321` |
| `KINGBASE_USER` | `kingbase` |
| `KINGBASE_PASSWORD` | `Kingbase123` |
| `KINGBASE_DB` | `kingbase` |

## 运行

```bash
./run_all.sh
```

或分别运行：

```bash
python3 oracle_kingbase_tx_compare.py
python3 oracle_kingbase_tx_supplement.py
```

结果文件默认：

```text
/tmp/tx_compare_results.json
/tmp/tx_supp_results.json
```

可通过 `TX_RESULT_FILE`、`TX_SUPP_RESULT_FILE` 覆盖。

## 测试范围

主测试集覆盖：

- autocommit on/off 可见性
- 显式事务中失败 DML / SELECT
- 未处理 RAISE / SQL 错误
- PL/SQL 异常块与 handler
- 嵌套异常块
- handler 自身异常
- 异常块内 COMMIT / ROLLBACK
- 过程内异常与 COMMIT
- 过程未处理异常
- PL/SQL / SQL savepoint
- 跨层 savepoint
- DDL / TRUNCATE 隐式提交
- READ ONLY / SERIALIZABLE 快照
- 自治事务提交与异常

补充测试集覆盖：

- savepoint 目标保留
- 同名 savepoint
- `COMMIT AND CHAIN`
- 函数 / 触发器内 `COMMIT`
- `FORALL` 失败
- `FORALL SAVE EXCEPTIONS` 预留

## 注意事项

1. 测试会创建并自动清理 `T*`、`SA`、`SB`、`SC`、`SD`、`SE`、`SF*` 等表。
2. Kingbase 主测试集默认设置 `ora_statement_level_rollback=on`，用于和 Oracle 严格语句级语义对比；产品默认该参数为 `off`，默认行为应参考 T-OFF-* 用例。
3. DB-API 驱动会自动开始事务，涉及“无 `BEGIN`”的服务器语义需要原生客户端（如 `ksql`）补充验证。
4. Oracle 实测版本为 26ai Free，不是 19c；如需 19c 精确基线，请单独部署 19c。
5. 测试结果写入 `/tmp`，不修改仓库中的 expected 文件。
