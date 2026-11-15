# TECC 扩展性能测试套件（同态/普通密态混合版）

`perf_ext_he` 是从原 `perf_ext` 中拆出的同态/普通密态混合测试项目，独立于当前普通密态版 `perf_ext`。该项目包含 45 个默认场景，用于单独评估同态列参与后的客户端加解密、JDBC 查询、JMeter 并发和服务端执行开销，并增加明文/半密态/全密态的单行 UPDATE 性能对比。

## 表模型

测试会创建三张字段一致的员工信息表：

- `employee_plain`：明文业务表，不包含密态业务列。
- `employee_half_enc`：半密态业务表，约一半业务列为密态列，其中 `age` 使用同态加法列。
- `employee_full_enc`：全密态业务表，除 `row_id` 外业务列均为密态列，其中 `age` 使用同态加法列，`grade` 使用同态乘法列。

同态列配置如下：

- `employee_half_enc.age`：`INT8_ENC`，`SUPPORT_ALGO = 'he-add'`，DEK 为 `dek_perf_ext_he_add`。
- `employee_full_enc.age`：`INT8_ENC`，`SUPPORT_ALGO = 'he-add'`，DEK 为 `dek_perf_ext_he_add`。
- `employee_full_enc.grade`：`INT8_ENC`，`SUPPORT_ALGO = 'he-mul'`，DEK 为 `dek_perf_ext_he_mul`。

除上述同态字段外，混合项目中的普通密态字段均使用默认随机加密；当前场景不执行这些字段的等值、范围或模糊匹配，因此不额外开启 DET、OPE 或 SEARCH。

普通密态列继续覆盖 `det`、`ope`、`search` 等算法，便于和同态列开销一起观察。

## 运行方式

在 `src/test/modules/tecc_tests` 目录或源码根目录运行：

```sh
make check-tecc-perf-ext-he
```

默认读取：

- 场景配置：`perf_ext_he/config/scenarios.json`
- DDL：`perf_ext_he/sql/perf_ext_ddl.sql`
- 数据倍增 SQL：`perf_ext_he/sql/perf_ext_multiply.sql`
- 结果目录：`perf_ext_he/results/`

当前普通密态版仍使用：

```sh
make check-tecc-perf-ext
```

两套项目共享 `t/tecc_perf_ext_utils.pm` 的执行逻辑，但通过入口脚本设置的 `PERF_EXT_PROJECT` 自动选择不同目录。

## 密钥文件

初始化时会生成两类 KEK 文件：

- `kek_perf_ext.key`：普通密态列使用的 SM4 KEK。
- `kek_perf_ext_he.priv` / `kek_perf_ext_he.pub`：同态列使用的 SM2 KEK 密钥对。

默认目录为 `tmp_check/perf_ext_he_keys`。如需复用已有库或客户端/服务端分立部署，可显式设置：

```sh
export TECC_KEK_PATH=/path/to/perf_ext_he_keys
```

使用 `PERF_EXT_SKIP_INIT=1` 复用已有数据时，必须保证上述密钥文件仍存在且路径可被 psql 和 JMeter 所在客户端访问。

## 常用环境变量

- `PERF_EXT_ROWS=10000000|30000000|50000000`：指定每张表目标行数。
- `PERF_EXT_SEED_ROWS=100000`：客户端加密生成的种子数据行数。
- `PERF_EXT_SEED_BATCH=500`：种子数据批量插入大小。
- `TECC_TEST=场景名[,场景名...]`：只运行指定场景。
- `PERF_EXT_SKIP_INIT=1`：跳过建表、建密钥和造数，复用已有数据库继续跑。
- `PERF_EXT_KEEP_RESULTS=1`：保留已有结果文件。
- `PERF_EXT_JMETER_BIN=/path/to/jmeter`：指定 JMeter。
- `PERF_EXT_JDBC_JAR=/path/to/postgresql.jar`：指定 TECC JDBC 驱动。
- `JVM_ARGS="-Xms4g -Xmx16g"`：为大结果集或高并发场景提高 JMeter JVM 堆内存。

## 客户端/服务端分立部署

服务端提前启动并允许客户端连接后，在客户端设置：

```sh
export TECC_PERF_EXT_EXTERNAL_CONNSTR="host=192.168.102.12 port=5436 dbname=postgres user=postgres"
export TECC_PERF_EXT_EXTERNAL_PSQL=/path/to/tecc/bin/psql
export TECC_PERF_EXT_EXTERNAL_KTOOL=/path/to/tecc/bin/tecc_ktool

export PERF_EXT_DB_HOST=192.168.102.12
export PERF_EXT_DB_PORT=5436
export PERF_EXT_DB_NAME=postgres
export PERF_EXT_DB_USER=postgres
export PERF_EXT_JMETER_BIN=/opt/apache-jmeter-5.6.3/bin/jmeter
export PERF_EXT_JDBC_JAR=/path/to/postgresql.jar

make check-tecc-perf-ext-he
```

`TECC_PERF_EXT_EXTERNAL_CONNSTR` 用于 psql 建表、造数、`\timing` 和 `EXPLAIN`；`PERF_EXT_DB_*` 或 `PERF_EXT_JDBC_URL` 用于 JMeter 并发压测。

## 输出结果

结果保存在 `perf_ext_he/results/`：

- `perf_ext_results.json`：完整 JSON 结果。
- `perf_ext_results.csv`：汇总 CSV。
- `*.jmx`：每个场景生成的 JMeter 测试计划。
- `*.jtl`：JMeter 原始采样结果。
- `*.log`：JMeter 执行日志。
