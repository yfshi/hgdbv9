set ivorysql.compatible_mode to mysql;
set  datestyle to 'ISO,YMD';

--MySQL datetime type test case start
select cast('1.1' as datetime);--NULL
select cast('11.1' as datetime);--NULL
select cast('1.11' as datetime);--NULL
select cast('11.11' as datetime);--NULL
select cast('11.111' as datetime);--2011-11-01 00:00:00 
select cast('99.111' as datetime);--1999-11-01 00:00:00 
select cast('99.311' as datetime);--NULL
select cast('1.1111' as datetime);--2001-11-11 00:00:00
select cast('1.111122222' as datetime);--2001-11-11 22:22:02
select cast('998.3' as datetime);--1999-08-03 00:00:00
select cast('991.31' as datetime);--1999-01-31 00:00:00
select cast('999.31' as datetime);--NULL
select cast('111.111' as datetime);--2011-01-11 01:00:00
select cast('116.119' as datetime);--2011-06-11 09:00:00  
select cast('116.419' as datetime);--NULL
select cast('998.2222' as datetime);--1999-08-22 22:00:00 
select cast('998.2224' as datetime);--NULL
select cast('998.222333' as datetime);--1999-08-22 23:33:00
select cast('998.222360' as datetime);--NULL
select cast('998.22235959' as datetime);--1999-08-22 23:59:59 
select cast('998.222359599999' as datetime);--1999-08-22 23:59:59 
select cast('998.2223595999999999999999' as datetime);--1999-08-22 23:59:59 
select cast('1118.1' as datetime);--NULL
select cast('1118.12' as datetime);--NULL
select cast('1118.122' as datetime);--1118-12-02 00:00:00 
select cast('1118.122212' as datetime);--1118-12-22 12:00:00 
select cast('1118.123212' as datetime);--NULL
select cast('1118.123124' as datetime);--NULL
select cast('1118.12312323' as datetime);--1118-12-31 23:23:00 
select cast('1118.12312366' as datetime);--NULL
select cast('1118.1222235659' as datetime);--1118-12-22 23:56:59 
select cast('1118.122223565999999999999999' as datetime);--1118-12-22 23:56:59  

select cast('11118.1' as datetime);--2011-11-08 01:00:00 
select cast('11118.12' as datetime);--2011-11-08 12:00:00 
select cast('11118.32' as datetime);--NULL
select cast('11118.111111111' as datetime);--2011-11-08 11:11:11
select cast('11118.116011111' as datetime);--NULL
select cast('11118.110660111' as datetime);--NULL
select cast('11118.1106019999999999999999' as datetime);--2011-11-08 11:06:01 

select cast('111121.1' as datetime);--2011-11-21 01:00:00 
select cast('111121.12' as datetime);--2011-11-21 12:00:00 
select cast('111121.24' as datetime);--NULL
select cast('111121.231' as datetime);--2011-11-21 23:01:00 
select cast('111121.2312' as datetime);--2011-11-21 23:12:00
select cast('111121.231260' as datetime);--NULL
select cast('111521.231259' as datetime);--NULL
select cast('111121.231259' as datetime);--2011-11-21 23:12:59
select cast('111121.2312599999999999999' as datetime);--2011-11-21 23:12:59

select cast('1111213.5' as datetime);--2011-11-21 03:05:00
select cast('1111213.11' as datetime);--2011-11-21 03:11:00
select cast('1111213.112' as datetime);--2011-11-21 03:11:02
select cast('1111213.1122' as datetime);--2011-11-21 03:11:22
select cast('1111213.11222222222' as datetime);--2011-11-21 03:11:22
select cast('1111213.99' as datetime);--NULL
select cast('1111213.59' as datetime);--2011-11-21 03:59:00
select cast('1111213.60' as datetime);--NULL
select cast('1111213.5960' as datetime);--NULL
select cast('1111213.5959' as datetime);--2011-11-21 03:59:59
select cast('1111213.595988888888888' as datetime);--2011-11-21 03:59:59

select cast('11111121.9' as datetime);--1111-11-21 09:00:00
select cast('11111121.23' as datetime);--1111-11-21 23:00:00 
select cast('11111121.24' as datetime);--NULL
select cast('11111121.235555' as datetime);--1111-11-21 23:55:55
select cast('11111121.245555' as datetime);--NULL
select cast('11111121.236055' as datetime);--NULL
select cast('11111121.235560' as datetime);--NULL
select cast('11111121.235555999999999' as datetime);--1111-11-21 23:55:55

select cast('111111211.2' as datetime);--2011-11-11 21:01:02 
select cast('111111241.2' as datetime);--NULL
select cast('111111231.2' as datetime);--2011-11-11 23:01:02
select cast('111111239.2' as datetime);--2011-11-11 23:09:02
select cast('111111239.59' as datetime);--2011-11-11 23:09:59
select cast('111111239.60' as datetime);--NULL
select cast('111111239.5999999999' as datetime);--2011-11-11 23:09:59
select cast('1111112111.2' as datetime);--2011-11-11 21:11:02
select cast('1111112411.2' as datetime);--NULL
select cast('1111112159.2' as datetime);--2011-11-11 21:59:02
select cast('1111112160.2' as datetime);--NULL
select cast('1111112159.59' as datetime);--2011-11-11 21:59:59
select cast('1111112159.5999999999999' as datetime);--2011-11-11 21:59:59

select cast('11111121591.2' as datetime);--2011-11-11 21:59:01 
select cast('11111121599.23' as datetime);--2011-11-11 21:59:09
select cast('11111121599.23444' as datetime);--2011-11-11 21:59:09 
select cast('11111121599.99999999999999999' as datetime);--2011-11-11 21:59:10
select cast('11111124599.23' as datetime);--NULL
select cast('11111121609.23444' as datetime);--NULL
select cast('111111215059.2' as datetime);--2011-11-11 21:50:59
select cast('111111215060.2' as datetime);--NULL
select cast('111111215019.23444' as datetime);--2011-11-11 21:50:19 
select cast('111111215059.23455556' as datetime);--2011-11-11 21:50:59 
select cast('111111215059.99999999' as datetime);--2011-11-11 21:51:00  

select cast('1111112150591.9' as datetime);--2011-11-11 21:50:59
select cast('1111112150601.9' as datetime);--NULL
select cast('1111112150591.99' as datetime);--2011-11-11 21:50:59
select cast('1111112150591.9999999999999' as datetime);--2011-11-11 21:50:59

select cast('11111111201911.9' as datetime);--1111-11-11 20:19:12
select cast('11111111241911.9' as datetime);--NULL
select cast('11111111236011.9' as datetime);--NULL
select cast('11111111201911.9999999999' as datetime);-- 1111-11-11 20:19:12

select cast('111111112019111.9' as datetime);--1111-11-11 20:19:11
select cast('111111111260111.9' as datetime);--NULL
select cast('11111111201911111111.9' as datetime);--1111-11-11 20:19:11
select cast('1111111112360111.9' as datetime);--1111-11-11 12:36:01
select cast('111111112019113333.9999999999' as datetime);--1111-11-11 20:19:11

select cast('1.1.1' as datetime);--2001-01-01 00:00:00 
select cast('1.1.11' as datetime);--2001-01-11 00:00:00
select cast('11.11.11' as datetime);--2011-11-11 00:00:00
select cast('11.13.11' as datetime);--NULL
select cast('11.13.32' as datetime);--NULL
select cast('99.11.11' as datetime);--1999-11-11 00:00:00

select cast('99.111.11' as datetime);--1999-11-01 11:00:00
select cast('99.111.1111' as datetime);--1999-11-01 11:11:00
select cast('99.111.3111' as datetime);--NULL
select cast('99.111.116111' as datetime);--NULL
select cast('99.111.115961' as datetime);--NULL
select cast('99.111.115911' as datetime);--1999-11-01 11:59:11
select cast('99.1111.115911' as datetime);--1999-11-11 11:59:11
select cast('99.1133.115911' as datetime);--NULL
select cast('99.1130.115911' as datetime);--1999-11-30 11:59:11
select cast('99.1311.11' as datetime);--NULL

select cast('999.1.11' as datetime);--1999-09-01 11:00:00
select cast('999.11.11' as datetime);--1999-09-11 11:00:00
select cast('999.30.11' as datetime);--1999-09-30 11:00:00
select cast('999.34.11' as datetime);--NULL
select cast('999.30.25' as datetime);--NULL

select cast('999.111.11' as datetime);--1999-09-11 01:11:00
select cast('999.239.20' as datetime);--1999-09-23 09:20:00
select cast('999.319.29' as datetime);--NULL
select cast('999.3191.29' as datetime);--NULL
select cast('999.2124.29' as datetime);--NULL
select cast('999.2123.29' as datetime);-- 1999-09-21 23:29:00 
select cast('999.2111.59' as datetime);--1999-09-21 11:59:00
select cast('999.2111.60' as datetime);--NULL
select cast('999.2111.5959' as datetime);--1999-09-21 11:59:59
select cast('999.2111.5960' as datetime);--NULL
select cast('999.2111.595999999999999' as datetime);--1999-09-21 11:59:59

select cast('999.21112.5959' as datetime);--1999-09-21 11:02:59 
select cast('999.21112.6059' as datetime);--NULL
select cast('999.211111.5959' as datetime);--1999-09-21 11:11:59 
select cast('999.212411.5959' as datetime);--NULL
select cast('999.211160.5959' as datetime);--NULL

select cast('999.2111111.5959' as datetime);--1999-09-21 11:11:02 
select cast('999.2111111.999' as datetime);--1999-09-21 11:11:02 
select cast('999.2111111.9999999' as datetime);--1999-09-21 11:11:02 
select cast('999.21111111.9999999' as datetime);--1999-09-21 11:11:12
select cast('999.21111160.9999999' as datetime);--NULL
select cast('999.21116059.9999999' as datetime);--NULL

select cast('1999.1.1' as datetime);-- 1999-01-01 00:00:00
select cast('1999.1.11' as datetime);--1999-01-11 00:00:00
select cast('1999.11.11' as datetime);--1999-11-11 00:00:00
select cast('1999.13.11' as datetime);--NULL

select cast('1999.111.11' as datetime);--1999-11-01 11:00:00
select cast('1999.1112.11' as datetime);--1999-11-12 11:00:00 
select cast('1999.1112.145678' as datetime);--NULL
select cast('1999.1112.14562299999999' as datetime);--1999-11-12 14:56:22

select cast('1999.11122.19' as datetime);-- 1999-11-12 02:19:00
select cast('1999.11122.1959' as datetime);--1999-11-12 02:19:59
select cast('1999.11122.1960' as datetime);--NULL
select cast('1999.111212.11' as datetime);--1999-11-12 12:11:00
select cast('1999.111224.11' as datetime);--NULL

select cast('1999.1112221.11' as datetime);--1999-11-12 22:01:11
select cast('1999.11122212.99' as datetime);--NULL
select cast('1999.11122212.19' as datetime);--1999-11-12 22:12:19 

select cast('1999.111222121.19' as datetime);--1999-11-12 22:12:01 
select cast('1999.1112221212.19' as datetime);--1999-11-12 22:12:12
select cast('1999.1112221260.19' as datetime);--NULL
select cast('1999.111222121212999.19' as datetime);--1999-11-12 22:12:12--warning

select cast('10000.1.1' as datetime);--2010-00-00 01:01:00
select cast('10001.1.1' as datetime);--2010-00-01 01:01:00 
select cast('11111.1.1' as datetime);--2011-11-01 01:01:00
select cast('11111.11.11' as datetime);--2011-11-01 11:11:00
select cast('11111.24.11' as datetime);--NULL
select cast('199999.23.59' as datetime);--NULL
select cast('11111.23.11' as datetime);--2011-11-01 23:11:00
select cast('11111.23.59' as datetime);--2011-11-01 23:59:00 
select cast('11111.23232.99999' as datetime);--2011-11-01 23:23:03
select cast('11111.232323.99999' as datetime);--2011-11-01 23:23:24
select cast('11111.23232233.99999' as datetime);--2011-11-01 23:23:22

select cast('111111.23.9' as datetime);--2011-11-11 23:09:00
select cast('111111.24.9' as datetime);--NULL
select cast('111111.221.9' as datetime);--2011-11-11 22:01:09
select cast('111111.22111111.9' as datetime);--2011-11-11 22:11:11
select cast('111111.2260111.9999' as datetime);--NULL
select cast('111111.22111111.9999999999' as datetime);--2011-11-11 22:11:11

select cast('1111111.3231.9' as datetime);--2011-11-11 01:32:32
select cast('1111111.6031.9' as datetime);--NULL
select cast('1111111.5931.9' as datetime);--2011-11-11 01:59:32 
select cast('1111221.59.29' as datetime);--2011-11-22 01:59:29 
select cast('1111221.59599999999.29' as datetime);--2011-11-22 01:59:59

select cast('11111211.1.29' as datetime);--1111-12-11 01:29:00
select cast('11111211.1112.29' as datetime);--1111-12-11 11:12:29 
select cast('11111211.1160.29' as datetime);--NULL
select cast('11111211.1120.69' as datetime);--NULL
select cast('11111211.1159599999999.29' as datetime);--1111-12-11 11:59:59
select cast('11111211.111616.99999' as datetime);--1111-12-11 11:16:17

