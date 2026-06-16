ALTER SYSTEM SET hg_sysmac.global_on TO ON;

SELECT pg_reload_conf();
SELECT pg_sleep(1);

-- check security label format
SELECT 's0:c0.c2,c4.c6,c9'::hg_security.seclabel;
SELECT 's16'::hg_security.seclabel; -- error
SELECT 's0:c1024'::hg_security.seclabel; -- error
SELECT 's0:c0!'::hg_security.seclabel; -- error

-- set the security label to user
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
SELECT hg_security.set_user_seclabel('u1'::regrole, 's0');
SELECT provider,label FROM pg_shseclabel WHERE objoid = 'u1'::regrole;

-- modify user security label
SELECT hg_security.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_security.get_user_seclabel('u1'::regrole);
SELECT hg_security.get_user_seclabel('u2'::regrole);
DROP USER u2;

-- set the security label to table
SET ROLE u1;
CREATE TABLE tb1(id int);
CREATE TABLE tb2(id int);
RESET ROLE;
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's2');
SELECT hg_security.get_table_seclabel('tb1'::regclass);
SELECT hg_security.get_table_seclabel('tb2'::regclass);
DROP TABLE tb2;

-- when dropping a table,its security label should also be removed.
SET ROLE u1;
DROP TABLE tb1;
RESET ROLE;
SELECT * FROM pg_seclabel;

-- when dropping a user,its security label should also be removed.
DROP USER u1;
SELECT * FROM pg_shseclabel;

-- compare two security labels
SELECT hg_security.compare_seclabel('s1:c0.c1023', 's0:c0.c1023');
SELECT hg_security.compare_seclabel('s0:c0.c1023', 's0:c0.c102');
SELECT hg_security.compare_seclabel('s3:c0.c1023', 's3:c0.c1023');
SELECT hg_security.compare_seclabel('s1:c0.c1023', 's3:c0.c102');

-- bug #z6431
CREATE USER sec_user WITH PASSWORD 'sec@123';
SET ROLE sec_user;
CREATE TABLE sec_table(id int, data varchar(50));
RESET ROLE;
GRANT EXECUTE ON function hg_security.get_user_seclabel(oid) TO sec_user;
GRANT EXECUTE ON function hg_security.get_table_seclabel(oid) TO sec_user;
GRANT EXECUTE ON function hg_security.get_column_seclabel(oid, int) TO sec_user;
GRANT SELECT ON pg_shseclabel TO sec_user;
GRANT SELECT ON pg_seclabel TO sec_user;
DROP TABLE sec_table;
DROP USER sec_user;

-- table-level security test

CREATE USER u1 with password 'Hello@123';
SET ROLE u1;
CREATE TABLE tb1(id int);
CREATE VIEW v_tb1 AS SELECT * FROM tb1;
INSERT INTO tb1 VALUES (1),(2),(3);
RESET ROLE;

-- user-controlled table security label.
ALTER SYSTEM SET hg_sysmac.global_table_level TO ON;
ALTER SYSTEM SET hg_sysmac.global_on TO ON;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's0');
SELECT hg_security.set_table_seclabel('v_tb1'::regclass, 's0');
SET ROLE u1;
SELECT * FROM tb1;
SELECT * FROM v_tb1;
UPDATE tb1 SET id = 4 WHERE id = 1;
DELETE FROM tb1 WHERE id = 1;
INSERT INTO tb1 VALUES (4);

-- user equals the table’s security label.
RESET ROLE;
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's0');
SELECT hg_security.set_user_seclabel('u1'::regrole, 's0');
SET ROLE u1;
INSERT INTO tb1 VALUES (4);
DELETE FROM tb1 WHERE id = 1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT * FROM tb1;
SELECT * FROM v_tb1;

-- table-controlled user security label.
RESET ROLE;
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's1');
SELECT hg_security.set_user_seclabel('u1'::regrole, 's0');
SET ROLE u1;
INSERT INTO tb1 VALUES (1);
SELECT * FROM tb1;
SELECT * FROM v_tb1;
UPDATE tb1 SET id = 5 WHERE id = 2;
UPDATE v_tb1 SET id = 5 WHERE id = 2;
DELETE FROM tb1 WHERE id = 1;
DELETE FROM v_tb1 WHERE id = 1;

