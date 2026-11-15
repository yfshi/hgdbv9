-- text_like_large - TECC性能测试（前缀匹配5字符，命中率20%：返回10000行，扫描约50000行）
-- 数据：val_search = 5个5字符前缀轮换（第一数据组..第五数据组）+ 6位数字后缀
SELECT val_search FROM enc_text WHERE val_search LIKE '第一数据组%' LIMIT 10000;
