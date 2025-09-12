--set character
set CHARACTER SET ucs2;	--err
set CHARACTER SET utf16;	--err
set CHARACTER SET utf16le;	--err
set CHARACTER SET utf16le;	--err
set CHARACTER SET utf32;	--err
set CHARACTER SET utf8;
set CHARACTER SET default;

set CHARSET ucs2;	--err
set CHARSET utf16;	--err
set CHARSET utf16le;	--err
set CHARSET utf16le;	--err
set CHARSET utf32;	--err
set CHARSET utf8;
set CHARSET default;

set mysql.at_or_atat = on;
--TC_IVYSQL_MYSQL_SET_ CHARACTER_001
SET CHARACTER SET utf8mb4;
SET CHARSET gbk;

--TC_IVYSQL_MYSQL_SET_ CHARACTER_002
SET CHARACTER SET  'utf8wertet'; --err
SET CHARSET  'utf8wertet'; --err

--TC_IVYSQL_MYSQL_SET_ CHARACTER_003
set CHARACTER SET 'utf8mb4';
set CHARACTER SET "gbk";
set CHARACTER SET 'utf8';

--TC_IVYSQL_MYSQL_SET_ CHARACTER_004
set CHARACTER SET DEFAULT;

--TC_IVYSQL_MYSQL_SET_ CHARACTER_005
set CHARACTER SET ucs2; --err
set CHARACTER SET utf16; --err
set CHARACTER SET utf16le; --err
set CHARACTER SET utf32; --err

set mysql.at_or_atat = off;
