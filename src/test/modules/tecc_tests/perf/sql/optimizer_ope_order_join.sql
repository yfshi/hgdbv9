-- 优化器基线：OPE 等值连接应保留排序和索引友好的路径。
SELECT a.val_ope
FROM enc_int4 a
JOIN enc_int4_b b ON a.val_ope = b.val_ope
WHERE a.id <= 100000 AND b.id <= 100000
ORDER BY a.val_ope
LIMIT 10000;
