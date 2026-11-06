DROP KEK IF EXISTS kek_test CASCADE;
DROP DEK IF EXISTS kek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
CREATE DEK kek_test WITH VALUES(KEK = kek_test);
DROP DEK kek_test CASCADE;
DROP KEK kek_test CASCADE;
