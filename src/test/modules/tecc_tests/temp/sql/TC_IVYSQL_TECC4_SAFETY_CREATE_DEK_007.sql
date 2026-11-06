DROP KEK IF EXISTS kek_test CASCADE;
DROP DEK IF EXISTS abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghjk CASCADE;
DROP DEK IF EXISTS abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk CASCADE;
CREATE KEK kek_test WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
CREATE DEK abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghjk WITH VALUES(KEK = kek_test);
DROP DEK abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghjk CASCADE;
CREATE DEK abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk WITH VALUES(KEK = kek_test);
DROP DEK abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk CASCADE;
DROP KEK kek_test CASCADE;
