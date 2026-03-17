--
-- (+) outer join testcase
--
create table t1 (i int, j int);
insert into t1 values (1,2);
insert into t1 values (2,2);
insert into t1 values (3,1);
insert into t1 values (null, null);
create table t2 as select * from t1;
create table t3 as select * from t1;
create table t4 as select * from t1;
create table t5 as select * from t1;
create table t6 as select * from t1;
create table t7 as select * from t1;
create table t8 as select * from t1;
create table t9 as select * from t1;

--simple testcase
select * from t1, t2 where t1.i = t2.i(+);
select * from t1, t2 where t2.i(+)= 2;--no join
select * from t1,t2 where t1.i = t2.i(+) + t2.j(+);
select * from t1, t2 where t1.i + t2.i(+) = 1;
select * from t1,t2 where t1.i(+) = t1.j(+);--ok
select * from t1,t2,t3,t4,t5 where t1.i+t2.i(+) = t3.i+t4.i+t5.i;
select * from t1,t2 where t2.i = t1.i(+)+t2.j;--dedumplicate join t2
select * from t1 t,t1 tt where t.i(+) = tt.j;
select * from t1, t2 where t2.i= 1 and t1.i(+) =1 and t2.i(+)=t2.i(+);--bug#z956

--multi AND testcase
select * from t1,t2,t3,t4,t5 where t1.i(+)+t2.i = t3.i and t4.i(+)= t2.i + t5.i;
select * from t1,t2,t3,t4,t5,t6,t7 where t1.i(+)= t2.i+t3.i and t2.i(+) = t4.i+t5.i and t5.i(+) = t6.i+t7.i;
select * from t1,t2,t3,t4,t5,t6,t7 where t1.i(+)= t2.i and t2.i(+) = t3.i and  t3.i(+) = t4.i and t5.i(+) = t6.i+t7.i;
select * from t1,t2,t3,t4,t5,t6,t7,t8,t9 where t1.i(+)= t2.i+t3.i and t2.i(+) = t4.i+t5.i and t6.i(+) = t7.i+t8.i and t7.i(+)= t9.i;
select * from t1,t2,t3,t4,t5,t6 where t1.i(+)=t2.i+t3.i and t2.i(+) = t4.i and t3.i(+) = t5.i+t6.i;
select * from t1, t2, t3 where t1.i = t2.i(+) and t2.j(+) is not null and t3.i(+) is not null;
select * from t1, t2, t3 where t1.i = t2.i(+) and t2.j(+) is null and t3.i(+) is not null;
select * from t1, t2, t3 where t1.i = t2.i(+) and t2.j(+) is null and t3.i(+) is not null and t1.j(+) is not null;

--like testcase
select * from t1, t2 where t1.i like t2.j(+);

--between and
select * from t1, t2 where t1.i between t2.i(+) and t2.j(+);
select * from t1,t2,t3 where t1.i between t2.i(+) and t3.i(+);

--IS (NOT) NULL testcase
select * from t1, t2, t3 where t1.i = t2.i(+) and t3.i(+) is not null;
select * from t1, t2 where t1.i = t2.i(+) and t2.i is not null;
select * from t1,t2,t3,t4 where t1.i(+)+t2.i is not null and t3.i+t4.i is null;
select * from t1, t2 where t1.i = t2.i(+) and t2.j(+) is not null;
select * from t1, t2, t3 where t1.i = t2.i(+) and t2.j(+)+t3.i is not null;

--IN testcase
select * from t1, t2 where t1.i(+) in (t1.i(+));
select * from t1, t2 where t1.i in (t2.i(+));
select * from t1, t2, t3, t4, t5 where t1.i+t3.i in (t2.i(+)+t4.i+t3.j+t5.i);
select * from t1 where exists(select * from t2 where t1.i=t2.i(+));--bug#z959
select * from t1 where t1.i in (select t2.i from t2 where t1.i=t2.i(+));--bug#z959

--subquery has (+) outer join
select * from t1, t2 where t2.i = (select max(t1.i) from t1,t2 where t1.i(+)=t2.i) ;

--FUNC args has (+) outer join
select * from t1,t2 where abs(t1.i(+)+1) = t2.i;
select * from t1,t2 where abs(t1.i+1) = t2.i(+);

--NOT testcase
select * from t1,t2 where not (t1.i = t2.i(+));
select * from t1,t2,t3 where not (t1.i = t2.i(+) or  t2.i = t3.i(+));
select * from t1,t2 where not (t1.i(+) in (t2.i,1));
select * from t1,t2,t3 where not (t1.i(+) in (t2.i,t3.i));
select * from t1,t2 where not (not (t1.i = t2.i(+)));

--or condition has no plus
select * from t1,t2,t3 where (t1.i= t2.i or t2.i =1) and t1.i(+) = t3.i;
select * from t1,t2 where (t1.i= t2.i or t2.i =1) and t1.i(+) = 1;

