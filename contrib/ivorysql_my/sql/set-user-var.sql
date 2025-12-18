--set user define variable
set mysql.at_or_atat = on;
set @vara = 4;
select @vara;
select @vara := @vara + 2;
select @vara + 2.2;
set @vara = 4.5;
select @vara;
select @vara := @vara + 2;
select @vara + 2.2;
set @vara = length('abc');
select @vara := @vara + 2 * length('aaa');
select @vara := @vara + length('aaa') / length('aaa');
set @vara = 4, @vara = 5;
select @vara;
select @vara := @vara + 2;
select @vara + 2.2;
create table settb as select @vara;
\d settb
insert into settb values(1),(2),(3);
select * from settb;
drop table settb;
--@vara is a integer，and use this varibale to paricipate in the calculation
set @vara := 4;
select @vara;
select @vara + 1;
select @vara - 1;
select @vara * 2;
select @vara / 3;
select @vara + 1 * 3;
select @vara + 4 / 3;
select @vara - 1 * 3;
select @vara - 4 / 3;
select (@vara + 1) * 3;
select (@vara + 4) / 3;
select (@vara - 1) * 3;
select (@vara - 4) / 3;
create table settb as select @vara;
\d settb
insert into settb values(1),(2),(3);
select * from settb;
drop table settb;
--@vara is a float，and use this varibale to paricipate in the calculation
set @vara := 4.5;
select @vara;
select @vara + 1;
select @vara - 1;
select @vara * 2;
select @vara / 3;
select @vara + 1 * 3;
select @vara + 4 / 3;
select @vara - 1 * 3;
select @vara - 4 / 3;
select (@vara + 1) * 3;
select (@vara + 4) / 3;
select (@vara - 1) * 3;
select (@vara - 4) / 3;
--@vara is a float，and use this varibale to paricipate in the calculation
set @vara := 5.5;
select @vara;
select @vara + 1.6;
select @vara - 1.5;
select @vara * 2.3;
select @vara / 3.5;
select @vara + 1.8 * 3.9;
select @vara + 4.4 / 3.7;
select @vara - 1.8 * 3.9;
select @vara - 4.4 / 3.7;
select (@vara + 1.8) * 3.7;
select (@vara + 4.6) / 3.3;
select (@vara - 1.8) * 3.7;
select (@vara - 4.6) / 3.3;
create table settb as select @vara;
\d settb
insert into settb values(1.1),(2.2),(3.3);
select * from settb;
drop table settb;
--If the same user define variable is assigned twice, the last assignment will take precedence
set @varab := 4, @varab := 5;
select @varab;
select @varab + 1;
select @varab - 1;
select @varab * 2;
select @varab / 2;
select @varab + 1 * 3;
select @varab + 6 / 3;
select @varab - 1 * 3;
select @varab - 6 / 3;
select (@varab + 1) * 3;
select (@varab + 6) / 3;
select (@varab - 1) * 3;
select (@varab - 6) / 3;
create table settb as select @vara;
\d settb
insert into settb values(1.1),(2.2),(3.3);
select * from settb;
drop table settb;
--If the same user define variable is assigned twice, the last assignment will take precedence
set @varab := 4.6, @varab := 5.4;
select @varab;
select @varab + 1;
select @varab - 1;
select @varab * 2;
select @varab / 2;
select @varab + 1 * 3;
select @varab + 6 / 3;
select @varab - 1 * 3;
select @varab - 6 / 3;
select (@varab + 1) * 3;
select (@varab + 6) / 3;
select (@varab - 1) * 3;
select (@varab - 6) / 3;
create table settb as select @vara;
\d settb
insert into settb values(1.2),(2.3),(3.4);
select * from settb;
drop table settb;
--If the same user define variable is assigned twice, the last assignment will take precedence
set @varab := 5.4, @varab := 4.6;
select @varab;
select @varab + 1.5;
select @varab - 1.7;
select @varab * 2.9;
select @varab / 2.3;
select @varab + 1.5 * 3;
select @varab + 6.5 / 3;
select @varab - 1.9 * 3;
select @varab - 6.7 / 3;
select (@varab + 1.7) * 3.6;
select (@varab + 6.6) / 3.4;
select (@varab - 1.4) * 3.7;
select (@varab - 6.2) / 3.1;
--If the same user define variable is assigned twice, the last assignment will take precedence
set @varab := 4, @varab := 5;
select @varab := @varab + 2 * length('aaa');
select @varab := @varab + 5 / length('aaa');
set @varab := 4.5, @varab := 5.3;
select @varab := @varab + 2 * length('aaa');
select @varab := @varab + 5 / length('aaa');
set @varab := 5.3, @varab := 4.5;
select @varab := @varab + 2.6 * length('aaa');
select @varab := @varab + 5.1 / length('aaa');
--@vara is a integer，and use this varibale to paricipate in the calculation
set @varc := 1 + 1;
create table test(id int, flg varchar(10));
insert into test values(1, 'a'), (2, 'b'), (3, 'c'), (4, 'd');
select @varc+1*length('aaa') as rownum, * from test;
select @varc := @varc + 1 as rownum, * from test;
select * from test where id = @varc - 2;
--@vara is a integer，and use this varibale to paricipate in the calculation
set @vard := 0;
select @vard := @vard + 1 as rownum, * from test;
set @vard = 0;
select @vard := @vard + 1 as rownum, * from test;
select @vara;
select @vard := @vard + 1 as rownum, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e where @vard < 5 order by @vard;
select @vard := @vard + 1 as rownum, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e order by id desc;
select @vard;
select @vara;
select @vard, @vard := @vard + 1 as rownum, @vard, e.* from (select @vara := 1, @vard := 0)r ,test e;
select @vara;
select @vard := @vard - 1 as rownum, @vara := @vara - 2 from test e;
set @vard := 1;
select @vard := @vard + 1 as rownum, @vara := @vara - 2, e.* from test e;
set @d := length('aa');
select @vard := @vard + 1 as rownum, @vara := @vara + 1 + 1, @d = 1, e.* from test e;
select int4out(rownum1::int), * from (select @vard := @vard + 1 as rownum1, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e) f;
select @vard, @vard := @vard + 2, @vard, rownum1 from (select @vard, @vard := @vard + 1 as rownum1, @vard, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e) f;
select @vard, @vard := @vard + 2, @vard, * from (select @vard, @vard := @vard + 1 as rownum, @vard, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e) f;
select @vard, @vard := @vard + 2, @vard, * from (select @vard, @vard := @vard + 1 as rownum, @vard, @vara := @vara - 1, e.* from (select @vara := 1, @vard := 0)r ,test e) f;
--@vara is a integer，and use this varibale to paricipate in the calculation
set @vara:=0;
select @vara:=@vara+1, * from test where @vara=0;
set @varab:=0;
select @varab:=@varab+1, * from test where @varab=1;
set @varc:=0;
select * from (select @varc:=@varc+1, * from test)g where @varc=0;
--@vara is a float，and use this varibale to paricipate in the calculation
set @vard := 2;
select @vard := @vard + 1.8 as rownum, * from test;
set @vara :=  2;
select @vard := @vard + 1.2 as rownum, @vara := @vara - 1.4, e.* from (select @vara := 1.8, @vard := 0.5)r ,test e;
select @vard := @vard + 1.2 as rownum, @vara := @vara - 1.4, e.* from (select @vara := 1.8, @vard := 0.5)r ,test e order by id desc;
select @vard, @vard := @vard + 1.3 as rownum, @vard, e.* from (select @vara := 1.6, @vard := 0.3)r ,test e;
select @vard;
select @vara;
select @vard := @vard - 1.2 as rownum, @vara := @vara - 2.2, e.* from test e;
set @vard := 1.4;
select @vard := @vard + 1.2 as rownum, @vara := @vara - 2.4, e.* from test e;
set @d := length('aa');
select @vard := @vard + 1.5 as rownum, @vara := @vara + 1.1 + 1.3, @d = 1.5, e.* from test e;
select @vard:=@vard+1 from (select @vard:=@vard+2 from (select @vard:=@vard+1 as row, id from (select @vard:=0)h, test f)g)h ;
select @vard+1*2 from (select @vard:=@vard+1, id from (select @vard:=1)h, test f)g;
select @vard from (select @vard:=@vard+1, id from (select @vard:=1, @vard:=1.2)h, test f)g;
select @e := @e + 1 as rownum from (select @e := 2, @e := 3) r;
set @vara := 'aaa';
select @vara;
select length(@vara);
insert into test values (5, @vara);
select * from test where flg = @vara;
select max(rownum1) num from (select (@vard:=@vard+1) as rownum1 from test result_table, (select @vard :=0) result_num)result;
select @vara;
drop table test;
--create table thout @var
set @var = 'test';
create table settb as select @var;
\d settb
insert into settb values('a'),('ab'),('abc');
select * from settb;
drop table settb;

