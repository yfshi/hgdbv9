DROP KEK IF EXISTS "select" CASCADE;
DROP KEK IF EXISTS "table" CASCADE;
CREATE KEK "select" WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
DROP KEK "select" CASCADE;
CREATE KEK "table" WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek1.key');
DROP KEK "table" CASCADE;
