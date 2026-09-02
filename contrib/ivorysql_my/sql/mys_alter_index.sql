--------- ADD INDEX/KEY
--------- ADD {INDEX | KEY} [index_name] [index_type] (key_part,...) [index_option]
------ ADD {INDEX | KEY} [index_name]
-- no index_name
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b);
show create table test_index;
show index from test_index;
drop table test_index;

-- ignore index_name
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index b_idx (b);
show create table test_index;
show index from test_index;
drop table test_index;

------ ADD {INDEX | KEY} [index_type]
-- test: default using btree
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index(b);
show create table test_index;
show index from test_index;
\d+ test_index;
drop table test_index;

-- test: using btree (col)
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using btree (b);
\d+ test_index
drop table test_index;

-- test: using hash (col)
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b);
show create table test_index;
show index from test_index;
\d+ test_index
drop table test_index;

------ ADD {INDEX | KEY} (key_part,...)
------ (key_part,...) is (col ASC|DESC)
-- test: using btree (col ASC)
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using btree (b ASC);
show create table test_index;
show index from test_index;
\d+ test_index
drop table test_index;

-- test: using btree (col DESC)
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using btree (b DESC);
show create table test_index;
show index from test_index;
\d+ test_index
drop table test_index;

-- test: using hash (col ASC)
-- mysql succ, v9 error as hash did not suppot collate
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b ASC);
show create table test_index;
show index from test_index;
\d+ test_index
drop table test_index;

-- test: using hash (col DESC)
-- mysql succ, v9 error as hash did not suppot collate
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b ASC);
show create table test_index;
show index from test_index;
\d+ test_index
drop table test_index;

------ (key_part,...)
-- c(10) -- mysql succ, v9 error
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (c(10));
\d+ test_index
drop table test_index;

-- ASC, not showing is asc default
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (a ASC);
\d+ test_index
drop table test_index;

-- DESC, showing b DESC
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b DESC);
\d+ test_index
drop table test_index;

-- ASC DESC, error
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (a ASC DESC);
\d+ test_index
drop table test_index;

-- b(10) DESC -- error
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b(10) DESC);
\d+ test_index
drop table test_index;

-- abs(b) -- mysql error, v9 succ
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (abs(b));
\d+ test_index
drop table test_index;

------ (key_part, key_part)
-- int, int
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (a, b);
\d+ test_index
show index from test_index;
drop table test_index;

-- int, varchar
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b, c);
\d+ test_index
show index from test_index;
drop table test_index;

-- int ASC, int DESC
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (a ASC, b DESC);
\d+ test_index
show index from test_index;
drop table test_index;

-- int DESC, varchar DESC
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b DESC, c DESC);
\d+ test_index
show index from test_index;
drop table test_index;

------ ADD [index_option]
-- [index_option]: KEY_BLOCK_SIZE
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) KEY_BLOCK_SIZE 200;
\d+ test_index
drop table test_index;

-- [index_option]: WITH PARSER
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) with parser MyISAM;
\d+ test_index
drop table test_index;

-- [index_option]: COMMENT
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) with parser MyISAM COMMENT 'test add index comment';
\d+ test_index
show index from test_index;
drop table test_index;

-- [index_option]: using btree
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) using btree;
\d+ test_index
drop table test_index;

-- [index_option]: using hash
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) using hash;
\d+ test_index
drop table test_index;

------ ADD {INDEX | KEY} [index_type] (col) [index_option]
------ if index_option has USING, use it
-- using hash, using btree
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b) using btree;
\d+ test_index
drop table test_index;

-- using btree, using hash
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using btree (b) using hash;
\d+ test_index
drop table test_index;

-- using btree, using btree
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using btree (b) using btree;
\d+ test_index
drop table test_index;

-- using hash, using hash
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b) using hash;
\d+ test_index
drop table test_index;

-- using hash (a AESC) using hash
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (a ASC) using hash;
\d+ test_index
drop table test_index;

-- using hash (a DESC) using hash
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index using hash (b DESC) using hash;
\d+ test_index
drop table test_index;

