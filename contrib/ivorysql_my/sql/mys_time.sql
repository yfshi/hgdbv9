set ivorysql.compatible_mode to mysql;
set  datestyle to 'ISO,YMD';

--MySQL time type test case start
select cast('1.1' as time);-- 00:00:01
select cast('11.1' as time);--00:00:11
select cast('1.11' as time);--00:00:01
select cast('11.11' as time);--00:00:11
select cast('11.111' as time);--00:00:11
select cast('99.111' as time);--NULL warning
select cast('99.311' as time);--NULL warning
select cast('1.1111' as time);--00:00:01
select cast('1.111122222' as time);--00:00:01
select cast('998.3' as time);--NULL warning
select cast('991.31' as time);--NULL warning
select cast('999.31' as time);--NULL warning
select cast('111.111' as time);-- 00:01:11
select cast('116.119' as time);-- 00:01:16
select cast('116.419' as time);-- 00:01:16
select cast('998.2222' as time);--NULL warning
select cast('998.2224' as time);--NULL warning
select cast('998.222333' as time);--NULL warning
select cast('998.222360' as time);--NULL warning
select cast('998.22235959' as time);--NULL warning
select cast('998.222359599999' as time);--NULL warning
select cast('998.2223595999999999999999' as time);--NULL warning
select cast('1118.1' as time);--00:11:18
select cast('1118.12' as time);--00:11:18
select cast('1118.122' as time);--00:11:18
select cast('1118.122212' as time);--00:11:18
select cast('1118.123212' as time);--00:11:18
select cast('1118.123124' as time);--00:11:18
select cast('1118.12312323' as time);--00:11:18
select cast('1118.12312366' as time);--00:11:18
select cast('1118.1222235659' as time);--00:11:18
select cast('1118.122223565999999999999999' as time);--00:11:18

select cast('11118.1' as time);--01:11:18
select cast('11118.12' as time);--01:11:18
select cast('11118.32' as time);--01:11:18
select cast('11118.111111111' as time);--01:11:18
select cast('11118.116011111' as time);--01:11:18
select cast('11118.110660111' as time);--01:11:18
select cast('11118.1106019999999999999999' as time);--01:11:18

select cast('111121.1' as time);--11:11:21
select cast('111121.12' as time);--11:11:21
select cast('111121.24' as time);--11:11:21
select cast('111121.231' as time);--11:11:21
select cast('111121.2312' as time);--11:11:21
select cast('111121.231260' as time);--11:11:21
select cast('111521.231259' as time);--11:15:21
select cast('111121.231259' as time);--11:11:21
select cast('111121.2312599999999999999' as time);--11:11:21

select cast('1111213.5' as time);--111:12:14
select cast('1111213.11' as time);--111:12:13
select cast('1111213.112' as time);--111:12:13
select cast('1111213.1122' as time);--111:12:13
select cast('1111213.11222222222' as time);--111:12:13
select cast('1111213.99' as time);--111:12:14
select cast('1111213.59' as time);--111:12:14
select cast('1111213.60' as time);-- 111:12:14
select cast('1111213.5960' as time);-- 111:12:14
select cast('1111213.5959' as time);--111:12:14 
select cast('1111213.595988888888888' as time);--111:12:14

select cast('11111121.9' as time);--838:59:59 warning
select cast('11111121.23' as time);--838:59:59 warning
select cast('11111121.24' as time);--838:59:59 warning
select cast('11111121.235555' as time);--838:59:59 warning
select cast('11111121.245555' as time);--838:59:59 warning
select cast('11111121.236055' as time);--838:59:59 warning
select cast('11111121.235560' as time);--838:59:59 warning
select cast('11111121.235555999999999' as time);--838:59:59 warning

select cast('111111211.2' as time);--838:59:59 warning
select cast('111111241.2' as time);--838:59:59 warning
select cast('111111231.2' as time);--838:59:59 warning
select cast('111111239.2' as time);--838:59:59 warning
select cast('111111239.59' as time);--838:59:59 warning
select cast('111111239.60' as time);--838:59:59 warning
select cast('111111239.5999999999' as time);--838:59:59 warning
select cast('1111112111.2' as time);--838:59:59 warning
select cast('1111112411.2' as time);--838:59:59 warning
select cast('1111112159.2' as time);--838:59:59 warning
select cast('1111112160.2' as time);--NULL warning
select cast('1111112159.59' as time);--838:59:59 warning
select cast('1111112159.5999999999999' as time);--838:59:59 warning

select cast('11111121591.2' as time);--21:59:01 
select cast('11111121599.23' as time);--21:59:09
select cast('11111121599.23444' as time);-- 21:59:09
select cast('11111121599.99999999999999999' as time);--21:59:10
select cast('11111124599.23' as time);--NULL warning
select cast('11111121609.23444' as time);--NULL warning
select cast('111111215059.2' as time);--21:50:59
select cast('111111215060.2' as time);--NULL warning
select cast('111111215019.23444' as time);--21:50:19
select cast('111111215059.23455556' as time);--21:50:59
select cast('111111215059.99999999' as time);--21:51:00 

select cast('1111112150591.9' as time);--21:50:59 warning
select cast('1111112150601.9' as time);--NULL warning
select cast('1111112150591.99' as time);--21:50:59 warning
select cast('1111112150591.9999999999999' as time);--21:50:59 warning

select cast('11111111201911.9' as time);--20:19:12
select cast('11111111241911.9' as time);--NULL warning
select cast('11111111236011.9' as time);--NULL warning
select cast('11111111201911.9999999999' as time);--20:19:12

select cast('111111112019111.9' as time);--20:19:11
select cast('111111111260111.9' as time);--NULL warning
select cast('11111111201911111111.9' as time);--20:19:11 warning
select cast('1111111112360111.9' as time);--12:36:01 warning
select cast('111111112019113333.9999999999' as time);--20:19:11 warning

select cast('1.1.1' as time);--00:00:01 warning
select cast('1.1.11' as time);--00:00:01 warning
select cast('11.11.11' as time);--00:00:11 warning
select cast('11.13.11' as time);--00:00:11 warning
select cast('11.13.32' as time);--00:00:11 warning
select cast('99.11.11' as time);--NULL warning

select cast('99.111.11' as time);--NULL warning
select cast('99.111.1111' as time);--NULL warning
select cast('99.111.3111' as time);--NULL warning
select cast('99.111.116111' as time);--NULL warning
select cast('99.111.115961' as time);--NULL warning
select cast('99.111.115911' as time);--NULL warning
select cast('99.1111.115911' as time);--NULL warning
select cast('99.1133.115911' as time);--NULL warning
select cast('99.1130.115911' as time);--NULL warning
select cast('99.1311.11' as time);--NULL warning

select cast('10000.1.1' as time);--01:00:00 warning
select cast('10001.1.1' as time);--01:00:01 warning
select cast('11111.1.1' as time);--01:11:11 warning
select cast('11111.11.11' as time);-- 01:11:11 warning
select cast('11111.24.11' as time);--01:11:11 warning
select cast('199999.23.59' as time);--NULL warning
select cast('11111.23.11' as time);--01:11:11 warning
select cast('11111.23.59' as time);--01:11:11  warning
select cast('11111.23232.99999' as time);-- 01:11:11 warning
select cast('11111.232323.99999' as time);--01:11:11 warning
select cast('11111.23232233.99999' as time);--01:11:11 warning

select cast('111111.23.9' as time);--11:11:11 warning
select cast('111111.24.9' as time);--11:11:11 warning
select cast('111111.221.9' as time);--11:11:11 warning
select cast('111111.22111111.9' as time);--11:11:11 warning
select cast('111111.2260111.9999' as time);--11:11:11 warning
select cast('111111.22111111.9999999999' as time);--11:11:11 warning

select cast('1111111.3231.9' as time);--111:11:11 warning
select cast('1111111.6031.9' as time);--111:11:12 warning
select cast('1111111.5931.9' as time);--111:11:12 warning
select cast('1111221.59.29' as time);--111:12:22 warning
select cast('1111221.59599999999.29' as time);--111:12:22 warning

select cast('11111211.1.29' as time);--838:59:59 warning
select cast('11111211.1112.29' as time);--838:59:59 warning
select cast('11111211.1160.29' as time);--838:59:59 warning
select cast('11111211.1120.69' as time);--838:59:59 warning
select cast('11111211.1159599999999.29' as time);--838:59:59 warning
select cast('11111211.111616.99999' as time);--838:59:59 warning

select cast('111112111.1.29' as time);--838:59:59 warning
select cast('111112241.1.29' as time);--838:59:59 warning
select cast('111112231.1.29' as time);--838:59:59 warning
select cast('111112239.1.29' as time);--838:59:59 warning
select cast('111112239.60.29' as time);--838:59:59 warning
select cast('111112239.6.9999' as time);--838:59:59 warning
select cast('1111122329.6.9' as time);--838:59:59 warning
select cast('1111122329.6.9999' as time);--838:59:59 warning
select cast('1111122329.60.9' as time);--838:59:59 warning
select cast('1111122360.16.9' as time);--NULL warning
select cast('1111122416.16.9' as time);--838:59:59 warning
select cast('1111122216.599999.999999999999' as time);--838:59:59 warning

select cast('11111222161.1.1' as time);--NULL warning
select cast('11111222161.9999999999.1' as time);--22:16:02 warning
select cast('11111222161.9999999999.9999999999' as time);--22:16:02 warning
select cast('111112226011.9.9' as time);--NULL warning
select cast('111112226011.9999999999.9999999999' as time);--NULL warning
select cast('111112225911.9.9' as time);--NULL warning
select cast('111112222611.9999999999.9999999999' as time);--22:26:12 warning
select cast('1111122216119.9999.99999' as time);-- 22:16:11 warning
select cast('1111122216111.9999.99999' as time);-- 22:16:11 warning

select cast('11111222161121.9999.99999' as time);--NULL warning
select cast('111112221611211.9999.99999' as time);--16:11:21 warning
select cast('1111122216112111111.9999.99999' as time);--16:11:21 warning

select cast('1.1.1.1' as time);--00:00:01 warning
select cast('11.11.11.11' as time);--00:00:11 warning
select cast('1.1.1.11.11' as time);--00:00:01 warning
select cast('111.11.11.11' as time);--00:01:11  warning
select cast('1111.11.11.11' as time);--00:11:11 warning
select cast('1111.60.4.11' as time);--00:11:12 warning
select cast('111111.11.11.11' as time);--11:11:11 warning
select cast('1111111.111.11.11' as time);--111:11:11  warning
select cast('11111111.11.4.11' as time);--838:59:59 warning
select cast('111111111.11.4.11' as time);--838:59:59 warning
select cast('1111111111.11.4.11' as time);--838:59:59 warning
select cast('11111111111.11.4.11' as time);-- NULL warning
select cast('111111111111.11.4.11' as time);--NULL warning
select cast('1111111111111.11.4.11' as time);--11:11:11 warning
select cast('11111111111111.11.4.11' as time);--NULL warning
select cast('11111111111111111.11.4.11' as time);--11:11:11 warning
select cast('11.11.11.23.34.50' as time);--00:00:11 warning 
select cast('9999.12.31.23.59.59.999999.99' as time);--NULL warning
select cast('99.12.31.23.59.59.999999.99' as time);--NULL warning


select cast('111/11/11/23/34/50' as time);--00:01:11 warning
select cast('10000/11/11/23/34/50' as time);--01:00:00 warning
select cast('1000/13/11/23/34/50' as time);--00:10:00 warning
select cast('9999/11/60/23/34/50' as time);--NULL warning
select cast('9999/11/59/24/34/50' as time);--NULL warning
select cast('9999/11/59/23/60/50' as time);--NULL warning
select cast('9999/11/59/23/59/60' as time);--NULL warning
select cast('9999/11/19/23/59/59/99999999999' as time);--NULL warning

select cast('111.11.11.23.34.50' as time);--00:01:11 warning
select cast('1111/11.11.23/34/50' as time);--00:11:11 warning
select cast('111/11.11.23/34/50' as time);--00:01:11 warning
select cast('111/11.11/23/34/50' as time);--00:01:11 warning
select cast('111/11.11.24/34/50' as time);--00:01:11 warning
select cast('111/11.11.23/60/50' as time);--00:01:11 warning
select cast('111/11.11.23/34/60' as time);--00:01:11 warning
select cast('111/11.11.23/34/59/9999999' as time);--00:01:11 warning
select cast('10000/11/11/23/34/50' as time);--01:00:00 warning

select cast(' ' as time);--NULL
select cast('' as time);--NULL

select cast('2023-03-03 20:01:20AM' as time);--20:01:20 warning
select cast('20230303200120PM' as time);--NULL warning
select cast('20230303200120AM' as time);--NULL  warning
select cast('20230303T200120' as time);--20:01:20
select cast('2003-03-03T20:01:20' as time);--00:20:03 warning

select cast('111121.444444' as time(3));--11:11:21.444
select cast('111121.555555' as time(3));--11:11:21.556
select cast('111121.444444444' as time(3));--11:11:21.444
select cast('111121.555555555' as time(3));--11:11:21.556
select cast('111121.555555555' as time(6));--11:11:21.555556

