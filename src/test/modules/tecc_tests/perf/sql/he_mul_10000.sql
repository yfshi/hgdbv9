-- he_mul_10000 - TECC性能测试：同态乘法列 解密性能（返回1万行密文，客户端解密）
-- 只返回不解密（select a，非 a*a 运算），体现客户端解密性能
SELECT val_mul FROM he_int8 LIMIT 10000;