-- all option fields
CREATE TABLE test_index(a int, b int, c varchar(20));
ALTER TABLE test_index add index (b) using btree KEY_BLOCK_SIZE 300 with parser MyISAM COMMENT 'test add index all fields';
\d+ test_index
show index from test_index;
drop table test_index;


--------- ADD {FULLTEXT | SPATIAL} [INDEX | KEY]
--------- ADD {FULLTEXT | SPATIAL} [INDEX | KEY] [index_name] (key_part,...) [index_option]
----- ADD {FULLTEXT | SPATIAL} [INDEX | KEY]
-- ADD {FULLTEXT} [INDEX]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT index (a) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {FULLTEXT} [KEY]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT key (b) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL} [INDEX]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL index (a) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL} [KEY]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL key (b) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {FULLTEXT}
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT (a) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL}
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL (b) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

----- ADD {FULLTEXT | SPATIAL}  [index_name]
-- ADD {FULLTEXT} [index_name]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT idx_name(a) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL} [index_name]
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL idx_name(b) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

----- ADD {FULLTEXT | SPATIAL}  [index_name] (key_part,...)
-- ADD {FULLTEXT} [index_name] b(10) -- error
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT idx_name(b(10)) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {FULLTEXT} [index_name] b ASC
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT idx_name(b ASC) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL} [index_name] b DESC
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL idx_name(c DESC) ;
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

------ ADD {FULLTEXT | SPATIAL} (key_part,...) [index_option]
-- ADD {FULLTEXT} [index_name] b ASC
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add FULLTEXT idx_name(b ASC) using btree KEY_BLOCK_SIZE 300 with parser MyISAM COMMENT 'test add full index';
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

-- ADD {SPATIAL} [index_name] c DESC using hash
CREATE TABLE test_fullindex(a int, b int, c varchar(20));
ALTER TABLE test_fullindex add SPATIAL idx_name(c DESC) using hash KEY_BLOCK_SIZE 300 with parser MyISAM COMMENT 'test add spital index';
\d+ test_fullindex
show index from test_fullindex;
drop table test_fullindex;

------------- ADD UNIQUE KEY TEST CASE
------------- ADD [CONSTRAINT [symbol]] UNIQUE [INDEX|KEY] [index_name] [index_type] (key_part,...) [index_option]

------ UNIQUE [INDEX|KEY]
-- UNIQUE index
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY (a);
\d+ add_uk_t3
show create table add_uk_t3;
drop table add_uk_t3;

-- UNIQUE key
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY (a);
\d+ add_uk_t3
drop table add_uk_t3;

-- UNIQUE
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE (a);
\d+ add_uk_t3
drop table add_uk_t3;

------ [index_name]
-- ADD UNIQUE KEY, ignore the index_name
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY uk_3 (a);
\d+ add_uk_t3
drop table add_uk_t3;

-- ADD UNIQUE, ignore the index_name
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE uk_3 (a);
\d+ add_uk_t3
drop table add_uk_t3;

------ [index_type]
-- default
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY (a);
\d+ add_uk_t3
drop table add_uk_t3;

-- ignore index_type(using btree)
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY using btree (a);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY using btree (a);
\d+ add_uk_t3
drop table add_uk_t3;

-- ignore index_type(using hash)
------ using btree default because hash did not support unique
CREATE TABLE add_uk_t3 (a int, b int);
ALTER TABLE add_uk_t3 ADD UNIQUE KEY using hash (a);
\d+ add_uk_t3
show index from add_uk_t3;
drop table add_uk_t3;

------ [index_option]
------ [index_option]: KEY_BLOCK_SIZE
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique key (b) key_block_size 100;
\d+ add_uk_t3
drop table add_uk_t3;

------ [index_option]: WITH PARSER
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique key (b) key_block_size 100 with parser MyISAM;
\d+ add_uk_t3
drop table add_uk_t3;

------ [index_option]: COMMENT
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique index (b) key_block_size 100 with parser MyISAM comment 'test add unique key comment';
\d+ add_uk_t3
show index from add_uk_t3;
drop table add_uk_t3;