select cast('111112111.1.29' as datetime);--2011-11-12 11:01:01
select cast('111112241.1.29' as datetime);--NULL
select cast('111112231.1.29' as datetime);--2011-11-12 23:01:01 
select cast('111112239.1.29' as datetime);-- 2011-11-12 23:09:01
select cast('111112239.60.29' as datetime);--NULL
select cast('111112239.6.9999' as datetime);--2011-11-12 23:09:07
select cast('1111122329.6.9' as datetime);--2011-11-12 23:29:07
select cast('1111122329.6.9999' as datetime);--2011-11-12 23:29:07
select cast('1111122329.60.9' as datetime);--NULL
select cast('1111122360.16.9' as datetime);--NULL
select cast('1111122416.16.9' as datetime);--NULL
select cast('1111122216.599999.999999999999' as datetime);--2011-11-12 22:16:59--warning

select cast('11111222161.1.1' as datetime);--2011-11-12 22:16:01 
select cast('11111222161.9999999999.1' as datetime);--2011-11-12 22:16:02 
select cast('11111222161.9999999999.9999999999' as datetime);--2011-11-12 22:16:02 
select cast('111112226011.9.9' as datetime);--NULL
select cast('111112226011.9999999999.9999999999' as datetime);--NULL
select cast('111112225911.9.9' as datetime);--2011-11-12 22:59:12 
select cast('111112222611.9999999999.9999999999' as datetime);--2011-11-12 22:26:12 
select cast('1111122216119.9999.99999' as datetime);--2011-11-12 22:16:11
select cast('1111122216111.9999.99999' as datetime);--2011-11-12 22:16:11  

select cast('11111222161121.9999.99999' as datetime);--1111-12-22 16:11:22
select cast('111112221611211.9999.99999' as datetime);--1111-12-22 16:11:21
select cast('1111122216112111111.9999.99999' as datetime);--1111-12-22 16:11:21 --warning

select cast('1.1.1.1' as datetime);--2001-01-01 01:00:00
select cast('11.11.11.11' as datetime);--2011-11-11 11:00:00 
select cast('1.1.1.11.11' as datetime);--2001-01-01 11:11:00
select cast('111.11.11.11' as datetime);--2011-01-11 11:11:00 
select cast('1111.11.11.11' as datetime);--1111-11-11 11:00:00
select cast('1111.60.4.11' as datetime);--NULL
select cast('111111.11.11.11' as datetime);--2011-11-11 11:11:11
select cast('1111111.111.11.11' as datetime);--2011-11-11 01:11:01
select cast('11111111.11.4.11' as datetime);--1111-11-11 11:04:11
select cast('111111111.11.4.11' as datetime);--2011-11-11 11:01:11
select cast('1111111111.11.4.11' as datetime);--2011-11-11 11:11:11
select cast('11111111111.11.4.11' as datetime);-- 2011-11-11 11:11:01
select cast('111111111111.11.4.11' as datetime);--2011-11-11 11:11:11
select cast('1111111111111.11.4.11' as datetime);--2011-11-11 11:11:11 
select cast('11111111111111.11.4.11' as datetime);--1111-11-11 11:11:11 
select cast('11111111111111111.11.4.11' as datetime);--1111-11-11 11:11:11
select cast('11.11.11.23.34.50' as datetime);--2011-11-11 23:34:50 
select cast('9999.12.31.23.59.59.999999.99' as datetime);--NULL
select cast('99.12.31.23.59.59.999999.99' as datetime);--2000-01-01 00:00:00


select cast('111/11/11/23/34/50' as datetime);--0111-11-11 23:34:50
select cast('10000/11/11/23/34/50' as datetime);--NULL
select cast('1000/13/11/23/34/50' as datetime);--NULL
select cast('9999/11/60/23/34/50' as datetime);--NULL
select cast('9999/11/59/24/34/50' as datetime);--NULL
select cast('9999/11/59/23/60/50' as datetime);--NULL
select cast('9999/11/59/23/59/60' as datetime);--NULL
select cast('9999/11/19/23/59/59/99999999999' as datetime);--9999-11-19 23:59:59

select cast('111.11.11.23.34.50' as datetime);--2011-01-11 11:23:35
select cast('1111/11.11.23/34/50' as datetime);--1111-11-11 23:34:50
select cast('111/11.11.23/34/50' as datetime);--0111-11-11 23:34:50
select cast('111/11.11/23/34/50' as datetime);--0111-11-11 23:34:50
select cast('111/11.11.24/34/50' as datetime);--NULL
select cast('111/11.11.23/60/50' as datetime);--NULL
select cast('111/11.11.23/34/60' as datetime);--NULL
select cast('111/11.11.23/34/59/9999999' as datetime);--0111-11-11 23:34:59
select cast('10000/11/11/23/34/50' as datetime);--NULL

select cast(' ' as datetime);--NULL
select cast('' as datetime);--NULL

select cast('2023-03-03 20:01:20AM' as datetime);--2023-03-03 20:01:20 
select cast('20230303200120PM' as datetime);--NULL
select cast('20230303200120AM' as datetime);--NULL 
select cast('20230303T200120' as datetime);--2023-03-03 20:01:20
select cast('2003-03-03T20:01:20' as datetime);--2003-03-03 20:01:20

--explict conversion 
select datetime'20100231111';--error 
select datetime'2010-02-31';--error
select datetime'2010-02-31 10:22:33';--error
select datetime'20100231';--error
select datetime'0000-00-00';--error
select datetime'11';--error
select datetime'100';--error
select datetime'101';--error
select datetime'111';--error
select datetime'1111';--error
select datetime'9999';--error
select datetime'11111';--error
select datetime'111111';--error
select datetime'1111111';--error
select datetime'11111111';--error
select datetime'11111111111';--error
select datetime'111111111111111';--error
select datetime'11.1';--error
select datetime'11.11';--error
select datetime'111.11';--error
select datetime'111.111';--error
select datetime'111.11111';--error
select datetime'111.11111111';--error
select datetime'1111.1';--error
select datetime'1111.11';--error
select datetime'1111.111';--error
select datetime'1111.1111';--error
select datetime'1111.11111';--error
select datetime'1111.111111';--error
select datetime'1111.1111111';--error
select datetime'11111.1';--error
select datetime'11111.11';--error
select datetime'11111.111';--error
select datetime'11111.1111';--error
select datetime'11111.11111';--error
select datetime'111111.111111';--error
select datetime'1111111.111111';--error
select datetime'11111111.111111';--error
select datetime'111111111.111111';--error
select datetime'11111111111.111111';--error
select datetime'11111111111111.111111';--error
select datetime'1.1.1';--error
select datetime'11.11.11';--error
select datetime'11.11.1111';--error
select datetime'11.11.11111';--error
select datetime'11.11.1111111';--error
select datetime'11.111.1';--error
select datetime'11.111.11';--error
select datetime'11.111.111111';--error
select datetime'11.111.111111111';--error
select datetime'11.1111.11';--error
select datetime'11.1111.111111';--error
select datetime'111.1.1';--error
select datetime'111.11.1';--error
select datetime'111.11.11';--error
select datetime'111.11111.11';--error
select datetime'111.11.11111';--error
select datetime'1111.11.1';--error
select datetime'1111.11.11';--error
select datetime'1111.111.11';--error
select datetime'1111.11.11.11';--error
select datetime'1111.11.11.11.11';--error
select datetime'1111.11.11.11.11.11';--error
select datetime'1111.11.11.11.11.1111111';--error


select cast(1 as datetime);--NULL 
select cast(11 as datetime);--NULL
select cast(111 as datetime);--2000-01-11 00:00:00
select cast(1111 as datetime);--2000-11-11 00:00:00
select cast(1131 as datetime);--NULL
select cast(11111 as datetime);--2001-11-11 00:00:00
select cast(11111.77 as datetime);--2001-11-11 00:00:00
select cast(111111 as datetime);--2011-11-11 00:00:00
select cast(111311 as datetime);--NULL
select cast(111111.88 as datetime);--2011-11-11 00:00:00
select cast(113111 as datetime);--NULL
select cast(112160 as datetime);--NULL
select cast(1111111 as datetime);--0111-11-11 00:00:00
select cast(1111111.34 as datetime);--0111-11-11 00:00:00
select cast(1111161 as datetime);--NULL
select cast(11111111 as datetime);--1111-11-11 00:00:00
select cast(11111111.56 as datetime);--1111-11-11 00:00:00
select cast(11113111 as datetime);--NULL
select cast(11112160 as datetime);--NULL
select cast(111111111 as datetime);--2000-01-11 11:11:11
select cast(111111111.99 as datetime);--2000-01-11 11:11:12
select cast(111351111 as datetime);--NULL
select cast(1111111111 as datetime);--2000-11-11 11:11:11 
select cast(1111111171 as datetime);--NULL
select cast(11111116059 as datetime);--NULL
select cast(11111111111 as datetime);-- 2001-11-11 11:11:11 
select cast(111111111111 as datetime);--2011-11-11 11:11:11
select cast(11111111111.33 as datetime);--2001-11-11 11:11:11
select cast(111111111111.45 as datetime);--2011-11-11 11:11:11
select cast(111111241156 as datetime);--NULL
select cast(1111111111111 as datetime);--0111-11-11 11:11:11
select cast(1111111311105 as datetime);--NULL
select cast(11111111111111 as datetime);--1111-11-11 11:11:11
select cast(11111111111111.23 as datetime);--1111-11-11 11:11:11
select cast(11111111119905 as datetime);--NULL
select cast(11111111111165 as datetime);--NULL
select cast(11111111251105 as datetime);--NULL
select cast(111111111111111 as datetime);--NULL
select cast(1111111111111111 as datetime);--NULL
select cast(1111111111111111.44 as datetime);--NULL
select cast(11111111111233444 as datetime);--NULL
select cast(111111111111111111111 as datetime);--NULL
select cast(19830905 as datetime);--1983-09-05 00:00:00
select cast(119830905 as datetime);--NULL
select cast(198309051 as datetime);--NULL
select cast(198205051 as datetime);--NULL
select cast(128205051 as datetime);--2000-01-28 20:50:51
select cast(6995 as datetime);--NULL
select cast(2007-05-23 as datetime);--NULL
select cast(2007-05*23 as datetime);--NULL
select cast(830905 as datetime);--1983-09-05 00:00:00
select cast(9830905 as datetime);--0983-09-05 00:00:00 
select cast(30905 as datetime);--2003-09-05 00:00:00
select cast(230905 as datetime);--2023-09-05 00:00:00
select cast(690905 as datetime);--2069-09-05 00:00:00
select cast(700905 as datetime);--1970-09-05 00:00:00
select cast(0905 as datetime);--2000-09-05 00:00:00
select cast(905 as datetime);--2000-09-05 00:00:00
select cast(00000905 as datetime);--2000-09-05 00:00:00
select cast(0000000905 as datetime);-- 2000-09-05 00:00:00
select cast(2007*05*23 as datetime);--2023-08-05 00:00:00
select cast(05*23 as datetime);--2000-01-15 00:00:00
select cast(06/03 as datetime);--NULL 
select cast(300/03 as datetime);--NULL
select cast(30000000/3 as datetime); --1000-00-00 00:00:00

--round
select cast('2010.01.31.23.59.59.99999999' as datetime);--2010-02-01 00:00:00
select cast('2010.01.31.23.59.59.999999' as datetime);--2010-02-01 00:00:00
select cast('2010.00.31.23.59.59.99999999' as datetime);--NULL

--set sql_mode
set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt datetime);
insert into test values('2100-00-04 12:13:14');--ok
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--2 warning
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--warning
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--2 warning
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ERROR
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--2 warning
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ERROR
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ERROR
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--2 warning
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--warning
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ERROR
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL
select * from test;
delete from test;

