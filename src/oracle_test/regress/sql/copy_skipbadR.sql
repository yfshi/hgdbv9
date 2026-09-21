-- Begin - BUG#Z1616
--
-- copy test for skip bad record without output
--
CREATE TABLE copy_skipbad (
	id int unique,
	age int,
	buy numeric check(buy > 10),
	cost int not null,
	name varchar(20),
	address text not null
) ;

-- checking skip option, fail
-- using skip in text format
COPY copy_skipbad FROM STDIN with(skipbad);
\.

-- using skip in binary format
COPY copy_skipbad FROM STDIN with(format binary, skipbad);
\.

-- double setting
COPY copy_skipbad FROM STDIN with(format csv, skipbad, skipbad);
\.

-- using skipbad with or without taskid and filename is fine
-- skipbad only support COPY FROM
COPY copy_skipbad TO STDOUT with(format csv, skipbad);

-- checking skip option, success
-- Check Reading error, ask ',', input '|'
-- Check input function call error, ask int b, input varchar
COPY copy_skipbad FROM STDIN with(format csv, skipbad, taskid 'xxxyyy');
1,1,11.11,1,name_1,1
2|2|22.22|2|name_2|2
3,3,33.33|3|name_3|3
4,df,44.44,4,name_4,4
5,5,55.55,5,name_5,5
\.
SELECT * FROM copy_skipbad;

DELETE FROM copy_skipbad;

-- Constraints
-- Check Constraints numeric buy>10
-- Not-Null Constraints
COPY copy_skipbad FROM stdin with(format csv, delimiter ',', skipbad);
10,10,1.11,10,name_10,10
20,20,10,20,name_20,20
2,2,22.22,2,,2
3,3,33.3,,name_3,3
,4,44.4,4,name_4,4
5,5,55.5,5,name_5,
\.
SELECT * FROM copy_skipbad;

-- Unique Constraints
DELETE FROM copy_skipbad;
INSERT INTO copy_skipbad values(7,7,77.77,7,'name_7',7);

COPY copy_skipbad FROM stdin with(format csv, skipbad, logbad 'true');
6,6,66.6,6,name_6,6
7,7,77.7,7,name_7,7
8,8,88.8,8,name_8,8
\.
SELECT * FROM copy_skipbad;

-- Partition table, RANGE | LIST | HASH
-- PARTITION BY HASH
CREATE TABLE xph (
        a int,
        b int,
        c text not null,
        d varchar(10)
) PARTITION BY HASH (a) ;
CREATE TABLE xph_0 partition of xph FOR VALUES WITH (MODULUS 4, REMAINDER 0);
CREATE TABLE xph_1 partition of xph FOR VALUES WITH (MODULUS 4, REMAINDER 1);
CREATE TABLE xph_2 partition of xph FOR VALUES WITH (MODULUS 4, REMAINDER 2);
CREATE TABLE xph_3 partition of xph FOR VALUES WITH (MODULUS 4, REMAINDER 3);


COPY xph(a, b, c, d) FROM stdin with(format csv, delimiter ',', skipbad); 
1,1,name_1,1111111111
2,2,name_2,2222222222
3,3,name_3,3333333333
4,4,name_4,4444444444
5,5,name_5,555555555555
6,6,,6666666666
7,7,name_7,7777777777
7,8,name_8,88888888888888888
\.
SELECT * FROM xph;
SELECT * FROM xph_0;
SELECT * FROM xph_1;
SELECT * FROM xph_2;
SELECT * FROM xph_3;

-- Trigger
-- badtable feature should responsibility for this

-- sharding
CREATE TABLE xshd (
        a int primary key,
        b varchar(10)
) ;

COPY xshd(a, b) FROM stdin with(format csv, delimiter ',', skipbad, sharding 3, current_shard 0);
5,ffff
7,gggg
\.
SELECT * from xshd;

-- fk check error
-- FK check, non-partition table
-- delimiter ,
CREATE TABLE pk_t (
        pa integer primary key,
        pb integer unique,
        pc integer unique,
        pd varchar(10) unique
) ;
CREATE TABLE fk_t (
        fa integer,
        fb integer REFERENCES pk_t(pb)
) ;

