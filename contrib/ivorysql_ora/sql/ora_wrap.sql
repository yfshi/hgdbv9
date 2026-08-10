CREATE SCHEMA wrap_test;
SET search_path TO wrap_test, sys, public;
SHOW ivorysql.wrap_algo;
CREATE ROLE wrap_nonsuper;
SET ROLE wrap_nonsuper;
SET ivorysql.wrap_algo TO aes;
RESET ROLE;
DROP ROLE wrap_nonsuper;

SELECT COUNT(*) = 1 AS dbms_ddl_wrap_function_installed FROM pg_proc p JOIN pg_namespace n ON n.oid = p.pronamespace
  WHERE n.nspname = 'sys' AND p.proname = 'dbms_ddl_wrap';

SELECT COUNT(*) = 1 AS dbms_wrap_packages_installed FROM pg_package p JOIN pg_namespace n ON n.oid = p.pkgnamespace
  WHERE n.nspname = 'sys' AND lower(p.pkgname::text) = 'dbms_ddl';

SELECT COUNT(*) = 1 AS dbms_wrap_package_bodies_installed FROM pg_package_body b
  JOIN pg_package p ON p.oid = b.pkgoid JOIN pg_namespace n ON n.oid = p.pkgnamespace
  WHERE n.nspname = 'sys' AND lower(p.pkgname::text) = 'dbms_ddl';

SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_aes_probe return number is begin return 3; end;') LIKE '%''aes:%' AS dbms_ddl_wrap_aes;

CREATE OR REPLACE FUNCTION wrap_test.direct_wrapped_fn RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 1;
END;
/

CREATE OR REPLACE PROCEDURE wrap_test.direct_encrypted_proc ENCRYPTED IS
BEGIN
  NULL;
END;
/

CREATE OR REPLACE PACKAGE wrap_test.direct_wrapped_pkg WRAPPED IS
  FUNCTION get_value RETURN NUMBER;
END;
/

CREATE OR REPLACE PACKAGE BODY wrap_test.direct_wrapped_pkg ENCRYPTED IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 1;
  END;
END;
/

SELECT wrap_test.direct_wrapped_fn();
SELECT wrap_test.direct_wrapped_pkg.get_value();
SELECT prosrc LIKE 'ivorysql wrapped v1%' AS direct_wrapped_fn_hidden FROM pg_proc WHERE oid = 'wrap_test.direct_wrapped_fn()'::regprocedure;

CREATE TABLE wrap_test.proc_log(value number);

CREATE OR REPLACE PACKAGE wrap_test.direct_multi_pkg WRAPPED IS
  FUNCTION add_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER;
  FUNCTION subtract_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER;
  PROCEDURE write_log(p_value NUMBER);
END;
/

CREATE OR REPLACE PACKAGE BODY wrap_test.direct_multi_pkg ENCRYPTED IS
  FUNCTION add_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER IS
  BEGIN
    RETURN p_left + p_right;
  END;

  FUNCTION subtract_value(p_left NUMBER, p_right NUMBER) RETURN NUMBER IS
  BEGIN
    RETURN p_left - p_right;
  END;

  PROCEDURE write_log(p_value NUMBER) IS
  BEGIN
    INSERT INTO wrap_test.proc_log VALUES (p_value);
  END;
END;
/

SELECT wrap_test.direct_multi_pkg.add_value(8, 3) AS direct_multi_add_result;
SELECT wrap_test.direct_multi_pkg.subtract_value(8, 3) AS direct_multi_subtract_result;
CALL wrap_test.direct_multi_pkg.write_log(1001);
SELECT COUNT(*) = 1 AS direct_multi_procedure_executed FROM wrap_test.proc_log WHERE value = 1001;
DELETE FROM wrap_test.proc_log WHERE value = 1001;
SELECT pkgsrc LIKE 'ivorysql wrapped v1%' AS direct_multi_spec_hidden FROM pg_package WHERE lower(pkgname::text) = 'direct_multi_pkg';
SELECT b.bodysrc LIKE 'ivorysql wrapped v1%' AS direct_multi_body_hidden FROM pg_package_body b 
  JOIN pg_package p ON p.oid = b.pkgoid
  WHERE lower(p.pkgname::text) = 'direct_multi_pkg';

SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_probe return number is begin return 1; end;') LIKE '%ivorysql wrapped%' AS dbms_ddl_wrap_callable;
SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_probe_alias return number is begin return 2; end;') LIKE '%ivorysql wrapped%' AS dbms_ddl_wrap_alias_callable;
SELECT POSITION('WRAPPED' IN UPPER(dbms_ddl.wrap('create or replace function wrap_test.wrap_shape return number is begin return 3; end;'))) > 0
       AS dbms_ddl_wrap_oracle_shape;

SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_fn return number is begin return 42; end;') LIKE '%ivorysql wrapped%' AS ddl_wrap_has_marker;
SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_fn_alias return number is begin return 43; end;') LIKE '%ivorysql wrapped%' AS ddl_wrap_alias_has_marker;
WITH wrapped_once AS (
  SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_idempotent return number is begin return 7; end;') AS ddl
)
SELECT dbms_ddl.wrap(ddl) = ddl AS wrap_is_idempotent FROM wrapped_once;

SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_fn return number is begin return 42; end;') AS ddl_wrap
\gset
:ddl_wrap
/

SELECT dbms_ddl.wrap('create or replace function wrap_test.wrap_fn_alias return number is begin return 43; end;') AS ddl_wrap_alias
\gset
:ddl_wrap_alias
/

SELECT wrap_test.wrap_fn();
SELECT wrap_test.wrap_fn_alias();

SELECT dbms_ddl.wrap($wrap$
create or replace function wrap_test.wrap_special_chars(p_input varchar2) return varchar2 is
begin
  -- The wrapper must preserve non-ASCII and punctuation in string literals.
  /* This block comment is part of the source before wrapping. */
  return '中文单引号'' 双引号" 反斜杠\\ 与符号& 井号# @符号@ 百分号% 星号*'
         || chr(9) || '制表符' || chr(10) || '换行 数字123【括号】 输入值：' || p_input;
end;
$wrap$) AS special_chars_wrap
\gset
:special_chars_wrap
/

SELECT wrap_test.wrap_special_chars('输入42') =
       '中文单引号'' 双引号" 反斜杠\\ 与符号& 井号# @符号@ 百分号% 星号*'
       || chr(9) || '制表符' || chr(10) || '换行 数字123【括号】 输入值：输入42'
       AS special_chars_round_trip;
SELECT prosrc LIKE 'ivorysql wrapped v1%' AS special_chars_source_wrapped
FROM pg_proc
WHERE oid = 'wrap_test.wrap_special_chars(varchar2)'::regprocedure;

-- Keep NOTICE visible to verify that dynamic wrapped DDL does not tokenize
-- the ciphertext as an overlength SQL identifier.
SET client_min_messages TO notice;

DECLARE
  v_src CLOB;
  v_wrap_code CLOB;
BEGIN
  v_src := 'create or replace function wrap_test.wrap_special_block_fn(p_input varchar2) return varchar2 is begin return ''中文单引号'' || chr(39) || '' 双引号" 反斜杠\\ 与符号'' || chr(38) || '' 井号# @符号@ 百分号% 星号*'' || chr(9) || ''制表符'' || chr(10) || ''换行 数字123【括号】 输入值：'' || p_input; end;';
  v_wrap_code := dbms_ddl.wrap(v_src);
  EXECUTE v_wrap_code;
END;
/

SET client_min_messages TO warning;

SELECT wrap_test.wrap_special_block_fn('输入42') =
       '中文单引号'' 双引号" 反斜杠\\ 与符号& 井号# @符号@ 百分号% 星号*'
       || chr(9) || '制表符' || chr(10) || '换行 数字123【括号】 输入值：输入42'
       AS special_block_round_trip;

SELECT dbms_ddl.wrap($wrap$
create or replace function wrap_test.wrap_literal_fn return varchar2 is
begin
  return q'[single quote ' double quote " slash / -- marker /* marker */ ; & # @ % *]';
end;
$wrap$) AS literal_wrap
\gset
:literal_wrap
/