set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
insert into test values('2100-00-04 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
select cast('2010-02-31 00:00:00' as datetime);--NULL
select cast(20100231121314 as datetime);--NULL warning
select cast('9999-12-31 23:59:59.44' as datetime);--9999-12-31 23:59:59
select cast('9999-12-31 23:59:59.99' as datetime);--NULL
select cast(99991231235959 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.4 as datetime);--9999-12-31 23:59:59
select cast(99991231235959.5 as datetime);--NULL


insert into test values('');--error
insert into test values('99999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-02-31 12:13:14');--ERROR
insert into test values('9999-00-31 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR

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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--error
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14 
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14 
insert into test values(20200914231817.49999);--2020-09-14 
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
select * from test;

drop table test;
set sql_mode to default;
--

-- oper
---=
select cast('2022-01-21' as datetime) = cast('2022-01-21' as datetime);--true 
select 212 = cast('2022-01-32' as datetime);--NULL--warning
select 212 = cast('2022-01-30' as datetime);--false
select 20220121.01 = cast('2022-01-21' as datetime);--false
select NULL = cast('2022-01-21' as datetime);--NULL
select 20220121.5 = cast('2022-01-21' as datetime);--false
select 20220121.4 = cast('2022-01-21' as datetime);--false
select 20220121.00 = cast('2022-01-21' as datetime);--ture
select 20220121 = cast('2022-01-21' as datetime);--true
select cast('2022-01-21' as datetime) = cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) = cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) = 20220121.00; 
select cast('0000-00-00' as datetime) <=> cast(0 as datetime);
select cast(20220121102233.4 as datetime) = cast('2022-01-21 10:22:33' as datetime);--1
select cast(20220121102233.5 as datetime) = cast('2022-01-21 10:22:33.5' as datetime);--1
select cast(20220121102233.5 as datetime) = cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233 = cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.4 = cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.5 = cast('2022-01-21 10:22:33.5' as datetime);--0
select 20220121102233.5 = cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33' = cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.4' = cast('2022-01-21 10:22:33' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
select '2022-01-21 10:22:33.5' = cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.5' = cast('2022-01-21 10:22:33.5' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
--<>
select cast('2022-01-21' as datetime) <> cast('2022-01-21' as datetime);--0 
select 212 <> cast('2022-01-32' as datetime);--NULL--warning
select 212 <> cast('2022-01-30' as datetime);--1
select 20220121.01 <> cast('2022-01-21' as datetime);--1
select NULL <> cast('2022-01-21' as datetime);--NULL
select 20220121.5 <> cast('2022-01-21' as datetime);--1
select 20220121.4 <> cast('2022-01-21' as datetime);--1
select 20220121.00 <> cast('2022-01-21' as datetime);--0
select 20220121 <> cast('2022-01-21' as datetime);--0
select cast('2022-01-21' as datetime) <> cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) <> cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) <> 20220121.00;--0 
select cast('0000-00-00' as datetime) <> cast(0 as datetime);--NULL warning
select cast(20220121102233.4 as datetime) <> cast('2022-01-21 10:22:33' as datetime);--0
select cast(20220121102233.5 as datetime) <> cast('2022-01-21 10:22:33.5' as datetime);--0
select cast(20220121102233.5 as datetime) <> cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233 <> cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.4 <> cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.5 <> cast('2022-01-21 10:22:33.5' as datetime);--1
select 20220121102233.5 <> cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33' <> cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.4' <> cast('2022-01-21 10:22:33' as datetime);--1--Need to be modified: text type conversion bit timestamp in mysql !!!!
select '2022-01-21 10:22:33.5' <> cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.5' <> cast('2022-01-21 10:22:33.5' as datetime);--1--Need to be modified: text type conversion bit timestamp in mysql !!!!
--<
select cast('2022-01-21' as datetime) < cast('2022-01-21' as datetime);--0 
select 212 < cast('2022-01-32' as datetime);--NULL--warning
select 212 < cast('2022-01-30' as datetime);--1
select 20220121.01 < cast('2022-01-21' as datetime);--0
select NULL < cast('2022-01-21' as datetime);--NULL
select 20220121.5 < cast('2022-01-21' as datetime);--0
select 20220121.4 < cast('2022-01-21' as datetime);--0
select 20220121.00 < cast('2022-01-21' as datetime);--0
select 20220121 < cast('2022-01-21' as datetime);--0
select cast('2022-01-21' as datetime) < cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) < cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) < 20220121.00;--0 
select cast('0000-00-00' as datetime) < cast(0 as datetime);--NULL warning
select cast('0000-01-02' as datetime) < cast('0000-00-00' as datetime);--NULL warning
select cast(20220121102233.4 as datetime) < cast('2022-01-21 10:22:33' as datetime);--0
select cast(20220121102233.5 as datetime) < cast('2022-01-21 10:22:33.5' as datetime);--0
select cast(20220121102233.5 as datetime) < cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233 < cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.4 < cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.5 < cast('2022-01-21 10:22:33.5' as datetime);--1
select 20220121102233.5 < cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33' < cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.4' < cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.5' < cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.5' < cast('2022-01-21 10:22:33.5' as datetime);--1--Need to be modified: text type conversion bit timestamp in mysql !!!!
-->
select cast('2022-01-21' as datetime) > cast('2022-01-21' as datetime);--0 
select 212 > cast('2022-01-32' as datetime);--NULL--warning
select 212 > cast('2022-01-30' as datetime);--0
select 20220121.01 > cast('2022-01-21' as datetime);--1
select NULL > cast('2022-01-21' as datetime);--NULL
select 20220121.5 > cast('2022-01-21' as datetime);--1
select 20220121.4 > cast('2022-01-21' as datetime);--1
select 20220121.00 > cast('2022-01-21' as datetime);--0
select 20220121 > cast('2022-01-21' as datetime);--0
select cast('2022-01-21' as datetime) > cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) > cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) > 20220121.00;--0 
select cast('0000-00-00' as datetime) > cast(0 as datetime);--NULL warning
select cast('0000-01-02' as datetime) > cast('0000-00-00' as datetime);--NULL warning
select cast(20220121102233.4 as datetime) > cast('2022-01-21 10:22:33' as datetime);--0
select cast(20220121102233.5 as datetime) > cast('2022-01-21 10:22:33.5' as datetime);--0
select cast(20220121102233.5 as datetime) > cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233 > cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.4 > cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.5 > cast('2022-01-21 10:22:33.5' as datetime);--0
select 20220121102233.5 > cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33' > cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.4' > cast('2022-01-21 10:22:33' as datetime);--1--Need to be modified: text type conversion bit timestamp in mysql !!!!
select '2022-01-21 10:22:33.5' > cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.5' > cast('2022-01-21 10:22:33.5' as datetime);--0
--<=
select cast('2022-01-21' as datetime) <= cast('2022-01-21' as datetime);--1 
select 212 <= cast('2022-01-32' as datetime);--NULL--warning
select 212 <= cast('2022-01-30' as datetime);--1
select 20220121.01 <= cast('2022-01-21' as datetime);--0
select NULL <= cast('2022-01-21' as datetime);--NULL
select 20220121.5 <= cast('2022-01-21' as datetime);--0
select 20220121.4 <= cast('2022-01-21' as datetime);--0
select 20220121.00 <= cast('2022-01-21' as datetime);--1
select 20220121 <= cast('2022-01-21' as datetime);--1
select cast('2022-01-21' as datetime) <= cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) <= cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) <= 20220121.00;--1 
select cast('0000-00-00' as datetime) <= cast(0 as datetime);--NULL warning
select cast('0000-01-02' as datetime) <= cast('0000-00-00' as datetime);--NULL warning
select cast(20220121102233.4 as datetime) <= cast('2022-01-21 10:22:33' as datetime);--1
select cast(20220121102233.5 as datetime) <= cast('2022-01-21 10:22:33.5' as datetime);--1
select cast(20220121102233.5 as datetime) <= cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233 <= cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.4 <= cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.5 <= cast('2022-01-21 10:22:33.5' as datetime);--1
select 20220121102233.5 <= cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33' <= cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.4' <= cast('2022-01-21 10:22:33' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
select '2022-01-21 10:22:33.5' <= cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.5' <= cast('2022-01-21 10:22:33.5' as datetime);--1
-->=
select cast('2022-01-21' as datetime) >= cast('2022-01-21' as datetime);--1 
select 212 >= cast('2022-01-32' as datetime);--NULL--warning
select 212 >= cast('2022-01-30' as datetime);--0
select 20220121.01 >= cast('2022-01-21' as datetime);--1
select NULL >= cast('2022-01-21' as datetime);--NULL
select 20220121.5 >= cast('2022-01-21' as datetime);--1
select 20220121.4 >= cast('2022-01-21' as datetime);--1
select 20220121.00 >= cast('2022-01-21' as datetime);--1
select 20220121 >= cast('2022-01-21' as datetime);--1
select cast('2022-01-21' as datetime) >= cast('0000-00-00' as datetime);--NULL warning
select cast('0000-00-00' as datetime) >= cast('0000-00-00' as datetime);--NULL warning
select cast('2022-01-21' as datetime) >= 20220121.00;--1 
select cast('0000-00-00' as datetime) >= cast(0 as datetime);--NULL warning
select cast('0000-01-02' as datetime) >= cast('0000-00-00' as datetime);--NULL warning
select cast(20220121102233.4 as datetime) >= cast('2022-01-21 10:22:33' as datetime);--1
select cast(20220121102233.5 as datetime) >= cast('2022-01-21 10:22:33.5' as datetime);--1
select cast(20220121102233.5 as datetime) >= cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233 >= cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.4 >= cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.5 >= cast('2022-01-21 10:22:33.5' as datetime);--0
select 20220121102233.5 >= cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33' >= cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.4' >= cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.5' >= cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.5' >= cast('2022-01-21 10:22:33.5' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
--<=>
select cast('0000-00-00' as datetime) <=> NULL;--1--warning
select NULL <=> NULL;--1
select cast(0 as datetime) <=> NULL;--0
select cast('0000-00-00' as datetime) <=> cast(0 as datetime);--0 --warning
select cast('0000-00-00' as datetime) <=> cast('0000-00-00' as datetime);--1--warning
select cast('2022-01-21' as datetime) <=> 4; --0
select cast('2022-01-21' as datetime) <=> 4.4;--0
select cast('2022-01-21' as datetime) <=> 20220121;--1
select cast('2022-01-21' as datetime) <=> 20220121.00;--1
select cast('2022-01-21' as datetime) <=> 20220121.43;--0
select 20220121.00 <=> cast('2022-01-21' as datetime);--1
select cast(20220121102233.4 as datetime) <=> cast('2022-01-21 10:22:33' as datetime);--1
select cast(20220121102233.5 as datetime) <=> cast('2022-01-21 10:22:33.5' as datetime);--1
select cast(20220121102233.5 as datetime) <=> cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233 <=> cast('2022-01-21 10:22:33' as datetime);--1
select 20220121102233.4 <=> cast('2022-01-21 10:22:33' as datetime);--0
select 20220121102233.5 <=> cast('2022-01-21 10:22:33.5' as datetime);--0
select 20220121102233.5 <=> cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33' <=> cast('2022-01-21 10:22:33' as datetime);--1
select '2022-01-21 10:22:33.4' <=> cast('2022-01-21 10:22:33' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
select '2022-01-21 10:22:33.5' <=> cast('2022-01-21 10:22:33' as datetime);--0
select '2022-01-21 10:22:33.5' <=> cast('2022-01-21 10:22:33.5' as datetime);--0--Need to be modified: text type conversion bit timestamp in mysql !!!!
--+
select 2.3 +  cast('2022-02-32' as datetime) ;--NULL warning
select 2 +  cast('2022-02-32' as datetime) ;--NULL warning
select cast('2022-01-21' as datetime) +  cast('2022-02-32' as datetime);--NULL warning
select cast('2022-01-21' as datetime) +  NULL ;--NULL
select cast('2022-01-21' as datetime) +  cast('9999-02-30' as datetime);--NULL warning
select cast('2022-01-01' as datetime) +  cast('2022-02-30' as datetime);--NULL warning
select cast('2022-01-01' as datetime) +  cast('2022-02-20' as datetime);--40440321
select cast(0 as datetime)  + 44;--44
select cast(0 as datetime)  + 44.88;--44.88
select 20220121.5 + cast('2022-01-21' as datetime);--40440242.5
select 20220121.4 + cast('2022-01-21' as datetime);--40440242.4
select 20220121.00 + cast('2022-01-21' as datetime);--40440242.00
select 20220121 + cast('2022-01-21' as datetime);--40440242
---
select 2.3 -  cast('2022-02-32' as datetime);--NULL warning
select 2 -  cast('2022-02-32' as datetime);--NULL warning
select cast('2022-01-21' as datetime) -  cast('2022-02-32' as datetime);--NULL warning
select cast('2022-01-21' as datetime) -  NULL;--NULL
select cast('2022-01-21' as datetime) -  cast('9999-02-30' as datetime);--NULL warning
select cast('2022-01-01' as datetime) -  cast('2022-02-30' as datetime);--NULL warning
select cast('2022-01-01' as datetime) -  cast('2022-02-20' as datetime);---119
select cast(0 as datetime)  - 44;---44
select cast(0 as datetime)  - 44.88;---44.88
select 20220121.5 - cast('2022-01-21' as datetime);--0.5
select 20220121.4 - cast('2022-01-21' as datetime);--0.4
select 20220121.00 - cast('2022-01-21' as datetime);--0.00
select 20220121 - cast('2022-01-21' as datetime);--0

--btree and hash index
create table idx_1 (idx datetime);
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx > '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx = '1022-01-01';
explain (COSTS OFF) select * from idx_1 where idx <= '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx >= '1022-01-01';

create table idx_2 (idx datetime);
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
explain (COSTS OFF) select * from idx_2 where idx = '1522-01-01';
explain (COSTS OFF) select * from idx_2 where idx > '1522-01-01';

drop table idx_1,idx_2;
create table test_date(c1 datetime, c2 datetime);
insert into test_date values('2012-01-01','2012-01-02'),('2013-01-01','2013-01-02');
create table test_tmp(c1 datetime, c2 datetime);
insert into test_tmp values('2012-01-01','2012-01-02'),('2013-02-01','2013-02-02');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a datetime);
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
explain (COSTS OFF) select * from tb1 where a <= '1004-04-04';
explain (COSTS OFF) select * from tb1 where a = '1004-04-04';
explain (COSTS OFF) select * from tb1 where a != '1004-04-04';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

create table tb1(id datetime);
create table tb2(id datetime);
insert into tb1 values('1970-05-02');
insert into tb2 select id from tb1;
insert into tb2 select id + 8800 from tb1;
select * from tb1;
select * from tb2;
drop table tb1,tb2;

select cast( cast('1970-02-03 00:01:01' as datetime) + 8800 as datetime);--NULL
select cast( cast(19700203000101.8 as datetime) + 8800 as datetime);--NULL

create table tb3(id int);
insert into tb3 values(19220212);
insert into tb3 values(99999999);
create table tb4 as select id::datetime from tb3;
select id::datetime from tb3;
create table tb5 as select '19950399'::datetime;
drop table tb3;

-- *
select cast('221222222222' as datetime) + cast('221222222222' as datetime); --40442444444444
select cast('221222222222' as datetime) - cast('221222222220' as datetime); --2
select cast('221222222222' as datetime) * cast('221222222220' as datetime); --ERROR
select cast('221222222222' as datetime) * 111; --2244555666666642
select cast('221222222222' as datetime) * 111.11; --2246780001111086.42
select 111 * cast('221222222220' as datetime); --2244555666666420
select 111.11 * cast('221222222220' as datetime); --2246780001110864.20
select cast('221222222222' as datetime) * 111111111111; --ERROR

-- /
-- Need to be modified: mysql takes the last 4 digits !!!!!!!
select cast('221222222222' as datetime) / cast('221222222220' as datetime); --1.0000
select cast('221222222222' as datetime) / 111; --182173173173.1712
select cast('221222222222' as datetime) / 111.1; -- 182009200920.0900
select cast('221222222222' as datetime) / 115; -- 175836714975.8435
select cast('221222222222' as datetime) / 0; -- NULL
select 111111111111 / cast('221222222220' as datetime); --0.0055
select 111111111111.111 / cast('221222222220' as datetime);--0.0054948
select 0 / cast('221222222220' as datetime); --0.0000

-- div
select cast('221222222222' as datetime) div cast('221222222220' as datetime); --1
select cast('221222222222' as datetime) div 111; --182173173173
select cast('221222222222' as datetime) div 111.1; -- 182009200920
select cast('221222222222' as datetime) div 115; -- 175836714975
select cast('221222222222' as datetime) div 0; -- NULL
select 111111111111 div cast('221222222220' as datetime); --0
select 111111111111.111 div cast('221222222220' as datetime);--0
select 0 div cast('221222222220' as datetime); --0
select cast('221222222222' as datetime) div 115.1; --175683946326
select cast('221222222222' as datetime) div 115.2; --175531442901
-- %
select cast('111111111111' as datetime) % cast('111111111110' as datetime); --1
select cast('111111111111' as datetime) % 111; --20
select cast('221222222222' as datetime) % 111.1; --10.0
select cast('111111111111' as datetime) % 0; --NULL
select 111111111111 % cast('221222222220' as datetime); --111111111111
select 111111111111.111 % cast('221222222220' as datetime); --111111111111.111
select 0 % cast('221222222220' as datetime); --0

-- mod
select cast('111111111111' as datetime) mod cast('111111111110' as datetime); --1
select cast('111111111111' as datetime) mod 111; --20
select cast('221222222222' as datetime) mod 111.1; --10.0
select cast('111111111111' as datetime) mod 0; --NULL
select 111111111111 mod cast('221222222220' as datetime); --111111111111
select 111111111111.111 mod cast('221222222220' as datetime); --111111111111.111
select 0 mod cast('221222222220' as datetime); --0
select cast('111111111111' as datetime) MOD cast('111111111110' as datetime); --1

-- &
select cast('111111111111' as datetime) & cast('111111111110' as datetime); --20111111111110
select cast('111111111111' as datetime) & 111; --71
select cast('221222222222' as datetime) & 111.1; --14
select cast('111111111111' as datetime) & 0; --0
select 111111111111 & cast('221222222220' as datetime); --511967620
select 111111111111.111 & cast('221222222220' as datetime); --511967620
select 0 & cast('221222222220' as datetime); --0
select cast('221222222222' as datetime) & 1.4; --0
select cast('221222222222' as datetime) & 1.5; --2

-- |
select cast('111111111111' as datetime) | cast('111111111110' as datetime); --20111111111111
select cast('111111111111' as datetime) | 111; --20111111111151
select cast('221222222222' as datetime) | 111.1; --20221222222319
select cast('111111111111' as datetime) | 0; --020111111111111
select 111111111111 | cast('221222222220' as datetime); --20331821365711
select 111111111111.111 | cast('221222222220' as datetime); --20331821365711
select 0 | cast('221222222220' as datetime); --20221222222220
select cast('221222222222' as datetime) | 1.4; --20221222222223
select cast('221222222222' as datetime) | 1.5; --20221222222222

-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~cast('111111111111' as datetime) ;--18446723962598440504

-- ^
select cast('111111111111' as datetime) ^ cast('111111111110' as datetime); --1
select cast('111111111111' as datetime) ^ 111; --20111111111080
select cast('221222222222' as datetime) ^ 111.1; --20221222222305
select cast('111111111111' as datetime) ^ 0; --20111111111111
select 111111111111 ^ cast('221222222220' as datetime); --20331309398091
select 111111111111.111 ^ cast('221222222220' as datetime); --20331309398091
select 0 ^ cast('221222222220' as datetime); --20221222222220
select cast('221222222222' as datetime) ^ 1.4; --20221222222223
select cast('221222222222' as datetime) ^ 1.5; --20221222222220

-- >>
select cast('111111111111' as datetime) >> cast('111111111110' as datetime); --0
select cast('111111111111' as datetime) >> 111; --0
select cast('221222222222' as datetime) >> 111.1; --0
select cast('111111111111' as datetime) >> 0; --20111111111111
select 111111111111 >> cast('221222222220' as datetime); --0
select 111111111111.111 >> cast('221222222220' as datetime); --0
select 0 >> cast('221222222220' as datetime); --0
select cast('221222222222' as datetime) >> 1.4; --10110611111111
select cast('221222222222' as datetime) >> 1.5; --5055305555555

-- <<
select cast('111111111111' as datetime) << cast('111111111110' as datetime); --0
select cast('111111111111' as datetime) << 111; --0
select cast('221222222222' as datetime) << 111.1; --0
select cast('111111111111' as datetime) << 0; --20111111111111
select 111111111111 << cast('221222222220' as datetime); --0
select 111111111111.111 << cast('221222222220' as datetime); --0
select 0 << cast('221222222220' as datetime); --0
select cast('221222222222' as datetime) << 1.4; --40442444444444
select cast('221222222222' as datetime) << 1.5; --80884888888888

-- bit_count
select bit_count(cast('111111111111' as datetime));  --21
select bit_count(cast('111111' as datetime));  --20
select bit_count(cast('99991231235959' as datetime));  --25
select bit_count(cast('111111111111.4' as datetime));  --21
select bit_count(cast('111111111111.5' as datetime));  --19

-- &&
select cast('111111111111' as datetime) && cast('111111111110' as datetime); --1
select cast('111111111111' as datetime) && 111; --1
select cast('221222222222' as datetime) && 111.1; --1
select cast('111111111111' as datetime) && 0; --0
select 111111111111 && cast('221222222220' as datetime); --1
select 111111111111.111 && cast('221222222220' as datetime); --1
select 0 && cast('221222222220' as datetime); --0
select cast('221222222222' as datetime) && 1.4; --1
select cast('221222222222' as datetime) && 1.5; --1

-- ||
set sql_mode='ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
select cast('111111111111' as datetime) || cast('111111111110' as datetime); --1
select cast('111111111111' as datetime) || true; --1
select cast('221222222222' as datetime) || false; --1
select true || cast('111111111111' as datetime); --1
select false || cast('221222222222' as datetime); --1

-- !
select !cast('111111111111' as datetime) ;--0

-- and or xor
select cast('221222222222' as datetime) and cast('221222222222' as datetime );--1
select cast('221222222222' as datetime) and cast('221222222222' as datetime ) and 0::boolean;--0

select cast('221222222222' as datetime) or cast('221222222222' as datetime );--1
select cast('221222222222' as datetime) or cast('221222222222' as datetime ) or 0::boolean;--1

select cast('221222222222' as datetime) xor cast('221222222222' as datetime );--0
select cast('221222222222' as datetime) xor cast('221222222222' as datetime ) xor 0::boolean;--0
select cast('221222222222' as datetime) xor cast('221222222222' as datetime ) xor 1::boolean;--1

-- between and
create table t1 (f1 datetime, f2 datetime, f3 datetime );
insert into t1 values('2001-01-01','2001-01-01 01:01:01','2001-01-01 01:01:01');
insert into t1 values('2001-02-05','2001-02-05 00:00:00','2001-02-05 01:01:01');
insert into t1 values('2001-03-10','2001-03-09 01:01:01','2001-03-10 01:01:01');
insert into t1 values('2001-04-15','2001-04-15 00:00:00','2001-04-15 00:00:00');
insert into t1 values('2001-05-20','2001-05-20 01:01:01','2001-05-20 01:01:01');
select f2 from t1 where f2 between '2001-2-5' and '01-04-14';
select f1, f2, f3 from t1 where f1 between f2 and f3;
select f2 from t1 where '2001-04-10 12:34:56' between f2 and '01-05-01';--Need to be modified: text type conversion bit timestamp in mysql !!!!
--select f2 from t1 where '2001-04-10 12:34:56' between f2 and '01-05-01';
--         f2          
-----------------------
-- 2001-01-01 01:01:01
-- 2001-02-05 00:00:00
-- 2001-03-09 01:01:01
--(3 rows)
select f2 from t1 where cast('2001-04-10 12:34:56' as datetime) between f2 and '01-05-01';
select f2, f3 from t1 where '01-03-10' between f2 and f3;
select f2, f3 from t1 where '01-03-10' not between f2 and f3;
drop table t1;

-- in
create table t1(f2 datetime);
insert into t1 values('01-01-01 00:00:00'),('01-02-03 12:34:56'),('02-04-06 11:22:33');
select * from t1 where f2 in ('01-01-01','01-02-03 12:34:56','01-02-03');
select * from t1 where f2 not in ('01-01-01','01-02-03 12:34:56','01-02-03');
drop table t1;

-- is
CREATE TABLE t1 (a DATETIME, b INT);
INSERT INTO t1 VALUES ('01-02-03 12:34:56',1), ('1999-05-10 00:00:00',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 datetime);
insert into t1 values('2001-01-01 01:01:01');
insert into t1 values('9999-12-31 23:59:59');
insert into t1 values('9999-12-31 23:59:59.6');

select f1 from t1 where f1 like '2001-01-01 01:01:01'; --1
select f1 from t1 where f1 like cast('2001-01-01 01:01:01' as datetime); --1
select f1 from t1 where f1 like 010101010101; --0
select f1 from t1 where f1 like '010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like 20010101010101; --0
select f1 from t1 where f1 like '20010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like cast('010101010101' as datetime); --1
select f1 from t1 where f1 like cast(010101010101 as datetime); --1
select f1 from t1 where f1 like cast(20010101010101 as datetime); --1
select f1 from t1 where f1 like cast('20010101.010101' as datetime); --1
select f1 from t1 where f1 like cast('99991231.2359595' as datetime);
drop table t1;

--MySQL datetimes type test case start
select cast('1.1' as datetime(3));--NULL warning
select cast('11.1' as datetime(3));--NULL warning
select cast('1.11' as datetime(3));--NULL warning
select cast('11.11' as datetime(3));--NULL warning
select cast('11.111' as datetime(3));--2011-11-01 00:00:00.000 
select cast('99.111' as datetime(3));--1999-11-01 00:00:00.000 
select cast('99.311' as datetime(3));--NULL warning
select cast('1.1111' as datetime(3));--2001-11-11 00:00:00.000
select cast('1.111122222' as datetime(3));--2001-11-11 22:22:02.000
select cast('998.3' as datetime(3));--1999-08-03 00:00:00.000
select cast('991.31' as datetime(3));--1999-01-31 00:00:00.000
select cast('999.31' as datetime(3));--NULL warning
select cast('111.111' as datetime(3));--2011-01-11 01:00:00.000
select cast('116.119' as datetime(3));--2011-06-11 09:00:00.000  
select cast('116.419' as datetime(3));--NULL warning
select cast('998.2222' as datetime(3));--1999-08-22 22:00:00.000 
select cast('998.2224' as datetime(3));--NULL warning
select cast('998.222333' as datetime(3));--1999-08-22 23:33:00.000
select cast('998.222360' as datetime(3));--NULL warning
select cast('998.22235959' as datetime(3));--1999-08-22 23:59:59.000 
select cast('998.222359599999' as datetime(3));--1999-08-22 23:59:59.000  warning
select cast('998.2223595999999999999999' as datetime(3));--1999-08-22 23:59:59.000 warning
select cast('1118.1' as datetime(3));--NULL warning
select cast('1118.12' as datetime(3));--NULL warning
select cast('1118.122' as datetime(3));--1118-12-02 00:00:00.000 
select cast('1118.122212' as datetime(3));--1118-12-22 12:00:00.000
select cast('1118.123212' as datetime(3));--NULL warning
select cast('1118.123124' as datetime(3));--NULL warning
select cast('1118.12312323' as datetime(3));--1118-12-31 23:23:00.000 
select cast('1118.12312366' as datetime(3));--NULL warning
select cast('1118.1222235659' as datetime(3));--1118-12-22 23:56:59.000
select cast('1118.122223565999999999999999' as datetime(3));--1118-12-22 23:56:59.000

select cast('11118.1' as datetime(3));--2011-11-08 01:00:00.000
select cast('11118.12' as datetime(3));--2011-11-08 12:00:00.000
select cast('11118.32' as datetime(3));--NULL
select cast('11118.111111111' as datetime(3));--2011-11-08 11:11:11.000
select cast('11118.116011111' as datetime(3));--NULL
select cast('11118.110660111' as datetime(3));--NULL
select cast('11118.1106019999999999999999' as datetime(3));--2011-11-08 11:06:01.000

select cast('111121.1' as datetime(3));--2011-11-21 01:00:00.000
select cast('111121.12' as datetime(3));--2011-11-21 12:00:00.000
select cast('111121.24' as datetime(3));--NULL
select cast('111121.231' as datetime(3));--2011-11-21 23:01:00.000
select cast('111121.2312' as datetime(3));--2011-11-21 23:12:00.000
select cast('111121.231260' as datetime(3));--NULL
select cast('111521.231259' as datetime(3));--NULL
select cast('111121.231259' as datetime(3));--2011-11-21 23:12:59.000
select cast('111121.2312599999999999999' as datetime(3));--2011-11-21 23:12:59.000

select cast('1111213.5' as datetime(3));--2011-11-21 03:05:00.000
select cast('1111213.11' as datetime(3));--2011-11-21 03:11:00.000
select cast('1111213.112' as datetime(3));--2011-11-21 03:11:02.000
select cast('1111213.1122' as datetime(3));--2011-11-21 03:11:22.000
select cast('1111213.11222222222' as datetime(3));--2011-11-21 03:11:22.000
select cast('1111213.99' as datetime(3));--NULL
select cast('1111213.59' as datetime(3));--2011-11-21 03:59:00.000
select cast('1111213.60' as datetime(3));--NULL
select cast('1111213.5960' as datetime(3));--NULL
select cast('1111213.5959' as datetime(3));--2011-11-21 03:59:59.000
select cast('1111213.595988888888888' as datetime(3));--2011-11-21 03:59:59.000

select cast('11111121.9' as datetime(3));--1111-11-21 09:00:00.000
select cast('11111121.23' as datetime(3));--1111-11-21 23:00:00.000
select cast('11111121.24' as datetime(3));--NULL
select cast('11111121.235555' as datetime(3));--1111-11-21 23:55:55.000
select cast('11111121.245555' as datetime(3));--NULL
select cast('11111121.236055' as datetime(3));--NULL
select cast('11111121.235560' as datetime(3));--NULL
select cast('11111121.235555999999999' as datetime(3));--1111-11-21 23:55:55.000

select cast('111111211.2' as datetime(3));--2011-11-11 21:01:02.000 
select cast('111111241.2' as datetime(3));--NULL
select cast('111111231.2' as datetime(3));--2011-11-11 23:01:02.000
select cast('111111239.2' as datetime(3));--2011-11-11 23:09:02.000
select cast('111111239.59' as datetime(3));--2011-11-11 23:09:59.000
select cast('111111239.60' as datetime(3));--NULL
select cast('111111239.5999999999' as datetime(3));--2011-11-11 23:09:59.000
select cast('1111112111.2' as datetime(3));--2011-11-11 21:11:02.000
select cast('1111112411.2' as datetime(3));--NULL
select cast('1111112159.2' as datetime(3));--2011-11-11 21:59:02.000
select cast('1111112160.2' as datetime(3));--NULL
select cast('1111112159.59' as datetime(3));--2011-11-11 21:59:59.000
select cast('1111112159.5999999999999' as datetime(3));--2011-11-11 21:59:59.000

select cast('11111121591.2' as datetime(3));--2011-11-11 21:59:01.200
select cast('11111121599.23' as datetime(3));--2011-11-11 21:59:09.230
select cast('11111121599.23444' as datetime(3));--2011-11-11 21:59:09.234 
select cast('11111121599.99999999999999999' as datetime(3));--2011-11-11 21:59:10.000
select cast('11111124599.23' as datetime(3));--NULL
select cast('11111121609.23444' as datetime(3));--NULL
select cast('111111215059.2' as datetime(3));--2011-11-11 21:50:59.200
select cast('111111215060.2' as datetime(3));--NULL
select cast('111111215019.23444' as datetime(3));--2011-11-11 21:50:19.234
select cast('111111215059.23455556' as datetime(3));--2011-11-11 21:50:59.235
select cast('111111215059.99999999' as datetime(3));--2011-11-11 21:51:00.000

select cast('1111112150591.9' as datetime(3));--2011-11-11 21:50:59.000
select cast('1111112150601.9' as datetime(3));--NULL
select cast('1111112150591.99' as datetime(3));--2011-11-11 21:50:59.000
select cast('1111112150591.9999999999999' as datetime(3));--2011-11-11 21:50:59.000

select cast('11111111201911.9' as datetime(3));--1111-11-11 20:19:11.900
select cast('11111111241911.9' as datetime(3));--NULL
select cast('11111111236011.9' as datetime(3));--NULL
select cast('11111111201911.9999999999' as datetime(3));--1111-11-11 20:19:12.000

select cast('111111112019111.9' as datetime(3));--1111-11-11 20:19:11.000
select cast('111111111260111.9' as datetime(3));--NULL
select cast('11111111201911111111.9' as datetime(3));--1111-11-11 20:19:11.000
select cast('1111111112360111.9' as datetime(3));--1111-11-11 12:36:01.000
select cast('111111112019113333.9999999999' as datetime(3));--1111-11-11 20:19:11.000

select cast('1.1.1' as datetime(3));--2001-01-01 00:00:00.000
select cast('1.1.11' as datetime(3));--2001-01-11 00:00:00.000
select cast('11.11.11' as datetime(3));--2011-11-11 00:00:00.000
select cast('11.13.11' as datetime(3));--NULL
select cast('11.13.32' as datetime(3));--NULL
select cast('99.11.11' as datetime(3));--1999-11-11 00:00:00.000

select cast('99.111.11' as datetime(3));--1999-11-01 11:00:00.000
select cast('99.111.1111' as datetime(3));--1999-11-01 11:11:00.000
select cast('99.111.3111' as datetime(3));--NULL
select cast('99.111.116111' as datetime(3));--NULL
select cast('99.111.115961' as datetime(3));--NULL
select cast('99.111.115911' as datetime(3));--1999-11-01 11:59:11.000
select cast('99.1111.115911' as datetime(3));--1999-11-11 11:59:11.000
select cast('99.1133.115911' as datetime(3));--NULL
select cast('99.1130.115911' as datetime(3));--1999-11-30 11:59:11.000
select cast('99.1311.11' as datetime(3));--NULL

select cast('999.1.11' as datetime(3));--1999-09-01 11:00:00.000
select cast('999.11.11' as datetime(3));--1999-09-11 11:00:00.000
select cast('999.30.11' as datetime(3));--1999-09-30 11:00:00.000
select cast('999.34.11' as datetime(3));--NULL
select cast('999.30.25' as datetime(3));--NULL

select cast('999.111.11' as datetime(3));--1999-09-11 01:11:00.000
select cast('999.239.20' as datetime(3));--1999-09-23 09:20:00.000
select cast('999.319.29' as datetime(3));--NULL
select cast('999.3191.29' as datetime(3));--NULL
select cast('999.2124.29' as datetime(3));--NULL
select cast('999.2123.29' as datetime(3));-- 1999-09-21 23:29:00.000 
select cast('999.2111.59' as datetime(3));--1999-09-21 11:59:00.000
select cast('999.2111.60' as datetime(3));--NULL
select cast('999.2111.5959' as datetime(3));--1999-09-21 11:59:59.000
select cast('999.2111.5960' as datetime(3));--NULL
select cast('999.2111.595999999999999' as datetime(3));--1999-09-21 11:59:59.000

select cast('999.21112.5959' as datetime(3));--1999-09-21 11:02:59.000  
select cast('999.21112.6059' as datetime(3));--NULL
select cast('999.211111.5959' as datetime(3));--1999-09-21 11:11:59.000
select cast('999.212411.5959' as datetime(3));--NULL
select cast('999.211160.5959' as datetime(3));--NULL

select cast('999.2111111.5959' as datetime(3));--1999-09-21 11:11:01.596
select cast('999.2111111.999' as datetime(3));--1999-09-21 11:11:01.999
select cast('999.2111111.9999999' as datetime(3));--1999-09-21 11:11:02.000
select cast('999.21111111.9999999' as datetime(3));--1999-09-21 11:11:12.000
select cast('999.21111160.9999999' as datetime(3));--NULL
select cast('999.21116059.9999999' as datetime(3));--NULL

select cast('1999.1.1' as datetime(3));-- 1999-01-01 00:00:00.000
select cast('1999.1.11' as datetime(3));--1999-01-11 00:00:00.000
select cast('1999.11.11' as datetime(3));--1999-11-11 00:00:00.000
select cast('1999.13.11' as datetime(3));--NULL

select cast('1999.111.11' as datetime(3));--1999-11-01 11:00:00.000
select cast('1999.1112.11' as datetime(3));--1999-11-12 11:00:00.000 
select cast('1999.1112.145678' as datetime(3));--NULL
select cast('1999.1112.14562299999999' as datetime(3));--1999-11-12 14:56:22.000

select cast('1999.11122.19' as datetime(3));-- 1999-11-12 02:19:00.000
select cast('1999.11122.1959' as datetime(3));--1999-11-12 02:19:59.000
select cast('1999.11122.1960' as datetime(3));--NULL
select cast('1999.111212.11' as datetime(3));--1999-11-12 12:11:00.000
select cast('1999.111224.11' as datetime(3));--NULL

select cast('1999.1112221.11' as datetime(3));--1999-11-12 22:01:11.000
select cast('1999.11122212.99' as datetime(3));--NULL
select cast('1999.11122212.19' as datetime(3));--1999-11-12 22:12:19.000

select cast('1999.111222121.19' as datetime(3));--1999-11-12 22:12:01.190
select cast('1999.1112221212.19' as datetime(3));--1999-11-12 22:12:12.190
select cast('1999.1112221260.19' as datetime(3));--NULL
select cast('1999.111222121212999.19' as datetime(3));--1999-11-12 22:12:12.000--warning

select cast('10000.1.1' as datetime(3));--2010-00-00 01:01:00.000
select cast('10001.1.1' as datetime(3));--2010-00-01 01:01:00.000 
select cast('11111.1.1' as datetime(3));--2011-11-01 01:01:00.000
select cast('11111.11.11' as datetime(3));--2011-11-01 11:11:00.000
select cast('11111.24.11' as datetime(3));--NULL
select cast('199999.23.59' as datetime(3));--NULL
select cast('11111.23.11' as datetime(3));--2011-11-01 23:11:00.000
select cast('11111.23.59' as datetime(3));--2011-11-01 23:59:00 .000
select cast('11111.23232.99999' as datetime(3));--2011-11-01 23:23:03.000
select cast('11111.232323.99999' as datetime(3));--2011-11-01 23:23:24.000
select cast('11111.23232233.99999' as datetime(3));--2011-11-01 23:23:22.000

select cast('111111.23.9' as datetime(3));--2011-11-11 23:09:00.000
select cast('111111.24.9' as datetime(3));--NULL
select cast('111111.221.9' as datetime(3));--2011-11-11 22:01:09.000
select cast('111111.22111111.9' as datetime(3));--2011-11-11 22:11:11.000
select cast('111111.2260111.9999' as datetime(3));--NULL
select cast('111111.22111111.9999999999' as datetime(3));--2011-11-11 22:11:11.000

select cast('1111111.3231.9' as datetime(3));--2011-11-11 01:32:31.900
select cast('1111111.6031.9' as datetime(3));--NULL
select cast('1111111.5931.9' as datetime(3));--2011-11-11 01:59:31.900 
select cast('1111221.59.29' as datetime(3));--2011-11-22 01:59:29.000 
select cast('1111221.59599999999.29' as datetime(3));--2011-11-22 01:59:59.000 

select cast('11111211.1.29' as datetime(3));--1111-12-11 01:29:00.000
select cast('11111211.1112.29' as datetime(3));--1111-12-11 11:12:29.000
select cast('11111211.1160.29' as datetime(3));--NULL
select cast('11111211.1120.69' as datetime(3));--NULL
select cast('11111211.1159599999999.29' as datetime(3));--1111-12-11 11:59:59.000
select cast('11111211.111616.99999' as datetime(3));--1111-12-11 11:16:17.000

select cast('111112111.1.29' as datetime(3));--2011-11-12 11:01:01.290
select cast('111112241.1.29' as datetime(3));--NULL
select cast('111112231.1.29' as datetime(3));--2011-11-12 23:01:01.290 
select cast('111112239.1.29' as datetime(3));-- 2011-11-12 23:09:01.290
select cast('111112239.60.29' as datetime(3));--NULL
select cast('111112239.6.9999' as datetime(3));--2011-11-12 23:09:07.000
select cast('1111122329.6.9' as datetime(3));--2011-11-12 23:29:06.900
select cast('1111122329.6.9999' as datetime(3));--2011-11-12 23:29:07.000
select cast('1111122329.60.9' as datetime(3));--NULL
select cast('1111122360.16.9' as datetime(3));--NULL
select cast('1111122416.16.9' as datetime(3));--NULL
select cast('1111122216.599999.999999999999' as datetime(3));--2011-11-12 22:16:59.000--warning

select cast('11111222161.1.1' as datetime(3));--2011-11-12 22:16:01.100
select cast('11111222161.9999999999.1' as datetime(3));--2011-11-12 22:16:02.000
select cast('11111222161.9999999999.9999999999' as datetime(3));--2011-11-12 22:16:02.000
select cast('111112226011.9.9' as datetime(3));--NULL
select cast('111112226011.9999999999.9999999999' as datetime(3));--NULL
select cast('111112225911.9.9' as datetime(3));--2011-11-12 22:59:11.900
select cast('111112222611.9999999999.9999999999' as datetime(3));--2011-11-12 22:26:12.000
select cast('1111122216119.9999.99999' as datetime(3));--2011-11-12 22:16:11.000
select cast('1111122216111.9999.99999' as datetime(3));--2011-11-12 22:16:11.000

select cast('11111222161121.9999.99999' as datetime(3));--1111-12-22 16:11:22.000
select cast('111112221611211.9999.99999' as datetime(3));--1111-12-22 16:11:21.000
select cast('1111122216112111111.9999.99999' as datetime(3));--1111-12-22 16:11:21.000 --warning

select cast('1.1.1.1' as datetime(3));--2001-01-01 01:00:00.000
select cast('11.11.11.11' as datetime(3));--2011-11-11 11:00:00.000 
select cast('1.1.1.11.11' as datetime(3));--2001-01-01 11:11:00.000
select cast('111.11.11.11' as datetime(3));--2011-01-11 11:11:00.000 
select cast('1111.11.11.11' as datetime(3));--1111-11-11 11:00:00.000
select cast('1111.60.4.11' as datetime(3));--NULL
select cast('111111.11.11.11' as datetime(3));--2011-11-11 11:11:11.000
select cast('1111111.111.11.11' as datetime(3));--2011-11-11 01:11:01.110
select cast('11111111.11.4.11' as datetime(3));--1111-11-11 11:04:11.000
select cast('111111111.11.4.11' as datetime(3));--2011-11-11 11:01:11.400
select cast('1111111111.11.4.11' as datetime(3));--2011-11-11 11:11:11.400
select cast('11111111111.11.4.11' as datetime(3));--2011-11-11 11:11:01.110
select cast('111111111111.11.4.11' as datetime(3));--2011-11-11 11:11:11.110
select cast('1111111111111.11.4.11' as datetime(3));--2011-11-11 11:11:11.000 
select cast('11111111111111.11.4.11' as datetime(3));--1111-11-11 11:11:11.110
select cast('11111111111111111.11.4.11' as datetime(3));--1111-11-11 11:11:11.000
select cast('11.11.11.23.34.50' as datetime(3));--2011-11-11 23:34:50.000
select cast('9999.12.31.23.59.59.999999.99' as datetime(3));--NULL
select cast('99.12.31.23.59.59.999999.99' as datetime(3));--2000-01-01 00:00:00.000


select cast('111/11/11/23/34/50' as datetime(3));--0111-11-11 23:34:50.000
select cast('10000/11/11/23/34/50' as datetime(3));--NULL
select cast('1000/13/11/23/34/50' as datetime(3));--NULL
select cast('9999/11/60/23/34/50' as datetime(3));--NULL
select cast('9999/11/59/24/34/50' as datetime(3));--NULL
select cast('9999/11/59/23/60/50' as datetime(3));--NULL
select cast('9999/11/59/23/59/60' as datetime(3));--NULL
select cast('9999/11/19/23/59/59/99999999999' as datetime(3));--9999-11-19 23:59:59.000

select cast('111.11.11.23.34.50' as datetime(3));--2011-01-11 11:23:34.500
select cast('1111/11.11.23/34/50' as datetime(3));--1111-11-11 23:34:50.000
select cast('111/11.11.23/34/50' as datetime(3));--0111-11-11 23:34:50.000
select cast('111/11.11/23/34/50' as datetime(3));--0111-11-11 23:34:50.000
select cast('111/11.11.24/34/50' as datetime(3));--NULL
select cast('111/11.11.23/60/50' as datetime(3));--NULL
select cast('111/11.11.23/34/60' as datetime(3));--NULL
select cast('111/11.11.23/34/59/9999999' as datetime(3));--0111-11-11 23:34:59.000
select cast('10000/11/11/23/34/50' as datetime(3));--NULL

select cast(' ' as datetime(3));--NULL
select cast('' as datetime(3));--NULL

select cast('2023-03-03 20:01:20AM' as datetime(3));--2023-03-03 20:01:20.000
select cast('20230303200120PM' as datetime(3));--NULL
select cast('20230303200120AM' as datetime(3));--NULL 
select cast('20230303T200120' as datetime(3));--2023-03-03 20:01:20.000
select cast('2003-03-03T20:01:20' as datetime(3));--2003-03-03 20:01:20.000

--explict conversion 
select datetime(3)'20100231111';--error 

select cast(1 as datetime(3));--NULL 
select cast(11 as datetime(3));--NULL
select cast(111 as datetime(3));--2000-01-11 00:00:00.000
select cast(1111 as datetime(3));--2000-11-11 00:00:00.000
select cast(1131 as datetime(3));--NULL
select cast(11111 as datetime(3));--2001-11-11 00:00:00.000
select cast(11111.77 as datetime(3));--2001-11-11 00:00:00.000
select cast(111111 as datetime(3));--2011-11-11 00:00:00.000
select cast(111311 as datetime(3));--NULL
select cast(111111.88 as datetime(3));--2011-11-11 00:00:00.000
select cast(113111 as datetime(3));--NULL
select cast(112160 as datetime(3));--NULL

select cast(1111111 as datetime(3));--0111-11-11 00:00:00.000
select cast(1111111.34 as datetime(3));--0111-11-11 00:00:00.000
select cast(1111161 as datetime(3));--NULL
select cast(11111111 as datetime(3));--1111-11-11 00:00:00.000
select cast(11111111.56 as datetime(3));--1111-11-11 00:00:00.000
select cast(11113111 as datetime(3));--NULL
select cast(11112160 as datetime(3));--NULL
select cast(111111111 as datetime(3));--2000-01-11 11:11:11.000
select cast(111111111.99 as datetime(3));--2000-01-11 11:11:11.990
select cast(111351111 as datetime(3));--NULL
select cast(1111111111 as datetime(3));--2000-11-11 11:11:11.000
select cast(1111111171 as datetime(3));--NULL
select cast(11111116059 as datetime(3));--NULL

select cast(11111111111 as datetime(3));-- 2001-11-11 11:11:11.000
select cast(111111111111 as datetime(3));--2011-11-11 11:11:11.000
select cast(11111111111.33 as datetime(3));--2001-11-11 11:11:11.330
select cast(111111111111.45 as datetime(3));--2011-11-11 11:11:11.450
select cast(111111241156 as datetime(3));--NULL
select cast(1111111111111 as datetime(3));--0111-11-11 11:11:11.000
select cast(1111111311105 as datetime(3));--NULL
select cast(11111111111111 as datetime(3));--1111-11-11 11:11:11.000
select cast(11111111111111.23 as datetime(3));--1111-11-11 11:11:11.230
select cast(11111111119905 as datetime(3));--NULL
select cast(11111111111165 as datetime(3));--NULL
select cast(11111111251105 as datetime(3));--NULL
select cast(111111111111111 as datetime(3));--NULL
select cast(1111111111111111 as datetime(3));--NULL
select cast(1111111111111111.44 as datetime(3));--NULL
select cast(11111111111233444 as datetime(3));--NULL
select cast(111111111111111111111 as datetime(3));--NULL
select cast(19830905 as datetime(3));--1983-09-05 00:00:00.000
select cast(119830905 as datetime(3));--NULL
select cast(198309051 as datetime(3));--NULL
select cast(198205051 as datetime(3));--NULL
select cast(128205051 as datetime(3));--2000-01-28 20:50:51.000
select cast(6995 as datetime(3));--NULL
select cast(2007-05-23 as datetime(3));--NULL
select cast(2007-05*23 as datetime(3));--NULL
select cast(830905 as datetime(3));--1983-09-05 00:00:00.000
select cast(9830905 as datetime(3));--0983-09-05 00:00:00.000
select cast(30905 as datetime(3));--2003-09-05 00:00:00.000
select cast(230905 as datetime(3));--2023-09-05 00:00:00.000
select cast(690905 as datetime(3));--2069-09-05 00:00:00.000
select cast(700905 as datetime(3));--1970-09-05 00:00:00.000
select cast(0905 as datetime(3));--2000-09-05 00:00:00.000
select cast(905 as datetime(3));--2000-09-05 00:00:00.000
select cast(00000905 as datetime(3));--2000-09-05 00:00:00.000
select cast(0000000905 as datetime(3));-- 2000-09-05 00:00:00.000
select cast(2007*05*23 as datetime(3));--2023-08-05 00:00:00.000
select cast(05*23 as datetime(3));--2000-01-15 00:00:00.000
select cast(06/03 as datetime(3));--NULL 
select cast(300/03 as datetime(3));--NULL
select cast(30000000/3 as datetime(3)); --1000-00-00 00:00:00.000
select cast(11111111111111.5555554995 as datetime(6));--1111-11-11 11:11:11.555555
select cast(11111111111111.555555999 as datetime(6));--1111-11-11 11:11:11.555556
--round
select cast('2010.01.31.23.59.59.99999999' as datetime(3));--2010-02-01 00:00:00.000
select cast('2010.01.31.23.59.59.999999' as datetime(3));--2010-02-01 00:00:00.000
select cast('2010.00.31.23.59.59.99999999' as datetime(3));--NULL

--set sql_mode
set sql_mode ='';
--strict mode :NO_ZERO_IN_DATE
create table test(dt datetime(3));
insert into test values('2100-00-04 12:13:14');--ok
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--set NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--warning
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--NULL
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--strict mode :NO_ZERO_DATE
set sql_mode ='NO_ZERO_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--set NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE';
insert into test values('2100-00-04 00:00:00');--warning
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--strict mode :NO_ZERO_IN_DATE
set sql_mode ='NO_ZERO_IN_DATE,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ERROR
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));-- 9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--set ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--warning
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--2010-02-31 00:00:00.000
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