insert into pk_t values(1,1,1,'aaaa');
insert into pk_t values(2,2,2,'bbbb');
insert into pk_t values(3,3,3,'cccc');
insert into pk_t values(10,10,10,'101010');
insert into pk_t values(11,11,11,'111111');

COPY fk_t FROM stdin with(format csv, delimiter ',', skipbad);
1,1
2,2
10,100
3,3
5,5
\.
SELECT * from fk_t order by fa;;

-- fk is unique(a,b)
CREATE TABLE pk_t_u (
        pa integer primary key,
        pb integer,
        pc integer,
        pd varchar(10) unique,
        UNIQUE(pb, pc)
) ;

CREATE TABLE fk_t_u (
        fa integer,
        fb integer,
        fc integer,
        FOREIGN KEY (fb, fc) REFERENCES pk_t_u (pb,pc)
) ;
insert into pk_t_u values(1,1,10,'aaaa');
insert into pk_t_u values(2,2,20,'bbbb');
insert into pk_t_u values(3,3,30,'cccc');

COPY fk_t_u FROM stdin with(format csv, delimiter ',', skipbad, taskid 'abcd', logbad 'true');
1,2,20
2,2,20
3,30,300
4,3,30
\.
SELECT * from fk_t_u order by fa;

-- clean up
DROP TABLE copy_skipbad;
DROP TABLE xph,xph_0,xph_1,xph_2,xph_3;
DROP TABLE xshd;
DROP TABLE fk_t,pk_t;
DROP TABLE pk_t_u,fk_t_u;

-- End - BUG#Z1616

-- Begin - BUG#M0000526
create table xxxx(n1 varchar2(100), n2 varchar2(100), n3 varchar2(100), n4 varchar2(100), n5 varchar2(100));
-- more clomun than expected, skipped
copy xxxx from stdin with(format csv, delimiter '|');
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012|345678
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012
\.

copy xxxx from stdin with(format csv, delimiter '|', skipbad);
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012|345678
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012
\.

select * from xxxx;

-- less cloumn than expected, skipped
delete from xxxx;
copy xxxx from stdin with(format csv, delimiter '|');
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012
\.

copy xxxx from stdin with(format csv, delimiter '|', skipbad);
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90
15033479888@kuandai|000001|111.63.248.12|82:e8:3d:74:45:90|REJT:109026012
\.

select * from xxxx;
delete from xxxx;
-- if there have one quote, return an error
copy xxxx from stdin with (format csv, delimiter '|');
15033479888@kuandai|000001|111.63.248.12|"82:e8:3d:74:45:90|REJT:109026012
18833877712@kuandai|000001|111.63.250.22|78:46:5f:24:e1:11|REJT:109026012
\.


copy xxxx from stdin with (format csv, delimiter '|', skipbad);
15033479888@kuandai|000001|111.63.248.12|"82:e8:3d:74:45:90|REJT:109026012
18833877712@kuandai|000001|111.63.250.22|78:46:5f:24:e1:11|REJT:109026012
\.


copy xxxx from stdin with (format csv, delimiter '|');
15033479888@kuandai|000001|111.63.248.12|"82:e8:3d:74:45:90|REJT:109026012
18833877712@kuandai|000001|111.63.250.22|78:46:5f:24:e1:11"|REJT:109026012
\.

copy xxxx from stdin with (format csv, delimiter '|', skipbad);
15033479888@kuandai|000001|111.63.248.12|"82:e8:3d:74:45:90|REJT:109026012
18833877712@kuandai|000001|111.63.250.22|78:46:5f:24:e1:11"|REJT:109026012
\.

select * from xxxx;
-- if no column for a table, should skip all lines when enable skipbad
create table no_column();
copy no_column from stdin with(format csv, delimiter '|');
1|2|3
\.

copy no_column from stdin with(format csv, delimiter '|', skipbad);
1|2|3
asdf
T&Y
\.

drop table xxxx;
drop table no_column;
-- End - BUG#M0000526
