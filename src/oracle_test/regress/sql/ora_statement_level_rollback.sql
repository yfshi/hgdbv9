CREATE TABLE testslrb(col1 int);

-- 1. commit after failed
SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(3);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(3);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(3);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(3);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- procedure commit 
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  COMMIT;
  INSERT INTO testslrb VALUES(10);
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro2 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(11);
  INSERT INTO testslrb VALUES(12);
  CALL testslrb_pro();
  INSERT INTO testslrb VALUES(13);
  ROLLBACK;
  INSERT INTO testslrb VALUES(14);
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro3 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(15);
  INSERT INTO testslrb VALUES(16);
  CALL testslrb_pro2();
  INSERT INTO testslrb VALUES(17);
  ROLLBACK;
  INSERT INTO testslrb VALUES(18);
END;
/

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro2();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro2();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro3();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro3();
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- procedure eception commit 
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(10);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro2 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(11);
  INSERT INTO testslrb VALUES(12);
  CALL testslrb_pro();
  INSERT INTO testslrb VALUES(13);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro3 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(15);
  INSERT INTO testslrb VALUES(16);
  CALL testslrb_pro2();
  INSERT INTO testslrb VALUES(17);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(18);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro();  -- no commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro();  -- only top level commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro2();  -- no commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro2();  -- only top level commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro3();  -- no commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro3();  -- only top level commit
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- procedure eception no commit 
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(10);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(11);
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro2 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(12);
  INSERT INTO testslrb VALUES(13);
  CALL testslrb_pro();
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
END;
/

CREATE OR REPLACE PROCEDURE testslrb_pro3 () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(16);
  INSERT INTO testslrb VALUES(17);
  CALL testslrb_pro2();
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(18);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(19);
END;
/

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro();  -- only top exception
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro();  -- success
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro2();  -- only top exception
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro2();  -- success
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
CALL testslrb_pro3();  -- only top exception
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
CALL testslrb_pro3();  -- success
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN;
INSERT INTO testslrb VALUES(1);
CALL testslrb_pro(); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- AUTOCOMMIT
SET ivorysql.ora_statement_level_rollback = off;
\set AUTOCOMMIT off
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
\set AUTOCOMMIT on
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
\set AUTOCOMMIT off
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
COMMIT;
\set AUTOCOMMIT on
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
\set AUTOCOMMIT off
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
\set AUTOCOMMIT on
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
\set AUTOCOMMIT off
INSERT INTO testslrb VALUES(1);
INSERT INTO testslrb VALUES('a'); --failed
INSERT INTO testslrb VALUES(2);
ROLLBACK;
\set AUTOCOMMIT on
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- 2. commit after exception in plsql
SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(3);
    INSERT INTO testslrb VALUES(4);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(5);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(6);
        COMMIT;
        INSERT INTO testslrb VALUES(7);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(3);
    INSERT INTO testslrb VALUES(4);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(5);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(6);
        COMMIT;
        INSERT INTO testslrb VALUES(7);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(3);
    INSERT INTO testslrb VALUES(4);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(5);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(6);
        ROLLBACK;
        INSERT INTO testslrb VALUES(7);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(3);
    INSERT INTO testslrb VALUES(4);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(5);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(6);
        ROLLBACK;
        INSERT INTO testslrb VALUES(7);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
DECLARE
    var1 INTEGER := 0;
BEGIN
    var1 := 11;
    var1 := 'a'; --failed
    var1 := 22;
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
        RAISE NOTICE 'var = %', var1;
END;
/

SET ivorysql.ora_statement_level_rollback = on;
DECLARE
    var1 INTEGER := 0;
BEGIN
    var1 := 11;
    var1 := 'a'; --failed
    var1 := 22;
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
        RAISE NOTICE 'var = %', var1;
END;
/

SET ivorysql.ora_statement_level_rollback = off;
DECLARE
    var1 INTEGER := 0;
BEGIN
    var1 := 11;
    var1 := 'a'; --failed
    var1 := 22;
EXCEPTION
    WHEN OTHERS THEN
        ROLLBACK;
        RAISE NOTICE 'var = %', var1;
END;
/

SET ivorysql.ora_statement_level_rollback = on;
DECLARE
    var1 INTEGER := 0;
BEGIN
    var1 := 11;
    var1 := 'a'; --failed
    var1 := 22;
EXCEPTION
    WHEN OTHERS THEN
        ROLLBACK;
        RAISE NOTICE 'var = %', var1;
END;
/

-- procedure commit
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  COMMIT;
  INSERT INTO testslrb VALUES(10);
END;
/

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();  --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();  --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();  --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();  --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- procedure no commit 
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
END;
/

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a'); --failed
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- procedure eception commit 
CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(10);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        ROLLBACK;
        INSERT INTO testslrb VALUES(16);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro();
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

--exception failed
SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES('b');
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        COMMIT;
        INSERT INTO testslrb VALUES('b');
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

CREATE OR REPLACE PROCEDURE testslrb_pro () 
AUTHID CURRENT_USER
AS
BEGIN
  INSERT INTO testslrb VALUES(8);
  INSERT INTO testslrb VALUES(9);
  INSERT INTO testslrb VALUES('a');
  INSERT INTO testslrb VALUES(10);
EXCEPTION
    WHEN OTHERS THEN
        INSERT INTO testslrb VALUES(15);
        INSERT INTO testslrb VALUES('b');
        COMMIT;
END;
/

SET ivorysql.ora_statement_level_rollback = off;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_statement_level_rollback = on;
BEGIN
    INSERT INTO testslrb VALUES(11);
    INSERT INTO testslrb VALUES(12);
    CALL testslrb_pro(); --failed
    INSERT INTO testslrb VALUES(13);
    INSERT INTO testslrb VALUES('a');
    INSERT INTO testslrb VALUES(14);
EXCEPTION
    WHEN OTHERS THEN
        COMMIT;
END;
/
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

-- 3. commit before disconnect
SET ivorysql.ora_commit_before_disconnect = off;
BEGIN;
INSERT INTO testslrb VALUES(17);
INSERT INTO testslrb VALUES(18);
\c regression -
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_commit_before_disconnect = on;
BEGIN;
INSERT INTO testslrb VALUES(17);
INSERT INTO testslrb VALUES(18);
\c regression -
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

\set AUTOCOMMIT off
SET ivorysql.ora_commit_before_disconnect = off;
INSERT INTO testslrb VALUES(17);
INSERT INTO testslrb VALUES(18);
\c regression -
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

SET ivorysql.ora_commit_before_disconnect = on;
INSERT INTO testslrb VALUES(17);
INSERT INTO testslrb VALUES(18);
\c regression -
SELECT * FROM testslrb ORDER BY 1;
TRUNCATE testslrb;

DROP TABLE testslrb;
DROP PROCEDURE testslrb_pro();
COMMIT;

\set AUTOCOMMIT on
RESET ivorysql.ora_statement_level_rollback;
RESET ivorysql.ora_commit_before_disconnect;