--on condition has (+) outer join
select * from t1 left join t2 on t1.i = t2.i(+);
select * from t1 full join t2 on t1.i = t2.i(+);
select * from t1 inner join t2 on t1.i = t2.i(+);
select * from t1 left join t2 on t1.i+t2.i(+) is not null;
select * from t1 left join t2 on t1.i(+) = t1.j(+);
select * from t1 inner join t2 on t1.i between t2.i(+) and t2.j(+);

--show explain(verbose, costs off)
explain (costs off) select * from t1, t2 where t1.i = t2.i(+);
explain (costs off) select * from t1,t2,t3,t4,t5 where t1.i(+)+t2.i = t3.i and t4.i(+)= t2.i + t5.i;
explain (costs off) select * from t1, t2 where t2.i = (select max(t1.i) from t1,t2 where t1.i(+)=t2.i) ;
explain (costs off) select * from t1,t2 where not (t1.i = t2.i(+));

--view test
create view v1(i1,j1,i2,j2) as select * from t1, t2 where t1.i = t2.i(+);
select * from v1;

--error testcase
select * from t1,t2 where t1.i(+) = t2.i(+);--join to each other
select * from t1,t2 where t1.i = t1.j(+); 
select * from t1,t2,t3 where t2.j+t2.i(+) = t3.i;
select * from t1,t2 where t1.i(+) + t2.j(+) = 1;
select * from t1 left join t2 on t1.i = t2.i,t3,t4 where t4.i = t3.i(+);--mixed ANSI join and outer join
select * from t1,t2 where t1.i(+);
select * from t1,t2,t3 where t1.i(+)+t2.i(+);
select * from t1,t2 where t1.i = t2.i(+) or t2.j(+) = 2;--not support OR
select * from t1, t2 where t1.i in (t2.i(+), 3);--IN multi exprs
select * from t1, t2, t3 where t1.i(+)+t1.j in (t3.j);
select * from t1, t2 where t1.i(+) in (t1.j);
select * from t1, t2, t3 where t1.i(+)+t2.i(+) in (t3.j);
select * from t1, t2 where t1.i(+) in (t2.i(+)+1);
select * from t1,t2 where t2.i(+)= (select max(t1.i) from t1);--not support outer join subquery
select * from t1, t2, t3 where t2.i = (select max(t1.i) from t1,t2 where t1.i=t3.i(+)) ;--cannot ref outer table
select * from t1,t2,t3 where t1.i=t2.i(+) and t2.i = t3.i(+) and t3.i = t1.i(+);--loop check
select * from t1,t2,t3,t4,t5 where t1.i=t2.i(+) and t2.i = t3.i(+) and t3.i = t1.i(+) and t4.i(+)=t5.i;--loop check
select * from t1,t2 where abs(t1.i(+)+1) = t2.i(+);--both func arg and right oper has (+) 
select * from t1,t2,t3,t4,t5 where not (t1.i = t2.i(+) or  t2.i = t3.i(+) and t4.i(+) = t5.i);--not (A and B)
select * from t1,t2,t3 where not (t1.i = t2.i(+) and  t2.i = t3.i(+));--not (A and B)
select * from t1,t2 where t1.i(+) not between t2.i and 1;--not between and
select * from t1,t2 where (t1.i(+)= t2.i or t2.i =1) and t1.i(+) = 1;
select sum(t1.i(+)) from t1, t2 where t1.i = t2.i(+);
select t1.i(+) from t1;
select * from t1,t2 left join t3 on t2.i = t3.i(+) and t2.j(+) = t3.j;
select * from t1 right join t2 on t1.i = t2.i(+);
select * from t1 right join t2 on t1.i(+) = t2.i(+);
select * from t1 right join t2 on t1.i(+) = t1.i;
select * from t1 left join t2 on t1.i(+) in (t2.i, t2.j);
select * from t1 left join t2 on t1.i in (t2.i(+), 4);
select * from t1 left join t2 on t1.i(+) in (t2.i);
select * from t1 inner join t2 on t1.i(+) in (t2.i,4);
select * from t1 right join t2 on t1.i(+) in (t2.i(+), 4);

--this test case is difference from oracle
select * from t1, t2,t3,t4 where t1.i = t2.i(+) and t3.i = t4.i and t4.j = 1 or t2.i(+) = 1;

drop view v1;
drop table t1;
drop table t2;
drop table t3;
drop table t4;
drop table t5;
drop table t6;
drop table t7;
drop table t8;
drop table t9;

CREATE TABLE toj1 (id int, id2 int, name  VARCHAR(10));  
CREATE TABLE toj2 (id int, id2 int, name  VARCHAR(10)); 
CREATE TABLE toj3 (id int, id2 int, name  VARCHAR(10)); 
CREATE TABLE toj4 (id int,id2  int, name  VARCHAR(10)); 
INSERT INTO toj1 VALUES(1,0,'A');  
INSERT INTO toj1 VALUES(3,0,'C');  
  
