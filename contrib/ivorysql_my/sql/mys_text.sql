set mysql.mysql_convert = true;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_001
create table tb(col national text);  -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_002
create table tb (col text character set utf32);  -- success
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_003
create table tb(col text collate armscii8_general_ci);
drop table if exists tb;

-- CREATE TABLE(col TEXT[M])
create table tb1(col text(0));  -- text
\d tb1
create table tb2(col text(255));  -- tinytext
\d tb2
create table tb3(col text(256)); -- text
\d tb3
create table tb4(col text(65535)); -- text
\d tb4
create table tb5(col text(65536)); -- mediumtext
\d tb5
create table tb6(col text(16777215)); -- mediumtext
\d tb6
create table tb7(col text(16777216)); -- longtext
\d tb7
create table tb8(col text(4294967295)); -- longtext
\d tb8
create table tb9(col text(4294967296)); -- fail
drop table if exists tb1,tb2,tb3,tb4,tb5,tb6,tb7,tb8,tb9;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_004
create table tb1(col text(0) charset latin1);  -- tinytext
\d tb1
create table tb2(col text(255) charset latin1); -- tinytext
\d tb2
create table tb3(col text(256) charset latin1); -- text
\d tb3
create table tb4(col text(65535) charset latin1); -- text
\d tb4
create table tb5(col text(65536) charset latin1); -- mediumtext
\d tb5
create table tb6(col text(16777215) charset latin1); -- mediumtext
\d tb6
create table tb7(col text(16777216) charset latin1); -- longtext
\d tb7
create table tb8(col text(4294967295) charset latin1); -- longtext
\d tb8
create table tb9(col text(4294967296) charset latin1); -- fail
drop table if exists tb,tb1,tb2,tb3,tb4,tb5,tb6,tb7,tb8,tb9;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_005
create table tb1(col text(0) charset gbk);  -- tinytext
\d tb1
create table tb2(col text(127) charset gbk);  -- tinytext
\d tb2
create table tb3(col text(128) charset gbk); -- text
\d tb3
create table tb4(col text(32767) charset gbk); -- text
\d tb4
create table tb5(col text(32768) charset gbk); -- mediumtext
\d tb5
create table tb6(col text(8388607) charset gbk); -- mediumtext
\d tb6
create table tb7(col text(8388608) charset gbk); -- longtext
\d tb7
create table tb8(col text(4294967295) charset gbk); -- longtext
\d tb8
create table tb9(col text(4294967296) charset gbk); -- fail
drop table if exists tb,tb1,tb2,tb3,tb4,tb5,tb6,tb7,tb8,tb9;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_006
create table tb(col text charset gbk);
insert into tb values(repeat('中',32767)); -- success
insert into tb values(repeat('中',32768)); -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_007
create table tb(col text charset utf8);
insert into tb values(repeat('中',21845)); -- success
insert into tb values(repeat('中',21846)); -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_008
create table tb1(col text(86) charset utf8);  -- text
\d tb1
create table tb2(col text(21845) charset utf8); --text
\d tb2
insert into tb1 values(repeat('中',21845)); -- success
insert into tb2 values(repeat('中',21845)); -- fail
drop table if exists tb1,tb2;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_SQLMODE_001
create table tb(col text charset utf8);
insert into tb values(repeat('中',21846)); -- fail
set sql_mode='';
insert into tb values(repeat('中',21846)); -- success
select char_length(col) from tb;  -- 21845
set sql_mode='TRADITIONAL';
insert into tb values(repeat('中',21846)); -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_SQLMODE_002
create table tb(id int,col text);
insert into tb values(1,'abc'); -- success
select char_length(col) from tb where id=1; -- 3
set sql_mode = 'PAD_CHAR_TO_FULL_LENGTH'; -- success
insert into tb values(2,'abc'); -- success
select char_length(col) from tb where id=2; -- 3
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_SQLMODE_003
create table tb(col text);
insert into tb values (' abcd'),('abc '),('abc  '),('abc   ');
select col,char_length(col) from tb where col=' abcd';
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_001
create table tb(id int,col1 text, col2 text);
insert into tb values(1,'a','b');
select col1+col2 from tb where id=1; -- 0
select col1-col2 from tb where id=1; -- 0
select col1*col2 from tb where id=1; -- 0
select col1/col2 from tb where id=1; -- NULL
select col1%col2 from tb where id=1; -- NULL
select NULL+col2 from tb where id=1; -- NULL
drop table if exists tb;

