set mysql.at_or_atat = on;
--set global variable
set GLOBAL datestyle = 'ISO, YMD', GLOBAL lower_case_table_names = 1, GLOBAL mysql.at_or_atat = false;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat;
set GLOBAL mysql.at_or_atat = true;
set @@GLOBAL.datestyle = 'ISO, YMD', @@GLOBAL.lower_case_table_names = 0, @@GLOBAL.mysql.at_or_atat = true;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat;

--set session variable
set SESSION datestyle = 'ISO, YMD', SESSION lower_case_table_names = 1, SESSION mysql.at_or_atat = false;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat;
set SESSION mysql.at_or_atat = true;
set @@SESSION.datestyle = 'ISO, DMY', @@SESSION.lower_case_table_names = 0, @@SESSION.mysql.at_or_atat = true;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat;
set datestyle = 'ISO, YMD', lower_case_table_names = 1, mysql.at_or_atat = false;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat;
set mysql.at_or_atat = true;
set @@LOCAL.datestyle = 'ISO, DMY', @@LOCAL.lower_case_table_names = 0, @@LOCAL.mysql.at_or_atat = true;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat ;
set LOCAL datestyle = 'ISO, YMD', LOCAL lower_case_table_names = 1, LOCAL mysql.at_or_atat = false;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat ;
set GLOBAL datestyle = 'ISO, YMD', lower_case_table_names = 1, LOCAL mysql.at_or_atat = false;
show datestyle;
show lower_case_table_names;
show mysql.at_or_atat ;
set mysql.at_or_atat = true;
set @x = 1, GLOBAL datestyle = 'ISO, YMD',lower_case_table_names = 1;
select @x;
show datestyle;
show lower_case_table_names;
set LOCAL mysql.at_or_atat = true;
show mysql.at_or_atat;

set @@GLOBAL.datestyle =  @@SESSION.datestyle;
set @@GLOBAL.datestyle =  @@LOCAL.datestyle;
set @@GLOBAL.datestyle =  @@GLOBAL.datestyle;
set @@SESSION.datestyle =  @@GLOBAL.datestyle;
set @@SESSION.datestyle =  @@LOCAL.datestyle;
set @@SESSION.datestyle =  @@SESSION.datestyle;
set @@LOCAL.datestyle =  @@GLOBAL.datestyle;
set @@LOCAL.datestyle =  @@SESSION.datestyle;
set @@LOCAL.datestyle =  @@LOCAL.datestyle;

set LOCAL datestyle = default, LOCAL lower_case_table_names = default, LOCAL mysql.at_or_atat = default;
set mysql.at_or_atat = true;
set @@LOCAL.datestyle = default, @@LOCAL.lower_case_table_names = default, @@LOCAL.mysql.at_or_atat = default;
set mysql.at_or_atat = true;
set SESSION datestyle = default, SESSION lower_case_table_names = default, SESSION mysql.at_or_atat = default;
set mysql.at_or_atat = true;
set @@SESSION.datestyle = default, @@SESSION.lower_case_table_names = default, @@SESSION.mysql.at_or_atat = default;
set mysql.at_or_atat = true;
set GLOBAL datestyle = default, GLOBAL lower_case_table_names = default, GLOBAL mysql.at_or_atat = default;
set mysql.at_or_atat = true;
set @@GLOBAL.datestyle = default, @@GLOBAL.lower_case_table_names = default, @@GLOBAL.mysql.at_or_atat = default;

create table settb(id int, state text);
insert into settb values (1, 'off'),(2, 'on');
set mysql.at_or_atat = (select state from settb where id = 1);
show mysql.at_or_atat;
set mysql.at_or_atat = (select state from settb where id = 2);
show mysql.at_or_atat;
set mysql.at_or_atat = off;
drop table settb;

create table settb(id int, state bool);
insert into settb values (1, false),(2, true);
set mysql.at_or_atat = (select state from settb where id = 1);
show mysql.at_or_atat;
set mysql.at_or_atat = (select state from settb where id = 2);
show mysql.at_or_atat;
set @@GLOBAL.mysql.at_or_atat = (select state from settb where id = 1);
show mysql.at_or_atat;
set mysql.at_or_atat = on;
set @@SESSION.mysql.at_or_atat = (select state from settb where id = 1);
show mysql.at_or_atat;
set mysql.at_or_atat = on;
set @@LOCAL.mysql.at_or_atat = (select state from settb where id = 1);
show mysql.at_or_atat;
drop table settb;


set mysql.at_or_atat = on;
set datestyle = (concat('ISO', ', DMY'));
show datestyle;
set @@LOCAL.datestyle = (concat('ISO', ', YMD'));
show datestyle;
set @@SESSION.datestyle = (concat('ISO', ', DMY'));
show datestyle;
set @@GLOBAL.datestyle = (concat('ISO', ', YMD'));
show datestyle;
reset datestyle;

set lower_case_table_names = (0+1);
show lower_case_table_names;
set lower_case_table_names = (1-1);
show lower_case_table_names;


--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_002
set @@GLOBAL.max_sort_length=5;
set @@GLOBAL.work_mem='8MB';
select @@max_sort_length,@@work_mem;	--5, work_mem 8MB
select @@global.max_sort_length, @@global.work_mem;  --max_sort_length 5, work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_003
set GLOBAL test_guc=2;  --err test_guc
set GLOBAL test_guc=2;  --err test_guc

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_004
set @@GLOBAL.max_sort_length=8;
set @@GLOBAL.max_sort_length=5;
set @@GLOBAL.work_mem='5MB';
set @@GLOBAL.work_mem='8MB';

