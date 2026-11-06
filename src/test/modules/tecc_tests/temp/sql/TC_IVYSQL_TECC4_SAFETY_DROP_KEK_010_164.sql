DROP KEK IF EXISTS kek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_delete_test.key');
DROP KEK kek_test CASCADE;