INSERT INTO toj2 VALUES(1,0,'AA');  
INSERT INTO toj2 VALUES(2,0,'BB');  
INSERT INTO toj3 VALUES(3,0,'CCC');  
INSERT INTO toj3 VALUES(4,0,'DDD'); 
INSERT INTO toj4 VALUES(4,0,'DDDD');
--create view
create view v1 (i1, j1, k1, i2,j2,k2 ) as select *from toj1, toj2 where toj1.id(+) = toj2.id;
select *from v1;
\d+ v1
drop view v1;
create view v1 (i1, j1, k1, i2, j2, k2,i3, j3, k3 )as select *from toj1, toj2, toj3 
	where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id;
select *from v1;
\d+ v1
drop view v1;
create view v1 (i1, j1, k1, i2,j2,k2 ) as select * from toj1, toj2 where abs(toj1.id(+) + 1 - 1)  = toj2.id;
select *from v1;
\d+ v1

--muiti tables using (+), all SQL  execute successfully
select *from toj1, toj2 where toj1.id(+) = toj2.id;
select *from toj1, toj2 where toj1.id = toj2.id(+);
select *from toj1, toj2, toj3,toj4 where toj1.id = toj2.id(+) and toj3.id = toj4.id(+);
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2 and toj1.name(+) = toj2.name;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id = 1;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj1.id2(+) = toj2.id2;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id;
select *from toj1, toj2, toj3 where toj2.id = toj1.id(+) and toj2.id (+)= toj3.id;
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id (+)= toj1.id;
select *from toj1, toj2 where toj1.id(+) = 1;
select *from toj1 t1, toj1 t2 where t1.id(+) = t2.id2;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj2.id = 1;
select *from toj1, toj2 where toj1.id(+) = toj2.id and 1 = 1 and toj2.id = 1;
select *from toj1, toj2, toj3 where toj2.id = toj1.id and toj2.id = toj3.id(+);
select *from toj1, toj2, toj3 where toj2.id = toj3.id(+) and toj2.id = toj1.id;

select *from toj1, toj2, toj3,toj4 where 
     toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj3.id(+) = toj4.id;
	 
select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id; 
	 
select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj1.id2(+) = toj2.id2;
	 
select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj2.id(+) = toj3.id;

select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id (+)= toj1.id;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id (+)= toj1.id;
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id = toj1.id(+);
select *from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj3.id = toj1.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id = toj1.id(+);
select *from toj1, toj2, toj3 where toj2.id(+) = toj1.id and toj2.id (+)= toj3.id;

--some exeception cases, raise error
select *from toj1, toj2, toj3 where toj2.id = toj1.id(+) and toj2.id (+)= toj5.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2 = toj2.id2(+);
select * from toj1, toj2 where abs(toj1.id(+)+toj1.id2) = toj2.id;
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id = toj2.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj2.id (+)= toj3.id and toj1.id2 = toj2.id2(+);
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj2.id(+) = toj1.id;

select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj2.id(+) = toj1.id;
	 
select *from toj1, toj2,toj3, toj5 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj1.id2(+) = toj2.id2;

select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and toj3.id(+) = toj4.id and toj2.id(+) = toj3.id
	 and toj4.id(+) = toj1.id;

--others cases
create or replace function outerjoin_f11(p1 int, p2 int) return integer as
begin
	return 1;
end;
/

create or replace function outerjoin_f12(p1 boolean) return int as
begin
	return 0;
end;
/

--execute successfully
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id, toj1.id)  = toj2.id(+);
select *from toj1, toj2 where toj1.id(+) = abs(-20 * abs(toj2.id));
select *from toj1, toj2 where toj1.id = abs(toj2.id(+) + 1);
select *from toj1, toj2 where toj1.id = outerjoin_f11(toj2.id(+),1);
select * from toj1, toj2, toj3  where  toj2.id - toj2.id = toj1.id(+);
select *from toj1, toj2 where toj2.id = toj1.id(+) - toj2.id;
select *from toj1, toj2 where toj1.id = toj2.id(+) + toj1.id;
select *from toj1, toj2 where toj1.id  - toj1.id2 = toj2.id(+) + toj1.id;
select *from toj1, toj2 where toj1.id(+) + toj2.id2 = toj2.id;
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id, toj3.id)  = toj2.id(+); 
select * from toj1, toj2, toj3 where outerjoin_f11(toj1.id(+), toj3.id)  = toj2.id;
select *from toj1, toj2,toj3 where toj1.id = abs(toj2.id(+) + toj3.id); 
select *from toj1, toj2, toj3 where toj1.id(+) + toj2.id2 = toj2.id - toj3.id2;

--should raise error
select *from toj1, toj2 where toj1.id = abs(toj2.id(+) + toj2.id);
select *from toj1, toj2 where toj1.id = toj2.id(+) + toj2.id;
select *from toj1, toj2 where toj1.id  - toj2.id = toj2.id(+) + toj1.id; 
select *from toj1, toj2 where toj1.id(+) + toj1.id2 = toj2.id;
select *from toj1, toj2 where toj1.id(+) + toj2.id2 = toj2.id - toj1.id2;
select *from toj1, toj2 where toj1.id(+)  = toj2.id(+);

