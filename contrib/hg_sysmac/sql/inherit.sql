SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO ON;
CREATE USER u1 with password 'Hello@123';
CREATE USER u2 with password 'Hello@123';
CREATE USER u3 with password 'Hello@123';
CREATE USER u4 with password 'Hello@123';

-- Set the table-level security label only on the parent table.

SET ROLE u1;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3;
GRANT ALL ON inh_child TO u1,u2,u3;

SET ROLE syssso;
SELECT set_table_seclabel('inh_parent'::regclass, 's2');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');

SET ROLE u1;
INSERT INTO inh_parent VALUES (1, 'alice');
INSERT INTO inh_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (2, 'bob');
INSERT INTO inh_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (3, 'tom');
INSERT INTO inh_child VALUES (33, 'ttom');

SET ROLE u1;
UPDATE inh_parent SET id = 4 WHERE id = 1;
UPDATE inh_child SET id = 44 WHERE id = 11;
SET ROLE u2;
UPDATE inh_parent SET id = 5 WHERE id = 2;
UPDATE inh_child SET id = 55 WHERE id = 22;
SET ROLE u3;
UPDATE inh_parent SET id = 6 WHERE id = 3;
UPDATE inh_child SET id = 66 WHERE id = 33;

SET ROLE u1;
DELETE FROM inh_parent WHERE id = 4;
DELETE FROM inh_child WHERE id = 44;
SET ROLE u2;
DELETE FROM inh_parent WHERE id = 5;
DELETE FROM inh_child WHERE id = 55;
SET ROLE u3;
DELETE FROM inh_parent WHERE id = 6;
DELETE FROM inh_child WHERE id = 66;

SET ROLE u1;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
SET ROLE u2;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
SET ROLE u3;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;

-- Set the table-level security label only on the child table.

SET ROLE u1;
DROP TABLE inh_parent CASCADE;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3;
GRANT ALL ON inh_child TO u1,u2,u3;

SET ROLE syssso;
SELECT set_table_seclabel('inh_child'::regclass, 's2');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');

SET ROLE u1;
INSERT INTO inh_parent VALUES (1, 'alice');
INSERT INTO inh_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (2, 'bob');
INSERT INTO inh_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (3, 'tom');
INSERT INTO inh_child VALUES (33, 'ttom');

SET ROLE u1;
UPDATE inh_parent SET id = 4 WHERE id = 1;
UPDATE inh_child SET id = 44 WHERE id = 11;
SET ROLE u2;
UPDATE inh_parent SET id = 5 WHERE id = 2;
UPDATE inh_child SET id = 55 WHERE id = 22;
SET ROLE u3;
UPDATE inh_parent SET id = 6 WHERE id = 3;
UPDATE inh_child SET id = 66 WHERE id = 33;

SET ROLE u1;
DELETE FROM inh_parent WHERE id = 4;
DELETE FROM inh_child WHERE id = 44;
SET ROLE u2;
DELETE FROM inh_parent WHERE id = 5;
DELETE FROM inh_child WHERE id = 55;
SET ROLE u3;
DELETE FROM inh_parent WHERE id = 6;
DELETE FROM inh_child WHERE id = 66;

SET ROLE u1;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
RESET ROLE;
SET ROLE u2;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
RESET ROLE;
SET ROLE u3;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;

-- Set the table-level security label on both the parent table and child tables.

SET ROLE u1;
DROP TABLE inh_parent CASCADE;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3,u4;
GRANT ALL ON inh_child TO u1,u2,u3,u4;

SET ROLE syssso;
SELECT set_table_seclabel('inh_parent'::regclass, 's2');
SELECT set_table_seclabel('inh_child'::regclass, 's3');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');
SELECT set_user_seclabel('u4'::regrole, 's4');

SET ROLE u1;
INSERT INTO inh_parent VALUES (1, 'alice');
INSERT INTO inh_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (2, 'bob');
INSERT INTO inh_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (3, 'tom');
INSERT INTO inh_child VALUES (33, 'ttom');
SET ROLE u4;
INSERT INTO inh_parent VALUES (4, 'tim');
INSERT INTO inh_child VALUES (44, 'ttim');

SET ROLE u1;
UPDATE inh_parent SET id = 5 WHERE id = 1;
UPDATE inh_child SET id = 55 WHERE id = 11;
SET ROLE u2;
UPDATE inh_parent SET id = 6 WHERE id = 2;
UPDATE inh_child SET id = 66 WHERE id = 22;
SET ROLE u3;
UPDATE inh_parent SET id = 7 WHERE id = 3;
UPDATE inh_child SET id = 77 WHERE id = 33;
SET ROLE u4;
UPDATE inh_parent SET id = 8 WHERE id = 4;
UPDATE inh_child SET id = 88 WHERE id = 44;