-- user-controlled the column’s security label.
CREATE TABLE tb2(id int);
INSERT INTO tb2 VALUES (1),(2),(3);
RESET ROLE;
ALTER SYSTEM SET hg_sysmac.global_table_level TO ON;
ALTER SYSTEM SET hg_sysmac.global_on TO ON;
ALTER SYSTEM SET hg_sysmac.global_column_level TO ON;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_security.set_table_seclabel('tb2'::regclass, 's1');
SELECT hg_security.set_column_seclabel('tb2'::regclass, 1, 's0');
SET ROLE u1;
SELECT id FROM tb2;
INSERT INTO tb2 VALUES (4);
UPDATE tb2 SET id = 6 WHERE id = 3;
SELECT id FROM tb2;

-- user equals the column’s security label.
RESET ROLE;
SELECT hg_security.set_column_seclabel('tb2'::regclass, 1, 's1');
SET ROLE u1;
INSERT INTO tb2 VALUES (4);
UPDATE tb2 SET id = 5 WHERE id = 1;
DELETE FROM tb2 WHERE id = 2;
SELECT id FROM tb2;

-- column-controlled user security label.
RESET ROLE;
SELECT hg_security.set_column_seclabel('tb2'::regclass, 1, 's2');
SET ROLE u1;
INSERT INTO tb2 VALUES (6);
UPDATE tb2 SET id = 5 WHERE id = 1;
DELETE FROM tb2 WHERE id = 2;
SELECT id FROM tb2;
DROP TABLE tb1,tb2 CASCADE;
RESET ROLE;
DROP USER u1;

-- row-level security testing.
ALTER SYSTEM SET hg_sysmac.global_row_level TO ON;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
CREATE USER u3 with password 'Hello@123';
SET ROLE u1;
CREATE TABLE tb1(id int);
GRANT ALL ON tb1 TO u1,u2,u3;
RESET ROLE;
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's3');
SELECT hg_security.apply_row_policy('tb1'::regclass, 'apply');
SELECT hg_security.apply_row_policy('tb1'::regclass, 'enable');
ALTER SYSTEM SET hg_sysmac.global_table_level TO OFF;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_security.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_security.set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO tb1 VALUES (1);
RESET ROLE;
SET ROLE u2;
INSERT INTO tb1 VALUES (2);
RESET ROLE;
SET ROLE u3;
INSERT INTO tb1 VALUES (3);
RESET ROLE;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM tb1;
RESET ROLE;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM tb1;
RESET ROLE;
SET ROLE u1;
SELECT __hg_seclabel__,* FROM tb1;
SET ROLE u3;
UPDATE tb1 SET id = 4 WHERE id = 1;
SELECT __hg_seclabel__,* FROM tb1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT __hg_seclabel__,* FROM tb1;
DELETE FROM tb1 WHERE id = 4;
SELECT __hg_seclabel__,* FROM tb1;

RESET ROLE;
ALTER SYSTEM SET hg_sysmac.global_row_level TO OFF;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SET ROLE u1;
TRUNCATE TABLE tb1;
INSERT INTO tb1 VALUES (1);
RESET ROLE;
SET ROLE u2;
INSERT INTO tb1 VALUES (2);
RESET ROLE;
SET ROLE u3;
INSERT INTO tb1 VALUES (3);
RESET ROLE;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM tb1;
RESET ROLE;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM tb1;
RESET ROLE;
SET ROLE u1;
SELECT __hg_seclabel__,* FROM tb1;
SET ROLE u1;
UPDATE tb1 SET id = 4 WHERE id = 1;
SELECT __hg_seclabel__,* FROM tb1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT __hg_seclabel__,* FROM tb1;
DELETE FROM tb1 WHERE id = 4;
SELECT __hg_seclabel__,* FROM tb1;
DROP TABLE tb1;
RESET ROLE;
DROP USER u1,u2,u3;