------ [index_type][index_option]
-- using btree using btree
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique key using btree (b) using btree;
\d+ add_uk_t3
drop table add_uk_t3;

-- using hash using hash
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique key using hash (b) using hash;
\d+ add_uk_t3
drop table add_uk_t3;

-- using btree using hash
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add unique key using btree (b) using hash;
\d+ add_uk_t3
drop table add_uk_t3;

------ (key_part): col_name [(length)] ASC| DESC
-- col_name [(length)], error
create table add_uk_t1 (a int, b int, c varchar(20));
alter table add_uk_t1 add unique key (c(10));
\d+ add_uk_t1
show index from add_uk_t1;
drop table add_uk_t1;

-- col_name ASC
create table add_uk_t1 (a int, b int, c varchar(20));
alter table add_uk_t1 add primary key (a ASC);
\d+ add_uk_t1
show index from add_uk_t1;
drop table add_uk_t1;

-- col_name DESC
create table add_uk_t1 (a int, b int, c varchar(20));
alter table add_uk_t1 add primary key (b DESC);
\d+ add_uk_t1
show index from add_uk_t1;
drop table add_uk_t1;

-- int DESC, int ASC
create table add_uk_t1 (a int, b int, c varchar(20));
alter table add_uk_t1 add primary key (b DESC, a ASC);
\d+ add_uk_t1
\d+ add_uk_t1_pkey
show index from add_uk_t1;
drop table add_uk_t1;

-- int DESC, varchar DESC
create table add_uk_t1 (a int, b int, c varchar(20));
alter table add_uk_t1 add primary key (b DESC, c DESC);
\d+ add_uk_t1
\d+ add_uk_t1_pkey
show index from add_uk_t1;
drop table add_uk_t1;

------ ADD [CONSTRAINT [symbol]] UNIQUE
-- ADD [CONSTRAINT symbol] UNIQUE
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add constraint ukkkkkk unique key using btree (b) using hash;
\d+ add_uk_t3
drop table add_uk_t3;

-- ADD [CONSTRAINT] UNIQUE
create table add_uk_t3 (a int, b int);
alter table add_uk_t3 add constraint unique key using btree (b) using hash;
\d+ add_uk_t3
drop table add_uk_t3;

------------- ADD PRIMARY KEY TEST CASE
------------- ADD [CONSTRAINT [symbol]] PRIMARY KEY [index_type] (key_part,...)[index_option]
------ [CONSTRAINT [symbol]] PRIMARY KEY
-- [CONSTRAINT symbol] PRIMARY KEY
CREATE TABLE add_pk_t2 (a int, b int);
ALTER TABLE add_pk_t2 ADD CONSTRAINT add_pk_sym PRIMARY KEY (b);
\d+ add_pk_t2
show create table add_pk_t2;
show index from add_pk_t2;
drop table add_pk_t2;

-- [CONSTRAINT] PRIMARY KEY
CREATE TABLE add_pk_t2 (a int, b int);
ALTER TABLE add_pk_t2 ADD CONSTRAINT PRIMARY KEY (b);
\d+ add_pk_t2
drop table add_pk_t2;

-- PRIMARY KEY
CREATE TABLE add_pk_t2 (a int, b int);
ALTER TABLE add_pk_t2 ADD PRIMARY KEY (b);
\d+ add_pk_t2
drop table add_pk_t2;

----- ADD more than one PK
CREATE TABLE add_pk_t2 (a int, b int);
ALTER TABLE add_pk_t2 ADD PRIMARY KEY (b);
ALTER TABLE add_pk_t2 ADD PRIMARY KEY (b);
\d+ add_pk_t2
drop table add_pk_t2;

----- ADD more than one PK in one SQL
CREATE TABLE add_pk_t2 (a int, b int);
ALTER TABLE add_pk_t2 ADD PRIMARY KEY (b), ADD PRIMARY KEY (a);
\d+ add_pk_t2
drop table add_pk_t2;

