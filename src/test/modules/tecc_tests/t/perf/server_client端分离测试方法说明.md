# TECC libpq 性能回归外部服务端运行说明

## 使用场景

需要把服务端和客户端分离时，在客户端机器运行 `make check-tecc-perf-libpq`，通过外部连接串访问远端 TECC PostgreSQL 服务端。

## 服务端准备

1. 使用包含待测内核改动的源码编译并安装服务端。
2. 初始化并启动数据库，确保 `postgres` 用户可从客户端访问。
3. `postgresql.conf` 中配置监听地址和端口，例如：

```conf
listen_addresses = '*'
port = 5436
```

4. `pg_hba.conf` 放通客户端地址，例如：

```conf
host all postgres 客户端IP/32 trust
```

## 客户端准备

客户端需要使用 TECC 版 `psql`、`libpq` 和 `tecc_ktool`。

```bash
export PATH=/path/to/tecc/bin:$PATH
export LD_LIBRARY_PATH=/path/to/tecc/lib:$LD_LIBRARY_PATH
export TECC_KEK_PATH=/path/to/client/keys
mkdir -p "$TECC_KEK_PATH"
```

先确认客户端能连通服务端：

```bash
psql "host=服务端IP port=5436 dbname=postgres user=postgres" -c "select version();"
```
若需要验证 TECC 客户端透明加密链路，连接时应显式开启客户端加密：

```bash
psql -k on "host=服务端IP port=5436 dbname=postgres user=postgres" -c "select 1;"
```
## 运行性能回归

```bash
cd src/test/modules/tecc_tests

TECC_PERF_EXTERNAL_CONNSTR="host=服务端IP port=5436 dbname=postgres user=postgres" \
TECC_PERF_EXTERNAL_PSQL=/path/to/tecc/bin/psql \
TECC_PERF_EXTERNAL_KTOOL=/path/to/tecc/bin/tecc_ktool \
TECC_PERF_EXTERNAL_PORT=5436 \
TECC_PERF_EXTERNAL_DATADIR=/path/to/server/data \
TECC_PERF_CLIENT_MODE=on \
PERF_ROWS=1000000 \
PERF_RUNS=5 \
make check-tecc-perf-libpq
```

只跑指定用例：

```bash
TECC_TEST=optimizer_det_hash_join.sql \
TECC_PERF_EXTERNAL_CONNSTR="host=服务端IP port=5436 dbname=postgres user=postgres" \
TECC_PERF_CLIENT_MODE=on \
make check-tecc-perf-libpq
```

## 注意事项

- 设置 `TECC_PERF_EXTERNAL_CONNSTR` 后，脚本不会创建本机临时 PostgreSQL 节点。
- 外部服务端上的目标数据库会执行 `perf_ddl.sql`，其中包含 `DROP TABLE/DEK/KEK`，请使用专用测试库。
- KEK 文件目录以客户端 `TECC_KEK_PATH` 为准，脚本会在客户端生成同态 KEK 私钥文件。
- 客户端资源报告仍采集客户端 `psql` 进程的 CPU、RSS、缺页和上下文切换数据。
- 若不设置 `TECC_PERF_EXTERNAL_PSQL` 或 `TECC_PERF_EXTERNAL_KTOOL`，脚本会从客户端 `PATH` 中查找。
- `TECC_PERF_CLIENT_MODE` 控制 `psql -k` 参数，默认值为 `on`；服务端密文倍增阶段固定使用 `-k off`，避免把已有密文再次加密。