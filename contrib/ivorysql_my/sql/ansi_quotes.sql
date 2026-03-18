-- string as column alias
select 1 as 'string_alias';

-- TypeCast(::) will ignore ANSI_QUOTES and identify double quotes as IDENT flag always
set sql_mode = 'ANSI_QUOTES';
select 1::text, "this is a string", 1::"text", 1::text;

set sql_mode = '';
select 1::text, "this is a string", 1::"text", 1::text;
\gdesc

-- Begin - bug#6888
CREATE TABLE test_real1(col REAL);
DESC test_real1;

SET sql_mode = 'REAL_AS_FLOAT';
CREATE TEMPORARY TABLE test_real2(col REAL);
DESC test_real2;

drop table test_real1, test_real2;
-- End - bug#6888

set sql_mode = default;