------ [index_type] USIN btree | USING HASH
------ [index_type]: using btree
-- default using btree
CREATE TABLE add_pk_t1 (a int, b int);
ALTER TABLE add_pk_t1 ADD PRIMARY KEY (a);
show create table add_pk_t1;
show index from add_pk_t1;
\d+ add_pk_t1
drop table add_pk_t1;

-- add pk using btree (a)
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using btree (a) ;
\d+ add_pk_t1
drop table add_pk_t1;

-- add pk(a) using btree
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key (a) using btree;
\d+ add_pk_t1
drop table add_pk_t1;

-- add pk using btree (a) using btree
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using btree (a) using btree;
\d+ add_pk_t1
drop table add_pk_t1;

------ [index_type]: using hash
------ using btree default because hash did not support unique
-- add pk using hash (a)
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using hash (a)  ;
\d+ add_pk_t1
show create table add_pk_t1;
show index from add_pk_t1;
drop table add_pk_t1;

-- add pk(a) using hash
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key (a) using hash ;
\d+ add_pk_t1
drop table add_pk_t1;

-- add pk using hash (a) using hash
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using hash (a) using hash ;
\d+ add_pk_t1
drop table add_pk_t1;

------ [index_type][index_option]: using btree and using hash
-- btree, hash
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using btree (a) using hash ;
\d+ add_pk_t1
drop table add_pk_t1;

-- hash, btree
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key using hash (a) using btree ;
\d+ add_pk_t1
drop table add_pk_t1;

------ [index_option]: KEY_BLOCK_SIZE | WITH PARSER | COMMENT
-- KEY_BLOCK_SIZE
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key (a) key_block_size 100;
\d+ add_pk_t1
drop table add_pk_t1;

-- WITH PARSER
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key (a) key_block_size 100 with parser MyISAM;
\d+ add_pk_t1
drop table add_pk_t1;

-- COMMENT
create table add_pk_t1 (a int, b int);
alter table add_pk_t1 add primary key (a) key_block_size 100 with parser MyISAM comment 'test add pk comment';
\d+ add_pk_t1
show index from add_pk_t1;
drop table add_pk_t1;

------ (key_part): col_name [(length)] ASC|DESC
-- col_name [(length)], error
create table add_pk_t1 (a int, b int, c varchar(20));
alter table add_pk_t1 add primary key (c(10));
\d+ add_pk_t1
show index from add_pk_t1;
drop table add_pk_t1;

-- col_name ASC
create table add_pk_t1 (a int, b int, c varchar(20));
alter table add_pk_t1 add primary key (a ASC);
\d+ add_pk_t1
show index from add_pk_t1;
drop table add_pk_t1;

-- col_name DESC
create table add_pk_t1 (a int, b int, c varchar(20));
alter table add_pk_t1 add primary key (b DESC);
\d+ add_pk_t1
show index from add_pk_t1;
drop table add_pk_t1;

-- int DESC, int ASC
create table add_pk_t1 (a int, b int, c varchar(20));
alter table add_pk_t1 add primary key (b DESC, a ASC);
\d+ add_pk_t1
\d+ add_pk_t1_pkey
show index from add_pk_t1;
drop table add_pk_t1;

-- int DESC, varchar DESC
create table add_pk_t1 (a int, b int, c varchar(20));
alter table add_pk_t1 add primary key (b DESC, c DESC);
\d+ add_pk_t1
\d+ add_pk_t1_pkey
show index from add_pk_t1;
drop table add_pk_t1;

---------- ADD FOREIGN KEY
---------- ADD [CONSTRAINT [symbol]] FOREIGN KEY [index_name] (col_name,...) reference_definition
------ [CONSTRAINT]
CREATE TABLE fk_idx_parent(id int primary key);
CREATE TABLE fk_idx_child(id int, pid int);
alter table fk_idx_child ADD CONSTRAINT foreign key fk_ignore_idx (pid) references fk_idx_parent(id);
\d+ fk_idx_child
drop table fk_idx_child;
drop table fk_idx_parent;