create table tb (col1 text, col2 text);
insert into tb (col1,col2) VALUES
    ('  -.1.2', '1.0'),('  +.1.2', '-2.0'),('  123abc', '3.0'),('  +123.45', '-2.0'),
    ('  -123.45','4.0'),('  0.1','-.0'),('  +0.1','1.0'),('  -0.1','2.0'),('  123','0.1.0'),
    ('  +123','-.0.5'),('  -123','9.0'),('  .1.2','5'),('  1.2.3','-0'),('  123abc456','-5'),
    ('  0x10','+.0'),('  1.23.45','+2.0'),('  ','-x2.0'),('  abc123','0.0'),('  -0.0','2.0'),
    ('  +.0','2.0'),('  0x10abc','2.0');
select col1+col2 from tb;
select col1-col2 from tb;
select col1*col2 from tb;
select col1/col2 from tb;
select col1%col2 from tb;
select NULL+col2 from tb;
select col1^col2 from tb;
select col1 xor col2 from tb;
set sql_mode='';
select col1 || col2 from tb;
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_002
create table tb (c1 int,c2 text);
insert into tb values(1,'a'),(2,'b'),(3,NULL);
select * from tb where c2='a';  -- (1,a)
select * from tb where c2>'a';  -- (2,b)
select * from tb where c2>='a';  -- (1,a)(2,b)
select * from tb where c2<'b';  -- (1,a)
select * from tb where c2<='b';  -- (1,a)(2,b)
select * from tb where c2<=>NULL;  -- (3,NULL)
select * from tb where c2<>'a';  -- (2,b)
select * from tb where c2!='a';  -- (2,b)
select * from tb where c2 between 'a' and 'b';  -- (1,a)(2,b)
select * from tb where c2 is null;  -- (3,NULL)
select * from tb where c2 is not null;  -- (1,a)(2,b)
select * from tb where isnull(c2);  -- (3,NULL) 
select * from tb where c2 in ('a','b');  -- (1,a)(2,b)
select * from tb where c2 not in ('a','b');  -- no result 
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_003
create table tb (col text);
insert into tb values('a');
select not col from tb;  -- 1
select !col from tb;  -- 1
select col and col from tb;  -- 0
select col && col from tb;  -- 0
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_004
create table tb (col text);
insert into tb values('JetPack 1000');
insert into tb values('JetPack 2000');
insert into tb values('JetPack 3000');
insert into tb values('1 ton anvil');
insert into tb values('2 ton anvil');
insert into tb values('.5 ton anvil');
insert into tb values('Furball Inc.');
insert into tb values('TNT (1 stick)');
insert into tb values('TNT (5 sticks)');
select * from tb where col regexp '1000';  -- JetPack 1000
select * from tb where col regexp '.000';  -- JetPack 1000, JetPack 2000, JetPack 3000  
select * from tb where col regexp '1000|2000';  -- JetPack 1000, JetPack 2000
select * from tb where col regexp '[123] Ton';  -- 1 ton anvil, 2 ton anvil
select * from tb where col regexp '[1-5] Ton';  -- 1 ton anvil, 2 ton anvil, .5 ton anvil
select * from tb where col regexp '\\.';  --  .5 ton anvil, Furball Inc. 
select * from tb where col regexp '\\([0-9] sticks?\\)';  -- TNT (1 stick), TNT (5 stick)
select * from tb where col regexp '^[0-9]';  -- 1 ton anvil, 2 ton anvil
select * from tb where col regexp '\\.$';  -- Furball Inc.
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_005
create table tb (col text);
insert into tb values('JetPack 1000');
insert into tb values('JetPack 2000');
insert into tb values('JetPack 3000');
insert into tb values('1 ton anvil');
insert into tb values('2 ton anvil');
insert into tb values('.5 ton anvil');
insert into tb values('Furball Inc.');
insert into tb values('TNT (1 stick)');
insert into tb values('TNT (5 sticks)');
select * from tb where col not regexp '1000';
select * from tb where col not regexp '.000';
select * from tb where col not regexp '1000|2000';
select * from tb where col not regexp '[123] Ton';
select * from tb where col not regexp '[1-5] Ton';
select * from tb where col not regexp '\\.';
select * from tb where col not regexp '\\([0-9] sticks?\\)';
select * from tb where col not regexp '^[0-9]';
select * from tb where col not regexp '\\.$';
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_006
create table tb (col text);
insert into tb values('JetPack 1000');
insert into tb values('JetPack 2000');
insert into tb values('JetPack 3000');
insert into tb values('1 ton anvil');
insert into tb values('2 ton anvil');
insert into tb values('.5 ton anvil');
insert into tb values('Furball Inc.');
insert into tb values('TNT (1 stick)');
insert into tb values('TNT (5 sticks)');
select * from tb where col rlike '1000';  -- JetPack 1000
select * from tb where col rlike '.000';  -- JetPack 1000, JetPack 2000, JetPack 3000
select * from tb where col rlike '1000|2000';  -- JetPack 1000, JetPack 2000
select * from tb where col rlike '[123] Ton';  -- 1 ton anvil, 2 ton anvil
select * from tb where col rlike '[1-5] Ton';  -- 1 ton anvil, 2 ton anvil, .5 ton anvil
select * from tb where col rlike '\\.';  --  .5 ton anvil, Furball Inc.
select * from tb where col rlike '\\([0-9] sticks?\\)';  -- TNT (1 stick), TNT (5 stick)
select * from tb where col rlike '^[0-9]';  -- 1 ton anvil, 2 ton anvil
select * from tb where col rlike '\\.$';  -- Furball Inc.
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_007
create table tb1(col text);
insert into tb1 values (' bc');
insert into tb1 values ('abc');
insert into tb1 values ('abcd');
insert into tb1 values ('bem');
insert into tb1 values ('abcdefg');
insert into tb1 values ('小巷人家');
insert into tb1 values ('三体');
insert into tb1 values ('Sam');
insert into tb1 values ('sam');
insert into tb1 values ('am');
select * from tb1 where col like 'a%';  -- abc, abcd, abcdefg, am
select * from tb1 where col like ' %';  -- bc
select * from tb1 where col like '%d';  -- abcd
select * from tb1 where col like '%e%';  -- bem, abcdefg
select * from tb1 where col like '%';  -- all rows
select * from tb1 where col like '_m';  -- am
select * from tb1 where col like 'be_';  -- bem
select * from tb1 where col like '_b_';  -- bc, abc
select * from tb1 where col like '%_';  -- all rows
select * from tb1 where col like '_%';  -- all rows
select * from tb1 where col like '%人%';  -- 小巷人家
select * from tb1 where col like 'S%';  -- Sam
select * from tb1 where col not like 'a%';
select * from tb1 where col not like ' %';
select * from tb1 where col not like '%d';
select * from tb1 where col not like '%e%';
select * from tb1 where col not like '%';
select * from tb1 where col not like '_m';
select * from tb1 where col not like 'be_';
select * from tb1 where col not like '_b_';
select * from tb1 where col not like '%_';
select * from tb1 where col not like '_%';
select * from tb1 where col not like '%人%';
select * from tb1 where col not like 'S%';
drop table if exists tb1;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_008
create table tb1(col text);
insert into tb1 values (' bc');
insert into tb1 values ('abc');
insert into tb1 values ('abcd');
insert into tb1 values ('bem');
insert into tb1 values ('abcdefg');
insert into tb1 values ('小巷人家');
insert into tb1 values ('三体');
insert into tb1 values ('Sam');
insert into tb1 values ('sam');
insert into tb1 values ('am');
select * from tb1 where col like binary 'a%';  -- abc,abcd,abcdefg,am
select * from tb1 where col like binary ' %';  -- bc
select * from tb1 where col like binary '%d';  -- abcd
select * from tb1 where col like binary '%e%';  -- bem,abcdefg
select * from tb1 where col like binary '%';  -- all
select * from tb1 where col like binary '_m';  -- am
select * from tb1 where col like binary 'be_';  -- bem
select * from tb1 where col like binary '_b_';  -- bc,abc
select * from tb1 where col like binary '%_';  -- all rows
select * from tb1 where col like binary '_%';  -- all rows
select * from tb1 where col like binary '%人%';  -- 小巷人家
select * from tb1 where col like binary 'S%';  -- Sam
select * from tb1 where col not like binary 'a%';
select * from tb1 where col not like binary ' %';
select * from tb1 where col not like binary '%d';
select * from tb1 where col not like binary '%e%';
select * from tb1 where col not like binary '%';
select * from tb1 where col not like binary '_m';
select * from tb1 where col not like binary 'be_';
select * from tb1 where col not like binary '_b_';
select * from tb1 where col not like binary '%_';
select * from tb1 where col not like binary '_%';
select * from tb1 where col not like binary '%人%';
select * from tb1 where col not like binary 'S%';
drop table if exists tb1;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_OPER_009
create table tb1(col text);
insert into tb1 values (' ab%m');
insert into tb1 values ('123m_ab');
insert into tb1 values ('ert#qw');
insert into tb1 values ('fa\fsd');
insert into tb1 values('x''y');
select * from tb1 where col like '%\\%%' escape '\\';  -- ab%m
select * from tb1 where col like '%\\_%' escape '\\';  -- 123m_ab
select * from tb1 where col like '%\\#%' escape '\\';  -- ert#qw
select * from tb1 where col like "%\'%" escape '\\';  --  x'y
select * from tb1 where col like '%+%%' escape '+';  -- ab%m
select * from tb1 where col like '%++%%' escape '++';  -- error “Incorrect arguments to ESCAPE”
drop table if exists tb1;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_BINARY_001
create table tb (col1 text,col2 text);
insert into tb values('a','A');
select col1=col2 from tb;  -- 1
select binary col1=binary col2 from tb;  -- 0
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_001
create table tb (col text);
insert into tb values('xy');
select cast(col as binary) from tb;  -- 0x7879
select cast(col as binary(0)) from tb;  -- 0x
select cast(col as binary(10)) from tb;  -- 0x78790000000000000000
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_002
set sql_mode='';
create table tb (col text);
insert into tb values('xy');
select cast(col as char), char_length(cast(col as char)) from tb;  -- ('xy',2)
select cast(col as char(0)), char_length(cast(col as char(0)))  from tb;  -- (,0)
select cast(col as char(10)), char_length(cast(col as char(10)))  from tb;  -- ('xy',2)
select cast(col as char(10)), char_length(cast(col as char(10) charset utf16))  from tb;  -- ('xy',2)
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_003
create table tb (id int,col text);
insert into tb values(1,'20240628');
insert into tb values(2,'2024-06-28');
insert into tb values(3,'2024/06/28');
insert into tb values(4,'2024-06-28 12:12:12');
insert into tb values(5,'06282024');
select cast(col as date) from tb where id=1;  -- 2024-06-28
select cast(col as date) from tb where id=2;  -- 2024-06-28
select cast(col as date) from tb where id=3;  -- 2024-06-28
select cast(col as date) from tb where id=4;  -- 2024-06-28
select cast(col as date) from tb where id=5;  -- NULL
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_004
create table tb (id int,col text);
insert into tb values(1,'2024-06-28 12:12:12');
insert into tb values(2,'2024-06-28 12:12:12');
insert into tb values(3,'20240628 12:12:12');
insert into tb values(4,'123');
select cast(col as datetime) from tb where id=1;  -- 2024-06-28 12:12:12
select cast(col as datetime(6)) from tb where id=2; -- 2024-06-28 12:12:12.000000
select cast(col as datetime) from tb where id=3;  -- NULL
select cast(col as datetime) from tb where id=4;  -- NULL
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_005
create table tb (col text);
insert into tb values('123.4');
select cast(col as decimal(5)) from tb;  -- 123
select cast(col as decimal(5,1)) from tb;  -- 123.4
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_006
create table tb(col text);
insert into tb values('{"name":"CoderAcademy", "age": 30}');
select cast(col as json) from tb;  -- {"age": 30, "name": "CoderAcademy"}
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_007
set sql_mode='';
create table tb (col text);
insert into tb values('xy');
select cast(col as nchar), char_length(cast(col as nchar)) from tb;  -- ('xy',2)
select cast(col as nchar(0)), char_length(cast(col as nchar(0))) from tb;  -- (,0)
select cast(col as nchar(10)), char_length(cast(col as nchar(10))) from tb;  -- ('xy',2)
select cast(col as nchar(10)), char_length(cast(col as nchar(10) charset utf16)) from tb;  -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_009
create table tb (id int,col text);
insert into tb values(1,'abc');
insert into tb values(2,'123');
insert into tb values(3,'123.4');
select cast(col as integer) from tb where id=1;  -- 0
select cast(col as integer) from tb where id=2;  -- 123
select cast(col as integer) from tb where id=3;  -- 123
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_010
create table tb (id int,col text);
insert into tb values(1,'123');
insert into tb values(2,'12-12-12');
insert into tb values(3,'12:12:12');
select cast(col as time) from tb where id=1;  -- 00:01:23
select cast(col as time) from tb where id=2;  -- 00:00:12
select cast(col as time) from tb where id=3;  -- 12:12:12
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_012
create table tb1 as select cast('abc'::text as binary(10));  -- binary
\d+ tb1
create table tb2 as select cast('abc'::text as char(10));  -- char(10)
\d+ tb2
create table tb3 as select cast('2024-06-28'::text as date);  -- date
\d+ tb3
create table tb4 as select cast('2024-06-28 12:12:12'::text as datetime);  -- datetime
\d+ tb4
create table tb5 as select cast('123.4'::text as decimal(5,1));  -- decimal(5,1)
\d+ tb5
create table tb6 as select cast('{"name":"CoderAcademy", "age": 30}'::text as json);  -- json
\d+ tb6
create table tb7 as select cast('abc'::text as nchar(5));  -- nchar(5)
\d+ tb7
create table tb8 as select cast('12:12:12'::text as time(6));  -- time(6)
\d+ tb8
drop table if exists tb1,tb2,tb3,tb4,tb5,tb6,tb7,tb8;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CAST_013
create table tb1 as select cast('abc'::text as xml);  -- xml
\d+ tb1
create table tb2 as select cast('abc'::text as name);  -- name
\d+ tb2
create table tb3 as select cast('2024-06-28 12:12:12'::text as text);  -- text
\d+ tb3
create table tb4 as select cast('123.4'::text as bpchar);  -- bpchar
\d+ tb4
create table tb5 as select cast('{"name":"CoderAcademy", "age": 30}'::text as varchar); -- varchar
\d+ tb5
drop table if exists tb1,tb2,tb3,tb4,tb5;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_001
set mysql.cast_to_char = on;
create table tb (col text);
insert into tb values('abc');
select convert('abc' using gbk);  -- success
select convert('abc' using gb18030);  -- success
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_002
create table tb (col text);
insert into tb values('xy');
select convert(col , binary) from tb;  -- 0x7879
select convert(col , binary(0)) from tb;  -- 0x
select convert(col , binary(10)) from tb;  -- 0x78790000000000000000
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_003
create table tb (col text);
insert into tb values('xy');
select convert(col , char), char_length(convert(col , char)) from tb;  -- ('xy',2)
select convert(col , char(0)), char_length(convert(col , char(0)))  from tb;  -- (,0)
select convert(col , char(10)), char_length(convert(col , char(10)))  from tb;  -- ('xy',2)
select convert(col , char(10)), char_length(convert(col , char(10) charset utf16))  from tb;  -- ('xy',2)
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_004
create table tb (id int,col text);
insert into tb values(1,'20240628');
insert into tb values(2,'2024-06-28');
insert into tb values(3,'2024/06/28');
insert into tb values(4,'2024-06-28 12:12:12');
insert into tb values(5,'06282024');
select convert(col , date) from tb where id=1;  -- 2024-06-28
select convert(col , date) from tb where id=2;  -- 2024-06-28
select convert(col , date) from tb where id=3;  -- 2024-06-28
select convert(col , date) from tb where id=4;  -- 2024-06-28
select convert(col , date) from tb where id=5;  -- NULL
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_005
create table tb (id int,col text);
insert into tb values(1,'2024-06-28 12:12:12');
insert into tb values(2,'2024-06-28 12:12:12');
insert into tb values(3,'20240628 12:12:12');
insert into tb values(4,'123');
select convert(col , datetime) from tb where id=1;  -- 2024-06-28 12:12:12
select convert(col , datetime(6)) from tb where id=2;  -- 2024-06-28 12:12:12.000000
select convert(col , datetime) from tb where id=3;  -- NULL
select convert(col , datetime) from tb where id=4;  -- NULL
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_006
create table tb (col text);
insert into tb values('123.4');
select convert(col , decimal(5)) from tb;  -- 123
select convert(col , decimal(5,1)) from tb;  -- 123.4
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_007
create table tb(col text);
insert into tb values('{"name":"CoderAcademy", "age": 30}');
select convert(col , json) from tb;  -- {"age": 30, "name": "CoderAcademy"}
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_008
create table tb (col text);
insert into tb values('xy');
select convert(col , nchar), char_length(convert(col , nchar)) from tb;  -- ('xy',2)
select convert(col , nchar(0)), char_length(convert(col , nchar(0))) from tb;  -- (,0)
select convert(col , nchar(10)), char_length(convert(col , nchar(10))) from tb;  -- ('xy',2)
select convert(col , nchar(10)), char_length(convert(col , nchar(10) charset utf16)) from tb; -- fail
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_009
create table tb (id int,col text);
insert into tb values(1,'abc');
insert into tb values(2,'123');
insert into tb values(3,'123.4');
select convert(col , signed) from tb where id=1;  -- 0
select convert(col , signed) from tb where id=2;  -- 123
select convert(col , signed) from tb where id=3;  -- 123
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_010
create table tb (id int,col text);
insert into tb values(1,'abc');
insert into tb values(2,'123');
insert into tb values(3,'123.4');
select convert(col , integer) from tb where id=1;  -- 0
select convert(col , integer) from tb where id=2;  -- 123
select convert(col , integer) from tb where id=3;  -- 123
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_011
create table tb (id int,col text);
insert into tb values(1,'123');
insert into tb values(2,'12-12-12');
insert into tb values(3,'12:12:12');
select convert(col , time) from tb where id=1;  -- 00:01:23
select convert(col , time) from tb where id=2;  -- 00:00:12
select convert(col , time) from tb where id=3;  -- 12:12:12
drop table if exists tb;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_012
create table tb (id int,col text);
insert into tb values(1,'abc');
insert into tb values(2,'123');
insert into tb values(3,'123.4');
select convert(col ,unsigned) from tb where id=1;  -- 0
select convert(col , unsigned) from tb where id=2;  -- 123
select convert(col , unsigned) from tb where id=3;  -- 123
drop table if exists tb;
set mysql.cast_to_char = off;
-- TC_IVYSQL_MSQL_DATATYPE_TEXT_CONVERT_013
create table tb (col text);
insert into tb values('123');
create table tb1 as select col+ 1 as result from tb ;  -- double
\d+ tb1
create table tb2 as select col+ 1.2 as result from tb ;  -- double
\d+ tb2
create table tb3 as select col='123' as result from tb ;  -- int
\d+ tb3
drop table if exists tb,tb1,tb2,tb3;

