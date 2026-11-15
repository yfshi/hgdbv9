-- perf_multiply.sql — 从 stage 倍增密文到 target 表
-- 由 test 脚本根据 PERF_ROWS/PERF_SEED 控制迭代次数
-- 注意：密文是纯字节，服务端复制不触发客户端加密

INSERT INTO enc_int4 (val_det, val_ope) SELECT val_det, val_ope FROM stage_int4;
INSERT INTO enc_int4_b (val_det, val_ope) SELECT val_det, val_ope FROM stage_int4;
INSERT INTO enc_float8 (val_det, val_ope) SELECT val_det, val_ope FROM stage_float8;
INSERT INTO enc_text (val_det, val_ope, val_search) SELECT val_det, val_ope, val_search FROM stage_text;
INSERT INTO he_int8 (val_add, val_mul) SELECT val_add, val_mul FROM stage_he_int8;