------ [CONSTRAINT symbol]
-- conn_idx fk
CREATE TABLE fk_idx_parent_2(id int primary key);
CREATE TABLE fk_idx_child_2(id int, pid int);
alter table fk_idx_child_2 ADD CONSTRAINT conn_idx foreign key (pid) references fk_idx_parent_2(id);
\d+ fk_idx_child_2
drop table fk_idx_child_2;
drop table fk_idx_parent_2;

------ [index_name]
-- ignore index_name
CREATE TABLE fk_idx_parent(id int primary key);
CREATE TABLE fk_idx_child(id int, pid int);
ALTER TABLE fk_idx_child ADD foreign key fk_ignore_idx (pid) references fk_idx_parent(id);
\d+ fk_idx_child
drop table fk_idx_child;
drop table fk_idx_parent;

------ [CONSTRAINT [symbol]] FOREIGN KEY [index_name]
-- conn_idx fk
create table fk_idx_parent_2(id int primary key);
create table fk_idx_child_2(id int, pid int);
ALTER TABLE fk_idx_child_2 ADD CONSTRAINT conn_idx foreign key fk_ignore_idx (pid) references fk_idx_parent_2(id);
\d+ fk_idx_child_2
drop table fk_idx_child_2;
drop table fk_idx_parent_2;

--------- ADD CHECK： mysql just syntax support, did not run the logical
-- if add check (a>10)， when insert values, dit not check the value

--------- DROP INDEX
-- drop index
CREATE TABLE drop_idx_t1 (a int, b int, INDEX idx_drop_t1_b (b));
show create table drop_idx_t1;
show index from drop_idx_t1;
\d+ drop_idx_t1
ALTER TABLE drop_idx_t1 DROP INDEX idx_drop_t1_b;
show create table drop_idx_t1;
show index from drop_idx_t1;
\d+ drop_idx_t1
drop table drop_idx_t1;

-- drop key
CREATE TABLE drop_idx_t1 (a int, b int, INDEX idx_drop_t1_b (b));
\d+ drop_idx_t1
ALTER TABLE drop_idx_t1 DROP KEY idx_drop_t1_b;
\d+ drop_idx_t1
drop table drop_idx_t1;

-- index did not exist
CREATE TABLE drop_idx_t1 (a int, b int, INDEX idx_drop_t1_b (b));
ALTER TABLE drop_idx_t1 DROP INDEX drop_idx_t1_b;
drop table drop_idx_t1;

-- drop twice
CREATE TABLE drop_idx_t1 (a int, b int, INDEX idx_drop_t1_b (b));
ALTER TABLE drop_idx_t1 DROP KEY idx_drop_t1_b;
ALTER TABLE drop_idx_t1 DROP KEY idx_drop_t1_b;
drop table drop_idx_t1;

-- index did not belong to the table
CREATE TABLE drop_idx_t1 (a int, b int, INDEX idx_drop_t1_b (b));
CREATE TABLE drop_idx_t2 (a int, b int, INDEX idx_drop_t2_b (b));
\d+ drop_idx_t1
\d+ drop_idx_t2
ALTER TABLE drop_idx_t1 DROP key drop_idx_t2_b_idx;
drop table drop_idx_t1;
drop table drop_idx_t2;

-- index is pk
CREATE TABLE drop_idx_pk_t (a int PRIMARY KEY, b int);
\d+ drop_idx_pk_t
ALTER TABLE drop_idx_pk_t DROP INDEX drop_idx_pk_t_pkey;
\d+ drop_idx_pk_t
drop table drop_idx_pk_t;

-- index is uk
CREATE TABLE drop_idx_uk_t (a int, b int);
ALTER TABLE drop_idx_uk_t ADD UNIQUE KEY idx_uniq_key_b (b);
\d+ drop_idx_uk_t
ALTER TABLE drop_idx_uk_t DROP INDEX idx_uniq_key_b;
\d+ drop_idx_uk_t
drop table drop_idx_uk_t;

-- index is fk, error
CREATE TABLE drop_idx_fkname_parent (id int PRIMARY KEY);
CREATE TABLE drop_idx_fkname_child (id int, pid int);
ALTER TABLE drop_idx_fkname_child ADD CONSTRAINT fk_by_name FOREIGN KEY (pid)
  REFERENCES drop_idx_fkname_parent(id);
