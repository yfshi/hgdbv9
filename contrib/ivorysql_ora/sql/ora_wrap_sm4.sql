CREATE SCHEMA wrap_sm4_test;

SET ivorysql.wrap_algo TO sm4;
SHOW ivorysql.wrap_algo;

SELECT dbms_ddl.wrap('create or replace function wrap_sm4_test.wrap_sm4_probe return number is begin return 4; end;') LIKE '%''sm4:%' AS dbms_ddl_wrap_sm4;

CREATE OR REPLACE FUNCTION wrap_sm4_test.direct_sm4_fn RETURN NUMBER WRAPPED IS
BEGIN
  RETURN 712;
END;
/

SELECT wrap_sm4_test.direct_sm4_fn() AS sm4_function_result;
SELECT prosrc LIKE 'ivorysql wrapped v1%sm4:%' AS direct_sm4_fn_hidden
FROM pg_proc
WHERE oid = 'wrap_sm4_test.direct_sm4_fn()'::regprocedure;

SET ivorysql.wrap_algo TO aes;
SELECT wrap_sm4_test.direct_sm4_fn() AS sm4_after_aes_setting;
\echo sm4_wrap_test_complete
