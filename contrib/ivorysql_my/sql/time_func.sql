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


/*MAKEDATE*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('makedate(''2003'',''61'')', makedate('2003','61'));
insert into func_test(functionName, result) values('makedate(null, 10)', makedate(null, 10));
insert into func_test(functionName, result) values('makedate(2000, null)', makedate(2000, null));
insert into func_test(functionName, result) values('makedate(-1, 20)', makedate(-1, 20));
insert into func_test(functionName, result) values('makedate(10000, 20)', makedate(10000, 20));
insert into func_test(functionName, result) values('makedate(2000, 0)', makedate(2000, 0));
insert into func_test(functionName, result) values('makedate(2000, 60)', makedate(2000, 60));
insert into func_test(functionName, result) values('makedate(2000, 380)', makedate(2000, 380));
insert into func_test(functionName, result) values('makedate(69, 32)',makedate(69, 32));
insert into func_test(functionName, result) values('makedate(70, 32)', makedate(70, 32));
insert into func_test(functionName, result) values('makedate(100,32)', makedate(100,32));
insert into func_test(functionName, result) values('makedate(9999,366)', makedate(9999,366));
insert into func_test(functionName, result) values('makedate(2001, 9223372036854775807)', makedate(2001, 9223372036854775807));
insert into func_test(functionName, result) values('makedate(cast(1901 as year), 10)', makedate(cast(1901 as year), 10));
select * from func_test;
drop table func_test;
/*MAKETIME*/
select maketime('25.5', '30.4', '30');
select maketime(25.5, 30.4, 30);

