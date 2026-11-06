-- float8_range_large - TECC性能测试
SELECT val_ope FROM enc_float8 WHERE val_ope >= 0.000500 AND val_ope < 0.000600 LIMIT 10000;
