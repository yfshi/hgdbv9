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

select day(true);       -- bug#z5063
select day(false);      -- bug#z5063
select day(0);          -- bug#z5063

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
select dayofmonth('2025-07-25 15:30:00'::timestamp);    -- bug#z5586

/*DAYOFWEEK*/
SELECT dayofweek('2022-10-09 09:55:28.12345');
SELECT dayofweek(20221009);
select dayofweek('14:14:14');
select dayofweek(time'2022-06-28 00:00:00');


/*DAYOFYEAR*/
SELECT dayofyear('2022-10-09 09:55:28.12345');
SELECT dayofyear(20221009);
select dayofyear('20:38:40');
select dayofyear(203840);


/*QUARTER*/
SELECT quarter('2022-10-09 09:55:28.12345');
SELECT quarter(20221009);
select quarter('20:38:40');
--select quarter(time'20:38:40');
select quarter(203840);
--select quarter(time'203840');
select quarter(true);       -- bug#z5083
select quarter(false);      -- bug#z5083
select quarter(0);          -- bug#z5083

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
select hour(true);      -- bug#z5086
select hour(false);     -- bug#z5086
select hour(0);         -- bug#z5086
select hour(cast('14:12:12' as time));         -- bug#z5587

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
select microsecond(true);       -- bug#z5093
select microsecond(false);      -- bug#z5093
select microsecond(0);          -- bug#z5093
select microsecond(cast('2024-03-15 12:34:56' as datetime));        -- bug#z5585

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
select minute(true);    -- bug#z5096
select minute(false);   -- bug#z5096
select minute(0);       -- bug#z5096
select minute(cast('2024-03-15 12:34:56' as datetime));     -- bug#z5588

/*SECOND*/
SELECT second('2022-10-09 09:55:28.12345');
select second('10:20:30');
select second('30:20:10.123456');
select second('2022-10-09 11:22:33');
select second('2022-10-09 11:22:33.456789');
select second('20221009112233');
select second('373839');
select second('2022-10-09');
select second(true);    -- bug#z5101
select second(false);   -- bug#z5101
select second(0);       -- bug#z5099

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
/* Begin - bug#z5127 */
select timediff('1:1:1.100000', '1:1:0.200000'); -- 00:00:00.900000
select timediff('1:1:1.100000', '1:1:1.200000'); -- -00:00:00.100000
select timediff('1:1:2.100000', '1:1:1.200000'); -- 00:00:00.900000
select timediff('1:1:1.100000', '1:1:2.200000'); -- -00:00:01.100000
select TIMEDIFF('01-01-01 00:00:00','01-01-01 00:00:00.000001'); -- -00:00:00.000001
select TIMEDIFF('1-01-1 20:59:59', '2001-01-01 21:00'); --  -838:59:59
select timediff('1:1:1.100000', '1:1:90.200000'); -- NULL
select timediff('1:1:91.100000', '1:1:0.200000'); -- NULL
/* End - bug#z5127 */

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

/* Begin - bug#z5128 */
SELECT TIME('00:00:00');    -- 00:00:00
SELECT TIME('23:59:59');    -- 23:59:59
SELECT TIME('-23:59:59');   -- -23:59:59
select TIME(240000.000001); -- 24:00:00.000001
select TIME('838:59:59');   -- 838:59:59
select TIME('839:59:59');   -- 838:59:59
select TIME('836:99:59');   -- NULL
select TIME('836:59:99');   -- NULL
select time('-0:0:0.100000');  -- -00:00:00.100000
/* End - bug#z5128 */

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

set datestyle = 'ISO, YMD';
--to_days
select to_days(950501);--728779
select to_days(0);--NULL warning
select to_days(1);--NULL warning
select to_days(100);--NULL warning
select to_days(101);--730485
select to_days(20000012);--NULL warning
select to_days(20001111250000);--NULL warning
select to_days(121212.22);--warning 735214
select to_days(20120000121212.22);--NULL warning
select to_days('2007-10-07');--733321
select to_days('2011'::year);--error
select to_days(cast('2025-04-21 10:12:12.22' as datetime(6)));--739727
select to_days(cast(201212128182822.222 as datetime));--NULL warning
select to_days();--error
select to_days(NULL);--NULL
select to_days(1,2,3);--error

