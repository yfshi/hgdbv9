-- text_range_large - TECC性能测试
SELECT val_ope FROM enc_text WHERE val_ope >= '第500条测试记录' AND val_ope < '第600条测试记录' LIMIT 10000;
