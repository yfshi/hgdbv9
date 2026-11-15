# TECC 扩展性能测试套件

`perf_ext` 是一套更贴近客户员工信息业务模型的扩展性能测试，独立于现有 `perf` 测试套件。

测试会创建三张字段完全一致的员工信息表，共 19 个逻辑字段：

- `employee_plain`：不包含密态业务列。
- `employee_full_enc`：业务列全部为密态列；`row_id` 保持明文，用于大结果集分页和预置数据倍增。
- `employee_half_enc`：约一半业务列为密态列。

当前 `perf_ext` 默认用例不包含同态加密列，避免同态解密成本干扰普通密态列性能回归。`employee_full_enc.age`、`employee_full_enc.grade` 和 `employee_half_enc.age` 使用普通密态整型列。

默认场景配置位于 `config/scenarios.json`，共 45 个场景，覆盖三种表形态、1 / 50 / 200 并发，以及以下三类测试：

- DML性能对比：insert，以及通过明文 `row_id` 定位单行并更新 `phone` 的 update。
- 等值返回：DET/明文等值查询固定返回 1 行。
- 批量返回：OPE/明文 `BETWEEN` 固定返回 1000 行，SEARCH/明文 `LIKE` 固定返回 1000 行。

普通 `perf_ext` 不包含同态数据类型。谓词专项在明文、半密态、全密态表上使用相同业务字段和相同 SQL 形态，主要用于比较普通密态与明文查询性能：

| 场景前缀 | 对比字段 | 查询形式 | 固定返回数 |
|---|---|---|---|
| `eq1` | `name`：明文 / DET | `WHERE name = ?` | 1 行 |
| `bt1k` | `salary`：明文 / OPE | `WHERE salary BETWEEN 300000.01 AND 300010.00` | 1000 行 |
| `like1k` | `address`：明文 / SEARCH | `WHERE address LIKE 'PERF-LIKE-MATCH-%'` | 1000 行 |

等值查询为三张表的 `name` 创建 hash 索引；范围查询复用三张表的 `salary` btree 索引。`LIKE` 场景不额外创建索引，用于比较明文和 SEARCH 密态表达式的扫描与匹配成本。

造数阶段在目标总行数中预留 10000 条谓词测试数据：名称 `perf_eq_1` 至 `perf_eq_10000` 各出现一次，工资为 `300000.01` 至 `300100.00`，其中前 1000 条地址以前缀 `PERF-LIKE-MATCH-` 开头。JMeter 等值场景随机选择其中一个唯一名称，独立 psql 计时固定使用 `perf_eq_1`；工资区间和地址前缀查询均精确返回 1000 行，且不使用 `LIMIT`。所有查询只选择 `row_id` 和参与谓词的业务列，减少无关字段解密对性能对比的干扰。

所有查询均不使用 `ORDER BY`。UPDATE 场景只更新 `phone`：该字段在半密态和全密态表中使用默认随机加密，`WHERE` 使用明文且已建索引的 `row_id`，用于对比明文写入与普通密态重新加密的端到端成本。

密态字段按实际查询用途配置算法：仅 `name` 开启 DET，`salary` 开启 OPE，`address` 开启 SEARCH；其余普通密态字段使用默认随机加密，不为未使用的查询能力附加额外密文和计算成本。

## 测试步骤

1. 准备 TECC PostgreSQL 构建环境。

   在 Linux 开发环境中进入源码根目录，完成配置、编译和临时安装：

   ```sh
   ./teccdevconf.sh
   make -j$(nproc)
   ```

   `make check-tecc-perf-ext` 会通过 PostgreSQL TAP 框架启动临时数据库节点。

2. 准备 JMeter 和 JDBC 驱动。

   确认 JMeter 可执行文件可用。如果 `jmeter` 不在 `PATH` 中，需要显式指定：

   ```sh
   export PERF_EXT_JMETER_BIN=/opt/apache-jmeter-5.6.3/bin/jmeter
   ```

   如果需要指定 TECC JDBC 驱动 jar：

   ```sh
   export PERF_EXT_JDBC_JAR=/path/to/postgresql.jar
   ```

   若已设置 `TECC_JDBC_HOME`，测试脚本会优先尝试从 `TECC_JDBC_HOME/pgjdbc/build/libs/` 中寻找 `postgresql*.jar`。
   同目录下若有历次构建残留的多个版本，按**版本号取最高**（优先 `-all.jar`，即 Gradle 的 shadow 产物），
   规则见 `t/tecc_jdbc_jar.pm`，选中的 JAR 会打到输出里。

3. 选择数据量和场景。

   默认情况下，脚本会根据本次选中的场景自动选择最大数据量标签造数。完整默认组合会准备到 5000 万行；只跑 1000 万场景时，只需要准备 1000 万行。

   可以显式指定每张表目标行数：

   ```sh
   export PERF_EXT_ROWS=10000000
   ```

   可以用 `TECC_TEST` 过滤场景：

   ```sh
   export TECC_TEST=eq1_full_c50_10m
   ```

   多个场景用逗号分隔：

   ```sh
   export TECC_TEST=eq1_plain_c50_10m,eq1_half_c50_10m,eq1_full_c50_10m
   ```

