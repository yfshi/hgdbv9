create extension sql_sentinel;

--set paramters
ALTER SYSTEM SET sql_sentinel.sql_injection = on;

select  schemaname, funcname AS has_oid from hg_security.sql_sentinel_sentive;
SELECT hg_security.sql_sentinel_delete_sentive('pg_catalog', 'pg_sleep(double precision)');

select pg_reload_conf();
select pg_sleep(2);

show sql_sentinel.intercept_target;

--6 common sql
SELECT '正常查询通过' AS result;

--target list not
SELECT 2 = 1 + 1 AS ordinary_constant_expression;

--all false
SELECT 1 AS no_rows_expected WHERE 2 = 3;

--fail: match constant_tautology
SELECT 1 WHERE false OR 2 = 1 + 1;

--fail: match sensitive_function
SELECT pg_read_file('postgresql.conf', 0, 10);

--success:not match any sensitive function
select length('abc');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'length(text)');


--fail:match length
select length('abc');

--fail:match length
DO $$
BEGIN
	FOR i IN 1..100 LOOP
	PERFORM length('abcd');
	END LOOP;
END $$;

create table t1 (id int);
insert into t1 values(1);

--fail:match length
update t1 set id=2 where length('abcd') = 4;

--fail: match stacked_query
select 1\; select 2;

ALTER SYSTEM SET sql_sentinel.intercept_target = "taut_const, sensitive_fun, stack_qry, catalog, union, comment";
select pg_reload_conf();

--fail: match constant_tautology,comment
SELECT 1 WHERE false OR 'a' = 'a' /* 模拟注释截断特征 */;

--fail: have comment in sql
SELECT 1 WHERE false /* 模拟注释截断特征 */;

--fail: use union and system_catalog
select tablename, 'table' as type from pg_tables union select viewname, 'view' from pg_views;

--fail: match system_catalog
select relname from pg_class;

--xss test

ALTER SYSTEM SET sql_sentinel.xss_detection = on;
ALTER SYSTEM SET sql_sentinel.xss_columns = 'public.sql_security_v01_test.plain_text=plain_text,public.sql_security_v01_test.rich_html=safe_html';
ALTER SYSTEM SET sql_sentinel.xss_unsafe_tags = '';  --example: 'p,strong,em,b,i,u,a,ul,ol,li,blockquote,code,pre,br'

select pg_reload_conf();
select pg_sleep(2);

CREATE TABLE public.sql_security_v01_test
(
    id          integer PRIMARY KEY,
    plain_text  text,
    rich_html   text
);

-- plain_text not include HTML. rich_html only allowed babel and secure URL, should be written successfully
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES
(
    1,
    'Alice',
    '<p>Hello <strong>world</strong> <a href="https://example.com">link</a></p>'
);

UPDATE public.sql_security_v01_test
   SET plain_text = 'Alice Updated'
 WHERE id = 1;

ALTER SYSTEM SET sql_sentinel.xss_unsafe_tags = 'strong';  --example: 'p,strong,em,b,i,u,a,ul,ol,li,blockquote,code,pre,br'
select pg_reload_conf();
select pg_sleep(2);

-- fail:tags 'strong' in xss_unsafe_tags
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES
(
    1,
    'Alice',
    '<p>Hello <strong>world</strong> <a href="https://example.com">link</a></p>'
);

-- fail:tags 'p' in xss_unsafe_tags
UPDATE public.sql_security_v01_test
   SET plain_text = 'Alice Updated'
 WHERE id = 1;


--fail：plain_text not allowed HTML
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (2, '<b>Alice</b>', '<p>安全内容</p>');

-- fail：img is not in the default safe tags list, and it also contains event handler attributes.
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (3, 'Alice', '<img src="x" onerror="alert(1)">');

--success: not html is allowed
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (3, 'Alice', '<img src="x" o');

-- fail：allowed p tags in onclick event handler attribute
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (4, 'Alice', '<p onclick="alert(1)">点击</p>');

-- fail：href use javascript , dangerous
INSERT INTO public.sql_security_v01_test(id, plain_text, rich_html)
VALUES (5, 'Alice', '<a href="javascript:alert(1)">点击</a>');

-- fail：UPDATE
UPDATE public.sql_security_v01_test
SET plain_text = '<script>alert(1)</script>'
WHERE id = 1;

--clean data
DROP TABLE public.sql_security_v01_test;

SET sql_sentinel.max_sql_length = 100;
show sql_sentinel.max_sql_length;

SET sql_sentinel.max_sql_length = '1kB';
show sql_sentinel.max_sql_length;

SET sql_sentinel.max_sql_length = '1MB';
show sql_sentinel.max_sql_length;

SET sql_sentinel.max_sql_length = '1GB';
show sql_sentinel.max_sql_length;

ALTER SYSTEM SET sql_sentinel.intercept_target = "taut_const, sensitive_fun, stack_qry";
select pg_reload_conf();

-- Create two overloads to exercise name/argument-type handling.
CREATE FUNCTION my_func(integer) RETURNS integer
LANGUAGE sql IMMUTABLE AS 'SELECT $1';
CREATE FUNCTION my_func(text) RETURNS text
LANGUAGE sql IMMUTABLE AS 'SELECT $1';

-- Insert into hg_security.sql_sentinel_sentive; the OID is resolved from schemaname + funcname(args).
SELECT hg_security.sql_sentinel_insert_sentive('public', 'my_func(integer)');
SELECT hg_security.sql_sentinel_insert_sentive('public', 'my_func(text)');

SELECT schemaname, funcname, f_oid IS NOT NULL AS has_oid
FROM hg_security.sql_sentinel_sentive ORDER BY funcname;

-- f_oid must resolve to a matching pg_proc entry.
SELECT count(*) AS oid_matches
FROM hg_security.sql_sentinel_sentive t
JOIN pg_proc p ON p.oid = t.f_oid
JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE t.schemaname = n.nspname
  AND p.proname = 'my_func';

-- Inserting an unknown function must raise undefined_function.
DO $$
BEGIN
    PERFORM hg_security.sql_sentinel_insert_sentive('public', 'no_such_func(integer)');
    RAISE EXCEPTION 'unexpected success';
EXCEPTION WHEN undefined_function THEN
    RAISE NOTICE 'unknown function correctly rejected';
END;
$$;

-- Explicit delete removes only the requested overload.
SELECT hg_security.sql_sentinel_delete_sentive('public', 'my_func(integer)');
SELECT count(*) AS rows_after_delete FROM hg_security.sql_sentinel_sentive;

-- Dropping a function triggers the hook and auto-deletes its hg_security.sql_sentinel_sentive row.
DROP FUNCTION my_func(text);
SELECT count(*) AS rows_after_drop FROM hg_security.sql_sentinel_sentive;

insert into hg_security.sql_sentinel_sentive values('pg_catalog', 'pg_read_file(text)', 2624);
update hg_security.sql_sentinel_sentive set f_oid=2625 where f_oid=2624;