--this test case is difference from oracle
select * from toj1, toj2 where (toj1.id(+) = toj2.id and toj1.id = 2) or (toj1.id = 2 or toj2.id = 3);

--IN , NOT IN
select * from toj1, toj2 where toj1.id in (toj2.id(+));
select * from toj1, toj2 where toj1.id in (toj2.id(+) + 1);
select * from toj1, toj2 where toj1.id in (toj2.id(+) - toj1.id2);
select * from toj1, toj2 where toj1.id - toj1.id2  in (toj2.id(+));
select * from toj1, toj2 where toj1.id(+) - toj2.id in (toj2.id); 

select * from toj1, toj2 where toj1.id not in (toj2.id(+) - toj1.id2);
select * from toj1, toj2 where toj1.id - toj1.id2  not in (toj2.id(+));
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id);
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id, toj2.id2);

--error
select * from toj1, toj2 where toj1.id in (toj2.id(+) , 1);
select * from toj1, toj2 where toj1.id in (toj2.id(+) - toj2.id2);
select * from toj1, toj2 where toj1.id - toj2.id2  in (toj2.id(+));
select * from toj1, toj2 where toj1.id(+) - toj1.id2 in (toj2.id);

select * from toj1, toj2 where toj1.id not in (toj2.id(+) - toj2.id2);
select * from toj1, toj2 where toj1.id - toj2.id2  not in (toj2.id(+));
select * from toj1, toj2 where toj1.id(+) - toj2.id not in (toj2.id, toj1.id2);
select * from toj1, toj2 where toj1.id(+) - toj1.id2 not in (toj2.id);

select * from toj1, toj2 where toj1.id(+) in  (select toj2.id from toj2); 
select * from toj1, toj2 where toj1.id(+) in  (select toj2.id from toj2, toj3 where toj2.id(+) = toj3.id);

--is null, is not null
select * from toj1 where toj1.id(+) is not null; 
select * from toj1 where toj1.id(+) is  null;

--between and, not between and
select * from  toj1, toj2 where toj1.id  between toj2.id and 1; 
select * from  toj1, toj2 where toj1.id(+)  between toj2.id and 1; 
select * from  toj1, toj2 where toj1.id(+) - toj2.id2  between toj2.id and 1;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) and 1;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) - toj1.id and 1; 
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+);
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+) - toj1.id;
select * from  toj1 t1, toj1 t2 where t1.id between t2.id(+) and t1.id;
select * from  toj1, toj2 where toj1.id  between toj2.id(+) and toj2.id;

select * from  toj1, toj2 where toj1.id(+) not  between toj2.id and 1; --error
select * from  toj1, toj2 where toj1.id not between toj2.id(+) and 1; --error
select * from  toj1, toj2 where toj1.id(+)  between toj2.id and toj2.id2(+); --error
select * from  toj1, toj2 where toj1.id(+)  between toj2.id(+) and toj2.id2(+); --error
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj1.id2(+); --error
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+) - toj2.id; --error

--like, not like
select * from  toj1, toj2 where toj2.name LIKE toj1.name(+);
select * from  toj1, toj2 where toj2.name(+) LIKE toj1.name; 
select * from  toj1, toj2 where toj2.name NOT LIKE toj1.name(+);
select * from  toj1, toj2 where toj2.name(+) NOT LIKE toj1.name; 

select *from toj1, toj2,toj3, toj4 where 
     toj1.id(+) = toj2.id and
	 toj3.id(+) = toj4.id and 
	 toj2.id(+) = toj3.id and
	 toj1.id2(+) = toj2.id2;

--bug0000313
select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id(+) = 1;
select * from toj1,toj2 where toj1.id = toj2.id(+) and toj1.id(+) = 1; 
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj3.id(+) = 3;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and toj1.id(+) = 1;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and toj1.id(+) between 2 and 3;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and
   toj1.id(+) between toj2.id and 3;

select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 between toj1.id(+) and 3;
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 between 0 and toj1.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 between toj1.id2(+) and toj1.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 between 1 and toj3.id(+);

select *from toj1, toj2 where toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2
  and 1 between toj1.id and toj2.id;

select * from toj1, toj2 where toj1.id in (toj2.id(+)) and 1 in (toj1.id);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+) and 1 in (toj1.id);
select * from toj1, toj2 where toj1.id(+) = toj2.id and 1 not in (toj2.id, toj2.id2);
select *from toj1, toj2, toj3 where toj1.id(+) = toj2.id and toj1.id = toj3.id(+)
  and 1 not in (toj1.id2, toj2.id2);