--strict mode :ALLOW_INVALID_DATES
set sql_mode ='ALLOW_INVALID_DATES,STRICT_TRANS_TABLES';
insert into test values('2100-00-04 00:00:00');--ok
insert into test values('0000-00-00 00:00:00');--ok
insert into test values('1111-01-01 25:00:00');--ERROR
insert into test values('9999-12-31 23:59:59.44');--ok
insert into test values('9999-12-31 23:59:59.99');--ok
select cast('2010-02-31 00:00:00' as datetime(3));--2010-02-31 00:00:00.000
select cast(20100231121314 as datetime(3));--NULL
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500
select * from test;
delete from test;

set sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE';
--default strict+NO_ZERO_IN_DATE+NO_ZERO_DATE
insert into test values('2100-00-04 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR
insert into test values('1111-01-01 25:00:00');--ERROR
select cast('2010-02-31 00:00:00' as datetime(3));--NULL
select cast(20100231121314 as datetime(3));--NULL warning
select cast('9999-12-31 23:59:59.44' as datetime(3));--9999-12-31 23:59:59.440
select cast('9999-12-31 23:59:59.99' as datetime(3));--9999-12-31 23:59:59.990
select cast(99991231235959 as datetime(3));--9999-12-31 23:59:59.000
select cast(99991231235959.4 as datetime(3));--9999-12-31 23:59:59.400
select cast(99991231235959.5 as datetime(3));--9999-12-31 23:59:59.500


insert into test values('');--error
insert into test values('99999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-04-32 12:13:14');--ERROR
insert into test values('9999-02-31 12:13:14');--ERROR
insert into test values('9999-00-31 12:13:14');--ERROR
insert into test values('0000-00-00 12:13:14');--ERROR

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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ok
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--0000-00-00--warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14 
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--error
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00--warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14 
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--0000-00-00--warning
insert into test values(231817.49999);--0000-00-00--warning
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14 
insert into test values(20200914231817.49999);--2020-09-14 
insert into test values(20200934231817.49999);--error
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14 
insert into test values(14231817.49999);--error
insert into test values(231817.49999);--error
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--ERROR
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--2023-00-17
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
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
insert into test values(20200914231817);--2020-09-14
insert into test values(20200914231817.66);--2020-09-14
insert into test values(20200914231817.49999);--2020-09-14
insert into test values(20200934231817.49999);--0000-00-00 --warning
insert into test values(0914231817.49999);--2000-09-14
insert into test values(190914231817.49999);--2019-09-14
insert into test values(790914231817.49999);--1979-09-14
insert into test values(14231817.49999);--0000-00-00 --warning
insert into test values(231817.49999);--0000-00-00 --warning
insert into test values(230017);--0000-00-00 --warning
insert into test values(99991231235959);--ok
insert into test values(99991231235959.44);--ok
insert into test values(99991231235959.99);--2 warning
select * from test;

drop table test;
set sql_mode to default;
--

-- oper
---=
select cast('2022-01-21 10:22:33.4' as datetime(3)) = cast('2022-01-21 10:22:33.4' as datetime(4));--1
select cast(20220121102233.4 as datetime(3)) = cast('2022-01-21 10:22:33.4' as datetime(3));--1
select 20220121102233.4 = cast('2022-01-21 10:22:33.4' as datetime(3));--1
select '2022-01-21 10:22:33.000' = cast('2022-01-21 10:22:33' as datetime(3));--1
select '2022-01-21 10:22:33.000' = cast('2022-01-21 10:22:33' as datetime(4));--1
--<>
select cast('2022-01-21 10:22:33.4' as datetime(3)) <> cast('2022-01-21 10:22:33.4' as datetime(4));--0
select cast(20220121102233.4 as datetime(3)) <> cast('2022-01-21 10:22:33.4' as datetime(3));--0
select 20220121102233.4 <> cast('2022-01-21 10:22:33.4' as datetime(3));--0
select '2022-01-21 10:22:33.000' <> cast('2022-01-21 10:22:33' as datetime(3));--0
select '2022-01-21 10:22:33.000' <> cast('2022-01-21 10:22:33' as datetime(4));--0
--<
select cast('2022-01-21 10:22:33.4' as datetime(3)) < cast('2022-01-21 10:22:33.4' as datetime(4));--0
select cast(20220121102233.4 as datetime(3)) < cast('2022-01-21 10:22:33.4' as datetime(3));--0
select 20220121102233.4 < cast('2022-01-21 10:22:33.4' as datetime(3));--0
select '2022-01-21 10:22:33.000' < cast('2022-01-21 10:22:33' as datetime(3));--0
select '2022-01-21 10:22:33.000' < cast('2022-01-21 10:22:33' as datetime(4));--0
-->
select cast('2022-01-21 10:22:33.4' as datetime(3)) > cast('2022-01-21 10:22:33.4' as datetime(4));--0
select cast(20220121102233.4 as datetime(3)) > cast('2022-01-21 10:22:33.4' as datetime(3));--0
select 20220121102233.4 > cast('2022-01-21 10:22:33.4' as datetime(3));--0
select '2022-01-21 10:22:33.000' > cast('2022-01-21 10:22:33' as datetime(3));--0
select '2022-01-21 10:22:33.000' > cast('2022-01-21 10:22:33' as datetime(4));--0
--<=
select cast('2022-01-21 10:22:33.4' as datetime(3)) <= cast('2022-01-21 10:22:33.4' as datetime(4));--1
select cast(20220121102233.4 as datetime(3)) <= cast('2022-01-21 10:22:33.4' as datetime(3));--1
select 20220121102233.4 <= cast('2022-01-21 10:22:33.4' as datetime(3));--1
select '2022-01-21 10:22:33.000' <= cast('2022-01-21 10:22:33' as datetime(3));--1
select '2022-01-21 10:22:33.000' <= cast('2022-01-21 10:22:33' as datetime(4));--1
-->=
select cast('2022-01-21 10:22:33.4' as datetime(3)) >= cast('2022-01-21 10:22:33.4' as datetime(4));--1
select cast(20220121102233.4 as datetime(3)) >= cast('2022-01-21 10:22:33.4' as datetime(3));--1
select 20220121102233.4 >= cast('2022-01-21 10:22:33.4' as datetime(3));--1
select '2022-01-21 10:22:33.000' >= cast('2022-01-21 10:22:33' as datetime(3));--1
select '2022-01-21 10:22:33.000' >= cast('2022-01-21 10:22:33' as datetime(4));--1
--<=>
select cast('2022-01-21 10:22:33.4' as datetime(3)) <=> cast('2022-01-21 10:22:33.4' as datetime(4));--1
select cast(20220121102233.4 as datetime(3)) <=> cast('2022-01-21 10:22:33.4' as datetime(3));--1
select 20220121102233.4 <=> cast('2022-01-21 10:22:33.4' as datetime(3));--1
select '2022-01-21 10:22:33.000' <=> cast('2022-01-21 10:22:33' as datetime(3));--1
select '2022-01-21 10:22:33.000' <=> cast('2022-01-21 10:22:33' as datetime(4));--1

--+
select 2.3 +  cast('2022-01-21 10:22:33.4' as datetime(3)) ;--20220121102235.700
select 2 +  cast('2022-01-21 10:22:33.4' as datetime(3)) ;--20220121102235.400
select cast('2022-01-21 10:22:33.4' as datetime(3)) +  cast('2022-02-32' as datetime(3));--NULL
select cast('2022-01-21 10:22:33.4' as datetime(3)) + 44;--20220121102277.400
select cast('2022-01-21 10:22:33.4' as datetime(3)) + 44.88;--20220121102278.280

---
select 2.3 -  cast('2022-01-21 10:22:33.4' as datetime(3)) ;--  -20220121102231.100
select 2 -  cast('2022-01-21 10:22:33.4' as datetime(3)) ;--   -20220121102231.400
select cast('2022-01-21 10:22:33.4' as datetime(3)) -  cast('2022-02-32' as datetime(3));--NULL
select cast('2022-01-21 10:22:33.4' as datetime(3)) - 44;--20220121102189.400
select cast('2022-01-21 10:22:33.4' as datetime(3)) - 44.88;--20220121102188.520

--btree and hash index
create table idx_1 (idx datetime(3));
create index idx1 on idx_1 using btree (idx);
insert into idx_1 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
vacuum full analyze idx_1;
explain (COSTS OFF) select * from idx_1 where idx < '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx > '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx = '1022-01-01';
explain (COSTS OFF) select * from idx_1 where idx <= '2022-01-01';
explain (COSTS OFF) select * from idx_1 where idx >= '1022-01-01';

create table idx_2 (idx datetime(3));
create index idx2 on idx_2 using hash (idx);
vacuum full analyze idx_2;
insert into idx_2 values ('1022-01-01'),('1522-01-01'),('2022-01-01');
explain (COSTS OFF) select * from idx_2 where idx = '1522-01-01';
explain (COSTS OFF) select * from idx_2 where idx > '1522-01-01';

drop table idx_1,idx_2;
create table test_date(c1 datetime(3), c2 datetime(3));
insert into test_date values('2012-01-01','2012-01-02'),('2013-01-01','2013-01-02');
create table test_tmp(c1 datetime(3), c2 datetime(3));
insert into test_tmp values('2012-01-01','2012-01-02'),('2013-02-01','2013-02-02');
select * from test_date where(c1,c2) not in (select * from test_tmp);
drop table test_date,test_tmp;

create table tb1 (a datetime(3));
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
explain (COSTS OFF) select * from tb1 where a <= '1004-04-04';
explain (COSTS OFF) select * from tb1 where a = '1004-04-04';
explain (COSTS OFF) select * from tb1 where a != '1004-04-04';
set enable_seqscan =true;
set enable_bitmapscan =true;
drop procedure proc1;
drop table tb1;

create table tb1(id datetime(3));
create table tb2(id datetime(3));
insert into tb1 values('1970-05-02');
insert into tb2 select id from tb1;
insert into tb2 select id + 8800 from tb1;
select * from tb1;
select * from tb2;
drop table tb1,tb2;

select cast( cast('1970-02-03 00:01:01' as datetime(3)) + 8800 as datetime(3));--NULL
select cast( cast(19700203000101.8 as datetime(3)) + 8800 as datetime(3));--NULL

create table tb3(id int);
insert into tb3 values(19220212);
insert into tb3 values(99999999);
create table tb4 as select id::datetime(3) from tb3;
select id::datetime(3) from tb3;
create table tb5 as select '19950399'::datetime(3);
drop table tb3;

-- *
select cast('221222222222' as datetime(3)) + cast('221222222222' as datetime(3)); --40442444444444.000
select cast('221222222222' as datetime(3)) - cast('221222222220' as datetime(3)); --2.000
select cast('221222222222' as datetime(3)) * cast('221222222220' as datetime(3)); --408897828160444397506172840.000000
select cast('221222222222' as datetime(3)) * 111; --2244555666666642.000
select cast('221222222222' as datetime(3)) * 111.11; --2246780001111086.42000
select 111 * cast('221222222220' as datetime(3)); --2244555666666420.000
select 111.11 * cast('221222222220' as datetime(3)); --2246780001110864.20000
select cast('221222222222' as datetime(3)) * 111111111111; --2246802469133530975308642.000

-- /
-- Need to be modified: mysql takes the last 4 digits !!!!!!!
select cast('221222222222' as datetime(3)) / cast('221222222220' as datetime(3)); --1.0000000
select cast('221222222222' as datetime(3)) / 111; --182173173173.1711712
select cast('221222222222' as datetime(3)) / 111.1; -- 182009200920.0900090
select cast('221222222222' as datetime(3)) / 115; -- 175836714975.8434783
select cast('221222222222' as datetime(3)) / 0; -- NULL
select 111111111111 / cast('221222222220' as datetime(3)); --0.0055
select 111111111111.111 / cast('221222222220' as datetime(3));--0.0054948
select 111111111111.1111 / cast('221222222220' as datetime(3));--0.00549478
select 0 / cast('221222222220' as datetime(3)); --0.0000

-- div
select cast('221222222222' as datetime(3)) div cast('221222222220' as datetime(3)); --1
select cast('221222222222' as datetime(3)) div 111; --182173173173
select cast('221222222222' as datetime(3)) div 111.1; -- 182009200920
select cast('221222222222' as datetime(3)) div 115; -- 175836714975
select cast('221222222222' as datetime(3)) div 0; -- NULL
select 111111111111 div cast('221222222220' as datetime(3)); --0
select 111111111111.111 div cast('221222222220' as datetime(3));--0
select 0 div cast('221222222220' as datetime(3)); --0
select cast('221222222222' as datetime(3)) div 115.1; --175683946326
select cast('221222222222' as datetime(3)) div 115.2; --175531442901
-- %
select cast('111111111111' as datetime(3)) % cast('111111111110' as datetime(3)); --1.000
select cast('111111111111' as datetime(3)) % 111; --20.000
select cast('221222222222' as datetime(3)) % 111.1; --10.000
select cast('111111111111' as datetime(3)) % 0; --NULL
select 111111111111 % cast('221222222220' as datetime(3)); --111111111111.000
select 111111111111.111 % cast('221222222220' as datetime(3)); --111111111111.111
select 0 % cast('221222222220' as datetime(3)); -- 0.000

-- mod
select cast('111111111111' as datetime(3)) mod cast('111111111110' as datetime(3)); --1.000
select cast('111111111111' as datetime(3)) mod 111; -- 20.000
select cast('221222222222' as datetime(3)) mod 111.1; --10.000
select cast('111111111111' as datetime(3)) mod 0; --NULL
select 111111111111 mod cast('221222222220' as datetime(3)); --111111111111.000
select 111111111111.111 mod cast('221222222220' as datetime(3)); -- 111111111111.111
select 0 mod cast('221222222220' as datetime(3)); --0.000
select cast('111111111111' as datetime(3)) MOD cast('111111111110' as datetime(3)); -- 1.000

-- &
select cast('111111111111' as datetime(3)) & cast('111111111110' as datetime(3)); --20111111111110
select cast('111111111111' as datetime(3)) & 111; --71
select cast('221222222222' as datetime(3)) & 111.1; --14
select cast('111111111111' as datetime(3)) & 0; --0
select 111111111111 & cast('221222222220' as datetime(3)); --511967620
select 111111111111.111 & cast('221222222220' as datetime(3)); --511967620
select 0 & cast('221222222220' as datetime(3)); --0
select cast('221222222222' as datetime(3)) & 1.4; --0
select cast('221222222222' as datetime(3)) & 1.5; --2

-- |
select cast('111111111111' as datetime(3)) | cast('111111111110' as datetime(3)); --20111111111111
select cast('111111111111' as datetime(3)) | 111; --20111111111151
select cast('221222222222' as datetime(3)) | 111.1; --20221222222319
select cast('111111111111' as datetime(3)) | 0; -- 20111111111111
select 111111111111 | cast('221222222220' as datetime(3)); --20331821365711
select 111111111111.111 | cast('221222222220' as datetime(3)); --20331821365711
select 0 | cast('221222222220' as datetime(3)); --20221222222220
select cast('221222222222' as datetime(3)) | 1.4; --20221222222223
select cast('221222222222' as datetime(3)) | 1.5; --20221222222222

-- ~
-- Need to be modified: 64-bit unsigned number after the number is inverted in mysql !!!!!!!
select ~cast('111111111111' as datetime(3)) ;-- 18446723962598440504

-- ^
select cast('111111111111' as datetime(3)) ^ cast('111111111110' as datetime(3)); --1
select cast('111111111111' as datetime(3)) ^ 111; --20111111111080
select cast('221222222222' as datetime(3)) ^ 111.1; --20221222222305
select cast('111111111111' as datetime(3)) ^ 0; --20111111111111
select 111111111111 ^ cast('221222222220' as datetime(3)); -- 20331309398091
select 111111111111.111 ^ cast('221222222220' as datetime(3)); -- 20331309398091
select 0 ^ cast('221222222220' as datetime(3)); --20221222222220
select cast('221222222222' as datetime(3)) ^ 1.4; --20221222222223
select cast('221222222222' as datetime(3)) ^ 1.5; --20221222222220

-- >>
select cast('111111111111' as datetime(3)) >> cast('111111111110' as datetime(3)); --0
select cast('111111111111' as datetime(3)) >> 111; --0
select cast('221222222222' as datetime(3)) >> 111.1; --0
select cast('111111111111' as datetime(3)) >> 0; --20111111111111
select 111111111111 >> cast('221222222220' as datetime(3)); --0
select 111111111111.111 >> cast('221222222220' as datetime(3)); --0
select 0 >> cast('221222222220' as datetime(3)); --0
select cast('221222222222' as datetime(3)) >> 1.4; --10110611111111
select cast('221222222222' as datetime(3)) >> 1.5; --5055305555555

-- <<
select cast('111111111111' as datetime(3)) << cast('111111111110' as datetime(3)); --0
select cast('111111111111' as datetime(3)) << 111; --0
select cast('221222222222' as datetime(3)) << 111.1; --0
select cast('111111111111' as datetime(3)) << 0; --20111111111111
select 111111111111 << cast('221222222220' as datetime(3)); --0
select 111111111111.111 << cast('221222222220' as datetime(3)); --0
select 0 << cast('221222222220' as datetime(3)); --0
select cast('221222222222' as datetime(3)) << 1.4; --40442444444444
select cast('221222222222' as datetime(3)) << 1.5; --80884888888888

-- bit_count
select bit_count(cast('111111111111' as datetime(3)));  --21
select bit_count(cast('111111' as datetime(3)));  --20
select bit_count(cast('99991231235959' as datetime(3)));  --25
select bit_count(cast('111111111111.4' as datetime(3)));  --21
select bit_count(cast('111111111111.5' as datetime(3)));  --19

-- &&
select cast('111111111111' as datetime(3)) && cast('111111111110' as datetime(3)); --1
select cast('111111111111' as datetime(3)) && 111; --1
select cast('221222222222' as datetime(3)) && 111.1; --1
select cast('111111111111' as datetime(3)) && 0; --0
select 111111111111 && cast('221222222220' as datetime(3)); --1
select 111111111111.111 && cast('221222222220' as datetime(3)); --1
select 0 && cast('221222222220' as datetime(3)); --0
select cast('221222222222' as datetime(3)) && 1.4; --1
select cast('221222222222' as datetime(3)) && 1.5; --1

-- ||
set sql_mode = '';
select cast('111111111111' as datetime(3)) || cast('111111111110' as datetime(3)); --1
select cast('111111111111' as datetime(3)) || 111; --1
select cast('221222222222' as datetime(3)) || 111.1; --1
select cast('111111111111' as datetime(3)) || 0; --1
select 111111111111 || cast('221222222220' as datetime(3)); --1
select 111111111111.111 || cast('221222222220' as datetime(3)); --1
select 0 || cast('221222222220' as datetime(3)); --1
select cast('221222222222' as datetime(3)) || 1.4; --1
select cast('221222222222' as datetime(3)) || 1.5; --1

reset sql_mode;
select cast('111111111111' as datetime(3)) || cast('111111111110' as datetime(3)); --1

-- !
select !cast('111111111111' as datetime(3)) ;--0

-- and or xor
select cast('221222222222' as datetime(3)) and cast('221222222222' as datetime(3) );--1
select cast('221222222222' as datetime(3)) and cast('221222222222' as datetime(3) ) and 0::boolean;--0

select cast('221222222222' as datetime(3)) or cast('221222222222' as datetime(3) );--1
select cast('221222222222' as datetime(3)) or cast('221222222222' as datetime(3) ) or 0::boolean;--1

select cast('221222222222' as datetime(3)) xor cast('221222222222' as datetime(3) );--0
select cast('221222222222' as datetime(3)) xor cast('221222222222' as datetime(3) ) xor 0::boolean;--0
select cast('221222222222' as datetime(3)) xor cast('221222222222' as datetime(3) ) xor 1::boolean;--1

-- between and
create table t1 (f1 datetime(3), f2 datetime(3), f3 datetime(3) );
insert into t1 values('2001-01-01','2001-01-01 01:01:01','2001-01-01 01:01:01');
insert into t1 values('2001-02-05','2001-02-05 00:00:00','2001-02-05 01:01:01');
insert into t1 values('2001-03-10','2001-03-09 01:01:01','2001-03-10 01:01:01');
insert into t1 values('2001-04-15','2001-04-15 00:00:00','2001-04-15 00:00:00');
insert into t1 values('2001-05-20','2001-05-20 01:01:01','2001-05-20 01:01:01');
select f2 from t1 where f2 between '2001-2-5' and '01-04-14';
select f1, f2, f3 from t1 where f1 between f2 and f3;
select f2 from t1 where '2001-04-10 12:34:56' between f2 and '01-05-01';--Need to be modified: text type conversion bit timestamp in mysql !!!!
--select f2 from t1 where '2001-04-10 12:34:56' between f2 and '01-05-01';
--         f2          
-----------------------
-- 2001-01-01 01:01:01
-- 2001-02-05 00:00:00
-- 2001-03-09 01:01:01
--(3 rows)
select f2 from t1 where cast('2001-04-10 12:34:56' as datetime(3)) between f2 and '01-05-01';
select f2, f3 from t1 where '01-03-10' between f2 and f3;
select f2, f3 from t1 where '01-03-10' not between f2 and f3;
drop table t1;

-- in
create table t1(f2 datetime(3));
insert into t1 values('01-01-01 00:00:00'),('01-02-03 12:34:56'),('02-04-06 11:22:33');
select * from t1 where f2 in ('01-01-01','01-02-03 12:34:56','01-02-03');
select * from t1 where f2 not in ('01-01-01','01-02-03 12:34:56','01-02-03');
drop table t1;

-- is
CREATE TABLE t1 (a DATETIME, b INT);
INSERT INTO t1 VALUES ('01-02-03 12:34:56',1), ('1999-05-10 00:00:00',2);
SELECT * FROM t1 WHERE a IS true;
SELECT * FROM t1 WHERE a IS not true;
SELECT * FROM t1 WHERE a IS NULL;
SELECT * FROM t1 WHERE a IS not NULL;
drop table t1;

-- like
create table t1 (f1 datetime(3));
insert into t1 values('2001-01-01 01:01:01');
insert into t1 values('9999-12-31 23:59:59');
insert into t1 values('9999-12-31 23:59:59.6');

select f1 from t1 where f1 like '2001-01-01 01:01:01'; --1
select f1 from t1 where f1 like cast('2001-01-01 01:01:01' as datetime(3)); --1
select f1 from t1 where f1 like 010101010101; --0
select f1 from t1 where f1 like '010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like 20010101010101; --0
select f1 from t1 where f1 like '20010101010101'; --0--Need to be modified: text type conversion bit timestamp in mysql!!!!!!
select f1 from t1 where f1 like cast('010101010101' as datetime(3)); --1
select f1 from t1 where f1 like cast(010101010101 as datetime(3)); --1
select f1 from t1 where f1 like cast(20010101010101 as datetime(3)); --1
select f1 from t1 where f1 like cast('20010101.010101' as datetime(3)); --1
select f1 from t1 where f1 like cast('99991231.2359595' as datetime(3));
drop table t1;

SET sql_mode = default;
--Type Conversion
select cast(111111111111 as datetime );
select cast('111111111111' as datetime );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as datetime );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as datetime );
select cast('11-11-11 11:11:11'::pg_catalog.date as datetime );
select cast('2020-08-02 07:30:40'::pg_catalog.date as datetime );
select cast('12-10-08 10:10:10' ::pg_catalog.date as datetime );