set @var = 'test'::varchar(10);
create table settb as select @var;
\d settb
insert into settb values('a'),('ab'),('abc');
select * from settb;
insert into settb values('abcdefghijk');
insert into settb values('abcdefghij');
select * from settb;
drop table settb;

--symbol
set @'Var' = 'abc';
select @Var, @'Var', @"var", @`Var`;
select @VAR, @'VAR', @"VAR", @`VAR`;
set @var_a = 1;
select @VAR_a;
set @var_$ = 2;
select @VAR_$;
select @VAR_$, @'VAR_$', @"VAR_$", @`VAR_$`;
select @VaR_$, @'vAR_$', @"VAr_$", @`var_$`;
set @var_11 = 9999.111;
select @var_11,  @`var_11`,  @'var_11',  @"var_11";

--Requirements document
create table taxable_transactions (id int, tax float);
insert into taxable_transactions values(1, 12);
insert into taxable_transactions values(2, 22.9);
set @name = 43;
set @total_tax = (select sum(tax) from taxable_transactions);
select @total_tax;

insert into taxable_transactions values(3, 66.5);
insert into taxable_transactions values(4, 22.12);
set @total_tax = (select sum(tax) from taxable_transactions);
select @total_tax;

set @a = @a + 1;
select @a;
set @a = 1;
select @a;