select *from toj1, toj2 where toj1.id(+) = 1 and  toj1.id(+) = toj2.id;
select * from toj1,toj2 where toj1.id(+) = 1 and  toj1.id = toj2.id(+); 
select *from toj1, toj2, toj3 where toj3.id(+) = 3 and toj1.id(+) = toj2.id;
select *from toj1, toj2, toj3 where toj1.id(+) = 1 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) between 2 and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where toj1.id(+) between toj2.id and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where 1 between toj1.id(+) and 3 and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where 1 between 0 and toj1.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where 1 between toj1.id2(+) and toj1.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2, toj3 where 1 between 1 and toj3.id(+) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select *from toj1, toj2 where 1 between toj1.id and toj2.id and toj1.id(+) = toj2.id and toj1.id2(+) = toj2.id2;
select * from toj1, toj2 where 1 in (toj1.id) and toj1.id in (toj2.id(+));
select *from toj1, toj2, toj3 where 1 in (toj1.id) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);
select * from toj1, toj2 where 1 not in (toj2.id, toj2.id2) and toj1.id(+) = toj2.id;
select *from toj1, toj2, toj3 where 1 not in (toj1.id2, toj2.id2) and toj1.id(+) = toj2.id and toj1.id = toj3.id(+);

--bug0000328
select * from toj1,toj2 where toj1.id(+) in (1,2,null);

--bug0000321
select * from toj1,toj2 where toj1.id = toj1.id(+);
select * from toj1,toj2 where toj1.id in toj1.id(+);
select * from toj1,toj2 where toj1.id(+) in toj1.id;
select * from toj1,toj2 where toj1.id between toj1.id(+) and 1;
select * from toj1,toj2 where toj1.id(+) between toj1.id and 1;

--bug0000312
select * from toj1,toj2 where toj1.id= toj2.id(+) + toj2.id2(+) order by toj1.id;
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) > 2;
select * from toj1,toj2 where 1 in ( toj1.id(+) + toj1.id2(+));
select * from toj1,toj2 where 1 not in (toj1.id(+) + toj1.id2(+));
select * from toj1,toj2 where 1 between toj1.id2(+) and toj1.id(+);
select * from toj1,toj2 where 1 between toj1.id(+) and toj2.id(+);
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) between toj2.id and 2;
--the cases should raise error
select * from toj1,toj2,toj3 where toj1.id = toj2.id(+) + toj3.id(+);
select * from toj1,toj2,toj3 where toj2.id(+) + toj3.id(+) > toj1.id ;
select * from toj1,toj2 where 1 in ( toj1.id(+) + toj2.id(+));
select * from toj1,toj2 where toj1.id(+) + toj2.id(+) in 1;
select * from toj1,toj2,toj3 where toj1.id(+) + toj2.id(+) in toj3.id;
select * from toj1,toj2 where 1 not in (toj1.id(+) + toj2.id(+));
select * from toj1,toj2 where toj1.id(+) + toj2.id(+) not in 1;
select * from toj1,toj2,toj3 where toj1.id(+) + toj2.id(+) not in toj3.id;
select * from toj1,toj2 where toj1.id(+) + toj1.id2(+) between toj2.id(+) and 2;
--bug0000322
select * from toj1,toj2 where toj1.id(+) + toj2.id(+)> 2;--should raise error

--bug0000325
select * from toj1,toj2,toj3 where toj1.id between toj2.id(+) and toj3.id(+);
select * from  toj1, toj2 where toj1.id between toj2.id(+) and toj2.id2(+);

--bug0000317
insert into toj1(id2, name) values(0,'N');
insert into toj2(id2, name) values(0,'NN');

select * from toj1 where toj1.id(+) is not null;
select * from toj1, toj2 where toj1.id = toj2.id(+) and toj1.id(+) is not null;
select * from toj1, toj2 where toj1.id(+) is not null and toj1.id = toj2.id(+);
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) is not null and toj1.id(+) = toj3.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) = toj3.id and toj1.id(+) is not null;
select * from toj1, toj2, toj3 where toj1.id(+) is not null and toj1.id = toj2.id(+) and toj1.id(+) = toj3.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj2.id2(+) is not null;

select * from toj1, toj2 where toj1.id - toj2.id(+) is not null;
select * from toj1, toj2 where toj1.id(+) - toj2.id is not null;
select * from toj1, toj2 where toj1.id - toj2.id(+) is not null and toj1.id2 = toj2.id(+);
select * from toj1, toj2 where toj1.id2 = toj2.id(+) and toj1.id - toj2.id(+) is not null;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id2 is not null;

select * from toj1 where toj1.id(+) is null;
select * from toj1, toj2 where toj1.id = toj2.id(+) and toj1.id(+) is null;
select * from toj1, toj2 where toj1.id(+) is null and toj1.id = toj2.id(+);
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) is null and toj1.id(+) = toj3.id;
select * from toj1, toj2, toj3 where toj1.id = toj2.id(+) and toj1.id(+) = toj3.id and toj1.id(+) is null;
select * from toj1, toj2, toj3 where toj1.id(+) is null and toj1.id = toj2.id(+) and toj1.id(+) = toj3.id;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj2.id2(+) is null;

