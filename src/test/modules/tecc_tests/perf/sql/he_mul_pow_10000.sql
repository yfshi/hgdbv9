-- he_mul_pow_10000 - TECC性能测试：同态乘法列 幂运算（a^10000，只返回1行）
-- 服务端重复 FF-ElGamal 乘法 9999 次（a^2..a^10000），仅体现运算不体现解密
-- （客户端只解密 1 行结果）
SELECT val_mul ^ 10000 FROM he_int8 LIMIT 1;