4. 运行测试。

   在 `src/test/modules/tecc_tests` 目录或源码根目录下运行：

   ```sh
   make check-tecc-perf-ext
   ```

   脚本会自动完成以下动作：

   - 创建临时 PostgreSQL 节点；
   - 创建 KEK/DEK 和三张员工信息表；
   - 通过客户端加密生成 seed 数据；
   - 使用 `psql -k off` 在服务端复制密文，将数据扩展到目标行数；
   - 为每个场景生成 JMeter JMX 文件；
   - 依次采集 psql `\timing`、服务端 `EXPLAIN ANALYZE` 和 JMeter 并发压测结果。

5. 查看结果。

   结果文件保存在 `perf_ext/results/` 目录：

   - `perf_ext_results.json`：完整 JSON 结果。
   - `perf_ext_results.csv`：汇总 CSV，便于导入 Excel 或其他报表工具。
   - `*.jmx`：每个场景生成的 JMeter 测试计划。
   - `*.jtl`：JMeter 原始采样结果。
   - `*.log`：JMeter 执行日志。

## 客户端/服务端分立部署测试

支持客户端和服务端分开部署。测试脚本运行在客户端机器上，远端服务端需要提前启动并允许客户端 TCP 连接。

服务端侧准备：

```sh
pg_ctl -D /path/to/pgdata -l /tmp/tecc_perf_ext_server.log start -o "-p 5436"
```

客户端侧运行：

```sh
export TECC_PERF_EXT_EXTERNAL_CONNSTR="host=192.168.102.12 port=5436 dbname=postgres user=postgres"
export TECC_PERF_EXT_EXTERNAL_PSQL=/path/to/tecc/bin/psql
export TECC_PERF_EXT_EXTERNAL_KTOOL=/path/to/tecc/bin/tecc_ktool
export TECC_PERF_EXT_EXTERNAL_PORT=5436

export PERF_EXT_DB_HOST=192.168.102.12
export PERF_EXT_DB_PORT=5436
export PERF_EXT_DB_NAME=postgres
export PERF_EXT_DB_USER=postgres
export PERF_EXT_DB_PASSWORD=""
export PERF_EXT_JMETER_BIN=/opt/apache-jmeter-5.6.3/bin/jmeter
export PERF_EXT_JDBC_JAR=/path/to/postgresql.jar

make check-tecc-perf-ext
```

注意事项：

- `TECC_PERF_EXT_EXTERNAL_CONNSTR` 用于 psql 建表、造数、`\timing` 和 `EXPLAIN ANALYZE`。
- `PERF_EXT_DB_HOST` / `PERF_EXT_DB_PORT` 或 `PERF_EXT_JDBC_URL` 用于 JMeter 并发压测。
- KEK 文件路径写入元数据后由客户端读取。分立部署时，运行 psql 和 JMeter 的客户端需要能访问相同的 `TECC_KEK_PATH` 或相同的 `key_path`。
- 如果 psql 和 JMeter 不在同一台客户端机器上，需要把生成的 KEK 文件同步到 JMeter 所在机器，并保持路径一致。

## 常用环境变量

