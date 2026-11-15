-- ReqID:TECC4.0 - Frontend diagnostic SQL rewrite regression.
-- Author: <wangxp@highgo.com>, 2026-09-17.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
\set ON_ERROR_STOP on
\! tecc_ktool generate --key-name kek_raw_he --key-algo sm2 --key-path "$TECC_KEK_PATH" >/dev/null
\! tecc_ktool generate --key-name kek_raw_sym --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null
CREATE KEK kek_raw_he WITH (KEY_STORE='localkms', KEY_ALGO='sm2', KEY_PATH='%KEKPATH%/kek_raw_he.priv');
CREATE KEK kek_raw_sym WITH (KEY_STORE='localkms', KEY_PATH='%KEKPATH%/kek_raw_sym.key');
CREATE DEK dek_raw_add WITH VALUES (KEK=kek_raw_he, KEY_ALGO='he-add');
CREATE DEK dek_raw_mul WITH VALUES (KEK=kek_raw_he, KEY_ALGO='he-mul');
CREATE DEK dek_raw_sym WITH VALUES (KEK=kek_raw_sym);
CREATE TABLE raw_he_test (
    id int,
    a int4_enc ENCRYPTED WITH (DEK=dek_raw_sym, HE_ADD_DEK=dek_raw_add, SUPPORT_ALGO='ope,he-add'),
    "M,)" int4_enc ENCRYPTED WITH (HE_MUL_DEK=dek_raw_mul, SUPPORT_ALGO='he-mul')
);
INSERT INTO raw_he_test VALUES (1, 1, 3), (2, 2, 4), (3, NULL, NULL);
\enable hg_tecc.mode

-- Original failure: no WHERE constants to trigger a different SQL rewrite.
SELECT tecc_decrypt_raw(a, 'he_add') FROM raw_he_test ORDER BY id;

-- Multiple raw projections mixed with ordinary columns and an encrypted
-- WHERE constant; quoted names/comments must not affect call boundaries.
SELECT id, a,
       tecc_decrypt_raw(raw_he_test.a, 'he_add') AS add_hex,
       tecc_decrypt_raw("M,)" /* , ) */, 'he_mul') AS mul_hex,
       'tecc_decrypt_raw(a, ''he_add'')' AS literal
FROM raw_he_test WHERE a >= 1 ORDER BY id;

-- NULL is preserved and the next query must not inherit raw-result flags.
SELECT tecc_decrypt_raw(a, 'he_add') AS null_hex FROM raw_he_test WHERE id=3;
SELECT id, a, "M,)" FROM raw_he_test ORDER BY id;

DROP TABLE raw_he_test;
DROP DEK dek_raw_add;
DROP DEK dek_raw_mul;
DROP DEK dek_raw_sym;
DROP KEK kek_raw_he;
DROP KEK kek_raw_sym;
