set ivorysql.compatible_mode to mysql;
set  datestyle to 'ISO,YMD';

--MySQL date type test case start
select cast('1.1' as date);--NULL
select cast('11.1' as date);--NULL
select cast('1.11' as date);--NULL
select cast('11.11' as date);--NULL
select cast('11.111' as date);--2011-11-01
select cast('99.111' as date);--1999-11-01
select cast('99.311' as date);--NULL
select cast('1.1111' as date);--2001-11-11
select cast('1.111122222' as date);--2001-11-11
select cast('998.3' as date);--1999-08-03
select cast('991.31' as date);--1999-01-31
select cast('999.31' as date);--NULL
select cast('111.111' as date);--2011-01-11
select cast('116.119' as date);--2011-06-11 
select cast('116.419' as date);--NULL
select cast('998.2222' as date);--1998-08-22
select cast('998.2224' as date);--NULL
select cast('998.222333' as date);--1999-08-22
select cast('998.222360' as date);--NULL
select cast('998.22235959' as date);--1999-08-22
select cast('998.222359599999' as date);--1999-08-22
select cast('998.2223595999999999999999' as date);--1999-08-22
select cast('1118.1' as date);--NULL
select cast('1118.12' as date);--NULL
select cast('1118.122' as date);--1118-12-02
select cast('1118.122212' as date);--1118-12-22
select cast('1118.123212' as date);--NULL
select cast('1118.123124' as date);--NULL
select cast('1118.12312323' as date);--1118-12-31
select cast('1118.12312366' as date);--NULL
select cast('1118.1222235659' as date);--1118-12-22
select cast('1118.122223565999999999999999' as date);--1118-12-22

select cast('11118.1' as date);--2011-11-08
select cast('11118.12' as date);--2011-11-08
select cast('11118.32' as date);--NULL
select cast('11118.111111111' as date);--2011-11-08
select cast('11118.116011111' as date);--NULL
select cast('11118.110660111' as date);--NULL
select cast('11118.1106019999999999999999' as date);--2011-11-08

select cast('111121.1' as date);--2011-11-21
select cast('111121.12' as date);--2011-11-21
select cast('111121.24' as date);--NULL
select cast('111121.231' as date);--2011-11-21
select cast('111121.2312' as date);--2011-11-21
select cast('111121.231260' as date);--NULL
select cast('111521.231259' as date);--NULL
select cast('111121.231259' as date);--2011-11-21
select cast('111121.2312599999999999999' as date);--2011-11-21

select cast('1111213.5' as date);--2011-11-21
select cast('1111213.11' as date);--2011-11-21
select cast('1111213.112' as date);--2011-11-21
select cast('1111213.1122' as date);--2011-11-21
select cast('1111213.11222222222' as date);--2011-11-21
select cast('1111213.99' as date);--NULL
select cast('1111213.59' as date);--2011-11-21
select cast('1111213.60' as date);--NULL
select cast('1111213.5960' as date);--NULL
select cast('1111213.5959' as date);--2011-11-21
select cast('1111213.595988888888888' as date);--2011-11-21

select cast('11111121.9' as date);--1111-11-21
select cast('11111121.23' as date);--1111-11-21
select cast('11111121.24' as date);--NULL
select cast('11111121.235555' as date);--1111-11-21
select cast('11111121.245555' as date);--NULL
select cast('11111121.236055' as date);--NULL
select cast('11111121.235560' as date);--NULL
select cast('11111121.235555999999999' as date);--1111-11-21

select cast('111111211.2' as date);--2011-11-11
select cast('111111241.2' as date);--NULL
select cast('111111231.2' as date);--2011-11-11
select cast('111111239.2' as date);--2011--11-11
select cast('111111239.59' as date);--2011--11-11
select cast('111111239.60' as date);--NULL
select cast('111111239.5999999999' as date);--2011-11-11
select cast('1111112111.2' as date);--2011-11-11
select cast('1111112411.2' as date);--NULL
select cast('1111112159.2' as date);--2011-11-11
select cast('1111112160.2' as date);--NULL
select cast('1111112159.59' as date);--2011-11-11
select cast('1111112159.5999999999999' as date);--2011-11-11

select cast('11111121591.2' as date);--2011-11-11
select cast('11111121599.23' as date);--2011-11-11
select cast('11111121599.23444' as date);--2011-11-11
select cast('11111121599.99999999999999999' as date);--2011-11-11
select cast('11111124599.23' as date);--NULL
select cast('11111121609.23444' as date);--NULL
select cast('111111215059.2' as date);--NULL
select cast('111111215060.2' as date);--NULL
select cast('111111215019.23444' as date);--2011-11-11 
select cast('111111215059.23455556' as date);--2011-11-11 
select cast('111111215059.99999999' as date);--2011-11-11 

select cast('1111112150591.9' as date);--2011-11-11
select cast('1111112150601.9' as date);--NULL
select cast('1111112150591.99' as date);--2011-11-11
select cast('1111112150591.9999999999999' as date);--2011-11-11

select cast('11111111201911.9' as date);--1111-11-11
select cast('11111111241911.9' as date);--NULL
select cast('11111111236011.9' as date);--NULL
select cast('11111111201911.9999999999' as date);--1111-11-11

select cast('111111112019111.9' as date);--1111-11-11
select cast('111111111260111.9' as date);--NULL
select cast('11111111201911111111.9' as date);--1111-11-11
select cast('1111111112360111.9' as date);--1111-11-11
select cast('111111112019113333.9999999999' as date);--1111-11-11

select cast('1.1.1' as date);--2001-01-01
select cast('1.1.11' as date);--2001-01-11
select cast('11.11.11' as date);--2011-11-11
select cast('11.13.11' as date);--NULL
select cast('11.13.32' as date);--NULL
select cast('99.11.11' as date);--1999-11-11

select cast('99.111.11' as date);--1999-11-01
select cast('99.111.1111' as date);--1999-11-01
select cast('99.111.3111' as date);--NULL
select cast('99.111.116111' as date);--NULL
select cast('99.111.115961' as date);--NULL
select cast('99.111.115911' as date);--1999-11-01
select cast('99.1111.115911' as date);--1999-11-11
select cast('99.1133.115911' as date);--NULL
select cast('99.1130.115911' as date);--1999-11-30
select cast('99.1311.11' as date);--NULL