select * from toj1, toj2 where toj1.id - toj2.id(+) is null;
select * from toj1, toj2 where toj1.id(+) - toj2.id is null;
select * from toj1, toj2 where toj1.id - toj2.id(+) is null and toj1.id2 = toj2.id(+);
select * from toj1, toj2 where toj1.id2 = toj2.id(+) and toj1.id - toj2.id(+) is null;
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id2 is null;
select * from toj1, toj2,toj3 where toj1.id - toj2.id(+) + toj3.id is not null;

--should raise error
select * from toj1, toj2 where toj1.id - toj2.id(+) + toj1.id(+) is not null;
select * from toj1, toj2,toj3 where toj1.id - toj2.id(+) + toj3.id(+) is not null;

--bug0000327
--should raise error
select toj1.id, toj3.id from toj1,toj2,toj3 where not (toj1.id between toj2.id(+) and toj3.id);
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id between toj2.id(+) and 1);
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id >= toj2.id(+) and toj1.id <= 1);
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id between toj2.id(+) and 1 and toj1.id > 0);
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id between toj2.id(+) and 1) and toj1.id > 0);
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+)=toj3.id and toj1.id = 1 or toj1.id2(+) <toj3.id2 );
select toj1.id, toj3.id from toj1,toj3 where not((toj1.id(+)=toj3.id and toj1.id = 1) or toj1.id2(+) <toj3.id2 );
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) not in (toj3.id, 1));
select toj1.id, toj3.id from toj1,toj3 where not(not(not(toj1.id(+) not in (toj3.id, 1))));
select toj1.id, toj3.id from toj1,toj3 where not(not(toj1.id(+) in (toj3.id, 1)));
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(toj1.id(+) = 1 or toj1.id(+) = toj3.id));
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(not(toj1.id(+) = 1 and toj1.id(+) = toj3.id)));
select toj1.id, toj3.id,toj2.id from toj1,toj3,toj2 where not (toj1.name ||'c' like (toj2.name(+) || 'c') and toj3.id(+)>toj1.id) order by toj1.id;
select toj1.id, toj3.id from toj1,toj3 where not( toj1.id(+) is not null or toj1.id(+) = toj3.id) and not(toj1.id(+) = toj3.id and toj1.id(+) = 1);
select toj1.id, toj2.id from toj1,toj2 where (toj1.id = toj2.id or toj2.id = 1) or toj1.id(+) = 1;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id or toj2.id = 1) and toj1.id(+) = 1;
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id  and toj1.id(+) = 1) or (toj2.id = 1 and toj1.id(+) = 1);
select toj1.id, toj2.id from toj1,toj2 where (toj1.id(+) = toj2.id  and toj1.id(+) = 1) or toj2.id = 1;
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  and not( toj1.id(+) = 1 and toj3.id2 = 1);
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  or not( toj1.id(+) = 1 or toj3.id2 = 1);
select toj1.id, toj2.id from toj1,toj2 where  toj1.id = toj2.id or toj2.id = 1 and toj1.id(+) = 1;
select toj1.id, toj2.id from toj1,toj2 where  toj1.id = toj2.id or (toj2.id = 1 and toj1.id(+) = 1);

--execute success
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) in (toj3.id, 1));
select toj1.id, toj3.id from toj1,toj3 where not(not(toj1.id(+) not in (toj3.id, 1)));
select toj1.id, toj3.id from toj1,toj3 where toj1.id(+) not in (toj3.id, 1);
select toj1.id, toj3.id from toj1,toj3 where not(not(not(toj1.id(+) in (toj3.id, 1))));
select toj1.id, toj3.id from toj1,toj2,toj3 where not ( NOT(toj1.id between toj2.id(+) and toj3.id) or toj1.id > 0);
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id = toj2.id(+));
select toj1.id, toj2.id from toj1,toj2 where not (toj1.id not between toj2.id(+) and 1);
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id between toj2.id(+) and 1));
select toj1.id, toj2.id from toj1,toj2 where not (not (toj1.id >= toj2.id(+) and toj1.id <= 1));
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id(+) in (toj3.id, 1));
select toj1.id, toj3.id from toj1,toj3 where not(toj1.id != 2 or not(toj1.id(+) = 1 and toj1.id(+) = toj3.id));
select toj1.id, toj3.id from toj1,toj3 where not( toj1.id(+) is not null or toj1.id(+) = toj3.id);
select toj1.id, toj2.id from toj1,toj2 where not (not(toj1.id = toj2.id or toj2.id = 1) or toj1.id(+) = 1);
select toj1.id, toj2.id from toj1,toj2 where (toj1.id = toj2.id or toj2.id = 1) and toj1.id(+) = 1;
select toj1.id, toj2.id from toj1,toj2,toj3 where toj1.id(+) = toj2.id  and not( toj3.id = 1 and toj3.id2 = 1);
select toj1.id, toj2.id from toj1,toj2 where not(toj1.id = toj2.id);
select toj1.id, toj2.id from toj1,toj2 where not(toj1.id = toj2.id);
select toj1.id, toj2.id from toj1,toj2 where  (toj1.id = toj2.id or toj2.id = 1) and toj1.id(+) = 1;