create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('maketime(0, -1, 0)', maketime(0, -1, 0)); -- minute < 0
insert into func_test(functionName, result) values ('maketime(0, 60, 0)', maketime(0, 60, 0));-- minute > 59
insert into func_test(functionName, result) values ('maketime(0, 0, -1)', maketime(0, 0, -1)); -- second < 0
insert into func_test(functionName, result) values ('maketime(0, 0, 60)', maketime(0, 0, 60)); -- second >= 60
insert into func_test(functionName, result) values ('maketime(4294967295, 1, 1)', maketime(4294967295, 1, 1));
insert into func_test(functionName, result) values ('maketime(-4294967295, 0, 1)', maketime(-4294967295, 0, 1));
insert into func_test(functionName, result) values ('maketime(2147483649, 1, 1)', maketime(2147483649, 1, 1));
insert into func_test(functionName, result) values ('maketime(-2147483649, 1, 1)', maketime(-2147483649, 1, 1));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('maketime(0, 0, 59.9999)', maketime(0, 0, 59.9999)); 
insert into func_test(functionName, result) values ('maketime(0, 0, 59.999999001)', maketime(0, 0, 59.999999001)); 
insert into func_test(functionName, result) values ('maketime(838, 59, 58.999999)', maketime(838, 59, 58.999999));
insert into func_test(functionName, result) values ('maketime(-838, 59, 58.999999)', maketime(-838, 59, 58.999999));
insert into func_test(functionName, result) values ('maketime(25, 30, 30)', maketime(25, 30, 30));
insert into func_test(functionName, result) values ('maketime(-25, 30, 30)', maketime(-25, 30, 30));
insert into func_test(functionName, result) values ('maketime(0, 59, 59.9999994999)', maketime(0, 59, 59.9999994999));
insert into func_test(functionName, result) values ('maketime(0, 59, 59.999998500)', maketime(0, 59, 59.999998500));
select * from func_test;
drop table func_test;
/*PERIOD_ADD*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_add(''202101'', ''10'')', period_add('202101', '10'));
insert into func_test(functionName, result) values ('period_add(''202101.4'', ''10.5'')', period_add('202101.4', '10.5'));
insert into func_test(functionName, result) values ('period_add(202101.4, 10.5)', period_add(202101.4, 10.5));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_add(0.0, 1)', period_add(0, 1)); -- p == 0
insert into func_test(functionName, result) values ('period_add(200000, 0)', period_add(200000, 0)); 
insert into func_test(functionName, result) values ('period_add(-202205, 1)', period_add(-202205, 1)); -- p < 0
insert into func_test(functionName, result) values ('period_add(202211, 100)', period_add(202211, 100)); 
insert into func_test(functionName, result) values ('period_add(197005, 7)', period_add(197005, 7));
insert into func_test(functionName, result) values ('period_add(202205, -12)', period_add(202205, -12)); -- N < 0 
insert into func_test(functionName, result) values ('period_add(1,12)', period_add(1,12)); 
insert into func_test(functionName, result) values ('period_add(6901,12)', period_add(6901,12));
insert into func_test(functionName, result) values ('period_add(7001,12)', period_add(7001,12)); 
insert into func_test(functionName, result) values ('period_add(10001,12)', period_add(10001,12)); 
insert into func_test(functionName, result) values ('period_add(10001, -12)', period_add(10001, -12)); 
insert into func_test(functionName, result) values ('period_add(10001, -12*30-1)', period_add(10001, -361));
select * from func_test;
drop table func_test;
/*PERIOD_DIFF*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_diff(''202101'', ''202102'')', period_diff('202101', '202102')); 
insert into func_test(functionName, result) values ('period_diff(''202101.4'', ''202102.5'')', period_diff('202101.4', '202102.5')); 
insert into func_test(functionName, result) values ('period_diff(202101.4, 202102.5)', period_diff(202101.4, 202102.5)); 
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_diff(NULL, 200001)', period_diff(NULL, 200001));
insert into func_test(functionName, result) values ('period_diff(202011, NULL)', period_diff(202011, NULL));
insert into func_test(functionName, result) values ('period_diff(-202011, 200001)', period_diff(-202011, 200001)); -- P1<0
insert into func_test(functionName, result) values ('period_diff(202011, -200001)', period_diff(202011, -200001)); -- P2<0
insert into func_test(functionName, result) values ('period_diff(707712, 202201)', period_diff(707712, 202201));
insert into func_test(functionName, result) values ('period_diff(6912, 10001)', period_diff(6912, 10001));
insert into func_test(functionName, result) values ('period_diff(7001, 10001)', period_diff(7001, 10001));
insert into func_test(functionName, result) values ('period_diff(10002, 6901)', period_diff(10002, 6901));
insert into func_test(functionName, result) values ('period_diff(10002, 7001)', period_diff(10002, 7001));
insert into func_test(functionName, result) values ('period_diff(0, 30)', period_diff(0, 30)); -- P1 == 0
insert into func_test(functionName, result) values ('period_diff(30, 0)', period_diff(30, 0)); -- P2 == 0
select * from func_test;
drop table func_test;
/*SEC_TO_TIME */
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('sec_to_time(''1000'')', sec_to_time('1000'));
insert into func_test(functionName, result) values ('sec_to_time(''1000.5'')', sec_to_time('1000.5'));
insert into func_test(functionName, result) values ('sec_to_time(''-5231'')', sec_to_time('-5231'));
insert into func_test(functionName, result) values ('sec_to_time(1000.5)', sec_to_time(1000.5));
insert into func_test(functionName, result) values ('sec_to_time(NULL)', sec_to_time(NULL));   
insert into func_test(functionName, result) values ('sec_to_time(1000)', sec_to_time(1000)); 
insert into func_test(functionName, result) values ('sec_to_time(0)', sec_to_time(0));
insert into func_test(functionName, result) values ('sec_to_time(-1000000)', sec_to_time(-1000000));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('sec_to_time(1000000.499)', sec_to_time(1000000.499));
insert into func_test(functionName, result) values ('sec_to_time(1000000.4990000)', sec_to_time(1000000.4990000));
insert into func_test(functionName, result) values ('sec_to_time(1000000.4999994999)', sec_to_time(1000000.4999994999));
insert into func_test(functionName, result) values ('sec_to_time(1000000.499999500)', sec_to_time(1000000.499999500));
insert into func_test(functionName, result) values ('sec_to_time(1000000.499998500)', sec_to_time(1000000.499998500));
insert into func_test(functionName, result) values ('sec_to_time(0.999999500)', sec_to_time(0.999999500));
insert into func_test(functionName, result) values ('sec_to_time(59.999999600)', sec_to_time(59.999999600));
insert into func_test(functionName, result) values ('sec_to_time(3020399)', sec_to_time(3020399)); 
insert into func_test(functionName, result) values ('sec_to_time(3020399 + 1)', sec_to_time(3020399 + 1)); 
insert into func_test(functionName, result) values ('sec_to_time(3020399.000000500)', sec_to_time(3020399.000000500)); 
insert into func_test(functionName, result) values ('sec_to_time(-100000000)', sec_to_time(-100000000));
insert into func_test(functionName, result) values ('sec_to_time(-999999999)', sec_to_time(-999999999));
insert into func_test(functionName, result) values ('sec_to_time(10000000000000000000)', sec_to_time(10000000000000000000));
insert into func_test(functionName, result) values ('sec_to_time(-10000000000000000000)', sec_to_time(-10000000000000000000));
insert into func_test(functionName, result) values ('sec_to_time(-9223372036854775808)', sec_to_time(-9223372036854775808));
select * from func_test;
drop table func_test;