--explict conversion 
select time'20100231111';--error
select time'2010-02-31';--error
select time'2010-02-31 10:22:33';--error
select time'20100231';--error
select time'0000-00-00';--error
select time'11';--00:00:11
select time'100';--00:01:00
select time'101';-- 00:01:01
select time'111';--00:01:11
select time'1111';--00:11:11
select time'9999';--error
select time'11111';--01:11:11
select time'111111';--11:11:11
select time'1111111';--111:11:11
select time'11111111';--error
select time'11111111111';--error
select time'111111111111111';--error
select time'11.1';--00:00:11.1
select time'11.11';--00:00:11.11
select time'111.11';--00:01:11.11
select time'111.111';--00:01:11.111
select time'111.11111';--00:01:11.11111
select time'111.11111111';--00:01:11.111111
select time'111.555555555';-- 00:01:11.555556
select time'1111.1';--00:11:11.1
select time'1111.11';--00:11:11.11
select time'1111.111';--00:11:11.111
select time'1111.1111';--00:11:11.1111
select time'1111.11111';--00:11:11.11111
select time'1111.111111';--00:11:11.111111
select time'1111.1111111';--00:11:11.111111
select time'11111.1';--01:11:11.1
select time'11111.11';--01:11:11.11
select time'11111.111';--01:11:11.111
select time'11111.1111';--01:11:11.1111
select time'11111.11111';--01:11:11.11111
select time'111111.111111';--11:11:11.111111
select time'1111111.111111';--111:11:11.111111 
select time'11111111.111111';--error
select time'111111111.111111';--error
select time'11111111111.111111';--error
select time'11111111111111.111111';--error
select time'1.1.1';--error
select time'11.11.11';--error
select time'11.11.1111';--error
select time'11.11.11111';--error
select time'11.11.1111111';--error
select time'11.111.1';--error
select time'11.111.11';--error
select time'11.111.111111';--error
select time'11.111.111111111';--error
select time'11.1111.11';--error
select time'11.1111.111111';--error
select time'111.1.1';--error
select time'111.11.1';--error
select time'111.11.11';--error
select time'111.11111.11';--error
select time'111.11.11111';--error
select time'1111.11.1';--error
select time'1111.11.11';--error
select time'1111.111.11';--error
select time'1111.11.11.11';--error
select time'1111.11.11.11.11';--error
select time'1111.11.11.11.11.11';--error
select time'1111.11.11.11.11.1111111';--error

select cast(1 as time);--00:00:01
select cast(11 as time);--00:00:11
select cast(111 as time);--00:01:11
select cast(1111 as time);--00:11:11
select cast(1131 as time);--00:11:31
select cast(11111 as time);--01:11:11
select cast(11111.77 as time);--01:11:12
select cast(111111 as time);--11:11:11
select cast(111311 as time);--11:13:11
select cast(111111.88 as time);--11:11:12
select cast(113111 as time);--11:31:11
select cast(112160 as time);--NULL warning
select cast(1111111 as time);--111:11:11
select cast(1111111.34 as time);--111:11:11
select cast(1111161 as time);--NULL warning
select cast(11111111 as time);--NULL warning
select cast(11111111.56 as time);--NULL warning
select cast(11113111 as time);--NULL warning
select cast(11112160 as time);--NULL warning
select cast(111111111 as time);--NULL warning
select cast(111111111.99 as time);--NULL warning
select cast(111351111 as time);--NULL warning
select cast(1111111111 as time);--NULL warning
select cast(1111111171 as time);--NULL warning
select cast(11111116059 as time);--NULL warning
select cast(11111111111 as time);--11:11:11
select cast(111111111111 as time);--11:11:11
select cast(11111111111.33 as time);--11:11:11
select cast(111111111111.45 as time);--11:11:11
select cast(111111241156 as time);--NULL warning
select cast(1111111111111 as time);--11:11:11
select cast(1111111311105 as time);--NULL warning
select cast(11111111111111 as time);--11:11:11
select cast(11111111111111.23 as time);--11:11:11
select cast(11111111119905 as time);--NULL warning
select cast(11111111111165 as time);--NULL warning
select cast(11111111251105 as time);--NULL warning
select cast(111111111111111 as time);--NULL warning
select cast(1111111111111111 as time);--NULL warning
select cast(1111111111111111.44 as time);--NULL warning
select cast(11111111111233444 as time);--NULL warning
select cast(111111111111111111111 as time);--NULL warning
select cast(19830905 as time);--NULL warning
select cast(119830905 as time);--NULL warning
select cast(198309051 as time);--NULL warning
select cast(198205051 as time);--NULL warning
select cast(128205051 as time);--NULL warning
select cast(6995 as time);--NULL warning
select cast(2007-05-23 as time);--NULL warning
select cast(2007-05*23 as time);--NULL warning
select cast(830905 as time);--83:09:05
select cast(9830905 as time);--NULL warning
select cast(30905 as time);--03:09:05
select cast(230905 as time);--23:09:05
select cast(690905 as time);--69:09:05
select cast(700905 as time);--70:09:05
select cast(0905 as time);--00:09:05
select cast(905 as time);--00:09:05
select cast(00000905 as time);--00:09:05
select cast(0000000905 as time);--00:09:05
select cast(2007*05*23 as time);--23:08:05
select cast(05*23 as time);--00:01:15
select cast(06/03 as time);--00:00:02
select cast(300/03 as time);--00:01:00
select cast(30000000/3 as time); --NULL warning

select cast(11111111111111.444444 as time);--11:11:11
select cast(11111111111111.444444444 as time);--11:11:11
select cast(11111111111111.555555 as time);--11:11:12
select cast(11111111111111.555555555 as time);--11:11:12
select cast(11111111111111.555555555 as time);--11:11:12

select cast(11111111111111.444444 as time(3));--11:11:11.444
select cast(11111111111111.444444444 as time(3));--11:11:11.444
select cast(11111111111111.555555 as time(3));--11:11:11.556
select cast(11111111111111.555555555 as time(3));--11:11:11.556
select cast(11111111111111.555555555 as time(6));--11:11:11.555556

select cast(8385959.444444 as time);--838:59:59
select cast(8385959.555555 as time);--838:59:59
select cast(8385959.555555555 as time);--838:59:59 warning

select cast(8385959.444444 as time(3));--838:59:59.000
select cast(8385959.555555 as time(3));--838:59:59.000
select cast(8385959.555555555 as time(3));--838:59:59 warning

select cast('8385959.444444' as time);--838:59:59 warning
select cast('8385959.555555' as time);--838:59:59 warning
select cast('8385959.555555555' as time);--838:59:59 warning

select cast('8385959.444444' as time(3));--838:59:59.000 warning
select cast('8385959.555555' as time(3));--838:59:59.000 warning
select cast('8385959.555555555' as time(3));--838:59:59 warning

--round
select cast('2010.01.31.23.59.59.99999999' as time);--00:20:10 warning
select cast('2010.01.31.23.59.59.999999' as time);--00:20:10 warning
select cast('2010.00.31.23.59.59.99999999' as time);--00:20:10 warning
select cast('23:59:59.99999999' as time);--24:00:00
select cast('23:59:59.999999' as time);--24:00:00
select cast('23:59:59.99999999' as time);--24:00:00
select cast(1111111.123454789 as time(5));--111:11:11.12346

--set sql_mode
set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt time);
insert into test values('2100-00-04 12:13:14');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00
select * from test;
delete from test;

set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
insert into test values('2100-00-04 12:13:14');--warning
insert into test values('0000-00-00 12:13:14');
insert into test values('1111-01-01 25:00:00');--ERROR
select cast('2010-02-31 00:00:00' as time);--NULL
select cast(20100231121314 as time);--NULL
select cast('9999-12-31 23:59:59.44' as time);--23:59:59
select cast('9999-12-31 23:59:59.99' as time);--24:00:00
select cast(99991231235959 as time);--23:59:59
select cast(99991231235959.4 as time);--23:59:59
select cast(99991231235959.5 as time);--24:00:00


insert into test values('');
insert into test values('99999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-02-31 12:13:14');--ERROR
insert into test values('9999-00-31 12:13:14');--warning
insert into test values('0000-00-00 12:13:14');

--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select *  from test;

set sql_mode ='ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

----20231013
set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE,ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

drop table test;
set sql_mode to default;
--
create table test1(dt1 time, dt2 time(3), dt3 time(6));
insert into test1 values('12:13:14.444444', '12:13:14.444444', '12:13:14.444444');
insert into test1 values('12:13:14.444444444', '12:13:14.444444444', '12:13:14.444444444');
insert into test1 values('12:13:14.555555', '12:13:14.555555', '12:13:14.555555');
insert into test1 values('12:13:14.555555555', '12:13:14.555555555', '12:13:14.555555555');
insert into test1 values(222.444444, 222.444444, 222.444444);
insert into test1 values(2221.444444444, 2221.444444444, 2221.444444444);
insert into test1 values(22211.555555, 22211.555555, 22211.555555);
insert into test1 values(22211.555555555, 22211.555555555, 22211.555555555);

select * from test1;
drop table test1;

create table test2(dt1 time(5));
insert into test2 values('12:13:14.123454789');
insert into test2 values(1111111.123454789);

select * from test2;
drop table test2;
-- oper
---=
select cast('22:01:21' as time) = cast('22:01:21' as time);--1
select 212 = cast('22:01:60' as time);--NULL warning
select 212 = cast('22:01:21' as time);--0
select 220121.01 = cast('22:01:21' as time);--0
select NULL = cast('22:01:21' as time);--NULL
select 220121.5 = cast('22:01:21' as time);--0
select 220121.4 = cast('22:01:21' as time);--0
select 220121.00 = cast('22:01:21' as time);--1
select 220121 = cast('22:01:21' as time);--1
select cast(102233.4 as time) = cast('10:22:33' as time);--1
select cast(102233.5 as time) = cast('10:22:33.5' as time);--1
select cast(102233.5 as time) = cast('10:22:33' as time);--0
select 102233 = cast('10:22:33' as time);--1
select 102233.4 = cast('10:22:33' as time);--0
select 102233.5 = cast('10:22:33.5' as time);--0
select 102233.5 = cast('10:22:33' as time);--0
select '10:22:33' = cast('10:22:33' as time);--1
select '10:22:33.4' = cast('10:22:33' as time);--0 -- Need to be modified
select '10:22:33.5' = cast('10:22:33' as time);--0
select '10:22:33.5' = cast('10:22:33.5' as time);--0 -- Need to be modified
select '10:22:33.5' = cast('10:22:33.5' as time(1));--1
select '10:22:33.5' = cast('10:22:33.5' as time(2));--0 -- Need to be modified

--<>
select cast('22:01:21' as time) <> cast('22:01:21' as time);--0
select 212 <> cast('22:01:60' as time);--NULL warning
select 212 <> cast('22:01:21' as time);--1
select 220121.01 <> cast('22:01:21' as time);--1
select NULL <> cast('22:01:21' as time);--NULL
select 220121.5 <> cast('22:01:21' as time);--1
select 220121.4 <> cast('22:01:21' as time);--1
select 220121.00 <> cast('22:01:21' as time);--0
select 220121 <> cast('22:01:21' as time);--0
select cast(102233.4 as time) <> cast('10:22:33' as time);--0
select cast(102233.5 as time) <> cast('10:22:33.5' as time);--0
select cast(102233.5 as time) <> cast('10:22:33' as time);--1
select 102233 <> cast('10:22:33' as time);--0
select 102233.4 <> cast('10:22:33' as time);--1
select 102233.5 <> cast('10:22:33.5' as time);--1
select 102233.5 <> cast('10:22:33' as time);--1
select '10:22:33' <> cast('10:22:33' as time);--0
select '10:22:33.4' <> cast('10:22:33' as time);--1 -- Need to be modified
select '10:22:33.5' <> cast('10:22:33' as time);--1
select '10:22:33.5' <> cast('10:22:33.5' as time);--1 -- Need to be modified
select '10:22:33.5' <> cast('10:22:33.5' as time(1));--0
select '10:22:33.5' <> cast('10:22:33.5' as time(2));--1 -- Need to be modified

--<
select cast('22:01:21' as time) < cast('22:01:21' as time);--0
select 212 < cast('22:01:60' as time);--NULL warning
select 212 < cast('22:01:21' as time);--1
select 220121.01 < cast('22:01:21' as time);--0
select NULL < cast('22:01:21' as time);--NULL
select 220121.5 < cast('22:01:21' as time);--0
select 220121.4 < cast('22:01:21' as time);--0
select 220121.00 < cast('22:01:21' as time);--0
select 220121 < cast('22:01:21' as time);--0
select cast(102233.4 as time) < cast('10:22:33' as time);--0
select cast(102233.5 as time) < cast('10:22:33.5' as time);--0
select cast(102233.5 as time) < cast('10:22:33' as time);--0
select 102233 < cast('10:22:33' as time);--0
select 102233.4 < cast('10:22:33' as time);--0
select 102233.5 < cast('10:22:33.5' as time);--1
select 102233.5 < cast('10:22:33' as time);--0
select '10:22:33' < cast('10:22:33' as time);--0
select '10:22:33.4' < cast('10:22:33' as time);--0
select '10:22:33.5' < cast('10:22:33' as time);--0
select '10:22:33.5' < cast('10:22:33.5' as time);--1 -- Need to be modified
select '10:22:33.5' < cast('10:22:33.5' as time(1));--0
select '10:22:33.5' < cast('10:22:33.5' as time(2));--1 -- Need to be modified

