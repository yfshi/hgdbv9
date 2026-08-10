create table t_int(
    a int1,
    b int2,
    c int3,
    d int4,
    e int8,
    f tinyint unsigned,
    g smallint unsigned,
    h mediumint unsigned,
    i int unsigned,
    j bigint unsigned
);

desc t_int;

create table t_str(
    a char,
    b char(100), 
    d varchar(100), 
    e text, 
    f tinytext, 
    g mediumtext, 
    h longtext
);

desc t_str;


create table t_real(
    a float,
    b double,
    c numeric(10, 2)
);

desc t_real;


create table t_time_type(
    a date,
    b time,
    c time(6),
    d datetime,
    e datetime(6),
    f timestamp,
    g timestamp(6)
);

desc t_time_type;

create table t_bin(
    a binary,
    a1 binary(100),
    b varbinary(100),
    c blob,
    c1 blob(100),
    d tinyblob,
    e mediumblob,
    f  longblob,
    g  bit(10)
);

desc t_bin;

drop table t_int;
drop table t_str;
drop table t_real;
drop table t_time_type;
drop table t_bin;