\d+ drop_idx_fkname_child
ALTER TABLE drop_idx_fkname_child DROP INDEX fk_by_name;
\d+ drop_idx_fkname_child
drop table drop_idx_fkname_child;
drop table drop_idx_fkname_parent;

-- its a check, could not drop by index
CREATE TABLE drop_idx_ch_t (a int, b int);
ALTER TABLE drop_idx_ch_t add check (b>100);
show create table drop_idx_ch_t;
alter table drop_idx_ch_t drop index drop_idx_ch_t_b_check;
show create table drop_idx_ch_t;
drop table drop_idx_ch_t;

--------- DROP PRIMARY KEY
-- has pk
CREATE TABLE drop_pk_t1 (a int, b int, primary key (b));
\d+ drop_pk_t1
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
\d+ drop_pk_t1
drop table drop_pk_t1;

-- drop twice
CREATE TABLE drop_pk_t1 (a int, b int);
ALTER TABLE drop_pk_t1 ADD PRIMARY KEY(b);
\d+ drop_pk_t1
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
\d+ drop_pk_t1
drop table drop_pk_t1;

-- did not have pk
CREATE TABLE drop_pk_t1 (a int, b int);
\d+ drop_pk_t1
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
\d+ drop_pk_t1
drop table drop_pk_t1;

-- its a index, could not drop by pk
CREATE TABLE drop_pk_t1 (a int, b int);
ALTER TABLE drop_pk_t1 ADD KEY(b);
\d+ drop_pk_t1
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
drop table drop_pk_t1;

-- its a unique key, could not drop by pk
CREATE TABLE drop_pk_t1 (a int, b int);
ALTER TABLE drop_pk_t1 ADD unique KEY(b);
\d+ drop_pk_t1
ALTER TABLE drop_pk_t1 DROP PRIMARY KEY;
drop table drop_pk_t1;

--------- DROP FOREIGN KEY
-- has fk
CREATE TABLE drop_fk_parent (id int PRIMARY KEY);
CREATE TABLE drop_fk_child (id int, pid int,
  CONSTRAINT drop_fk_sym FOREIGN KEY (pid) REFERENCES drop_fk_parent(id));
\d+ drop_fk_child
ALTER TABLE drop_fk_child DROP FOREIGN KEY drop_fk_sym;
\d+ drop_fk_child;
drop table drop_fk_child;
drop table drop_fk_parent;

-- no fk
CREATE TABLE drop_fk_parent (id int PRIMARY KEY);
CREATE TABLE drop_fk_child (id int, pid int);
\d+ drop_fk_child
ALTER TABLE drop_fk_child DROP FOREIGN KEY drop_fk_sym;
\d+ drop_fk_child;
drop table drop_fk_child;
drop table drop_fk_parent;

-- drop twice in one SQL
CREATE TABLE drop_fk_parent (id int PRIMARY KEY);
CREATE TABLE drop_fk_child (id int, pid int,
  CONSTRAINT drop_fk_sym FOREIGN KEY (pid) REFERENCES drop_fk_parent(id));
\d+ drop_fk_child
ALTER TABLE drop_fk_child DROP FOREIGN KEY drop_fk_sym, DROP FOREIGN KEY drop_fk_sym;
\d+ drop_fk_child;
drop table drop_fk_child;
drop table drop_fk_parent;

-- its pk, could not drop by fk
CREATE TABLE drop_fk_parent (id int PRIMARY KEY);
\d+ drop_fk_parent
ALTER TABLE drop_fk_parent DROP FOREIGN KEY drop_fk_parent_pkey;
\d+ drop_fk_parent;
drop table drop_fk_parent;

-- its uk, could not drop by fk
CREATE TABLE drop_fk_parent (id int, b int, unique key idx_b(b));
\d+ drop_fk_parent
ALTER TABLE drop_fk_parent DROP FOREIGN KEY idx_b;
\d+ drop_fk_parent;
drop table drop_fk_parent;