-->
select cast('22:01:21' as time) > cast('22:01:21' as time);--0
select 212 > cast('22:01:60' as time);--NULL warning
select 212 > cast('22:01:21' as time);--0
select 220121.01 > cast('22:01:21' as time);--1
select NULL > cast('22:01:21' as time);--NULL
select 220121.5 > cast('22:01:21' as time);--1
select 220121.4 > cast('22:01:21' as time);--1
select 220121.00 > cast('22:01:21' as time);--0
select 220121 > cast('22:01:21' as time);--0
select cast(102233.4 as time) > cast('10:22:33' as time);--0
select cast(102233.5 as time) > cast('10:22:33.5' as time);--0
select cast(102233.5 as time) > cast('10:22:33' as time);--1
select 102233 > cast('10:22:33' as time);--0
select 102233.4 > cast('10:22:33' as time);--1
select 102233.5 > cast('10:22:33.5' as time);--0
select 102233.5 > cast('10:22:33' as time);--1
select '10:22:33' > cast('10:22:33' as time);--0
select '10:22:33.4' > cast('10:22:33' as time);--1 -- Need to be modified
select '10:22:33.5' > cast('10:22:33' as time);--1
select '10:22:33.5' > cast('10:22:33.5' as time);--0
select '10:22:33.5' > cast('10:22:33.5' as time(1));--0
select '10:22:33.5' > cast('10:22:33.5' as time(2));--0

--<=
select cast('22:01:21' as time) <= cast('22:01:21' as time);--1
select 212 <= cast('22:01:60' as time);--NULL warning
select 212 <= cast('22:01:21' as time);--1
select 220121.01 <= cast('22:01:21' as time);--0
select NULL <= cast('22:01:21' as time);--NULL
select 220121.5 <= cast('22:01:21' as time);--0
select 220121.4 <= cast('22:01:21' as time);--0
select 220121.00 <= cast('22:01:21' as time);--1
select 220121 <= cast('22:01:21' as time);--1
select cast(102233.4 as time) <= cast('10:22:33' as time);--1
select cast(102233.5 as time) <= cast('10:22:33.5' as time);--1
select cast(102233.5 as time) <= cast('10:22:33' as time);--0
select 102233 <= cast('10:22:33' as time);--1
select 102233.4 <= cast('10:22:33' as time);--0
select 102233.5 <= cast('10:22:33.5' as time);--1
select 102233.5 <= cast('10:22:33' as time);--0
select '10:22:33' <= cast('10:22:33' as time);--1
select '10:22:33.4' <= cast('10:22:33' as time);--0 -- Need to be modified
select '10:22:33.5' <= cast('10:22:33' as time);--0
select '10:22:33.5' <= cast('10:22:33.5' as time);--1
select '10:22:33.5' <= cast('10:22:33.5' as time(1));--1
select '10:22:33.5' <= cast('10:22:33.5' as time(2));--1

-->=
select cast('22:01:21' as time) >= cast('22:01:21' as time);--1
select 212 >= cast('22:01:60' as time);--NULL warning
select 212 >= cast('22:01:21' as time);--0
select 220121.01 >= cast('22:01:21' as time);--1
select NULL >= cast('22:01:21' as time);--NULL
select 220121.5 >= cast('22:01:21' as time);--1
select 220121.4 >= cast('22:01:21' as time);--1
select 220121.00 >= cast('22:01:21' as time);--1
select 220121 >= cast('22:01:21' as time);--1
select cast(102233.4 as time) >= cast('10:22:33' as time);--1
select cast(102233.5 as time) >= cast('10:22:33.5' as time);--1
select cast(102233.5 as time) >= cast('10:22:33' as time);--1
select 102233 >= cast('10:22:33' as time);--1
select 102233.4 >= cast('10:22:33' as time);--1
select 102233.5 >= cast('10:22:33.5' as time);--0
select 102233.5 >= cast('10:22:33' as time);--1
select '10:22:33' >= cast('10:22:33' as time);--1
select '10:22:33.4' >= cast('10:22:33' as time);--1
select '10:22:33.5' >= cast('10:22:33' as time);--1
select '10:22:33.5' >= cast('10:22:33.5' as time);--0  -- Need to be modified
select '10:22:33.5' >= cast('10:22:33.5' as time(1));--1
select '10:22:33.5' >= cast('10:22:33.5' as time(2));--0 -- Need to be modified

--<=>
select cast('22:01:21' as time) <=> cast('22:01:21' as time);--1
select 212 <=> cast('22:01:60' as time);--NULL warning
select 212 <=> cast('22:01:21' as time);--0
select 220121.01 <=> cast('22:01:21' as time);--0
select NULL <=> cast('22:01:21' as time);--0
select 220121.5 <=> cast('22:01:21' as time);--0
select 220121.4 <=> cast('22:01:21' as time);--0
select 220121.00 <=> cast('22:01:21' as time);--1
select 220121 <=> cast('22:01:21' as time);--1
select cast(102233.4 as time) <=> cast('10:22:33' as time);--1
select cast(102233.5 as time) <=> cast('10:22:33.5' as time);--1
select cast(102233.5 as time) <=> cast('10:22:33' as time);--0
select 102233 <=> cast('10:22:33' as time);--1
select 102233.4 <=> cast('10:22:33' as time);--0
select 102233.5 <=> cast('10:22:33.5' as time);--0
select 102233.5 <=> cast('10:22:33' as time);--0
select '10:22:33' <=> cast('10:22:33' as time);--1
select '10:22:33.4' <=> cast('10:22:33' as time);--0  -- Need to be modified
select '10:22:33.5' <=> cast('10:22:33' as time);--0
select '10:22:33.5' <=> cast('10:22:33.5' as time);--0  -- Need to be modified
select '10:22:33.5' <=> cast('10:22:33.5' as time(1));--1
select '10:22:33.5' <=> cast('10:22:33.5' as time(2));--0

--+
-- Need to be modified
select cast('22:01:21' as time) +  cast('22:01:88' as time);--NULL warning
select cast('22:01:21' as time) +  NULL ;--NULL
select cast('22:01:21' as time) +  cast('22:01:88' as time);--NULL warning
select cast('22:01:01' as time) +  cast('22:01:88' as time);--NULL warning
select cast('22:01:01' as time) +  cast('2022-02-20' as time);--222123 warning
select cast(0 as time)  + 44;--44
select cast(0 as time)  + 44.88;--44.88
select 220121.5 + cast('22:01:21' as time);--440242.5
select 220121.4 + cast('22:01:21' as time);--440242.4
select 220121.00 + cast('22:01:21' as time);--440242.00
select 220121 + cast('22:01:21' as time);--440242
select 220121.5 + cast('22:01:21.5555' as time(3));--440243.056

--- -- Need to be modified
select cast('22:01:21' as time) -  cast('22:01:88' as time);--NULL warning
select cast('22:01:21' as time) -  NULL;--NULL
select cast('22:01:21' as time) -  cast('22:01:88' as time);--NULL warning
select cast('22:01:01' as time) -  cast('22:01:88' as time);--NULL warning
select cast('22:01:01' as time) -  cast('22:01:01' as time);-- 0
select cast(0 as time)  - 44;-- -44
select cast(0 as time)  - 44.88;-- -44.88
select 220121.5 - cast('22:01:21' as time);--0.5
select 220121.4 - cast('22:01:21' as time);--0.4
select 220121.00 - cast('22:01:21' as time);--0.00
select 220121 - cast('22:01:21' as time);--0
select 220121.5 - cast('22:01:21.3' as time(1));--0.2

--btree and hash index
create table idx_1 (idx time);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('01:01:21'),('11:01:21'),('22:01:21');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx > '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx = '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx <= '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx >= '11:01:21';

create table idx_2 (idx time);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('01:01:21'),('11:01:21'),('22:01:21');
explain (COSTS OFF) select * from idx_2 where idx = '11:01:21';
explain (COSTS OFF) select * from idx_2 where idx > '11:01:21';

drop table idx_1,idx_2;
create table test_date(c1 time, c2 time);
insert into test_date values('01:01:21','01:01:22'),('22:01:21','22:01:22');
create table test_tmp(c1 time, c2 time);
insert into test_tmp values('01:02:21','01:02:22'),('22:02:21','22:02:22');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a time);
create or replace procedure proc1()
as
$$
declare
i int := 1;
j int := 1;
k int := 1;
begin
    while i <= 20 loop
	j = 1;
		while j <=20 loop 
		k = 1;
			while k <= 59 loop
				insert into tb1 (a) values (i*10000+j*100+k);
				k = k+1;
			end loop;
			j = j+1;
		end loop;
    i = i+1;
    end loop;
end;
$$ language plmysql;
call proc1();
create index idx1 on tb1 using hash(a);
vacuum full analyze tb1;
explain (COSTS OFF) select * from tb1 where a = '22:02:21';

drop index idx1;
create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < '01:01:02';
explain (COSTS OFF) select * from tb1 where a <= '01:01:02';
explain (COSTS OFF) select * from tb1 where a = '01:01:02';
explain (COSTS OFF) select * from tb1 where a != '01:01:02';
explain (COSTS OFF) select * from tb1 where a >= '20:20:58';
explain (COSTS OFF) select * from tb1 where a > '20:20:58';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

create table tb1(id time);
create table tb2(id time);
insert into tb1 values('22:02:21');
insert into tb2 select id from tb1;
insert into tb2 select id + 100 from tb1;
select * from tb1;
select * from tb2;
drop table tb1,tb2;

select cast( cast('22:02:21' as time) + 100 as time);--22:03:21
select cast( cast(000101.8 as time) + 100 as time);--00:02:02

create table tb3(id int);
insert into tb3 values(102233);
insert into tb3 values(102244);
create table tb4 as select cast(id as time) from tb3;
select cast(id as time) from tb3;
create table tb5 as select cast('102255' as time);
drop table tb3;
drop table tb4;
drop table tb5;

-- * -- Need to be modified
select cast('222222' as time) + cast('222222' as time); --444444
select cast('222222' as time) - cast('222220' as time); --2
select cast('222222' as time) * cast('222220' as time); --49382172840
select cast('222222' as time) * 111; --24666642
select cast('222222' as time) * 111.11; --24691086.42
select 111 * cast('222220' as time); --24666420
select 111.11 * cast('222220' as time); --24690864.20
select cast('222222' as time) * 111111; --24691308642
select cast('222222.1111' as time(2)) * cast('222220.2222' as time(1));--49382241728.622

-- / -- Need to be modified
select cast('222222' as time) / cast('222220' as time); --1.0000
select cast('222222' as time) / 111; --2002.0000
select cast('222222' as time) / 111.1; -- 2000.1980
select cast('222222' as time) / 115; -- 1932.3652
select cast('222222' as time) / 0; -- NULL warning
select 111111 / cast('222220' as time); --0.5000
select 111111.111 / cast('222220' as time);--0.5000050
select 0 / cast('222220' as time); --0.0000
select cast('222222.1111' as time(2)) / cast('222220.44' as time(1));--1.000008

-- div
select cast('222222' as time) div cast('222220' as time); --1
select cast('222222' as time) div 111; --2002
select cast('222222' as time) div 111.1; -- 2000
select cast('222222' as time) div 115; -- 1932
select cast('222222' as time) div 0; -- NULL warning
select 111111 div cast('222220' as time); --0
select 111111.111 div cast('222220' as time);--0
select 0 div cast('222220' as time); --0
select cast('222222' as time) div 115.1; --1930
select cast('222222' as time) div 115.2; --1929
select cast('222222.444' as time(2)) div 115.2;--1929

-- %
select cast('111111' as time) % cast('111110' as time); --1
select cast('111111' as time) % 111; --0
select cast('222222' as time) % 111.1; --22.0
select cast('111111' as time) % 0; --NULL warning
select 111111 % cast('222220' as time); --111111
select 111111.111 % cast('222220' as time); --111111.111
select 0 % cast('222220' as time); --0
select cast('111111.11' as time(1)) % cast('111110.33' as time(3)); --0.770

-- mod
select cast('111111' as time) mod cast('111110' as time); --1
select cast('111111' as time) mod 111; --0
select cast('222222' as time) mod 111.1; --22.0
select cast('111111' as time) mod 0; --NULL warning
select 111111 mod cast('222220' as time); --111111
select 111111.111 mod cast('222220' as time); --111111.111
select 0 mod cast('222220' as time); --0
select cast('111111' as time) MOD cast('111110' as time); --1
select cast('111111.66' as time(1)) MOD cast('111110.22222' as time(2));--1.48

-- &
select cast('111111' as time) & cast('111110' as time); --111110
select cast('111111' as time) & 111; --7
select cast('222222' as time) & 111.1; --14
select cast('111111' as time) & 0; --0
select 111111 & cast('222220' as time); --73732
select 111111.111 & cast('222220' as time); --73732
select 0 & cast('222220' as time); --0
select cast('222222' as time) & 1.4; --0
select cast('222222' as time) & 1.5; --2
select cast('222222.444444' as time(5)) & 1.5;--2
select cast('1.1' as time(5)) & 3;--1
select cast('1.9' as time(5)) & 3;--2

