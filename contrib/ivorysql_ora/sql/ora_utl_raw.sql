CREATE USER u_utl_raw PASSWORD 'highgo@123';
SET SESSION AUTHORIZATION u_utl_raw;
-- check constant
set serveroutput ON;
declare
begin
dbms_output.put_line('value of big_endian is ' || utl_raw.big_endian);
dbms_output.put_line('value of little_endian is ' || utl_raw.little_endian);
dbms_output.put_line('value of machine_endian is ' || utl_raw.machine_endian);
end;
/
set serveroutput OFF;
-- check bit operation func
select utl_raw.bit_and(null, '');
select utl_raw.bit_and(null, 'aabb');
select utl_raw.bit_and('aabb', '');
select utl_raw.bit_and('aabb', 'bbccdd');
select utl_raw.bit_complement(null);
select utl_raw.bit_complement('');
select utl_raw.bit_complement('11223344');
select utl_raw.bit_or(null, '');
select utl_raw.bit_or(null, 'aabb');
select utl_raw.bit_or('aabb', '');
select utl_raw.bit_or('aabb', 'bbccdd');
select utl_raw.bit_xor(null, '');
select utl_raw.bit_xor(null, 'aabb');
select utl_raw.bit_xor('aabb', '');
select utl_raw.bit_xor('aabb', 'bbccdd');
-- check cast from
select utl_raw.cast_from_binary_double(123.4567);
select utl_raw.cast_from_binary_double(123.4567,2);
select utl_raw.cast_from_binary_double(123.4567,4);
select utl_raw.cast_from_binary_double(BINARY_DOUBLE_INFINITY, 2);
select utl_raw.cast_from_binary_double('');
select utl_raw.cast_from_binary_double(123.4567,'');

select utl_raw.cast_from_binary_float(123.4567);
select utl_raw.cast_from_binary_float(123.4567,2);
select utl_raw.cast_from_binary_float(123.4567,4);
select utl_raw.cast_from_binary_float(BINARY_FLOAT_INFINITY, 2);
select utl_raw.cast_from_binary_float('');
select utl_raw.cast_from_binary_float(123.4567,'');

select utl_raw.cast_from_binary_integer(123);
select utl_raw.cast_from_binary_integer(123,2);
select utl_raw.cast_from_binary_integer(123,4);
select utl_raw.cast_from_binary_integer('');
select utl_raw.cast_from_binary_integer(123,'');

select utl_raw.cast_from_number(123);
select utl_raw.cast_from_number(123.4567);
select utl_raw.cast_from_number('');
-- check cast to
select utl_raw.cast_to_binary_double(utl_raw.cast_from_binary_double(123.4567));
select utl_raw.cast_to_binary_double(utl_raw.cast_from_binary_double(123.4567, 2), 2);
select utl_raw.cast_to_binary_double('\x5305a3923add5e40', 2);            -- 123.4567 eg. 1.234567E+002
select utl_raw.cast_to_binary_double('\x5305a3923add5e401234567890', 2);  -- 123.4567 eg. 1.234567E+002
select utl_raw.cast_to_binary_double('\x5305a3923add5e', 2);              -- not enough long, error
select utl_raw.cast_to_binary_double('\x5305a3923add5e40', 4);            -- error endianess
select utl_raw.cast_to_binary_double('');
select utl_raw.cast_to_binary_double('\x5305a3923add5e40','');

select utl_raw.cast_to_binary_float(utl_raw.cast_from_binary_float(123.4567));        -- 123.4567 eg. 1.234567E+002
select utl_raw.cast_to_binary_float(utl_raw.cast_from_binary_float(123.4567, 2), 2);  -- 123.4567 eg. 1.234567E+002
select utl_raw.cast_to_binary_float('\x42f6e9d5', 1);                                 -- 123.4567 eg. 1.234567E+002
-- 123.4567 big_endian to raw, little to float, oracle is -3.21554848E+013, highgo is -3.2155485E+013
select utl_raw.cast_to_binary_float('\x42f6e9d53add5e401234567890', 2);
-- 123.4567 little_endian from binary_double to raw, then cast to binary_float use little_endian, oracle is -1.02880587E-027, highgo is -1.0288059E-027
select utl_raw.cast_to_binary_float('\x5305a3923add5e', 2);
select utl_raw.cast_to_binary_float('\x5305a3923add5e40', 4);
select utl_raw.cast_to_binary_float('');
select utl_raw.cast_to_binary_float('\x5305a3923add5e40','');

