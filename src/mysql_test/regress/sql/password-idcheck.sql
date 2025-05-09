--
-- Tests for password types
--

-- Tests for GUC password_encryption
SET password_encryption = 'novalue'; -- error
SET password_encryption = true; -- error
SET password_encryption = 'sm3'; -- ok, similar to sm3
SET password_encryption = 'scram-sm3'; -- ok, similar to scram-sm3

-- New roles for sm3 and scram-sm3
SET password_encryption = 'sm3';
CREATE ROLE regress_sm3_1 PASSWORD 'role_sm3_pwd1';
CREATE ROLE regress_sm3_2 PASSWORD 'role_sm3_pwd2';
SET password_encryption = 'scram-sm3';
CREATE ROLE regress_sm3_3 PASSWORD 'role_sm3_pwd2';
CREATE ROLE regress_sm3_4 PASSWORD NULL;

-- check list of created entries
--
-- For scram-sm3, it will be similar:
-- SCRAM-SM3$4096:<salt>:<storedkey>:<serverkey>
--
-- Since the salt is random, the exact value stored will be different on every test
-- run. Use a regular expression to mask the changing parts.
SELECT rolname, regexp_replace(rolpassword, '(SCRAM-SM3)\$(\d+):([a-zA-Z0-9+/=]+)\$([a-zA-Z0-9+=/]+):([a-zA-Z0-9+/=]+)', '\1$\2:<salt>$<storedkey>:<serverkey>') as rolpassword_masked
    FROM pg_authid
    WHERE rolname LIKE 'regress_sm3%'
    ORDER BY rolname, rolpassword;

-- Rename a role
ALTER ROLE regress_sm3_2 RENAME TO regress_sm3_2_new;
-- sm3 entry should have been removed
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_sm3_2_new'
    ORDER BY rolname, rolpassword;
ALTER ROLE regress_sm3_2_new RENAME TO regress_sm3_2;

-- Change passwords with ALTER USER. With plaintext or already-encrypted
-- passwords.
SET password_encryption = 'sm3';

-- encrypt with SM3
ALTER ROLE regress_sm3_2 PASSWORD 'foo';
-- already encrypted, use as they are
ALTER ROLE regress_sm3_1 PASSWORD 'sm316df05c2ba9113f5f64ce2205877e7252ea4baa7564202864cae151ce1d6a703';
ALTER ROLE regress_sm3_3 PASSWORD 'SCRAM-SM3$4096:6G/ZfWVhaV5/sxcReKlQHA==$ylMSEtUVD4NxtNO9qiWxGO3feCMofGlXQCpePAOq/w8=:u9tblPENQ7q5vBcDfRwULREi0mtbAVAj0dIuosWpgww=';

SET password_encryption = 'scram-sm3';
-- create SCRAM secret
ALTER ROLE  regress_sm3_4 PASSWORD 'foo';
-- already encrypted with SM3, use as it is
CREATE ROLE regress_sm3_5 PASSWORD 'sm3bdc36be70df917e5a457da60884b784a134972564af5e86d4f0296bdd499d5e7';

-- This looks like a valid SCRAM-SM3 secret, but it is not
-- so it should be hashed with SCRAM-SM3.
CREATE ROLE regress_sm3_6 PASSWORD 'SCRAM-SM3$1234';
-- These may look like valid SM3 secrets, but they are not, so they
-- should be hashed with SCRAM-SM3.
-- trailing garbage at the end
CREATE ROLE regress_sm3_7 PASSWORD 'sm3012345678901234567890123456789zz012345678901234567890123456789zz';
-- invalid length
CREATE ROLE regress_sm3_8 PASSWORD 'sm3012345678901234567890123456789zz01234567890123456789012345678901zz';

SELECT rolname, regexp_replace(rolpassword, '(SCRAM-SM3)\$(\d+):([a-zA-Z0-9+/=]+)\$([a-zA-Z0-9+=/]+):([a-zA-Z0-9+/=]+)', '\1$\2:<salt>$<storedkey>:<serverkey>') as rolpassword_masked
    FROM pg_authid
    WHERE rolname LIKE 'regress_sm3_%'
    ORDER BY rolname, rolpassword;

-- An empty password is not allowed, in any form
CREATE ROLE regress_sm3_empty PASSWORD '';
ALTER ROLE regress_sm3_empty PASSWORD 'sm3fd1bba8db9d7f2724e79db19ee683113644616a1c26e64c29d5108b5b4872980';
ALTER ROLE regress_sm3_empty PASSWORD 'SCRAM-SM3$4096:9hjiTokMsf8DjOItpbqfzA==$Ou+ZY7VuZCtFl1qo5fTfIvbGWf1H6No6RQnAGaCnryU=:IVdktooOBuUEo8/3rn5mSBNUyR7lZqFEj7TK2Z0bmbA=';
SELECT rolpassword FROM pg_authid WHERE rolname='regress_sm3_empty';

-- Test with invalid stored and server keys.
--
-- The first is valid, to act as a control. The others have too long
-- stored/server keys. They will be re-hashed.
CREATE ROLE regress_passwd_sm3_len0 PASSWORD 'SCRAM-SM3$4096:KZRjfXEKP+tg2Wl2iZnFBw==$U/Dlc5pxvodTLimsZ1GXBtfi4myfyQ8FfTyrtA0T/Mc=:cu6p5WuW4ZgTlYjpCYNYfi4IA001wcnuxWEk+sWU5OA=';
CREATE ROLE regress_passwd_sm3_len1 PASSWORD 'SCRAM-SM3$4096:KZRjfXEKP+tg2Wl2iZnFBw==$U/Dlc5pxvodTLimsZ1GXBtfi4myfyQ8FfTyrtA0T/McAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=:cu6p5WuW4ZgTlYjpCYNYfi4IA001wcnuxWEk+sWU5OA=';
CREATE ROLE regress_passwd_sm3_len2 PASSWORD 'SCRAM-SM3$4096:KZRjfXEKP+tg2Wl2iZnFBw==$U/Dlc5pxvodTLimsZ1GXBtfi4myfyQ8FfTyrtA0T/Mc=:cu6p5WuW4ZgTlYjpCYNYfi4IA001wcnuxWEk+sWU5OAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=';

-- Check that the invalid secrets were re-hashed. A re-hashed secret
-- should not contain the original salt.
SELECT rolname, rolpassword not like '%KZRjfXEKP+tg2Wl2iZnFBw==%' as is_rolpassword_rehashed
    FROM pg_authid
    WHERE rolname LIKE 'regress_passwd_sm3_len%'
    ORDER BY rolname;

DROP ROLE regress_sm3_1;
DROP ROLE regress_sm3_2;
DROP ROLE regress_sm3_3;
DROP ROLE regress_sm3_4;
DROP ROLE regress_sm3_5;
DROP ROLE regress_sm3_6;
DROP ROLE regress_sm3_7;
DROP ROLE regress_sm3_8;
DROP ROLE regress_sm3_empty;
DROP ROLE regress_passwd_sm3_len0;
DROP ROLE regress_passwd_sm3_len1;
DROP ROLE regress_passwd_sm3_len2;
	
-- all entries should have been removed
SELECT rolname, rolpassword
    FROM pg_authid
    WHERE rolname LIKE 'regress_sm3_%'
    ORDER BY rolname, rolpassword;