-- |
select cast('111111' as time) | cast('111110' as time); --111111
select cast('111111' as time) | 111; --111215
select cast('222222' as time) | 111.1; --222319
select cast('111111' as time) | 0; --111111
select 111111 | cast('222220' as time); --259599
select 111111.111 | cast('222220' as time); --259599
select 0 | cast('222220' as time); --222220
select cast('222222' as time) | 1.4; --222223
select cast('222222' as time) | 1.5; --222222
select cast('111111.77777' as time(3)) | cast('111110.4444' as time(2)); --111118

-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~cast('111111' as time) ;--18446744073709440504

-- ^
select cast('111111' as time) ^ cast('111110' as time); --1
select cast('111111' as time) ^ 111; --111208
select cast('222222' as time) ^ 111.1; --222305
select cast('111111' as time) ^ 0; --111111
select 111111 ^ cast('222220' as time); --185867
select 111111.111 ^ cast('222220' as time); --185867
select 0 ^ cast('222220' as time); --222220
select cast('222222' as time) ^ 1.4; --222223
select cast('222222' as time) ^ 1.5; --222220

-- >>
select cast('111111' as time) >> cast('111110' as time); --0
select cast('111111' as time) >> 111; --0
select cast('222222' as time) >> 111.1; --0
select cast('111111' as time) >> 0; --111111
select 111111 >> cast('222220' as time); --0
select 111111.111 >> cast('222220' as time); --0
select 0 >> cast('222220' as time); --0
select cast('222222' as time) >> 1.4; --111111
select cast('222222' as time) >> 1.5; --55555
select cast('111111.7777' as time(3)) >> 2;--27778

-- <<
select cast('111111' as time) << cast('111110' as time); --0
select cast('111111' as time) << 111; --0
select cast('222222' as time) << 111.1; --0
select cast('111111' as time) << 0; --111111
select 111111 << cast('222220' as time); --0
select 111111.111 << cast('222220' as time); --0
select 0 << cast('222220' as time); --0
select cast('222222' as time) << 1.4; --444444
select cast('222222' as time) << 1.5; --888888
select cast('111111.7777' as time(3)) << 2;--444448

-- bit_count
select bit_count(cast('111111' as time));  --8
select bit_count(cast('111111' as time));  --8
select bit_count(cast('99991231235959' as time));  --12
select bit_count(cast('111111.4' as time));  --8
select bit_count(cast('111111.5' as time));  --6
select bit_count(cast('111111.5' as time(4))); --6

-- &&
select cast('111111' as time) && cast('111110' as time); --1
select cast('111111' as time) && 111; --1
select cast('222222' as time) && 111.1; --1
select cast('111111' as time) && 0; --0
select 111111 && cast('222220' as time); --1
select 111111.111 && cast('222220' as time); --1
select 0 && cast('222220' as time); --0
select cast('222222' as time) && 1.4; --1
select cast('222222' as time) && 1.5; --1
select cast('222222.555555' as time(3)) && 1.5;--1

-- ||
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
select cast('111111' as time) || cast('111110' as time); --1
select cast('111111' as time) || true; --1
select cast('222222' as time) || false; --1
select true || cast('111111' as time); --1
select false || cast('222222' as time); --1

-- !
select !cast('111111' as time) ;--0

-- and or xor
select cast('222222' as time) and cast('222222' as time );--1
select cast('222222' as time) and cast('222222' as time ) and 0::boolean;--0
select cast('222222' as time) or cast('222222' as time );--1
select cast('222222' as time) or cast('222222' as time ) or 0::boolean;--1
select cast('222222' as time) xor cast('222222' as time );--0
select cast('222222' as time) xor cast('222222' as time ) xor 0::boolean;--0
select cast('222222' as time) xor cast('222222' as time ) xor 1::boolean;--1

-- between and
create table t1 (f1 time, f2 time, f3 time);
insert into t1 values('01:03:01','01:01:01.555555','01:11:01.555555');
insert into t1 values('01:03:02','01:01:02.555555','01:11:01.555555');
insert into t1 values('01:03:03','01:01:03.555555','01:11:01.555555');
insert into t1 values('01:03:04','01:01:04.555555','01:11:01.555555');
insert into t1 values('01:03:05','01:01:05.555555','01:11:01.555555');
select f2 from t1 where f2 between '01:01:02' and '01:01:04';
select f1, f2, f3 from t1 where f1 between f2 and f3;
select f2 from t1 where '01:01:05.6666' between f2 and '01:01:05.777777'; -- Need to be modified
select f2 from t1 where cast('01:01:05.6666' as time) between f2 and '01:01:05.777777'; -- Need to be modified
/* select f2 from t1 where cast('01:01:05.6666' as time) between f2 and '01:01:05.777777';
f2 
----
(0 rows)*/
drop table t1;

-- in
create table t1(f2 time);
insert into t1 values('00:00:00'),('12:34:56'),('11:22:33');
select * from t1 where f2 in ('00:00:00','12:34:56','11:22:33');
select * from t1 where f2 not in ('12:34:56');
drop table t1;

-- is
CREATE TABLE t1 (a time, b INT);
INSERT INTO t1 VALUES ('12:34:56',1), ('00:00:00',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 time, f2 time(3));
insert into t1 values('01:01:01', '01:01:01.555555');
insert into t1 values('23:59:59', '23:59:59.555555');
insert into t1 values('23:59:59.6', '23:59:59.555555');

select f1 from t1 where f1 like '01:01:01'; 
select f1 from t1 where f1 like cast('01:01:01' as time);
select f1 from t1 where f1 like 10101;
select f1 from t1 where f1 like '10101'; -- Need to be modified
select f1 from t1 where f1 like cast('10101' as time);
select f1 from t1 where f1 like cast(10101 as time);
select f1 from t1 where f1 like cast('10101.555555' as time);
select f1 from t1 where f1 like cast('10101.555555' as time(3));

select f2 from t1 where f2 like 10101;
select f2 from t1 where f2 like '10101';
select f2 from t1 where f2 like cast('10101' as time);
select f2 from t1 where f2 like cast(10101 as time);
drop table t1;


--MySQL time() type test case start
select cast('1.1' as time(5));-- 00:00:01.10000
select cast('11.1' as time(5));--00:00:11.10000
select cast('1.11' as time(5));--00:00:01.11000
select cast('11.11' as time(5));--00:00:11.11000
select cast('11.111' as time(5));--00:00:11.11100
select cast('99.111' as time(5));--NULL warning
select cast('99.311' as time(5));--NULL warning
select cast('1.1111' as time(5));--00:00:01.11110
select cast('1.111122222' as time(5));--00:00:01.11112
select cast('998.3' as time(5));--NULL warning
select cast('991.31' as time(5));--NULL warning
select cast('999.31' as time(5));--NULL warning
select cast('111.111' as time(5));-- 00:01:11.11100
select cast('116.119' as time(5));-- 00:01:16.11900
select cast('116.419' as time(5));-- 00:01:16.41900
select cast('998.2222' as time(5));--NULL warning
select cast('998.2224' as time(5));--NULL warning
select cast('998.222333' as time(5));--NULL warning
select cast('998.222360' as time(5));--NULL warning
select cast('998.22235959' as time(5));--NULL warning
select cast('998.222359599999' as time(5));--NULL warning
select cast('998.2223595999999999999999' as time(5));--NULL warning
select cast('1118.1' as time(5));--00:11:18.10000
select cast('1118.12' as time(5));-- 00:11:18.12000
select cast('1118.122' as time(5));-- 00:11:18.12200
select cast('1118.122212' as time(5));--00:11:18.12221
select cast('1118.123212' as time(5));--00:11:18.12321
select cast('1118.123124' as time(5));--00:11:18.12312
select cast('1118.12312323' as time(5));--00:11:18.12312
select cast('1118.12312366' as time(5));--00:11:18.12312
select cast('1118.1222235659' as time(5));--00:11:18.12222
select cast('1118.122223565999999999999999' as time(5));--00:11:18.12222

select cast('11118.1' as time(5));--01:11:18.10000
select cast('11118.12' as time(5));-- 01:11:18.12000
select cast('11118.32' as time(5));-- 01:11:18.32000
select cast('11118.111111111' as time(5));--01:11:18.11111
select cast('11118.116011111' as time(5));--01:11:18.11601
select cast('11118.110660111' as time(5));--01:11:18.11066
select cast('11118.1106019999999999999999' as time(5));--01:11:18.11060

select cast('111121.1' as time(5));--11:11:21.10000
select cast('111121.12' as time(5));--11:11:21.12000
select cast('111121.24' as time(5));--11:11:21.24000
select cast('111121.231' as time(5));--11:11:21.23100
select cast('111121.2312' as time(5));--11:11:21.23120
select cast('111121.231260' as time(5));--11:11:21.23126
select cast('111521.231259' as time(5));--11:15:21.23126
select cast('111121.231259' as time(5));--11:11:21.23126
select cast('111121.2312599999999999999' as time(5));--11:11:21.23126

select cast('1111213.5' as time(5));--111:12:13.50000
select cast('1111213.11' as time(5));--111:12:13.11000
select cast('1111213.112' as time(5));-- 111:12:13.11200
select cast('1111213.1122' as time(5));--111:12:13.11220
select cast('1111213.11222222222' as time(5));--111:12:13.11222
select cast('1111213.99' as time(5));--111:12:13.99000
select cast('1111213.59' as time(5));--111:12:13.59000
select cast('1111213.60' as time(5));-- 111:12:13.60000
select cast('1111213.5960' as time(5));-- 111:12:13.59600
select cast('1111213.5959' as time(5));--111:12:13.59590
select cast('1111213.595988888888888' as time(5));--111:12:13.59599

select cast('11111121.9' as time(5));--838:59:59.00000 warning
select cast('11111121.23' as time(5));--838:59:59.00000 warning
select cast('11111121.24' as time(5));--838:59:59.00000 warning
select cast('11111121.235555' as time(5));--838:59:59.00000 warning
select cast('11111121.245555' as time(5));--838:59:59.00000 warning
select cast('11111121.236055' as time(5));--838:59:59.00000 warning
select cast('11111121.235560' as time(5));--838:59:59.00000 warning
select cast('11111121.235555999999999' as time(5));--838:59:59.00000 warning

select cast('111111211.2' as time(5));--838:59:59.00000 warning
select cast('111111241.2' as time(5));--838:59:59.00000 warning
select cast('111111231.2' as time(5));--838:59:59.00000 warning
select cast('111111239.2' as time(5));--838:59:59.00000 warning
select cast('111111239.59' as time(5));--838:59:59.00000 warning
select cast('111111239.60' as time(5));--838:59:59.00000 warning
select cast('111111239.5999999999' as time(5));--838:59:59.00000 warning
select cast('1111112111.2' as time(5));--838:59:59.000009 warning
select cast('1111112411.2' as time(5));--838:59:59.00000 warning
select cast('1111112159.2' as time(5));--838:59:59.00000 warning
select cast('1111112160.2' as time(5));--NULL warning
select cast('1111112159.59' as time(5));--838:59:59.00000 warning
select cast('1111112159.5999999999999' as time(5));--838:59:59.00000 warning

select cast('11111121591.2' as time(5));--21:59:01.20000
select cast('11111121599.23' as time(5));--21:59:09.23000
select cast('11111121599.23444' as time(5));-- 21:59:09.23444
select cast('11111121599.99999999999999999' as time(5));--21:59:10.00000
select cast('11111124599.23' as time(5));--NULL warning
select cast('11111121609.23444' as time(5));--NULL warning
select cast('111111215059.2' as time(5));--21:50:59.20000
select cast('111111215060.2' as time(5));--NULL warning
select cast('111111215019.23444' as time(5));--21:50:19.23444
select cast('111111215059.23455556' as time(5));--21:50:59.23456
select cast('111111215059.99999999' as time(5));--21:51:00.00000

select cast('1111112150591.9' as time(5));--21:50:59.00000 warning
select cast('1111112150601.9' as time(5));--NULL warning
select cast('1111112150591.99' as time(5));--221:50:59.00000 warning
select cast('1111112150591.9999999999999' as time(5));--21:50:59.00000 warning

select cast('11111111201911.9' as time(5));--20:19:11.90000
select cast('11111111241911.9' as time(5));--NULL warning
select cast('11111111236011.9' as time(5));-- NULL warning
select cast('11111111201911.9999999999' as time(5));--20:19:12.00000

select cast('111111112019111.9' as time(5));-- 20:19:11.00000
select cast('111111111260111.9' as time(5));--NULL warning
select cast('11111111201911111111.9' as time(5));--20:19:11.00000 warning
select cast('1111111112360111.9' as time(5));--12:36:01.00000 warning
select cast('111111112019113333.9999999999' as time(5));--20:19:11.00000 warning

select cast('1.1.1' as time(5));--00:00:01.1000 warning
select cast('1.1.11' as time(5));--00:00:01.10000 warning
select cast('11.11.11' as time(5));--00:00:11.11000 warning
select cast('11.13.11' as time(5));--00:00:11.13000 warning
select cast('11.13.32' as time(5));--00:00:11.13000 warning
select cast('99.11.11' as time(5));--NULL warning