select @@global.max_sort_length, @@global.work_mem;	-- max_sort_length 5, work_mem 8MB
set @@GLOBAL.max_sort_length=DEFAULT;
set @@GLOBAL.work_mem=DEFAULT;
select @@global.max_sort_length, @@global.work_mem;	-- max_sort_length 1024, work_mem 4MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_005
set SESSION sql_mode='';
set SESSION work_mem='8MB';
select @@sql_mode, @@work_mem;	 --sql_mode '', work_mem 8MB
select @@global.sql_mode, @@global.work_mem;	--sql_mode '', work_mem 8MB
select @@sql_mode, @@work_mem, @@global.sql_mode,@@global.work_mem;	--'', work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_006
set LOCAL sql_mode='';
set LOCAL work_mem='8MB';
select @@sql_mode, @@work_mem;	--sql_mode '', work_mem 8MB
select @@global.sql_mode;,@@global.work_mem;	--sql_mode'', work_mem 8MB
select @@sql_mode,@@work_mem, @@global.sql_mode,@@global.work_mem;  --'', work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_007
set @@SESSION.sql_mode='';
set @@SESSION.work_mem='8MB';
select @@sql_mode,@@work_mem;	--sql_mode '', work_mem 8MB
select @@global.sql_mode,@@global.work_mem;	--sql_mode '', work_mem 8MB
select @@sql_mode, @@work_mem, @@global.sql_mode, @@global.work_mem;--sql_mode''、work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_008
set @@LOCAL.sql_mode='';
set @@LOCAL.work_mem='8MB';
select @@sql_mode,@@work_mem;	--sql_mode '', work_mem 8MB
select @@global.sql_mode,@@global.work_mem;	--sql_mode '', work_mem 8MB
select @@sql_mode,@@work_mem,@@global.sql_mode,@@global.work_mem;	-- sql_mode '', work_mem 8MB
select @@sql_mode,@@work_mem,@@global.sql_mode,@@global.work_mem;	--sql_mode'', work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_009
set @@sql_mode='';
set @@work_mem='8MB';
select @@sql_mode,@@work_mem;	--sql_mode '', work_mem 8MB
select @@global.sql_mode,@@global.work_mem;	--'', work_mem 8MB
select @@sql_mode,@@work_mem,@@global.sql_mode,@@global.work_mem;	--sql_mode'', work_mem 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_010
set sql_mode='';
set work_mem='8MB';
select @@sql_mode,@@work_mem;	--sql_mode '', work_mem 8MB
select @@global.sql_mode,@@global.work_mem;	--sql_mode'', work_mem 8MB
select @@sql_mode,@@work_mem,@@global.sql_mode,@@global.work_mem;	--sql_mode'', 8MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_011
set sql_mode='';
set work_mem='8MB';
set sql_mode=default;
set work_mem=default;
select @@sql_mode,@@work_mem;	--sql_mode'', work_mem 4MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_012
set @var=1,@@sql_mode='';
set @var=1 @@sql_mode='';  --err
set @var=1:/#/%/&/**/-/@@sql_mode='';  --err

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_015
set @@global.sql_mode='',@var='test123',GLOBAL work_mem='8MB';
select @@global.sql_mode,@var,@@global.work_mem; --@@global.sql_mode:'', @var:test123, @@global.work_mem:8MB
set @@max_sort_length=8,@var='test1234',@@work_mem='2MB';
select @@max_sort_length,@var,@@work_mem; --@@max_sort_length:8, @var:test1234, @@global.work_mem:2MB
set @@global.max_sort_length=10,@var='test12345',@@work_mem='5MB';
select @@global.max_sort_length,@var,@@work_mem; --@@global.max_sort_length:10, @var:test12345, @@work_mem:5MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_016
set @@sql_mode='',SESSION work_mem='8MB';
select @@sql_mode,@@work_mem;	--'', 8MB
set @@global.max_sort_length=8,@@work_mem='2MB';
select @@global.max_sort_length,@@work_mem; --8, 2MB
set @@global.max_sort_length=10,@@global.work_mem='5MB';
select @@global.max_sort_length,@@global.work_mem; --10, 5MB

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_017
set SESSION sql_mode='', max_sort_length='5',div_precision_increment=5;
select @@sql_mode,@@max_sort_length,@@div_precision_increment,@@global.sql_mode,@@global.max_sort_length,@@global.div_precision_increment; --'', 5, 5, '', 5, 5

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_018
set SESSION sql_mode='', GLOBAL work_mem='6MB',max_sort_length=5;
select @@sql_mode,@@max_sort_length,@@work_mem,@@global.sql_mode,@@global.work_mem,@@global.max_sort_length; --'', 5, 6MB, '', 6MB, 5
set GLOBAL sql_mode='', SESSION work_mem='6MB',max_sort_length=5;
select @@sql_mode,@@max_sort_length,@@work_mem,@@global.sql_mode,@@global.work_mem,@@global.max_sort_length; --'', 5, 6MB, '', 6MB, 5

--TC_IVYSQL_MYSQL_SET_CONFIG_PARAM_019
set @@GLOBAL.sql_mode='',max_sort_length= 5;
select @@max_sort_length,@@global.max_sort_length; -- 5
set @@SESSION.sql_mode='',max_sort_length= 6;
select @@max_sort_length,@@global.max_sort_length; -- 6
set @@sql_mode='',max_sort_length=8;
select @@max_sort_length,@@global.max_sort_length; -- 8
set @@GLOBAL.sql_mode='',@@SESSION.work_mem='6MB',@@div_precision_increment=5,max_sort_length=DEFAULT;
select @@max_sort_length,@@global.max_sort_length; -- 1024

set mysql.at_or_atat = off;

--fix bug 5838
set sql_mode=default;
reset sql_mode;