----------------- char(0) ----------------
drop table if exists t1;
create table t1 (b char(0) not null);
create table if not exists t1 (b char(0) not null); --error
insert into t1 values (''),(null);
insert into t1 values ('');
select * from t1;
drop table t1;

create table t1 (b char(0) not null); 
create index idxt1 on t1(b); --error
drop table t1;

CREATE TABLE t1 (
bool     char(0) default NULL,
not_null varchar(20) NOT NULL default '',
misc     integer not null
);
INSERT INTO t1 VALUES (NULL,'a',4), (NULL,'b',5), (NULL,'c',6), (NULL,'d',7); 
select * from t1 where misc > 5 and bool is null; --two records
delete   from t1 where misc > 5 and bool is null;
select * from t1 where misc > 5 and bool is null; --no record
select count(*) from t1; --two records
delete from t1 where 1 > 2;
select count(*) from t1;  --two records
delete from t1 where 3 > 2;
select count(*) from t1; --no record
drop table t1;

CREATE TABLE t1(a char(0));
INSERT IGNORE INTO t1 (SELECT -pi());	--Not support yet
select * from t1;
DROP TABLE t1;

-- select length(quote(concat(''::char(0),"test"))); --8
select length(concat(char(0),"test")); --not support, mysq is 5


create table t1 as select
--nullif(null, 'string') as c14,
trim('string' from null) as c15,
replace(null, 'from', 'to') as c32;

-- mysql COLUMN type is char(0), we are text
\d t1
drop table t1;


create table t1 (
type_bool bool not null default false,
empty_char char(0));
\d t1
drop table t1;

CREATE TABLE t6(c1 CHAR(0) NULL);
\d t6
DROP TABLE t6;

CREATE TABLE t6(c1 VARCHAR(0) NULL);
\d t6
DROP TABLE t6;

CREATE TABLE t6(c1 CHAR(0) NOT NULL);
\d t6
DROP TABLE t6;

CREATE TABLE t6(c1 VARCHAR(0) NOT NULL);
\d t6
DROP TABLE t6;


create table t1 (
c1 char(10),
c2 varchar(100),
c3 varchar(1000),
d varchar(8000),
a int primary key
);
-- select a, trim(trailing char(0) from c1) as c1,c2,c3 from t1 order by a;
select a as c1,c2,c3 from t1 order by a;
drop table t1;

create table t1 (
a int primary key,
b char(0),
c char(1));
insert into t1 values(1, '', 'a'), (2, NULL, 'b');
select * from t1 order by a;
drop table t1;

create table t1 (
a int primary key,
b char(0),
c bit(1)
);
insert into t1 values(1, '', 0),
(2, '', 1),
(3, '', NULL),
(4, NULL, 0),
(5, NULL, 1),
(6, NULL, NULL);
select a,b,bin(c) from t1 where b = '' and c is null order by a;
select a,b,bin(c) from t1 where b is null and c is null order by a;
select a,b,bin(c) from t1 where b = '' and c = 0 order by a;
select a,b,bin(c) from t1 where b is null and c = 0 order by a;
select a,b,bin(c) from t1 where b = '' and c = 1 order by a;
select a,b,bin(c) from t1 where b is null and c = 1 order by a;
drop table t1;

create table t1 (
a int primary key,
b char(0) not null,
c char(1)
);
insert into t1 values(1, '', 'a'), (2, '', 'b');
select * from t1 order by a;
select count(*) from t1 where b = '';
select count(*) from t1 where b is null;
drop table t1;


create table t1 (a int primary key, b char(0));
insert into t1 values (1,'');
insert into t1 values (2,NULL);
select * from t1 order by a;
select * from t1 order by b;
select * from t1 where b IS NULL;
select * from t1 where b IS NOT NULL;
drop table t1;

create table t1(a int, b char(0));
insert into t1 values(1,''), (2, NULL);
--replace_column 10 # 11 #
explain select * from t1 where b = '';
select * from t1 where b = '';
drop table t1;

create table t1 (
  a int primary key,
  b char(0),
  c char(1)
);

--echo #test for insertion in char(0) column
insert into t1 values(1, '', 'a'), (2, NULL, 'b');
--echo #testing values stored in the table t1
select * from t1 order by a;
select count(*) from t1 where b = '';
select count(*) from t1 where b is null;
drop table t1;


--echo #testing bit(1) with char(0)
create table t1 (
  a int primary key,
  b char(0),
  c bit(1)
);
--echo #testing insertion
insert into t1 values(1, '', 0),
                     (2, '', 1),
                     (3, '', NULL),
                     (4, NULL, 0),
                     (5, NULL, 1),
                     (6, NULL, NULL);

