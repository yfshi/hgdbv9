-- text_like_small - TECC性能测试
SELECT val_search FROM enc_text WHERE val_search LIKE '第%500%' LIMIT 100;
