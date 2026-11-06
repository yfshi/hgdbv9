DROP KEK IF EXISTS kek_exist CASCADE;
CREATE KEK kek_exist WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_exist.key');
DROP KEK kek_exist CASCADE;
