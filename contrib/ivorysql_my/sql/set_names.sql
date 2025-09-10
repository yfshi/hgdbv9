--set names

--original
set names ucs2;	--err
set names utf16;	--err
set names utf16le;	--err
set names utf16le;	--err
set names utf32;	--err
set names utf8;
set names default;

--compatible
set names ucs2 COLLATE 'xxx';	--err
set names utf16 COLLATE 'xxx';	--err
set names utf16le COLLATE 'xxx';	--err
set names utf16le COLLATE 'xxx';	--err
set names utf32 COLLATE 'xxx';	--err
set names utf8 COLLATE default;

set names ucs2 COLLATE xxx;	--err
set names utf16 COLLATE xxx;	--err
set names utf16le COLLATE xxx;	--err
set names utf16le COLLATE xxx;	--err
set names utf32 COLLATE xxx;	--err
set names utf8 COLLATE default;

set mysql.at_or_atat = on;
--TC_IVYSQL_MYSQL_SET_ NAMES_001
set NAMES 'utf8mb4';
show client_encoding;  --UTF8

--TC_IVYSQL_MYSQL_SET_ NAMES_002
set NAMES 'utf8wertet'; --err

--TC_IVYSQL_MYSQL_SET_ NAMES_003
set NAMES 'utf8mb4'; --UTF8
set NAMES "gbk";	--GBK
set NAMES utf8;	--UTF8

--TC_IVYSQL_MYSQL_SET_ NAMES_004
SET NAMES 'utf8mb4';
select @@client_encoding;
SET NAMES 'gbk';
select @@client_encoding;

--TC_IVYSQL_MYSQL_SET_ NAMES_005 --no set
SET NAMES 'utf8mb4' COLLATE 'utf8mb4_unicode_ci';
select @@client_encoding;
SET NAMES 'utf8mb4' COLLATE 'utf32_unicode_ci';
select @@client_encoding;

--TC_IVYSQL_MYSQL_SET_ NAMES_006 --no set
set NAMES 'utf8mb4' COLLATE  'utf8mb4_unicode_ci';
select @@client_encoding;
set NAMES 'utf8mb4' COLLATE  "utf8mb4_general_ci";
select @@client_encoding;
set NAMES 'utf8mb4' COLLATE  utf8mb4_unicode_ci;
select @@client_encoding;

--TC_IVYSQL_MYSQL_SET_ NAMES_007
set NAMES DEFAULT;
select @@client_encoding;  --UTF8

--TC_IVYSQL_MYSQL_SET_ NAMES_008
set NAMES ucs2;  --err
set NAMES utf16; --err
set NAMES utf16le; --err
set NAMES utf32; --err

set mysql.at_or_atat = off;