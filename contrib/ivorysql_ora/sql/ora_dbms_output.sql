--
-- Regression tests for Oracle dbms_output package
--

-- without call enable(), put_line not work.
CREATE TABLE dbms_output_test(buff varchar2(1024), status int);
DECLARE
	buff varchar2(1024);
	status int;
BEGIN	
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- with enable(), put_line and get_line can work.
DECLARE
	buff varchar2(1024);
	status int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- disable() will lose the content already put into the buffer.
DECLARE
	buff varchar2(1024);
	status int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- the buffer of dbms_output life is session level.
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
END;
/
DECLARE
	buff varchar2(1024);
	status int;
BEGIN
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;


-- enable serveroutput does not print the content that has already been read.
-- so the table dbms_output_test has one tupe "ORAFCE TEST 1", serveroutput only print "ORAFCE TEST 2".
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
END;
/

SET SERVEROUTPUT ON

DECLARE
	buff varchar2(1024);
	status int;
BEGIN
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;
SET SERVEROUTPUT OFF

-- the test result is the same as above except that the contents of buffer are written after serveroutput starts.
SET SERVEROUTPUT ON

DECLARE
	buff varchar2(1024);
	status int;
BEGIN	
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;
SET SERVEROUTPUT OFF

-- after diable() buffer is empty, so serveroutput print nothing and get_line read failed.
SET SERVEROUTPUT ON

DECLARE
	buff varchar2(1024);
	status int;
BEGIN	
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE('ORAFCE TEST 2');
	DBMS_OUTPUT.GET_LINE(buff,status);
	INSERT INTO dbms_output_test VALUES (buff, status);	
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;
SET SERVEROUTPUT OFF

-- put_line('') = new_line()
SET SERVEROUTPUT ON
BEGIN
	DBMS_OUTPUT.PUT ('ORA');
	DBMS_OUTPUT.PUT ('F');
	DBMS_OUTPUT.PUT ('CE');
	DBMS_OUTPUT.PUT_LINE ('');
	DBMS_OUTPUT.PUT ('ABC');
	DBMS_OUTPUT.PUT_LINE ('');
END;
/
SET SERVEROUTPUT OFF

-- after get_line, the buffer become dirty and the buffer will be reset when put/put_line data again.
DECLARE
	buff varchar2(1024);
	status int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE ('ORAFCE TEST 1');
	DBMS_OUTPUT.PUT_LINE ('ORAFCE TEST 2');
	DBMS_OUTPUT.GET_LINE(buff, status);
	INSERT INTO dbms_output_test VALUES (buff, status);
	DBMS_OUTPUT.PUT_LINE ('ORAFCE TEST 3');
	DBMS_OUTPUT.GET_LINE(buff, status);
	INSERT INTO dbms_output_test VALUES (buff, status);
	DBMS_OUTPUT.GET_LINE(buff, status);
	INSERT INTO dbms_output_test VALUES (buff, status);
	DBMS_OUTPUT.GET_LINE(buff, status);
	INSERT INTO dbms_output_test VALUES (buff, status);
	DBMS_OUTPUT.DISABLE();
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- new_line() generate a single empty line.
DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE ('ORAFCE TEST 1');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.PUT_LINE ('ORAFCE TEST 3');
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- after all the lines in the buffer have been read, further reading fails and the status is 1.
DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- get_line read the lines of the buffer using the new line character as the delimiter.
-- data without new line character will not be read.
DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT('aa');
	DBMS_OUTPUT.PUT('bb');
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- empty lines can be read correctly by get_line, but will not be printed by serveroutput. 
SET SERVEROUTPUT ON
BEGIN
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE(NULL);
END;
/

BEGIN
	DBMS_OUTPUT.PUT_LINE('line 1');
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE('line 2');
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE('line 3');
END;
/

BEGIN
	DBMS_OUTPUT.PUT_LINE('line 1');
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.PUT_LINE('line 2');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.PUT_LINE('line 3');
END;
/
SET SERVEROUTPUT OFF

DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.PUT_LINE(NULL);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT_LINE('');
	DBMS_OUTPUT.PUT_LINE('');
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- put(null) do nothing.
DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT('a');
	DBMS_OUTPUT.PUT(NULL);
	DBMS_OUTPUT.PUT('b');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- as long as the buffer is not dirty, the put content is retained, so the test result is 'abc'.
BEGIN
DBMS_OUTPUT.ENABLE();
END;
/

BEGIN
	DBMS_OUTPUT.PUT('a');
	DBMS_OUTPUT.PUT('b');
END;
/

DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.PUT('c');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- same as above, use serveroutput to dirty the buffer, so the test result is 'c'.
SET SERVEROUTPUT ON

BEGIN
	DBMS_OUTPUT.PUT('a');
	DBMS_OUTPUT.PUT('b');
END;
/

DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.PUT('c');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;
SET SERVEROUTPUT OFF

-- same as above.
SET SERVEROUTPUT ON

BEGIN
	DBMS_OUTPUT.PUT('a');
	DBMS_OUTPUT.PUT('b');
END;
/

BEGIN
	DBMS_OUTPUT.PUT('c');
	DBMS_OUTPUT.NEW_LINE();
END;
/
SET SERVEROUTPUT OFF

SET SERVEROUTPUT ON
BEGIN
	DBMS_OUTPUT.PUT_LINE('AAAAA');
	DBMS_OUTPUT.PUT('bbb');
END;
/

BEGIN
	DBMS_OUTPUT.PUT('c');
	DBMS_OUTPUT.NEW_LINE();
END;
/
SET SERVEROUTPUT OFF

-- enable serveroutput means implicit automatic read buffer.
SET SERVEROUTPUT ON
DECLARE
	buff varchar2(1024) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.PUT_LINE('aaa');
	DBMS_OUTPUT.PUT_LINE('bbb');
	DBMS_OUTPUT.PUT_LINE('ccc');
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;
SET SERVEROUTPUT OFF

SET SERVEROUTPUT ON
BEGIN
	DBMS_OUTPUT.PUT_LINE('aaa');
	DBMS_OUTPUT.PUT_LINE('bbb');
	DBMS_OUTPUT.PUT_LINE('ccc');
END;
/

BEGIN
	DBMS_OUTPUT.PUT_LINE('ffff');
	DBMS_OUTPUT.PUT_LINE('gggg');
END;
/
SET SERVEROUTPUT OFF

-- buffer_size greater max value(1000000), it's equivalent to max value instead of reporting an error.
BEGIN
	DBMS_OUTPUT.ENABLE(10000001);
END;
/

-- enable(null), buffer_size is unlimited.
-- different from oracle, our unlimited limit is equal to 1GB.
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(NULL);
	buff := rpad('a', 10, 'b');
	for i in 1..1000000 loop
		DBMS_OUTPUT.PUT_LINE(i || ' = ' || buff);
	end loop;
END;
/

-- error: buffer overflow, limit of 1000000 bytes.
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(100000000);
	buff := rpad('a', 10, 'b');
	for i in 1..1000000 loop
		DBMS_OUTPUT.PUT_LINE(buff);
	end loop;
END;
/

-- if buffer_size < 2000 then buffer_size = 2000.
-- error: buffer overflow, limit of 2000 bytes.
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(-111);
	buff := rpad('a', 10, 'b');
	for i in 1..1000000 loop
		DBMS_OUTPUT.PUT_LINE(buff);
	end loop;
END;
/

DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(0);
	buff := rpad('a', 10, 'b');
	for i in 1..1000000 loop
		DBMS_OUTPUT.PUT_LINE(buff);
	end loop;
END;
/

DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(111);
	buff := rpad('a', 10, 'b');
	for i in 1..1000000 loop
		DBMS_OUTPUT.PUT_LINE(buff);
	end loop;
END;
/

-- the put lines before set serveroutput will be printed.
SET SERVEROUTPUT OFF
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(100000);
	DBMS_OUTPUT.PUT_LINE('AAAAA');
	DBMS_OUTPUT.PUT_LINE('BBBBB');
END;
/

SET SERVEROUTPUT ON

BEGIN
	DBMS_OUTPUT.PUT_LINE('CCCCC');
END;
/
SET SERVEROUTPUT OFF

-- the maximum line size is 32767 bytes(not include the newline character).
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(1000000);
	buff := rpad('a', 32767, 'b');
	DBMS_OUTPUT.PUT_LINE(buff);
END;
/

DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(1000000);
	buff := rpad('a', 32767, 'b');
	DBMS_OUTPUT.PUT_LINE(buff || 'a');
END;
/

-- when set serveroutput to on and issue error, will flush the buffer, print all
-- lines of put before error. 
SET SERVEROUTPUT ON
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(1000000);
	buff := rpad('a', 100, 'b');
	DBMS_OUTPUT.PUT(buff);
	DBMS_OUTPUT.NEW_LINE();
	buff := rpad('a', 32760, 'b');
	DBMS_OUTPUT.PUT(buff);
	DBMS_OUTPUT.PUT(buff);
END;
/
SET SERVEROUTPUT OFF