select cast('99.111.11' as time(5));--NULL warning
select cast('99.111.1111' as time(5));--NULL warning
select cast('99.111.3111' as time(5));--NULL warning
select cast('99.111.116111' as time(5));--NULL warning
select cast('99.111.115961' as time(5));--NULL warning
select cast('99.111.115911' as time(5));--NULL warning
select cast('99.1111.115911' as time(5));--NULL warning
select cast('99.1133.115911' as time(5));--NULL warning
select cast('99.1130.115911' as time(5));--NULL warning
select cast('99.1311.11' as time(5));--NULL warning

select cast('10000.1.1' as time(5));--01:00:00.10000 warning
select cast('10001.1.1' as time(5));--01:00:01.10000 warning
select cast('11111.1.1' as time(5));-- 01:11:11.10000 warning
select cast('11111.11.11' as time(5));-- 01:11:11.11000 warning
select cast('11111.24.11' as time(5));--01:11:11.24000 warning
select cast('199999.23.59' as time(5));--NULL warning
select cast('11111.23.11' as time(5));--01:11:11.23000 warning
select cast('11111.23.59' as time(5));--01:11:11.23000  warning
select cast('11111.23232.99999' as time(5));-- 01:11:11.23232 warning
select cast('11111.232323.99999' as time(5));--01:11:11.23232 warning
select cast('11111.23232233.99999' as time(5));--01:11:11.23232 warning

select cast('111111.23.9' as time(5));--11:11:11.23000 warning
select cast('111111.24.9' as time(5));--11:11:11.24000 warning
select cast('111111.221.9' as time(5));--11:11:11.22100 warning
select cast('111111.22111111.9' as time(5));--11:11:11.22111 warning
select cast('111111.2260111.9999' as time(5));--11:11:11.22601 warning
select cast('111111.22111111.9999999999' as time(5));--11:11:11.22111 warning

select cast('1111111.3231.9' as time(5));--111:11:11.32310 warning
select cast('1111111.6031.9' as time(5));--111:11:11.60310 warning
select cast('1111111.5931.9' as time(5));--111:11:11.59310 warning
select cast('1111221.59.29' as time(5));--111:12:21.59000 warning
select cast('1111221.59599999999.29' as time(5));-- 111:12:21.59600 warning

select cast('11111211.1.29' as time(5));--838:59:59.00000 warning
select cast('11111211.1112.29' as time(5));--838:59:59.00000 warning
select cast('11111211.1160.29' as time(5));--838:59:59.00000 warning
select cast('11111211.1120.69' as time(5));--838:59:59.00000 warning
select cast('11111211.1159599999999.29' as time(5));--838:59:59.00000 warning
select cast('11111211.111616.99999' as time(5));--838:59:59.00000 warning

select cast('111112111.1.29' as time(5));--838:59:59.00000 warning
select cast('111112241.1.29' as time(5));--838:59:59.00000 warning
select cast('111112231.1.29' as time(5));--838:59:59.00000 warning
select cast('111112239.1.29' as time(5));--838:59:59.00000 warning
select cast('111112239.60.29' as time(5));--838:59:59.00000 warning
select cast('111112239.6.9999' as time(5));--838:59:59.00000 warning
select cast('1111122329.6.9' as time(5));--838:59:59.00000 warning
select cast('1111122329.6.9999' as time(5));--838:59:59.00000 warning
select cast('1111122329.60.9' as time(5));--838:59:59.00000 warning
select cast('1111122360.16.9' as time(5));--NULL warning
select cast('1111122416.16.9' as time(5));--838:59:59.00000 warning
select cast('1111122216.599999.999999999999' as time(5));--838:59:59.00000 warning

select cast('11111222161.1.1' as time(5));--NULL warning
select cast('11111222161.9999999999.1' as time(5));--22:16:02.00000 warning
select cast('11111222161.9999999999.9999999999' as time(5));--22:16:02.00000 warning
select cast('111112226011.9.9' as time(5));--NULL warning
select cast('111112226011.9999999999.9999999999' as time(5));--NULL warning
select cast('111112225911.9.9' as time(5));--NULL warning
select cast('111112222611.9999999999.9999999999' as time(5));--22:26:12.00000 warning
select cast('1111122216119.9999.99999' as time(5));-- 22:16:11.00000 warning
select cast('1111122216111.9999.99999' as time(5));-- 22:16:11.00000 warning

select cast('11111222161121.9999.99999' as time(5));--NULL warning
select cast('111112221611211.9999.99999' as time(5));--16:11:21.00000 warning
select cast('1111122216112111111.9999.99999' as time(5));--16:11:21.00000 warning

select cast('1.1.1.1' as time(5));--00:00:01.10000 warning
select cast('11.11.11.11' as time(5));--00:00:11.11000 warning
select cast('1.1.1.11.11' as time(5));--00:00:01.10000 warning
select cast('111.11.11.11' as time(5));--00:01:11.11000  warning
select cast('1111.11.11.11' as time(5));--00:11:11.11000 warning
select cast('1111.60.4.11' as time(5));--00:11:11.60000 warning
select cast('111111.11.11.11' as time(5));--11:11:11.11000 warning
select cast('1111111.111.11.11' as time(5));--111:11:11.11100  warning
select cast('11111111.11.4.11' as time(5));--838:59:59.00000 warning
select cast('111111111.11.4.11' as time(5));--838:59:59.00000 warning
select cast('1111111111.11.4.11' as time(5));--838:59:59.00000 warning
select cast('11111111111.11.4.11' as time(5));-- NULL warning
select cast('111111111111.11.4.11' as time(5));--NULL warning
select cast('1111111111111.11.4.11' as time(5));--11:11:11.00000 warning
select cast('11111111111111.11.4.11' as time(5));--NULL warning
select cast('11111111111111111.11.4.11' as time(5));--11:11:11.00000 warning
select cast('11.11.11.23.34.50' as time(5));--00:00:11.11000 warning 
select cast('9999.12.31.23.59.59.999999.99' as time(5));--NULL warning
select cast('99.12.31.23.59.59.999999.99' as time(5));--NULL warning

select cast('111/11/11/23/34/50' as time(5));--00:01:11.00000 warning
select cast('10000/11/11/23/34/50' as time(5));--01:00:00.00000 warning
select cast('1000/13/11/23/34/50' as time(5));--00:10:00.00000 warning
select cast('9999/11/60/23/34/50' as time(5));--NULL warning
select cast('9999/11/59/24/34/50' as time(5));--NULL warning
select cast('9999/11/59/23/60/50' as time(5));--NULL warning
select cast('9999/11/59/23/59/60' as time(5));--NULL warning
select cast('9999/11/19/23/59/59/99999999999' as time(5));--NULL warning

select cast('111.11.11.23.34.50' as time(5));--00:01:11.11000 warning
select cast('1111/11.11.23/34/50' as time(5));--00:11:11.00000  warning
select cast('111/11.11.23/34/50' as time(5));--00:01:11.00000 warning
select cast('111/11.11/23/34/50' as time(5));--00:01:11.00000 warning
select cast('111/11.11.24/34/50' as time(5));--00:01:11.00000 warning
select cast('111/11.11.23/60/50' as time(5));--00:01:11.00000 warning
select cast('111/11.11.23/34/60' as time(5));--00:01:11.00000 warning
select cast('111/11.11.23/34/59/9999999' as time(5));--00:01:11.00000 warning
select cast('10000/11/11/23/34/50' as time(5));--01:00:00.00000 warning

select cast(' ' as time(5));--NULL
select cast('' as time(5));--NULL

select cast('2023-03-03 20:01:20AM' as time(5));-- 20:01:20.00000 warning
select cast('20230303200120PM' as time(5));--NULL warning
select cast('20230303200120AM' as time(5));--NULL  warning
select cast('20230303T200120' as time(5));--20:01:20.00000
select cast('2003-03-03T20:01:20' as time(5));--00:20:03.00000 warning

select cast('111121.444444' as time(5));--11:11:21.44444
select cast('111121.555555' as time(5));--11:11:21.55556
select cast('111121.444444444' as time(5));--11:11:21.44444
select cast('111121.555555555' as time(5));--11:11:21.55556
select cast('111121.555555555' as time(6));--11:11:21.11:11:21.555556

--explict conversion 
select cast(1 as time(5));--00:00:01.00000
select cast(11 as time(5));-- 00:00:11.00000
select cast(111 as time(5));-- 00:01:11.00000
select cast(1111 as time(5));--00:11:11.00000
select cast(1131 as time(5));--00:11:31.00000
select cast(11111 as time(5));--01:11:11.00000
select cast(11111.77 as time(5));--01:11:11.77000
select cast(111111 as time(5));--11:11:11.00000
select cast(111311 as time(5));--11:13:11.00000
select cast(111111.88 as time(5));--11:11:11.88000
select cast(113111 as time(5));--11:31:11.00000
select cast(112160 as time(5));--NULL warning
select cast(1111111 as time(5));--111:11:11.00000
select cast(1111111.34 as time(5));--111:11:11.34000
select cast(1111161 as time(5));--NULL warning
select cast(11111111 as time(5));--NULL warning
select cast(11111111.56 as time(5));--NULL warning
select cast(11113111 as time(5));--NULL warning
select cast(11112160 as time(5));--NULL warning
select cast(111111111 as time(5));--NULL warning
select cast(111111111.99 as time(5));--NULL warning
select cast(111351111 as time(5));--NULL warning
select cast(1111111111 as time(5));--NULL warning
select cast(1111111171 as time(5));--NULL warning
select cast(11111116059 as time(5));--NULL warning
select cast(11111111111 as time(5));--11:11:11.00000
select cast(111111111111 as time(5));--11:11:11.00000
select cast(11111111111.33 as time(5));-- 11:11:11.33000
select cast(111111111111.45 as time(5));--11:11:11.45000
select cast(111111241156 as time(5));--NULL warning
select cast(1111111111111 as time(5));--11:11:11.00000
select cast(1111111311105 as time(5));--NULL warning
select cast(11111111111111 as time(5));--11:11:11.00000
select cast(11111111111111.23 as time(5));--11:11:11.23000
select cast(11111111119905 as time(5));--NULL warning
select cast(11111111111165 as time(5));--NULL warning
select cast(11111111251105 as time(5));--NULL warning
select cast(111111111111111 as time(5));--NULL warning
select cast(1111111111111111 as time(5));--NULL warning
select cast(1111111111111111.44 as time(5));--NULL warning
select cast(11111111111233444 as time(5));--NULL warning
select cast(111111111111111111111 as time(5));--NULL warning

select cast(19830905 as time(5));--NULL warning
select cast(119830905 as time(5));--NULL warning
select cast(198309051 as time(5));--NULL warning
select cast(198205051 as time(5));--NULL warning
select cast(128205051 as time(5));--NULL warning
select cast(6995 as time(5));--NULL warning
select cast(2007-05-23 as time(5));--NULL warning
select cast(2007-05*23 as time(5));--NULL warning
select cast(830905 as time(5));--83:09:05.00000
select cast(9830905 as time(5));--NULL warning
select cast(30905 as time(5));--03:09:05.00000
select cast(230905 as time(5));--23:09:05.00000
select cast(690905 as time(5));--69:09:05.00000
select cast(700905 as time(5));--70:09:05.00000
select cast(0905 as time(5));--00:09:05.00000
select cast(905 as time(5));--00:09:05.00000
select cast(00000905 as time(5));-- 00:09:05.00000
select cast(0000000905 as time(5));--00:09:05.00000
select cast(2007*05*23 as time(5));--23:08:05.00000
select cast(05*23 as time(5));--00:01:15.00000
select cast(06/03 as time(5));--00:00:02.00000
select cast(300/03 as time(5));--00:01:00.00000
select cast(30000000/3 as time(5)); --NULL warning

select cast(11111111111111.444444 as time(5));--11:11:11.44444
select cast(11111111111111.444444444 as time(5));--11:11:11.44444
select cast(11111111111111.555555 as time(5));--11:11:11.55556
select cast(11111111111111.555555555 as time(5));--11:11:11.55556
select cast(11111111111111.555555555 as time(6));--11:11:11.555556

--round
select cast('2010.01.31.23.59.59.99999999' as time(5));--00:20:10.01000 warning
select cast('2010.01.31.23.59.59.999999' as time(5));--00:20:10.01000 warning
select cast('2010.00.31.23.59.59.99999999' as time(5));--00:20:10.00000 warning
select cast('23:59:59.99999999' as time(5));--24:00:00.00000
select cast('23:59:59.999999' as time(5));--24:00:00.00000
select cast('23:59:59.99999999' as time(5));--24:00:00.00000
select cast(1111111.123454789 as time(5)(5));--111:11:11.12346

--set sql_mode
set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt time(5));
insert into test values('2100-00-04 12:13:14');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));-- 23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));-- 23:59:59.99000
select cast(99991231235959 as time(5));-- 23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));-- 23:59:59.40000
select cast(99991231235959.5 as time(5));-- 23:59:59.50000
select * from test;
delete from test;

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--warning
insert into test values('9999-12-31 23:59:59.99');--warning
select cast('2010-02-31 00:00:00' as time(5));--NULL
select cast(20100231121314 as time(5));--NULL
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000
select * from test;
delete from test;

