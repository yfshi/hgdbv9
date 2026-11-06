DROP KEK IF EXISTS kek_test CASCADE;
DROP DEK IF EXISTS dek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
CREATE DEK dek_test WITH VALUES(KEK = kek_test);
CREATE DEK dek_test WITH VALUES(KEK = kek_test);
DROP DEK dek_test CASCADE;
DROP KEK kek_test CASCADE;