- `TECC_PERF_EXT_EXTERNAL_CONNSTR=...`：启用客户端/服务端分立部署测试。设置后，脚本不会启动本地临时 PostgreSQL 节点，而是通过该连接串连接远端服务端。
- `TECC_PERF_EXT_EXTERNAL_PSQL=/path/to/psql`：分立部署时使用的 TECC 版 `psql`。
- `TECC_PERF_EXT_EXTERNAL_KTOOL=/path/to/tecc_ktool`：分立部署时使用的 `tecc_ktool`。
- `TECC_PERF_EXT_EXTERNAL_PORT=5436`：分立部署时用于报告展示的远端端口。
- `PERF_EXT_DB_HOST=server_ip`、`PERF_EXT_DB_PORT=5436`、`PERF_EXT_DB_NAME=postgres`、`PERF_EXT_DB_USER=postgres`、`PERF_EXT_DB_PASSWORD=...`：JMeter 连接远端服务端时使用的数据库连接参数。
- `PERF_EXT_JDBC_URL=...`：也可以直接指定完整 JDBC URL。分立部署时推荐显式设置，确保 JMeter 压测连接到远端服务端。
- `PERF_EXT_ROWS=10000000|30000000|50000000`：每张表的目标行数。未设置时，按所选场景中的最大数据量标签造数。
- `PERF_EXT_SEED_ROWS=100000`：先通过客户端加密生成的种子行数，后续通过服务端复制密文扩容。
- `PERF_EXT_JMETER_DURATION=60`：JMeter ThreadGroup 持续时间，单位秒。
- `PERF_EXT_JMETER_RAMP=10`：JMeter ramp-up 时间，单位秒。
- `PERF_EXT_JMETER_BIN=/path/to/jmeter`：JMeter 可执行文件路径。
- `PERF_EXT_JDBC_JAR=/path/to/postgresql.jar`：可选的 JDBC 驱动 classpath。
- `PERF_EXT_JMETER_RESULT_SET_HANDLER=Count Records|Store as String` : 可选的JMeter ResultSet 处理方式。默认 1 万行和 10 万行查询使用 `Count Records`, 避免 JMeter 保存大结果集导致JVM OOM;其他场景使用`Store as String`.
- `PERF_EXT_JMETER_FETCH_SIZE=1000`：可选的 JDBC fetch size。默认 1 万行和 10 万行查询使用 1000，并将 JMeter JDBC 连接设为 `autocommit=false`，让 PgJDBC 使用游标分批拉取结果，避免一次性取回大结果集导致客户端 OOM。
- `PERF_EXT_JMETER_AUTOCOMMIT=true|false`：可选的 JMeter JDBC autocommit 覆盖值。未设置时，大结果集查询为 `false`，其他场景为 `true`。
- `PERF_EXT_JMETER_RESULT_SET_HANDLER=Count Records|Store as String`：可选的 JMeter ResultSet 处理方式。默认 1000 行及以上查询使用 `Count Records`，避免 JMeter 保存大结果集导致 JVM OOM；其他场景使用 `Store as String`。
- `PERF_EXT_JMETER_FETCH_SIZE=1000`：可选的 JDBC fetch size。默认 1000 行及以上查询使用 1000，并将 JMeter JDBC 连接设为 `autocommit=false`，让 PgJDBC 使用游标分批拉取结果，避免一次性取回大结果集导致客户端 OOM。
- `PERF_EXT_JMETER_AUTOCOMMIT=true|false`：可选的 JMeter JDBC autocommit 覆盖值。未设置时，大结果集查询为 `false`，其他场景为 `true`。
- `PERF_EXT_JMETER_PREINIT=true|false`：可选的 JMeter JDBC 连接池预初始化开关。默认 `false`。密态 JDBC 建连会加载 TECC 元数据和密钥上下文，高并发下不建议预先一次性创建全部连接，否则可能在线程组有效采样前耗尽测试窗口，导致 JTL 只有表头、`samples=0`。
- `PERF_EXT_JMETER_CONNECTION_AGE=0`：可选的 JMeter JDBC 连接最大生命周期，单位毫秒。默认 `0`，表示不按年龄回收连接。密态压测不建议设置过小值；例如 `5000` 会导致连接每 5 秒被回收，反复触发密态建连和上下文初始化，容易造成 JMeter 空采样或吞吐异常偏低。
- `PERF_EXT_JDBC_URL=...`：可选的完整 JDBC URL。默认包含 `enable_tecc=1`，不再强制 `preferQueryMode=simple`。大结果集查询依赖 PgJDBC fetch size 和服务端游标分批拉取结果，强制 simple query mode 会破坏该路径并显著增加 JMeter/JDBC 内存压力。
- `TECC_TEST=eq1_full_c50_10m`：只运行单个场景，也可以用逗号分隔运行多个场景。
- `PERF_EXT_SKIP_INIT=1`：跳过建表、建密钥和预置数据初始化，直接复用已有库继续跑剩余场景。该模式必须同时设置 `TECC_PERF_EXT_EXTERNAL_CONNSTR`，并保证客户端仍能访问初始化时使用的 `kek_perf_ext.key`。
- `PERF_EXT_KEEP_RESULTS=1`：保留 `perf_ext/results/` 下已有结果文件。未设置时，普通初始化运行会清理旧结果；`PERF_EXT_SKIP_INIT=1` 时默认不清理旧结果。

## 中断后继续运行

如果测试中途断开，但数据库中的三张员工表和 key 文件已经保留，可以跳过初始化继续跑指定场景：

```sh
export TECC_PERF_EXT_EXTERNAL_CONNSTR="host=192.168.94.40 port=5432 dbname=postgres user=postgres"
export TECC_PERF_EXT_EXTERNAL_PSQL=/path/to/tecc/bin/psql
export TECC_PERF_EXT_EXTERNAL_KTOOL=/path/to/tecc/bin/tecc_ktool
export TECC_KEK_PATH=/path/to/perf_ext_keys
export PERF_EXT_SKIP_INIT=1
export TECC_TEST=q1k_plain_c50_30m,q1k_half_c50_30m

make check-tecc-perf-ext
```
注意：不要在复用已有密态数据时重新生成 key 文件，否则已有密文无法解密。若 DDL 或场景字段配置发生变化，需要取消 `PERF_EXT_SKIP_INIT` 并重新初始化。

## 采集指标

每个场景会记录以下指标：

- 一次 psql `\timing` 往返耗时；
- 一次服务端 `EXPLAIN (ANALYZE, TIMING OFF, SUMMARY ON)` 执行时间；
- 一次 JMeter 并发压测结果，包括平均耗时、吞吐量、错误数；
- JSON/CSV 中的 `kind` 标识 `insert`、`update`、`query`，`query_type` 标识 `eq`、`between`、`like`；批量扫描场景该字段为空；
- 在 GNU `time -v` 可用时，采集 JMeter 进程 CPU、内存 RSS 等客户端资源占用。

生成的 JMX、JTL、JSON、CSV 和资源采样数据保存在 `perf_ext/results/` 目录。
