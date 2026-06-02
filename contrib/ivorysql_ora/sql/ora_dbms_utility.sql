-- ***********************************************************************************************
-- This case includes queries OID of FUNCTION/PROCEDURE. If this 
-- case FAILED due to newly created or previously deleted object before this case is executed,
-- the expected output of the case should be adjuested accordingly.
-- ***********************************************************************************************
SET SERVEROUTPUT ON;
SET ivorysql.enable_ora_default_collation TO OFF;
SET ivorysql.identifier_case_switch TO interchange;

-- ==============================================================
-- Test Suite for PROCEDURE CANONICALIZE(
--      name IN VARCHAR2, 
--      canon_name OUT VARCHAR2, 
--      canon_len IN BINARY_INTEGER)
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    test_record RECORD;
    result VARCHAR2(128);
    error_occurred BOOLEAN;
    errmsg TEXT;
BEGIN
    -- create test table
    CREATE TEMP TABLE test_cases (
        input_text VARCHAR2(128),
        expected VARCHAR2(128),
        should_error BOOLEAN,
        description TEXT,
        max_len BINARY_INTEGER
    );

    -- normal case (should_error = false)
    INSERT INTO test_cases VALUES
        ('', '', FALSE, 'Empty string input', 30),
        ('emp', 'EMP', FALSE, 'Unquoted simple identifier', 30),
        ('Emp', 'EMP', FALSE, 'Mixed case unquoted → uppercase', 30),
        ('MY_TABLE', 'MY_TABLE', FALSE, 'Already uppercase', 30),
        ('"Emp"', 'Emp', FALSE, 'Quoted identifier preserves case', 30),
        ('"my table"', 'my table', FALSE, 'Quoted with space allowed', 30),
        ('"cott.emp, extra"', 'cott.emp, extra', FALSE, 'Quoted with dot, comma, space', 30),
        ('hr."Emp".Name', '"HR"."Emp"."NAME"', FALSE, 'Mixed quoted/unquoted qualified name', 30),
        ('"hr"."Emp"."Name"', '"hr"."Emp"."Name"', FALSE, 'Fully quoted qualified name', 30),
        ('SCOTT."Emp".ID', '"SCOTT"."Emp"."ID"', FALSE, 'Uppercase unquoted + quoted', 30),
        ('"hr.Emp.Name"', 'hr.Emp.Name', FALSE, 'Whole-quoted with dots (not split)', 30),
        ('"a,b.c d"', 'a,b.c d', FALSE, 'Whole-quoted with comma, dot, space', 30),
        ('VeryLongTableName', 'VERYLONGTABLENAM', FALSE, 'Truncate to 16 chars (max_len=16)', 16),
        ('"Short"', 'Short', FALSE, 'Quoted short name with max_len=5', 5),
        ('a.b.c', '"A"."B"."C', FALSE, 'Qualified name truncated to 10 chars (hard truncate)', 10),
        ('   ', '   ', FALSE, 'Unquoted spaces → NOT trimmed (Oracle returns as-is)', 30),
        ('"   "', '   ', FALSE, 'Quoted spaces preserved', 30),
        ('"   .table "', '   .table ', FALSE, 'Leading space', 30),
        ('"table.  "', 'table.  ', FALSE, 'Trailing space', 30),
        ('"_table"', '_table', FALSE, 'Begin by dots and underscore', 30),
        ('"123table"', '123table', FALSE, 'Begin by dots and number', 30),

        -- expection case (should_error = true)
        ('""', NULL, TRUE, 'Empty quoted identifier', 30),
        ('my table', NULL, TRUE, 'Unquoted identifier cannot contain spaces', 30),
        ('emp,dept', NULL, TRUE, 'Unquoted identifier cannot contain comma', 30),
        ('hr."Emp",Name', NULL, TRUE, 'Comma in unquoted part of qualified name', 30),
        ('a..b', NULL, TRUE, 'Double dot (empty component)', 30),
        ('.emp', NULL, TRUE, 'Leading dot', 30),
        ('emp.', NULL, TRUE, 'Trailing dot', 30),
        ('   .table ', '', TRUE, 'Leading space', 30),
        ('table.  ', '', TRUE, 'Trailing space', 30),
        ('123table', '', TRUE, 'Begin by number', 30),
        ('_table', '', TRUE, 'Begin by underscore', 30),

        -- Special: NULL input → returns NULL (not error)
        (NULL, NULL, FALSE, 'NULL input → returns NULL (Oracle behavior)', 30)
    ;

    FOR test_record IN SELECT * FROM test_cases ORDER BY ctid LOOP
        BEGIN
            dbms_utility.canonicalize(name => test_record.input_text, canon_name => result, canon_len => test_record.max_len);
            error_occurred := FALSE;
            errmsg := NULL;
            EXCEPTION WHEN OTHERS THEN
                error_occurred := TRUE;
                errmsg := SQLERRM;
                result := NULL;
        END;

        IF test_record.should_error THEN
            IF error_occurred THEN
                RAISE NOTICE 'PASS: %', test_record.description;
            ELSE
                RAISE NOTICE 'FAIL: % - Expected error but got [%]', test_record.description, result;
            END IF;
        ELSE
            IF (result IS NULL AND test_record.expected IS NULL)
                OR (result IS NOT NULL AND test_record.expected IS NOT NULL AND result = test_record.expected) THEN
                RAISE NOTICE 'PASS: %', test_record.description;
            ELSE
                RAISE NOTICE 'FAIL: % - Expected [%], got [%]', 
                test_record.description,
                COALESCE('''' || test_record.expected || '''', '{NULL}'),
                COALESCE('''' || result || '''', '{NULL}');
            END IF;
            END IF;
    END LOOP;

    DROP TABLE test_cases;
    RAISE NOTICE '=== PostgreSQL test suite completed ===';
END $$;
/

-- ==============================================================
-- Test for PROCEDURE TABLE_TO_COMMA() and COMMA_TO_TABLE()
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DECLARE
    l_names   text[4];
    l_list    VARCHAR2(32767);
    l_count   BINARY_INTEGER;
BEGIN
    l_names[1] := 'SCOTT';
    l_names[2] := 'HR';
    l_names[3] := 'OE';
    l_names[4] := NULL; 

    DBMS_UTILITY.TABLE_TO_COMMA(
        tab    => l_names,
        tablen => l_count,
        list   => l_list
    );

    DBMS_OUTPUT.PUT_LINE('Count: ' || l_count);
    DBMS_OUTPUT.PUT_LINE('List : ' || l_list);
END;
/

DECLARE
    l_name VARCHAR2 := ' "Emp, Table", DEPT , "My_Schema".    "My_Table" ';
    l_tablen BINARY_INTEGER;
    l_tab VARCHAR2[];
BEGIN
    DBMS_UTILITY.COMMA_TO_TABLE(l_name, l_tablen, l_tab);
    FOR i IN 1..l_tablen LOOP
        RAISE NOTICE '[%]: [%]', i, l_tab[i];
    END LOOP;
END;
/

DECLARE
    l_name VARCHAR2 := ' Emp,    Table, DEPT , "My_Schema",    "My_Table" ';
    l_tablen BINARY_INTEGER;
    l_tab VARCHAR2[];
BEGIN
    DBMS_UTILITY.COMMA_TO_TABLE(l_name, l_tablen, l_tab);
    FOR i IN 1..l_tablen LOOP
        RAISE NOTICE '[%]: [%]', i, l_tab[i];
    END LOOP;
END;
/

-- ==============================================================
-- Test for PROCEDURE CREATE_ALTER_TYPE_ERROR_TABLE (
--      schema IN VARCHAR2,
--      table_name IN VARCHAR2)
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
BEGIN
    DBMS_UTILITY.CREATE_ALTER_TYPE_ERROR_TABLE('PUBLIC', 'error_tb');
    IF EXISTS(SELECT 1 FROM pg_class WHERE relname = 'error_tb') THEN
        DBMS_OUTPUT.PUT_LINE('create error table public.error success');
    END IF;
END $$;
/

DO $$
BEGIN
    DBMS_UTILITY.CREATE_ALTER_TYPE_ERROR_TABLE('PUBLIC', 'error_tb');
    IF EXISTS(SELECT 1 FROM pg_class WHERE relname = 'error_tb') THEN
        DBMS_OUTPUT.PUT_LINE('create error table public.error success');
    END IF;
END $$;
/

DO $$
BEGIN
    DBMS_UTILITY.CREATE_ALTER_TYPE_ERROR_TABLE('PUBLIC1', 'error_tb');
    IF EXISTS(SELECT 1 FROM pg_class WHERE relname = 'error_tb') THEN
        DBMS_OUTPUT.PUT_LINE('create error table public.error success');
    END IF;
END $$;
/

DROP TABLE public.error_tb;

-- ==============================================================
-- Test for PROCEDURE DB_VERSION(
--      version OUT VARCHAR2,
--      compatibility OUT VARCHAR2)
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    version VARCHAR2;
    compatibility VARCHAR2;
BEGIN
    DBMS_UTILITY.DB_VERSION(version, compatibility);
    DBMS_OUTPUT.PUT_LINE('version: [' || LEFT(version, 23) || ']');
    DBMS_OUTPUT.PUT_LINE('version: [' || compatibility || ']');
END $$;
/

-- ==============================================================
-- Test for PROCEDURE EXEC_DDL_STATEMENT(
--      parse_string IN VARCHAR2)
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    entry RECORD;
BEGIN
    DBMS_UTILITY.EXEC_DDL_STATEMENT('CREATE TABLE ddl_tb(id int)');
    IF EXISTS(SELECT 1 FROM pg_class WHERE relname = 'ddl_tb') THEN
        DBMS_OUTPUT.PUT_LINE('create table ddl_tb success');
    END IF;
    DBMS_UTILITY.EXEC_DDL_STATEMENT('ALTER TABLE ddl_tb add column name text');
    FOR entry IN SELECT table_name, column_name, data_type FROM SYS.ALL_TAB_COLS WHERE UPPER(table_name) = 'DDL_TB' LOOP
        DBMS_OUTPUT.PUT_LINE(entry.table_name || '  ' || entry.column_name || '  ' || entry.data_type);
    END LOOP;
    DBMS_UTILITY.EXEC_DDL_STATEMENT('DROP TABLE ddl_tb');
    IF NOT EXISTS(SELECT 1 FROM pg_class WHERE relname = 'ddl_tb') THEN
        DBMS_OUTPUT.PUT_LINE('drop table ddl_tb success');
    END IF;
END $$;
/

CALL DBMS_UTILITY.EXEC_DDL_STATEMENT('
    CREATE OR REPLACE PROCEDURE exec_ddl_proc IS
    BEGIN
        DBMS_OUTPUT.PUT_LINE(''This is a procedure created by exec_ddl_statement'');
    END;
');
CALL exec_ddl_proc();
CALL DBMS_UTILITY.EXEC_DDL_STATEMENT('DROP PROCEDURE exec_ddl_proc');

-- ==============================================================
-- Test for PROCEDURE dbms_utility.expand_sql_text(
--     input_sql_text   IN  TEXT,
--     output_sql_text  OUT NOCOPY TEXT
-- )
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================

-- --------------------------------------------------------------
-- Helper: Compare two SQL queries for result equality
-- --------------------------------------------------------------
CREATE OR REPLACE FUNCTION results_match(sql1 TEXT, sql2 TEXT)
RETURNS BOOLEAN AS $$
DECLARE
    diff_count INT;
BEGIN
    sql1 := trim(sql1);
    sql2 := trim(sql2);
    IF sql1 = '' OR sql2 = '' THEN
        RETURN sql1 = sql2;
    END IF;

    EXECUTE format('
        SELECT COUNT(*) FROM (
            ( %s ) EXCEPT ( %s )
            UNION ALL
            ( %s ) EXCEPT ( %s )
        ) AS diff
    ', sql1, sql2, sql2, sql1) INTO diff_count;

    RETURN diff_count = 0;
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE 'Comparison failed: %', SQLERRM;
    RETURN FALSE;
END;
$$ LANGUAGE plpgsql;
/

-- --------------------------------------------------------------
-- Setup test data
-- --------------------------------------------------------------
DROP TABLE IF EXISTS employees, departments;
CREATE TABLE employees (
    emp_id INT,
    name TEXT,
    dept_id INT,
    salary NUMERIC,
    manager_id INT
);

CREATE TABLE departments (
    dept_id   INT PRIMARY KEY,
    dept_name TEXT NOT NULL
);

INSERT INTO departments (dept_id, dept_name) VALUES
    (1, 'Engineering'),
    (2, 'Marketing'),
    (3, 'Human Resources'),
    (4, 'Sales');

INSERT INTO employees (emp_id, name, dept_id, salary, manager_id) VALUES
    (1, 'Alice CEO', 1, 200000, NULL),
    (2, 'Bob Engineer', 1, 95000, 1),
    (3, 'Charlie Coder', 1, 85000, 1),
    (4, 'David Dev', 1, 65000, 2),
    (5, 'Eve Architect', 1, 110000, 1),
    (6, 'Frank Marketer', 2, 75000, 1),
    (7, 'Grace Analyst', 2, 68000, 6),
    (8, 'Henry HR', 3, 72000, 1),
    (9, 'Ivy Sales', 4, 80000, 1),
    (10, 'Jack Closer', 4, 90000, 1);

-- --------------------------------------------------------------
-- Create test views
-- --------------------------------------------------------------
CREATE OR REPLACE VIEW v_emp_simple AS
SELECT emp_id, name, dept_id, salary FROM employees;

CREATE OR REPLACE VIEW v_dept AS
SELECT dept_id, dept_name FROM departments;

CREATE OR REPLACE VIEW v_emp_display AS
SELECT emp_id, UPPER(name) AS full_name, salary * 12 AS annual_salary
FROM employees;

CREATE OR REPLACE VIEW v_high_paid AS
SELECT emp_id, name, salary, dept_id
FROM v_emp_simple
WHERE salary > 70000;

CREATE OR REPLACE VIEW v_top_engineers AS
SELECT e.name
FROM v_high_paid e
JOIN v_dept d ON e.dept_id = d.dept_id
WHERE d.dept_name = 'Engineering';

CREATE OR REPLACE VIEW v_with_cte AS
WITH regional_salaries AS (
    SELECT dept_id, AVG(salary) as avg_sal
    FROM v_emp_simple
    GROUP BY dept_id
)
SELECT e.name, r.avg_sal
FROM v_emp_simple e
JOIN regional_salaries r ON e.dept_id = r.dept_id;

CREATE OR REPLACE VIEW v_recursive_org AS
WITH RECURSIVE subordinates AS (
    SELECT emp_id, name, manager_id
    FROM employees
    WHERE emp_id = 1
UNION ALL
    SELECT e.emp_id, e.name, e.manager_id
    FROM employees e
    INNER JOIN subordinates s ON e.manager_id = s.emp_id
)
SELECT * FROM subordinates;

CREATE OR REPLACE VIEW v_with_lateral AS
SELECT e.name, top_pays.salary
FROM v_emp_simple e,
LATERAL (
    SELECT salary
    FROM v_emp_simple e2
    WHERE e2.dept_id = e.dept_id
    ORDER BY salary DESC
    LIMIT 3
) AS top_pays;

CREATE OR REPLACE VIEW v_top_earners AS
SELECT emp_id, name, salary
FROM v_emp_simple
ORDER BY salary DESC
LIMIT 5;

CREATE OR REPLACE VIEW v_static_data AS
SELECT * FROM (VALUES (100, 'Static Name')) AS t(emp_id, name);
-- --------------------------------------------------------------
-- Run test cases
-- --------------------------------------------------------------
DO $$
DECLARE
    test_record RECORD;
    result VARCHAR2(8192);
    test_result RECORD;
BEGIN
    CREATE TEMP TABLE test_case_tb(
        original_sql VARCHAR2(1024),
        description TEXT
    );
    CREATE TEMP TABLE test_result_tb(
        description TEXT,
        result VARCHAR2(1024)
    );
    INSERT INTO test_case_tb VALUES
        ('SELECT emp_id FROM employees', 'Test 1: Base table query'),
        ('SELECT emp_id, name FROM v_emp_simple', 'Test 2: Single-level view'),
        ('SELECT name FROM v_high_paid', 'Test 3: Two-level nested view'),
        ('SELECT name FROM v_top_engineers', 'Test 4: Three-level nested view'),
        ('SELECT full_name, annual_salary FROM v_emp_display WHERE emp_id = 101', 'Test 5: View with expressions and aliases'),
        ('SELECT e.name, d.dept_name FROM v_emp_simple e
            JOIN v_dept d ON e.dept_id = d.dept_id
            WHERE d.dept_name IS NOT NULL', 'Test 6: Subquery with JOIN of two views'),
        ('SELECT 
            (SELECT COUNT(*) FROM v_emp_simple) AS emp_count,
            (SELECT MAX(salary) FROM v_emp_simple) AS max_salary', 'Test 7: Scalar subqueries'),
        ('SELECT item
            FROM (
                SELECT name AS item FROM v_emp_simple WHERE emp_id IN (101, 102)
                UNION ALL
                SELECT dept_name AS item FROM v_dept
            ) combined
            ORDER BY item', 'Test 8: UNION in subquery'),
        ('', 'Test 9: Empty SQL string'),
        ('SELECT name FROM v_emp_simple WHERE emp_id = 101', 'Test 10: WHERE on view'),
        ('SELECT dept_id, COUNT(*) FROM v_emp_simple GROUP BY dept_id ORDER BY dept_id', 'Test 11: Aggregation'),
        ('WITH tmp_cte (emp_id, name, salary, dept_id) AS (
                SELECT emp_id, name, salary, dept_id FROM v_high_paid
                UNION ALL
                SELECT emp_id, name, salary, dept_id FROM v_emp_simple
            )
            SELECT emp_id, name, salary, dept_id FROM tmp_cte', 'Test 12: CTE'),
        ('SELECT * FROM v_with_cte', 'Test 13: View containing a CTE'),
        ('SELECT * FROM v_recursive_org', 'Test 14: View containing a recursive CTE'),
        ('SELECT * FROM v_with_lateral', 'Test 15: View containing a LATERAL JOIN'),
        ('SELECT * FROM v_top_earners', 'Test 16: View with ORDER BY and LIMIT'),
        ('SELECT * FROM v_static_data', 'Test 17: View based on VALUES clause'),
        ('SELECT * FROM public.v_emp_simple', 'Test 18: Schema-qualified view name'),
        ('SELECT * FROM (SELECT * FROM (SELECT * FROM v_emp_simple) AS inner1) AS outer1', 'Test 19: Deeply nested subqueries (4+ levels)'),
        ('SELECT e1.name, e2.name FROM v_emp_simple e1, v_emp_simple e2 WHERE e1.dept_id = e2.dept_id AND e1.emp_id != e2.emp_id', 'Test 20: Multiple references to the same view'),
        ('select (select count(*) from v_top_engineers) as temp_count, 1 from dual', 'Test 21: Scalar subqueries and RTE'),
        ('WITH tmp_cte (emp_id, name, salary, dept_id) AS (
                SELECT emp_id, name, salary, dept_id FROM v_high_paid
                UNION ALL
                SELECT emp_id, name, salary, dept_id FROM v_emp_simple
            )
            SELECT emp_id, name, salary, dept_id FROM v_high_paid', 'Test 22: CTE but not used');

    FOR test_record IN SELECT * FROM test_case_tb ORDER BY ctid LOOP
        BEGIN
            dbms_utility.expand_sql_text(input_sql_text => test_record.original_sql, output_sql_text => result);
            RAISE NOTICE 'test case [%], original SQL [%], Expanded SQL: [%]', test_record.description, test_record.original_sql, result;
            IF test_record.original_sql = '' THEN
                IF result = '' THEN
                    INSERT INTO test_result_tb VALUES (test_record.description, 'PASS');
                ELSE
                    INSERT INTO test_result_tb VALUES (test_record.description, 'FAILED: ' || result);
                END IF;
            ELSE
                IF result IS NULL THEN
                    INSERT INTO test_result_tb VALUES (test_record.description, 'FAILED: expand_sql_text return NULL');
                END IF;

                IF results_match(test_record.original_sql, result) THEN
                    INSERT INTO test_result_tb VALUES (test_record.description, 'PASS');
                ELSE
                    INSERT INTO test_result_tb VALUES (test_record.description, 'FAILED: result sets differ');
                END IF;
            END IF;
            
        END;
    END LOOP;

    FOR test_result IN SELECT * FROM test_result_tb ORDER BY ctid LOOP
        RAISE NOTICE 'test case [%], result [%]', test_result.description, test_result.result;
    END LOOP;
    DROP TABLE test_case_tb;
    DROP TABLE test_result_tb;
END $$;
/

DROP FUNCTION results_match(TEXT, TEXT);
DROP TABLE employees CASCADE;
DROP TABLE departments CASCADE;

-- ==============================================================
-- Test for PEOCEDURE INVALIDATE() AND VALIDATE()
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
CREATE OR REPLACE FUNCTION test_func_invalid RETURN TEXT IS
BEGIN
    RETURN 'Hello from function test_func_invalid!';
END;
/

CREATE OR REPLACE PROCEDURE test_proc_invalid AS
BEGIN
    DBMS_OUTPUT.PUT_LINE('Hello from procedure test_proc_invalid!');
END;
/

SELECT * FROM test_func_invalid();

-- invalidate object
DO $$
DECLARE
    proc_id OID;
BEGIN
    test_proc_invalid();
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_func_invalid';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);

    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_proc_invalid';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);
END $$;
/

SELECT object_name, object_type, status FROM all_objects WHERE lower(object_name) IN ('test_func_invalid', 'test_proc_invalid') ORDER BY object_name;

DO $$
BEGIN
    test_proc_invalid();
END $$;
/

SELECT * FROM test_func_invalid();

-- validate object
DO $$
DECLARE
    proc_id OID;
BEGIN
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_func_invalid';
    DBMS_UTILITY.VALIDATE(proc_id::text::number);

    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_proc_invalid';
    DBMS_UTILITY.VALIDATE(proc_id::text::number);
END $$;
/

SELECT object_name, object_type, status FROM all_objects WHERE lower(object_name) IN ('test_func_invalid', 'test_proc_invalid') ORDER BY object_name;

DO $$
BEGIN
    test_proc_invalid();
END $$;
/

SELECT * FROM test_func_invalid();

-- invalidate object
DO $$
DECLARE
    proc_id OID;
BEGIN
    test_proc_invalid();
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_func_invalid';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);

    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_proc_invalid';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);
END $$;
/

SELECT object_name, object_type, status FROM all_objects WHERE lower(object_name) IN ('test_func_invalid', 'test_proc_invalid') ORDER BY object_name;

DO $$
BEGIN
    test_proc_invalid();
END $$;
/

SELECT * FROM test_func_invalid();

-- validate object
DO $$
DECLARE
    nspoid OID;
    usr VARCHAR2;
BEGIN
    IF ORA_CASE_TRANS(current_user::VARCHAR2) = current_user THEN
        usr := (current_user::VARCHAR2);
    ELSE
        usr := UPPER(current_user::VARCHAR2);
    END IF;
    SELECT oid INTO nspoid FROM pg_namespace WHERE nspname = current_schema;
    DBMS_UTILITY.VALIDATE(owner => usr, objname => 'TEST_FUNC_INVALID', namespace => nspoid::text::number);
    DBMS_UTILITY.VALIDATE(owner => usr, objname => 'TEST_PROC_INVALID', namespace => nspoid::text::number);
END $$;
/

SELECT object_name, object_type, status FROM all_objects WHERE lower(object_name) IN ('test_func_invalid', 'test_proc_invalid') ORDER BY object_name;

DROP PROCEDURE test_proc_invalid;
DROP FUNCTION test_func_invalid;

CREATE OR REPLACE PROCEDURE "test_PROC" IS
BEGIN
    DBMS_OUTPUT.PUT_LINE('This is a test proc');
END;
/

CALL "test_PROC"();
DO $$
DECLARE
    proc_id OID;
BEGIN
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_PROC';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);
END $$;
/

CALL "test_PROC"();
DO $$
DECLARE
    proc_id OID;
BEGIN
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_PROC';
    DBMS_UTILITY.VALIDATE(proc_id::text::number);
END $$;
/

CALL "test_PROC"();
DO $$
DECLARE
    proc_id OID;
BEGIN
    SELECT oid INTO proc_id FROM pg_proc WHERE proname = 'test_PROC';
    DBMS_UTILITY.INVALIDATE(proc_id::text::number);
END $$;
/

CALL "test_PROC"();
DO $$
DECLARE
    nspoid OID;
    usr VARCHAR2;
BEGIN
    IF ORA_CASE_TRANS(current_user::VARCHAR2) = current_user THEN
        usr := (current_user::VARCHAR2);
    ELSE
        usr := UPPER(current_user::VARCHAR2);
    END IF;
    SELECT oid INTO nspoid FROM pg_namespace WHERE nspname = current_schema;
    DBMS_UTILITY.VALIDATE(owner => usr, objname => 'test_PROC', namespace => nspoid::text::number);
END $$;
/

CALL "test_PROC"();

DROP PROCEDURE "test_PROC";

-- ==============================================================
-- Test for PROCEDURE NAME_RESOLVE()
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
CREATE TABLE dbms_utility_tb(id int, created TIMESTAMPTZ);
INSERT INTO dbms_utility_tb VALUES (0, NOW());
CREATE INDEX dbms_utility_tb_idx on dbms_utility_tb(id);
CREATE VIEW dbms_utility_v1 AS SELECT * FROM dbms_utility_tb;
CREATE MATERIALIZED VIEW dbms_utility_v2 AS SELECT * FROM dbms_utility_tb;
CREATE SEQUENCE public.dbms_utility_seq;
CREATE SYNONYM syn1 FOR dbms_utility_tb;
CREATE TYPE typ AS (id int, name text);

CREATE FUNCTION update_time_func() RETURNS TRIGGER AS $$
BEGIN
    NEW.CREATED = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
/
CREATE TRIGGER dbms_utility_tb_trig BEFORE INSERT ON dbms_utility_tb
FOR EACH ROW
EXECUTE FUNCTION update_time_func();
/

CREATE PROCEDURE test_proc IS
BEGIN
    RAISE NOTICE 'test procedure';
END;
/

CREATE OR REPLACE PACKAGE test_pkg AS
    PROCEDURE proc_1();
    FUNCTION func_1() RETURN TEXT;
END;
/
CREATE OR REPLACE PACKAGE BODY test_pkg IS
    PROCEDURE proc_1() AS
    BEGIN
        RAISE NOTICE 'test procedure';
    END;

    FUNCTION func_1() RETURN TEXT IS
    BEGIN
        RETURN 'test function'::TEXT;
    END;
END;
/

DO $$
DECLARE
    l_schema VARCHAR2(128);
    l_part1 VARCHAR2(128);
    l_part2 VARCHAR2(128);
    l_dblink VARCHAR2(128);
    l_part1_type NUMBER;
    l_object_number NUMBER;
    tup RECORD;
BEGIN
    CREATE TABLE name_resolve_tb(name VARCHAR2(128), context NUMBER);
    INSERT INTO name_resolve_tb VALUES('dbms_utility_v1', 0),
                                    ('public.dbms_utility_v1', 0),
                                    ('sys.dbms_utility_v1', 0),
                                    ('dbms_utility_v2', 0),
                                    ('dbms_utility_tb', 0),
                                    ('test_proc', 1),
                                    ('update_time_func', 1),
                                    ('test_pkg', 1),
                                    ('public.test_proc', 1),
                                    ('public.test_pkg', 1),
                                    ('test_pkg.proc_1', 1),
                                    ('test_pkg.func_1', 1),
                                    ('public.test_pkg.func_1', 1),
                                    ('public.dbms_utility_seq', 2),
                                    ('sys.dbms_utility_seq', 2),
                                    ('dbms_utility_seq', 2),
                                    ('dbms_utility_tb_trig', 3),
                                    ('syn1', 4),
                                    ('typ', 7),
                                    ('dbms_utility_tb_idx', 9),
                                    ('public.dbms_utility_tb@dblink', 0),
                                    
                                    ('DBMS_UTILITY_V1', 0),
                                    ('DBMS_UTILITY_TB', 0),
                                    ('TEST_PROC', 1),
                                    ('UPDATE_TIME_FUNC', 1),
                                    ('TEST_PKG', 1),
                                    ('PUBLIC.TEST_PROC', 1),
                                    ('PUBLIC.TEST_PKG', 1),

                                    ('dbms_utility_v1', 1),
                                    ('public.dbms_utility_v1', 1),
                                    ('sys.dbms_utility_v1', 1),
                                    ('dbms_utility_v2', 1),
                                    ('dbms_utility_tb', 1),
                                    ('test_proc', 2),
                                    ('update_time_func', 2),
                                    ('test_pkg', 2),
                                    ('public.test_proc', 2),
                                    ('public.test_pkg', 2),
                                    ('test_pkg.proc_1', 2),
                                    ('test_pkg.func_1', 2),
                                    ('public.test_pkg.func_1', 2),
                                    ('public.dbms_utility_seq', 3),
                                    ('sys.dbms_utility_seq', 3),
                                    ('dbms_utility_seq', 3),
                                    ('dbms_utility_tb_trig', 4),
                                    ('syn1', 5),
                                    ('typ', 8),
                                    ('dbms_utility_tb_idx', 8),
                                    ('public.dbms_utility_tb@dblink', 1);
    FOR tup IN SELECT * FROM name_resolve_tb LOOP
        BEGIN
            DBMS_OUTPUT.PUT_LINE('----------------------------' || tup.name || '----------------------------');
            DBMS_UTILITY.NAME_RESOLVE(tup.name, tup.context, l_schema, l_part1, l_part2, l_dblink, l_part1_type, l_object_number);
            DBMS_OUTPUT.PUT_LINE('schema     :' || l_schema);
            DBMS_OUTPUT.PUT_LINE('part1      :' || l_part1);
            DBMS_OUTPUT.PUT_LINE('part2      :' || l_part2);
            DBMS_OUTPUT.PUT_LINE('part1_type :' || l_part1_type);
            DBMS_OUTPUT.PUT_LINE('dblink     :' || l_dblink);

        EXCEPTION WHEN OTHERS THEN
                RAISE NOTICE 'ERROR: %', SQLERRM;
        END;
        
    END LOOP;
    DROP TABLE name_resolve_tb;
END $$;
/

DROP TRIGGER dbms_utility_tb_trig;
DROP FUNCTION update_time_func;
DROP VIEW dbms_utility_v1;
DROP MATERIALIZED VIEW dbms_utility_v2;
DROP TABLE dbms_utility_tb;
DROP SEQUENCE public.dbms_utility_seq;
DROP SYNONYM syn1;
DROP TYPE typ;
DROP PROCEDURE test_proc;
DROP PACKAGE test_pkg;

-- ==============================================================
-- Test for PROCEDURE NAME_TOKENIZE()
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    l_a VARCHAR2;
    l_b VARCHAR2;
    l_c VARCHAR2;
    l_dblink VARCHAR2;
    l_nextpos BINARY_INTEGER;
    PROCEDURE test_tokenize(p_name IN VARCHAR2) IS
    BEGIN
        DBMS_UTILITY.NAME_TOKENIZE(name => p_name, a => l_a, b => l_b, c => l_c, dblink => l_dblink, nextpos => l_nextpos);
        DBMS_OUTPUT.PUT_LINE('------------ Input: ' || p_name || '---------------');
        DBMS_OUTPUT.PUT_LINE('a         = [' || NVL(l_a, NULL) || ']');
        DBMS_OUTPUT.PUT_LINE('b         = [' || NVL(l_b, NULL) || ']');
        DBMS_OUTPUT.PUT_LINE('b         = [' || NVL(l_c, NULL) || ']');
        DBMS_OUTPUT.PUT_LINE('db_link   = [' || NVL(l_dblink, NULL) || ']');
        DBMS_OUTPUT.PUT_LINE('nextpos   = [' || NVL(l_nextpos, NULL) || ']');
    END;
BEGIN
    test_tokenize('EMP');
    test_tokenize('HR.EMPLOYEES');
    test_tokenize('MY_PKG.MY_PROC.MY_SUB');
    test_tokenize('TABLE1@REMOTE');
    test_tokenize('"MySchema"."MyTable"@MyLink');
    test_tokenize('"MySchema"."MyTable"@"MyLink"');
    test_tokenize('"MySchema".MyTable@"MyLink"');
    test_tokenize('my_schema.My_Table');
END $$;
/

-- ==============================================================
-- Test for FUNCTION FORMAT_CALL_STACK() RETURN VARCHAR2
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
SET SERVEROUTPUT OFF;

CREATE OR REPLACE PACKAGE test_call_pkg AS
    PROCEDURE proc_1();
    FUNCTION func_1() RETURN TEXT;
END;
/

CREATE OR REPLACE PACKAGE BODY test_call_pkg IS
    PROCEDURE proc_1() AS
    BEGIN
        DBMS_OUTPUT.PUT_LINE('--- in test_call_pkg.proc_1 ---');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_CALL_STACK);
    END;

    FUNCTION func_1() RETURN TEXT IS
    BEGIN
        DBMS_OUTPUT.PUT_LINE('--- in test_call_pkg.func_1 ---');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_CALL_STACK);
        RETURN 'test function'::TEXT;
    END;
END;
/

CREATE SCHEMA test_call_sch;
CREATE OR REPLACE PACKAGE test_call_sch.test_call_pkg AS
    PROCEDURE proc_1();
    FUNCTION func_1() RETURN TEXT;
END;
/

CREATE OR REPLACE PACKAGE BODY test_call_sch.test_call_pkg IS
    PROCEDURE proc_1() AS
    BEGIN
        DBMS_OUTPUT.PUT_LINE('--- in test_call_sch.test_call_pkg.proc_1 ---');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_CALL_STACK);
    END;

    FUNCTION func_1() RETURN TEXT IS
    BEGIN
        DBMS_OUTPUT.PUT_LINE('--- in test_call_sch.test_call_pkg.func_1 ---');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_CALL_STACK);
        RETURN 'test function'::TEXT;
    END;
END;
/

CREATE OR REPLACE PROCEDURE proc_c IS
BEGIN
    DBMS_OUTPUT.PUT_LINE('--- in PROC_C ---');
    DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_CALL_STACK);
END;
/

CREATE OR REPLACE PROCEDURE proc_b IS
BEGIN
    proc_c;
END;
/

CREATE OR REPLACE PROCEDURE proc_a IS
BEGIN
    proc_b;
END;
/

DO $$
BEGIN
    proc_a;
    test_call_pkg.proc_1;
    DBMS_OUTPUT.PUT_LINE(test_call_pkg.func_1());
    DBMS_OUTPUT.PUT_LINE(test_call_sch.test_call_pkg.func_1());
END $$;
/

DROP PROCEDURE proc_a;
DROP PROCEDURE proc_b;
DROP PROCEDURE proc_c;
DROP PACKAGE test_call_pkg;
DROP PACKAGE test_call_sch.test_call_pkg;
DROP SCHEMA test_call_sch;

-- ==============================================================
-- Test for FUNCTION FORMAT_ERROR_STACK/BACKTRACE
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
SET SERVEROUTPUT ON;
CREATE OR REPLACE PROCEDURE P0 IS
    a int;
BEGIN
    a = 10 / 0;
END P0;
/

CREATE OR REPLACE PROCEDURE P1 IS
BEGIN
    P0();
END P1;
/

CREATE OR REPLACE PROCEDURE P2 IS
BEGIN
    P1();
END P2;
/

CREATE OR REPLACE PROCEDURE P3 IS
BEGIN
    P2();
    EXCEPTION WHEN OTHERS THEN
        INSERT INTO TB VALUES(1);
END P3;
/

CREATE OR REPLACE PROCEDURE P4 IS
BEGIN P3(); END P4;
/

CREATE OR REPLACE PROCEDURE P5 IS
BEGIN P4(); END P5;
/


CREATE OR REPLACE PROCEDURE Top_With_Logging IS
BEGIN
    BEGIN
        P0();
        EXCEPTION WHEN OTHERS THEN
            BEGIN
                INSERT INTO TB VALUES(0);
                EXCEPTION WHEN OTHERS THEN
                    BEGIN
                        P3();
                        EXCEPTION WHEN OTHERS THEN
                            DBMS_OUTPUT.PUT_LINE('ERROR STACK 1:');
                            DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
                            DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE 1:');
                            DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
                    END;
                DBMS_OUTPUT.PUT_LINE('ERROR STACK 2:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
                DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE 2:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
            END;

            BEGIN
                ALTER TABLE TB ADD COLUMN NAME TEXT;
                EXCEPTION WHEN OTHERS THEN
                    DBMS_OUTPUT.PUT_LINE('ERROR STACK 3:');
                    DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
                    DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE 3:');
                    DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
            END;
    END;

    BEGIN
        P5();
        EXCEPTION WHEN OTHERS THEN
            DBMS_OUTPUT.PUT_LINE('ERROR STACK 4:');
            DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
            DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE 4:');
            DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
    END;
END Top_With_Logging;
/

Set ServerOutput On;
call Top_With_Logging();

drop procedure Top_With_Logging;
drop procedure p5;
drop procedure p4;
drop procedure p3;
drop procedure p2;
drop procedure p1;
drop procedure p0;

-- ==============================================================
-- Special test for exception nesting scenarios
-- ==============================================================
CREATE OR REPLACE PROCEDURE P0 IS
    a int;
BEGIN
    a = 10 / 0;
    EXCEPTION WHEN OTHERS THEN
        INSERT INTO TB VALUES(1);
END P0;
/

CREATE OR REPLACE PROCEDURE P1 IS
BEGIN
    INSERT INTO TB1 VALUES(1);
    EXCEPTION WHEN OTHERS THEN
        BEGIN
            P0();
            EXCEPTION WHEN OTHERS THEN
                DBMS_OUTPUT.PUT_LINE('ERROR STACK inner:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
                DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE inner:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
        END;

        DBMS_OUTPUT.PUT_LINE('ERROR STACK outer:');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
        DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE outer:');
        DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
END P1;
/

CALL P1();

DROP PROCEDURE P1;
DROP PROCEDURE P0;

-- ==============================================================
-- Special test for package
-- ==============================================================
CREATE OR REPLACE PROCEDURE P0 IS
    a int;
BEGIN
    a = 10 / 0;
END P0;
/

CREATE OR REPLACE PACKAGE test_pkg AS
    PROCEDURE proc_1();
    FUNCTION func_1() RETURN TEXT;
END;
/
CREATE OR REPLACE PACKAGE BODY test_pkg IS
    PROCEDURE proc_1() AS
    BEGIN
        p0;
        EXCEPTION WHEN OTHERS THEN
        BEGIN
            INSERT INTO TB1 VALUES(1);
            EXCEPTION WHEN OTHERS THEN
                DBMS_OUTPUT.PUT_LINE('ERROR STACK:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_STACK());
                DBMS_OUTPUT.PUT_LINE('ERROR BACKTRACE:');
                DBMS_OUTPUT.PUT_LINE(DBMS_UTILITY.FORMAT_ERROR_BACKTRACE());
        END;
    END;

    FUNCTION func_1() RETURN TEXT IS
    BEGIN
        INSERT INTO TB1 VALUES(1);
        RETURN 'test function'::TEXT;
    END;
END;
/

CALL test_pkg.proc_1();

DROP PACKAGE test_pkg;
-- ==============================================================
-- Test for FUNCTION GET_SQL_HASH() RETURN VARCHAR2
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    name text := 'select * from tb';
    hash raw;
    pre10ihash number;
    ret number;
BEGIN
    ret := DBMS_UTILITY.GET_SQL_HASH(name, hash, pre10ihash);
    DBMS_OUTPUT.PUT_LINE('origin hash value:');
    DBMS_OUTPUT.PUT_LINE(hash);
    DBMS_OUTPUT.PUT_LINE('return hash value:');
    DBMS_OUTPUT.PUT_LINE(ret);
END $$;
/

-- ==============================================================
-- Test for FUNCTION GET_PARAMETER_VALUE() RETURN NUMBER
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
DO $$
DECLARE
    tup RECORD;
    intval NUMBER;
    strval VARCHAR2;
    ret NUMBER;
BEGIN
    CREATE TABLE param_tb(name VARCHAR2(128));
    INSERT INTO param_tb VALUES
        ('autovacuum'),
        ('autovacuum_max_workers'),
        ('ivorysql.compatible_mode'),
        ('transaction_isolation'),
        ('invalid_parameter_name');
    FOR tup IN SELECT * FROM param_tb LOOP
        DBMS_OUTPUT.PUT_LINE('---------------param: ' || tup.name || '-------------------');
        ret := DBMS_UTILITY.GET_PARAMETER_VALUE(tup.name, intval, strval);
        IF ret = 0 THEN
            DBMS_OUTPUT.PUT_LINE('parameter type is integer or bool');
            DBMS_OUTPUT.PUT_LINE('parameter value: ' || intval);
        ELSE
            DBMS_OUTPUT.PUT_LINE('parameter type is string');
            DBMS_OUTPUT.PUT_LINE('parameter value: ' || strval);
            DBMS_OUTPUT.PUT_LINE('length of parameter value: ' || intval);
        END IF;
    END LOOP;
    DROP TABLE param_tb;
END $$;
/

-- ==============================================================
-- Test for FUNCTION IS_BIT_SET()
-- Uses CALL inside PL/pgSQL to capture OUT parameter.
-- ==============================================================
select * from dbms_utility.is_bit_set('\x0101', 0);
select * from dbms_utility.is_bit_set('\x0101', 3);
select * from dbms_utility.is_bit_set('\x0101', 8);
select * from dbms_utility.is_bit_set('\x0101', -1);
select * from dbms_utility.is_bit_set('\x0101', 65);

-- ==============================================================
-- ANALYZE_PART_OBJECT
-- ==============================================================
CREATE SCHEMA test_SCH_1;
CREATE SCHEMA "test_SCH";
CREATE TABLE "test_SCH".test_part(id int, name text) PARTITION BY HASH(id) PARTITIONS 2;
CREATE TABLE "test_SCH"."TEST_part_1"(id int, name text) PARTITION BY HASH(id) PARTITIONS 2;

CREATE TABLE test_SCH_1.test_part(id int, name text) PARTITION BY HASH(id) PARTITIONS 2;
CREATE TABLE test_SCH_1."TEST_part_1"(id int, name text) PARTITION BY HASH(id) PARTITIONS 2;

CALL DBMS_UTILITY.ANALYZE_PART_OBJECT('test_SCH', 'TEST_PART');
CALL DBMS_UTILITY.ANALYZE_PART_OBJECT('test_SCH', 'TEST_part_1');

CALL DBMS_UTILITY.ANALYZE_PART_OBJECT('TEST_SCH_1', 'TEST_PART');
CALL DBMS_UTILITY.ANALYZE_PART_OBJECT('TEST_SCH_1', 'TEST_part_1');

DROP SCHEMA test_SCH_1 CASCADE;
DROP SCHEMA "test_SCH" CASCADE;


-- ==============================================================
-- ANALYZE_SCHEMA
-- ==============================================================
CREATE USER system WITH superuser;
CREATE USER enrol11 superuser;
\c - enrol11
SET ivorysql.identifier_case_switch TO interchange;

CREATE SCHEMA test_sch_1;
CREATE SCHEMA "test_SCH";
CREATE TABLE "test_SCH".test_tb(id int, name text);
CREATE TABLE "test_SCH"."TEST_tb_1"(id int, name text);

CREATE TABLE test_sch_1.test_tb(id int, name text);
CREATE TABLE test_sch_1."TEST_tb_1"(id int, name text);

CALL DBMS_UTILITY.ANALYZE_SCHEMA('TEST_SCH_1');
CALL DBMS_UTILITY.ANALYZE_SCHEMA('test_SCH');

DROP SCHEMA test_sch_1 CASCADE;
DROP SCHEMA "test_SCH" CASCADE;

\c - system
drop user enrol11;
declare
    name text;
begin
        select into name 10::regrole;
		execute format('set session authorization %s;',name);
end;
/

DROP USER system;
