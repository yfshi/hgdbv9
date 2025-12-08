--create index 
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE INDEX t10_1 on t10 (id);
CREATE INDEX t10_2 on t10 (id, str);
\d+ t10
drop table t10;

--create index unique/fulltext/spatial
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE UNIQUE INDEX t10_1 on t10 (id);
CREATE UNIQUE INDEX t10_2 on t10 (id, str);
CREATE UNIQUE INDEX t10_3 on t10 (str(3));

\d+ t10
drop index t10_1,t10_2,t10_3;

CREATE FULLTEXT INDEX t10_3 on t10 (id);
CREATE FULLTEXT INDEX t10_4 on t10 (id, str);
\d+ t10
drop index t10_3,t10_4;

CREATE SPATIAL INDEX t10_5 on t10 (id);
CREATE SPATIAL INDEX t10_6 on t10 (id, str);
\d+ t10
drop index t10_5,t10_6;
drop table t10;

--create index USING BTREE/HASH
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE INDEX t10_1 USING BTREE on t10 (id);
CREATE INDEX t10_2 USING BTREE on t10 (id, str);
CREATE INDEX t10_3 USING HASH on t10 (id);
CREATE INDEX t10_4 USING HASH on t10 (id, str); --error: pg not support ,but mysql can
CREATE UNIQUE INDEX t10_5 USING HASH on t10 (id);  --error: pg not support ,but mysql can
CREATE INDEX t10_6 USING XXXX on t10 (id, str); --error
CREATE INDEX t10_7 on t10 (id, str) USING XXXX; --error

\d+ t10
drop table t10;

--colanme(length)
create table t11(id int, str1 varchar(10), str2 varchar(10));
create index t11_1 on t11(str1(5));
create index t11_2 on t11(str1(5), str2(6));
create index t11_3 on t11(id ,str1(5), str2(6));
create index t11_4 on t11(str1(5),id, str2(6));
create index t11_5 on t11(str1(5), str2(6), id);
create index t11_6 on t11(str1(5), str2(6), max(id)); --error
/*Begin - BUG#Z6293 */
create index t11_7 on t11(str1(0)); --error
create index t11_8 on t11(str1(11)); --error
create index t11_10 on t11(str1('10')); --error
create index t11_9 on t11(str1(10));
/*End - BUG#Z6293 */
\d+ t11
drop table t11;

--Begin - BUG#Z6657
CREATE TABLE test_table (
    id INT PRIMARY KEY,
    tinytext_column TINYTEXT,
    text_column TEXT,
    mediumtext_column MEDIUMTEXT,
    longtext_column LONGTEXT
);

CREATE INDEX idx_tinytext ON test_table (tinytext_column(86)); --error
CREATE INDEX idx_tinytext ON test_table (tinytext_column(85));
CREATE INDEX idx_text ON test_table (text_column(1025)); --error
CREATE INDEX idx_text ON test_table (text_column(1024));
CREATE INDEX idx_mediumtext ON test_table (mediumtext_column(1025)); --error
CREATE INDEX idx_mediumtext ON test_table (mediumtext_column(1024));
CREATE INDEX idx_longtext ON test_table (longtext_column (1025)); --error
CREATE INDEX idx_longtext ON test_table (longtext_column (1024));

--End - BUG#Z6657

create table t11(id int, str1 varchar(10), str2 varchar(10));
create UNIQUE index t11_7 on t11 (str1(6));
insert into t11 values(1, 'aaaaaa', 'bbbbb');
insert into t11 values(1, 'aaaaab', 'bbbbb');
insert into t11 values(2, 'aaaaaa', 'cccccc'); --error
drop table t11;

create table t11(abs varchar(10), str varchar(10));
/* when colname the same as function name. choice colname first */
create index t11_1 on t11(abs(6));
/* when SQL has function, chioce pg processing method */
create index t11_2 on t11(abs(6), length(abs));
\d+ t11
drop table t11;

--colanme(length) ASC/DESC
create table t11(id1 int, id2 int, str varchar(10));
create index t11_1 on t11(id1 ASC);
create index t11_2 on t11(id1 ASC, id2 DESC);
create index t11_3 on t11(id1 ASC, id2 DESC, str DESC);
\d+ t11
drop table t11;

--create index KEY_BLOCK_SIZE/USING/WITH PARSER/COMMENT
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE INDEX t10_1 on t10 (id) KEY_BLOCK_SIZE=1;
CREATE INDEX t10_2 on t10 (id) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM;
CREATE INDEX t10_3 on t10 (id) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM COMMENT 'test test test';
CREATE INDEX t10_4 on t10 (id) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM COMMENT 'test test test';
CREATE INDEX t10_5 on t10 (id) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM COMMENT 'test test test' USING BTREE;
CREATE INDEX t10_6 on t10 (id) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM COMMENT 'test test test' USING HASH;
CREATE INDEX t10_7 USING BTREE on t10 (id) USING HASH;
CREATE INDEX t10_8 USING HASH on t10 (id) USING BTREE;

CREATE INDEX t10_9 on t10 (id) with (deduplicate_items = off) USING BTREE; -- error

\d+ t10
drop table t10;

--create index algorithm
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE INDEX t10_1 on t10 (id) ALGORITHM = DEFAULT;
CREATE INDEX t10_2 on t10 (id) ALGORITHM = INPLACE;
CREATE INDEX t10_2 on t10 (id) ALGORITHM = COPY;
\d+ t10
drop table t10;

--create index algorithm
create table t10(id int, str varchar(10));
insert into t10 values(1,'aa');
insert into t10 values(2,'bb');
insert into t10 values(3,'cc');
insert into t10 values(4,'dd');
insert into t10 values(10,'aabbcc');
insert into t10 values(20,'bbccdd');
insert into t10 values(5,'qqaazz');
CREATE INDEX t10_1 on t10 (id) LOCK = DEFAULT;
CREATE INDEX t10_2 on t10 (id) LOCK = NONE;
CREATE INDEX t10_3 on t10 (id) LOCK = SHARED;
CREATE INDEX t10_4 on t10 (id) LOCK = EXCLUSIVE;
\d+ t10
drop table t10;

create table t10(id int, str varchar(10));
CREATE UNIQUE INDEX t10_1 on t10 (id DESC, str(5)) KEY_BLOCK_SIZE=1 WITH PARSER MyISAM COMMENT 'test test test' USING BTREE ALGORITHM = DEFAULT LOCK = DEFAULT;
\d+ t10
drop table t10;

/* Begin - bug#5180 */
create table tb_blob1(c1 blob, index(c1(10)));
drop table tb_blob1;
/* End - bug#5180 */
