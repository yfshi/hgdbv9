create table t (c bit);
\d t;
drop table t;
create table t (c bit(0));
create table t (c bit(65));
create table t (c bit(32));
\d t;

insert into t values(1234);
insert into t values('abcd');
insert into t values(true);
insert into t values('11');
insert into t values(0x12345678);
insert into t values(0x12005678);

COPY t FROM stdin;
0x12
0x1234
0x12340078
0x12345678
\.

select * from t;
select c > c from t;
\gdesc
select c >= c from t;
\gdesc
select c < c from t;
\gdesc
select c <> c from t;
\gdesc
select c <= c from t;
\gdesc
select c <=> c from t;
\gdesc
select c = c from t;
\gdesc
select c like c from t;
\gdesc
select c not like c from t;
\gdesc
select c & c from t;
\gdesc
select c >> c from t;
\gdesc
select c << c from t;
\gdesc
select c ^ c from t;
\gdesc
select c | c from t;
\gdesc
select ~c from t;
\gdesc
select bit_count(c) from t;
select BIT_COUNT(c) from t;

select +c from t;
\gdesc
select -c from t;
\gdesc

select c % c from t;
\gdesc
select c mod c from t;
\gdesc
select c + c from t;
\gdesc
select c - c from t;
\gdesc
select c / c from t;
\gdesc
select c div c from t;
\gdesc
select c * c from t;
\gdesc

select c and c from t;
\gdesc
select c && c from t;
\gdesc
select !c from t;
\gdesc
select not c from t;
\gdesc
select c or c from t;
\gdesc
select c || c from t;
\gdesc
select c xor c from t;
\gdesc

select binary(c) from t;
select c + 0 from t;
select c is true from t;
select c is false from t;
select c is not true from t;
select c is not false from t;
drop table t;

create table t (c bit(5));
insert into t values(b'1010');
insert into t values('a');
select * from t;
drop table t;

create table t (c bit(10));
insert into t values(b'1010');
insert into t values('a');
select * from t;
drop table t;

create table t (c bit(18));
insert into t values(b'1010');
insert into t values('ab');
select * from t;
drop table t;

CREATE TABLE BIT_TABLE(b BIT(11));
INSERT INTO BIT_TABLE VALUES (B'10'); -- too short
INSERT INTO BIT_TABLE VALUES (B'00000000000');
INSERT INTO BIT_TABLE VALUES (B'11011000000');
INSERT INTO BIT_TABLE VALUES (B'01010101010');
INSERT INTO BIT_TABLE VALUES (B'101011111010'); -- too long
SELECT * FROM BIT_TABLE;
drop table BIT_TABLE;

