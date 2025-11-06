--
-- MYSQL SELECT
--

--test dual
select 1 + 1 from dual;
select * from dual;

create table sales (product varchar(50), region varchar(50), amount decimal(10,2));
insert into sales (product, region, amount) values ('ProductA', 'North', 100),('ProductA', 'South', 150),('ProductB', 'North', 200),('ProductB', 'South', 250);
select * from sales union select * from dual;
select * from dual, sales;

--with rollup
select product, region,SUM(amount) AS total_amount from sales group by product, region with rollup;
select product, region,SUM(amount) AS total_amount from sales group by product, region, amount with rollup;

--group by desc/asc
select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc;
select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc order by product asc, region desc;


--having without group by
select * from sales having amount > 100;
select * from sales where product = 'ProductA' having amount > 0;
select sum(amount) from sales having  SUM(amount) > 10;

--view test
create view dual_v as select 1+1 from dual;
select * from dual_v;
create view with_rollup_v as select product, region,SUM(amount) AS total_amount from sales group by product, region with rollup;
select * from with_rollup_v;
create view groupby_v as select product, region,SUM(amount) AS total_amount from sales group by product desc, region asc;
select * from groupby_v;
create view having_v as select * from sales having amount > 100;
select * from having_v;

drop view dual_v;
drop view with_rollup_v;
drop view groupby_v;
drop view having_v;
DROP TABLE sales;