/*SUBTIME*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('subtime(''2000-02-28 20:59:59'', ''-25:00'')', subtime('2000-02-28 20:59:59', '-25:00') );
insert into func_test(functionName, result) values('subtime(''2000-02-28 20:59:59'', ''25:00'')', subtime('2000-02-28 20:59:59', '25:00') );
insert into func_test(functionName, result) values('SUBTIME(''2007-12-31 23:59:59.999999'',''1 1:1:1.000002'')', SUBTIME('2007-12-31 23:59:59.999999','1 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2007-12-31 23:59:59.999999'',''100 1:1:1.000002'')',SUBTIME('2007-12-31 23:59:59.999999','100 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2007-12-31 0:0:0.999999'',''1 1:1:1.000002'')', SUBTIME('2007-12-31 0:0:0.999999','1 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2007-12-31 23:59:59.999999'',''1 1:1:1.000002'')', SUBTIME('2007-12-31 23:59:59.999999','1 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2007-13-31 23:59:59.999999'',''1 1:1:1.000002'')', SUBTIME('2007-13-31 23:59:59.999999','1 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2007-12-40 23:59:59.999999'',''1 1:1:1.000002'')', SUBTIME('2007-12-40 23:59:59.999999','1 1:1:1.000002'));
insert into func_test(functionName, result) values('SUBTIME(''2017-2-29 12:00:00'',''12:00:00'')', SUBTIME('2017-2-29 12:00:00','12:00:00'));
insert into func_test(functionName, result) values('subtime(''20000228205959'', ''250000'')', subtime('20000228205959', '250000') );
insert into func_test(functionName, result) values('subtime(''20000228205959.000002'', ''250000.000001'')', subtime('20000228205959.000002', '250000.000001') );
insert into func_test(functionName, result) values('subtime(20000228205959, 250000)', subtime(20000228205959, 250000) );
insert into func_test(functionName, result) values('subtime(20000228205959.000002, 250000.000001)', subtime(20000228205959.000002, 250000.000001) );
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('subtime(''0001-01-1 20:59:59'', ''21:00'');', subtime('0001-01-1 20:59:59', '21:00')   );
insert into func_test(functionName, result) values('subtime(''9999-12-31 20:59:59'', ''-3:00:1'')', subtime('9999-12-31 20:59:59', '-3:00:1') );
insert into func_test(functionName, result) values('subtime(''10000-1-1 20:59:59'', ''30:00:1'')', subtime('10000-1-1 20:59:59', '30:00:1') );
insert into func_test(functionName, result) values('subtime(''9999-12-31 20:59:59'', ''839:59:59'')', subtime('9999-12-31 20:59:59', '839:59:59') );
select * from func_test;
drop table func_test;

/*TIMEDIFF*/
-- test timediff(date,unknown)
select timediff(date'2022-01-01','2022-01-02');
select timediff('2022-01-01',date'2022-01-02');
select timediff('asdf', '00:00:00');
select timediff(time'00:00:00', 'asdfg');

