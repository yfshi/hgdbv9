set mysql.use_mysql_do = true; 

do 1;
do 1+1;
do 1+1,1-1;
do hex(1234);

create table t1 (a int);
select * from t1 where a=2;
do (select * from t1 where a=2);
insert into t1 values(2);
select * from t1 where a=2;
do (select * from t1 where a=2);

create procedure v1()
language plpgsql as 
$$
declare
area integer := 0;
begin
area :=1+1; 
end;
$$;
do v1();
call v1();

create procedure v2()
language plpgsql as 
$$
declare
area integer := 0;
begin
area :=1+1; 
do 1+1; 
end;
$$;
do v2();
call v2();

drop procedure v1(),v2();

create function testf1() returns int 
language plpgsql
as $$
begin
do hex(123);
return 1;
end;
$$;

select testf1();
do testf1();
drop function testf1();

set mysql.use_mysql_do = false; 