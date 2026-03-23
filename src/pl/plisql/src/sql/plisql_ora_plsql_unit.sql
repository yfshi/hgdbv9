CREATE TABLE plsql_unit_test(id INT);
CREATE TABLE plsql_unit_tb(proc_name TEXT, proc_type TEXT);

SET SERVEROUTPUT ON;

-- 1. normal function
CREATE OR REPLACE FUNCTION "f_plsql_unit"
RETURN NUMBER IS
    v_func VARCHAR2(100) := $$PLSQL_UNIT;
    v_type VARCHAR2(100) := $$PLSQL_UNIT_TYPE;
BEGIN
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT = ' || v_func);
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT_TYPE = ' || v_type);
    return 1;
END;
/

SELECT * FROM "f_plsql_unit"();

DROP FUNCTION "f_plsql_unit"();

CREATE OR REPLACE FUNCTION f_plsql_unit RETURN VARCHAR2
IS
    v_result VARCHAR2(128);
BEGIN
    IF $$PLSQL_UNIT = 'f_plsql_unit' THEN
        RAISE NOTICE 'This is a debug function';
    END IF;
    v_result := 'From: ' || $$PLSQL_UNIT;
    RETURN v_result;
END;
/

SELECT * FROM f_plsql_unit();

CREATE OR REPLACE FUNCTION f_plsql_unit 
RETURN VARCHAR2 
IS
    v_sql VARCHAR2(4000);
    v_result VARCHAR2(100);
BEGIN
    v_sql := '
        BEGIN
        DBMS_OUTPUT.PUT_LINE(''Exec: ' || $$PLSQL_UNIT || ''');
        :result := ''From: ' || $$PLSQL_UNIT || ''';
        END;
    ';
    EXECUTE IMMEDIATE v_sql USING OUT v_result;
    RETURN v_result;
END;
/

SELECT * FROM f_plsql_unit();

DROP FUNCTION f_plsql_unit();

-- 2. normal procedure
CREATE OR REPLACE PROCEDURE p_plsqlunit IS
    v_unit_name VARCHAR2(100);
    v_unit_type VARCHAR2(100);
BEGIN
    v_unit_name := $$PLSQL_UNIT;
    v_unit_type := $$PLSQL_UNIT_TYPE;
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT = ' || v_unit_name);
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT_TYPE = ' || v_unit_type);
END;
/

DO $$
BEGIN
    p_plsqlunit;
END $$;

CREATE OR REPLACE PROCEDURE p_plsqlunit()
AS $$
BEGIN
    RAISE NOTICE 'test_proc: %', $$PLSQL_UNIT;
    RAISE NOTICE 'test_proc: %', $$PLSQL_UNIT;
    RAISE NOTICE 'test_proc: %', $$PLSQL_UNIT;
END $$;
/

DO $$
BEGIN
    RAISE NOTICE 'test_proc: %', $$PLSQL_UNIT;
    p_plsqlunit;
END $$;

CREATE TABLE log_plsqlunit(unit varchar2(100),time date); 
CREATE OR REPLACE PROCEDURE p_plsqlunit IS
    v_sql VARCHAR2(4000);
    v_unit VARCHAR2(100) := $$PLSQL_UNIT;
BEGIN
    v_sql := 'INSERT INTO log_plsqlunit (unit,time) ' ||
            'VALUES (:1,SYSDATE)';
    EXECUTE IMMEDIATE v_sql USING v_unit;
END;
/

CALL p_plsqlunit();
SELECT unit FROM log_plsqlunit;

DROP TABLE log_plsqlunit;

CREATE OR REPLACE PROCEDURE p_plsqlunit IS
    v_table_name VARCHAR2(100);
    v_sql VARCHAR2(4000);
BEGIN
    v_table_name := 'AUDIT_' || $$PLSQL_UNIT;

    v_sql := '
        CREATE TABLE ' || v_table_name || ' (
        id    NUMBER PRIMARY KEY,
        called_by   VARCHAR2(100) DEFAULT ''' || $$PLSQL_UNIT || '''
        )
    ';

    DBMS_OUTPUT.PUT_LINE('Create Table: ' || v_table_name);
    DBMS_OUTPUT.PUT_LINE('SQL: ' || v_sql);

    EXECUTE IMMEDIATE v_sql;
END;
/

CALL p_plsqlunit();
DROP TABLE AUDIT_p_plsqlunit;

DROP SCHEMA IF EXISTS s1;
CREATE SCHEMA s1;
CREATE OR REPLACE PROCEDURE s1.p_plsqlunit IS
    v_unit_name VARCHAR2(100);
