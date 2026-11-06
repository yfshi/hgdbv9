DROP KEK IF EXISTS kek_test CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/nonexistent_dir/kek.key');
DROP KEK IF EXISTS kek_test CASCADE;