create table t(id int);
insert into t values (1),(2),(3);
set @var = 1.1;
select @var + 1, @var := 'a' from (select @var := 'a') t1, t;
drop table t;

set @abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz = 1;
select @abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz;
select @abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijk;
select @abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij; --no data

set @VAR = 1;
select @var;
select @Var;
select @vAr;
select @vaR;
select @VaR;
select @VAR;

select X'41';
set @v1 = X'41';
select @v1;

select X'41' + 0;
set @v2 = X'41' + 0;
select @v2;

set @v3 = cast(X'41' AS UNSIGNED);
select @v3;
select @v1, @v2, @v3;

set @t1 = 1, @t2 = 2, @t3 = 4;
select @t1, @t2, @t3, @t4 := @t1 + @t2 + @t3;
select @t1, @t2, @t3, @t4, @t5 = @t1 + @t2 + @t3 + @t4;
select @t3 = 2*@t2, @t4 = 2*@t2;

set @varaaa = 'aaa';
create view v_view as select @varaaa; --err
select VARIABLE_NAME, VARIABLE_VALUE from performance_schema.user_variable_by_thread;

--test explain analyze
set @varname = 'zhangsan';
create table tb_exp_analyze(id int, name text, addr text);
insert into tb_exp_analyze values (1, 'zhangsan', 'shanghai');
explain (costs off) select * from tb_exp_analyze where @varname =  name;
drop table tb_exp_analyze;

