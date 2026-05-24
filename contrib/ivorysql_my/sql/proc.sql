set mysql.at_or_atat = true;
set mysql.default_function_security_is_definer = true;
set mysql.create_function_with_sql_mode = true;

CREATE USER proc_user WITH PASSWORD 'ivorysql@123';
GRANT ALL ON SCHEMA public, mysql TO proc_user;
SET ROLE proc_user;

-- comment
CREATE OR REPLACE FUNCTION comment_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
LANGUAGE SQL;

\df+ comment_test

-- deterministic
-- default is not deterministic(volatile)
CREATE OR REPLACE FUNCTION deterministic_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
LANGUAGE SQL;

\df+ deterministic_test

-- set to immutable
CREATE OR REPLACE FUNCTION deterministic_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
deterministic
LANGUAGE SQL;

\df+ deterministic_test

-- set to volatile
CREATE OR REPLACE FUNCTION deterministic_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
not deterministic
LANGUAGE SQL;

\df+ deterministic_test

-- security
-- default is definer
CREATE OR REPLACE FUNCTION security_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
LANGUAGE SQL;

\df+ security_test

-- set to definer
CREATE OR REPLACE FUNCTION security_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
sql security definer
LANGUAGE SQL;

\df+ security_test

-- set to innoker
CREATE OR REPLACE FUNCTION security_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
sql security invoker
LANGUAGE SQL;

\df+ security_test

-- operator keywords
CREATE OR REPLACE FUNCTION operator_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
no sql contains sql reads sql data modifies sql data
sql security invoker
LANGUAGE SQL;

\df+ operator_test

-- definer
-- default is CURRENT_USER
CREATE OR REPLACE 
DEFINER = CURRENT_USER
FUNCTION definer_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
no sql contains sql reads sql data modifies sql data
sql security invoker
LANGUAGE SQL;

select proname from pg_proc where proname = 'definer_test' and proowner = current_userid();

-- set to CURRENT_USER
CREATE OR REPLACE 
DEFINER = CURRENT_USER
FUNCTION definer_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
no sql contains sql reads sql data modifies sql data
sql security invoker
LANGUAGE SQL;

select proname from pg_proc where proname = 'definer_test' and proowner = current_userid();

-- set to CURRENT_USER()
CREATE OR REPLACE 
DEFINER = CURRENT_USER()
FUNCTION definer_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
no sql contains sql reads sql data modifies sql data
sql security invoker
LANGUAGE SQL;

select proname from pg_proc where proname = 'definer_test' and proowner = current_userid();

CREATE OR REPLACE 
DEFINER = 'proc_user'@'%'
FUNCTION definer_test(bool)
RETURNS int
AS 'select !($1::int);'
comment 'ths is comment'
no sql contains sql reads sql data modifies sql data
sql security invoker
LANGUAGE SQL;

\df+ definer_test

-- mysql.proc
desc mysql.proc;

-- show create function
show create function comment_test;
show create function deterministic_test;
show create function security_test;
show create function operator_test;

-- show create procedure
show create procedure comment_test;
show create procedure deterministic_test;
show create procedure security_test;
show create procedure operator_test;

create or replace procedure proc_test()
as
$$
declare
i int := 1000;
j int := 1;
k int := 1;
begin
    while i <= 1200 loop
	j = 1;
		while j <=12 loop 
		k = 1;
			while k <= 28 loop
				k = k+1;
			end loop;
			j = j+1;
		end loop;
    i = i+1;
    end loop;
end;
$$ language plmysql;

show create function proc_test;
show create procedure proc_test;

-- user define variable argument test
-- user define variable can be any type
create or replace procedure arg_test(IN arg1 text, OUT arg2 text, INOUT arg3 text)
as $$
begin
	set arg1 = concat(arg1, '-1');
	set arg2 = concat(arg2, '-2');
	set arg3 = concat(arg3, '-3');
end;
$$ language plmysql;

-- case 1
set @var1 = 'a';
set @var2 = 'b';
set @var3 = 'c';

-- a, null, c-3
call arg_test(@var1, @var2, @var3);

-- a, null, c-3
select @var1, @var2, @var3;

-- case 2
set @var1 = 1::text;
set @var2 = 2::text;
set @var3 = 3::text;

-- 1, null, 3-3
call arg_test(@var1, @var2, @var3);

-- 1, null, 3-3
select @var1, @var2, @var3;

-- case 3
set @var1 = 1;
set @var2 = 2;
set @var3 = 3;

-- 1, null, 3-3
call arg_test(@var1::text, @var2::text, @var3::text);

-- 1, null, 3
select @var1, @var2, @var3;

-- system variable can only be IN-type
call arg_test(@var1::text, @@autocommit, @var3::text);
call arg_test(@var1::text, @var2::text, @@autocommit);

-- test sql_mode
set sql_mode = 'ansi_quotes';
create or replace function sql_mode_func1()
returns text
as $$
declare
	ret text;
begin
	select current_setting('sql_mode') into ret;
	return ret;
end;
$$ language plmysql;

set sql_mode = 'pipes_as_concat';
create or replace function sql_mode_func2()
returns text
as $$
declare
	ret text;
begin
	select current_setting('sql_mode') into ret;
	return ret;
end;
$$ language plmysql;

set sql_mode = 'only_full_group_by';
create or replace function sql_mode_func3()
returns text
as $$
declare
	ret text;
begin
	select current_setting('sql_mode') into ret;
	return ret;
end;
$$ language plmysql;

set sql_mode = '';
select sql_mode_func1(), sql_mode_func2(), sql_mode_func3();
show sql_mode;

set sql_mode = 'no_backslash_escapes';
select sql_mode_func1(), sql_mode_func2(), sql_mode_func3();
show sql_mode;

set sql_mode = default;

-- finish
set mysql.at_or_atat = false;
set mysql.default_function_security_is_definer = false;
set mysql.create_function_with_sql_mode = false;