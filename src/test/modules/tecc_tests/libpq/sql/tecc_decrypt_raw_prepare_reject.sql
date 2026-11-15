-- ReqID:TECC4.0 - Gap D: fail-closed reject tecc_decrypt_raw in PREPARE ... AS.
-- Rejection tests: PREPARE ... AS with tecc_decrypt_raw must be rejected.
-- Author: <wangxp@highgo.com>, 2026-09-20.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
\set ON_ERROR_STOP on
\! tecc_ktool generate --key-name kek_raw_prep2 --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null
CREATE KEK kek_raw_prep2 WITH (KEY_STORE='localkms', KEY_PATH='%KEKPATH%/kek_raw_prep2.key');
CREATE DEK dek_raw_prep2 WITH VALUES (KEK=kek_raw_prep2);
CREATE TABLE raw_prep2 (
    id int,
    c_det text_enc ENCRYPTED WITH (DEK=dek_raw_prep2, SUPPORT_ALGO='det'),
    c_plain text
);
INSERT INTO raw_prep2 VALUES (1, 'alpha', 'hello'), (2, 'bravo', 'world');
\enable hg_tecc.mode
-- Rejection tests: each PREPARE with tecc_decrypt_raw must be rejected.
-- Note: \set ON_ERROR_STOP is OFF for this section so all three are tested.
\set ON_ERROR_STOP off
PREPARE bad1 AS SELECT tecc_decrypt_raw(c_det, 'det') FROM raw_prep2 WHERE id = $1;
PREPARE bad2 AS SELECT pg_catalog.tecc_decrypt_raw(c_det, 'det') FROM raw_prep2 WHERE id = $1;
PREPARE bad3 AS SELECT TECC_DECRYPT_RAW(c_det, 'det') FROM raw_prep2 WHERE id = $1;
\set ON_ERROR_STOP on
DROP TABLE raw_prep2;
DROP DEK dek_raw_prep2;
DROP KEK kek_raw_prep2;
