DROP TABLE IF EXISTS tb_numeric CASCADE; DROP DEK IF EXISTS dek_t1 CASCADE; DROP DEK IF EXISTS dek_t2 CASCADE; DROP KEK IF EXISTS kek_t1 CASCADE;
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);
CREATE DEK dek_t2 WITH VALUES (KEK=kek_t1);
select 1::int2_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2),pg_typeof(1::int2_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2));
select 1::int4_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2),pg_typeof(1::int4_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2));
select 1::int8_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2),pg_typeof(1::int8_enc encrypted with (dek = dek_t1)::numeric_enc encrypted with (dek = dek_t2));
DROP TABLE IF EXISTS tb_numeric CASCADE; DROP DEK IF EXISTS dek_t1 CASCADE; DROP DEK IF EXISTS dek_t2 CASCADE; DROP KEK IF EXISTS kek_t1 CASCADE;
