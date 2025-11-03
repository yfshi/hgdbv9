DROP USER IF EXISTS credtest;
DROP EXTENSION credcheck CASCADE;
CREATE EXTENSION credcheck;
ALTER SYSTEM SET credcheck.enable TO ON;
ALTER SYSTEM SET credcheck.login_msg_display TO OFF;
SELECT PG_RELOAD_CONF();
SELECT hg_security.pg_password_history_reset();
SELECT * FROM hg_security.pg_password_history WHERE rolename = 'credtest';
SET credcheck.password_reuse_history = 2;
-- When creating user the password must be stored in the history
CREATE USER credtest WITH PASSWORD 'H8Hdre=S2';
ALTER USER credtest PASSWORD 'J8YuRe=6O';
SELECT rolename, password_hash FROM hg_security.pg_password_history WHERE rolename = 'credtest' ORDER BY password_date;
-- fail, the credential is still in the history
ALTER USER credtest PASSWORD 'J8YuRe=6O';
-- eject the first credential from the history and add a new one
ALTER USER credtest PASSWORD 'AJ8YuRe=6O0';
SELECT rolename, password_hash FROM hg_security.pg_password_history WHERE rolename = 'credtest' ORDER BY password_date ;
-- fail, the credential is still in the history
ALTER USER credtest PASSWORD 'J8YuRe=6O';
-- success, eject the second credential from the history and reuse the first one
ALTER USER credtest PASSWORD 'H8Hdre=S2';
-- success, the second credential has been removed from the history
ALTER USER credtest PASSWORD 'J8YuRe=6O';
-- Dropping the user must empty the record in history table
DROP USER credtest;
SELECT * FROM hg_security.pg_password_history WHERE rolename = 'credtest';
-- Reset the password history
SELECT hg_security.pg_password_history_reset();