select cast('999.1.11' as date);--1999-09-01
select cast('999.11.11' as date);--1999-09-11
select cast('999.30.11' as date);--1999-09-30
select cast('999.34.11' as date);--NULL
select cast('999.30.25' as date);--NULL

select cast('999.111.11' as date);--1999-09-11
select cast('999.239.20' as date);--1999-09-23
select cast('999.319.29' as date);--NULL
select cast('999.3191.29' as date);--NULL
select cast('999.2124.29' as date);--NULL
select cast('999.2123.29' as date);--1999-09-21
select cast('999.2111.59' as date);--1999-09-21
select cast('999.2111.60' as date);--NULL
select cast('999.2111.5959' as date);--1999-09-21 
select cast('999.2111.5960' as date);--NULL
select cast('999.2111.595999999999999' as date);

select cast('999.21112.5959' as date);--1999-09-21  
select cast('999.21112.6059' as date);--NULL
select cast('999.211111.5959' as date);--1999-09-21
select cast('999.212411.5959' as date);--NULL
select cast('999.211160.5959' as date);--NULL

select cast('999.2111111.5959' as date);--1999-09-21 
select cast('999.2111111.999' as date);--1999-09-21 
select cast('999.2111111.9999999' as date);--1999-09-21 
select cast('999.21111111.9999999' as date);--1999-09-21
select cast('999.21111160.9999999' as date);--NULL
select cast('999.21116059.9999999' as date);--NULL

select cast('1999.1.1' as date);
select cast('1999.1.11' as date);
select cast('1999.11.11' as date);
select cast('1999.13.11' as date);--NULL

select cast('1999.111.11' as date);--1999-11-01
select cast('1999.1112.11' as date);--1999-11-12
select cast('1999.1112.145678' as date);--NULL
select cast('1999.1112.14562299999999' as date);--1999-11-12

select cast('1999.11122.19' as date);--1999-11-12
select cast('1999.11122.1959' as date);--1999-11-12
select cast('1999.11122.1960' as date);--NULL
select cast('1999.111212.11' as date);--1999-11-12
select cast('1999.111224.11' as date);--NULL

select cast('1999.1112221.11' as date);--1999-11-12
select cast('1999.11122212.99' as date);--NULL
select cast('1999.11122212.19' as date);--1999-11-12

select cast('1999.111222121.19' as date);--1999-11-12
select cast('1999.1112221212.19' as date);--1999-11-12
select cast('1999.1112221260.19' as date);--NULL
select cast('1999.111222121212999.19' as date);--1999-11-12--warning

select cast('10000.1.1' as date);--2010-00-00
select cast('10001.1.1' as date);--2010-00-01
select cast('11111.1.1' as date);--2011-11-01
select cast('11111.11.11' as date);--2011-11-01
select cast('11111.24.11' as date);--NULL
select cast('199999.23.59' as date);--NULL
select cast('11111.23.11' as date);--2011-11-01
select cast('11111.23.59' as date);--2011-11-01
select cast('11111.23232.99999' as date);--2011-11-01
select cast('11111.232323.99999' as date);--2011-11-01
select cast('11111.23232233.99999' as date);--2011-11-01

select cast('111111.23.9' as date);--2011-11-11
select cast('111111.24.9' as date);--NULL
select cast('111111.221.9' as date);--2011-11-11
select cast('111111.22111111.9' as date);--2011-11-11
select cast('111111.2260111.9999' as date);--NULL
select cast('111111.22111111.9999999999' as date);--2011-11-11

select cast('1111111.3231.9' as date);--2011-11-11
select cast('1111111.6031.9' as date);--NULL
select cast('1111111.5931.9' as date);--2011-11-11
select cast('1111221.59.29' as date);--2011-11-22
select cast('1111221.59599999999.29' as date);--2011-11-22

select cast('11111211.1.29' as date);--1111-12-11
select cast('11111211.1112.29' as date);--1111-12-11
select cast('11111211.1160.29' as date);--NULL
select cast('11111211.1120.69' as date);--NULL
select cast('11111211.1159599999999.29' as date);--1111-12-11
select cast('11111211.111616.99999' as date);--1111-12-11

select cast('111112111.1.29' as date);--2011-11-12
select cast('111112241.1.29' as date);--NULL
select cast('111112231.1.29' as date);--2011-11-12
select cast('111112239.1.29' as date);--2011-11-12
select cast('111112239.60.29' as date);--NULL
select cast('111112239.6.9999' as date);--2011-11-12
select cast('1111122329.6.9' as date);--2011-11-12
select cast('1111122329.6.9999' as date);--2011-11-12
select cast('1111122329.60.9' as date);--NULL
select cast('1111122360.16.9' as date);--NULL
select cast('1111122416.16.9' as date);--NULL
select cast('1111122216.599999.999999999999' as date);--2011-11-12--warning

select cast('11111222161.1.1' as date);--2011-11-12 
select cast('11111222161.9999999999.1' as date);--2011-11-12 
select cast('11111222161.9999999999.9999999999' as date);--2011-11-12 
select cast('111112226011.9.9' as date);--NULL
select cast('111112226011.9999999999.9999999999' as date);--NULL
select cast('111112225911.9.9' as date);--2011-11-12 
select cast('111112222611.9999999999.9999999999' as date);--2011-11-12 
select cast('1111122216119.9999.99999' as date);--2011-11-12
select cast('1111122216111.9999.99999' as date);--2011-11-12 

select cast('11111222161121.9999.99999' as date);--1111-12-22
select cast('111112221611211.9999.99999' as date);--1111-12-22
select cast('1111122216112111111.9999.99999' as date);--1111-12-22--warning

