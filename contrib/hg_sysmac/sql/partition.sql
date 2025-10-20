SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO ON;
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
CREATE USER u3 with password 'Hello@123';
CREATE USER u4 with password 'Hello@123';

-- Set the table-level security label only on the parent table.

SET ROLE u1;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3;
GRANT ALL ON part_child TO u1,u2,u3;

SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_parent'::regclass, 's2');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');

SET ROLE u1;
INSERT INTO part_parent VALUES (1, 'alice');
INSERT INTO part_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (2, 'bob');
INSERT INTO part_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (3, 'tom');
INSERT INTO part_child VALUES (33, 'ttom');

SET ROLE u1;
UPDATE part_parent SET id = 4 WHERE id = 1;
UPDATE part_child SET id = 44 WHERE id = 11;
SET ROLE u2;
UPDATE part_parent SET id = 5 WHERE id = 2;
UPDATE part_child SET id = 55 WHERE id = 22;
SET ROLE u3;
UPDATE part_parent SET id = 6 WHERE id = 3;
UPDATE part_child SET id = 66 WHERE id = 33;

SET ROLE u1;
DELETE FROM part_parent WHERE id = 4;
DELETE FROM part_child WHERE id = 44;
SET ROLE u2;
DELETE FROM part_parent WHERE id = 5;
DELETE FROM part_child WHERE id = 55;
SET ROLE u3;
DELETE FROM part_parent WHERE id = 6;
DELETE FROM part_child WHERE id = 66;

SET ROLE u1;
SELECT * FROM part_parent;
SELECT * FROM part_child;
SET ROLE u2;
SELECT * FROM part_parent;
SELECT * FROM part_child;
SET ROLE u3;
SELECT * FROM part_parent;
SELECT * FROM part_child;

-- Set the table-level security label only on the child table.

SET ROLE u1;
DROP TABLE part_parent CASCADE;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3;
GRANT ALL ON part_child TO u1,u2,u3;

SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_child'::regclass, 's2');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');

SET ROLE u1;
INSERT INTO part_parent VALUES (1, 'alice');
INSERT INTO part_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (2, 'bob');
INSERT INTO part_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (3, 'tom');
INSERT INTO part_child VALUES (33, 'ttom');

SET ROLE u1;
UPDATE part_parent SET id = 4 WHERE id = 1;
UPDATE part_child SET id = 44 WHERE id = 11;
SET ROLE u2;
UPDATE part_parent SET id = 5 WHERE id = 2;
UPDATE part_child SET id = 55 WHERE id = 22;
SET ROLE u3;
UPDATE part_parent SET id = 6 WHERE id = 3;
UPDATE part_child SET id = 66 WHERE id = 33;

SET ROLE u1;
DELETE FROM part_parent WHERE id = 4;
DELETE FROM part_child WHERE id = 44;
SET ROLE u2;
DELETE FROM part_parent WHERE id = 5;
DELETE FROM part_child WHERE id = 55;
SET ROLE u3;
DELETE FROM part_parent WHERE id = 6;
DELETE FROM part_child WHERE id = 66;

SET ROLE u1;
SELECT * FROM part_parent;
SELECT * FROM part_child;
RESET ROLE;
SET ROLE u2;
SELECT * FROM part_parent;
SELECT * FROM part_child;
RESET ROLE;
SET ROLE u3;
SELECT * FROM part_parent;
SELECT * FROM part_child;

-- Set the table-level security label on both the parent table and child tables.

SET ROLE u1;
DROP TABLE part_parent CASCADE;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3,u4;
GRANT ALL ON part_child TO u1,u2,u3,u4;

SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_parent'::regclass, 's2');
SELECT hg_secsch.set_table_seclabel('part_child'::regclass, 's3');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');
SELECT hg_secsch.set_user_seclabel('u4'::regrole, 's4');

SET ROLE u1;
INSERT INTO part_parent VALUES (1, 'alice');
INSERT INTO part_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (2, 'bob');
INSERT INTO part_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (3, 'tom');
INSERT INTO part_child VALUES (33, 'ttom');
SET ROLE u4;
INSERT INTO part_parent VALUES (4, 'tim');
INSERT INTO part_child VALUES (44, 'ttim');

SET ROLE u1;
UPDATE part_parent SET id = 5 WHERE id = 1;
UPDATE part_child SET id = 55 WHERE id = 11;
SET ROLE u2;
UPDATE part_parent SET id = 6 WHERE id = 2;
UPDATE part_child SET id = 66 WHERE id = 22;
SET ROLE u3;
UPDATE part_parent SET id = 7 WHERE id = 3;
UPDATE part_child SET id = 77 WHERE id = 33;
SET ROLE u4;
UPDATE part_parent SET id = 8 WHERE id = 4;
UPDATE part_child SET id = 88 WHERE id = 44;

