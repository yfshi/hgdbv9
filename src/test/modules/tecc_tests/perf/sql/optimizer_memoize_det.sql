-- 优化器基线：重复 DET lateral 查找应允许选择 memoize 路径。
SELECT count(*)
FROM enc_int4 a
JOIN LATERAL (
    SELECT b.id
    FROM enc_int4_b b
    WHERE b.val_det = a.val_det
    LIMIT 1
) b ON true
WHERE a.id <= 300000;