-- TC_IVYSQL_MSQL_DATATYPE_TEXT_INDEX_001
create table tb(c1 text, c2 text, c3 text, c4 text);
insert into tb values('a', 'ab', 'abc', 'abcd');
set enable_seqscan =off;
create index ind1 on tb using btree(c1);
explain (costs false) select * from tb where c1='a';   -- ind1
create index ind2 on tb using hash(c2);
explain (costs false) select * from tb where c2='ab';  -- ind2
drop table if exists tb cascade;

reset mysql.mysql_convert;

-- bug#z5157
create table tb9(col text(4294967296));

-- cast to integer
create table tb10(a int, b text, c varchar(20));
insert into tb10 values ('-1', '-1', '-1');
insert into tb10 values ('24.4', '24.4', '24.4');
insert into tb10 values ('24.5', '24.5', '24.5');

select a, cast(b as int), cast(c as int) from tb10;                     -- [-1, -1, -1], [24, 24, 24], [25, 24, 24]
select a, cast(b as unsigned), cast(c as unsigned) from tb10;   -- [-1, 4294967295, 4294967295], [24, 24, 24], [25, 24, 24]
select a, cast(b as int8), cast(c as int8) from tb10;                   -- [-1, -1, -1], [24, 24, 24], [25, 24, 24]
select a, cast(b as int8 unsigned), cast(c as int8 unsigned) from tb10; -- [-1, 18446744073709551615, 18446744073709551615], [24, 24, 24], [25, 24, 24]
drop table tb10;

