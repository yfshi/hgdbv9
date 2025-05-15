/*DAY*/

create table test_day(funcname text, result text);
insert into test_day values('day(''2021-1-1'')', day('2021-1-1'));
insert into test_day values('day(''2021-1-1 00:00:00'')', day('2021-1-1 00:00:00'));
insert into test_day values('day(''2021-1-1 23:59:59.9999995'')', day('2021-1-1 23:59:59.9999995'));
insert into test_day values('day(''0000-1-1 23:59:59.9999995'')', day('0000-1-1 23:59:59.9999995'));
/*Because of there is difference in the handling of datetime type date in the year '0000' when rounding occurs,lead to difference in the result of this case*/
insert into test_day values('day(''0000-12-31 22:59:59.9999995'')', day('0000-12-31 22:59:59.9999995'));
insert into test_day values('day(''0000-12-31 23:59:59.9999995'')', day('0000-12-31 23:59:59.9999995'));

insert into test_day values('day(''0000-1-1'')', day('0000-1-1'));
insert into test_day values('day(''9999-12-31'')', day('9999-12-31'));
insert into test_day values('day(''0000-1-1 00:00:00'')', day('0000-1-1 00:00:00'));
insert into test_day values('day(''9999-12-31 23:59:59.999999'')', day('9999-12-31 23:59:59.999999'));

insert into test_day values('day(date''0000-1-1'')', day(date'0000-1-1'));
insert into test_day values('day(cast(''9999-12-31 12:12:12'' as datetime))', day(cast('9999-12-31 12:12:12' as datetime)));
insert into test_day values('day(101)', day(101));
insert into test_day values('day(99991231)', day(99991231));
insert into test_day values('day(99991231235959.999999)', day(99991231235959.999999));
insert into test_day values('day(20211112235959.999999)', day(20211112235959.999999));
insert into test_day values('day(20211112235959.9999999)', day(20211112235959.9999999));

insert into test_day values('day(null)', day(null));
insert into test_day values('day(''2021-00-01'')', day('2021-00-01'));
insert into test_day values('day(''2021-01-00'')', day('2021-01-00'));
insert into test_day values('day(''2021-00-00'')', day('2021-00-00'));
insert into test_day values('day(20210001)', day(20210001));
insert into test_day values('day(20210100)', day(20210100));
insert into test_day values('day(20210000)', day(20210000));
insert into test_day values('day(0)', day(0));
select * from test_day;
drop table test_day;