--select into @var
create table settb(id int, name text);
insert into settb values (1, 'zhangsanfeng');
select id, name into @id, @name from settb;
select @id, @name;
select @ID, @NAME;
insert into settb values (2, 'zhangcuishan');
select id, name into @id, @name from settb; --err
select @id, @name;
select @ID, @NAME;
drop table settb;

--PREPARE
--set @cretetb = 'create table settb (id int, name text)';
create table settb (id int, name text);
set @inserttb = 'insert into settb values (1, ''zhangsan'')';
set @selecttb = 'select * from settb';
set @var = 'lisi';
set @inserttbnew = 'insert into settb values (1, @var)';
--PREPARE prep1 AS @cretetb;
PREPARE prep2 AS @inserttb;
PREPARE prep3 AS @selecttb;
PREPARE prep4 AS @inserttbnew;
--EXECUTE prep1;
EXECUTE prep3;
EXECUTE prep2;
EXECUTE prep3;
EXECUTE prep2;
EXECUTE prep3;

EXECUTE prep4;
EXECUTE prep3;
drop table settb;
--user define  variable used in user function
create or replace function func(id int) returns int as $$
declare
	var int;
begin
	var = @var + func1(@var);
	return var;
end;
$$ language plmysql;

create or replace function func1(id int) returns int as $$
declare
	var int;
	var1 int;
begin
	set var = @var + id, var1 = sum(@var);
	return var;
end;
$$ language plmysql;

set @var = 1;
select func(1); --3
set @var = 2;
select func(1);	--6
set @var = 3;
select func(1);	--9

create or replace function func(id int) returns int as $$
declare
	var int;
begin
	set var = @var + id + func1(@var);
	return var;
end;
$$ language plmysql;

create or replace function func1(id int) returns int as $$
declare
	var int;
	var1 int;
begin
	set var = @var + id, var1 = sum(@var);
	return var;
end;
$$ language plmysql;

set @var = 1;
select func(1); --4
set @var = 2;
select func(1);	--7
set @var = 3;
select func(1);	--10

set mysql.at_or_atat = on;
create or replace procedure proc(id int) as $$
declare
	var int;
	var1 int;
begin
	set var = @var + id, var1 = sum(@var)*@var*id;
	raise notice 'var = %, var1 = %', var, var1;
end;
$$ language plmysql;

call proc(1);	--4 9
call proc(2);	--5 18
call proc(3);	--6 27
--set mysql.at_or_atat = off;

create table tbb (id int, name text);
insert into tbb value (1, 'zhangsan');
create or replace function funct1(id1 text) returns int as $$
declare
	var int;
	var1 text;
begin
	select tbb.id, tbb.name, id1 into var, var1, @var2 from tbb where tbb.id = 1;
	raise notice 'var:%, var1:%, @var2:%', var, var1,@var2;
	return var;
end;
$$ language plmysql;

select funct1('test');
select @var2;

drop table tb;
drop function funct1;

create or replace function funct21(id1 text) returns int as $$
declare
	var int;
	var1 text;
begin
	set var = 1;
	set @vara = id1, @varb = @vara || 'bb';
	return var;