SELECT wrap_test.wrap_literal_fn() = q'[single quote ' double quote " slash / -- marker /* marker */ ; & # @ % *]'
       AS alternative_quote_round_trip;

CREATE OR REPLACE FUNCTION wrap_test."Odd Fn" RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 611;
END;
/
CREATE OR REPLACE FUNCTION wrap_test."Odd Fn" RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 612;
END;
/
SELECT wrap_test."Odd Fn"() AS quoted_identifier_replace_result;

CREATE OR REPLACE PACKAGE wrap_test.wrap_structure_pkg WRAPPED IS
  FUNCTION get_value(p_input NUMBER DEFAULT 7) RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY wrap_test.wrap_structure_pkg WRAPPED IS
  FUNCTION get_value(p_input NUMBER DEFAULT 7) RETURN NUMBER IS
  BEGIN
    BEGIN
      IF p_input < 0 THEN
        RETURN 1 / 0;
      END IF;
      RETURN p_input;
    EXCEPTION
      WHEN OTHERS THEN
        RETURN -1;
    END;
  END;
END;
/
SELECT wrap_test.wrap_structure_pkg.get_value() AS default_parameter_result;
SELECT wrap_test.wrap_structure_pkg.get_value(-1) AS nested_exception_result;

CREATE OR REPLACE FUNCTION wrap_test.corrupt_payload_fn RETURN NUMBER WRAPPED
ivorysql wrapped v1
'aes:not-base642345234523hjjl2k345';
/
SELECT wrap_test.corrupt_payload_fn();

-- WRAPPED syntax error
CREATE OR REPLACE FUNCTION wrap_test.invalid_wrapped_text_fn RETURN NUMBER
WRAPPED xxxx IS
BEGIN
  RETURN 1;
END;
/

SELECT COUNT(*) = 0 AS catalog_plain_source_hidden
FROM pg_proc
WHERE proname = 'wrap_literal_fn'
  AND prosrc ILIKE '%single quote%';
SELECT pg_get_functiondef('wrap_test.wrap_literal_fn()'::regprocedure)
       NOT ILIKE '%single quote%' AS function_definition_plain_source_hidden;
SELECT COUNT(*) = 0 AS user_source_plain_source_hidden
FROM user_source
WHERE name = 'WRAP_LITERAL_FN'
  AND text ILIKE '%single quote%';

SELECT dbms_ddl.wrap($wrap$
create or replace function wrap_test.invalid_body_fn return number is
begin
  return;
end;
$wrap$) AS invalid_body_wrap
\gset
:invalid_body_wrap
/

SELECT wrap_test.invalid_body_fn();

SELECT dbms_ddl.wrap($wrap$
create or replace function wrap_test.missing_dependency_fn return number is
  v_count number;
begin
  select count(*) into v_count from wrap_test.missing_dependency_table;
  return v_count;
end;
$wrap$) AS missing_dependency_wrap
\gset
:missing_dependency_wrap
/

SELECT wrap_test.missing_dependency_fn();

SELECT dbms_ddl.wrap('create or replace procedure wrap_test.wrap_proc is begin insert into wrap_test.proc_log values (88); end;') AS proc_wrap
\gset
:proc_wrap
/

CALL wrap_test.wrap_proc();
SELECT * FROM wrap_test.proc_log;

SELECT COUNT(*) = 0 AS function_source_hidden FROM user_source WHERE name = 'WRAP_FN' AND text ILIKE '%return 42%';

SELECT pg_get_functiondef('wrap_test.wrap_fn()'::regprocedure) LIKE '%ivorysql wrapped%' AS function_def_wrapped;
SELECT pg_get_functiondef('wrap_test.wrap_fn()'::regprocedure) NOT LIKE '%return 42%' AS function_def_hidden;

SELECT dbms_ddl.wrap('create or replace package wrap_test.wrap_pkg is function get_value return number; end;') AS pkg_spec_wrap
\gset
:pkg_spec_wrap
/

