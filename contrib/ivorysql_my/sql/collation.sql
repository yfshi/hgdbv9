CREATE DATABASE mys_db_test encoding=:charset LC_COLLATE='C' LC_CTYPE ='C' TEMPLATE=template0;
\c mys_db_test

CREATE OR REPLACE FUNCTION default_collation_test(text)
RETURNS text
AS $$
DECLARE
    default_collation text;
    ret text;
BEGIN
    select "Default collation" from show_mysql_charsets() where charset=$1 into default_collation;
    
    execute format ($y$ create table test(val text collate %s) $y$, default_collation);

    insert into test values ('a');
    insert into test values ('A');

    select count(*) from test where val = 'a' into ret;
    
    drop table test;

    return ret;
END
$$
LANGUAGE PLPGSQL;

CREATE OR REPLACE FUNCTION bin_collation_test(text)
RETURNS text
AS $$
DECLARE
    default_collation text;
    ret text;
BEGIN
    
    execute format ($y$ create table test(val text collate %s_bin) $y$, $1);

    insert into test values ('a');
    insert into test values ('A');

    select count(*) from test where val = 'a' into ret;
    
    drop table test;

    return ret;
END
$$
LANGUAGE PLPGSQL;

select bin_collation_test(:'charset');
\qecho ''
select default_collation_test(:'charset');

-- clean
DROP FUNCTION default_collation_test;
\c postgres
DROP DATABASE mys_db_test;
\qecho '\n\n'
