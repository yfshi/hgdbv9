create table t_int2(a int2, b int2);
insert into t_int2 values (1, 0);
set sql_mode = '';
select a || b from t_int2;
set sql_mode = 'pipes_as_concat';
select a || b from t_int2;
select pg_typeof(a || b) from t_int2;

create table t_uint1(a uint1, b uint1);
insert into t_uint1 values (1, 0);
set sql_mode = '';
select a || b from t_uint1;
set sql_mode = 'pipes_as_concat';
select a || b from t_uint1;
select pg_typeof(a || b) from t_uint1;

create table t_uint2(a uint2, b uint2);
insert into t_uint2 values (1, 0);
set sql_mode = '';
select a || b from t_uint2;
set sql_mode = 'pipes_as_concat';
select a || b from t_uint2;
select pg_typeof(a || b) from t_uint2;

create table t_uint4(a uint4, b uint4);
insert into t_uint4 values (1, 0);
set sql_mode = '';
select a || b from t_uint4;
set sql_mode = 'pipes_as_concat';
select a || b from t_uint4;
select pg_typeof(a || b) from t_uint4;

create table t_uint8(a uint8, b uint8);
insert into t_uint8 values (1, 0);
set sql_mode = '';
select a || b from t_uint8;
set sql_mode = 'pipes_as_concat';
select a || b from t_uint8;
select pg_typeof(a || b) from t_uint8;

create table t_float8(a float8, b float8);
insert into t_float8 values (1.1, 0.9);
set sql_mode = '';
select a || b from t_float8;
set sql_mode = 'pipes_as_concat';
select a || b from t_float8;
select pg_typeof(a || b) from t_float8;

create table t_date(a date, b date);
insert into t_date values ('2010-1-1', '2011-2-2');
set sql_mode = '';
select a || b from t_date;
set sql_mode = 'pipes_as_concat';
select a || b from t_date;
select pg_typeof(a || b) from t_date;

create table t_datetime(a datetime, b datetime);
insert into t_datetime values ('2010-1-1', '2011-2-2');
set sql_mode = '';
select a || b from t_datetime;
set sql_mode = 'pipes_as_concat';
select a || b from t_datetime;
select pg_typeof(a || b) from t_datetime;

create table t_datetimes(a datetime(6), b datetime(6));
insert into t_datetimes values ('2010-1-1', '2011-2-2');
set sql_mode = '';
select a || b from t_datetimes;
set sql_mode = 'pipes_as_concat';
select a || b from t_datetimes;
select pg_typeof(a || b) from t_datetimes;

create table t_timestamp(a timestamp, b timestamp);
insert into t_timestamp values ('1.1111', '1.1111');
set sql_mode = '';
select a || b from t_timestamp;
set sql_mode = 'pipes_as_concat';
select a || b from t_timestamp;
select pg_typeof(a || b) from t_timestamp;

create table t_timestamps(a timestamp(6), b timestamp(6));
insert into t_timestamps values ('1.1111', '1.1111');
set sql_mode = '';
select a || b from t_timestamps;
set sql_mode = 'pipes_as_concat';
select a || b from t_timestamps;
select pg_typeof(a || b) from t_timestamps;

create table t_time(a time, b time);
insert into t_time values ('1.1111', '1.1111');
set sql_mode = '';
select a || b from t_time;
set sql_mode = 'pipes_as_concat';
select a || b from t_time;
select pg_typeof(a || b) from t_time;

create table t_times(a time(6), b time(6));
insert into t_times values ('1.1111', '1.1111');
set sql_mode = '';
select a || b from t_times;
set sql_mode = 'pipes_as_concat';
select a || b from t_times;
select pg_typeof(a || b) from t_times;