--bug0000584
create table a_bug584(i int, j int);
create table b_bug584(i int, j int);
create table c_bug584(i int, j int);

create view idx_bug584 as
  SELECT 'a' AS zyfl
  from
   (select i  AS type1, j AS type11 from a_bug584 ) t1,
   (select i  AS type2, j AS type22 from b_bug584 ) t2,
   (select i  AS type3, j AS type33 from c_bug584 ) t3
   where t1.type1 = t2.type2(+) and t1.type11 = t2.type22(+)
       and t1.type1 = t3.type3(+) and t1.type11 = t3.type33(+);
\d+ idx_bug584
drop view idx_bug584;
drop table a_bug584;
drop table b_bug584;
drop table c_bug584;

DROP VIEW  v1;
DROP TABLE toj1;
DROP TABLE toj2;
DROP TABLE toj3;
DROP TABLE toj4;

create table t1(id int, a int, b int, c varchar(100));
create table t2(id int, a int, b int, c varchar(100));
create table t3(id int, x int, y int, z varchar(100));

insert into t2 values(2, 22, 22, '2');
insert into t2 values(4, 44, 44, '4');
insert into t2 values(3, 3, 3, '3');
insert into t2 values(4, 44, 44, '4');
insert into t2 values(2, 22, 22, '2');
insert into t2 values(4, 44, 44, '4');

insert into t1 values(1, 1, 1, '1');
insert into t1(a,b,c) values( 2, 2, '2');
insert into t1(id,b,c)values(3, 3, '3');
insert into t1 values(4, 4, 4, '4');
insert into t1(id,b,c)values(5, 5, '4');
insert into t1(a,b,c) values( 4, 5, '4');
insert into t1 values(6, 4, 5, '4');

insert into t3 values(1, 1, 1, 'ssda');
insert into t3(id,y,z) values(3, 3, 'ssda');
insert into t3 values(4, 44, 44, 'ssda');
insert into t3(x,y,z) values( 55, 55, 'ssda');
--bug#m316
select t1.id, t3.id from t1,t3 where not(t1.id(+)=t3.id or t1.a(+) <t3.x ) and t1.id(+)<t3.y order by t1.id;

--bug#m313
select * from t1,t3 where t1.id = t3.id(+) and t3.x(+)=1 order by t1.id;

--bug#m319
select t1.id, t2.id,t3.id ,t.id from t1,t3 ,t2,t1 t where t1.id(+)=t2.id and t1.a<t3.x(+) and t.id>t2.id(+);

--bug#m320
select t1.id, t2.id,t3.id ,t.id from t1,t3 ,t2,t1 t,t2 tt where t1.id(+)=t2.id and t1.a<t3.x(+) and t.id>t2.id(+)and tt.id(+)=t.id;

drop table t1;
drop table t2;
drop table t3;

--bugm626
CREATE TABLE toj1 (id int,id2 int,name VARCHAR(10));
CREATE TABLE toj2 (id int,id2 int,name VARCHAR(10));
INSERT INTO toj1 VALUES(1,0,'A');
INSERT INTO toj1 VALUES(3,0,'C');
INSERT INTO toj2 VALUES(1,0,'AA');
INSERT INTO toj2 VALUES(2,0,'BB');
select * from toj1, toj2 where (toj1.id(+) = toj2.id and toj1.id = 2) or (toj1.id = 2 or toj2.id = 3);
drop table toj1;
drop table toj2;

--bug#z1334
create table t_a (
id number,
code number,
name varchar2(10)
);
create table t_b (
id number,
code number,
name varchar2(10)
);
create table t_c (
id number,
code number,
name varchar2(10)
);
create table t_d (
id number,
code number,
name varchar2(10)
);

insert into t_a(id,code,name) values(1,2,'A');
insert into t_a(id,code,name) values(2,1,'B');
insert into t_a(id,code,name) values(3,5,'C');
insert into t_a(id,code,name) values(4,6,'D');
insert into t_a(id,code,name) values(5,7,'E');

insert into t_b(id,code,name) values(1,3,'AA');
insert into t_b(id,code,name) values(1,4,'BB');
insert into t_b(id,code,name) values(2,1,'CC');
insert into t_b(id,code,name) values(1,2,'DD');
insert into t_b(id,code,name) values(7,5,'GG');

insert into t_c(id,code,name) values(1,3,'AAA');
insert into t_c(id,code,name) values(1,4,'BBB');
insert into t_c(id,code,name) values(2,1,'CCC');
insert into t_c(id,code,name) values(1,2,'DDD');
insert into t_c(id,code,name) values(7,5,'GGG');