SET ROLE u1;
DELETE FROM inh_parent WHERE id = 5;
DELETE FROM inh_child WHERE id = 55;
SET ROLE u2;
DELETE FROM inh_parent WHERE id = 6;
DELETE FROM inh_child WHERE id = 66;
SET ROLE u3;
DELETE FROM inh_parent WHERE id = 7;
DELETE FROM inh_child WHERE id = 77;
SET ROLE u4;
DELETE FROM inh_parent WHERE id = 8;
DELETE FROM inh_child WHERE id = 88;

SET ROLE u1;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
RESET ROLE;
SET ROLE u2;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
RESET ROLE;
SET ROLE u3;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;
SET ROLE u4;
SELECT * FROM inh_parent;
SELECT * FROM inh_child;

SET ROLE syssso;
SET hg_sysmac.global_on TO OFF;
SET hg_sysmac.global_table_level TO OFF;
SET ROLE u1;
DROP TABLE inh_parent CASCADE;


-- Set the row-level security label only on the parent table.
SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3;
GRANT ALL ON inh_child TO u1,u2,u3;
SET ROLE syssso;
SELECT set_table_seclabel('inh_parent'::regclass, 's2');
SELECT apply_row_policy('inh_parent'::regclass, 'apply');
SELECT apply_row_policy('inh_parent'::regclass, 'enable');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO inh_parent VALUES (1, 'alice');
INSERT INTO inh_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (2, 'bob');
INSERT INTO inh_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (3, 'tom');
INSERT INTO inh_child VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM inh_parent;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM inh_parent;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM inh_parent;


-- Set the row-level security label only on the child table.

SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
DROP TABLE inh_parent CASCADE;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3;
GRANT ALL ON inh_child TO u1,u2,u3;
SET ROLE syssso;
SELECT set_table_seclabel('inh_child'::regclass, 's2');
SELECT apply_row_policy('inh_child'::regclass, 'apply');
SELECT apply_row_policy('inh_child'::regclass, 'enable');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO inh_parent VALUES (1, 'alice');
INSERT INTO inh_child VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (2, 'bob');
INSERT INTO inh_child VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (3, 'tom');
INSERT INTO inh_child VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM inh_child;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM inh_child;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM inh_child;


-- Set the row-level security label on both the parent table and child tables.

SET ROLE syssso;
SET hg_sysmac.global_on TO ON;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO ON;
SET ROLE u1;
DROP TABLE inh_parent CASCADE;
CREATE TABLE inh_parent (
    id int,
    tenant text
);
CREATE TABLE inh_child (
    CHECK (id < 100)
) INHERITS (inh_parent);
GRANT ALL ON inh_parent TO u1,u2,u3;
GRANT ALL ON inh_child TO u1,u2,u3;
SET ROLE syssso;
SELECT set_table_seclabel('inh_parent'::regclass, 's2');
SELECT set_table_seclabel('inh_child'::regclass, 's2');
SELECT apply_row_policy('inh_child'::regclass, 'apply');
SELECT apply_row_policy('inh_child'::regclass, 'enable');
SELECT set_user_seclabel('u1'::regrole, 's1');
SELECT set_user_seclabel('u2'::regrole, 's2');
SELECT set_user_seclabel('u3'::regrole, 's3');
SET ROLE u1;
INSERT INTO inh_child VALUES (1, 'alice');
SET ROLE u2;
INSERT INTO inh_child VALUES (2, 'bob');
SET ROLE u3;
INSERT INTO inh_child VALUES (3, 'tom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM inh_child;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM inh_child;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM inh_child;

SET ROLE syssso;
SELECT apply_row_policy('inh_parent'::regclass, 'apply');
SELECT apply_row_policy('inh_parent'::regclass, 'enable');

SET ROLE u1;
INSERT INTO inh_parent VALUES (11, 'aalice');
SET ROLE u2;
INSERT INTO inh_parent VALUES (22, 'bbob');
SET ROLE u3;
INSERT INTO inh_parent VALUES (33, 'ttom');
SET ROLE u1;
SELECT __hg_seclabel__,* FROM inh_parent;
SET ROLE u2;
SELECT __hg_seclabel__,* FROM inh_parent;
SET ROLE u3;
SELECT __hg_seclabel__,* FROM inh_parent;

SET ROLE syssso;
SET hg_sysmac.global_on TO OFF;
SET hg_sysmac.global_table_level TO OFF;
SET hg_sysmac.global_row_level TO OFF;
SET ROLE u1;
DROP TABLE inh_parent CASCADE;
RESET ROLE;
DROP USER u1,u2,u3,u4;