end;
$$ language plmysql;
select funct21('test-1');
select @vara, @varb;
drop function funct21;

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_001
set @onlyletter='letter';
select @onlyletter;		-- letter
set @7643642='digit';
select @7643642;		-- digit
--set @_$__.$$..='symbol';
--select @_$__.$$..;
set @测试变量='Chinese';
select @测试变量;		--Chinese

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_002
set @let987='letteranddigit';
select @let987;	--letteranddigit
set @qw_er.a$='letterandsymbol';
select @qw_er.a$; --letterandsymbol
set @qwer测试='letterandChinese';
select @qwer测试;	--letterandChinese
set @45_80.1$9='digitandsymbol';
select @45_80.1$9;	--digitandsymbol
set @测试12354='digitandChinese';
select @测试12354; --digitandChinese
set @测_$.试='Chineseandsymbol';
select @测_$.试;	--Chineseandsymbol

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_003
set @let987测试='letadditadchn';
select @let987测试; --letadditadchn
set @qw_er.a$99='letadditasyl';
select @qw_er.a$99; --letadditasyl
set @tt_set.测$试='letadchnadsyl';
select @tt_set.测$试;	--letadchnadsyl
set @666_测$.试='ditadchnadsyl';
select @666_测$.试;		--ditadchnadsyl
set @6qaz66_测$.试='all';
select @6qaz66_测$.试;	--all

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_004
set @test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test=56;
set @te""s''``ttest=56;

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_005
set @'test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test'=56;
set @'te""s''``ttest'=56;	--err
select @'test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test';	--56
select @'te""s''``ttest';	--err
set @"test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test"=66;
set @"te""s''``ttest"=66;
select @"test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test";	--66
select @"te""s''``ttest";	--66
set @`test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test`=76;
set @`te""s''``ttest`=76;	-- null
select @`test!/~/%/^/&/(/)/-/+/=<>《》/:/{}/^/?/,*test`;	--76
select @`te""s''``ttest`;	--null

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_006
set @TESTVAR='ver1';
select @TESTVAR;	--ver1
set @testvar='ver2';
select @TESTVAR;	--ver2
set @teSTVAr='ver3';
select @TESTVAR;	--ver3

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_007
set @teSTVAr='ver3';
select @TESTVAR;     --ver3
select @testvar;     --ver3
select @TESTVAr;     --ver3

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARNAME_008
set @1234567890123456789012345678901234567890123456789012345678901234='length';
set @12345678901234567890123456789012345678901234567890123456789012345='length'; --err

--TC_IVYSQL_MYSQL_SET_CUSTOM_SCOPE_001
set @session_var1='session_var1';
set @session_var2='session_var2';
set @session_var3='session_var3';
select @session_var1, @session_var2, @session_var3;	--session_var1 session_var2 session_var3

--TC_IVYSQL_MYSQL_SET_CUSTOM_SCOPE_002
set @session_var1='session_var1';
select @session_var1;	--session_var1

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_001
set @var1=1234;
select @var1; --1234
set @var2=test; --err
set @var2=@#%; --err
set @var2=测试; --err

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_002
set @var1='1234';
select @var1;	--1234
set @var2='test';
select @var2;	--test

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_003
set @var1='1234';
select @var1;	--1234
set @var2='test';
select @var2;	--test

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_004
set @var1:='1234';
select @var1;	--1234

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_005
set @var1='1234';
select @var1;	--1234

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_006
set @var1:='1234';
select @var2:=@var1+6;	--1240
select @var2=@var1+6;	--t
select @var2=@var1+8;	--f
select @var3=@var1+8;	--null

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_007
create table tb(tid int);
insert into tb values(7),(8),(12);

set @var= 4 + 3;
select @var;	--7
set @var= (select avg(tid) from tb);
select @var; --9.0
set @var= @var+1;
select @var;	--10.0
set @var= (select avg(tid),sum(tid) from tb);	--f
set @var= (select tid from tb); --f
drop table tb;


--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_008
set @var=@aa+1;
select @var;  --null
set @var21=@var21+1;
select @var21;

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_009
set @var= NULL;
select @var; -- null
set @var= '';
select @var; --''
set @var= null;
select @var; --null

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_010
SET @var_tinyint = CAST(127 AS SIGNED),@var_int1 = CAST(255 AS UNSIGNED),@var_smallint = CAST(32767 AS SIGNED),@var_int2 = CAST(65535 AS UNSIGNED),@var_mediumint = CAST(8388607 AS SIGNED),@var_int3 = CAST(16777215 AS UNSIGNED),@var_int = CAST(2147483647 AS SIGNED),@var_int4 = CAST(-2147483648 AS SIGNED),@var_integer = CAST(4294967295 AS UNSIGNED);
select @var_tinyint,@var_int1,@var_smallint,@var_int2,@var_mediumint,@var_int3,@var_int,@var_int4,@var_integer; --127,255,32767,65535,8388607,16777215,2147483647,-2147483648 ,4294967295

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_011
SET @var_float = CAST(3.14 AS DECIMAL(10,2)),
    @var_double = CAST(3.1415926 AS DECIMAL(20,10)),
    @var_real = CAST(1.23 AS DECIMAL(15,5));