insert into t_d(id,code,name) values(1,3,'AAAA');
insert into t_d(id,code,name) values(1,4,'BBBB');
insert into t_d(id,code,name) values(2,1,'CCCC');
insert into t_d(id,code,name) values(1,2,'DDDD');
insert into t_d(id,code,name) values(7,5,'GGGG');

select * from t_a a,t_b b,t_c c,t_d d
where a.id=b.code(+)
and b.code=c.id(+)
and b.id=d.code(+);

drop table t_a;
drop table t_b;
drop table t_c;
drop table t_d;


--OA#2199
create table test_t1(county_id integer,ne_model integer,b_mis_pro_no integer);
create table test_t2(int_id integer,  name varchar2(256));
create table test_t3(int_id integer, name varchar2(256));
create table test_t4(int_id integer);

insert into test_t1 values(1,1,1);
insert into test_t1 values(2,2,2);
insert into test_t1 values(3,3,3);

insert into test_t2 values(1,'one');
insert into test_t2 values(2,'two');

insert into test_t3 values(1,'one');
insert into test_t3 values(3,'three');

insert into test_t4 values(1);
insert into test_t4 values(2);
insert into test_t4 values(3);
insert into test_t4 values(5);

--4 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d where a.county_id = b.int_id(+) and
 a.ne_model = c.int_id(+) and  d.int_id = a.b_mis_pro_no(+) order by a.county_id;

--12 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d where a.county_id = b.int_id(+) and
 a.ne_model = c.int_id(+) order by a.county_id,c.name,d.int_id;

--3 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d where a.ne_model = c.int_id(+) and
 c.int_id = d.int_id(+) and d.int_id = b.int_id(+) order by a.county_id;

--3 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d where a.ne_model = c.int_id(+) and
 c.int_id = b.int_id(+) and b.int_id = d.int_id(+) order by a.county_id;

--4 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d where a.county_id + c.int_id = b.int_id(+) and
 a.ne_model = c.int_id(+) and  d.int_id = a.b_mis_pro_no(+) order by a.county_id;

create table test_t5 as select  *from test_t4 with no data;

insert into test_t5 values(1);
insert into test_t5 values(2);
insert into test_t5 values(3);
insert into test_t5 values(4);

--4 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d,test_t5 e where a.county_id + e.int_id = b.int_id(+) and
 a.ne_model = c.int_id(+) and  d.int_id = a.b_mis_pro_no(+) and d.int_id = e.int_id(+) order by a.county_id;

--4 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d,test_t5 e where a.county_id + e.int_id = b.int_id(+) and
 a.ne_model = c.int_id(+) and  d.int_id = a.b_mis_pro_no(+) and d.int_id = e.int_id(+) and c.int_id = e.int_id(+) order by a.county_id;

--8 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d,test_t5 e where a.county_id + e.int_id = b.int_id(+) and a.county_id = b.int_id(+) and
  d.int_id = a.b_mis_pro_no(+) and  c.int_id = e.int_id(+) order by a.county_id,e.int_id;

--4 rows
select * from test_t1 a,test_t2 b, test_t3 c,test_t4 d ,test_t5 e where a.county_id = b.int_id(+) and
 a.ne_model = c.int_id(+) and d.int_id = e.int_id(+)  and d.int_id = a.b_mis_pro_no(+) order by a.county_id;

--6 rows
select * from test_t1 a,test_t2 b,test_t3 c where a.county_id + b.int_id = c.int_id(+)
 and b.int_id = c.int_id(+) order by a.county_id,b.name;

--6 rows
select * from test_t1 a,test_t2 b,test_t3 c where
 b.int_id = c.int_id(+) and a.county_id + b.int_id = c.int_id(+) order by a.county_id,b.name;

--6 rows
select * from test_t1 a,test_t2 b,test_t3 c where a.county_id + b.int_id = c.int_id(+) order by a.county_id,b.name;

--6 rows
select * from test_t1 a,test_t2 b,test_t3 c where a.county_id = c.int_id(+) and b.int_id = c.int_id(+) order by a.county_id,b.name;

--2 rows
select * from test_t1 a,test_t2 b,test_t3 c where a.county_id = c.int_id(+) and b.int_id = c.int_id(+)
 and a.county_id = b.int_id + 1 order by a.county_id;

--3 rows
select * from test_t1 a,test_t2 b,test_t3 c where a.county_id = c.int_id(+) and b.int_id = c.int_id(+) and
 a.county_id = b.int_id(+) + 1 order by a.county_id;

--6 rows
select * from test_t1 a,test_t2 b,test_t3 c,test_t4 d where a.county_id = b.int_id(+)
 and c.int_id = d.int_id(+) order by a.county_id,d.int_id;

 --cllean data
drop table test_t1;
drop table test_t2;
drop table test_t3;
drop table test_t4;
drop table test_t5;