set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
insert into test values('2100-00-04 12:13:14');--warning
insert into test values('0000-00-00 12:13:14');
insert into test values('1111-01-01 25:00:00');--ERROR
select cast('2010-02-31 00:00:00' as time(5));--NULL warning
select cast(20100231121314 as time(5));--NULL warning
select cast('9999-12-31 23:59:59.44' as time(5));--23:59:59.44000
select cast('9999-12-31 23:59:59.99' as time(5));--23:59:59.99000
select cast(99991231235959 as time(5));--23:59:59.00000
select cast(99991231235959.4 as time(5));--23:59:59.40000
select cast(99991231235959.5 as time(5));--23:59:59.50000

insert into test values('');
insert into test values('99999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-02-31 12:13:14');--ERROR
insert into test values('9999-00-31 12:13:14');--warning
insert into test values('0000-00-00 12:13:14');

--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select *  from test;

set sql_mode ='ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--ERROR
insert into test values(2020091423);--ERROR
insert into test values(202009142311);--ERROR
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--ERROR
insert into test values(0914231817.49999);--ERROR
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--ERROR
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

----20231013
set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE,ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);
insert into test values(2);
insert into test values(21);
insert into test values(222);
insert into test values(2221);
insert into test values(22211);
insert into test values(21211);
insert into test values(20200933);--warning
insert into test values(2020091423);--warning
insert into test values(202009142311);--warning
insert into test values(20200914231817);--warning
insert into test values(20200914231817.66);--warning
insert into test values(20200914231817.49999);--warning
insert into test values(20200934231817.49999);--warning
insert into test values(0914231817.49999);--warning
insert into test values(190914231817.49999);--warning
insert into test values(790914231817.49999);--warning
insert into test values(14231817.49999);--warning
insert into test values(231817.49999);
insert into test values(230017);
insert into test values(99991231235959);--warning
insert into test values(99991231235959.44);--warning
insert into test values(99991231235959.99);--warning
select * from test;

drop table test;
set sql_mode to default;
--
create table test1(dt1 time(5), dt2 time(3), dt3 time(5));
insert into test1 values('12:13:14.444444', '12:13:14.444444', '12:13:14.444444');
insert into test1 values('12:13:14.444444444', '12:13:14.444444444', '12:13:14.444444444');
insert into test1 values('12:13:14.555555', '12:13:14.555555', '12:13:14.555555');
insert into test1 values('12:13:14.555555555', '12:13:14.555555555', '12:13:14.555555555');
insert into test1 values(222.444444, 222.444444, 222.444444);
insert into test1 values(2221.444444444, 2221.444444444, 2221.444444444);
insert into test1 values(22211.555555, 22211.555555, 22211.555555);
insert into test1 values(22211.555555555, 22211.555555555, 22211.555555555);

select * from test1;
drop table test1;

create table test2(dt1 time(5));
insert into test2 values('12:13:14.123454789');
insert into test2 values(1111111.123454789);

select * from test2;
drop table test2;
-- oper
---=
select cast('22:01:21' as time(5)) = cast('22:01:21' as time(5));--1
select 212 = cast('22:01:60' as time(5));--NULL warning
select 212 = cast('22:01:21' as time(5));--0
select 220121.01 = cast('22:01:21' as time(5));--0
select NULL = cast('22:01:21' as time(5));--NULL
select 220121.5 = cast('22:01:21' as time(5));--0
select 220121.4 = cast('22:01:21' as time(5));--0
select 220121.00 = cast('22:01:21' as time(5));--1
select 220121 = cast('22:01:21' as time(5));--1
select cast(102233.4 as time(5)) = cast('10:22:33' as time(5));--0
select cast(102233.5 as time(5)) = cast('10:22:33.5' as time(5));--1
select cast(102233.5 as time(5)) = cast('10:22:33' as time(5));--0
select 102233 = cast('10:22:33' as time(5));--1
select 102233.4 = cast('10:22:33' as time(5));--0
select 102233.5 = cast('10:22:33.5' as time(5));--1
select 102233.5 = cast('10:22:33' as time(5));--0
select '10:22:33' = cast('10:22:33' as time(5));--0 -- Need to be modified
select '10:22:33.4' = cast('10:22:33' as time(5));--0
select '10:22:33.5' = cast('10:22:33' as time(5));--0
select '10:22:33.5' = cast('10:22:33.5' as time(5));--0 -- Need to be modified
select '10:22:33.5' = cast('10:22:33.5' as time(1));--1
select '10:22:33.5' = cast('10:22:33.5' as time(2));--0 -- Need to be modified

--<>
select cast('22:01:21' as time(5)) <> cast('22:01:21' as time(5));--0
select 212 <> cast('22:01:60' as time(5));--NULL warning
select 212 <> cast('22:01:21' as time(5));--1
select 220121.01 <> cast('22:01:21' as time(5));--1
select NULL <> cast('22:01:21' as time(5));--NULL
select 220121.5 <> cast('22:01:21' as time(5));--1
select 220121.4 <> cast('22:01:21' as time(5));--1
select 220121.00 <> cast('22:01:21' as time(5));--0
select 220121 <> cast('22:01:21' as time(5));--0
select cast(102233.4 as time(5)) <> cast('10:22:33' as time(5));--1
select cast(102233.5 as time(5)) <> cast('10:22:33.5' as time(5));--0
select cast(102233.5 as time(5)) <> cast('10:22:33' as time(5));--1
select 102233 <> cast('10:22:33' as time(5));--0
select 102233.4 <> cast('10:22:33' as time(5));--1
select 102233.5 <> cast('10:22:33.5' as time(5));--0
select 102233.5 <> cast('10:22:33' as time(5));--1
select '10:22:33' <> cast('10:22:33' as time(5));--1 -- Need to be modified
select '10:22:33.4' <> cast('10:22:33' as time(5));--1
select '10:22:33.5' <> cast('10:22:33' as time(5));--1
select '10:22:33.5' <> cast('10:22:33.5' as time(5));--1 -- Need to be modified
select '10:22:33.5' <> cast('10:22:33.5' as time(1));--0
select '10:22:33.5' <> cast('10:22:33.5' as time(2));--1 -- Need to be modified

--<
select cast('22:01:21' as time(5)) < cast('22:01:21' as time(5));--0
select 212 < cast('22:01:60' as time(5));--NULL warning
select 212 < cast('22:01:21' as time(5));--1
select 220121.01 < cast('22:01:21' as time(5));--0
select NULL < cast('22:01:21' as time(5));--NULL
select 220121.5 < cast('22:01:21' as time(5));--0
select 220121.4 < cast('22:01:21' as time(5));--0
select 220121.00 < cast('22:01:21' as time(5));--0
select 220121 < cast('22:01:21' as time(5));--0
select cast(102233.4 as time(5)) < cast('10:22:33' as time(5));--0
select cast(102233.5 as time(5)) < cast('10:22:33.5' as time(5));--0
select cast(102233.5 as time(5)) < cast('10:22:33' as time(5));--0
select 102233 < cast('10:22:33' as time(5));--0
select 102233.4 < cast('10:22:33' as time(5));--0
select 102233.5 < cast('10:22:33.5' as time(5));--0
select 102233.5 < cast('10:22:33' as time(5));--0
select '10:22:33' < cast('10:22:33' as time(5));--1
select '10:22:33.4' < cast('10:22:33' as time(5));--0
select '10:22:33.5' < cast('10:22:33' as time(5));--0
select '10:22:33.5' < cast('10:22:33.5' as time(5));--1 -- Need to be modified
select '10:22:33.5' < cast('10:22:33.5' as time(1));--0
select '10:22:33.5' < cast('10:22:33.5' as time(2));--1 -- Need to be modified

-->
select cast('22:01:21' as time(5)) > cast('22:01:21' as time(5));--0
select 212 > cast('22:01:60' as time(5));--NULL warning
select 212 > cast('22:01:21' as time(5));--0
select 220121.01 > cast('22:01:21' as time(5));--1
select NULL > cast('22:01:21' as time(5));--NULL
select 220121.5 > cast('22:01:21' as time(5));--1
select 220121.4 > cast('22:01:21' as time(5));--1
select 220121.00 > cast('22:01:21' as time(5));--0
select 220121 > cast('22:01:21' as time(5));--0
select cast(102233.4 as time(5)) > cast('10:22:33' as time(5));--1
select cast(102233.5 as time(5)) > cast('10:22:33.5' as time(5));--0
select cast(102233.5 as time(5)) > cast('10:22:33' as time(5));--1
select 102233 > cast('10:22:33' as time(5));--0
select 102233.4 > cast('10:22:33' as time(5));--1
select 102233.5 > cast('10:22:33.5' as time(5));--0
select 102233.5 > cast('10:22:33' as time(5));--1
select '10:22:33' > cast('10:22:33' as time(5));--0
select '10:22:33.4' > cast('10:22:33' as time(5));--1
select '10:22:33.5' > cast('10:22:33' as time(5));--1
select '10:22:33.5' > cast('10:22:33.5' as time(5));--0
select '10:22:33.5' > cast('10:22:33.5' as time(1));--0
select '10:22:33.5' > cast('10:22:33.5' as time(2));--0

--<=
select cast('22:01:21' as time(5)) <= cast('22:01:21' as time(5));--1
select 212 <= cast('22:01:60' as time(5));--NULL warning
select 212 <= cast('22:01:21' as time(5));--1
select 220121.01 <= cast('22:01:21' as time(5));--0
select NULL <= cast('22:01:21' as time(5));--NULL
select 220121.5 <= cast('22:01:21' as time(5));--0
select 220121.4 <= cast('22:01:21' as time(5));--0
select 220121.00 <= cast('22:01:21' as time(5));--1
select 220121 <= cast('22:01:21' as time(5));--1
select cast(102233.4 as time(5)) <= cast('10:22:33' as time(5));--0
select cast(102233.5 as time(5)) <= cast('10:22:33.5' as time(5));--1
select cast(102233.5 as time(5)) <= cast('10:22:33' as time(5));--0
select 102233 <= cast('10:22:33' as time(5));--1
select 102233.4 <= cast('10:22:33' as time(5));--0
select 102233.5 <= cast('10:22:33.5' as time(5));--1
select 102233.5 <= cast('10:22:33' as time(5));--0
select '10:22:33' <= cast('10:22:33' as time(5));--1
select '10:22:33.4' <= cast('10:22:33' as time(5));--0
select '10:22:33.5' <= cast('10:22:33' as time(5));--0
select '10:22:33.5' <= cast('10:22:33.5' as time(5));--1
select '10:22:33.5' <= cast('10:22:33.5' as time(1));--1
select '10:22:33.5' <= cast('10:22:33.5' as time(2));--1

-->=
select cast('22:01:21' as time(5)) >= cast('22:01:21' as time(5));--1
select 212 >= cast('22:01:60' as time(5));--NULL warning
select 212 >= cast('22:01:21' as time(5));--0
select 220121.01 >= cast('22:01:21' as time(5));--1
select NULL >= cast('22:01:21' as time(5));--NULL
select 220121.5 >= cast('22:01:21' as time(5));--1
select 220121.4 >= cast('22:01:21' as time(5));--1
select 220121.00 >= cast('22:01:21' as time(5));--1
select 220121 >= cast('22:01:21' as time(5));--1
select cast(102233.4 as time(5)) >= cast('10:22:33' as time(5));--1
select cast(102233.5 as time(5)) >= cast('10:22:33.5' as time(5));--1
select cast(102233.5 as time(5)) >= cast('10:22:33' as time(5));--1
select 102233 >= cast('10:22:33' as time(5));--1
select 102233.4 >= cast('10:22:33' as time(5));--1
select 102233.5 >= cast('10:22:33.5' as time(5));--1
select 102233.5 >= cast('10:22:33' as time(5));--1
select '10:22:33' >= cast('10:22:33' as time(5));--0 -- Need to be modified
select '10:22:33.4' >= cast('10:22:33' as time(5));--1
select '10:22:33.5' >= cast('10:22:33' as time(5));--1
select '10:22:33.5' >= cast('10:22:33.5' as time(5));--0 -- Need to be modified
select '10:22:33.5' >= cast('10:22:33.5' as time(1));--1
select '10:22:33.5' >= cast('10:22:33.5' as time(2));--0 -- Need to be modified

--<=>
select cast('22:01:21' as time(5)) <=> cast('22:01:21' as time(5));--1
select 212 <=> cast('22:01:60' as time(5));--0 warning
select 212 <=> cast('22:01:21' as time(5));--0
select 220121.01 <=> cast('22:01:21' as time(5));--0
select NULL <=> cast('22:01:21' as time(5));--0
select 220121.5 <=> cast('22:01:21' as time(5));--0
select 220121.4 <=> cast('22:01:21' as time(5));--0
select 220121.00 <=> cast('22:01:21' as time(5));--1
select 220121 <=> cast('22:01:21' as time(5));--1
select cast(102233.4 as time(5)) <=> cast('10:22:33' as time(5));--0
select cast(102233.5 as time(5)) <=> cast('10:22:33.5' as time(5));--1
select cast(102233.5 as time(5)) <=> cast('10:22:33' as time(5));--0
select 102233 <=> cast('10:22:33' as time(5));--1
select 102233.4 <=> cast('10:22:33' as time(5));--0
select 102233.5 <=> cast('10:22:33.5' as time(5));--1
select 102233.5 <=> cast('10:22:33' as time(5));--0
select '10:22:33' <=> cast('10:22:33' as time(5));--0 -- Need to be modified
select '10:22:33.4' <=> cast('10:22:33' as time(5));--0
select '10:22:33.5' <=> cast('10:22:33' as time(5));--0
select '10:22:33.5' <=> cast('10:22:33.5' as time(5));--0 -- Need to be modified
select '10:22:33.5' <=> cast('10:22:33.5' as time(1));--1
select '10:22:33.5' <=> cast('10:22:33.5' as time(2));--0 -- Need to be modified