select cast(111111111111 as datetime(3) );
select cast('111111111111' as datetime(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamp as datetime(3) );
select cast('11-11-11 11:11:11'::pg_catalog.timestamptz as datetime(3) );
select cast('11-11-11 11:11:11'::pg_catalog.date as datetime(3) );

-- mysql type to mysql type
select cast(cast('11-11-11 11:11:11.111111' as date) as datetime );
select cast(cast('11-11-11 11:11:11.111111' as datetime) as datetime );
select cast(cast('11-11-11 11:11:11.111111' as datetime(4)) as datetime );
select cast(cast('11-11-11 11:11:11.111111' as timestamp) as datetime );
select cast(cast('11-11-11 11:11:11.111111' as timestamp(4)) as datetime );
--select cast(cast('11-11-11 11:11:11.111111' as time) as datetime );
--select cast(cast('11-11-11 11:11:11.111111' as time(4)) as datetime );

select cast(cast('11-11-11 11:11:11.111111' as date) as datetime(4) );
select cast(cast('11-11-11 11:11:11.111111' as datetime) as datetime(4) );
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as datetime(4) );
select cast(cast('11-11-11 11:11:11.111111' as timestamp) as datetime(4) );
select cast(cast('11-11-11 11:11:11.111111' as timestamp(5)) as datetime(4) );
--select cast(cast('11-11-11 11:11:11.111111' as time) as datetime(4) );
--select cast(cast('11-11-11 11:11:11.111111' as time(5)) as datetime(4) );