--echo #verify data in table
select a,b,bin(c) from t1 where b = '' and c is null order by a;
select a,b,bin(c) from t1 where b is null and c is null order by a;
select a,b,bin(c) from t1 where b = '' and c = 0 order by a;
select a,b,bin(c) from t1 where b is null and c = 0 order by a;
select a,b,bin(c) from t1 where b = '' and c = 1 order by a;
select a,b,bin(c) from t1 where b is null and c = 1 order by a;
drop table t1;


create table t1 (
  a int primary key,
  b char(0) not null,
  c char(1)
);
insert into t1 values(1, '', 'a'), (2, '', 'b');
--echo #testing values stored in the table t1
select * from t1 order by a;
select count(*) from t1 where b = '';
select count(*) from t1 where b is null;
drop table t1;


create table t1 (b char(0) not null);
create index t1idx on t1(b); --err
drop  table t1;

create table t1 (b char(0));
create index t1idx on t1(b); --err
drop  table t1;
create table t1 (b char(0) primary key); --err

create table t1 (b char(0) default 'aaa'); --err
create table t1 (b char(0) default 'aa'); --err
create table t1 (b char(0) default 'a'); --err
create table t1 (b char(0) default ''); --succ
drop table t1;

create table t1 (b char(0) default not null); --err
create table t1 (b char(0) default null); --succ
drop table t1;


create table t1 (b char(0));
insert into t1 values ('aaaaa'); --err
insert into t1 values ('aaaa'); --err
insert into t1 values ('aaa'); --err
insert into t1 values ('aa'); --err
insert into t1 values ('a'); --err
insert into t1 values ('');
insert into t1 values (null);
select * from t1;
delete from t1;

set sql_mode = '';
insert into t1 values ('aaaaa');
insert into t1 values ('aaaa');
insert into t1 values ('aaa');
insert into t1 values ('aa');
insert into t1 values ('a');
insert into t1 values ('');
insert into t1 values (null);
select * from t1;
delete from t1;
drop table t1;
reset sql_mode;


create table t1 (b char(0));
insert into t1 values (repeat('a', 5));
insert into t1 values (repeat('a', 4));
insert into t1 values (repeat('a', 3));
insert into t1 values (repeat('a', 2));
insert into t1 values (repeat('a', 1));
select * from t1;
delete from t1;
insert into t1 values (concat('ab', repeat('a', 5)));
insert into t1 values (concat('ab', repeat('a', 4)));
insert into t1 values (concat('ab', repeat('a', 3)));
insert into t1 values (concat('ab', repeat('a', 2)));
insert into t1 values (concat('ab', repeat('a', 1)));
select * from t1;
delete from t1;
set sql_mode = '';
insert into t1 values (repeat('a', 5));
insert into t1 values (repeat('a', 4));
insert into t1 values (repeat('a', 3));
insert into t1 values (repeat('a', 2));
insert into t1 values (repeat('a', 0));
select * from t1;
delete from t1;
insert into t1 values (concat('ab', repeat('a', 5)));
insert into t1 values (concat('ab', repeat('a', 4)));
insert into t1 values (concat('ab', repeat('a', 3)));
insert into t1 values (concat('ab', repeat('a', 2)));
insert into t1 values (concat('ab', repeat('a', 1)));
select * from t1;
delete from t1;
drop table t1;
reset sql_mode;

----------- nchar varchar(n) ------------
select cast('aaa' as nchar varchar(5));
select cast('' as nchar varchar(5));
create table tb(id int, sex nchar varchar(1));
\d tb
insert into tb values (1, 'fff'); --err
insert into tb values (2, 'ff'); --err
insert into tb values (3, 'f');
insert into tb values (4, '');
select * from tb;
drop table tb;


----------- PAD_CHAR_TO_FULL_LENGTH ------------
select length(bpchar_smaller('aaaaac ', 'aaccccc ')); --6
select length(bpchar_smaller('aaaaac        ', 'aaccccc '));  --6
set sql_mode = 'PAD_CHAR_TO_FULL_LENGTH';
select length(bpchar_smaller('aaaaac ', 'aaccccc ')); --7
select length(bpchar_smaller('aaaaac        ', 'aaccccc '));  --14
reset sql_mode; 

create table tb(id int, name text);
create table tb1(id int, name char(10));