SELECT dbms_ddl.wrap('create or replace package body wrap_test.wrap_pkg is function get_value return number is begin return 99; end; end;') AS pkg_body_wrap
\gset
:pkg_body_wrap
/

SELECT wrap_test.wrap_pkg.get_value();

SELECT dbms_ddl.wrap($wrap$
create or replace package wrap_test.dbms_multi_pkg is
  function add_value(p_left number, p_right number) return number;
  function subtract_value(p_left number, p_right number) return number;
  procedure write_log(p_value number);
end;
$wrap$) AS dbms_multi_spec_wrap
\gset
:dbms_multi_spec_wrap
/

SELECT dbms_ddl.wrap($wrap$
create or replace package body wrap_test.dbms_multi_pkg is
  function add_value(p_left number, p_right number) return number is
  begin
    return p_left + p_right;
  end;

  function subtract_value(p_left number, p_right number) return number is
  begin
    return p_left - p_right;
  end;

  procedure write_log(p_value number) is
  begin
    insert into wrap_test.proc_log values (p_value);
  end;
end;
$wrap$) AS dbms_multi_body_wrap
\gset
:dbms_multi_body_wrap
/

SELECT wrap_test.dbms_multi_pkg.add_value(9, 4) AS dbms_multi_add_result;
SELECT wrap_test.dbms_multi_pkg.subtract_value(9, 4) AS dbms_multi_subtract_result;
CALL wrap_test.dbms_multi_pkg.write_log(1002);
SELECT COUNT(*) = 1 AS dbms_multi_procedure_executed FROM wrap_test.proc_log WHERE value = 1002;
SELECT pkgsrc LIKE 'ivorysql wrapped v1%' AS dbms_multi_spec_hidden FROM pg_package WHERE lower(pkgname::text) = 'dbms_multi_pkg';
SELECT b.bodysrc LIKE 'ivorysql wrapped v1%' AS dbms_multi_body_hidden FROM pg_package_body b
JOIN pg_package p ON p.oid = b.pkgoid WHERE lower(p.pkgname::text) = 'dbms_multi_pkg';

SELECT COUNT(*) = 0 AS package_source_hidden FROM user_package_source WHERE name = 'WRAP_PKG' AND text ILIKE '%return 99%';

SELECT COUNT(*) > 0 AS package_source_wrapped FROM pg_package WHERE lower(pkgname::text) = 'wrap_pkg' AND pkgsrc ILIKE '%ivorysql wrapped%';

SELECT COUNT(*) > 0 AS package_body_source_wrapped FROM pg_package_body b JOIN pg_package p ON p.oid = b.pkgoid
WHERE lower(p.pkgname::text) = 'wrap_pkg' AND b.bodysrc ILIKE '%ivorysql wrapped%';

-- Replacing a wrapped function without WRAPPED stores and runs plaintext.
CREATE OR REPLACE FUNCTION wrap_test.replace_plain_fn RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 801;
END;
/
CREATE OR REPLACE FUNCTION wrap_test.replace_plain_fn RETURN NUMBER IS
BEGIN
  RETURN 802;
END;
/
SELECT wrap_test.replace_plain_fn() AS replaced_plain_function_result;
SELECT prosrc NOT LIKE 'ivorysql wrapped v1%' AS replaced_function_is_plaintext
FROM pg_proc
WHERE oid = 'wrap_test.replace_plain_fn()'::regprocedure;

-- A wrapped package specification and plaintext body compile independently.
CREATE OR REPLACE PACKAGE wrap_test.mixed_wrapped_spec WRAPPED IS
  FUNCTION get_value RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY wrap_test.mixed_wrapped_spec IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 811;
  END;
END;
/
SELECT wrap_test.mixed_wrapped_spec.get_value() AS wrapped_spec_plain_body_result;
SELECT p.pkgsrc LIKE 'ivorysql wrapped v1%' AS mixed_spec_is_wrapped,
       b.bodysrc NOT LIKE 'ivorysql wrapped v1%' AS mixed_body_is_plaintext
FROM pg_package p
JOIN pg_package_body b ON b.pkgoid = p.oid
WHERE lower(p.pkgname::text) = 'mixed_wrapped_spec';