select @var_float,@var_double,@var_real;  --3.14,3.1415926000,1.23000


--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_012
create table binary_types_tb (
    bin_col BINARY(16),
    varbin_col VARBINARY(255) ,
    blob_col BLOB,
    tinyblob_col TINYBLOB,
    mediumblob_col MEDIUMBLOB,
    longblob_col LONGBLOB
) ;
insert into binary_types_tb values('A','B','C','D','E','F');

set @bin=(select bin_col from binary_types_tb),@varbin=(select varbin_col from binary_types_tb),@blob=(select blob_col from binary_types_tb),@tinyblob=(select tinyblob_col from binary_types_tb),@mediumblob=(select mediumblob_col from binary_types_tb),@longblob=(select longblob_col from binary_types_tb);
select @bin,@varbin,@blob,@tinyblob,@mediumblob,@longblob; --0x41000000000000000000000000000000|0x42|0x43|0x44|0x45|0x46 
drop table binary_types_tb;

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_013
CREATE TABLE text_types_table (
    char_col CHAR(20),
    varchar_col VARCHAR(100),
    text_col TEXT,
    tinytext_col TINYTEXT,
    mediumtext_col MEDIUMTEXT,
    longtext_col LONGTEXT
);

INSERT INTO text_types_table VALUES (
    '固定长度文本',
    '可变长度字符串示例',
    '这是标准TEXT类型内容，可存储约65KB数据',
    '小文本',
    CONCAT('中等长度文本', REPEAT('...',500)),
    CONCAT('超长文本数据', REPEAT('---',10000))
);

SET @char_var = (SELECT char_col FROM text_types_table),
    @varchar_var = (SELECT varchar_col FROM text_types_table),
    @text_var = (SELECT text_col FROM text_types_table),
    @tinytext_var = (SELECT tinytext_col FROM text_types_table),
    @mediumtext_var = (SELECT mediumtext_col FROM text_types_table),
    @longtext_var = (SELECT longtext_col FROM text_types_table);

SELECT
    @char_var,@varchar_var,LEFT(@text_var, 20) AS text_sample,@tinytext_var,LEFT(@mediumtext_var, 30) AS mediumtext_sample,LEFT(@longtext_var, 40) AS longtext_sample;  --固定长度文本|可变长度字符串示例|这是标准TEXT类型内容，可存储约65K|小文本|中等长度文本........................| 超长文本数据----------------------------------
drop table text_types_table;

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_014
CREATE TABLE time_data_types (
    date_col DATE,
    datetime_col DATETIME,
    timestamp_col TIMESTAMP,
    time_col TIME,
    year_col YEAR(4)
);-- 插入测试数据
INSERT INTO time_data_types VALUES (     '2025-07-28',      '2025-07-28 17:30:00',      '2025-07-28 17:30:00.000000',     '17:30:00',      2025 );

SET @date_var = (SELECT date_col FROM time_data_types),
    @datetime_var = (SELECT datetime_col FROM time_data_types),
    @timestamp_var = (SELECT timestamp_col FROM time_data_types),
    @time_var = (SELECT time_col FROM time_data_types),
    @year_var = (SELECT year_col FROM time_data_types);
select @date_var,@datetime_var,@timestamp_var,@time_var,@year_var ; --2025-07-28 | 2025-07-28 17:30:00 | 2025-07-28 17:30:00 | 17:30:00  |      2025
drop table time_data_types;