select cast(cast('11-11-11 11:11:11.111111' as datetime) as date);
select cast(cast('11-11-11 11:11:11.111111' as datetime) as datetime);
select cast(cast('11-11-11 11:11:11.111111' as datetime) as datetime(3));
select cast(cast('11-11-11 11:11:11.111111' as datetime) as timestamp);
select cast(cast('11-11-11 11:11:11.111111' as datetime) as timestamp(4));
select cast(cast('11-11-11 11:11:11.111111' as datetime) as time);
select cast(cast('11-11-11 11:11:11.111111' as datetime) as time(4));

select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as date);
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as datetime);
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as datetime(3));
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as timestamp);
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as timestamp(4));
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as time);
select cast(cast('11-11-11 11:11:11.111111' as datetime(5)) as time(4));


create table datetime_01(a1 datetime);
set sql_mode ='';
insert into datetime_01 values('1111111111111.1');
insert into datetime_01 values('111111111111111.1');


set sql_mode=default;
insert into datetime_01 values('1111111111111.1');
insert into datetime_01 values('111111111111111.1');
select * from datetime_01;

drop table datetime_01;


-- agg
CREATE TABLE t1(m1 datetime);
INSERT INTO t1 VALUES ('2001-04-10 01:01:01'),('2001-05-09 02:01:01'),('2001-06-10 04:01:01');
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