SET SERVEROUTPUT ON
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(1000000);
	buff := rpad('a', 32767, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	buff := rpad('a', 100, 'b');
	DBMS_OUTPUT.PUT(buff);
	DBMS_OUTPUT.NEW_LINE();
	buff := rpad('a', 32760, 'b');
	DBMS_OUTPUT.PUT(buff);
	DBMS_OUTPUT.PUT(buff);
END;
/
SET SERVEROUTPUT OFF

-- reset buffer to smaller, all lines that have not been read are valid. 
DROP TABLE dbms_output_test;
CREATE TABLE dbms_output_test(buff varchar2(6000), status int);

DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(4000);
	buff := rpad('a', 4000, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.ENABLE(3000);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT length(buff), lengthb(buff) FROM dbms_output_test;	-- 4000
DELETE FROM dbms_output_test;

-- test the validity of the old data row before the second call to enable(x).
DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(4000);
	buff := rpad('a', 100, 'a');
	DBMS_OUTPUT.PUT_line(buff);
	buff := rpad('b', 100, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	buff := rpad('c', 100, 'c');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.GET_LINE(buff,stts);	-- make buffer dirty
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.ENABLE(6000);
	buff := rpad('d', 100, 'd');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(4000);
	buff := rpad('a', 100, 'a');
	DBMS_OUTPUT.PUT_line(buff);
	buff := rpad('b', 100, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	buff := rpad('c', 100, 'c');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.GET_LINE(buff,stts);	-- make buffer dirty
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.ENABLE(6000);			-- enable()
	DBMS_OUTPUT.GET_LINE(buff,stts);	-- get line now 
	INSERT INTO dbms_output_test VALUES (buff, stts);
	buff := rpad('d', 100, 'd');
	DBMS_OUTPUT.PUT_line(buff);			-- buffer dirty, so reset buffer
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- newline characters do not take up the storage space of buffer.
DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(3000);
	for i in 1..30000 loop
		DBMS_OUTPUT.PUT_LINE(NULL);		-- OK
	end loop;
END;
/

DECLARE
	buff varchar2(32767) := 'init';
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(3000);
	for i in 1..3001 loop
		DBMS_OUTPUT.PUT_LINE('a');		-- ERROR
	end loop;
END;
/

-- enable(): buffer_size from limited to unlimited.
DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(4000);
	buff := rpad('a', 3000, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.ENABLE(3000);
	DBMS_OUTPUT.ENABLE(3100);
	DBMS_OUTPUT.ENABLE(NULL);
	buff := rpad('c', 3000, 'd');
	DBMS_OUTPUT.PUT_line(buff);
END;
/

-- enable(): buffer_size from umlimited to limited.
DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(NULL);
	buff := rpad('a', 3000, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.ENABLE(3000);
	DBMS_OUTPUT.ENABLE(2200);
	buff := rpad('c', 3000, 'd');
	DBMS_OUTPUT.PUT_line(buff);
END;
/

DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(NULL);
	buff := rpad('a', 3000, 'b');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.ENABLE(3000);
	DBMS_OUTPUT.ENABLE(2200);
	buff := rpad('c', 2000, 'd');
	DBMS_OUTPUT.PUT_line(buff);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT length(buff), lengthb(buff) FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- newline within line content.
DECLARE
	buff varchar2(32767) := 'init';
	stts int;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE(20000);
	DBMS_OUTPUT.PUT_LINE('BBB'||chr(10)||'CCC');
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
	DBMS_OUTPUT.GET_LINE(buff,stts);
	INSERT INTO dbms_output_test VALUES (buff, stts);
END;
/
SELECT * FROM dbms_output_test;
DELETE FROM dbms_output_test;

-- get_lines not compatible with Oracle, we implement it using a text array.
DECLARE
	arr text[];
	stts int4 := 8;
BEGIN
	DBMS_OUTPUT.DISABLE();
	DBMS_OUTPUT.ENABLE();
	DBMS_OUTPUT.PUT ('line 1');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.PUT ('line 2');
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.PUT_LINE('line 3');
	DBMS_OUTPUT.PUT_LINE(null);
	DBMS_OUTPUT.NEW_LINE();
	DBMS_OUTPUT.GET_LINES(arr, stts);
	for i in 1..stts loop
		INSERT INTO dbms_output_test VALUES (arr[i], stts);
	end loop;
	DBMS_OUTPUT.DISABLE();
END;
/
SELECT * FROM dbms_output_test;
DROP TABLE dbms_output_test;

-- the parameter data type of PUT/PUT_LINE is implemented as anyelement.
SET SERVEROUTPUT ON
CREATE DOMAIN myz AS VARCHAR2(64 CHAR);
DECLARE
	pg_var1 boolean := 't';
	pg_var2 int2 := 2025;
	pg_var3 int4 := 2025;
	pg_var4 int8 := 2025;
	pg_var5 numeric := 3.14;
	pg_var6 float4 := 3.14;
	pg_var7 float8 := 3.14;
	pg_var8 bytea := 1010;
	pg_var9 text := 'pg text';
	pg_var10 pg_catalog.varchar(32) := 'pg varchar';
	pg_var11 pg_catalog.bpchar(32) := 'pg char';
	pg_var12 pg_catalog.date := '2025-1-1';
	pg_var13 pg_catalog.timestamp := '2025-1-1 11:11:11';
	pg_var14 pg_catalog.timestamptz := '2025-1-1 11:11:11 +08:00';
	pg_var15 pg_catalog.time := '11:11:11';
	pg_var16 pg_catalog.timetz := '11:11:11 +08:00';
	pg_var17 pg_catalog.interval := '1 days';

	ora_var1 number := 3.14;
	ora_var2 binary_float := 3.14F;
	ora_var3 binary_double := 3.14D;
	ora_var4 char(32 char) := 'oracle char(32 char)';
	ora_var5 char(32 byte) := 'oracle char(32 byte)';
	ora_var6 varchar2(32 char) := 'oracle varchar2(32 char)';
	ora_var7 varchar2(32 byte) := 'oracle varchar2(32 byte)';
	ora_var8 date := date'2025-1-1';
	ora_var9 timestamp without time zone := timestamp'2025-1-1 11:11:11';
	ora_var10 timestamp with time zone := timestamp'2025-1-1 11:11:11';
	ora_var11 timestamp with local time zone := timestamp'2025-1-1 11:11:11';
	ora_var12 interval year to month := '1-11';
	ora_var13 interval day to second := '1 11:11:11';

	other_var1 myz := 'this is a domain';
BEGIN
	DBMS_OUTPUT.PUT_LINE('this is unknown type');
	DBMS_OUTPUT.PUT_LINE(pg_var1);
	DBMS_OUTPUT.PUT_LINE(pg_var2);
	DBMS_OUTPUT.PUT_LINE(pg_var3);
	DBMS_OUTPUT.PUT_LINE(pg_var4);
	DBMS_OUTPUT.PUT_LINE(pg_var5);
	DBMS_OUTPUT.PUT_LINE(pg_var6);
	DBMS_OUTPUT.PUT_LINE(pg_var7);
	DBMS_OUTPUT.PUT_LINE(pg_var8);
	DBMS_OUTPUT.PUT_LINE(pg_var9);
	DBMS_OUTPUT.PUT_LINE(pg_var10);
	DBMS_OUTPUT.PUT_LINE(pg_var11);
	DBMS_OUTPUT.PUT_LINE(pg_var12);
	DBMS_OUTPUT.PUT_LINE(pg_var13);
	DBMS_OUTPUT.PUT_LINE(pg_var14);
	DBMS_OUTPUT.PUT_LINE(pg_var15);
	DBMS_OUTPUT.PUT_LINE(pg_var16);
	DBMS_OUTPUT.PUT_LINE(pg_var17);

	DBMS_OUTPUT.PUT_LINE(ora_var1);
	DBMS_OUTPUT.PUT_LINE(ora_var2);
	DBMS_OUTPUT.PUT_LINE(ora_var3);
	DBMS_OUTPUT.PUT_LINE(ora_var4);
	DBMS_OUTPUT.PUT_LINE(ora_var5);
	DBMS_OUTPUT.PUT_LINE(ora_var6);
	DBMS_OUTPUT.PUT_LINE(ora_var7);
	DBMS_OUTPUT.PUT_LINE(ora_var8);
	DBMS_OUTPUT.PUT_LINE(ora_var9);
	DBMS_OUTPUT.PUT_LINE(ora_var10);
	DBMS_OUTPUT.PUT_LINE(ora_var11);
	DBMS_OUTPUT.PUT_LINE(ora_var12);
	DBMS_OUTPUT.PUT_LINE(ora_var13);

	DBMS_OUTPUT.PUT_LINE(other_var1);
END;
/
DROP DOMAIN myz;
SET SERVEROUTPUT OFF

-- system variable "SERVEROUTPUT"
-- set serveroutput without size option uses the SIZE and FORMAT of the previous
-- SET SERVEROUTPUT ON SIZE n FORMAT f, or uses default values if no SET SERVEROUTPUT
-- command was previously issued in the current connection.
SET SERVEROUT ON
SHOW SERVEROUT

SET SERVEROUTPUT ON SIZE 2222
SHOW SERVEROUTPUT

SET SERVEROUTPU ON SIZE 3333
SHOW SERVEROUTPU

SET SERVEROUTP ON SIZE 4444
SHOW SERVEROUTP

SET SERVEROUT ON SIZE 5555
SHOW SERVEROUT

SET SERVEROUT ON
SHOW SERVEROUT

SET SERVEROUTPUT OFF size 2020
SHOW SERVEROUTPUT
SET SERVEROUTPUT ON
SHOW SERVEROUTPUT

-- system variable "SERVEROUTPUT" not support transaction.
SET SERVEROUTPUT OFF
SHOW SERVEROUTPUT
BEGIN;
SET SERVEROUTPUT ON
ROLLBACK;
SHOW SERVEROUTPUT
SET SERVEROUTPUT OFF