select cast('1.1.1.1' as date);--2001-01-01
select cast('11.11.11.11' as date);--2011-11-11 
select cast('1.1.1.11.11' as date);--2001-01-01
select cast('111.11.11.11' as date);--2011-01-11 
select cast('1111.11.11.11' as date);--1111-11-11
select cast('1111.60.4.11' as date);--NULL
select cast('111111.11.11.11' as date);--2011-11-11
select cast('1111111.111.11.11' as date);--2011-11-11
select cast('11111111.11.4.11' as date);--1111-11-11
select cast('111111111.11.4.11' as date);--2011-11-11
select cast('1111111111.11.4.11' as date);--2011-11-11
select cast('11111111111.11.4.11' as date);--2011-11-11
select cast('111111111111.11.4.11' as date);--2011-11-11
select cast('1111111111111.11.4.11' as date);--2011-11-11
select cast('11111111111111.11.4.11' as date);--1111-11-11
select cast('11111111111111111.11.4.11' as date);--1111-11-11
select cast('11.11.11.23.34.50' as date);--2011-11-11
select cast('9999.12.31.23.59.59.999999.99' as date);--NULL
select cast('99.12.31.23.59.59.999999.99' as date);--2000-01-01


select cast('111/11/11/23/34/50' as date);--0111-11-11
select cast('10000/11/11/23/34/50' as date);--NULL
select cast('1000/13/11/23/34/50' as date);--NULL
select cast('9999/11/60/23/34/50' as date);--NULL
select cast('9999/11/59/24/34/50' as date);--NULL
select cast('9999/11/59/23/60/50' as date);--NULL
select cast('9999/11/59/23/59/60' as date);--NULL
select cast('9999/11/19/23/59/59/99999999999' as date);--9999-11-19

select cast('111.11.11.23.34.50' as date);--2011-01-11
select cast('1111/11.11.23/34/50' as date);--1111-11-11
select cast('111/11.11.23/34/50' as date);--0111-11-11
select cast('111/11.11/23/34/50' as date);--0111-11-11
select cast('111/11.11.24/34/50' as date);--NULL
select cast('111/11.11.23/60/50' as date);--NULL
select cast('111/11.11.23/34/60' as date);--NULL
select cast('111/11.11.23/34/59/9999999' as date);--0111-11-11
select cast('10000/11/11/23/34/50' as date);--NULL

select cast(' ' as date);--NULL
select cast('' as date);--NULL

select cast('2023-03-03 20:01:20AM' as date);--2023-03-03
select cast('20230303200120PM' as date);--NULL
select cast('20230303200120AM' as date);--NULL 
select cast('20230303T200120' as date);--2023-03-03
select cast('2003-03-03T20:01:20' as date);--2023-03-03

--explict conversion 
select date'20100231111';--error 
select date'2010-02-31';--error
select date'2010-02-31 10:22:33';--error
select date'20100231';--error
select date'0000-00-00';--error
select date'11';--error
select date'100';--error
select date'101';--error
select date'111';--error
select date'1111';--error
select date'9999';--error
select date'11111';--2011-11-01
select date'111111';--2011-11-11
select date'1111111';--error
select date'11111111';--1111-11-11
select date'11111111111';--error
select date'111111111111111';--error
select date'11.1';--error
select date'11.11';--error
select date'111.11';--2011-01-11
select date'111.111';--error
select date'111.11111';--error
select date'111.11111111';--error
select date'1111.1';--error
select date'1111.11';--error
select date'1111.111';--1111-11-01
select date'1111.1111';--1111-11-11
select date'1111.11111';--error
select date'1111.111111';--error
select date'1111.1111111';--error
select date'11111.1';--error
select date'11111.11';--error
select date'11111.111';--error
select date'11111.1111';--error
select date'11111.11111';--error
select date'111111.111111';--error
select date'1111111.111111';--error
select date'11111111.111111';--error
select date'111111111.111111';--error
select date'11111111111.111111';--error
select date'11111111111111.111111';--error
select date'1.1.1';--2001-01-01
select date'11.11.11';--2011-11-11
select date'11.11.1111';--error
select date'11.11.11111';--error
select date'11.11.1111111';--error
select date'11.111.1';--error
select date'11.111.11';--error
select date'11.111.111111';--error
select date'11.111.111111111';--error
select date'11.1111.11';--error
select date'11.1111.111111';--error
select date'111.1.1';--error
select date'111.11.1';--error
select date'111.11.11';--error
select date'111.11111.11';--error
select date'111.11.11111';--error
select date'1111.11.1';--1111-11-01
select date'1111.11.11';--1111-11-11
select date'1111.111.11';--error
select date'1111.11.11.11';--error
select date'1111.11.11.11.11';--error
select date'1111.11.11.11.11.11';--error
select date'1111.11.11.11.11.1111111';--error


select cast(1 as date);--NULL 
select cast(11 as date);--NULL
select cast(111 as date);--2000-01-11
select cast(1111 as date);--2000-11-11
select cast(1131 as date);--NULL
select cast(11111 as date);--2001-11-11
select cast(11111.77 as date);--2001-11-11
select cast(111111 as date);--2011-11-11
select cast(111311 as date);--NULL
select cast(111111.88 as date);--2011-11-11
select cast(113111 as date);--NULL
select cast(112160 as date);--NULL
select cast(1111111 as date);--0111-11-11
select cast(1111111.34 as date);--0111-11-11
select cast(1111161 as date);--NULL
select cast(11111111 as date);--1111-11-11
select cast(11111111.56 as date);--1111-11-11
select cast(11113111 as date);--NULL
select cast(11112160 as date);--NULL
select cast(111111111 as date);--2000-01-11
select cast(111111111.99 as date);--2000-01-11
select cast(111351111 as date);--NULL
select cast(1111111111 as date);--2000-11-11
select cast(1111111171 as date);--NULL
select cast(11111116059 as date);--NULL
select cast(11111111111 as date);--2001-11-11
select cast(111111111111 as date);--2011-11-11
select cast(11111111111.33 as date);--2001-11-11
select cast(111111111111.45 as date);--2011-11-11
select cast(111111241156 as date);--NULL
select cast(1111111111111 as date);--0111-11-11
select cast(1111111311105 as date);--NULL
select cast(11111111111111 as date);--1111-11-11
select cast(11111111111111.23 as date);--1111-11-11
select cast(11111111119905 as date);--NULL
select cast(11111111111165 as date);--NULL
select cast(11111111251105 as date);--NULL
select cast(111111111111111 as date);--NULL
select cast(1111111111111111 as date);--NULL
select cast(1111111111111111.44 as date);--NULL
select cast(11111111111233444 as date);--NULL
select cast(111111111111111111111 as date);--NULL
select cast(19830905 as date);--1983-09-05
select cast(119830905 as date);--NULL
select cast(198309051 as date);--NULL
select cast(198205051 as date);--NULL
select cast(128205051 as date);--2000-01-28
select cast(6995 as date);--NULL
select cast(2007-05-23 as date);--NULL
select cast(2007-05*23 as date);--NULL
select cast(830905 as date);--1983-09-05
select cast(9830905 as date);--0983-09-05 
select cast(30905 as date);--2003-09-05
select cast(230905 as date);--2023-09-05
select cast(690905 as date);--2069-09-05
select cast(700905 as date);--1970-09-05
select cast(0905 as date);--2000-09-05
select cast(905 as date);--2000-09-05
select cast(00000905 as date);--2000-09-05
select cast(0000000905 as date);--2000-09-05
select cast(2007*05*23 as date);--2023-08-05
select cast(05*23 as date);--2000-01-15
select cast(06/03 as date);--NULL 
select cast(300/03 as date);--NULL
select cast(30000000/3 as date); --1000-00-00