-- text to uint1
create table t_text(a text);
create table t_utinyint(a tinyint unsigned);
insert into t_text values ('a');

set sql_mode = '';
insert into t_utinyint select * from t_text;

set sql_mode = default;
insert into t_utinyint select * from t_text;

select * from t_utinyint;

drop table t_text;
drop table t_utinyint;


-- text to uint2
create table t_text(a text);
create table t_uint2_1(a smallint unsigned);
insert into t_text values ('a');

set sql_mode = '';
insert into t_uint2_1 select * from t_text;

set sql_mode = default;
insert into t_uint2_1 select * from t_text;

select * from t_uint2_1;

drop table t_text;
drop table t_uint2_1;

-- text to uint4
create table t_text(a text);
create table t_uint4_1(a int unsigned);
insert into t_text values ('a');

set sql_mode = '';
insert into t_uint4_1 select * from t_text;

set sql_mode = default;
insert into t_uint4_1 select * from t_text;

select * from t_uint4_1;

drop table t_text;
drop table t_uint4_1;

-- text to uint8
create table t_text(a text);
create table t_uint8_1(a bigint unsigned);
insert into t_text values ('a');

set sql_mode = '';
insert into t_uint8_1 select * from t_text;

set sql_mode = default;
insert into t_uint8_1 select * from t_text;

select * from t_uint8_1;

drop table t_text;
drop table t_uint8_1;