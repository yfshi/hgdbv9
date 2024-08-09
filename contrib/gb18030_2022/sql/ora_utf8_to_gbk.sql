-- TEST CREATE DATABASE
create database utf8_enc encoding='utf8' LC_COLLATE='C' LC_CTYPE ='C' TEMPLATE=template0;
\c utf8_enc
load 'gb18030_2022';
show server_encoding;

-- insert into UTF8 data whose gbk encoding greater than 0xfe50
create table utf8_to_gbk_test(x text);
insert into utf8_to_gbk_test values('');

-- test UTF8 to GBK
set client_encoding = 'GBK';
show client_encoding;
select * from utf8_to_gbk_test;

-- finish
\c postgres;
drop database utf8_enc;