--round
select cast('2010.01.31.23.59.59.99999999' as date);
select cast('2010.01.31.23.59.59.999999' as date);
select cast('2010.00.31.23.59.59.99999999' as date);
select date'2010.01.31.23.59.59.99999999';
select date'2010.00.31.23.59.59.99999999';

set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt date);
insert into test values('2100-00-04');--ok
insert into test values('0000-00-00');--ok
select cast('2010-02-31' as date);--NULL
select date'2010-02-31';--error
select cast(20100231 as date);--NULL
select * from test;
--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04');--ok
insert into test values('0000-00-00');--warning
select cast('2010-02-31' as date);--NULL
select date'2010-02-31';--error
select cast(20100231 as date);--NULL
select * from test;
--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04');--ok
insert into test values('0000-00-00');--error
select cast('2010-02-31' as date);--NULL
select date'2010-02-31';--error
select cast(20100231 as date);--NULL warning

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04');--ok warning
insert into test values('0000-00-00');--ok
select cast('2010-02-31' as date);--NULL
select date'2010-02-31';--error
select cast(20100231 as date);--NULL warning
--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04');--error
insert into test values('0000-00-00');--ok
select cast('2010-02-31' as date);--NULL warning
select date'2010-02-31';--error
select cast(20100231 as date);--NULL warning


--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04');--ok
insert into test values('0000-00-00');--ok
select cast('2010-02-31' as date);--ok
select date'2010-02-31';--ok
select cast(20100231 as date);--NULL warning
--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04');--ok
insert into test values('0000-00-00');--ok
select cast('2010-02-31' as date);--ok
select date'2010-02-31';--ok
select cast(20100231 as date);--NULL warning

set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
insert into test values('2100-00-04');--error
insert into test values('0000-00-00');--error
select cast('2010-02-31' as date);--NULL
select date'2010-02-31';--error
select cast(20100231 as date);--NULL warning
insert into test values('');--error
insert into test values('99999-04-32');--error
insert into test values('9999-04-32');--error
insert into test values('9999-04-32');--error
insert into test values('9999-02-31');--error
insert into test values('9999-00-31');--error
insert into test values('0000-00-00');--error

--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
delete from test;
insert into test values(0);--error
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
select * from test;

set sql_mode ='';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
select * from test;

set sql_mode ='NO_ZERO_DATE';
delete from test;
insert into test values(0);--0000-00-00--warning
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
select * from test;

set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--error
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
select * from test;

set sql_mode ='NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--0000-00-00--warning
select * from test;

set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--error
select *  from test;

set sql_mode ='ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00--warning
insert into test values(21);--0000-00-00--warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00--warning
insert into test values(22211);--0000-00-00--warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00--warning
insert into test values(2020091423);--0000-00-00--warning
insert into test values(202009142311);--0000-00-00--warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--2023-00-17
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--error
insert into test values(21);--error
insert into test values(222);--2000-02-22
insert into test values(2221);--error
insert into test values(22211);--error
insert into test values(21211);--2002-12-11
insert into test values(20200933);--error
insert into test values(2020091423);--error
insert into test values(202009142311);--error
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
select * from test;

----20231013
set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_DATE';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--2023-00-17
select * from test;

set sql_mode ='ALLOW_INVALID_DATES,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
select * from test;

set sql_mode ='NO_ZERO_DATE,NO_ZERO_IN_DATE,ALLOW_INVALID_DATES';
delete from test;
insert into test values(0);--0000-00-00 --warning
insert into test values(2);--0000-00-00 --warning
insert into test values(21);--0000-00-00 --warning
insert into test values(222);--2000-02-22
insert into test values(2221);--0000-00-00 --warning
insert into test values(22211);--0000-00-00 --warning
insert into test values(21211);--2002-12-11
insert into test values(20200933);--0000-00-00 --warning
insert into test values(2020091423);--0000-00-00 --warning
insert into test values(202009142311);--0000-00-00 --warning
insert into test values(20200914231817);--2020-09-14--warning
insert into test values(20200914231817.66);--2020-09-14 --warning
insert into test values(20200914231817.49999);--2020-09-14 --warning
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14 --warning
insert into test values(190914231817.49999);--2019-09-14 --warning
insert into test values(790914231817.49999);--1979-09-14 --warning
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
select * from test;
drop table test;
set sql_mode to default;

-- oper
---=
select cast('2022-01-21' as date) = cast('2022-01-21' as date);--true 
select 212 = cast('2022-01-32' as date);--NULL--warning
select 212 = cast('2022-01-30' as date);--false
select 20220121.01 = cast('2022-01-21' as date);--false
select NULL = cast('2022-01-21' as date);--NULL
select 20220121.5 = cast('2022-01-21' as date);--false
select 20220121.4 = cast('2022-01-21' as date);--false
select 20220121.00 = cast('2022-01-21' as date);--ture
select 20220121 = cast('2022-01-21' as date);--true
select cast('2022-01-21' as date) = cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) = cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) = 20220121.00; 
select cast('0000-00-00' as date) <=> cast(0 as date);

