# TECC 优化器性能基线

本文档用于记录优化器改动的可重复性能评估方法，覆盖密态操作符能力判断和密态函数代价调整。

## 覆盖范围

优化器专项性能用例如下：

- `optimizer_det_hash_join.sql`：DET 等值连接，验证安全 hash join 路径是否可用。
- `optimizer_ope_order_join.sql`：OPE 等值连接并按 OPE 列排序，验证保序密态列的排序/索引路径。
- `optimizer_semijoin_det.sql`：DET `IN` 半连接，验证安全 hash semijoin 路径是否可用。
- `optimizer_memoize_det.sql`：重复 DET lateral 查找，验证 memoize 路径是否可用。
- `optimizer_search_like_cost.sql`：SEARCH `LIKE` 叠加另一个选择性密态谓词，验证 `procost` 对连接/过滤顺序的影响。

`perf_ddl.sql` 会创建 `enc_int4_b`，其密态列定义和 DEK 与 `enc_int4` 保持一致。`perf_multiply.sql`
通过服务端密文复制从 `stage_int4` 填充 `enc_int4_b`，因此新增对照表不会增加客户端加密造数时间。

## 只运行这些用例

`TECC_TEST` 匹配完整 SQL 文件名，多个文件名用逗号分隔：

```bash
TECC_TEST=optimizer_det_hash_join.sql,optimizer_ope_order_join.sql,optimizer_semijoin_det.sql,optimizer_memoize_det.sql,optimizer_search_like_cost.sql \
PERF_ROWS=1000000 PERF_SEED=100000 PERF_RUNS=5 \
make check-tecc-perf-libpq
```

新增阈值项当前统一设置为 `0`，表示只记录耗时、不因阈值失败中断性能评估。等目标机器完成基线采集后，再固化真实阈值。

## 改动前后采集

建议用独立 worktree 采集改动前基线，这样无需重置当前工作区：

```bash
git worktree add ../tecc4_optimizer_before HEAD
cd ../tecc4_optimizer_before/src/test/modules/tecc_tests
TECC_TEST=optimizer_det_hash_join.sql,optimizer_ope_order_join.sql,optimizer_semijoin_det.sql,optimizer_memoize_det.sql,optimizer_search_like_cost.sql \
PERF_ROWS=1000000 PERF_SEED=100000 PERF_RUNS=5 \
make check-tecc-perf-libpq
```

然后在已应用优化器改动的工作区运行同一组命令：

```bash
cd ../tecc4_on_pg14/src/test/modules/tecc_tests
TECC_TEST=optimizer_det_hash_join.sql,optimizer_ope_order_join.sql,optimizer_semijoin_det.sql,optimizer_memoize_det.sql,optimizer_search_like_cost.sql \
PERF_ROWS=1000000 PERF_SEED=100000 PERF_RUNS=5 \
make check-tecc-perf-libpq
```

分别归档两次运行生成的 `perf/results/*.json` 和 `SUMMARY_*.txt`。优先比较 `avg_ms`、`runs_ms`
和 SQL 错误；如果耗时波动较大，可使用 `PERF_RUNS=9` 重新采集。

libpq 性能回归默认还会生成客户端资源报告：`perf/results/RESOURCE_*.md` 和 `RESOURCE_*.csv`。
报告包含客户端 `psql` 进程的 user/sys CPU、CPU 占比、最大 RSS、缺页和上下文切换；如需关闭，
可设置 `PERF_CLIENT_RESOURCE=0`。

采集完成后，将实测值填入 `baselines/optimizer_baseline_template.json`，或在同目录保存一份带日期的副本，
例如 `optimizer_baseline_20260824.json`。

## 执行计划检查

为了定位性能变化原因，可在改动前后分别对五个 SQL 文件手工执行 `EXPLAIN (COSTS ON)`，记录优化器选择的连接节点。
预期行为方向如下：

- DET 等值路径仅在密态操作数兼容时，可以使用 hash join、hash semijoin 和 memoize。
- OPE 等值路径仅在支持保序能力时，可以使用 merge/order-sensitive 路径。
- SEARCH 和同态函数在 `pg_proc.dat` 调整后，应向优化器体现更高计算代价。