-- TIMEDIFF ( datetime, datetime )
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(''2007-12-31 23:59:59.999999'',''2007-12-01 1:1:1.000002'')', TIMEDIFF('2007-12-31 23:59:59.999999','2007-12-01 1:1:1.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2007-12-31 23:59:59.999999'',''2007-12-01 1:1:1.000002'')',TIMEDIFF('2007-12-31 23:59:59.999999','2007-12-01 1:1:1.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2007-12-31 0:0:0.999999'',''2007-12-30 1:1:1.000002'')', TIMEDIFF('2007-12-31 0:0:0.999999','2007-12-30 1:1:1.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2000-01-01 00:00:00'',''2000-01-02 00:00:00.000001'')', TIMEDIFF('2000-01-01 00:00:00','2000-01-02 00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''2000-01-01 00:00:00'',''2000-02-01 00:00:00.000001'')', TIMEDIFF('2000-01-01 00:00:00','2000-02-01 00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''2000-01-01 00:00:01'',''2000-01-01 00:00:00.999999'')', TIMEDIFF('2000-01-01 00:00:01','2000-01-01 00:00:00.999999'));
insert into func_test(functionName, result) values('TIMEDIFF(''01-01-01 00:00:00'',''01-01-01 00:00:00.000001'')', TIMEDIFF('01-01-01 00:00:00','01-01-01 00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''0-01-01 00:00:00'',''2000-01-01 00:00:00.000001'')', TIMEDIFF('0-01-01 00:00:00','2000-01-01 00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.000001'',''2008-12-30 01:01:01.000002'')', TIMEDIFF('2008-12-31 23:59:59.000001','2008-12-30 01:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 24:59:59.000001'',''2008-12-30 01:01:01.000002'')', TIMEDIFF('2008-12-31 24:59:59.000001','2008-12-30 01:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.0000015'',''2008-12-30 01:01:01.999999'')', TIMEDIFF('2008-12-31 23:59:59.0000015','2008-12-30 01:01:01.999999'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.0000014'',''2008-12-30 01:01:01.000002'')', TIMEDIFF('2008-12-31 23:59:59.0000014','2008-12-30 01:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:60:59.000001'',''2008-12-30 01:01:01.000002'')', TIMEDIFF('2008-12-31 23:60:59.000001','2008-12-30 01:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.000001'',''2008-12-30 01:60:01.000002'')', TIMEDIFF('2008-12-31 23:59:59.000001','2008-12-30 01:60:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.000001'',''2008-12-30 24:01:01.000002'')', TIMEDIFF('2008-12-31 23:59:59.000001','2008-12-30 24:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(''2008-12-31 23:59:59.000001'',''2008-12-30 01:01:01.000002'')', TIMEDIFF('2008-12-31 23:59:59.000001','2008-12-30 01:01:01.000002'));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(''2007-12-40 23:59:59.999999'',''2000-02-28 20:59:59'')', TIMEDIFF('2007-12-40 23:59:59.999999','2000-02-28 20:59:59'));
insert into func_test(functionName, result) values('TIMEDIFF(''2007-13-31 23:59:59.999999'',''2000-02-28 20:59:59'')', TIMEDIFF('2007-13-31 23:59:59.999999','2000-02-28 20:59:59'));
insert into func_test(functionName, result) values('TIMEDIFF(''2000-02-28 20:59:59'', NULL)', TIMEDIFF('2000-02-28 20:59:59', NULL) );
insert into func_test(functionName, result) values('TIMEDIFF(NULL, ''2000-02-28 20:59:59'')', TIMEDIFF(NULL, '2000-02-28 20:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''abcd'', ''-2000-02-28 20:59:59'')', TIMEDIFF('abcd', '-2000-02-28 20:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''2000-02-28 20:59:59'', ''abcd'')', TIMEDIFF('2000-02-28 20:59:59', 'abcd') );
insert into func_test(functionName, result) values('TIMEDIFF(''2000-13-01 00:00:00'',''2000-01-01 00:00:00.000001'')', TIMEDIFF('2000-13-01 00:00:00','2000-01-01 00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''9999-12-31 23:59:59.9999999'',''9999-12-31 23:59:59.9'')', TIMEDIFF('9999-12-31 23:59:59.9999999','9999-12-31 23:59:59.9'));
insert into func_test(functionName, result) values('TIMEDIFF(''1-01-1 20:59:59'', ''2001-01-01 21:00'');', TIMEDIFF('1-01-1 20:59:59', '2001-01-01 21:00')   );
insert into func_test(functionName, result) values('TIMEDIFF(''10000-1-1 20:59:59'', ''2001-01-01 21:00'')', TIMEDIFF('10000-1-1 20:59:59', '2001-01-01 21:00') );
insert into func_test(functionName, result) values('TIMEDIFF(''1000-1-1 20:59:59'', ''1000-01-01 1:00'')', TIMEDIFF('1000-1-1 20:59:59', '1000-01-01 1:00') );
insert into func_test(functionName, result) values('TIMEDIFF(''294277-1-9 4:00:54.775807'',''294277-1-9 4:00:54.775806'')', TIMEDIFF('294277-1-9 4:00:54.775807','294277-1-9 4:00:54.775806') );
select * from func_test;
drop table func_test;
-- TIMEDIFF ( time, time )
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(''-37:59:59'', ''-39:59:59'')', TIMEDIFF('-37:59:59', '-39:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''-37:59:59'', ''39:59:59'')', TIMEDIFF('-37:59:59', '39:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''37:59:59'', ''-39:59:59'')', TIMEDIFF('37:59:59', '-39:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''37:59:59'', ''39:59:59'')', TIMEDIFF('37:59:59', '39:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''23:59:59'', ''01:01:01'')', TIMEDIFF('23:59:59', '01:01:01'));
insert into func_test(functionName, result) values('TIMEDIFF(''235959'', ''010101'')', TIMEDIFF('235959', '010101'));
insert into func_test(functionName, result) values('TIMEDIFF(235959, 010101)', TIMEDIFF(235959, 010101));
insert into func_test(functionName, result) values('TIMEDIFF(''01:00:00.999999'', ''02:00:00.999998'')', TIMEDIFF('01:00:00.999999', '02:00:00.999998'));
insert into func_test(functionName, result) values('TIMEDIFF(''24:00:00.999999'', ''02:00:00.999998'')', TIMEDIFF('24:00:00.999999', '02:00:00.999998'));
insert into func_test(functionName, result) values('TIMEDIFF(''00:00:00'',''00:00:00.000001'')', TIMEDIFF('00:00:00','00:00:00.000001'));
insert into func_test(functionName, result) values('TIMEDIFF(''02:59:00.0000001578'', ''02:00:00.999999'')', TIMEDIFF('02:59:00.0000001578', '02:00:00.999999'));
insert into func_test(functionName, result) values('TIMEDIFF(''-375959'', ''-395959'')', TIMEDIFF('-375959', '395959') );
insert into func_test(functionName, result) values('TIMEDIFF(''-375959.000002'', ''-395959.000001'')', TIMEDIFF('-375959.000002', '395959.000001') );
insert into func_test(functionName, result) values('TIMEDIFF(-375959, 395959)', TIMEDIFF(-375959, 395959) );  
insert into func_test(functionName, result) values('TIMEDIFF(-375959.000002, 395959.000001)', TIMEDIFF(-375959.000002, 395959.000001) );
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(NULL, ''-839:59:59'')', TIMEDIFF(NULL, '-839:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''-837:59:59'', NULL)', TIMEDIFF('-837:59:59', NULL) );
insert into func_test(functionName, result) values('TIMEDIFF(''abcd'', ''-839:59:59'')', TIMEDIFF('abcd', '-839:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(''-837:59:59'', ''abcd'')', TIMEDIFF('-837:59:59', 'abcd') );
insert into func_test(functionName, result) values('TIMEDIFF(''838:59:59'', ''-25:00'')', TIMEDIFF('838:59:59', '-25:00') );
insert into func_test(functionName, result) values('TIMEDIFF(''-838:59:59'', ''25:00'')', TIMEDIFF('-838:59:59', '25:00') );
select * from func_test;
drop table func_test;
-- TIMEDIFF ( time, datetime ), TIMEDIFF ( time, datetime )
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(''2000-02-28 20:59:59'', ''4:00'')', TIMEDIFF('2000-02-28 20:59:59', '4:00') );
insert into func_test(functionName, result) values('TIMEDIFF(''20:59:59'', ''2000-02-28 23:00'')', TIMEDIFF('2000-02-28 20:59:59', '2000-02-28 23:00') );
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMEDIFF(cast(235959 as time),cast(010101 as time))', TIMEDIFF(cast(235959 as time),cast(010101 as time)));
insert into func_test(functionName, result) values('TIMEDIFF(235959::time, 010101::time)', TIMEDIFF(235959::time, 010101::time));
insert into func_test(functionName, result) values('TIMEDIFF(cast(''2008-12-31 23:59:59.000001'' as datetime), time''22:01:01.000002'')', TIMEDIFF(cast('2008-12-31 23:59:59.000001' as datetime), time'22:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(cast(''2008-12-31 23:59:59.000001'' as datetime), date''2008-12-30'')', TIMEDIFF(cast('2008-12-31 23:59:59.000001' as datetime), date'2008-12-30'));
insert into func_test(functionName, result) values('TIMEDIFF(time''23:59:59.000001'', cast(''2008-12-30 22:01:01.000002'' as datetime)))', TIMEDIFF(time'23:59:59.000001', cast('2008-12-30 22:01:01.000002' as datetime)));
insert into func_test(functionName, result) values('TIMEDIFF(time''23:59:59.000001'', date''2008-12-30'')', TIMEDIFF(time'23:59:59.000001', date'2008-12-30'));
insert into func_test(functionName, result) values('TIMEDIFF(time''-37:59:59'', time''-39:59:59'')', TIMEDIFF(time'-37:59:59', time'-39:59:59') );
insert into func_test(functionName, result) values('TIMEDIFF(date''2008-12-31'', cast(''2008-12-30 01:01:01.000002'' as datetime)))', TIMEDIFF(date'2008-12-31', cast('2008-12-30 01:01:01.000002' as datetime)));
insert into func_test(functionName, result) values('TIMEDIFF(date''2008-12-31'', date''2008-12-30'')', TIMEDIFF(date'2008-12-31', date'2008-12-30'));
insert into func_test(functionName, result) values('TIMEDIFF(date''2008-12-31'', time''22:01:01.000002'')', TIMEDIFF(date'2008-12-31', time'22:01:01.000002'));
insert into func_test(functionName, result) values('TIMEDIFF(cast(235956 as unsigned), cast(010101 as unsigned))', TIMEDIFF(cast(235956 as unsigned), cast(010101 as unsigned)));
insert into func_test(functionName, result) values('TIMEDIFF(year''2021'', year''2022'')', TIMEDIFF(year'2021', year'2022'));
select * from func_test;
drop table func_test;
/*TIME*/

-- TIME (time)
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIME(''00:00:00'')',TIME('00:00:00'));
insert into func_test(functionName, result) values('TIME(''240000'')',TIME('240000'));
insert into func_test(functionName, result) values('TIME(240000)',TIME(240000));
insert into func_test(functionName, result) values('TIME(240000.000001)',TIME(240000.000001));
insert into func_test(functionName, result) values('TIME(''25:30:30'')',TIME('25:30:30'));
insert into func_test(functionName, result) values('TIME(''-25:30:30'')',TIME('-25:30:30'));
insert into func_test(functionName, result) values('TIME(''838:59:59'')',TIME('838:59:59'));
insert into func_test(functionName, result) values('TIME(''-838:59:59'')',TIME('-838:59:59'));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIME(''838:0:0'')',TIME('838:0:0'));
insert into func_test(functionName, result) values('TIME(''839:0:0'')',TIME('839:0:0'));
insert into func_test(functionName, result) values('TIME(''-839:0:0'')',TIME('-839:0:0'));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIME(''00:00:59.9999'')',TIME('00:00:59.9999'));
insert into func_test(functionName, result) values('TIME(''00:00:59.999999000'')',TIME('00:00:59.999999000'));
insert into func_test(functionName, result) values('TIME(''83:59:59.0000000009'')',TIME('83:59:59.0000000009'));
insert into func_test(functionName, result) values('TIME(''00:10:59.999999999'')',TIME('00:10:59.999999999'));
insert into func_test(functionName, result) values('TIME(''00:59:59.999999999'')',TIME('00:59:59.999999999'));
insert into func_test(functionName, result) values('TIME(''838:59:59.000000500'')',TIME('838:59:59.000000500'));
insert into func_test(functionName, result) values('TIME(''83:59:59.0000000004'')',TIME('83:59:59.0000000004'));
select * from func_test;
drop table func_test;
-- TIME(datetime)
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIME(''2003-12-31 01:02:03'')', TIME('2003-12-31 01:02:03'));
insert into func_test(functionName, result) values('TIME(''20031231010203'')', TIME('20031231010203'));
insert into func_test(functionName, result) values('TIME(''2003-12-31 01:02:03.000123'')', TIME('2003-12-31 01:02:03.000123'));
insert into func_test(functionName, result) values('TIME(''20031231010203.000123'')', TIME('20031231010203.000123'));
insert into func_test(functionName, result) values('TIME(20031231010203)', TIME(20031231010203));
insert into func_test(functionName, result) values('TIME(20031231010203.000123)', TIME(20031231010203.000123));
insert into func_test(functionName, result) values('TIME(''2003-12-31 01:02:03.0001234'')',TIME('2003-12-31 01:02:03.0001234'));
insert into func_test(functionName, result) values('TIME(''2003-12-31 01:02:03.0001235'')', TIME('2003-12-31 01:02:03.0001235'));
insert into func_test(functionName, result) values('TIME(''2003-12-31 01:02:03.00012345'')', TIME('2003-12-31 01:02:03.00012345'));
insert into func_test(functionName, result) values('TIME(''20031231010203.0001235'')', TIME('20031231010203.0001235'));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIME(''9999-12-31 59:59:59'')', TIME('9999-12-31 59:59:59'));
insert into func_test(functionName, result) values('TIME(''10000-01-01 00:00:00'')', TIME('10000-01-01 00:00:00'));
insert into func_test(functionName, result) values('TIME(''0000-12-31 59:59:59'')', TIME('0000-12-31 59:59:59'));
insert into func_test(functionName, result) values('TIME(''2003-12-40 01:02:03'')', TIME('2003-12-40 01:02:03'));
insert into func_test(functionName, result) values('TIME(''2003-13-31 01:02:03.000123'')', TIME('2003-13-31 01:02:03'));
insert into func_test(functionName, result) values('TIME(time''1:1:1'')', TIME(time'1:1:1'));
insert into func_test(functionName, result) values('TIME(date''2000-01-01'')', TIME(date'2000-01-01'));
insert into func_test(functionName, result) values('TIME(cast(''2001-12-10 23:59:59'' as datetime))', TIME(cast('2001-12-10 23:59:59' as datetime)));
insert into func_test(functionName, result) values('TIME(date''5874897-12-31'')', TIME(date'5874897-12-31'));
insert into func_test(functionName, result) values('TIME(date''10000-01-01'')', TIME(date'10000-01-01'));
select * from func_test;
drop table func_test;
/*TIME_FORMAT*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values(' TIME_FORMAT(''00:00:00'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('00:00:00', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''240000'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('240000', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(240000, ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT(240000, '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(240000.000001, ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT(240000.000001, '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''25:30:30'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('25:30:30', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''-25:30:30'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('-25:30:30', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''838:59:59'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('838:59:59', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''-838:59:59'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('-838:59:59', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values('TIME_FORMAT(''-838:59:59'',''%h'')', TIME_FORMAT('-838:59:59','%h'));
insert into func_test(functionName, result) values('TIME_FORMAT(''-838:59:59'',''%h'')', TIME_FORMAT('-838:59:59','%h'));
insert into func_test(functionName, result) values('TIME_FORMAT(''-838:59:59'',''%k'')', TIME_FORMAT('-838:59:59','%k'));
insert into func_test(functionName, result) values('TIME_FORMAT(''-838:59:59'',''%r'')', TIME_FORMAT('-838:59:59','%r'));
insert into func_test(functionName, result) values('TIME_FORMAT(''-838:59:59'',''%i'')', TIME_FORMAT('-838:59:59','%i'));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values(' TIME_FORMAT(''838:0:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('838:0:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''839:0:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('839:0:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''-839:0:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('-839:0:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:-1:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:-1:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:60:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:60:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:59.5:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:59.5:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:59.4:0'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:59.4:0', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:0:-1'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:0:-1', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0:0:60'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0:0:60', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''00:00:59.9999'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('00:00:59.9999', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''00:00:59.999999000'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('00:00:59.999999000', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''83:59:59.0000000009'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('83:59:59.0000000009', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''00:10:59.999999999'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('00:10:59.999999999', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''00:59:59.999999999'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('00:59:59.999999999', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''838:59:59.000000500'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('838:59:59.000000500', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''83:59:59.0000000004'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('83:59:59.0000000004', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''100:59:59.0123'', ''%a|%b|%c|%D|%d|%e|%j|%M|%m'')    ', TIME_FORMAT('100:59:59.0123', '%a|%b|%D|%j|%M|%U|%u|%V|%v|%W|%w|%X|%x'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''100:59:59.0123'', ''%U|%u|%V|%v|%W|%w|%X|%x|%Y|%y'')    ', TIME_FORMAT('100:59:59.0123', '%c|%d|%e|%m|%Y|%y'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''100:59:59.0123'', ''%N|%n abcd'')    ', TIME_FORMAT('100:59:59.0123', '%N|%n abcd'));
select * from func_test;
drop table func_test;
-- TIME_FORMAT(untime)
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values(' TIME_FORMAT(''2003-12-31 01:02:03.0123'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('2003-12-31 01:02:03.0123', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''20031231010203.0123'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('20031231010203.0123', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(20031231010203, ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT(20031231010203, '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(20031231010203.0123, ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT(20031231010203.0123, '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''10000-01-01 01:01:01'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('10000-01-01 01:01:01', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''0000-12-31 23:59:59'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('0000-12-31 23:59:59', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''2021-12-32 01:01:01'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('2021-12-32 01:01:01', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(''2021-12-31'', ''%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'')    ', TIME_FORMAT('2021-12-31', '%T|%r|%H|%h|%I|%i|%S|%f|%p|%k'));
insert into func_test(functionName, result) values(' TIME_FORMAT(date''2021-12-31'', ''%T|%r||%f'')    ', TIME_FORMAT(date'2021-12-31', '%T|%r||%f'));
insert into func_test(functionName, result) values(' TIME_FORMAT(time''01:01:01'', ''%T|%r||%f'')    ', TIME_FORMAT(time'01:01:01', '%T|%r||%f'));
insert into func_test(functionName, result) values(' TIME_FORMAT(cast(''2001-12-10 23:59:59'' as datetime), ''%T|%r||%f'')    ', TIME_FORMAT(cast('2001-12-10 23:59:59' as datetime), '%T|%r||%f'));
insert into func_test(functionName, result) values(' TIME_FORMAT(date''5874897-12-31'', ''%t|%r|%h|%h'')', TIME_FORMAT(date'5874897-12-31', '%t|%r|%h|%h'));
insert into func_test(functionName, result) values(' TIME_FORMAT(date''10000-1-1'', date''9999-12-31'')', TIME_FORMAT(date'10000-1-1', date'9999-12-31'));
select * from func_test;
drop table func_test;
/*TIMESTAMP*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31'')', TIMESTAMP('2003-12-31'));
insert into func_test(functionName, result) values('TIMESTAMP(''20031231'')', TIMESTAMP('20031231'));
insert into func_test(functionName, result) values('TIMESTAMP(20031231)', TIMESTAMP(20031231));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 12:00:00.123456'')', TIMESTAMP('2003-12-31 12:00:00.123456'));
insert into func_test(functionName, result) values('TIMESTAMP(''20000229120000.1234567'')', TIMESTAMP('20000229120000.1234567'));
insert into func_test(functionName, result) values('TIMESTAMP(20000229120000.1234567)', TIMESTAMP(20000229120000.1234567));
insert into func_test(functionName, result) values('TIMESTAMP(20000229120000)', TIMESTAMP(20000229120000));
insert into func_test(functionName, result) values('TIMESTAMP(''0000-01-01'')', TIMESTAMP('0000-01-01')); 
insert into func_test(functionName, result) values('TIMESTAMP(''9999-12-31'')', TIMESTAMP('9999-12-31')); 
insert into func_test(functionName, result) values('TIMESTAMP(''10000-01-01'')', TIMESTAMP('10000-01-01')); 
insert into func_test(functionName, result) values('TIMESTAMP(''0000-01-01 00:00:00.000001'')', TIMESTAMP('0000-01-01 00:00:00.000001')); 
insert into func_test(functionName, result) values('TIMESTAMP(''9999-12-31 23:59:59.999999'')', TIMESTAMP('9999-12-31 23:59:59.999999')); 
insert into func_test(functionName, result) values('TIMESTAMP(''10000-01-01 00:00:00'')', TIMESTAMP('10000-01-01 00:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(date''2003-01-01'')', TIMESTAMP(cast('2003-01-01' as date)));
insert into func_test(functionName, result) values('TIMESTAMP(cast(''2007-12-10 23:59:59'' as datetime))', TIMESTAMP(cast('2007-12-10 23:59:59' as datetime)));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMP(null)', TIMESTAMP(null)); 
insert into func_test(functionName, result) values('TIMESTAMP(''2003-13-01'')', TIMESTAMP('2003-13-01'));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-2-29'')', TIMESTAMP('2003-2-29'));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 24:00:00'')', TIMESTAMP('2003-12-31 24:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 23:60:00'')', TIMESTAMP('2003-12-31 23:60:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 23:59:60'')', TIMESTAMP('2003-12-31 23:59:60'));
select * from func_test;
drop table func_test;
-- TIMESTAMP(expr1, expr2)
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31'',''12:00:00.9999995'')', TIMESTAMP('2003-12-31','12:00:00.9999995'));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 12:00:00'',''32:00:00'')', TIMESTAMP('2003-12-31 12:00:00','32:00:00'));
insert into func_test(functionName, result) values('TIMESTAMP(''20000229'',''100:00:00.000001'')', TIMESTAMP('20000229','100:00:00.000001'));
insert into func_test(functionName, result) values('TIMESTAMP(20000229,1000000)', TIMESTAMP(20000229, 1000000));
insert into func_test(functionName, result) values('TIMESTAMP(20000229,1000000.000001)', TIMESTAMP(20000229, 1000000.000001));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 12:00:00'',''-12:00:00.000001'')', TIMESTAMP('2003-12-31 12:00:00','-12:00:00.000001'));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 12:00:00.000001'',''12:00:00.999999'')', TIMESTAMP('2003-12-31 12:00:00.000001','12:00:00.999999'));
insert into func_test(functionName, result) values('TIMESTAMP(20031231120000,-120000)', TIMESTAMP(20031231120000,-120000));
insert into func_test(functionName, result) values('TIMESTAMP(20031231120000.000002,-120000.000001)', TIMESTAMP(20031231120000.000002,-120000.000001));
insert into func_test(functionName, result) values('TIMESTAMP(date''2003-01-01'', date''2000-01-01'')', TIMESTAMP(date'2003-01-01', date'2000-01-01'));
insert into func_test(functionName, result) values('TIMESTAMP(cast(''2007-12-10 23:59:59'' as datetime), cast(''2007-12-10 22:59:59'' as datetime))', TIMESTAMP(cast('2007-12-10 23:59:59' as datetime), cast('2007-12-10 22:59:59' as datetime)));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMP(NULL, NULL)', TIMESTAMP(NULL, NULL)); 
insert into func_test(functionName, result) values('TIMESTAMP(''2003-2-29'',''25:00:00'')', TIMESTAMP('2003-2-29', '25:00:00'));
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 24:00:00'',''25:00:00'')', TIMESTAMP('2003-12-31 24:00:00', '25:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''2003-12-31 23:60:00'',''25:00:00'')', TIMESTAMP('2003-12-31 23:60:00', '25:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''10000-01-01'',''25:00:00'')', TIMESTAMP('10000-01-01', '25:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''1000-12-31'',''838:59:59'')', TIMESTAMP('1000-12-31', '838:59:59')); 
insert into func_test(functionName, result) values('TIMESTAMP(''9999-12-31'',''23:59:59.999999'')', TIMESTAMP('9999-12-31', '23:59:59.999999')); 
insert into func_test(functionName, result) values('TIMESTAMP(''0001-01-01'', ''00:00:00'')', TIMESTAMP('0001-01-01', '00:00:00')); 
insert into func_test(functionName, result) values('TIMESTAMP(''0001-01-01 00:00:00'', ''-00:00:00.000001'')', TIMESTAMP('0001-01-01 00:00:00', '-00:00:00.000001')); 
select * from func_test;
drop table func_test;

/*TIMESTAMPADD*/
-- date
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,1,''2022-07-27'')', TIMESTAMPADD(YEAR,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,-1,''2022-07-27'')', TIMESTAMPADD(YEAR,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,1,''2022-07-27'')', TIMESTAMPADD(MONTH,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,-1,''2022-07-27'')', TIMESTAMPADD(MONTH,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,1,''2022-07-27'')', TIMESTAMPADD(WEEK,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,-1,''2022-07-27'')', TIMESTAMPADD(WEEK,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,1,''2022-07-27'')', TIMESTAMPADD(DAY,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,-1,''2022-07-27'')', TIMESTAMPADD(DAY,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,1,''2022-07-27'')', TIMESTAMPADD(HOUR,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,-1,''2022-07-27'')', TIMESTAMPADD(HOUR,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,1,''2022-07-27'')', TIMESTAMPADD(MINUTE,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,-1,''2022-07-27'')', TIMESTAMPADD(MINUTE,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,1,''2022-07-27'')', TIMESTAMPADD(SECOND,1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,-1,''2022-07-27'')', TIMESTAMPADD(SECOND,-1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,-1, 2022-07-27)', TIMESTAMPADD(SECOND,-1, 20220727));
select * from func_test;
drop table func_test;
-- datetime
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(YEAR,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(YEAR,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(MONTH,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(MONTH,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(WEEK,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(WEEK,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(DAY,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(DAY,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(HOUR,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(HOUR,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(MINUTE,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(MINUTE,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,1,''2022-07-27 08:30:00'')', TIMESTAMPADD(SECOND,1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,-1,''2022-07-27 08:30:00'')', TIMESTAMPADD(SECOND,-1,'2022-07-27 08:30:00'));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,-1, 20220727083000)', TIMESTAMPADD(SECOND,-1, 20220727083000));
insert into func_test(functionName, result) values('TIMESTAMPADD(SECOND,-1, 20220727083000.000001)', TIMESTAMPADD(SECOND,-1, 20220727083000.000001));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,1.1,''2022-07-27'')', TIMESTAMPADD(YEAR,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,-1.1,''2022-07-27'')', TIMESTAMPADD(YEAR,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,1.6,''2022-07-27'')', TIMESTAMPADD(YEAR,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,-1.6,''2022-07-27'')', TIMESTAMPADD(YEAR,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,1.1,''2022-07-27'')', TIMESTAMPADD(MONTH,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,-1.1,''2022-07-27'')', TIMESTAMPADD(MONTH,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,1.6,''2022-07-27'')', TIMESTAMPADD(MONTH,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MONTH,-1.6,''2022-07-27'')', TIMESTAMPADD(MONTH,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,1.1,''2022-07-27'')', TIMESTAMPADD(WEEK,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,-1.1,''2022-07-27'')', TIMESTAMPADD(WEEK,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,1.6,''2022-07-27'')', TIMESTAMPADD(WEEK,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(WEEK,-1.6,''2022-07-27'')', TIMESTAMPADD(WEEK,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,1.1,''2022-07-27'')', TIMESTAMPADD(DAY,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,-1.1,''2022-07-27'')', TIMESTAMPADD(DAY,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,1.6,''2022-07-27'')', TIMESTAMPADD(DAY,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,-1.6,''2022-07-27'')', TIMESTAMPADD(DAY,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,1.1,''2022-07-27'')', TIMESTAMPADD(HOUR,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,-1.1,''2022-07-27'')', TIMESTAMPADD(HOUR,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,1.6,''2022-07-27'')', TIMESTAMPADD(HOUR,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,-1.6,''2022-07-27'')', TIMESTAMPADD(HOUR,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,1.1,''2022-07-27'')', TIMESTAMPADD(MINUTE,1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,-1.1,''2022-07-27'')', TIMESTAMPADD(MINUTE,-1.1,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,1.6,''2022-07-27'')', TIMESTAMPADD(MINUTE,1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(MINUTE,-1.6,''2022-07-27'')', TIMESTAMPADD(MINUTE,-1.6,'2022-07-27'));
insert into func_test(functionName, result) values('TIMESTAMPADD(HOUR,1,date''2022-04-05'')', TIMESTAMPADD(HOUR,1,date'2022-04-05'));
insert into func_test(functionName, result) values('TIMESTAMPADD(DAY,1,cast(''2022-04-05'' as datetime))', TIMESTAMPADD(day,1,cast('2022-04-05' as datetime)));
insert into func_test(functionName, result) values('TIMESTAMPADD(YEAR,''abc'',''0001-01-01'')', TIMESTAMPADD(year,'abc','0001-01-01'));
select * from func_test;
drop table func_test;