--<>
select cast('2022-01-21' as date) <> cast('2022-01-21' as date);--0 
select 212 <> cast('2022-01-32' as date);--NULL--warning
select 212 <> cast('2022-01-30' as date);--1
select 20220121.01 <> cast('2022-01-21' as date);--1
select NULL <> cast('2022-01-21' as date);--NULL
select 20220121.5 <> cast('2022-01-21' as date);--1
select 20220121.4 <> cast('2022-01-21' as date);--1
select 20220121.00 <> cast('2022-01-21' as date);--0
select 20220121 <> cast('2022-01-21' as date);--0
select cast('2022-01-21' as date) <> cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) <> cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) <> 20220121.00;--0 
select cast('0000-00-00' as date) <> cast(0 as date);--NULL warning

--<
select cast('2022-01-21' as date) < cast('2022-01-21' as date);--0 
select 212 < cast('2022-01-32' as date);--NULL--warning
select 212 < cast('2022-01-30' as date);--1
select 20220121.01 < cast('2022-01-21' as date);--0
select NULL < cast('2022-01-21' as date);--NULL
select 20220121.5 < cast('2022-01-21' as date);--0
select 20220121.4 < cast('2022-01-21' as date);--0
select 20220121.00 < cast('2022-01-21' as date);--0
select 20220121 < cast('2022-01-21' as date);--0
select cast('2022-01-21' as date) < cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) < cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) < 20220121.00;--0 
select cast('0000-00-00' as date) < cast(0 as date);--NULL warning
select cast('0000-01-02' as date) < cast('0000-00-00' as date);--NULL warning
-->
select cast('2022-01-21' as date) > cast('2022-01-21' as date);--0 
select 212 > cast('2022-01-32' as date);--NULL--warning
select 212 > cast('2022-01-30' as date);--0
select 20220121.01 > cast('2022-01-21' as date);--1
select NULL > cast('2022-01-21' as date);--NULL
select 20220121.5 > cast('2022-01-21' as date);--1
select 20220121.4 > cast('2022-01-21' as date);--1
select 20220121.00 > cast('2022-01-21' as date);--0
select 20220121 > cast('2022-01-21' as date);--0
select cast('2022-01-21' as date) > cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) > cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) > 20220121.00;--0 
select cast('0000-00-00' as date) > cast(0 as date);--NULL warning
select cast('0000-01-02' as date) > cast('0000-00-00' as date);--NULL warning

--<=
select cast('2022-01-21' as date) <= cast('2022-01-21' as date);--1 
select 212 <= cast('2022-01-32' as date);--NULL--warning
select 212 <= cast('2022-01-30' as date);--1
select 20220121.01 <= cast('2022-01-21' as date);--0
select NULL <= cast('2022-01-21' as date);--NULL
select 20220121.5 <= cast('2022-01-21' as date);--0
select 20220121.4 <= cast('2022-01-21' as date);--0
select 20220121.00 <= cast('2022-01-21' as date);--1
select 20220121 <= cast('2022-01-21' as date);--1
select cast('2022-01-21' as date) <= cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) <= cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) <= 20220121.00;--1 
select cast('0000-00-00' as date) <= cast(0 as date);--NULL warning
select cast('0000-01-02' as date) <= cast('0000-00-00' as date);--NULL warning

-->=
select cast('2022-01-21' as date) >= cast('2022-01-21' as date);--1 
select 212 >= cast('2022-01-32' as date);--NULL--warning
select 212 >= cast('2022-01-30' as date);--0
select 20220121.01 >= cast('2022-01-21' as date);--1
select NULL >= cast('2022-01-21' as date);--NULL
select 20220121.5 >= cast('2022-01-21' as date);--1
select 20220121.4 >= cast('2022-01-21' as date);--1
select 20220121.00 >= cast('2022-01-21' as date);--1
select 20220121 >= cast('2022-01-21' as date);--1
select cast('2022-01-21' as date) >= cast('0000-00-00' as date);--NULL warning
select cast('0000-00-00' as date) >= cast('0000-00-00' as date);--NULL warning
select cast('2022-01-21' as date) >= 20220121.00;--1 
select cast('0000-00-00' as date) >= cast(0 as date);--NULL warning
select cast('0000-01-02' as date) >= cast('0000-00-00' as date);--NULL warning

--<=>
select cast('0000-00-00' as date) <=> NULL;--1--warning
select NULL <=> NULL;--1
select cast(0 as date) <=> NULL;--0
select cast('0000-00-00' as date) <=> cast(0 as date);--0 --warning
select cast('0000-00-00' as date) <=> cast('0000-00-00' as date);--1--warning
select cast('2022-01-21' as date) <=> 4; --0
select cast('2022-01-21' as date) <=> 4.4;--0
select cast('2022-01-21' as date) <=> 20220121;--1
select cast('2022-01-21' as date) <=> 20220121.00;--1
select cast('2022-01-21' as date) <=> 20220121.43;--0
select 20220121.00 <=> cast('2022-01-21' as date);--1
--+
select 2.3 +  cast('2022-02-32' as date) ;--NULL warning
select 2 +  cast('2022-02-32' as date) ;--NULL warning
select cast('2022-01-21' as date) +  cast('2022-02-32' as date);--NULL warning
select cast('2022-01-21' as date) +  NULL ;--NULL
select cast('2022-01-21' as date) +  cast('9999-02-30' as date);--NULL warning
select cast('2022-01-01' as date) +  cast('2022-02-30' as date);--NULL warning
select cast('2022-01-01' as date) +  cast('2022-02-20' as date);--40440321
select cast(0 as date)  + 44;--44
select cast(0 as date)  + 44.88;--44.88
select 20220121.5 + cast('2022-01-21' as date);--40440242.5
select 20220121.4 + cast('2022-01-21' as date);--40440242.4
select 20220121.00 + cast('2022-01-21' as date);--40440242.00
select 20220121 + cast('2022-01-21' as date);--40440242
---
select 2.3 -  cast('2022-02-32' as date);--NULL warning
select 2 -  cast('2022-02-32' as date);--NULL warning
select cast('2022-01-21' as date) -  cast('2022-02-32' as date);--NULL warning
select cast('2022-01-21' as date) -  NULL;--NULL
select cast('2022-01-21' as date) -  cast('9999-02-30' as date);--NULL warning
select cast('2022-01-01' as date) -  cast('2022-02-30' as date);--NULL warning
select cast('2022-01-01' as date) -  cast('2022-02-20' as date);---119
select cast(0 as date)  - 44;---44
select cast(0 as date)  - 44.88;---44.88
select 20220121.5 - cast('2022-01-21' as date);--0.5
select 20220121.4 - cast('2022-01-21' as date);--0.4
select 20220121.00 - cast('2022-01-21' as date);--0.00
select 20220121 - cast('2022-01-21' as date);--0