--+
-- Need to be modified
select cast('22:01:21' as time(5)) +  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:21' as time(5)) +  NULL ;--NULL
select cast('22:01:21' as time(5)) +  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:01' as time(5)) +  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:01' as time(5)) +  cast('2022-02-20' as time(5));--222123.00000 warning
select cast(0 as time(5))  + 44;--44.00000
select cast(0 as time(5))  + 44.88;--44.88000
select 220121.5 + cast('22:01:21' as time(5));--440242.50000
select 220121.4 + cast('22:01:21' as time(5));--440242.40000
select 220121.00 + cast('22:01:21' as time(5));--440242.00000
select 220121 + cast('22:01:21' as time(5));--440242.00000
select 220121.5 + cast('22:01:21.5555' as time(3));--440243.056

--- -- Need to be modified
select cast('22:01:21' as time(5)) -  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:21' as time(5)) -  NULL;--NULL
select cast('22:01:21' as time(5)) -  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:01' as time(5)) -  cast('22:01:88' as time(5));--NULL warning
select cast('22:01:01' as time(5)) -  cast('22:01:01' as time(5));--  0.00000
select cast(0 as time(5))  - 44;-- -44.00000
select cast(0 as time(5))  - 44.88;-- -44.88000
select 220121.5 - cast('22:01:21' as time(5));-- 0.50000
select 220121.4 - cast('22:01:21' as time(5));--0.40000
select 220121.00 - cast('22:01:21' as time(5));--0.00000
select 220121 - cast('22:01:21' as time(5));-- 0.00000
select 220121.5 - cast('22:01:21.3' as time(1));--0.2

--btree and hash index
create table idx_1 (idx time(5));
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('01:01:21'),('11:01:21'),('22:01:21');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx > '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx = '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx <= '11:01:21';
explain (COSTS OFF) select * from idx_1 where idx >= '11:01:21';

create table idx_2 (idx time(5));
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('01:01:21'),('11:01:21'),('22:01:21');
explain (COSTS OFF) select * from idx_2 where idx = '11:01:21';
explain (COSTS OFF) select * from idx_2 where idx > '11:01:21';

drop table idx_1,idx_2;
create table test_date(c1 time(5), c2 time(5));
insert into test_date values('01:01:21','01:01:22'),('22:01:21','22:01:22');
create table test_tmp(c1 time(5), c2 time(5));
insert into test_tmp values('01:02:21','01:02:22'),('22:02:21','22:02:22');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a time(5));
create or replace procedure proc1()
as
$$
declare
i int := 1;
j int := 1;
k int := 1;
begin
    while i <= 20 loop
	j = 1;
		while j <=20 loop 
		k = 1;
			while k <= 59 loop
				insert into tb1 (a) values (i*10000+j*100+k);
				k = k+1;
			end loop;
			j = j+1;
		end loop;
    i = i+1;
    end loop;
end;
$$ language plmysql;
call proc1();
create index idx1 on tb1 using hash(a);
vacuum full analyze tb1;
explain (COSTS OFF) select * from tb1 where a = '22:02:21';

drop index idx1;
create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < '01:01:02';
explain (COSTS OFF) select * from tb1 where a <= '01:01:02';
explain (COSTS OFF) select * from tb1 where a = '01:01:02';
explain (COSTS OFF) select * from tb1 where a != '01:01:02';
explain (COSTS OFF) select * from tb1 where a >= '20:20:58';
explain (COSTS OFF) select * from tb1 where a > '20:20:58';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

create table tb1(id time(5));
create table tb2(id time(5));
insert into tb1 values('22:02:21');
insert into tb2 select id from tb1;
insert into tb2 select id + 100 from tb1;
select * from tb1;
select * from tb2;
drop table tb1,tb2;

select cast( cast('22:02:21.5555555' as time(5)) + 100 as time(5));--22:03:21.55556
select cast( cast(000101.8 as time(5)) + 100 as time(5));--00:02:01.80000

create table tb3(id int);
insert into tb3 values(102233);
insert into tb3 values(102244);
create table tb4 as select cast(id as time(5)) from tb3;
select cast(id as time(5)) from tb3;
create table tb5 as select cast('102255.00' as time(5));
drop table tb3;
drop table tb4;
drop table tb5;

-- *
select cast('222222.5555555' as time(5)) + cast('222222' as time(5)); --444444.55556
select cast('222222' as time(5)) - cast('222220' as time(5)); -- 2.00000
select cast('222222' as time(5)) * cast('222220' as time(5)); --49382172840.0000000000
select cast('222222' as time(5)) * 111; --24666642.00000
select cast('222222' as time(5)) * 111.11; --24691086.4200000
select 111 * cast('222220' as time(5)); -- 24666420.00000 
select 111.11 * cast('222220' as time(5)); --24690864.2000000
select cast('222222' as time(5)) * 111111; -- 24691308642.00000
select cast('222222.1111' as time(2)) * cast('222220.2222' as time(1));--49382241728.622

-- /
select cast('222222.5555555' as time(5)) / cast('222220' as time(5)); --1.000011500
select cast('222222' as time(5)) / 111; --2002.000000000
select cast('222222' as time(5)) / 111.1; -- 2000.198019801 -- Need to be modified:mysql Not supported Round to the nearest whole number
select cast('222222' as time(5)) / 115; -- 1932.365217391
select cast('222222' as time(5)) / 0; -- NULL warning
select 111111 / cast('222220' as time(5)); --0.5000
select 111111.111 / cast('222220' as time(5));--0.5000050
select 0 / cast('222220' as time(5)); --0.0000
select cast('222222.1111' as time(2)) / cast('222220.44' as time(1));--1.000008

-- div
select cast('222222.5555555' as time(5)) div cast('222220' as time(5)); --1
select cast('222222' as time(5)) div 111; --2002
select cast('222222' as time(5)) div 111.1; -- 2000
select cast('222222' as time(5)) div 115; -- 1932
select cast('222222' as time(5)) div 0; -- NULL warning
select 111111 div cast('222220' as time(5)); --0
select 111111.111 div cast('222220' as time(5));--0
select 0 div cast('222220' as time(5)); --0
select cast('222222' as time(5)) div 115.1; --1930
select cast('222222' as time(5)) div 115.2; --1929
select cast('222222.444' as time(2)) div 115.2;--1929

-- %
select cast('111111.5555555' as time(5)) % cast('111110' as time(5)); --1.55556
select cast('111111' as time(5)) % 111; -- 0.00000
select cast('222222' as time(5)) % 111.1; --22.00000
select cast('111111' as time(5)) % 0; --NULL warning
select 111111 % cast('222220' as time(5)); --111111.00000
select 111111.111 % cast('222220' as time(5)); --111111.11100
select 0 % cast('222220' as time(5)); --0.00000
select cast('111111.11' as time(1)) % cast('111110.33' as time(3)); --0.770

-- mod
select cast('111111.5555555' as time(5)) mod cast('111110' as time(5)); --1.55556
select cast('111111' as time(5)) mod 111; --0.00000
select cast('222222' as time(5)) mod 111.1; --22.00000
select cast('111111' as time(5)) mod 0; --NULL warning
select 111111 mod cast('222220' as time(5)); --111111.00000
select 111111.111 mod cast('222220' as time(5)); --111111.11100
select 0 mod cast('222220' as time(5)); --0.00000
select cast('111111' as time(5)) MOD cast('111110' as time(5)); --1.00000
select cast('111111.66' as time(1)) MOD cast('111110.22222' as time(2));--1.48

-- &
select cast('111111.5555555' as time(5)) & cast('111110' as time(5)); --111104
select cast('111111' as time(5)) & 111; --7
select cast('222222' as time(5)) & 111.1; --14
select cast('111111' as time(5)) & 0; --0
select 111111 & cast('222220' as time(5)); --73732
select 111111.111 & cast('222220' as time(5)); --73732
select 0 & cast('222220' as time(5)); --0
select cast('222222' as time(5)) & 1.4; --0
select cast('222222' as time(5)) & 1.5; --2
select cast('222222.444444' as time(5)) & 1.5;--2
select cast('1.1' as time(5)) & 3;--1
select cast('1.9' as time(5)) & 3;--2

-- |
select cast('111111.5555555' as time(5)) | cast('111110' as time(5)); --111118
select cast('111111' as time(5)) | 111; --111215
select cast('222222' as time(5)) | 111.1; --222319
select cast('111111' as time(5)) | 0; --111111
select 111111 | cast('222220' as time(5)); --259599
select 111111.111 | cast('222220' as time(5)); --259599
select 0 | cast('222220' as time(5)); --222220
select cast('222222' as time(5)) | 1.4; --222223
select cast('222222' as time(5)) | 1.5; --222222
select cast('111111.77777' as time(3)) | cast('111110.4444' as time(2)); --111118

-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~cast('111111' as time(5)) ;--18446744073709440504

-- ^
select cast('111111.5555555' as time(5)) ^ cast('111110' as time(5)); --14
select cast('111111' as time(5)) ^ 111; --111208
select cast('222222' as time(5)) ^ 111.1; --222305
select cast('111111' as time(5)) ^ 0; --111111
select 111111 ^ cast('222220' as time(5)); --185867
select 111111.111 ^ cast('222220' as time(5)); --185867
select 0 ^ cast('222220' as time(5)); --222220
select cast('222222' as time(5)) ^ 1.4; --222223
select cast('222222' as time(5)) ^ 1.5; --222220
select cast('111111.7777' as time(3)) ^ cast('111110' as time(5));--14

-- >>
select cast('111111.5555555' as time(5)) >> cast('111110' as time(5)); --0
select cast('111111' as time(5)) >> 111; --0
select cast('222222' as time(5)) >> 111.1; --0
select cast('111111' as time(5)) >> 0; --111111
select 111111 >> cast('222220' as time(5)); --0
select 111111.111 >> cast('222220' as time(5)); --0
select 0 >> cast('222220' as time(5)); --0
select cast('222222' as time(5)) >> 1.4; --111111
select cast('222222' as time(5)) >> 1.5; --55555
select cast('111111.7777' as time(3)) >> 2;--27778

-- <<
select cast('111111.5555555' as time(5)) << cast('111110' as time(5)); --0
select cast('111111' as time(5)) << 111; --0
select cast('222222' as time(5)) << 111.1; --0
select cast('111111' as time(5)) << 0; --111111
select 111111 << cast('222220' as time(5)); --0
select 111111.111 << cast('222220' as time(5)); --0
select 0 << cast('222220' as time(5)); --0
select cast('222222' as time(5)) << 1.4; --444444
select cast('222222' as time(5)) << 1.5; --888888
select cast('111111.7777' as time(3)) << 2;--444448

-- bit_count
select bit_count(cast('111111' as time(5)));  --8
select bit_count(cast('111111' as time(5)));  --8
select bit_count(cast('99991231235959' as time(5)));  --12
select bit_count(cast('111111.4' as time(5)));  --8
select bit_count(cast('111111.5' as time(5)));  --6
select bit_count(cast('111111.5' as time(5))); --6

-- &&
select cast('111111.5555555' as time(5)) && cast('111110' as time(5)); --1
select cast('111111' as time(5)) && 111; --1
select cast('222222' as time(5)) && 111.1; --1
select cast('111111' as time(5)) && 0; --0
select 111111 && cast('222220' as time(5)); --1
select 111111.111 && cast('222220' as time(5)); --1
select 0 && cast('222220' as time(5)); --0
select cast('222222' as time(5)) && 1.4; --1
select cast('222222' as time(5)) && 1.5; --1
select cast('222222.555555' as time(3)) && 1.5;--1

set sql_mode = '';
-- ||
select cast('111111.5555555' as time(5)) || cast('111110' as time(5)); --1
select cast('111111' as time(5)) || 111; --1
select cast('222222' as time(5)) || 111.1; --1
select cast('111111' as time(5)) || 0; --1
select 111111 || cast('222220' as time(5)); --1
select 111111.111 || cast('222220' as time(5)); --1
select 0 || cast('222220' as time(5)); --1
select cast('222222' as time(5)) || 1.4; --1
select cast('222222' as time(5)) || 1.5; --1
select cast('222222.555555' as time(3)) || 1.5;--1
reset sql_mode;
select cast('111111.5555555' as time(5)) || cast('111110' as time(5)); --1

-- !
select !cast('111111' as time(5)) ;--0

-- and or xor
select cast('222222' as time(5)) and cast('222222' as time(5) );--1
select cast('222222' as time(5)) and cast('222222' as time(5) ) and 0::boolean;--0
select cast('222222' as time(5)) or cast('222222' as time(5) );--1
select cast('222222' as time(5)) or cast('222222' as time(5) ) or 0::boolean;--1
select cast('222222' as time(5)) xor cast('222222' as time(5) );--0
select cast('222222' as time(5)) xor cast('222222' as time(5) ) xor 0::boolean;--0
select cast('222222' as time(5)) xor cast('222222' as time(5) ) xor 1::boolean;--1