insert into tb1 values (1, 'aaaaa');
insert into tb1 values (2, 'aaaa');
insert into tb1 values (3, 'aaa');
insert into tb1 values (4, 'aa');
insert into tb1 values (5, 'a');
insert into tb1 values (6, '');
insert into tb(id, name) select id, name from tb1;
select * from tb;
show ivorysql.compatible_mode ;
select * from tb1;
set mysql.mode_pad_char_to_full = on;
select * from tb1;
set mysql.mode_pad_char_to_full = off;
select name, length(name) from tb;
select repeat(name, '10') from tb;
select text(name), length(text(name)) from tb;
select name(name), length(name(name)) from tb;
select bpcharout(name) from tb1;
select bpchareq(name, 'aaaaa') from tb1;
select bpchareq(name, 'aaaa') from tb1;
select bpchareq(name, 'aaa') from tb1;
select bpchareq(name, 'aa') from tb1;
select bpchareq(name, 'a') from tb1;
select bpchareq(name, '') from tb1;
select bpcharlt(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharle(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpchargt(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharge(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharne(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select t.name > t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select t.name = t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select t.name < t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select t.name <> t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select character_length(name) from tb1;
select char_length(name) from tb1;
select length(name) from tb1;
select octet_length(name) from tb1;
select max(name) from tb1;
select min(name) from tb1;




delete from tb;
set sql_mode = 'PAD_CHAR_TO_FULL_LENGTH';
insert into tb(id, name) select id, name from tb1;
select * from tb;
select name, length(name) from tb;
select repeat(name, '10') from tb;
select text(name), length(text(name)) from tb;
select name(name), length(name(name)) from tb;
select bpcharout(name) from tb1;
select bpchareq(name, 'aaaaa     ') from tb1;
select bpchareq(name, 'aaaa      ') from tb1;
select bpchareq(name, 'aaa       ') from tb1;
select bpchareq(name, 'aa        ') from tb1;
select bpchareq(name, 'a         ') from tb1;
select bpchareq(name, '          ') from tb1;
select bpcharlt(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharle(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpchargt(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharge(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select bpcharne(t1.name, t1.name) from tb t left join tb1 t1 on (t.id = t1.id);
select t.name > t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select t.name = t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select t.name < t1.name from tb t left join tb1 t1 on (t.id = t1.id);
select length(name) from tb1;
select max(name) from tb;
select min(name) from tb;
delete from tb;

drop table tb;
drop table tb1;
reset sql_mode;

----------------- varchar(0) ----------------
CREATE TABLE t6(c1 VARCHAR(0) NULL);
\d t6
create index t6idx on t6(c1); --err
DROP TABLE t6;

CREATE TABLE t6(c1 VARCHAR(0) NOT NULL);
\d t6
create index t6idx on t6(c1); --err
DROP TABLE t6;
create table t6 (b char(0) primary key); --err

create table t6 (
  a int primary key,
  b varchar(0),
  c bit(1)
);
create index t6idx on t6(b); --err
--echo #testing insertion
insert into t6 values(1, '', 0),
                     (2, '', 1),
                     (3, '', NULL),
                     (4, NULL, 0),
                     (5, NULL, 1),
                     (6, NULL, NULL);
select * from t6;
DROP TABLE t6;

create table t6 (b char(0) default 'aaa'); --err
create table t6 (b char(0) default 'aa'); --err
create table t6 (b char(0) default 'a'); --err
create table t6 (b char(0) default ''); --succ
drop table t6;

create table t6 (b char(0) default not null); --err
create table t6 (b char(0) default null); --succ
drop table t6;


create table t6 (b varchar(0));
insert into t6 values ('aaaaa'); --err
insert into t6 values ('aaaa'); --err
insert into t6 values ('aaa'); --err
insert into t6 values ('aa'); --err
insert into t6 values ('a'); --err
insert into t6 values ('');
insert into t6 values (null);
select * from t6;
delete from t6;

set sql_mode = '';
insert into t6 values ('aaaaa');
insert into t6 values ('aaaa');
insert into t6 values ('aaa');
insert into t6 values ('aa');
insert into t6 values ('a');
insert into t6 values ('');
insert into t6 values (null);
select * from t6;
delete from t6;
drop table t6;
reset sql_mode;


create table t6 (b char(0));
insert into t6 values (repeat('a', 5));
insert into t6 values (repeat('a', 4));
insert into t6 values (repeat('a', 3));
insert into t6 values (repeat('a', 2));
insert into t6 values (repeat('a', 1));
select * from t6;
delete from t6;
insert into t6 values (concat('ab', repeat('a', 5)));
insert into t6 values (concat('ab', repeat('a', 4)));
insert into t6 values (concat('ab', repeat('a', 3)));
insert into t6 values (concat('ab', repeat('a', 2)));
insert into t6 values (concat('ab', repeat('a', 1)));
select * from t6;
delete from t6;

set sql_mode = '';
insert into t6 values (repeat('a', 5));
insert into t6 values (repeat('a', 4));
insert into t6 values (repeat('a', 3));
insert into t6 values (repeat('a', 2));
insert into t6 values (repeat('a', 0));
select * from t6;
delete from t6;
insert into t6 values (concat('ab', repeat('a', 5)));
insert into t6 values (concat('ab', repeat('a', 4)));
insert into t6 values (concat('ab', repeat('a', 3)));
insert into t6 values (concat('ab', repeat('a', 2)));
insert into t6 values (concat('ab', repeat('a', 1)));
select * from t6;
delete from t6;
drop table t6;
reset sql_mode;


----------------- fix bug m533 ----------------
set mysql.cast_to_char = on;
select cast('a' as char);
select cast('ab' as char);
select cast('abc' as char);
select cast('abcd' as char);
select cast('abcde' as char);
select cast('abcdef' as char);

select cast(repeat('a', 1) as char);
select cast(repeat('a', 2) as char);
select cast(repeat('a', 3) as char);
select cast(repeat('a', 4) as char);
select cast(repeat('a', 5) as char);

select cast('a'::char(10) as char);
select cast('ab'::char(10) as char);
select cast('abc'::char(10) as char);
select cast('abcd'::char(10) as char);
select cast('abcde'::char(10) as char);
select cast('abcdef'::char(10) as char);

---String cast to char, varchar with length
create table tb as select cast('a' as char);
\d tb
create table tb1 as select cast('ab' as char);
\d tb1
create table tb2 as select cast('abc' as char);
\d tb2
create table tb3 as select cast('abcd' as char);
\d tb3
create table tb4 as select cast('abcde' as char);
\d tb4
create table tb5 as select cast('abcdef' as char);
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;


---Expression cast to char, varchar without length
create table tb as select cast(repeat('a', 1) as char);
\d tb
create table tb1 as select cast(repeat('a', 2) as char);
\d tb1
create table tb2 as select cast(repeat('a', 3) as char);
\d tb2
create table tb3 as select cast(repeat('a', 4) as char);
\d tb3
create table tb4 as select cast(repeat('a', 5) as char);
\d tb4
create table tb5 as select cast(repeat('a', 6) as char);
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;

--nchar
select cast('a' as nchar);
select cast('ab' as nchar);
select cast('abc' as nchar);
select cast('abcd' as nchar);
select cast('abcde' as nchar);
select cast('abcdef' as nchar);
select cast('aaa' as nchar);

select cast(repeat('a', 1) as nchar);
select cast(repeat('a', 2) as nchar);
select cast(repeat('a', 3) as nchar);
select cast(repeat('a', 4) as nchar);
select cast(repeat('a', 5) as nchar);

select cast('a'::nchar(10) as nchar);
select cast('ab'::nchar(10) as nchar);
select cast('abc'::nchar(10) as nchar);
select cast('abcd'::nchar(10) as nchar);
select cast('abcde'::nchar(10) as nchar);
select cast('abcdef'::nchar(10) as nchar);

---String cast to nchar, varchar with length
create table tb as select cast('a' as nchar);
\d tb
create table tb1 as select cast('ab' as nchar);
\d tb1
create table tb2 as select cast('abc' as nchar);
\d tb2
create table tb3 as select cast('abcd' as nchar);
\d tb3
create table tb4 as select cast('abcde' as nchar);
\d tb4
create table tb5 as select cast('abcdef' as nchar);
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;


---Expression cast to nchar, varchar without length
create table tb as select cast(repeat('a', 1) as nchar);
\d tb
create table tb1 as select cast(repeat('a', 2) as nchar);
\d tb1
create table tb2 as select cast(repeat('a', 3) as nchar);
\d tb2
create table tb3 as select cast(repeat('a', 4) as nchar);
\d tb3
create table tb4 as select cast(repeat('a', 5) as nchar);
\d tb4
create table tb5 as select cast(repeat('a', 6) as nchar);
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;


--char(n)
select cast('a' as char(2));
select cast('ab' as char(3));
select cast('abc' as char(4));
select cast('abcd' as char(5));
select cast('abcde' as char(9));
select cast('abcdef' as char(10));

select cast(repeat('a', 1) as char(10));
select cast(repeat('a', 2) as char(10));
select cast(repeat('a', 3) as char(10));
select cast(repeat('a', 4) as char(10));
select cast(repeat('a', 5) as char(10));

---String cast to char, varchar with length
create table tb as select cast('a' as char(2));
\d tb
create table tb1 as select cast('ab' as char(3));
\d tb1
create table tb2 as select cast('abc' as char(4));
\d tb2
create table tb3 as select cast('abcd' as char(5));
\d tb3
create table tb4 as select cast('abcde' as char(9));
\d tb4
create table tb5 as select cast('abcdef' as char(20));
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;


---Expression cast to char, varchar without length
create table tb as select cast(repeat('a', 1) as char(10));
\d tb
create table tb1 as select cast(repeat('a', 2) as char(10));
\d tb1
create table tb2 as select cast(repeat('a', 3) as char(10));
\d tb2
create table tb3 as select cast(repeat('a', 4) as char(10));
\d tb3
create table tb4 as select cast(repeat('a', 5) as char(10));
\d tb4
create table tb5 as select cast(repeat('a', 6) as char(10));
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;


--nchar(n)
create table tb as select cast(repeat('a', 1) as nchar(10));
\d tb
create table tb1 as select cast(repeat('a', 2) as nchar(10));
\d tb1
create table tb2 as select cast(repeat('a', 3) as nchar(10));
\d tb2
create table tb3 as select cast(repeat('a', 4) as nchar(10));
\d tb3
create table tb4 as select cast(repeat('a', 5) as nchar(10));
\d tb4
create table tb5 as select cast(repeat('a', 6) as nchar(10));
\d tb5

drop table tb, tb1, tb2, tb3, tb4, tb5;
set mysql.mode_pad_char_to_full = on;


-- bug#z5074
create table tb(col char(0));
\d+ tb
insert into tb values();
select * from tb;
delete from tb;
insert into tb values(NULL);
select * from tb;
delete from tb;
insert into tb values('');
select * from tb;
delete from tb;
insert into tb values(' ');
select * from tb;
delete from tb;
insert into tb values(1);
select * from tb;
delete from tb;
insert into tb values('a');
select * from tb;
delete from tb;

insert into tb values('');
insert into tb values(' ');
insert into tb values('              ');
select * from tb;
drop table tb;


-- bug#z5108
create table tb (col char(10));
insert into tb values('xy');
select cast(col as char), char_length(cast(col as char)) from tb;
select cast(col as char(0)), char_length(cast(col as char(0))) from tb;
select cast(col as char(10)), char_length(cast(col as char(10))) from tb;
drop table tb;

-- bug#z5124
create table tb (col char(10)); 
insert into tb values('abc');
select convert('abc' using gbk);
select convert('abc' using gb18030);
drop table tb;

-- bug#z5134
create table tb (id int,col char(10)); 
insert into tb values(1,'abc'); 
insert into tb values(2,'123'); 
insert into tb values(3,'123.4');
select convert(col ,unsigned) from tb where id=1;  --0
select convert(col , unsigned) from tb where id=2; --123
select convert(col , unsigned) from tb where id=3; --123
drop table tb;

-- bug#z5148
create table tb(col varchar(0));
\d+ tb
insert into tb values();
insert into tb values(NULL);
insert into tb values('');
insert into tb values(' '); --succ
insert into tb values(1);
insert into tb values('a');
drop table tb;

-- bug#z5149
create table tb (col varchar(10)); 
insert into tb values('xy');
select cast(col as char), char_length(cast(col as char)) from tb; --('xy',2)
select cast(col as char(0)), char_length(cast(col as char(0))) from tb; --(,0)
select cast(col as char(10)), char_length(cast(col as char(10))) from tb; --('xy',2)
drop table tb;

-- bug#z5153
create table tb (id int,col varchar(10));
insert into tb values(1,'abc');
insert into tb values(2,'123');
insert into tb values(3,'123.4');
select convert(col ,unsigned) from tb where id=1; --0
select convert(col , unsigned) from tb where id=2; --123
select convert(col , unsigned) from tb where id=3; --123
set mysql.cast_to_char = off;
set mysql.mode_pad_char_to_full = off;

--bug#z5405
create table tb1(col char(a));	--err
create table tb2(col char(*));	--err
create table tb3(col char(-));	--err
create table tb4(col char());	--err
create table tb4(col char(-1));	--err
create table tb5(col char(256));
create table tb6(col char(0));
create table tb7(col char(10));
create table tb8(col char(255));
create table tb9(col char);
drop table tb5, tb6, tb7, tb8, tb9;