--btree and hash index
create table idx_1 (idx date);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx > '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx = '1022-01-01';
explain (COSTS OFF) select * from idx_1 where idx <= '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx >= '1022-01-01';

create table idx_2 (idx date);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
explain (COSTS OFF) select * from idx_2 where idx = '1522-01-01';
explain (COSTS OFF) select * from idx_2 where idx > '1522-01-01';

drop table idx_1,idx_2;
create table test_date(c1 date, c2 date);
insert into test_date values('2012-01-01','2012-01-02'),('2013-01-01','2013-01-02');
create table test_tmp(c1 date, c2 date);
insert into test_tmp values('2012-01-01','2012-01-02'),('2013-02-01','2013-02-02');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a date);
create or replace procedure proc1()
as
$$
declare
i int := 1000;
j int := 1;
k int := 1;
begin
    while i <= 1200 loop
	j = 1;
		while j <=12 loop 
		k = 1;
			while k <= 28 loop
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
explain (COSTS OFF) select * from tb1 where a = '1004-04-04';

drop index idx1;
create index idx_tree on tb1(a);
vacuum full analyze tb1;
set enable_bitmapscan =false;
set enable_seqscan =false;
explain (COSTS OFF) select * from tb1 where a < '1004-04-04';
explain (COSTS OFF) select * from tb1 where a = '1004-04-04';
explain (COSTS OFF) select * from tb1 where a >= '1004-04-04';
explain (COSTS OFF) select * from tb1 where a > '1104-04-04';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

create table tb1(id date);
create table tb2(id date);
insert into tb1 values('1970-05-02');
insert into tb2 select id from tb1;
insert into tb2 select id + 8800 from tb1;
select * from tb1;
select * from tb2;
drop table tb1,tb2;

select cast((date'1970-02-03' + 8800)as date);--NULL

create table tb3(id int);
insert into tb3 values(19220212);
insert into tb3 values(99999999);
create table tb4 as select id::date from tb3;
select id::date from tb3;
create table tb5 as select '19950399'::date;
drop table tb3;

-- *
select cast('221222222222' as date) + cast('221222222222' as date); --40442444
select cast('221222222222' as date) - cast('221222222220' as date); --0
select cast('221222222222' as date) * cast('221222222220' as date); --408897819173284
select cast('221222222222' as date) * 111; --2244555642
select cast('221222222222' as date) * 111.11; --2246779976.42
select 111 * cast('221222222220' as date); --2244555642
select 111.11 * cast('221222222220' as date); --2246779976.42
select cast('221222222222' as date) * 111111111111; --2246802444442197642

-- /
-- Need to be modified: mysql takes the last 4 digits !!!!!!!
select cast('221222222222' as date) / cast('221222222220' as date); --1.0000
select cast('221222222222' as date) / 111; --182173.1712
select cast('221222222222' as date) / 111.1; -- 182009.1989
select cast('221222222222' as date) / 115; -- 175836.7130
select cast('221222222222' as date) / 0; -- NULL warning
select 111111111111 / cast('221222222220' as date); --5494.7773
select 111111111111.111 / cast('221222222220' as date);--5494.7772746
select 0 / cast('221222222220' as date); --0.0000

-- div
select cast('221222222222' as date) div cast('221222222220' as date); --1
select cast('221222222222' as date) div 111; --182173
select cast('221222222222' as date) div 111.1; -- 182009
select cast('221222222222' as date) div 115; -- 175836
select cast('221222222222' as date) div 0; -- NULL warning
select 111111111111 div cast('221222222220' as date); --5494
select 111111111111.111 div cast('221222222220' as date);--5494
select 0 div cast('221222222220' as date); --0
select cast('221222222222' as date) div 115.1; --175683
select cast('221222222222' as date) div 115.2; --175531

-- %
select cast('111111111111' as date) % cast('111111111110' as date); --0
select cast('111111111111' as date) % 111; --20
select cast('221222222222' as date) % 111.1; --22.1
select cast('111111111111' as date) % 0; --NULL warning
select 111111111111 % cast('221222222220' as date); --15717443
select 111111111111.111 % cast('221222222220' as date); --15717443.111
select 0 % cast('221222222220' as date); --0

-- mod
select cast('111111111111' as date) mod cast('111111111110' as date); --0
select cast('111111111111' as date) mod 111; --20
select cast('221222222222' as date) mod 111.1; --22.1
select cast('111111111111' as date) mod 0; --NULL warning
select 111111111111 mod cast('221222222220' as date); --15717443
select 111111111111.111 mod cast('221222222220' as date); --15717443.111
select 0 mod cast('221222222220' as date); --0
select cast('111111111111' as date) MOD cast('111111111110' as date); --0

-- &
select cast('111111111111' as date) & cast('111111111110' as date); --20111111
select cast('111111111111' as date) & 111; --7
select cast('221222222222' as date) & 111.1; --38
select cast('111111111111' as date) & 0; --0
select 111111111111 & cast('221222222220' as date); --3408134
select 111111111111.111 & cast('221222222220' as date); --3408134
select 0 & cast('221222222220' as date); --0
select cast('221222222222' as date) & 1.4; --0
select cast('221222222222' as date) & 1.5; --2

-- |
select cast('111111111111' as date) | cast('111111111110' as date); --20111111
select cast('111111111111' as date) | 111; --20111215
select cast('221222222222' as date) | 111.1; --20221295
select cast('111111111111' as date) | 0; --20111111
select 111111111111 | cast('221222222220' as date); --111127924199
select 111111111111.111 | cast('221222222220' as date); --111127924199
select 0 | cast('221222222220' as date); --20221222
select cast('221222222222' as date) | 1.4; --20221223
select cast('221222222222' as date) | 1.5; --20221222