BEGIN
    v_unit_name := $$PLSQL_UNIT;
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT = ' || v_unit_name);
END;
/

CALL s1.p_plsqlunit();

DROP PROCEDURE s1.p_plsqlunit;
DROP SCHEMA s1;

CREATE OR REPLACE PROCEDURE p_plsqlunit IS
    v_step VARCHAR2(100);
    PROCEDURE validate_input IS
    BEGIN
        DBMS_OUTPUT.PUT_LINE('From sub proc: ' || $$PLSQL_UNIT);
        DBMS_OUTPUT.PUT_LINE('plsql line: ' || $$PLSQL_LINE);
        DBMS_OUTPUT.PUT_LINE('sub proc type:: ' || $$PLSQL_UNIT_TYPE);
    END;
BEGIN
    DBMS_OUTPUT.PUT_LINE('Start: ' || $$PLSQL_UNIT);
    DBMS_OUTPUT.PUT_LINE('plsql line: ' || $$PLSQL_LINE);
    
    v_step := 'Step1: init';
    DBMS_OUTPUT.PUT_LINE($$PLSQL_UNIT || ' - ' || v_step);
    
    DBMS_OUTPUT.PUT_LINE('From top proc: ' || $$PLSQL_UNIT);
    DBMS_OUTPUT.PUT_LINE('plsql line: ' || $$PLSQL_LINE);
    DBMS_OUTPUT.PUT_LINE('top proc type:: ' || $$PLSQL_UNIT_TYPE);

    validate_input;
    
    v_step := 'Step2: exec';
    DBMS_OUTPUT.PUT_LINE($$PLSQL_UNIT || ' - ' || v_step);
    
    DBMS_OUTPUT.PUT_LINE('From top proc: ' || $$PLSQL_UNIT);
    DBMS_OUTPUT.PUT_LINE('plsql line: ' || $$PLSQL_LINE);
    DBMS_OUTPUT.PUT_LINE('top proc type:: ' || $$PLSQL_UNIT_TYPE);

    RAISE NO_DATA_FOUND;

EXCEPTION
    WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('错误发生在: ' || $$PLSQL_UNIT);
        DBMS_OUTPUT.PUT_LINE('错误步骤: ' || v_step);
        DBMS_OUTPUT.PUT_LINE('错误信息: ' || SQLERRM);
END;
/

CALL p_plsqlunit();

DROP PROCEDURE p_plsqlunit;

-- 3. anonymous block
BEGIN
    RAISE NOTICE 'proname: %', $$PLSQL_UNIT;
    RAISE NOTICE 'protype: %', $$PLSQL_UNIT_TYPE;
END;
/

DO $$
BEGIN
    RAISE NOTICE 'proname: %', $$PLSQL_UNIT;
    RAISE NOTICE 'protype: %', $$PLSQL_UNIT_TYPE;
END $$;

DECLARE
    v_unit VARCHAR2(100);
BEGIN
    v_unit := $$PLSQL_UNIT;
    DBMS_OUTPUT.PUT_LINE('$$PLSQL_UNIT = ' || 
        NVL(v_unit, 'NULL'));
    DBMS_OUTPUT.PUT_LINE('Plsql type: ' || $$PLSQL_UNIT_TYPE);
END;
/

-- 4. trigger
CREATE OR REPLACE TRIGGER plsql_unit_trig
AFTER INSERT ON plsql_unit_test
FOR EACH ROW
BEGIN
    INSERT INTO plsql_unit_tb VALUES($$PLSQL_UNIT, $$PLSQL_UNIT_TYPE);
END;
/

INSERT INTO plsql_unit_test VALUES (0);
SELECT * FROM plsql_unit_tb;

DROP TRIGGER plsql_unit_trig;

-- 5. package
CREATE OR REPLACE PACKAGE plsql_unit_pkg AS
    FUNCTION pkg_func() RETURN TEXT;
END;
/

CREATE OR REPLACE PACKAGE BODY plsql_unit_pkg IS
    FUNCTION pkg_func() RETURN TEXT IS
    BEGIN
        RAISE NOTICE 'In plsql_unit_pkg.pkg_func: %', $$PLSQL_UNIT;
        DBMS_OUTPUT.PUT_LINE('Plsql type:: ' || $$PLSQL_UNIT_TYPE);
        RETURN 'test function'::TEXT;
    END;
END;
/

SELECT plsql_unit_pkg.pkg_func();

DROP PACKAGE plsql_unit_pkg;

DROP TABLE plsql_unit_test;
DROP TABLE plsql_unit_tb;
