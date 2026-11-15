-- ReqID:TECC4.0 - Gap D: fail-closed reject tecc_decrypt_raw in PREPARE ... AS.
-- tecc_decrypt_raw is a frontend pseudo-function; inside PREPARE the per-statement
-- pending_expr_col_raw_algorithms state is cleared by EXECUTE, so the segment-hex
-- intent is silently lost.  Reject at PREPARE time (fail-closed, aligned with JDBC).
-- Author: <wangxp@highgo.com>, 2026-09-20.
-- Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
\set ON_ERROR_STOP on
\! tecc_ktool generate --key-name kek_raw_prep --key-algo sm4 --key-path "$TECC_KEK_PATH" >/dev/null
CREATE KEK kek_raw_prep WITH (KEY_STORE='localkms', KEY_PATH='%KEKPATH%/kek_raw_prep.key');
CREATE DEK dek_raw_prep WITH VALUES (KEK=kek_raw_prep);
CREATE TABLE raw_prep_test (
    id int,
    c_det text_enc ENCRYPTED WITH (DEK=dek_raw_prep, SUPPORT_ALGO='det'),
    c_ope int4_enc ENCRYPTED WITH (DEK=dek_raw_prep, SUPPORT_ALGO='ope'),
    c_plain text
);
INSERT INTO raw_prep_test VALUES
    (1, 'alpha', 10, 'hello'),
    (2, 'bravo', 20, 'world'),
    (3, NULL, NULL, NULL);
\enable hg_tecc.mode
-- 1) Direct tecc_decrypt_raw (not in PREPARE) must still work.
SELECT tecc_decrypt_raw(c_det, 'det') FROM raw_prep_test WHERE id = 1;
-- 2) Normal PREPARE without tecc_decrypt_raw still works.
PREPARE good1 AS SELECT id, c_det FROM raw_prep_test WHERE id = $1;
EXECUTE good1(1);
EXECUTE good1(2);
DEALLOCATE good1;
-- 3) tecc_decrypt_raw in string literal inside PREPARE should NOT be rejected.
PREPARE good2 AS SELECT id FROM raw_prep_test WHERE c_plain = 'tecc_decrypt_raw is a function';
EXECUTE good2('tecc_decrypt_raw is a function');
DEALLOCATE good2;
-- 4) tecc_decrypt_raw in comment inside PREPARE should NOT be rejected.
PREPARE good3 AS SELECT id FROM raw_prep_test WHERE id = $1; -- tecc_decrypt_raw
EXECUTE good3(1);
DEALLOCATE good3;
DROP TABLE raw_prep_test;
DROP DEK dek_raw_prep;
DROP KEK kek_raw_prep;
