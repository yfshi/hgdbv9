DROP TABLE IF EXISTS tb1 CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
DROP KEK IF EXISTS kek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_test.key');
CREATE DEK dek_test WITH VALUES(KEK = kek_test);
CREATE TABLE tb1(id int, name text_enc encrypted with (dek=dek_test));
GRANT ALL ON TABLE tb1 TO PUBLIC;
GRANT USAGE ON KEK kek_test TO PUBLIC;
GRANT USAGE ON DEK dek_test TO PUBLIC;