-- A plaintext package specification and wrapped body compile independently.
CREATE OR REPLACE PACKAGE wrap_test.mixed_wrapped_body IS
  FUNCTION get_value RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY wrap_test.mixed_wrapped_body WRAPPED IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 812;
  END;
END;
/
SELECT wrap_test.mixed_wrapped_body.get_value() AS plain_spec_wrapped_body_result;
SELECT p.pkgsrc NOT LIKE 'ivorysql wrapped v1%' AS mixed_spec_is_plaintext,
       b.bodysrc LIKE 'ivorysql wrapped v1%' AS mixed_body_is_wrapped
FROM pg_package p
JOIN pg_package_body b ON b.pkgoid = p.oid
WHERE lower(p.pkgname::text) = 'mixed_wrapped_body';

-- Replacing both wrapped package parts without WRAPPED stores plaintext.
CREATE OR REPLACE PACKAGE wrap_test.replace_plain_pkg WRAPPED IS
  FUNCTION get_value RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY wrap_test.replace_plain_pkg WRAPPED IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 821;
  END;
END;
/
CREATE OR REPLACE PACKAGE wrap_test.replace_plain_pkg IS
  FUNCTION get_value RETURN NUMBER;
END;
/
CREATE OR REPLACE PACKAGE BODY wrap_test.replace_plain_pkg IS
  FUNCTION get_value RETURN NUMBER IS
  BEGIN
    RETURN 822;
  END;
END;
/
SELECT wrap_test.replace_plain_pkg.get_value() AS replaced_plain_package_result;
SELECT p.pkgsrc NOT LIKE 'ivorysql wrapped v1%' AS replaced_spec_is_plaintext,
       b.bodysrc NOT LIKE 'ivorysql wrapped v1%' AS replaced_body_is_plaintext
FROM pg_package p
JOIN pg_package_body b ON b.pkgoid = p.oid
WHERE lower(p.pkgname::text) = 'replace_plain_pkg';

DROP PACKAGE BODY wrap_test.wrap_pkg;
DROP PACKAGE wrap_test.wrap_pkg;
DROP PACKAGE BODY wrap_test.dbms_multi_pkg;
DROP PACKAGE wrap_test.dbms_multi_pkg;
DROP PACKAGE BODY wrap_test.direct_multi_pkg;
DROP PACKAGE wrap_test.direct_multi_pkg;
DROP PACKAGE BODY wrap_test.direct_wrapped_pkg;
DROP PACKAGE wrap_test.direct_wrapped_pkg;
DROP PROCEDURE wrap_test.wrap_proc();
DROP PROCEDURE wrap_test.direct_encrypted_proc();
DROP FUNCTION wrap_test.wrap_fn();
DROP FUNCTION wrap_test.wrap_fn_alias();
DROP FUNCTION wrap_test.wrap_special_chars(varchar2);
DROP FUNCTION wrap_test.wrap_special_block_fn(varchar2);
DROP FUNCTION wrap_test.wrap_literal_fn();
DROP FUNCTION wrap_test."Odd Fn"();
DROP PACKAGE BODY wrap_test.wrap_structure_pkg;
DROP PACKAGE wrap_test.wrap_structure_pkg;
DROP PACKAGE BODY wrap_test.mixed_wrapped_spec;
DROP PACKAGE wrap_test.mixed_wrapped_spec;
DROP PACKAGE BODY wrap_test.mixed_wrapped_body;
DROP PACKAGE wrap_test.mixed_wrapped_body;
DROP PACKAGE BODY wrap_test.replace_plain_pkg;
DROP PACKAGE wrap_test.replace_plain_pkg;
DROP FUNCTION wrap_test.replace_plain_fn();
DROP FUNCTION wrap_test.corrupt_payload_fn();
DROP FUNCTION wrap_test.invalid_body_fn();
DROP FUNCTION wrap_test.missing_dependency_fn();
DROP FUNCTION wrap_test.direct_wrapped_fn();
DROP TABLE wrap_test.proc_log;
DROP SCHEMA wrap_test;