-- between and
create table t1 (f1 time(5), f2 time(3), f3 time(6));
insert into t1 values('01:03:01','01:01:01.555555','01:11:01.555555');
insert into t1 values('01:03:02','01:01:02.555555','01:11:01.555555');
insert into t1 values('01:03:03','01:01:03.555555','01:11:01.555555');
insert into t1 values('01:03:04','01:01:04.555555','01:11:01.555555');
insert into t1 values('01:03:05','01:01:05.555555','01:11:01.555555');
select f2 from t1 where f2 between '01:01:02' and '01:01:04';
select f1, f2, f3 from t1 where f1 between f2 and f3;
select f2 from t1 where '01:01:05.6666' between f2 and '01:01:05.777777'; 
select f2 from t1 where cast('01:01:05.6666' as time(5)) between f2 and '01:01:05.777777';

drop table t1;

-- in
create table t1(f2 time(5));
insert into t1 values('00:00:00'),('12:34:56'),('11:22:33');
select * from t1 where f2 in ('00:00:00','12:34:56','11:22:33');
select * from t1 where f2 not in ('12:34:56');
drop table t1;

-- is
CREATE TABLE t1 (a time(5), b INT);
INSERT INTO t1 VALUES ('12:34:56',1), ('00:00:00',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 time(5), f2 time(3));
insert into t1 values('01:01:01.555555', '01:01:01.555555');
insert into t1 values('23:59:59.555555', '23:59:59.555555');
insert into t1 values('23:59:59.6', '23:59:59.555555');

select f1 from t1 where f1 like '01:01:01.555555'; 
select f1 from t1 where f1 like cast('01:01:01.555555' as time(5));
select f1 from t1 where f1 like 10101;
select f1 from t1 where f1 like '10101';
select f1 from t1 where f1 like cast('10101' as time(5));
select f1 from t1 where f1 like cast(10101 as time(5));
select f1 from t1 where f1 like cast('10101.555555' as time(5));
select f1 from t1 where f1 like cast('10101.555555' as time(3));

select f2 from t1 where f2 like 10101;
select f2 from t1 where f2 like '10101';
select f2 from t1 where f2 like cast('10101' as time(5));
select f2 from t1 where f2 like cast(10101 as time(5));
drop table t1;



select cast('111111.7777' as time(3)) ^ cast('111110' as time);--14

-- mysql-test

SET sql_mode = 'NO_ENGINE_SUBSTITUTION';
create table t1 (t time);
insert into t1 values("10:22:33"),("12:34:56.78"),(10),(1234),(123456.78),(1234559.99),("1"),("1:23"),("1:23:45"), ("10.22"), ("-10  1:22:33.45"),("20 10:22:33"),("1999-02-03 20:33:34");
insert t1 values (30),(1230),("1230"),("12:30"),("12:30:35"),("1 12:30:31.32");
select * from t1;
insert into t1 values("10.22.22"),(1234567),(123456789),(123456789.10),("10 22:22"),("12.45a");
select * from t1;
drop table t1;

CREATE TABLE t1 (t TIME);
INSERT INTO t1 VALUES (+10), (+10.0), (+10e0);
INSERT INTO t1 VALUES (-10), (-10.0), (-10e0);
SELECT * FROM t1;
DROP TABLE t1;

SELECT CAST(235959.123456 AS TIME);
SELECT CAST(0.235959123456e+6 AS TIME);
SELECT CAST(235959123456e-6 AS TIME);

SELECT CAST(235959.1234567 AS TIME);
SELECT CAST(0.2359591234567e6 AS TIME);

select cast('100:55:50' as time) < cast('24:00:00' as time);
select cast('100:55:50' as time) < cast('024:00:00' as time);
select cast('300:55:50' as time) < cast('240:00:00' as time);
select cast('100:55:50' as time) > cast('24:00:00' as time);
select cast('100:55:50' as time) > cast('024:00:00' as time);
select cast('300:55:50' as time) > cast('240:00:00' as time);

create table t1 (f1 time);
insert into t1 values ('24:00:00');
select cast('24:00:00' as time) = (select f1 from t1);
drop table t1;

create table t1(f1 time, f2 time);
insert into t1 values('20:00:00','150:00:00');
select 1 from t1 where cast('100:00:00' as time) between f1 and f2;
drop table t1;

CREATE TABLE t1 (c TIME);
INSERT INTO t1 VALUES ('0:00:00');
SELECT CAST(c AS TIME) FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (f1 TIME);
INSERT INTO t1 VALUES ('24:00:00');
SELECT      '24:00:00' = (SELECT f1 FROM t1);
SELECT CAST('24:00:00' AS TIME) = (SELECT f1 FROM t1);
SELECT CAST('-24:00:00' AS TIME) = (SELECT f1 FROM t1);
TRUNCATE t1;
INSERT INTO t1 VALUES ('-24:00:00');
SELECT CAST('24:00:00' AS TIME) = (SELECT f1 FROM t1);
SELECT CAST('-24:00:00' AS TIME) = (SELECT f1 FROM t1);
SELECT '-24:00:00' = (SELECT f1 FROM t1);
DROP TABLE t1;

CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES ('20:00:00'),('19:20:30');
SELECT * FROM t1 WHERE a<=>'19:20:30';
SELECT * FROM t1 WHERE a<=>TIME'19:20:30';
SELECT * FROM t1 WHERE a<=>192030;
DROP TABLE t1;

CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES ('10:10:10.9999994'), ('10:10:10.9999995');
INSERT INTO t1 VALUES (101010.9999994), (101010.9999995);
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES ('-10:60:59'), ('-10:59:60'), (-106059), (-105960);
SELECT * FROM t1;
DROP TABLE t1;

CREATE TABLE t1 (a INT);
INSERT INTO t1 VALUES (-106059), (-105960);
SELECT CAST(a AS TIME) FROM t1;
DROP TABLE t1;

--echo #
--echo # Testing CASE with TIME type without a found item
--echo #
SELECT CAST(CASE WHEN 0::boolean THEN '01:01:01' END AS TIME);
SELECT CAST(CASE WHEN 0::boolean THEN TIME'01:01:01' END AS TIME);

--echo #
--echo # Testing TIME field with NULL value with NOT IN
--echo #
CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES (NULL);
SELECT * FROM t1 WHERE a NOT IN (TIME'20:20:20',TIME'10:10:10');
DROP TABLE t1;

--echo #
--echo # Testing Item_func_numhybrid::val_int when TIME type
--echo #
CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES ('10:10:10');
SELECT COALESCE(a,a) FROM t1;
DROP TABLE t1;

--echo #
--echo # Testing Item_func_numhybrid::val_decimal when TIME type
--echo #
CREATE TABLE t1 (a TIME);
INSERT INTO t1 VALUES ('10:10:10');
SELECT CAST(COALESCE(a,a) AS numeric(23,6)) FROM t1;
DROP TABLE t1;

--echo #
--echo # Testing Item_func_numhybrid::get_time  when non-temporal type
--echo #
SELECT CAST(COALESCE(10,20) AS TIME);

--echo #
--echo # Testing Item_func_min_max::get_time with non-temporal arguments
--echo #
SELECT CAST(LEAST(111111,222222) AS TIME);


--echo #
--echo # Item::get_time_from_numeric
--echo #
SELECT CAST(sum(0) AS TIME);
SELECT CAST(sum(0 + 0e0) AS TIME);

SET sql_mode = default;

--Type Conversion
select cast(111111111111 as time );
select cast('111111111111' as time );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as time );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as time );
select cast('11-11-11 11:11:11'::pg_catalog.date as time );

select cast(111111111111 as time(3) );
select cast('111111111111' as time(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as time(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as time(3) );
select cast('11-11-11 11:11:11'::pg_catalog.date as time(3) );


-- mysql type to mysql type
select cast(cast('11-11-11 11:11:11.111111' as date) as time );
select cast(cast('11-11-11 11:11:11.111111' as datetime) as time );
select cast(cast('11-11-11 11:11:11.111111' as datetime(4)) as time );
select cast(cast('11-11-11 11:11:11.111111' as timestamp) as time );
select cast(cast('11-11-11 11:11:11.111111' as timestamp(4)) as time );
select cast(cast('11-11-11 11:11:11.111111' as time) as time );
select cast(cast('11-11-11 11:11:11.111111' as time(4)) as time );

select cast(cast('11-11-11 11:11:11.111111' as date) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as datetime) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as timestamp) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as timestamp(5)) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as time) as time(4) );
select cast(cast('11-11-11 11:11:11.111111' as time(5)) as time(4) );

--select cast(cast('11-11-11 11:11:11.111111' as time) as date);
--select cast(cast('11-11-11 11:11:11.111111' as time) as datetime);
--select cast(cast('11-11-11 11:11:11.111111' as time) as datetime(3));

select cast(cast('11-11-11 11:11:11.111111' as time) as time);
select cast(cast('11-11-11 11:11:11.111111' as time) as time(4));

--select cast(cast('11-11-11 11:11:11.111111' as time(5)) as date);
--select cast(cast('11-11-11 11:11:11.111111' as time(5)) as datetime);
--select cast(cast('11-11-11 11:11:11.111111' as time(5)) as datetime(3));

select cast(cast('11-11-11 11:11:11.111111' as time(5)) as time);
select cast(cast('11-11-11 11:11:11.111111' as time(5)) as time(4));

create table time_01(tm TIME);
set sql_mode ='';
INSERT INTO time_01 values ('-838:59:59.000000');
INSERT INTO time_01 values ('-837:00:00.000000');
INSERT INTO time_01 values ('230:56:59.999999');
INSERT INTO time_01 values ('838:59:59.000000');
INSERT INTO time_01 values ('-839:00:00.000000');
INSERT INTO time_01 values ('839:00:00.000000');
select * from time_01;
drop table time_01;

create table time_01(tm TIME);
set sql_mode =default;
INSERT INTO time_01 values ('-838:59:59.000000');
INSERT INTO time_01 values ('-837:00:00.000000');
INSERT INTO time_01 values ('230:56:59.999999');
INSERT INTO time_01 values ('838:59:59.000000');
INSERT INTO time_01 values ('-839:00:00.000000');
INSERT INTO time_01 values ('839:00:00.000000');
select * from time_01;
drop table time_01;

-- agg
CREATE TABLE t1(m1 time);
INSERT INTO t1 VALUES ('2001-04-10 01:01:01'),('2001-05-09 02:01:01'),('2001-06-10 04:01:01');
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

CREATE TABLE t1(m1 time(5));
INSERT INTO t1 VALUES ('2001-04-10 01:01:01.444444'),('2001-05-09 02:01:01.555555'),('2001-06-10 04:01:01.666666');
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

-- bug#6345
CREATE TABLE tb_test ( a DOUBLE, b INT, c VARCHAR(50), d time, e BINARY(10) );

INSERT INTO tb_test VALUES
(3.14, 3, '3.14', '2001-04-10 01:01:01.444444', 0x03),
(1000.5, 1000, '1000.5', '2001-05-09 02:01:01.555555', 0x04),
(0.001, 0, '0.001', '2001-06-10 04:01:01.666666', 0x00),
(NULL, NULL, NULL, NULL, NULL);

SELECT d, CAST(d AS DOUBLE) , pg_typeof(CAST(d AS DOUBLE))  FROM tb_test;

SELECT d, CAST(d AS float4) , pg_typeof(CAST(d AS float4))  FROM tb_test;
drop TABLE tb_test;

CREATE TABLE tb_test ( a DOUBLE, b INT, c VARCHAR(50), d time(3), e BINARY(10) );

INSERT INTO tb_test VALUES
(3.14, 3, '3.14', '2001-04-10 01:01:01.444444', 0x03),
(1000.5, 1000, '1000.5', '2001-05-09 02:01:01.555555', 0x04),
(0.001, 0, '0.001', '2001-06-10 04:01:01.666666', 0x00),
(NULL, NULL, NULL, NULL, NULL);

SELECT d, CAST(d AS DOUBLE) , pg_typeof(CAST(d AS DOUBLE))  FROM tb_test;

SELECT d, CAST(d AS float4) , pg_typeof(CAST(d AS float4))  FROM tb_test;
drop TABLE tb_test;

-- bug#6779
select convert('23:59:59', time), convert('00:00:00', time);
select cast('23:59:59' as  time), cast('00:00:00' as time);

select cast('1-1-1 00:00:00' as  datetime(3));
select cast('1-1-1 00:00:00' as  datetime);
select cast('00-00-00 00:00:00' as  datetime(3));
select cast('00-00-00 00:00:00' as  datetime);
select cast('00:00:00' as  time(3));
select cast('00:00:00' as  time);

-- bug#6777
SELECT TIME_TO_SEC('abc19:30:10'::time);
SELECT TIME_TO_SEC('%19:30:10'::time);

SELECT TIME_TO_SEC(cast('abc19:30:10' as time));
SELECT TIME_TO_SEC(cast('%19:30:10' as time));
SELECT TIME_TO_SEC(cast('abc19:30:10' as time(3)));
SELECT TIME_TO_SEC(cast('%19:30:10' as time(3)));