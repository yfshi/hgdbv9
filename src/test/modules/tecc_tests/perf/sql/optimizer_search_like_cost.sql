-- 优化器基线：SEARCH LIKE 的 procost 应避免不合理的连接/过滤顺序。
SELECT count(*)
FROM enc_text t
JOIN enc_int4 i ON i.id = t.id
WHERE t.val_search LIKE '第一数据组%'
  AND i.val_ope >= 500
  AND i.val_ope < 600;