select utl_raw.cast_to_binary_integer(utl_raw.cast_from_binary_integer(123));
select utl_raw.cast_to_binary_integer(utl_raw.cast_from_binary_integer(123, 2), 2);
-- 123 little_endian from binary_integer to raw, then cast to binary_integer use big_endian, eg. 2063597568
select utl_raw.cast_to_binary_integer('\x7b000000', 1);
select utl_raw.cast_to_binary_integer('\x7b0000001234', 2);    -- error, raw of binary_integer too long
select utl_raw.cast_to_binary_integer('', 2);
select utl_raw.cast_to_binary_integer('\x7b000000', '');

select utl_raw.cast_to_number(utl_raw.cast_from_number(123.456));
select utl_raw.cast_to_number(utl_raw.cast_from_number(-5678.1234));
select utl_raw.cast_to_number(utl_raw.cast_from_number(56781234));
select utl_raw.cast_to_number(utl_raw.cast_from_number(''));
select utl_raw.cast_to_number('');

-- check cast to nvarchar2/raw/varchar2
select utl_raw.cast_to_raw('Hello, HighGo!');
select utl_raw.cast_to_varchar2(utl_raw.cast_to_raw('Hello, HighGo!'));
select utl_raw.cast_to_nvarchar2(utl_raw.cast_to_raw('Hello, HighGo!'));

-- check compare
select utl_raw.compare('','');
select utl_raw.compare('','00');
select utl_raw.compare('','aa');
select utl_raw.compare('','aa', 'aabb');
select utl_raw.compare('aabbccdddd','aabbcc', 'ddaabb');

-- check concat
select utl_raw.concat();
select utl_raw.concat('a', '', 'b', '', 'c', '', 'd', '', 'e', '', 'f','');

-- check convert
select utl_raw.convert('','','');
select utl_raw.convert(utl_raw.cast_to_raw('Hello, HighGo!'),'al16utf16','utf8');
select utl_raw.convert(utl_raw.cast_to_raw('Hello, HighGo!'),'gbk','utf8');

-- check copies
select utl_raw.copies('',1);
select utl_raw.copies('a',1);
select utl_raw.copies('a',0);
select utl_raw.copies('a',10);

-- check length
select utl_raw.length('');
select utl_raw.length('123');
select utl_raw.length('123abc');

-- check overlay
select utl_raw.overlay('','');
select utl_raw.overlay('','1');
select utl_raw.overlay('12345','aabbccdd');
select utl_raw.overlay('12345','aabbccdd', 2);
select utl_raw.overlay('12345','aabbccdd', 10);
select utl_raw.overlay('12345','aabbccdd', 10, 5);
select utl_raw.overlay('12345','aabbccdd', 10, 5, 'ffeeddd');

-- check reverse
select utl_raw.reverse('');
select utl_raw.reverse('123');
select utl_raw.reverse('123456');

-- check substr
select utl_raw.substr('',0);
select utl_raw.substr('',1);
select utl_raw.substr('aaa', 1);
select utl_raw.substr('aaa', 1, 2);
select utl_raw.substr('aaa', 1, 3);
select utl_raw.substr('aaa', 2, 2);
select utl_raw.substr('aabbccdd', -5, 2);
select utl_raw.substr('aabbccdd', -4, 3);

-- check translate --param cannot be null, all error
select utl_raw.translate('','','');
select utl_raw.translate('aa','','');
select utl_raw.translate('aa','11','');
select utl_raw.translate('aabbccdd','11','223344');
select utl_raw.translate('aabbccdd','aa11','223344');
select utl_raw.translate('aabbccdd','aa11cc','223344');
select utl_raw.translate('aabbccdd','aa11cc','2233');

-- check transliterate
select utl_raw.transliterate('');
select utl_raw.transliterate('aabbccdde');
select utl_raw.transliterate('aabbccddee0100', '99887766');
select utl_raw.transliterate('aabbccddee0100', '', 'aabbccdd');
select utl_raw.transliterate('aabbccddee0100', '112233445566', 'aabbccdd');
select utl_raw.transliterate('aabbccddee0100', '112233', 'aabbccdd', 'fedcba');

-- check xrange
select utl_raw.xrange();
select utl_raw.xrange('77');
select utl_raw.xrange('', '33');
select utl_raw.xrange('77', '33');

RESET SESSION AUTHORIZATION;
DROP USER u_utl_raw CASCADE;