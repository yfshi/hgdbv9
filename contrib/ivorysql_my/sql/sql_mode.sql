-- check default value
SHOW sql_mode;

-- check single value
SET sql_mode = 'ANSI_QUOTES';
SHOW sql_mode;

SET sql_mode = 'IGNORE_SPACE';
SHOW sql_mode;

-- check multi value
SET sql_mode = 'ANSI_QUOTES,IGNORE_SPACE';
SHOW sql_mode;

-- check case sensitive
SET sql_mode = 'ansi,ANSI_quotes,ignore_SPACE';
SHOW sql_mode;

-- check combination sql modes
SET sql_mode = 'ANSI';
SHOW sql_mode;

SET sql_mode = 'DB2';
SHOW sql_mode;

SET sql_mode = 'MAXDB';
SHOW sql_mode;

SET sql_mode = 'MSSQL';
SHOW sql_mode;

SET sql_mode = 'MYSQL323';
SHOW sql_mode;

SET sql_mode = 'MYSQL40';
SHOW sql_mode;

SET sql_mode = 'ORACLE';
SHOW sql_mode;

SET sql_mode = 'POSTGRESQL';
SHOW sql_mode;

SET sql_mode = 'TRADITIONAL';
SHOW sql_mode;

-- check exception value
SET sql_mode = 'ddd'; -- error

-- check exception value with space
SET sql_mode = 'ANSI_QUOTES, IGNORE_SPACE'; -- error

-- check backquote identifier
CREATE TABLE `sql_mode_test`(id int);
DROP TABLE `sql_mode_test`;

select number_to_sql_mode(1 << 0);
select number_to_sql_mode(1 << 1);
select number_to_sql_mode(1 << 2);
select number_to_sql_mode(1 << 3);
select number_to_sql_mode(1 << 4);
select number_to_sql_mode(1 << 5);
select number_to_sql_mode(1 << 6);
select number_to_sql_mode(1 << 7);
select number_to_sql_mode(1 << 8);
select number_to_sql_mode(1 << 9);
select number_to_sql_mode(1 << 10);
select number_to_sql_mode(1 << 11);
select number_to_sql_mode(1 << 12);
select number_to_sql_mode(1 << 13);
select number_to_sql_mode(1 << 14);
select number_to_sql_mode(1 << 15);
select number_to_sql_mode(1 << 16);
select number_to_sql_mode(1 << 17);
select number_to_sql_mode(1 << 18);
select number_to_sql_mode(1 << 19);
select number_to_sql_mode(1 << 20);
select number_to_sql_mode(1 << 21);
select number_to_sql_mode(1 << 22);
select number_to_sql_mode(1 << 23);
select number_to_sql_mode(1 << 24);
select number_to_sql_mode(1 << 25);
select number_to_sql_mode(1 << 26);
select number_to_sql_mode(1 << 27);
select number_to_sql_mode(1 << 28);
select number_to_sql_mode(1 << 29);
select number_to_sql_mode(1 << 30);
select number_to_sql_mode(1 << 31);
select number_to_sql_mode(1 << 32);
select number_to_sql_mode(15);
