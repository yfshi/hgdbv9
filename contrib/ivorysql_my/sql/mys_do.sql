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

-- fix bug#Z6200
set mysql.at_or_atat = 1;
do
@temperature := 25.5,
@weather := IF(@temperature > 30, 'Hot',
  IF(@temperature > 20, 'Warm',
  IF(@temperature > 10, 'Cool', 'Cold')));

select @weather; --Warm

-- fix bug#Z6201
do @counter := 0,
@counter := @counter + 1,
@counter := @counter * 10,
@counter := @counter + 5;

select @counter; --15

-- fix bug#Z6202
do  @a := 10 + 5;
select @a as addition_result; --15

do @x := 100, @y := 50, @z := @x - @y;
select @x, @y, @z; --50

do @result := (15 * 3) + (20 / 4) - POWER(2, 3);
select @result as complex_calculation; --42

set mysql.at_or_atat = 0;
set mysql.use_mysql_do = false;