-- Begin #BUG7896 --
--
-- When restricted_dba is enabled, syssso should
-- also be able to set seclabel for users.
--
CREATE USER usr1 WITH PASSWORD 'usr1@123';
CREATE USER usr2 WITH PASSWORD 'usr2@123';

SELECT hg_security.set_user_seclabel('usr1'::regrole, 's0');
ALTER SYSTEM SET se_feature.mode TO ON;
ALTER SYSTEM SET se_feature.restricted_dba TO ON;
SELECT pg_reload_conf();
SELECT pg_sleep(1);

SELECT current_user AS boot_user \gset
\c - syssso
SHOW se_feature.mode;
SHOW se_feature.restricted_dba;
SELECT hg_security.set_user_seclabel('usr1'::regrole, 's1');

-- Begin #BUG8275 --
grant syssso_role to usr2;
\c - usr2
SELECT hg_security.set_user_seclabel('usr1'::regrole, 's2');
SELECT hg_security.get_user_seclabel('usr1'::regrole);
SELECT hg_security.compare_seclabel('s1:c0.c1023', 's0:c0.c1023');

ALTER SYSTEM SET hg_sysmac.global_row_level TO ON;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
\c - :boot_user
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
CREATE USER u3 with password 'Hello@123';
SET ROLE u1;
CREATE TABLE tb1(id int);
GRANT ALL ON tb1 TO u1,u2,u3;
\c - usr2;
SELECT hg_security.set_table_seclabel('tb1'::regclass, 's3');
SELECT hg_security.apply_row_policy('tb1'::regclass, 'apply');
SELECT hg_security.apply_row_policy('tb1'::regclass, 'enable');
ALTER SYSTEM SET hg_sysmac.global_table_level TO OFF;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
SELECT hg_security.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_security.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_security.set_user_seclabel('u3'::regrole, 's3');
\c - u1;
INSERT INTO tb1 VALUES (1);
\c - u2;
INSERT INTO tb1 VALUES (2);
\c - u3;
INSERT INTO tb1 VALUES (3);
\c - u3;
SELECT __hg_seclabel__,* FROM tb1;
\c - u2;
SELECT __hg_seclabel__,* FROM tb1;
\c - u1;
SELECT __hg_seclabel__,* FROM tb1;
\c - u3;
UPDATE tb1 SET id = 4 WHERE id = 1;
SELECT __hg_seclabel__,* FROM tb1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT __hg_seclabel__,* FROM tb1;
DELETE FROM tb1 WHERE id = 4;
SELECT __hg_seclabel__,* FROM tb1;

\c - usr2
ALTER SYSTEM SET hg_sysmac.global_row_level TO OFF;
SELECT pg_reload_conf();
SELECT pg_sleep(1);
\c - u1;
TRUNCATE TABLE tb1;
INSERT INTO tb1 VALUES (1);
RESET ROLE;
\c - u2;
INSERT INTO tb1 VALUES (2);
RESET ROLE;
\c - u3;
INSERT INTO tb1 VALUES (3);
\c - u3;
SELECT __hg_seclabel__,* FROM tb1;
\c - u2;
SELECT __hg_seclabel__,* FROM tb1;
\c - u1;
SELECT __hg_seclabel__,* FROM tb1;
\c - u1;
UPDATE tb1 SET id = 4 WHERE id = 1;
SELECT __hg_seclabel__,* FROM tb1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT __hg_seclabel__,* FROM tb1;
DELETE FROM tb1 WHERE id = 4;
SELECT __hg_seclabel__,* FROM tb1;
DROP TABLE tb1;
\c - :boot_user
DROP USER u1,u2,u3;

-- End #BUG8275 --

\c - syssso
ALTER SYSTEM SET se_feature.mode TO OFF;
ALTER SYSTEM SET se_feature.restricted_dba TO OFF;
SELECT pg_reload_conf();
SELECT pg_sleep(1);

\c - :boot_user
DROP USER usr1;
DROP USER usr2;
-- End #BUG7896 --
