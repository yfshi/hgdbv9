-- 优化器基线：DET IN 子查询应允许选择 hash semijoin 路径。
SELECT count(*)
FROM enc_int4 a
WHERE a.id <= 100000
  AND a.val_det IN (
      SELECT b.val_det
      FROM enc_int4_b b
      WHERE b.id <= 100000
  );