--sql_mode
/*Becauseof mystext type '0000-00-00' conversion mysdatetime type has difference with mysql,lead to difference in the result of this case*/
create table test_mode_day(funcname text, result text, mode text);
set sql_mode ='';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = ');
set sql_mode ='NO_ZERO_IN_DATE';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = NO_ZERO_IN_DATE');
set sql_mode ='NO_ZERO_DATE';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = NO_ZERO_DATE');
set sql_mode ='STRICT_TRANS_TABLES,STRICT_ALL_TABLES';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES');
set sql_mode ='STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
set sql_mode ='STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_IN_DATE');
set sql_mode ='STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE,NO_ZERO_IN_DATE';
insert into test_mode_day values('day(''0000-00-00'')', day('0000-00-00') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE,NO_ZERO_IN_DATE');
select * from test_mode_day;
drop table test_mode_day;


create table test_mode_day(funcname text, result text, mode text);
set sql_mode ='NO_ZERO_DATE';
insert into test_mode_day values('day(''10000-1-1'')', day('10000-1-1') ,'sql_mode = NO_ZERO_DATE');
insert into test_mode_day values('day(''9999-12-31 23:59:59.9999995'')', day('9999-12-31 23:59:59.9999995') ,'sql_mode = NO_ZERO_DATE');
insert into test_mode_day values('day(-1)', day(-1) ,'sql_mode = NO_ZERO_DATE');
insert into test_mode_day values('day(99991231235959.9999995)', day(99991231235959.9999995) ,'sql_mode = NO_ZERO_DATE');
insert into test_mode_day values('day(99991231235959.9999995)', day(99991231235959.9999995) ,'sql_mode = NO_ZERO_DATE');
insert into test_mode_day values('day(9999999999999999999999)', day(9999999999999999999999) ,'sql_mode = NO_ZERO_DATE');



set sql_mode ='STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE';
insert into test_mode_day values('day(''10000-1-1'')', day('10000-1-1') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
insert into test_mode_day values('day(''9999-12-31 23:59:59.9999995'')', day('9999-12-31 23:59:59.9999995') ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
insert into test_mode_day values('day(-1)', day(-1) ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
/*Because mysdatetimes are handled differently with mysql error when rounding occurs ,lead to difference in the result of this case*/
insert into test_mode_day values('day(99991231235959.9999995)', day(99991231235959.9999995) ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
insert into test_mode_day values('day(99991231235959.9999995)', day(99991231235959.9999995) ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
insert into test_mode_day values('day(9999999999999999999999)', day(9999999999999999999999) ,'sql_mode = STRICT_TRANS_TABLES,STRICT_ALL_TABLES,NO_ZERO_DATE');
select * from test_mode_day;
drop table test_mode_day;

set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
/*DAYOFMONTH*/
SELECT dayofmonth('2022-10-09 09:55:28.12345');
SELECT dayofmonth(20221009);
select dayofmonth('20:38:40');
select dayofmonth(203840);

/*DAYOFWEEK*/
SELECT dayofweek('2022-10-09 09:55:28.12345');
SELECT dayofweek(20221009);
select dayofweek('14:14:14');
select dayofweek(time'2022-06-28 00:00:00');
/*Because mystime conversion to mysdate is different from mysql*/
select dayofweek(time'838:59:59');

/*DAYOFYEAR*/
SELECT dayofyear('2022-10-09 09:55:28.12345');
SELECT dayofyear(20221009);
select dayofyear('20:38:40');
select dayofyear(203840);


/*QUARTER*/
SELECT quarter('2022-10-09 09:55:28.12345');
SELECT quarter(20221009);
select quarter('20:38:40');
select quarter(time'20:38:40');
select quarter(203840);
select quarter(time'203840');

/*HOUR*/
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
SELECT hour('2022-10-09 09:55:28.12345');
select hour('10:20:30');
select hour('30:20:10.123456');
select hour('2022-10-09 11:22:33');
select hour('2022-10-09 11:22:33.456789');
select hour('20221009112233');
select hour('373839');
select hour('2022-10-09');

create table test_hour(hour int);
insert into test_hour select hour('-838:59:59');
insert into test_hour select hour('838:59:59');
insert into test_hour select hour('-840:59:59');
insert into test_hour select hour('840:59:59');
insert into test_hour select hour('');
insert into test_hour select hour('abc');
insert into test_hour select hour('1234abc');
insert into test_hour select hour('23:59:59.999999');
insert into test_hour select hour('23:59:59.9999999');
insert into test_hour select hour('23:59:59.9999995');
insert into test_hour select hour('23:59:59.9999994');
insert ignore into test_hour select hour('-838:59:59');
insert ignore into test_hour select hour('838:59:59');
insert ignore into test_hour select hour('-840:59:59');
insert ignore into test_hour select hour('840:59:59');
insert ignore into test_hour select hour('');
insert ignore into test_hour select hour('abc');
insert ignore into test_hour select hour('1234abc');
insert ignore into test_hour select hour('23:59:59.999999');
insert ignore into test_hour select hour('23:59:59.9999999');
insert ignore into test_hour select hour('23:59:59.9999995');
insert ignore into test_hour select hour('23:59:59.9999994');
select * from test_hour;
drop table test_hour;

set sql_mode='';
create table test_hour(hour int);
insert into test_hour select hour('-838:59:59');
insert into test_hour select hour('838:59:59');
insert into test_hour select hour('-840:59:59');
insert into test_hour select hour('840:59:59');
insert into test_hour select hour('');
insert into test_hour select hour('abc');
insert into test_hour select hour('1234abc');
insert into test_hour select hour('23:59:59.999999');
insert into test_hour select hour('23:59:59.9999999');
insert into test_hour select hour('23:59:59.9999995');
insert into test_hour select hour('23:59:59.9999994');
select * from test_hour;
drop table test_hour;

/*MICROSECOND*/
SELECT microsecond('2022-10-09 09:55:28.12345');
select microsecond('10:20:30');
select microsecond('30:20:10.123456');
select microsecond('2022-10-09 11:22:33');
select microsecond('2022-10-09 11:22:33.456789');
select microsecond('20221009112233');
select microsecond('373839');
select microsecond('2022-10-09');

/*MINUTE*/
select MINUTE(datetime '2021-11-4 16:30:44.341191');
SELECT minute('2022-10-09 09:55:28.12345');
select minute('10:20:30');
select minute('30:20:10.123456');
select minute('2022-10-09 11:22:33');
select minute('2022-10-09 11:22:33.456789');
select minute('20221009112233');
select minute('373839');
select minute('2022-10-09');

/*SECOND*/
SELECT second('2022-10-09 09:55:28.12345');
select second('10:20:30');
select second('30:20:10.123456');
select second('2022-10-09 11:22:33');
select second('2022-10-09 11:22:33.456789');
select second('20221009112233');
select second('373839');
select second('2022-10-09');

/*WEEKDAY*/
SELECT weekday('2022-10-09 09:55:28.12345');
SELECT weekday(20221009);

/*WEEKOFYEAR*/
SELECT weekofyear('2022-10-09 09:55:28.12345');
SELECT weekofyear(20221009);

/*YEAR*/
select year('');
select year('2022');
select year('2022-06');
select year('2022-06-13');
select year('9999-06-13');
select year('10000-06-13');
select year(20221009);