SET ROLE u1;
DELETE FROM part_parent WHERE id = 5;
DELETE FROM part_child WHERE id = 55;
SET ROLE u2;
DELETE FROM part_parent WHERE id = 6;
DELETE FROM part_child WHERE id = 66;
SET ROLE u3;
DELETE FROM part_parent WHERE id = 7;
DELETE FROM part_child WHERE id = 77;
SET ROLE u4;
DELETE FROM part_parent WHERE id = 8;
DELETE FROM part_child WHERE id = 88;

SET ROLE u1;
SELECT * FROM part_parent;
SELECT * FROM part_child;
RESET ROLE;
SET ROLE u2;
SELECT * FROM part_parent;
SELECT * FROM part_child;
RESET ROLE;
SET ROLE u3;
SELECT * FROM part_parent;
SELECT * FROM part_child;
SET ROLE u4;
SELECT * FROM part_parent;
SELECT * FROM part_child;

SET ROLE syssso;
SET hg_sysmac.global_on TO OFF;
SET hg_sysmac.global_table_level TO OFF;
SET ROLE u1;
DROP TABLE part_parent CASCADE;

-- Set the row-level security label only on the parent table.
SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3;
GRANT ALL ON part_child TO u1,u2,u3;
SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_parent'::regclass, 's2');
SELECT hg_secsch.apply_row_policy('part_parent'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_parent'::regclass, 'enable');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO part_parent VALUES (1, 'alice');
INSERT INTO part_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (2, 'bob');
INSERT INTO part_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (3, 'tom');
INSERT INTO part_child VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM part_parent;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM part_parent;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM part_parent;


-- Set the row-level security label only on the child table.

SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
DROP TABLE part_parent CASCADE;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3;
GRANT ALL ON part_child TO u1,u2,u3;
SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_child'::regclass, 's2');
SELECT hg_secsch.apply_row_policy('part_child'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_parent'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_child'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_child'::regclass, 'enable');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO part_parent VALUES (1, 'alice');
INSERT INTO part_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (2, 'bob');
INSERT INTO part_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (3, 'tom');
INSERT INTO part_child VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM part_child;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM part_child;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM part_child;


-- Set the row-level security label on both the parent table and child tables.

SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
DROP TABLE part_parent CASCADE;
CREATE TABLE part_parent (
    id int,
    tenant text
) PARTITION BY RANGE (id);
CREATE TABLE part_child PARTITION OF part_parent FOR VALUES FROM (0) TO (100);
GRANT ALL ON part_parent TO u1,u2,u3;
GRANT ALL ON part_child TO u1,u2,u3;
SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_parent'::regclass, 's2');
SELECT hg_secsch.set_table_seclabel('part_child'::regclass, 's2');
SELECT hg_secsch.set_user_seclabel('u1'::regrole, 's1');
SELECT hg_secsch.set_user_seclabel('u2'::regrole, 's2');
SELECT hg_secsch.set_user_seclabel('u3'::regrole, 's3');
SELECT hg_secsch.apply_row_policy('part_parent'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_parent'::regclass, 'enable');
SELECT hg_secsch.apply_row_policy('part_child'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_child'::regclass, 'enable');
SET ROLE u1;
INSERT INTO part_parent VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child;

SET ROLE u1;
CREATE TABLE part_child3 (id int);
GRANT ALL ON part_child3 TO u1,u2,u3;
SET ROLE syssso;
SELECT hg_secsch.set_table_seclabel('part_child3'::regclass, 's2');
SELECT hg_secsch.apply_row_policy('part_child3'::regclass, 'apply');
SELECT hg_secsch.apply_row_policy('part_child3'::regclass, 'enable');
SET ROLE u1;
ALTER TABLE part_child3 ADD COLUMN tenant text;
INSERT INTO part_child3 VALUES (200, 'aaalice');
SET ROLE u2;
INSERT INTO part_child3 VALUES (250, 'bbbob');
SET ROLE u3;
INSERT INTO part_child3 VALUES (300, 'tttom');
SET ROLE u1;
ALTER TABLE part_parent ATTACH partition part_child3 FOR VALUES FROM (200) TO (300);
SET ROLE u1;
INSERT INTO part_parent VALUES (201, 'aalice');
SET ROLE u2;
INSERT INTO part_parent VALUES (222, 'bbob');
SET ROLE u3;
INSERT INTO part_parent VALUES (333, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child3;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child3;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM part_parent;
SELECT __hg_seclabel__,* FROM part_child3;

SET ROLE syssso;
SET hg_sysmac.global_on TO OFF;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO OFF;
SET ROLE u1;
DROP TABLE part_parent CASCADE;
RESET ROLE;
DROP USER u1,u2,u3,u4;