CREATE TABLE t1(m1 datetime(4));
INSERT INTO t1 VALUES ('2001-04-10 01:01:01.444444'),('2001-05-09 02:01:01.555555'),('2001-06-10 04:01:01.666666');
SELECT * FROM t1;
SELECT COUNT(*) AS total_rows, MIN(m1) AS min_value, MAX(m1) AS max_value FROM t1;
SELECT AVG(m1) AS avg_value FROM t1;
SELECT SUM(m1) AS sum_value FROM t1;
DROP TABLE t1;

-- bug#6345
CREATE TABLE tb_test ( a DOUBLE, b INT, c VARCHAR(50), d datetime, e BINARY(10) );

INSERT INTO tb_test VALUES
(3.14, 3, '3.14', '2001-04-10 01:01:01.444444', 0x03),
(1000.5, 1000, '1000.5', '2001-05-09 02:01:01.555555', 0x04),
(0.001, 0, '0.001', '2001-06-10 04:01:01.666666', 0x00),
(NULL, NULL, NULL, NULL, NULL);

SELECT d, CAST(d AS DOUBLE) , pg_typeof(CAST(d AS DOUBLE))  FROM tb_test;

SELECT d, CAST(d AS float4) , pg_typeof(CAST(d AS float4))  FROM tb_test;
drop TABLE tb_test;

