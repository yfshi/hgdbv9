-- TEST CREATE DATABASE
create database gbk_enc encoding='gbk' LC_COLLATE='C' LC_CTYPE ='C' TEMPLATE=template0;
\c gbk_enc
load 'gb18030_2022';


show server_encoding;
set client_encoding = 'UTF8';
show client_encoding;

-- 1. insert test character two-byte：
create table gbk_test(x text);
insert into gbk_test values ('狜狝狟狢狣狤狥狦狧狪狫狵狶狹狽狾狿猀猂猄猅猆猇猈
猉猋猌猍猏猐猑猒猔猘猙猚猟猠猣猤猦猧猨猭猯猰猲猳猵
猶猺猻猼猽獀獁獂獃獄獅獆獇獈獉獊獋獌獎獏獑獓獔獕獖
獘獙獚獛獜獝獞獟獡獢獣獤獥獦獧獨獩獪獫獮獰獱');

select * from gbk_test;



-- 2.convert to utf8 encoding
select convert_to(x, 'utf8') from gbk_test ;



-- 3. test gb18030 stuff
-- E021-03 character string literals
SELECT 'first line'
' - next line'
	' - third line'
	AS "Three lines to one";
 
-- illegal string continuation syntax
SELECT 'first line'
' - next line' /* this comment is not allowed here */
' - third line'
	AS "Illegal comment within continuation";
 
-- Unicode escapes
SET standard_conforming_strings TO on;
 
SELECT U&'d\0061t\+000061' AS U&"d\0061t\+000061";
SELECT U&'d!0061t\+000061' UESCAPE '!' AS U&"d*0061t\+000061" UESCAPE '*';
 
-- bytea
SET bytea_output TO hex;
SELECT E'\\xDeAdBeEf'::bytea;
SELECT E'\\x De Ad Be Ef '::bytea;
SELECT E'\\xDeAdBeE'::bytea;
SELECT E'\\xDeAdBeEx'::bytea;
SELECT E'\\xDe00BeEf'::bytea;
SELECT E'DeAdBeEf'::bytea;
SELECT E'De\\000dBeEf'::bytea;
SELECT E'De\123dBeEf'::bytea;
SELECT E'De\\123dBeEf'::bytea;
SELECT E'De\\678dBeEf'::bytea;
 
SET bytea_output TO escape;
SELECT E'\\xDeAdBeEf'::bytea;
SELECT E'\\x De Ad Be Ef '::bytea;
SELECT E'\\xDe00BeEf'::bytea;
SELECT E'DeAdBeEf'::bytea;
SELECT E'De\\000dBeEf'::bytea;
SELECT E'De\\123dBeEf'::bytea;
 
SET bytea_output TO hex;
 
SELECT CAST(name 'namefield' AS text) AS "text(name)";
 
-- E021-09 trim function
SELECT TRIM(BOTH FROM '  bunch o blanks  ') = 'bunch o blanks' AS "bunch o blanks";
 
-- E021-06 substring expression
SELECT SUBSTRING('1234567890' FROM 3) = '34567890' AS "34567890";
 
-- PostgreSQL extension to allow using back reference in replace string;
SELECT regexp_replace('1112223333', E'(\\d{3})(\\d{3})(\\d{4})', E'(\\1) \\2-\\3');
 
-- set so we can tell NULL from empty string
\pset null '\\N'
 
-- return all matches from regexp
SELECT regexp_matches('foobarbequebaz', $re$(bar)(beque)$re$);
 
-- split string on regexp
SELECT foo, length(foo) FROM regexp_split_to_table('the quick brown fox jumped over the lazy dog', $re$\s+$re$) AS foo;
SELECT regexp_split_to_array('the quick brown fox jumped over the lazy dog', $re$\s+$re$);
 
-- change NULL-display back
\pset null ''
 
-- E021-11 position expression
SELECT POSITION('4' IN '1234567890') = '4' AS "4";
 
SELECT POSITION('5' IN '1234567890') = '5' AS "5";
 
-- T312 character overlay function
SELECT OVERLAY('abcdef' PLACING '45' FROM 4) AS "abc45f";
 
-- E061-04 like predicate
SELECT 'hawkeye' LIKE 'h%' AS "true";
SELECT 'hawkeye' NOT LIKE 'h%' AS "false";
 
-- unused escape character
SELECT 'hawkeye' LIKE 'h%' ESCAPE '#' AS "true";
SELECT 'hawkeye' NOT LIKE 'h%' ESCAPE '#' AS "false";
 
--
-- test ILIKE (case-insensitive LIKE)
-- Be sure to form every test as an ILIKE/NOT ILIKE pair.
--
 
SELECT 'hawkeye' ILIKE 'h%' AS "true";
SELECT 'hawkeye' NOT ILIKE 'h%' AS "false";


--4. str function
select repeat('中国', 3);
 
select left('中国！number1', 7);
 
select length('中国！number1');
 
select reverse('中国！number1');
 
select md5('中国！number1');
 
-- test strpos
SELECT strpos('abcdef', 'cd') AS "pos_3";
SELECT strpos('abcdef', 'xy') AS "pos_0";
 
SELECT replace('yabadabadoo', 'ba', '123') AS "ya123da123doo";
 
select split_part('joeuser@mydatabase','@',3) AS "empty string";
 
select to_hex(256::bigint*256::bigint*256::bigint*256::bigint - 1) AS "ffffffff";
 
select ascii('xyz');
select ascii('中xyz');

\c postgres
drop database gbk_enc;