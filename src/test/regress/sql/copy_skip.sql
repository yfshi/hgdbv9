--
-- copy test for skip the special column
--
CREATE TABLE x_bad (
        taskid varchar(64),
        schema varchar(64),
        tablename  varchar(64),
        filename varchar(64),
        type char,
        lineno bigint,
        buff text
) ;
CREATE TABLE copy_skip_1 (
	id int,
	age int,
	buy numeric check(buy > 10),
	cost int not null,
	name varchar(20),
	address text not null
) ;

-- checking skip option, fail
-- using skip in text format
COPY copy_skip_1 FROM STDIN skiplist 4;
\.

-- using skip in binary format
COPY copy_skip_1 FROM STDIN with binary skiplist 4;
\.

-- using skip without value
COPY copy_skip_1 FROM STDIN with csv skiplist;
\.

-- double setting
COPY copy_skip_1 FROM STDIN with csv skiplist 4 skiplist 5;
\.

-- using skip with invalid number
COPY copy_skip_1 FROM STDIN with csv skiplist 0;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 3,0;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist -2;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist -2,2;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 2,-2;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 4,;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist ,4;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 5,5;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist a;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist _;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 4,d;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129;
\.

COPY copy_skip_1 FROM STDIN with csv skiplist 4,1729;
\.

-- skiplist only support COPY FROM
COPY copy_skip_1 TO STDOUT with csv skiplist 4;

-- checking skip option, success
COPY copy_skip_1 FROM STDIN with csv skiplist 5,4,3,2,1,6,7,8,9,10,20;
1,2,3,4,5,6,7,8,9,10,11,11,11.11,11,name_11,11
\.

SELECT * FROM copy_skip_1;
DELETE FROM copy_skip_1;

COPY copy_skip_1 FROM STDIN with csv skiplist 4;
10,1,11.11,xxx,1,name_1,1
20,2,22.22,,2,name_2,2
\.

SELECT * FROM copy_skip_1;

-- success,'4 ' equal '4'
COPY copy_skip_1 FROM STDIN with csv skiplist 4 ;
10,1,11.11,xxx,1,name_1,1
20,2,22.22,,2,name_2,2
\.

SELECT * FROM copy_skip_1;

-- skip the first column
CREATE TABLE copy_skip_2(
	id1 int,id2 int,id3 int,id4 int,id5 int,
	id6 int,id7 int,id8 int,id9 int,id10 int,
	id11 int,id12 int,id13 int,id14 int);

COPY copy_skip_2 FROM STDIN with csv skiplist 1;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
\.

-- skip the latest column
COPY copy_skip_2 FROM STDIN with csv skiplist 15;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
\.

-- skip multi column
COPY copy_skip_2 FROM STDIN with csv skiplist 1,5;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
\.

SELECT * FROM copy_skip_2;

-- missing one column
TRUNCATE copy_skip_2;
COPY copy_skip_2 FROM STDIN with csv skiplist 1,5,10;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
\.

-- set the column name
COPY copy_skip_2(id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11) FROM STDIN with csv skiplist 1,5,10,11,12;
1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
\.

SELECT * FROM copy_skip_2;

-- skip + quote
CREATE TABLE copy_skip_3(
	id int,
	name char(20),
	address varchar(20),
	number int,
	buy numeric);
COPY copy_skip_3 FROM STDIN with csv;
20,kaixin,beijing,20,20.02
30,tiantian,shanghai,30,30.03
\.

COPY copy_skip_3 from STDIN with csv quote '''';
20,kaixin,',beijing',20,20.02
30,'tiantian,','"shanghai',30,30.03
\.

SELECT * FROM copy_skip_3;

COPY copy_skip_3(id,name,number,buy) from STDIN with csv quote '''' skiplist 3;
20,kaixin,',beijing',20,20.02
30,'tiantian,','"shanghai',30,30.03
\.

COPY copy_skip_3(id,name,number,buy) from STDIN with csv skiplist 3;
20,"kaixin,",",beijing",20,20.02
30,"'tiantian'","'shanghai",30,30.03
\.

SELECT * FROM copy_skip_3;

-- skip + tolerate
-- as there is no ' or ",so insert bad record success
TRUNCATE copy_skip_3;
COPY copy_skip_3(id,name,number,buy) from STDIN with csv skiplist 3 taskid 'ihaveadream' filename 'stdin' badtable 'x_bad';
20,kaixin,beijing,20,20.02
30,tiantian,shanghai,30,30.03,111
\.

SELECT * from copy_skip_3;
SELECT * from x_bad where taskid='ihaveadream' order by lineno;

-- fk check error
TRUNCATE x_bad;
CREATE TABLE pk_t (
        pa integer primary key,
        pb integer unique,
        pc varchar(10) unique
) ;
CREATE TABLE fk_t (
        fa integer,
        fb integer REFERENCES pk_t(pb)
) ;

insert into pk_t values(1,1,'aaaa');
insert into pk_t values(2,2,'bbbb');
insert into pk_t values(3,3,'cccc');
insert into pk_t values(11,11,'111111');