CREATE TABLE tb_test ( a DOUBLE, b INT, c VARCHAR(50), d datetime(3), e BINARY(10) );

INSERT INTO tb_test VALUES
(3.14, 3, '3.14', '2001-04-10 01:01:01.444444', 0x03),
(1000.5, 1000, '1000.5', '2001-05-09 02:01:01.555555', 0x04),
(0.001, 0, '0.001', '2001-06-10 04:01:01.666666', 0x00),
(NULL, NULL, NULL, NULL, NULL);

SELECT d, CAST(d AS DOUBLE) , pg_typeof(CAST(d AS DOUBLE))  FROM tb_test;

SELECT d, CAST(d AS float4) , pg_typeof(CAST(d AS float4))  FROM tb_test;
drop TABLE tb_test;

set sql_mode='';
create table t1(a datetime);
insert into t1 values ('2001-04-10 01:01:01'),('2001-05-09 02:01:01'),('2001-06-10 04:01:01');
insert into t1 values ('2008-02-23 09:23:45'), ('2010-03-05 11:08:02');
insert into t1 values (' ');
insert into t1 values (NULL);
select * from t1 order by a;
DROP TABLE t1;

create table t1(a datetime(6));
insert into t1 values ('2001-04-10 01:01:01'),('2001-05-09 02:01:01'),('2001-06-10 04:01:01');
insert into t1 values ('2008-02-23 09:23:45'), ('2010-03-05 11:08:02');
insert into t1 values (' ');
insert into t1 values (NULL);
select * from t1 order by a;
DROP TABLE t1;
set sql_mode=default;

-- Begin - BUG#7487 
SELECT to_char('2026-05-15 10:30:45'::datetime, 'YYYY--MM');
SELECT to_char('2026-05-15 10:30:45'::datetime, 'YYYY-MM-DD HH24:MI:SS');
SELECT to_char('2026-05-15 10:30:45.123456'::datetime(6), 'YYYY--MM');
SELECT to_char('2026-05-15 10:30:45.123456'::datetime(6), 'YYYY-MM-DD HH24:MI:SS');
-- End - BUG#7487 