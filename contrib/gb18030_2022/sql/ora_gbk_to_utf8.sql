-- THIS FILE ENCODING WITH GBK !!!

-- create database with gbk encoding
CREATE DATABASE ora_gbk ENCODING='GBK' LC_COLLATE='C' LC_CTYPE='C' TEMPLATE='template0';
\c ora_gbk;
load 'gb18030_2022';
show server_encoding;

-- insert gbk data (> 0xfe50)
create table gbk_to_utf8_test(x text);
insert into gbk_to_utf8_test values('şPşQşRşSşTşUşVşWşXşYşZş[ş\ş]ş^ş_ş`şaşbşcşdşeşfşgşhşişjşkşlşmşnşoşpşqşrşsştşuşvşwşxşyşzş{ş|ş}ş~ş€şş‚şƒş„ş…ş†ş‡şˆş‰şŠş‹şŒşşşşş‘ş’ş“ş”ş•ş–ş—ş˜ş™şšş›şœşşşŸş ');

-- test GBK to UTF8
set client_encoding = 'UTF8';
show client_encoding;
select * from gbk_to_utf8_test;

-- finish
\c postgres
drop database ora_gbk