--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_017
set @int_num=2,@str_num ='25.00';
set @result1=@int_num *@str_num;
select @result1;	--50
set @result2=@int_num < @str_num;
select @result2;	--t

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_018
set @int_num=2,@str_num ='25.00';
set @result=CONCAT('One is ', @int_num,',Another is ',@str_num);
select @result;		--One is 2,Another is 25.00

--TC_IVYSQL_MYSQL_SET_CUSTOM_VARVALUES_019
--SET @flag = TRUE, @str_flag = '1';
--SET  @is_equal=(@flag = @str_flag);	--succ
--SELECT @is_equal;	--t
--SET @flag_f = FALSE, @str_flag_f= '0';
--SET  @is_equal=(@flag_f = @str_flag_f); 	--succ
--SELECT @is_equal;	--t
--SET  @is_equal=(@flag = @str_flag_f);
--SELECT @is_equal;	--f

--TC_IVYSQL_MYSQL_SET_CONFIG_PART_001
CREATE FUNCTION calculate_discount(price DECIMAL(10,2), discount_rate DECIMAL(5,2))
RETURNS DECIMAL(10,2) AS $$
DECLARE
 final_price DECIMAL(10,2);
 rate DECIMAL(10,2);
BEGIN
    SET rate := 1 - discount_rate;
    SET final_price = price * rate;
    RETURN final_price;
END $$ language plmysql;
select calculate_discount(700,0.2);	--560.00
set @price1=800;
select calculate_discount(@price1,0.5);	--400
drop function calculate_discount;

--TC_IVYSQL_MYSQL_SET_CONFIG_PART_002
CREATE PROCEDURE calculate_order_prices(
    IN order_counts INT,
    IN discount_rate DECIMAL(5,2),
    OUT final_price DECIMAL(10,2)
)
AS $$
DECLARE
 unit_price DECIMAL(10,2) DEFAULT 399.00;
 total_price DECIMAL(10,2);
BEGIN
    SET total_price = unit_price * order_counts;
    SET final_price := total_price * (1 - discount_rate/100);
END $$ language plmysql;
CALL calculate_order_prices(5, 10, @fin_pric);
select @fin_pric;	--1795.50
set @counts_num=20;
CALL calculate_order_prices(@counts_num, 10, @fin_pric2);
select @fin_pric2;	--7182.00
DROP PROCEDURE calculate_order_prices;

--TC_IVYSQL_MYSQL_SET_CONFIG_PART_003
CREATE FUNCTION add_numbers(a INT)
RETURNS INT AS $$
DECLARE
 result INT;
BEGIN
    SET result = a + b;
    RETURN result;
END  $$ language plmysql;
select add_numbers(2); --未知列名b

--TC_IVYSQL_MYSQL_SET_CONFIG_PART_004
CREATE FUNCTION return_result(a INT)
RETURNS INT
AS $$
    DECLARE result INT;
BEGIN
    SET result =  CONCAT('result is ', a);
    RETURN result;
END $$ language plmysql;
select return_result(4); --不正确的整数值
drop function return_result;

--fix bug 6816
drop table if exists t;
create table t(a int);
insert into t values (1), (2), (4);
select @var100, @var100 := @var102 / num, @var102 := @var102 + 1 from (select @var100 := a as num from t) tt;
drop table t;


--fix bug 6860
set @x4 = 5, @x5 = @x4 - 1;
select @x4, @x5; --5 NULL

set @var = 1;
set @var = @var + 1, @var2 = @var;
select @var, @var2; --2 1

--fix bug 6860
create table tb(tid int);
insert into tb values(7.0),(8.0),(12.0);
set @var = cast((select avg(tid) from tb) as unsigned);
select @var;
set @var = @var + 1;
select @var;

drop table tb;

set mysql.at_or_atat = off;