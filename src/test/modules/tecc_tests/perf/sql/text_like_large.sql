-- text_like_large - TECC性能测试
SELECT val_search FROM enc_text WHERE val_search LIKE '第%5%' LIMIT 10000;
