/* Begin - OA-REQ-2126 */
--
-- test CREATE USER/GRANT syntax
--
SET ivorysql.enable_ora_user_schema = on;

DROP SCHEMA IF EXISTS flyway;
DROP USER IF EXISTS flyway;

-- 1) CREATE USER ... IDENTIFIED BY ... (map to password clause)
CREATE USER flyway IDENTIFIED BY flyway;
SELECT rolname, (rolpassword IS NOT NULL) AS has_password
FROM pg_authid
WHERE rolname = 'flyway';

-- 2) GRANT RESOURCE TO ... (syntax compatibility only)
GRANT RESOURCE TO flyway;

-- 3) GRANT UNLIMITED TABLESPACE TO ... (syntax compatibility only)
GRANT UNLIMITED TABLESPACE TO flyway;

DROP USER flyway CASCADE;
/* End - OA-REQ-2126 */
