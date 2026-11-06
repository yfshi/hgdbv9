DROP KEK IF EXISTS kek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
DROP KEK kek_test CASCADE;
