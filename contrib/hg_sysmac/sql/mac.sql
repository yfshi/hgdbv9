SET hg_sysmac.global_on TO ON;

-- check security label format
SELECT 's0:c0.c2,c4.c6,c9'::seclabel;
SELECT 's16'::seclabel; -- error
SELECT 's0:c1024'::seclabel; -- error
SELECT 's0:c0!'::seclabel; -- error

-- set the security label to user
CREATE USER u1 with password 'Hello@123';
SELECT set_user_seclabel('u1'::regrole, 's0');
SELECT provider,label FROM pg_shseclabel WHERE objoid = 'u1'::regrole;

-- modify user security label
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT get_user_seclabel('u1'::regrole);

-- set the security label to table
SET ROLE u1;
CREATE TABLE tb1(id int);
RESET ROLE;
SELECT set_table_seclabel('tb1'::regclass, 's2');
SELECT get_table_seclabel('tb1'::regclass);

-- when dropping a table,its security label should also be removed.
SET ROLE u1;
DROP TABLE tb1;
RESET ROLE;
SELECT * FROM pg_seclabel;

-- when dropping a user,its security label should also be removed.
DROP USER u1;
SELECT * FROM pg_shseclabel;

-- compare two security labels
SELECT compare_seclabel('s1:c0.c1023', 's0:c0.c1023');
SELECT compare_seclabel('s0:c0.c1023', 's0:c0.c102');
SELECT compare_seclabel('s3:c0.c1023', 's3:c0.c1023');
SELECT compare_seclabel('s1:c0.c1023', 's3:c0.c102');

-- table-level security test

CREATE USER u1 with password 'Hello@123';
SET ROLE u1;
CREATE TABLE tb1(id int);
CREATE VIEW v_tb1 AS SELECT * FROM tb1;
INSERT INTO tb1 VALUES (1),(2),(3);
RESET ROLE;

-- user-controlled table security label.
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_table_seclabel('tb1'::regclass, 's0');
SELECT set_table_seclabel('v_tb1'::regclass, 's0');
SET hg_sysmac.global_table_level TO ON;
SET hg_sysmac.global_on TO ON;
SET ROLE u1;
SELECT * FROM tb1;
SELECT * FROM v_tb1;
UPDATE tb1 SET id = 4 WHERE id = 1;
DELETE FROM tb1 WHERE id = 1;
INSERT INTO tb1 VALUES (4);

-- user equals the table’s security label.
RESET ROLE;
SELECT set_table_seclabel('tb1'::regclass, 's0');
SELECT set_user_seclabel('u1'::regrole, 's0');
SET ROLE u1;
INSERT INTO tb1 VALUES (4);
DELETE FROM tb1 WHERE id = 1;
UPDATE tb1 SET id = 5 WHERE id = 2;
SELECT * FROM tb1;
SELECT * FROM v_tb1;

-- table-controlled user security label.
RESET ROLE;
SELECT set_table_seclabel('tb1'::regclass, 's1');
SELECT set_user_seclabel('u1'::regrole, 's0');
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
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_table_seclabel('tb2'::regclass, 's1');
SELECT set_column_seclabel('tb2'::regclass, 1, 's0');
SET hg_sysmac.global_table_level TO ON;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_column_level TO ON;
SET ROLE u1;
SELECT id FROM tb2;
INSERT INTO tb2 VALUES (4);
UPDATE tb2 SET id = 6 WHERE id = 3;
SELECT id FROM tb2;

-- user equals the column’s security label.
RESET ROLE;
SELECT set_column_seclabel('tb2'::regclass, 1, 's1');
SET ROLE u1;
INSERT INTO tb2 VALUES (4);
UPDATE tb2 SET id = 5 WHERE id = 1;
DELETE FROM tb2 WHERE id = 2;
SELECT id FROM tb2;

-- column-controlled user security label.
RESET ROLE;
SELECT set_column_seclabel('tb2'::regclass, 1, 's2');
SET ROLE u1;
INSERT INTO tb2 VALUES (6);
UPDATE tb2 SET id = 5 WHERE id = 1;
DELETE FROM tb2 WHERE id = 2;
SELECT id FROM tb2;
DROP TABLE tb1,tb2 CASCADE;
RESET ROLE;
DROP USER u1;

-- row-level security testing.
SET hg_sysmac.global_row_level TO ON;
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
CREATE USER u3 with password 'Hello@123';
SET ROLE u1;
CREATE TABLE tb1(id int);
GRANT ALL ON tb1 TO u1,u2,u3;
SET ROLE syssso;
SELECT set_table_seclabel('tb1'::regclass, 's3');
SELECT apply_row_policy('tb1'::regclass, 'apply');
SELECT apply_row_policy('tb1'::regclass, 'enable');
SET hg_sysmac.global_table_level TO OFF;
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');
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
SET hg_sysmac.global_row_level TO OFF;
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