--to_seconds
select to_seconds(950501);--62966505600
select to_seconds('2007-10-07');--63358934400
select to_seconds(1);--NULL warning
select to_seconds(100);--NULL warning
select to_seconds(101);--63113904000
select to_seconds(20000012);--NULL warning
select to_seconds(20001111250000);--NULL warning
select to_seconds(121212.22);--warning 63522489600
select to_seconds(20120000121212.22);--NULL warning
--select to_seconds(cast('2025-04-22 23:59:59' as time));
--select to_seconds(cast('23:59:59' as time));
--select to_seconds(cast(8382822.222 as time));
--select to_seconds(cast(8382822.5999999 as time));
--select to_seconds(cast(8382822.5999999 as time(6)));
select to_seconds(cast(201212128182822.222 as datetime));--NULL warning
select to_seconds(cast(20121212182822.222 as datetime));--63522556102
select to_seconds(cast(20121212182822.5999999 as datetime));--63522556103
select to_seconds(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--63912449532
select to_seconds();--error
select to_seconds(NULL);--NULL
select to_seconds(1,2);--error
/* Begin - BUG#8127 */
SELECT TO_SECONDS('0000-01-01 00:00:00');
/* End - BUG#8127 */

--dayname
set lc_time_names to default;
select dayname(950501);--Monday
select dayname('2007-10-07');--Sunday
select dayname(1);--NULL warning
select dayname(100);--NULL warning
select dayname(101);--Saturday
select dayname(101.6);--Saturday warning
select dayname(20000012);--NULL warning
select dayname(20001111250000);--NULL warning
select dayname(121212.22);--warning Wednesday
select dayname(20120000121212.22);--NULL warning
select dayname(cast(201212128182822.222 as datetime));--NULL warning
select dayname(cast(20121212182822.222 as datetime));--Wednesday
select dayname(cast(20121212182822.5999999 as datetime));--Wednesday
select dayname(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--Monday
select dayname();--error
select dayname(NULL);--NULL
select dayname(1,2);--error

--monthname
select monthname(950501);--May
select monthname('2007-10-07');--October
select monthname('2008-02-03');--February
select monthname(1);--NULL warning
select monthname(100);--NULL warning
select monthname(101);--January
select monthname(101.6);-- warning January
select monthname(20000012);--NULL
select monthname(20001111250000);--NULL warning
select monthname(121212.22);--warning December
select monthname(20120000121212.22);--NULL 
select monthname(cast(201212128182822.222 as datetime));--NULL warning
select monthname(cast(20121212182822.222 as datetime));--December
select monthname(cast(20121212182822.5999999 as datetime));--December
select monthname(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--April
select monthname();--error
select monthname(NULL);--NULL
select monthname(1,2);--error

--time_to_sec
select time_to_sec(101);--61
select time_to_sec(10);--10
select time_to_sec(1);--1
select time_to_sec(1.2);--1
select time_to_sec(0);--0
select time_to_sec(89);--NULL warning
select time_to_sec(8.9);--8
select time_to_sec(12128.9);--4888
select time_to_sec(950501);--342301
select time_to_sec('2007-10-07');--1207 warning
select time_to_sec('2008-02-03');--1208 warning
select time_to_sec(100);--60
select time_to_sec(101.6);--61
select time_to_sec(20000012);--NULL warning
select time_to_sec(20001111250000);--NULL warning
select time_to_sec(121212.22);--43932
select time_to_sec(20120000121212.22);--NULL warning
select time_to_sec('23:59:59');--86399
select time_to_sec('00:39:38');--2378
select time_to_sec(cast('2025-04-22 23:59:59' as time));--86399
select time_to_sec(cast('23:59:59' as time));--86399
select time_to_sec(cast(8382822.222 as time));--3018502
select time_to_sec(cast(8382822.5999999 as time));--3018503
select time_to_sec(cast(8382822.5999999 as time(6)));--3018502
select time_to_sec(cast(201212128182822.222 as datetime));--NULL warning
select time_to_sec(cast(20121212182822.222 as datetime));--66502
select time_to_sec(cast(20121212182822.5999999 as datetime));--66503
select time_to_sec(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--36732
select time_to_sec();--error
select time_to_sec(NULL);--NULL
select time_to_sec(1,2);--error

--date
select date(101);--2000-01-01
select date(10);--NULL warning
select date(1);--NULL warning
select date(1.2);--NULL warning
select date(0);--0000-00-00
select date(89);--NULL warning
select date(8.9);--NULL warning
select date(12128.9);--NULL warning
select date(950501);--1995-05-01
select date('2007-10-07');--2007-10-07
select date('2008-02-03');--2008-02-03
select date(100);--NULL warning
select date(101.6);--2000-01-01
select date(20000012);--2000-00-12
select date(20001111250000);--NULL warning
select date(200011112323233242424242456565);--NULL warning
select date(121212.22);--2012-12-12
select date(20120000121212.22);-- 2012-00-00 
select date('23:59:59');--NULL warning
select date('00:39:38');--NULL warning
--select date(cast('2025-04-22 23:59:59' as time));
--select date(cast('23:59:59' as time));
--select date(cast(8382822.222 as time));
--select date(cast(8382822.5999999 as time));
--select date(cast(8382822.5999999 as time(6)));
select date(cast(2011 as year));--NULL warning
select date(cast(201212128182822.222 as datetime));--NULL warning
select date(cast(20121212182822.222 as datetime));--2012-12-12
select date(cast(20121212182822.5999999 as datetime));--2012-12-12
select date(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--2025-04-21
select date();--syntax error
select date(NULL);--NULL
select date(1,2);--syntax error

--week
set default_week_format to default;
select week(101);--0
select week(10);--NULL warning
select week(1);--NULL warning
select week(1.2);--NULL warning
select week(0);--NULL warning
select week(89);--NULL warning
select week(8.9);--NULL warning
select week(12128.9);--NULL warning
select week(950501);--18
select week('2007-10-07');--40
select week('2008-02-03');--5
select week(100);--NULL warning
select week(101.6);--NULL warning
select week(20000012);--NULL warning
select week(20001111250000);--NULL warning
select week(200011112323233242424242456565);--NULL warning
select week(121212.22);--50 warning
select week(20120000121212.22);--NULL warning
select week('23:59:59');--NULL warning
select week('00:39:38');--NULL warning
select week(cast(2011 as year));--NULL warning
select week(cast(201212128182822.222 as datetime));--NULL warning
select week(cast(20121212182822.222 as datetime));--50
select week(cast(20121212182822.5999999 as datetime));--50
select week(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--16
select week('1987-01-01');--0
select week();--error
select week(NULL);--NULL
select week(1,2,3);--error

select week(101, 2);--52
select week(10, 2);--NULL warning
select week(1, 2);--NULL warning
select week(1.2, 2);--NULL warning
select week(0, 2);--NULL warning
select week(89, 2);--NULL warning
select week(8.9, 2);--NULL warning
select week(12128.9, 2);--NULL warning
select week(950501, 2);--18
select week('2007-10-07', 2);--40
select week('2008-02-03', 2);--5
select week(100, 2);--NULL warning
select week(101.6, 2);--NULL warning
select week(20000012, 2);--NULL warning
select week(20001111250000, 2);--NULL warning
select week(200011112323233242424242456565, 2);--NULL warning
select week(121212.22, 2);--50 warning
select week(20120000121212.22, 2);--NULL warning
select week('23:59:59', 2);--NULL warning
select week('00:39:38', 2);--NULL warning
select week(cast(2011 as year), 2);--NULL warning
select week(cast(201212128182822.222 as datetime), 2);--NULL warning
select week(cast(20121212182822.222 as datetime), 2);--50
select week(cast(20121212182822.5999999 as datetime), 2);--50
select week(cast('2025-04-21 10:12:12.6999992' as datetime(6)), 2);--16
select week('1987-01-01', 2);--52
select week(NULL, NULL);--NULL

--yearweek
select yearweek('1987-01-01');--198652
select yearweek(101);--199952
select yearweek(10);--NULL warning
select yearweek(1);--NULL warning
select yearweek(1.2);--NULL warning
select yearweek(0);--NULL warning
select yearweek(89);--NULL warning
select yearweek(8.9);--NULL warning
select yearweek(12128.9);--NULL warning
select yearweek(950501);--199518
select yearweek('2007-10-07');--200740
select yearweek('2008-02-03');--200805
select yearweek(100);--NULL warning
select yearweek(101.6);--199952 warning
select yearweek(20000012);--NULL warning
select yearweek(20001111250000);--NULL warning
select yearweek(200011112323233242424242456565);--NULL warning
select yearweek(121212.22);--201250 warning
select yearweek(20120000121212.22);--NULL warning
select yearweek('23:59:59');--NULL warning
select yearweek('00:39:38');--NULL warning
select yearweek(cast(2011 as year));--NULL warning
select yearweek(cast(201212128182822.222 as datetime));--NULL warning
select yearweek(cast(20121212182822.222 as datetime));--201250
select yearweek(cast(20121212182822.5999999 as datetime));--201250
select yearweek(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--202516
select yearweek();--error
select yearweek(NULL);--NULL

select yearweek('1987-01-01', 4);--198653
select yearweek(101, 4);--199952
select yearweek(10, 4);--NULL warning
select yearweek(1, 4);--NULL warning
select yearweek(1.2, 4);--NULL warning
select yearweek(0, 4);--NULL warning
select yearweek(89, 4);--NULL warning
select yearweek(8.9, 4);--NULL warning
select yearweek(12128.9, 4);--NULL warning
select yearweek(950501, 4);--199518
select yearweek('2007-10-07', 4);--200741
select yearweek('2008-02-03', 4);--200806
select yearweek(100, 4);--NULL warning
select yearweek(101.6, 4);--199952 warning
select yearweek(20000012, 4);--NULL warning
select yearweek(20001111250000, 4);--NULL warning
select yearweek(200011112323233242424242456565, 4);--NULL warning
select yearweek(121212.22, 4);--201250 warning
select yearweek(20120000121212.22, 4);--NULL warning
select yearweek('23:59:59', 4);--NULL warning
select yearweek('00:39:38', 4);--NULL warning
select yearweek(cast(2011 as year), 4);--NULL warning
select yearweek(cast(201212128182822.222 as datetime), 4);--NULL warning
select yearweek(cast(20121212182822.222 as datetime), 4);--201250
select yearweek(cast(20121212182822.5999999 as datetime), 4);--201250
select yearweek(cast('2025-04-21 10:12:12.6999992' as datetime(6)), 4);--202517
select yearweek(NULL, NULL);--NULL
select yearweek(1, 2, 3);--error
/* Begin - BUG#Z5575 */
select yearweek('2023-01-01',-3);
select yearweek('2023-01-01',-1);
select yearweek('2023-01-01',-13);
select yearweek('2023-01-01',-20);
select yearweek('2023-01-01',-1000);
/* End - BUG#Z5575 */
--datediff
select datediff('2007-12-31 23:59:59', '2007-12-30');--1
select datediff('2007-11-30 23:59:59', '2007-12-31');---31
select datediff(20071130, '2007-12-31');---31
select datediff(20071130, 101);--2890
select datediff(20071130, 101.55);--2890
select datediff('2007-11-30 23:59:59', 20071231);---31
select datediff(cast(20121212182822.222 as datetime), 20071231);--1808
select datediff(cast(20121212182822.222 as datetime), 20071231.555);--1808
select datediff(cast(20121212182822.222 as date), 20071231);--1808
select datediff(cast(20121212182822.222 as date), 20071231.333);--1808
select datediff(cast(20121212182822.222 as date), '2007-12-31');--1808
select datediff(cast(20121212182822.222 as datetime), '2007-12-31');--1808
select datediff(cast(20121212182822.222 as date), cast(20121112182822.222 as date));---30
select datediff(cast(20121212182822.222 as datetime), cast(20121112182822.222 as datetime));--30
select datediff(cast(20121212182822.222 as date), cast(20121112182822.222 as datetime));--30
select datediff(cast(20121212182822.222 as date), cast(20121112182822.222 as datetime(6)));--30
--select datediff(cast(20121212182822.222 as date), cast(20121112182822.222 as time));
--select datediff(cast(20121212182822.222 as date), cast(20121112182822.222 as time(6)));
select datediff(NULL, NULL);--NULL
select datediff('2007-12-31 23:59:59');--error
select datediff();--error

/* Begin - bug#z5751 */
select pg_typeof(datediff(20231010::int,20231009::int));
select pg_typeof(datediff('2023-10-10 12:34:56'::datetime,'2023-10-09 08:00:00'::datetime));
select pg_typeof(datediff('2023-10-10 12:34:56','2023-10-09 08:00:00'));
/* End - bug#z5751 */

--from_days
select from_days(101);--0000-00-00
select from_days(10);--0000-00-00
select from_days(1);--0000-00-00
select from_days(1.2);--0000-00-00
select from_days(0);--0000-00-00
select from_days(89);--0000-00-00
select from_days(8.9);--0000-00-00
select from_days(12128.9);-- 0033-03-17
select from_days(950501);--2602-05-21
select from_days(365.5);--0001-01-01
select from_days(365.4);--0000-00-00
select from_days(3652500);--0000-00-00
select from_days(3652499);--10000-03-15
select from_days(100);--0000-00-00
select from_days(101.6);--0000-00-00
select from_days(20000012);--0000-00-00
select from_days(20001111250000);--0000-00-00
select from_days(200011112323233242424242456565);--0000-00-00 warning
select from_days(121212.22);--0331-11-14
select from_days(20120000121212.22);--0000-00-00
select from_days('2007-10-07');--0005-06-30 warning
select from_days('2008-02-03');--0005-07-01 warning
select from_days('23:59:59');--0000-00-00 warning
select from_days('00:39:38');--0000-00-00 warning
select from_days(cast('2025-04-22 23:59:59' as time));--0646-01-13
select from_days(cast('23:59:59' as time));--0646-01-13
select from_days(cast(8382822.222 as time));--0000-00-00
select from_days(cast(8382822.5999999 as time));--0000-00-00
select from_days(cast(8382822.5999999 as time(6)));--0000-00-00
select from_days(cast(2011 as year));--0005-07-04
select from_days(cast(201212128182822.222 as datetime));--NULL warning
select from_days(cast(20121212182822.222 as datetime));--0000-00-00
select from_days(cast(20121212182822.5999999 as datetime));--0000-00-00
select from_days(cast('2025-04-21 10:12:12.6999992' as datetime(6)));--0000-00-00
select from_days();--error
select from_days(NULL);--NULL
select from_days(1, 2);--error

--convert_tz
select convert_tz(101,'-02:00','+10:00');--2000-01-01 12:00:00
select convert_tz('2004-01-01 12:00:00','-00:00','+00:00');--2004-01-01 12:00:00
select convert_tz('2004-01-01 12:00:00','00:00','+00:00');--NULL
select convert_tz('2004-01-01 12:00:00','+00:00','+13:00');--2004-01-02 01:00:00
select convert_tz('2004-01-01 12:00:00','+00:00','+14:00');--NULL
select convert_tz('2004-01-01 12:00:00','+00:00','-13:00');--NULL
select convert_tz('2004-01-01 12:00:00','+00:00','-12:59');--2003-12-31 23:01:00
select convert_tz(cast(2011 as year), '+02:00','+08:00');--NULL
select convert_tz(cast(201212128182822.222 as datetime), '+02:00', '+08:00');--NULL warning
select convert_tz(cast(20121212182822.222 as datetime), '+02:00', '+08:00');--2012-12-13 00:28:22
select convert_tz(cast(20121212182822.5999999 as datetime), '+02:00', '+08:00');-- 2012-12-13 00:28:23
select convert_tz(cast('2025-04-21 10:12:12.6999992' as datetime(6)), '+02:00', '+08:00');--2025-04-21 16:12:12.699999
select convert_tz();--error
select convert_tz(NULL);--error
select convert_tz(101);--error
select convert_tz(101,'-02:00');--error
select convert_tz(101, '+02:00', '+08:00', 1);--error  syntax error
select convert_tz(NULL,NULL,NULL);--NULL
/* Begin - BUG#Z6775 */
SELECT CONVERT_TZ('2023-01-01 12:00:00'::date, '+00:00', '+08:00');
select pg_typeof(CONVERT_TZ(CAST('2023-05-20' AS DATE),'+00:00', '+08:00'));
/* End - BUG#Z6775 */

-- get_format
select get_format(DATE, 'EUR');
select get_format(DATE, 'USA'); 
select get_format(DATE, 'JIS');
select get_format(DATE, 'ISO');
select get_format(DATE, 'INTERNAL');
select get_format(DATETIME, 'EUR');
select get_format(DATETIME, 'USA');
select get_format(DATETIME, 'JIS');
select get_format(DATETIME, 'ISO');
select get_format(DATETIME, 'INTERNAL');
select get_format(TIME, 'EUR');
select get_format(TIME, 'USA');
select get_format(TIME, 'JIS');
select get_format(TIME, 'ISO');
select get_format(TIME, 'INTERNAL');
select get_format(TIMESTAMP, 'EUR');
select get_format(TIMESTAMP, 'USA');
select get_format(TIMESTAMP, 'JIS');
select get_format(TIMESTAMP, 'ISO');
select get_format(TIMESTAMP, 'INTERNAL');
-- sepeical
select get_format(DATE, 'abc');
select get_format(DATE, null);
--error
select get_format(abc, 'EUR');

/* Begin - BUG#Z5533 */
SELECT GET_FORMAT(DATE, 'EUR') AS result;
SELECT GET_FORMAT(DATETIME, 'EUR') AS result;
SELECT GET_FORMAT(TIME, 'USA') AS result;
SELECT GET_FORMAT(TIME, 'USA') AS result;
CREATE TABLE test as SELECT GET_FORMAT(TIME, 'USA') AS result;
\d+ test;
DROP TABLE test;

select dayname(101);
CREATE TABLE test as SELECT dayname(101);
\d+ test;
DROP TABLE test;

select monthname(950501);
CREATE TABLE test as SELECT monthname(950501);
\d+ test;
DROP TABLE test;
/* End - BUG#Z5533 */


/* Begin - bug#z5367 */
select timestampadd(month, 1, '2019-1-30');     -- 2019-02-28
select timestampadd(month, 13, '2019-1-30');    -- 2020-02-29
select timestampadd(month, -1, '2019-3-30');    -- 2019-02-28
/* End - bug#z5367 */

/* Begin - BUG#Z5527 */
SET lc_time_names = 'zh_CN';
SELECT DAYNAME('2025-04-17');
SET lc_time_names = 'en_US';
SELECT DAYNAME('2025-04-17');
/* End - BUG#Z5527 */

/* Begin - BUG#Z5532 */
SELECT CONVERT_TZ('2025-02-30 12:00:00', '+00:00', '+08:00');
SELECT CONVERT_TZ('2023-01-01 12:00:00', 'Invalid', '+08:00');
SELECT CONVERT_TZ('2023-01-01 12:00:00', '+00:00', 'Invalid');
/* End - BUG#Z5532 */

/* Begin - BUG#Z5530 */
select yearweek('2023-01-01', NULL);
select yearweek(NULL, NULL);
/* End - BUG#Z5530 */

/* Begin - BUG#Z5528 */
select week('2023-01-01', NULL);
select week(NULL, NULL);
/* End - BUG#Z5528 */

/* Begin - BUG#Z5581 */
SELECT CONVERT_TZ(' ', '+00:00', '+08:00');
SELECT CONVERT_TZ('', '+00:00', '+08:00');
SELECT CONVERT_TZ('2023-01-01 12:00:00', ' ', '+08:00');
SELECT CONVERT_TZ('2023-01-01 12:00:00', '', '+08:00');
SELECT CONVERT_TZ('2023-01-01 12:00:00', ' ', ' ');
SELECT CONVERT_TZ('2023-01-01 12:00:00', '', '');
/* End - BUG#Z5581 */

/* Begin - BUG#Z5582 */
SELECT CONVERT_TZ('1979-01-01 00:00:01', '+00:00','+01:00qaz');
SELECT CONVERT_TZ('1979-01-01 00:00:01', '+00:00','axc+01:00');
SELECT CONVERT_TZ('1979-01-01 00:00:01', '+00:00','abc+01:00qaz');
SELECT CONVERT_TZ('1979-01-01 00:00:01', '+00:00','a01:00');
/* End - BUG#Z5582 */

/* Begin - BUG#Z5583 */
SELECT CONVERT_TZ('1990-01-01 00:00:01', ' +00:00','+10:00');
SELECT CONVERT_TZ('1990-01-01 00:00:01', '+00:00 ','+10:00');
SELECT CONVERT_TZ( '1990-01-01 00:00:01', ' +00:00 ','+10:00' );
/* End - BUG#Z5583 */

/* Begin - BUG#Z5564 */
create table test as select utc_timestamp(6);
\d+ test;
drop table test;
/* End - BUG#Z5564 */

/* Begin - BUG#Z5565 */
set sql_mode = '';
select dayname(" ");
select dayname("");
set sql_mode to default;
/* End - BUG#Z5565 */

/* Begin - BUG#Z5567 */
SELECT time_to_sec('4294967295:59:59');
/* End - BUG#Z5567 */

/* Begin - BUG#Z5566 */
SELECT TIME_TO_SEC(CAST('2:61:10' AS time));
SELECT TIME_TO_SEC(CAST('2:20:60' AS TIME));
/* End - BUG#Z5566 */

/* Begin - BUG#Z5568 */
create table test as SELECT time_to_sec(cast('2025-04-17' as date));
\d+ test;
drop table test;
/* End - BUG#Z5568 */

/* Begin - BUG#Z5569 */
SELECT TIME_TO_SEC(cast('20250517 14:30:00' as datetime));
SELECT TIME_TO_SEC(cast('20250517 14:30:00' as date));
/* End - BUG#Z5569 */

/* Begin - BUG#Z5571 */
select month(0);
SELECT month(20250228251203);
SELECT month(20250228206003);
SELECT month(20250228201260);
SELECT month(999990220);
SELECT month(20251301);
SELECT month(20250230);
select month(-20251201.999);
select month(-20251201.0);
select month(-20251218142030.9999);
select month(-20251218142030.01);

select date(0);
SELECT date(20250228251203);
SELECT date(20250228206003);
SELECT date(20250228201260);
SELECT date(999990220);
SELECT date(20251301);
SELECT date(20250230);
select date(-20251201.999);
select date(-20251201.0);
select date(-20251218142030.9999);
select date(-20251218142030.01);
/* End - BUG#Z5571 */

/* Begin - BUG#Z5572 */
SELECT month('2025-04-31'::date);
SELECT month('20251301'::date);
SELECT month('20251301'::date);
SELECT month('100000805'::date);
/* End - BUG#Z5572 */

/* Begin - BUG#Z5576 */
create table test as select YEARWEEK(20250730::int);
\d+ test;
drop table test;
/* End - BUG#Z5576 */

/* Begin - BUG#Z5578 */
select DATEDIFF('9999-12-31','0001-01-01');
select to_days('0001-01-01'); /* BUG#5607 */
/* End - BUG#Z5578 */

/* Begin - BUG#Z5604 */
set sql_mode=default;
select month(00000000);--0
select month('0000-00-00');--NULL warning
select date(00000000);--0000-00-00
select date('0000-00-00');--NULL warning
select monthname(00000000);--NULL
select monthname('0000-00-00');--NULL warning
select to_days(00000000);--NULL warning
select to_days('0000-00-00');--NULL warning
select dayname(00000000);--NULL warning
select dayname('0000-00-00');--NULL warning
select week(00000000);--NULL warning
select week('0000-00-00');--NULL warning
select yearweek(00000000);--NULL warning
select yearweek('0000-00-00');--NULL warning
/* End - BUG#Z5604 */

/* Begin - BUG#Z5605 */
set sql_mode='';
select month(00000000);--0
select month('0000-00-00');--0
select date(00000000);--0000-00-00
select date('0000-00-00');--0000-00-00
select monthname(00000000);--NULL
select monthname('0000-00-00');--NULL
select to_days(00000000);--NULL warning
select to_days('0000-00-00');--NULL warning
select dayname(00000000);--NULL warning
select dayname('0000-00-00');--NULL warning
select week(00000000);--NULL warning
select week('0000-00-00');--NULL warning
select yearweek(00000000);--NULL warning
select yearweek('0000-00-00');--NULL warning
/* End - BUG#Z5605 */

/* Begin - BUG#Z5610 */
SELECT CONVERT_TZ(2.0230101120000E13, '+00:00', '+08:00');
/* End - BUG#Z5610 */

/* Begin - bug#z5590 */
select sec_to_time('!@#');      -- 00:00:00.000000
select sec_to_time('123!@#');   -- 00:02:03.000000
/* End - bug#z5590 */

/* Begin - bug#z5125 */
select sec_to_time('2023-10-09 12:00:00');
select sec_to_time('2023-10-09 12:00:00 + 3600');
/* End - bug#z5125 */

/* Begin - bug#z5802 */
select convert(NULL, decimal(10,2)),convert('', decimal(10,2));
select convert('abc', decimal(10,2));
select convert(9999999999.9999999999, decimal(66,31));
select convert(9999999999.9999999999, decimal(65,31));
/* End - bug#z5802 */

/* STR_TO_DATE FUNCTION */
SELECT STR_TO_DATE('01,5,2013','%d,%m,%Y');
SELECT STR_TO_DATE('May 1, 2013','%M %d,%Y');
set sql_mode = '' ;
SELECT STR_TO_DATE('a09:30:17','a%h:%i:%s'); --error
set sql_mode = '';
SELECT STR_TO_DATE('09:30:17a','%h:%i:%sa'); --error
SELECT STR_TO_DATE('09:30:17a','%h:%i:%sabc'); --error
SELECT STR_TO_DATE('09:30a:17','%h:%ia:%s'); --error
SELECT STR_TO_DATE('09:a30:17','%h:%ai:%s');
SELECT STR_TO_DATE('09:a30:17','%h:a%i:%s'); --error
set sql_mode = traditional;
SELECT STR_TO_DATE('09:30:17a','%h:%i:%sa');
set sql_mode = '' ;
SELECT STR_TO_DATE('a09:30:17','%h:%i:%s');
set sql_mode = traditional;
SELECT STR_TO_DATE('a09:30:17','%h:%i:%s');
set sql_mode = '' ;
SELECT STR_TO_DATE('09:30:17a','%h:%i:%s');
set sql_mode = traditional;
SELECT STR_TO_DATE('09:30:17a','%h:%i:%s');
set sql_mode = '' ;
SELECT STR_TO_DATE('abc','abc');
set sql_mode = traditional;
SELECT STR_TO_DATE('abc','abc');
set sql_mode = '';
SELECT STR_TO_DATE('9','%m');
set sql_mode = traditional;
SELECT STR_TO_DATE('9','%m');
set sql_mode = '';
SELECT STR_TO_DATE('00/00/0000', '%m/%d/%Y'); --error
set sql_mode = NO_ZERO_DATE;
SELECT STR_TO_DATE('00/00/0000', '%m/%d/%Y');
SELECT STR_TO_DATE('2021-2-31', '%Y-%m-%d');
SELECT STR_TO_DATE('100000-2-31', '%Y-%m-%d');
SELECT STR_TO_DATE('2021-13-31', '%Y-%m-%d');
SELECT STR_TO_DATE('2021-13-32', '%Y-%m-%d');
SELECT STR_TO_DATE('01,5,2013','%d,%m,%Y');

/* mysql code test */
select str_to_date(concat('15-01-2001',' 2:59:58.999'), concat('%d-%m-%Y',' ','%H:%i:%s.%f'));
select STR_TO_DATE('2004.12.12 22.30.61','%Y.%m.%d %T');
SET sql_mode = 'NO_ENGINE_SUBSTITUTION';
create table t1 (date char(30), format char(30) not null);
insert into t1 values
('2003-01-02 10:11:12', '%Y-%m-%d %H:%i:%S'),
('03-01-02 8:11:2.123456', '%y-%m-%d %H:%i:%S.%#'),
('0003-01-02 8:11:2.123456', '%Y-%m-%d %H:%i:%S.%#'),
('03-01-02 8:11:2.123456',   '%Y-%m-%d %H:%i:%S.%#'),
('2003-01-02 10:11:12 PM', '%Y-%m-%d %h:%i:%S %p'),
('2003-01-02 01:11:12.12345AM', '%Y-%m-%d %h:%i:%S.%f%p'),
('2003-01-02 02:11:12.12345AM', '%Y-%m-%d %h:%i:%S.%f %p'),
('2003-01-02 12:11:12.12345 am', '%Y-%m-%d %h:%i:%S.%f%p'),
('2003-01-02 11:11:12Pm', '%Y-%m-%d %h:%i:%S%p'),
('10:20:10', '%H:%i:%s'),
('10:20:10', '%h:%i:%s.%f'),
('10:20:10', '%T'),
('10:20:10AM', '%h:%i:%s%p'),
('10:20:10AM', '%r'),
('10:20:10.44AM', '%h:%i:%s.%f%p'),
('15-01-2001 12:59:58', '%d-%m-%Y %H:%i:%S'),
('15 September 2001', '%d %M %Y'),
('15 SEPTEMB 2001', '%d %M %Y'),
('15 MAY 2001', '%d %b %Y'),
('15th May 2001', '%D %b %Y'),
('Sunday 15 MAY 2001', '%W %d %b %Y'),
('Sund 15 MAY 2001', '%W %d %b %Y'),
('Tuesday 00 2002', '%W %U %Y'),
('Thursday 53 1998', '%W %u %Y'),
('Sunday 01 2001', '%W %v %x'),
('Tuesday 52 2001', '%W %V %X'),
('060 2004', '%j %Y'),
('4 53 1998', '%w %u %Y'),
('15-01-2001', '%d-%m-%Y %H:%i:%S'),
('15-01-20', '%d-%m-%y'),
('15-2001-1', '%d-%Y-%c');
select date,format,str_to_date(date, format) as str_to_date from t1;
drop table t1;
--bug#z6403 z6405 z6414
select STR_TO_DATE('20230515143045', '%Y%m%d%H%i%s');
select STR_TO_DATE('20230515', '%Y%m%d');
SELECT STR_TO_DATE('15-10-2023', '%Y-%m-%d');
SELECT STR_TO_DATE('10000-01-01', '%Y-%m-%d');

-- bug#z6407 #z6402
set sql_mode = '';
SELECT STR_TO_DATE(00000000, '%Y-%m-%d');
set sql_mode = NO_ZERO_DATE;
SELECT STR_TO_DATE(00000000, '%Y-%m-%d');
SELECT STR_TO_DATE(20230515, '%Y%m%d');
SELECT STR_TO_DATE(CAST(20230515.999999 AS numeric), '%Y%m%d');
SELECT STR_TO_DATE(CAST(20230515.999999 AS DECIMAL(20,6)), '%Y%m%d') AS high_precision_decimal;
SELECT STR_TO_DATE('2021-12-30 14:30:00'::date, '%Y-%m-%d %H:%i:%s');
SELECT STR_TO_DATE('2021-12-30 14:30:00'::datetime, '%Y-%m-%d %H:%i:%s');
SELECT STR_TO_DATE('2021-12-30 14:30:00'::time, '%Y-%m-%d %H:%i:%s');
SELECT STR_TO_DATE('2021-12-30 14:30:00'::time, '%H:%i:%s');

/*TIMESTAMPDIFF*/
set sql_mode = default;
select timestampdiff(YEAR, '2002-05-01', '2001-01-01') as a;
select timestampdiff(QUARTER, '2002-05-01', '2001-01-01') as a;
select timestampdiff(MONTH, '2000-03-28', '2000-02-29') as a;
select timestampdiff(MONTH, '1991-03-28', '2000-02-29') as a;
select timestampdiff(SQL_TSI_WEEK, '2001-02-01', '2001-05-01') as a;
select timestampdiff(SQL_TSI_HOUR, '2001-02-01', '2001-05-01') as a;
select timestampdiff(SQL_TSI_DAY, '2001-02-01', '2001-05-01') as a;
select timestampdiff(SQL_TSI_MINUTE, '2001-02-01 12:59:59', '2001-05-01 12:58:59') as a;
select timestampdiff(SQL_TSI_SECOND, '2001-02-01 12:59:59', '2001-05-01 12:58:58') as a;
select timestampdiff(SQL_TSI_DAY, '1986-02-01', '1986-03-01') as a1,
       timestampdiff(SQL_TSI_DAY, '1900-02-01', '1900-03-01') as a2,
       timestampdiff(SQL_TSI_DAY, '1996-02-01', '1996-03-01') as a3,
       timestampdiff(SQL_TSI_DAY, '2000-02-01', '2000-03-01') as a4;
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-11 14:30:27');
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-11 14:30:28');
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-11 14:30:29');
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-12 14:30:27');
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-12 14:30:28');
SELECT TIMESTAMPDIFF(day,'2006-01-10 14:30:28','2006-01-12 14:30:29');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-17 14:30:27');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-17 14:30:28');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-17 14:30:29');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-24 14:30:27');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-24 14:30:28');
SELECT TIMESTAMPDIFF(week,'2006-01-10 14:30:28','2006-01-24 14:30:29');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-02-10 14:30:27');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-02-10 14:30:28');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-02-10 14:30:29');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-03-10 14:30:27');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-03-10 14:30:28');
SELECT TIMESTAMPDIFF(month,'2006-01-10 14:30:28','2006-03-10 14:30:29');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2007-01-10 14:30:27');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2007-01-10 14:30:28');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2007-01-10 14:30:29');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2008-01-10 14:30:27');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2008-01-10 14:30:28');
SELECT TIMESTAMPDIFF(year,'2006-01-10 14:30:28','2008-01-10 14:30:29');
select timestampdiff(month,'2004-09-11','2004-09-11');
select timestampdiff(month,'2004-09-11','2005-09-11');
select timestampdiff(month,'2004-09-11','2006-09-11');
select timestampdiff(month,'2004-09-11','2007-09-11');
select timestampdiff(month,'2005-09-11','2004-09-11');
select timestampdiff(month,'2005-09-11','2003-09-11');
select timestampdiff(month,'2004-02-28','2005-02-28');
select timestampdiff(month,'2004-02-29','2005-02-28');
select timestampdiff(month,'2004-02-28','2005-02-28');
select timestampdiff(month,'2004-03-29','2005-03-28');
select timestampdiff(month,'2003-02-28','2004-02-29');
select timestampdiff(month,'2003-02-28','2005-02-28');
select timestampdiff(month,'1999-09-11','2001-10-10');
select timestampdiff(month,'1999-09-11','2001-9-11');
select timestampdiff(year,'1999-09-11','2001-9-11');
select timestampdiff(year,'2004-02-28','2005-02-28');
select timestampdiff(year,'2004-02-29','2005-02-28');
--bug#z6783
select timestampdiff(sql_tsi_month,'1999-09-11','2001-10-10');
select timestampdiff(sql_tsi_year,'1999-09-11','2001-9-11');

--bug#z6049
set sql_mode ='';
SELECT TIMESTAMPDIFF(MINUTE,'00:00:01','2001-01-01 00:05:59');
set sql_mode = default;
SELECT TIMESTAMPDIFF(MINUTE,'00:00:01','2001-01-01 00:05:59');

--bug#z6412
SELECT TIMESTAMPDIFF(MICROSECOND, '2024-01-01 23:59:59.000123', '2024-01-01 23:59:59.123456');
SELECT TIMESTAMPDIFF(MICROSECOND, '2024-01-01 23:59:59', '2024-01-01 23:59:59.12345678');

--bug#6651
SELECT UNIX_TIMESTAMP('2023-01-01 12:30:45');
SELECT UNIX_TIMESTAMP('2023-01-01');
SELECT UNIX_TIMESTAMP('1970-01-01 00:00:00');
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07');
SELECT UNIX_TIMESTAMP(20380119031407);
SELECT UNIX_TIMESTAMP(20380119);
SELECT UNIX_TIMESTAMP('20250119031407'::DECIMAL(20,6));
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::datetime);
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::date);
SELECT UNIX_TIMESTAMP(20380119031407::time);
SELECT UNIX_TIMESTAMP('2038-00-19 03:14:07');
SELECT UNIX_TIMESTAMP('0000-1-0 00:00:00');
set time_zone='+00:00';
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07');
SELECT UNIX_TIMESTAMP('2023-01-01 12:30:45');
SELECT UNIX_TIMESTAMP('2023-01-01');
SELECT UNIX_TIMESTAMP('1970-01-01 00:00:00');
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07');
SELECT UNIX_TIMESTAMP(20380119031407);
SELECT UNIX_TIMESTAMP(20380119);
SELECT UNIX_TIMESTAMP('20250119031407'::DECIMAL(20,6));
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::datetime);
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::date);
SELECT UNIX_TIMESTAMP('2038-00-19 03:14:07');
SELECT UNIX_TIMESTAMP('0000-1-0 00:00:00');
set time_zone='+08:00';
SELECT UNIX_TIMESTAMP('2023-01-01 12:30:45');
SELECT UNIX_TIMESTAMP('2023-01-01');
SELECT UNIX_TIMESTAMP('1970-01-01 00:00:00');
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07');
SELECT UNIX_TIMESTAMP(20380119031407);
SELECT UNIX_TIMESTAMP(20380119);
SELECT UNIX_TIMESTAMP('20250119031407'::DECIMAL(20,6));
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::datetime);
SELECT UNIX_TIMESTAMP('2038-01-19 03:14:07'::date);
SELECT UNIX_TIMESTAMP('2038-00-19 03:14:07');
SELECT UNIX_TIMESTAMP('0000-1-0 00:00:00');