-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~cast('111111111111' as date) ;--18446744073689440504

-- ^
select cast('111111111111' as date) ^ cast('111111111110' as date); --0
select cast('111111111111' as date) ^ 111; --20111208
select cast('221222222222' as date) ^ 111.1; --20221257
select cast('111111111111' as date) ^ 0; --20111111
select 111111111111 ^ cast('221222222220' as date); --111124516065
select 111111111111.111 ^ cast('221222222220' as date); --111124516065
select 0 ^ cast('221222222220' as date); --20221222
select cast('221222222222' as date) ^ 1.4; --20221223
select cast('221222222222' as date) ^ 1.5; --20221220

-- >>
select cast('111111111111' as date) >> cast('111111111110' as date); --0
select cast('111111111111' as date) >> 111; --0
select cast('221222222222' as date) >> 111.1; --0
select cast('111111111111' as date) >> 0; --20111111
select 111111111111 >> cast('221222222220' as date); --0
select 111111111111.111 >> cast('221222222220' as date); --0
select 0 >> cast('221222222220' as date); --0
select cast('221222222222' as date) >> 1.4; --10110611
select cast('221222222222' as date) >> 1.5; --5055305

-- <<
select cast('111111111111' as date) << cast('111111111110' as date); --0
select cast('111111111111' as date) << 111; --0
select cast('221222222222' as date) << 111.1; --0
select cast('111111111111' as date) << 0; --20111111
select 111111111111 << cast('221222222220' as date); --0
select 111111111111.111 << cast('221222222220' as date); --0
select 0 << cast('221222222220' as date); --0
select cast('221222222222' as date) << 1.4; --40442444
select cast('221222222222' as date) << 1.5; --80884888

-- bit_count
select bit_count(cast('111111111111' as date));  --14
select bit_count(cast('111111' as date));  --14
select bit_count(cast('99991231235959' as date));  --21
select bit_count(cast('111111111111.4' as date));  --14
select bit_count(cast('111111111111.5' as date));  --14

-- &&
select cast('111111111111' as date) && cast('111111111110' as date); --1
select cast('111111111111' as date) && 111; --1
select cast('221222222222' as date) && 111.1; --1
select cast('111111111111' as date) && 0; --0
select 111111111111 && cast('221222222220' as date); --1
select 111111111111.111 && cast('221222222220' as date); --1
select 0 && cast('221222222220' as date); --0
select cast('221222222222' as date) && 1.4; --1
select cast('221222222222' as date) && 1.5; --1

-- ||
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
select cast('111111111111' as date) || cast('111111111110' as date); --1
select cast('111111111111' as date) || true; --1
select cast('221222222222' as date) || false; --1
select true || cast('221222222220' as date); --1
select false || cast('221222222220' as date); --1

-- !
select !cast('111111111111' as date); --0

-- and or xor
select cast('221222222222' as date) and cast('221222222222' as date );--1
select cast('221222222222' as date) and cast('221222222222' as date ) and 0::boolean;--0

select cast('221222222222' as date) or cast('221222222222' as date );--1
select cast('221222222222' as date) or cast('221222222222' as date ) or 0::boolean;--1

select cast('221222222222' as date) xor cast('221222222222' as date );--0
select cast('221222222222' as date) xor cast('221222222222' as date ) xor 0::boolean;--0
select cast('221222222222' as date) xor cast('221222222222' as date ) xor 1::boolean;--1

-- between and
create table t1 (f1 date, f2 date, f3 date );
insert into t1 values('2001-01-01','2001-01-01','2001-01-01');
insert into t1 values('2001-02-05','2001-02-05','2001-02-05');
insert into t1 values('2001-03-10','2001-03-09','2001-03-10');
insert into t1 values('2001-04-15','2001-04-15','2001-04-15');
insert into t1 values('2001-05-20','2001-05-20','2001-05-20');
select f2 from t1 where f2 between '2001-2-5' and '01-04-14';
select f1, f2, f3 from t1 where f1 between f2 and f3;
select f2 from t1 where '2001-04-10' between f2 and '01-05-01';--Need to be modified: text type conversion bit timestamp in mysql !!!!
--select f2 from t1 where '2001-04-10' between f2 and '01-05-01' --mysql result;
-- f2 
------
--2001-01-01
--2001-02-05
--2001-03-09
--(0 rows)

select f2 from t1 where cast('2001-04-10' as date) between f2 and '01-05-01';
select f2, f3 from t1 where '01-03-10' between f2 and f3;
select f2, f3 from t1 where '01-03-10' not between f2 and f3;
drop table t1;

-- in
create table t1(f2 date);
insert into t1 values('01-01-01'),('01-02-03'),('02-04-06');
select * from t1 where f2 in ('01-01-01','01-02-03','01-02-03');
select * from t1 where f2 not in ('01-01-01','01-02-03','01-02-03');
drop table t1;

-- is
CREATE TABLE t1 (a date, b INT);
INSERT INTO t1 VALUES ('01-02-03',1), ('1999-05-10',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 date);
insert into t1 values('2001-01-01');
insert into t1 values('9999-12-31');
insert into t1 values('9999-12-31');

select f1 from t1 where f1 like '2001-01-01'; --1
select f1 from t1 where f1 like cast('2001-01-01' as date); --1
select f1 from t1 where f1 like 010101010101; --0
select f1 from t1 where f1 like '010101010101'; --0 --Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like 20010101010101; --0
select f1 from t1 where f1 like '20010101010101'; --0 --Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like cast('010101010101' as date); --1
select f1 from t1 where f1 like cast(010101010101 as date); --1
select f1 from t1 where f1 like cast(20010101010101 as date); --1
select f1 from t1 where f1 like cast('20010101.010101' as date); --1
select f1 from t1 where f1 like cast('99991231.2359595' as date);
drop table t1;