-- its check, could not drop by fk
CREATE TABLE drop_fk_parent (id int, b int, check (b>10));
\d+ drop_fk_parent
ALTER TABLE drop_fk_parent DROP FOREIGN KEY drop_fk_parent_b_check;
\d+ drop_fk_parent;
drop table drop_fk_parent;

---------------------- test case for merge constraint for the same column name
-- merge pk + uk to pk
CREATE TABLE pk_uk_idx_merge (a int, b int, primary key (a), unique key(a));
show create table pk_uk_idx_merge;
drop table pk_uk_idx_merge;

-- merge pk + index to pk
CREATE TABLE pk_uk_idx_merge (a int, b int, primary key (a), index(a));
show create table pk_uk_idx_merge;
drop table pk_uk_idx_merge;

-- merge uk + index to uk
CREATE TABLE pk_uk_idx_merge (a int, b int,  unique key(a), index (a));
show create table pk_uk_idx_merge;
drop table pk_uk_idx_merge;

-- merge index + index to index
CREATE TABLE pk_uk_idx_merge (a int, b int,  index(a), index (a));
show create table pk_uk_idx_merge;
drop table pk_uk_idx_merge;

---------------------- partition table
------ test case add/drop pk
create table t_par(a int, id int, name varchar(20)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;
-- error as pk must include partition_key
alter table t_par add primary key(a);
-- succ
alter table t_par add primary key(a, id);
\d+ t_par;
\d+ t_par_1;
alter table t_par drop primary key;
\d+ t_par;
\d+ t_par_1;

drop table t_par;

------ test case add/drop uk
create table t_par(a int, id int, name varchar(20)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;
-- error uk need include partition_key
alter table t_par add unique key(a);
-- succ
alter table t_par add unique key(a, id);
\d+ t_par;
\d+ t_par_1;
alter table t_par drop unique key t_par_a_id_key;
\d+ t_par;
\d+ t_par_1;

drop table t_par;

------ test case add/drop index
create table t_par(a int, id int, name varchar(20)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;
-- succ
alter table t_par add index(a);
\d+ t_par;
\d+ t_par_1;
alter table t_par drop index t_par_a_idx;
\d+ t_par;
\d+ t_par_1;

drop table t_par;

-------------------- drop index on table
-- drop index
CREATE TABLE drop_idx_on_t (a int, b int, INDEX idx_drop_on_b (b));
\d+ drop_idx_on_t
show create table drop_idx_on_t;
DROP INDEX idx_drop_on_b ON drop_idx_on_t;
\d+ drop_idx_on_t
show create table drop_idx_on_t;
drop table drop_idx_on_t;

-- drop primary key
CREATE TABLE drop_idx_on_t (a int, b int, primary key(b));
\d+ drop_idx_on_t
show create table drop_idx_on_t;
DROP INDEX drop_idx_on_t_pkey ON drop_idx_on_t;
\d+ drop_idx_on_t
show create table drop_idx_on_t;
drop table drop_idx_on_t;

-- drop unique key
CREATE TABLE drop_idx_on_t (a int, b int, unique key(b));
\d+ drop_idx_on_t
show create table drop_idx_on_t;
DROP INDEX drop_idx_on_t_b_key ON drop_idx_on_t;
\d+ drop_idx_on_t
show create table drop_idx_on_t;
drop table drop_idx_on_t;

-- drop foreign key, error
CREATE TABLE drop_idx_on_t (a int, b int primary key);
CREATE TABLE drop_idx_on_t_c(aa int, bb int references drop_idx_on_t(b));
\d+ drop_idx_on_t_c
show create table drop_idx_on_t_c;
DROP INDEX drop_idx_on_t_c_bb_fkey ON drop_idx_on_t_c;
\d+ drop_idx_on_t_c
drop table drop_idx_on_t_c,drop_idx_on_t;

-- index did not belong to the table
-- index did not exist
CREATE TABLE drop_idx_on_t_1 (a int, b int, index(b));
CREATE TABLE drop_idx_on_t_2 (a int, b int, index(b));
\d+ drop_idx_on_t_2
DROP INDEX drop_idx_on_t_2_b_idx ON drop_idx_on_t_1;
DROP INDEX abcdef ON drop_idx_on_t_1;
DROP TABLE drop_idx_on_t_1,drop_idx_on_t_2;

-- drop primary key by `PRIMARY`
CREATE TABLE drop_idx_on_t (a int, b int, primary key(b));
\d+ drop_idx_on_t
show create table drop_idx_on_t;
DROP INDEX `PRIMARY` ON drop_idx_on_t;
\d+ drop_idx_on_t
show create table drop_idx_on_t;
drop table drop_idx_on_t;

----------- partition table
------ test case add/drop pk
create table t_par(a int, id int primary key, name varchar(20)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;

\d+ t_par;
\d+ t_par_1;
drop index t_par_pkey on t_par;
\d+ t_par;
\d+ t_par_1;

drop table t_par;

------ test case add/drop uk
create table t_par(a int, id int unique key, name varchar(20)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;

\d+ t_par;
\d+ t_par_1;
drop index t_par_id_key on t_par ;
\d+ t_par;
\d+ t_par_1;

drop table t_par;

-- drop index on table
create table t_par(a int, id int, name varchar(20), index(id)) partition by range(id);
create table t_par_1 partition of t_par for values from(0) to (100);
create table t_par_2 partition of t_par for values from(100) to (200);
create table t_par_3 partition of t_par for values from(200) to (MAXVALUE);

insert into t_par(a,id) values(10,101);
select * from t_par;

\d+ t_par;
\d+ t_par_1;
drop index t_par_id_idx on t_par;
\d+ t_par;
\d+ t_par_1;

drop table t_par;


------------------ PK/UK ASC|DESC on create table
-- PK: int ASC
create table t_create_pk(a int, b int, c varchar(20), primary key(b ASC));
\d+ t_create_pk;
\d+ t_create_pk_pkey;
show index from t_create_pk;
drop table t_create_pk;

-- PK: int DESC
create table t_create_pk(a int, b int, c varchar(20), primary key(a DESC));
\d+ t_create_pk;
\d+ t_create_pk_pkey;
show index from t_create_pk;
drop table t_create_pk;

-- PK: int DESC, varchar DESC
create table t_create_pk(a int, b int, c varchar(20),  primary key(a DESC, c DESC));
\d+ t_create_pk;
\d+ t_create_pk_pkey;
show index from t_create_pk;
drop table t_create_pk;

-- PK: int DESC, varchar ASC
create table t_create_pk(a int, b int, c varchar(20),  primary key(a DESC, c ASC));
\d+ t_create_pk;
\d+ t_create_pk_pkey;
show index from t_create_pk;
drop table t_create_pk;

-- UK: int ASC
create table t_create_uk(a int, b int, c varchar(20), UNIQUE key(b ASC));
\d+ t_create_uk;
\d+ t_create_uk_b_key;
show index from t_create_uk;
drop table t_create_uk;

-- UK: int DESC
create table t_create_uk(a int, b int, c varchar(20), UNIQUE(a DESC));
\d+ t_create_uk;
\d+ t_create_uk_a_key;
show index from t_create_uk;
drop table t_create_uk;

-- UK: int DESC, varchar DESC
create table t_create_uk(a int, b int, c varchar(20),  UNIQUE INDEX(a DESC, c DESC));
\d+ t_create_uk;
\d+ t_create_uk_a_c_key;
show index from t_create_uk;
drop table t_create_uk;

-- UK: int DESC, varchar ASC
create table t_create_uk(a int, b int, c varchar(20),  UNIQUE(a DESC, c ASC));
\d+ t_create_uk;
\d+ t_create_uk_a_c_key;
show index from t_create_uk;
drop table t_create_uk;

-- did not support expr
create table t_create_uk(a int, b int, c varchar(20),  unique key(a+10));
create table t_create_uk(a int, b int, c varchar(20),  unique key((a+10)));

------------------ test case: not accpet sch.idx
create table sch_idx(a int, b int, index idx_b(b));
show create table sch_idx;
drop index public.sch_idx_b_key on sch_idx;
show create table sch_idx;
drop table sch_idx;
