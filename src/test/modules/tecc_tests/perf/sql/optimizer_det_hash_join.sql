-- 优化器基线：DET 等值连接应允许选择 hash join 路径。
SELECT count(*)
FROM enc_int4 a
JOIN enc_int4_b b ON a.val_det = b.val_det
WHERE a.id <= 100000 AND b.id <= 100000;