--Type Conversion
select cast(111111111111 as date );
select cast('111111111111' as date );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as date );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as date );
select cast('11-11-11 11:11:11'::pg_catalog.date as date );
select cast('2020-08-02 07:30:40'::pg_catalog.date as date );
select cast('2020-08-02 07:30:40'::pg_catalog.timestamp as date );
select cast('2020-08-02 07:30:40'::pg_catalog.timestamptz as date );
select cast('12-10-08 10:10:10' ::pg_catalog.date as date);
select cast('12-10-08 10:10:10' ::pg_catalog.timestamp as date);
select cast('12-10-08 10:10:10' ::pg_catalog.timestamptz as date);

-- mysql type to mysql type
select cast(cast('11-11-11 11:11:11.111111' as date) as date ); 
select cast(cast('11-11-11 11:11:11.111111' as datetime) as date );
select cast(cast('11-11-11 11:11:11.111111' as datetime(4)) as date );
select cast(cast('11-11-11 11:11:11.111111' as timestamp) as date );
select cast(cast('11-11-11 11:11:11.111111' as timestamp(4)) as date );
--select cast(cast('11-11-11 11:11:11.111111' as time) as date );
--select cast(cast('11-11-11 11:11:11.111111' as time(4)) as date );

select cast(cast('11-11-11 11:11:11.111111' as date) as date);
select cast(cast('11-11-11 11:11:11.111111' as date) as datetime);
select cast(cast('11-11-11 11:11:11.111111' as date) as datetime(3));
select cast(cast('11-11-11 11:11:11.111111' as date) as timestamp);
select cast(cast('11-11-11 11:11:11.111111' as date) as timestamp(4));
select cast(cast('11-11-11 11:11:11.111111' as date) as time);
select cast(cast('11-11-11 11:11:11.111111' as date) as time(4));


-- mysql type opertor----Need to be modified
select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as datetime);
select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as timestamp);
--select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as time);
select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as datetime(3));
select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as timestamp(3));
--select cast('11-11-11 11:11:11.111111' as date) + cast('11-11-11 11:11:11.111111' as time(3));

select cast('11-11-11 11:11:11.111111' as datetime) + cast('11-11-11 11:11:11.111111' as timestamp);
--select cast('11-11-11 11:11:11.111111' as datetime) + cast('11-11-11 11:11:11.111111' as time);
select cast('11-11-11 11:11:11.111111' as datetime) + cast('11-11-11 11:11:11.111111' as timestamp(3));
--select cast('11-11-11 11:11:11.111111' as datetime) + cast('11-11-11 11:11:11.111111' as time(3));

select cast('11-11-11 11:11:11.111111' as timestamp) + cast('11-11-11 11:11:11.111111' as time);
select cast('11-11-11 11:11:11.111111' as timestamp) + cast('11-11-11 11:11:11.111111' as datetime(3));
select cast('11-11-11 11:11:11.111111' as timestamp) + cast('11-11-11 11:11:11.111111' as time(3));

--select cast('11-11-11 11:11:11.111111' as time) + cast('11-11-11 11:11:11.111111' as datetime(3));
select cast('11-11-11 11:11:11.111111' as time) + cast('11-11-11 11:11:11.111111' as timestamp(3));
select cast('11-11-11 11:11:11.111111' as time) + cast('11-11-11 11:11:11.111111' as timestamp);


create table date_01(a1 date);
set sql_mode ='';
insert into date_01 values('1111111111111.1');
insert into date_01 values('111111111111111.1');


set sql_mode=default;
insert into date_01 values('1111111111111.1');
insert into date_01 values('111111111111111.1');
select * from date_01;

drop table date_01;

-- agg
CREATE TABLE t1(m1 date);
INSERT INTO t1 VALUES ('2001-04-10'),('2001-05-09'),('2001-06-10');
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

-- bug#6334
create table sales_data (
	id int auto_increment primary key,
	region varchar(50),
	product_category varchar(50),
	salesperson varchar(50),
	amount int,
	sale_date date
);

insert into sales_data (region, product_category, salesperson, amount, sale_date) values
('North', 'Electronics', 'John', 5000, '2023-01-15'),
('North', 'Electronics', 'John', 3000, '2023-02-10'),
('South', 'Clothing', 'Jane', 2000, '2023-01-20'),
('South', 'Clothing', 'Jane', 2500, '2023-02-15'),
('East', 'Electronics', 'Mike', 8000, '2023-01-25'),
('East', 'Home Goods', 'Mike', 1500, '2023-02-20'),
('West', 'Electronics', 'Sarah', 6000, '2023-01-30'),
('West', 'Clothing', 'Sarah', 1800, '2023-02-25'),
('North', 'Home Goods', 'Tom', 1200, '2023-03-01'),
('South', 'Electronics', 'Lisa', 4500, '2023-03-05');

select salesperson,min(sale_date) as first_sale,max(sale_date) as last_sale,count(*) as total_sales
from sales_data
group by salesperson
having datediff(max(sale_date), min(sale_date)) > 20  order by first_sale;

drop table sales_data;

-- bug#6345
CREATE TABLE tb_test ( a DOUBLE, b INT, c VARCHAR(50), d DATE, e BINARY(10) );

INSERT INTO tb_test VALUES
(3.14, 3, '3.14', '2023-01-01', 0x03),
(1000.5, 1000, '1000.5', '2023-12-31', 0x04),
(0.001, 0, '0.001', '2000-01-01', 0x00),
(NULL, NULL, NULL, NULL, NULL);

SELECT d, CAST(d AS DOUBLE) , pg_typeof(CAST(d AS DOUBLE))  FROM tb_test;

SELECT d, CAST(d AS float4) , pg_typeof(CAST(d AS float4))  FROM tb_test;
drop TABLE tb_test;

/* Begin bug#Z8459 */
select cast('2026-06-29' as pg_catalog.date) - interval '14 days' as d_sub_14d;
select cast('2026-06-29' as date) + interval '14 days' as d_add_14d;
select cast('2026-06-29' as date) - interval '1 month' as d_sub_1mon;
select cast('2026-06-29' as date) + interval '1 year'  as d_add_1yr;
select cast('2026-06-29' as date) - interval '2 hours' as d_sub_2h;
select pg_typeof(cast('2026-06-29' as date) - interval '14 days') as result_type;
select (cast('2026-06-29' as date) - interval '14 days')::pg_catalog.timestamp::pg_catalog.date as back_to_date;
select cast(null as date) - interval '14 days' as null_in;
/* End bug#Z8459 */