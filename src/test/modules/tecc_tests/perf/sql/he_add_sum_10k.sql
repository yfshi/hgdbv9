-- he_add_sum_10k - TECC性能测试：同态加法 SUM 1万行
-- 需求 4.7.1：同态加法 SUM 1万行（服务端密文聚合 → 客户端解密一次）
-- 典型场景：对1万行订单/流水做密文求和
-- 用 WHERE 而非 LIMIT 限定 1 万行（LIMIT 会阻止 PG 并行聚合，
-- WHERE 过滤可触发 Gather + Partial Aggregate 并行聚合，加速服务端 EC 加法）。
SELECT SUM(val_add) FROM he_int8 WHERE id <= 10000;