COPY fk_t from stdin with csv delimiter ',' taskid 'fk_skip' filename 'fk1.csv' badtable 'x_bad' skiplist 2;
1,xx,1
2,xx,2
10,xx,100
3,xx,3
5,xx,5
\.
SELECT * from fk_t order by fa;;
SELECT * from x_bad where taskid='fk_skip' order by lineno;

DROP TABLE x_bad;
DROP TABLE copy_skip_1;
DROP TABLE copy_skip_2;
DROP TABLE copy_skip_3;
DROP TABLE fk_t,pk_t;

-- skiplist feature support max skipping 128 cloumns
-- if skiplist include number > 1728, raise an error
-- if not enable skiplist, not checking the skiplist[]

create table test_1728(id int);
--- enable the skiplist, skiplist is 1729, raise an error
copy test_1728 from stdin with csv skiplist 1729;
\.

---- not enable skiplist, if column more than 1600+128, raise a error(the same with 14.14)
-- if there have 1730 column
copy test_1728 from stdin with(format csv);
1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3
\.

--- enable the skiplist, but 1730 columns, raise an error
copy test_1728 from stdin with csv skiplist 2,3;
1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3
\.

drop table test_1728;

-- as there have ' or ",so output the record to the bad table
-- this is a bug for tolerate
-- COPY copy_skip_3(id,name,number,buy) from STDIN with csv skiplist 3 taskid 'ihaveadream' filename 'stdin' badfile '/home/postgres/x_bad.log';


------------- skip + gb18030 + gbk
-------- following case refer to contrib/gb18030_2022
create database gb18030_copy encoding='gb18030' LC_COLLATE='C' LC_CTYPE ='C' TEMPLATE=template0;
create database gbk_copy encoding='gbk' LC_COLLATE='C' LC_CTYPE ='C' TEMPLATE=template0;

------------- skip + gb18030
-- GB18030 Copy Test
\c gb18030_copy
load 'gb18030_2022';
show server_encoding;
set client_encoding to utf8;
show client_encoding;
-- 2. copy from csv
create table copy_csv_18030(x text,y text);
copy copy_csv_18030 from STDIN CSV DELIMITER '@' ESCAPE 'g' QUOTE '/';
狜狝狟狢gdasgds/狣㐴㐵㐶㐷㐸㐹㐺㐻㐼狤狥狦/@/狧狪狫狵㒰㒱㒲㒳㒴狶狹狽狾/
狿猀猂猄猅㓢㓣㓤/㓥㓦猆gdsgdsag猇/猈@猉猋/猌猍gfsaf㓿㔀/㔁㔂㔃sa猏猐猑猒
猔猘猙猚gdsgr/ds猟gfdsagds/猠gdsgds猣猤@猦/猧猨gds猭猯猰猲猳/
猵猶猺猻猼/猽獀/獁@獂獃/獄獅獆/獇獈　
獉獊獋獌/獎獏/獑獓@gd/sgr/as
獝獞獟獡獢/獣gda獤/獥@獦獧/獨獩gdsgds獪獫獮獰/
gdagg/ds獱/@1f/asfs/
\.

select * from copy_csv_18030;

copy copy_csv_18030(y) from STDIN CSV DELIMITER '@' ESCAPE 'g' QUOTE '/' skiplist 1;
狜狝狟狢gdasgds/狣㐴㐵㐶㐷㐸㐹㐺㐻㐼狤狥狦/@/狧狪狫狵㒰㒱㒲㒳㒴狶狹狽狾/
狿猀猂猄猅㓢㓣㓤/㓥㓦猆gdsgdsag猇/猈@猉猋/猌猍gfsaf㓿㔀/㔁㔂㔃sa猏猐猑猒
猔猘猙猚gdsgr/ds猟gfdsagds/猠gdsgds猣猤@猦/猧猨gds猭猯猰猲猳/
猵猶猺猻猼/猽獀/獁@獂獃/獄獅獆/獇獈　
獉獊獋獌/獎獏/獑獓@gd/sgr/as
獝獞獟獡獢/獣gda獤/獥@獦獧/獨獩gdsgds獪獫獮獰/
gdagg/ds獱/@1f/asfs/
\.

select * from copy_csv_18030;
drop table copy_csv_18030;

----------- skip + gbk
-- GBK Copy Test
\c gbk_copy
load 'gb18030_2022';
show server_encoding;
set client_encoding to utf8;
show client_encoding;

-- 2. copy from csv
create table copy_csv_gbk(x text,y text);
copy copy_csv_gbk from STDIN CSV DELIMITER '@' ESCAPE 'g' QUOTE '/';
狜狝狟狢狣/猣/@獎獏獑獓gl猇猈猉
copy /test g\/@獎/獏獑獓/獔獕獖
\.

select * from copy_csv_gbk;

copy copy_csv_gbk(x) from STDIN CSV DELIMITER '@' ESCAPE 'g' QUOTE '/' skiplist 2;
狜狝狟狢狣/猣/@獎獏獑獓gl猇猈猉
copy /test g\/@獎/獏獑獓/獔獕獖
\.

select * from copy_csv_gbk;

drop table copy_csv_gbk;

\c postgres
drop database gbk_copy;
drop database gb18030_copy;
