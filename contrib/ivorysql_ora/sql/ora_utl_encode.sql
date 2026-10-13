CREATE USER u_utl_encode PASSWORD 'highgo@123';
SET SESSION AUTHORIZATION u_utl_encode;
-- 0.check constant
set serveroutput ON;
declare
begin
dbms_output.put_line('value of base64 is ' || utl_encode.base64);
dbms_output.put_line('value of quoted_printable is ' || utl_encode.quoted_printable);
dbms_output.put_line('value of complete is ' || utl_encode.complete);
dbms_output.put_line('value of header_piece is ' || utl_encode.header_piece);
dbms_output.put_line('value of middle_piece is ' || utl_encode.middle_piece);
dbms_output.put_line('value of end_piece is ' || utl_encode.end_piece);
end;
/
set serveroutput OFF;
-- 1.check base64
-- 1.1 'SGVsbG8sIFdvcmxkIQ=='
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_encode(utl_raw.cast_to_raw('Hello, World!')));
-- 1.2 1byte encode to 'YQ=='
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_encode(utl_raw.cast_to_raw('a')));
-- 1.3. 2 bytes encode to 'YWI='
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_encode(utl_raw.cast_to_raw('ab')));
-- 1.4 3 bytes without '=' : 'YWJj'
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_encode(utl_raw.cast_to_raw('abc')));
-- 1.5 48 bytes encode to 64 bytes without 0D0A [\r\n] = 64
SELECT utl_raw.length(utl_encode.base64_encode(utl_raw.cast_to_raw(repeat('a', 48))));
-- 1.6 49 bytes encode to 64 + 0D0A + 4 = 70
SELECT utl_raw.length(utl_encode.base64_encode(utl_raw.cast_to_raw(repeat('a', 49))));
-- 1.7 96 bytes encode to 64 + 0D0A + 64 = 130
SELECT utl_raw.length(utl_encode.base64_encode(utl_raw.cast_to_raw(repeat('a', 96))));
-- 1.8 NULL or '' with error
SELECT utl_encode.base64_encode(NULL);
SELECT utl_encode.base64_encode('');

-- 1.9 decode to 'Hello, World!'
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_raw.cast_to_raw('SGVsbG8sIFdvcmxkIQ==')));
-- 1.10 'a'
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_raw.cast_to_raw('YQ==')));
-- 1.11 'ab'
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_raw.cast_to_raw('YWI=')));
-- 1.12 'abc'
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_raw.cast_to_raw('YWJj')));
-- 1.13 49 * a, remove 0D0A[CRLF]
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_encode.base64_encode(utl_raw.cast_to_raw(repeat('a', 49)))));
-- 1.14 decode with encode 
SELECT utl_raw.cast_to_varchar2(utl_encode.base64_decode(utl_encode.base64_encode(utl_raw.cast_to_raw('The quick brown fox')))) = 'The quick brown fox';
-- 1.15 remove all CR LF, decode to null
SELECT utl_encode.base64_decode('0D0A');
-- 1.16 invalid base64 1 byte decode to NULL
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('x'));
-- 1.17 invalid base64 1 byte with 3 '=' decode to NULL
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('0==='));
-- 1.18 remove character after '=' : '123456'
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('EjRWF===eJCr'));
-- 1.19 NULL or '' with error
SELECT utl_encode.base64_decode(NULL);
SELECT utl_encode.base64_decode('');
-- 1.20 invalid character ' ' with error
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('Y Q=='));
-- 1.21 invalid character '\t' with error
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('YQ\t=='));
-- 1.22 invalid character '!' with error
SELECT utl_encode.base64_decode(utl_raw.cast_to_raw('YQ!='));

-- 2.check mimeheader
-- 2.1 '=?UTF8?B?QUJD?='
SELECT utl_encode.mimeheader_encode('ABC', 'UTF8', 1) AS result;
-- 2.2 '=?UTF8?B?QUJDREVG?='
SELECT utl_encode.mimeheader_encode('ABCDEF', 'UTF8', 1) AS result;
-- 2.3 '=?UTF8?Q?ABC?='
SELECT utl_encode.mimeheader_encode('ABC', 'UTF8', 2) AS result;
-- 2.4 encode_charset is null '=?UTF8?Q?ABC?='
SELECT utl_encode.mimeheader_encode('ABC', NULL, NULL) AS result;
-- 2.5 1 line, 16 * QUFB
SELECT utl_encode.mimeheader_encode(rpad('A', 48, 'A'), 'UTF8', 1) AS result;
-- 2.6 2 lines, 16 * QUFB + CRLF + QQ==
SELECT utl_encode.mimeheader_encode(rpad('A', 49, 'A'), 'UTF8', 1) AS result;
-- 2.7 1 line, 64 * A
SELECT utl_encode.mimeheader_encode(rpad('A', 64, 'A'), 'UTF8', 2) AS result;
-- 2.8 2 lines, 64 * A + CRLF + A
SELECT utl_encode.mimeheader_encode(rpad('A', 65, 'A'), 'UTF8', 2) AS result;
-- 2.9 2 lines, 64 * A + CRLF + 64 *A
SELECT utl_encode.mimeheader_encode(rpad('A', 128, 'A'), 'UTF8', 2) AS result;
-- 2.10 3 lines, 64 * A + CRLF + 64 * A + CRLF + A
SELECT utl_encode.mimeheader_encode(rpad('A', 129, 'A'), 'UTF8', 2) AS result;
-- 2.11 '=?UTF8?Q?a=20b?='
SELECT utl_encode.mimeheader_encode('a b', 'UTF8', 2) AS result;
-- 2.12 '=?UTF8?Q?a=3Db?='
SELECT utl_encode.mimeheader_encode('a=b', 'UTF8', 2) AS result;
-- 2.13 '=?UTF8?Q?a=3Fb?='
SELECT utl_encode.mimeheader_encode('a?b', 'UTF8', 2) AS result;
-- 2.14 '=?UTF8?Q?a=5Fb?='
SELECT utl_encode.mimeheader_encode('a_b', 'UTF8', 2) AS result;
-- 2.15 '=?UTF8?Q?abc?='
SELECT utl_encode.mimeheader_encode('abc', 'UTF8', 2) AS result;
-- 2.16 '=?UTF8?Q?A=0DB?='
SELECT utl_encode.mimeheader_encode('A' || chr(13) || 'B', 'UTF8', 2) AS result;
-- 2.17 '=?UTF8?Q?A=0AB?='
SELECT utl_encode.mimeheader_encode('A' || chr(10) || 'B', 'UTF8', 2) AS result;
-- 2.18 '=?UTF8?Q?A + CRLF + B?='
SELECT utl_encode.mimeheader_encode('A' || chr(13) || chr(10) || 'B', 'UTF8', 2) AS result;
-- 2.19 7 * =E4=B8=AD
SELECT utl_encode.mimeheader_encode('中中中中中中中', 'UTF8', 2) AS result;
-- 2.20 7 * =E4=B8=AD + CRLF + =E4=B8=AD
SELECT utl_encode.mimeheader_encode('中中中中中中中中', 'UTF8', 2) AS result;
-- 2.21 '=?UTF8?B?w6k=?='
SELECT utl_encode.mimeheader_encode('é', 'UTF8', 1) AS result;
-- 2.22 '=?UTF8?B?w6k=?='
SELECT utl_encode.mimeheader_encode('é', NULL, 1) AS result;
-- 2.23 buf is null or '' with error
SELECT utl_encode.mimeheader_encode(NULL, NULL, 1);
SELECT utl_encode.mimeheader_encode('', NULL, 1);
-- 2.24 invalid encoding with error
SELECT utl_encode.mimeheader_encode('ABC', NULL, 0);
SELECT utl_encode.mimeheader_encode('ABC', NULL, 3);
-- 2.25 invalid encode_charset with error
SELECT utl_encode.mimeheader_encode('ABC', 'INVALID', 1);
SELECT utl_encode.mimeheader_encode('ABC', 'INVALID', 2);
SELECT utl_encode.mimeheader_encode('ABC', 'INVALID', NULL);

-- 2.26 'ABC'
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QUJD?=') AS result;
-- 2.27 'ABCDEF'
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QUJDREVG?=') AS result;
-- 2.28 'A'
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QQ==?=') AS result;
-- 2.29 'A'
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QQ?=') AS result;
-- 2.30 'ABC'
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QUJDQ?=') AS result;
-- 2.31 'ABC'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=') AS result;
-- 2.32 'A B'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A=20B?=') AS result;
-- 2.33 'A_B'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A_B?=') AS result;
-- 2.34 'A_B'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A=5FB?=') AS result;
-- 2.35 'ABC'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=41=42=43?=') AS result;
-- 2.36 '\r'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=0D?=') AS result;
-- 2.37 '\n'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=0A?=') AS result;
-- 2.38 remove last '=', 'A'
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A=?=') AS result;
-- 2.39 only '=' with error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=?=');
-- 2.40 invalid =xx with error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=4?=');
-- 2.41 first is 0x00 error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A=4?=');
-- 2.42 invalid hex ERROR
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=a1?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=G1?=');
-- 2.43 ? error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A?B?=');
-- 2.44 invalid encoding error
SELECT utl_encode.mimeheader_decode('=?UTF8?q?ABC?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?b?QUJD?=');
-- 2.45 CRLF, NULL, CR is true 'ABCDEF', others are error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(13) || chr(10) || '=?UTF8?Q?DEF?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?==?UTF8?Q?DEF?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?= =?UTF8?Q?DEF?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(13) || chr(10) || ' =?UTF8?Q?DEF?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(10) || '=?LATIN1?B?REVG?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(13) || chr(10) || '=?LATIN1?Q?DEF?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?B?QUJD?=' || chr(13) || chr(10) || '=?LATIN1?Q?DEF?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(13) || chr(10) || '=?LATIN1?B?REVG?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?==?LATIN1?B?REVG?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?ABC?=' || chr(13) || '=?LATIN1?B?REVG?=') AS result;

-- 2.46 encode text is null error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q??=');
SELECT utl_encode.mimeheader_decode('=?UTF8?B??=');

-- 2.47 encode text too long or too short error
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?' || rpad('A', 75, 'A') || '?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?B?Q?=');
SELECT utl_encode.mimeheader_decode('=?UTF8?B?Q' || chr(13) || chr(10) || '?=');

-- 2.48 0x00 
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?=00?=') AS result;
SELECT utl_encode.mimeheader_decode('=?UTF8?Q?A=00B?=') AS result;

-- 3 check quoted_printable
-- 3.1 20-7E is itself, other and '=' is =xx
SELECT utl_encode.quoted_printable_encode(utl_raw.cast_to_raw('abc')) FROM dual;
SELECT utl_encode.quoted_printable_encode(utl_raw.cast_to_raw('=')) FROM dual;
SELECT utl_encode.quoted_printable_encode('00') FROM dual;
SELECT utl_encode.quoted_printable_encode('09') FROM dual;
SELECT utl_encode.quoted_printable_encode('0D') FROM dual;
SELECT utl_encode.quoted_printable_encode('0A') FROM dual;
SELECT utl_encode.quoted_printable_encode('FF') FROM dual;

-- 3.2 0D0A
SELECT utl_encode.quoted_printable_encode('0D0A') FROM dual;
SELECT utl_encode.quoted_printable_encode('0D0A0D0A') FROM dual;
-- 3D3044 0D0A 3D3041
SELECT utl_encode.quoted_printable_encode('0D0D0A0A') FROM dual;
-- 3D3044 20 3D3041
SELECT utl_encode.quoted_printable_encode('0D200A') FROM dual;
-- 3D3230 0D0A
SELECT utl_encode.quoted_printable_encode('200D0A') FROM dual;
SELECT utl_encode.quoted_printable_encode('20') FROM dual;

-- 3.3 75 bytes with 3D0D0A 75, 79, 75
SELECT utl_raw.length(utl_encode.quoted_printable_encode(utl_raw.cast_to_raw(rpad('a',75,'a')))) FROM dual;
SELECT utl_raw.length(utl_encode.quoted_printable_encode(utl_raw.cast_to_raw(rpad('a',76,'a')))) FROM dual;
SELECT utl_raw.length(utl_encode.quoted_printable_encode(utl_raw.cast_to_raw(rpad('=',25,'=')))) FROM dual;

-- 3.4 null or '' with error
SELECT utl_encode.quoted_printable_encode(NULL) FROM dual;
SELECT utl_encode.quoted_printable_encode('') FROM dual;

-- 3.4 20-7E print ifself
SELECT utl_encode.quoted_printable_decode(utl_raw.cast_to_raw('abc')) FROM dual;
SELECT utl_encode.quoted_printable_decode('20') FROM dual;

-- 3.5 =xx decode to xx
SELECT utl_encode.quoted_printable_decode(utl_raw.cast_to_raw('=3D')) FROM dual;
SELECT utl_encode.quoted_printable_decode('3D3030') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D4646') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D3044') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D3041') FROM dual;

-- 3.6 remove 20 with 0D
SELECT utl_encode.quoted_printable_decode('200D0A') FROM dual;
SELECT utl_encode.quoted_printable_decode('200D41') FROM dual;
SELECT utl_encode.quoted_printable_decode('2020') FROM dual;
SELECT utl_encode.quoted_printable_decode('200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('20200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('2020200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('202020200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('202041') FROM dual;
SELECT utl_encode.quoted_printable_decode('20410D') FROM dual;
SELECT utl_encode.quoted_printable_decode('20204120200D') FROM dual;
SELECT utl_encode.quoted_printable_decode('20200A') FROM dual;
SELECT utl_encode.quoted_printable_decode('20200D0A') FROM dual;
SELECT utl_encode.quoted_printable_decode('20200D0D0A') FROM dual;

-- 3.7 0D0A
SELECT utl_encode.quoted_printable_decode('0D0A') FROM dual;
SELECT utl_encode.quoted_printable_decode('0D0A0D0A') FROM dual;

-- 3.8 3D00 decode to 00
SELECT utl_encode.quoted_printable_decode('3D00') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D003D00') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0041') FROM dual;

-- 3.9 3D0D 3D0A is NULL
SELECT utl_encode.quoted_printable_decode('3D0D') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0A') FROM dual;

-- 3.10 3D 0D/0A + 0D/0A is NULL
SELECT utl_encode.quoted_printable_decode('3D0D0D') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0D0A') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0A0D') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0A0A') FROM dual;

-- 3.11 decode '3D 0D/0A + xx' is equal to decode 'xx'
SELECT utl_encode.quoted_printable_decode('3D0D3D00') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0A3D00') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0A7700') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D0D7700') FROM dual;

-- 3.12 remove last 3D
SELECT utl_encode.quoted_printable_decode('41') FROM dual;
SELECT utl_encode.quoted_printable_decode('413D') FROM dual;

-- 3.13 null, '', invalid with error
SELECT utl_encode.quoted_printable_decode(NULL) FROM dual;
SELECT utl_encode.quoted_printable_decode('') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D4747') FROM dual;
SELECT utl_encode.quoted_printable_decode('3D41') FROM dual;

-- 3.14 encode and decode
SELECT CASE WHEN utl_raw.compare(utl_encode.quoted_printable_decode(
                    utl_encode.quoted_printable_encode('4142433D00FF0D0A20'))
                 ,'4142433D00FF0D0A20') =  0
            THEN 'OK' ELSE 'FAIL' END FROM dual;

-- 4 check text
-- 4.1 with Base64
-- 'QQ=='
SELECT utl_encode.text_encode('A', NULL, 1) FROM dual;
-- 'QUI='
SELECT utl_encode.text_encode('AB', NULL, 1) FROM dual;
-- 'QUJD'
SELECT utl_encode.text_encode('ABC', NULL, 1) FROM dual;
-- 'QUJDRA=='
SELECT utl_encode.text_encode('ABCD', NULL, 1) FROM dual;
-- 'QUJDREU='
SELECT utl_encode.text_encode('ABCDE', NULL, 1) FROM dual;
-- 'QUJDREVG'
SELECT utl_encode.text_encode('ABCDEF', NULL, 1) FROM dual;

-- 4.2 check CRLF
SELECT utl_encode.text_encode(rpad('A', 48, 'A'), NULL, 1) FROM dual;
-- 64 character + CRLF + 'QQ=='
SELECT utl_encode.text_encode(rpad('A', 49, 'A'), NULL, 1) FROM dual;
-- 64 character + CRLF + 64 character + CRLF + 'QUFBQQ=='
SELECT utl_encode.text_encode(rpad('A', 100, 'A'), NULL, 1) FROM dual;

-- 4.3 with QP
SELECT utl_encode.text_encode('Hello, World!', NULL, 2) FROM dual;
SELECT utl_encode.text_encode('a=b', NULL, 2) FROM dual;
SELECT utl_encode.text_encode('A' || chr(1) || 'B', NULL, 2) FROM dual;
SELECT utl_encode.text_encode('A' || chr(13) || 'B', NULL, 2) FROM dual;
SELECT utl_encode.text_encode('A' || chr(10) || 'B', NULL, 2) FROM dual;
SELECT utl_encode.text_encode('A' || chr(13) || chr(10) || 'B', NULL, 2) FROM dual;

-- 4.3 with QP CRLF
SELECT utl_encode.text_encode(rpad('A', 75, 'A'), NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 76, 'A'), NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 150, 'A'), NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 151, 'A'), NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 73, 'A') || chr(1), NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 74, 'A') || chr(1) || 'A', NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 74, 'A') || chr(13) || chr(10) || 'B', NULL, 2) FROM dual;
SELECT utl_encode.text_encode(rpad('A', 75, 'A') || chr(13) || chr(10) || 'B', NULL, 2) FROM dual;

-- 4.4 convert with charset
-- 'w6k='
SELECT utl_encode.text_encode('é', 'UTF8', 1) FROM dual;
-- 'w6k='
SELECT utl_encode.text_encode('é', NULL, 1) FROM dual;
-- '=C3=A9' 
SELECT utl_encode.text_encode('é', 'UTF8', 2) FROM dual;

-- 4.5 buf is null, '', invalid encoding with error
SELECT utl_encode.text_encode(NULL, NULL, 1);
SELECT utl_encode.text_encode('', NULL, 1);
SELECT utl_encode.text_encode(NULL, NULL, 2);
SELECT utl_encode.text_encode(NULL, NULL, NULL);
SELECT utl_encode.text_encode('ABC', NULL, 0);
SELECT utl_encode.text_encode('ABC', NULL, 3);

-- 4.6 decode base64 A AB ABC ABCD ABCDE ABCDEF
SELECT utl_encode.text_decode('QQ==', NULL, 1);
SELECT utl_encode.text_decode('QUI=', NULL, 1);
SELECT utl_encode.text_decode('QUJD', NULL, 1);
SELECT utl_encode.text_decode('QUJDRA==', NULL, 1);
SELECT utl_encode.text_decode('QUJDREU=', NULL, 1);
SELECT utl_encode.text_decode('QUJDREVG', NULL, 1);
-- NULL
SELECT utl_encode.text_decode(chr(13) || chr(10), NULL, 1);
SELECT utl_encode.text_decode(chr(13), NULL, 1);
SELECT utl_encode.text_decode(chr(10), NULL, 1);
SELECT utl_encode.text_decode(chr(13) || chr(10) || chr(13) || chr(10), NULL, 1);
SELECT utl_encode.text_decode('Q', NULL, 1);
SELECT utl_encode.text_decode('Q' || chr(13) || chr(10), NULL, 1);

-- 4.7 decode base64 remove after 3D:A NULL A AB
SELECT utl_encode.text_decode('QQ==AAAA', NULL, 1);
SELECT utl_encode.text_decode('Q=JD', NULL, 1);
SELECT utl_encode.text_decode('QU=JD', NULL, 1);
SELECT utl_encode.text_decode('QUJ=D', NULL, 1);

-- 4.8 add 3D when not enough: ABC ABCA ABCAA ABCAAA ABCAAA ABCAAAA
SELECT utl_encode.text_decode('QUJDQ', NULL, 1);
SELECT utl_encode.text_decode('QUJDQQ', NULL, 1);
SELECT utl_encode.text_decode('QUJDQUF', NULL, 1);
SELECT utl_encode.text_decode('QUJDQUFB', NULL, 1);
SELECT utl_encode.text_decode('QUJDQUFBQ', NULL, 1);
SELECT utl_encode.text_decode('QUJDQUFBQQ', NULL, 1);

-- 4.9 decode QP
SELECT utl_encode.text_decode('Hello', NULL, 2);
SELECT utl_encode.text_decode('=3D', NULL, 2);
SELECT utl_encode.text_decode('=01', NULL, 2);
SELECT utl_encode.text_decode('=C3=A9', NULL, 2);
SELECT utl_encode.text_decode('ABC=', NULL, 2);
SELECT utl_encode.text_decode('=00', NULL, 2);
SELECT utl_encode.text_decode('=0D', NULL, 2);
SELECT utl_encode.text_decode('=0A', NULL, 2);

-- 4.10 decode QP with 3D
-- 3D0D 3D0A 3D0D0D 3D0D0A 3D0A0A decode null
SELECT utl_encode.text_decode(chr(61) || chr(10), NULL, 2);
SELECT utl_encode.text_decode(chr(61) || chr(13), NULL, 2);
SELECT utl_encode.text_decode(chr(61) || chr(10) || chr(10), NULL, 2);
SELECT utl_encode.text_decode(chr(61) || chr(10) || chr(13), NULL, 2);
SELECT utl_encode.text_decode(chr(61) || chr(13) || chr(13), NULL, 2);
-- 3D0D xx is equal to xx
SELECT utl_encode.text_decode(chr(61) || chr(10) || chr(65), NULL, 2) = 'A' AS result;

-- 4.11 invalid hex, invalid encoding, buf is null or '' decode error
SELECT utl_encode.text_decode('=c3=a9', NULL, 2);
SELECT utl_encode.text_decode('=C3=a9', NULL, 2);
SELECT utl_encode.text_decode('=X', NULL, 2);
SELECT utl_encode.text_decode(NULL, NULL, 1);
SELECT utl_encode.text_decode('', NULL, 1);
SELECT utl_encode.text_decode(NULL, NULL, 2);
SELECT utl_encode.text_decode(NULL, NULL, NULL);
SELECT utl_encode.text_decode('ABC', NULL, 0);
SELECT utl_encode.text_decode('ABC', NULL, 3);

-- 4.12 remove 20 before 0D
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('200D'), NULL, 2) = chr(13) AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('20200D'), NULL, 2) = chr(13) AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('2020200D'), NULL, 2) = chr(13) AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('202041'), NULL, 2) = '  A' AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('20204120200D'), NULL, 2) = '  A' || chr(13) AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('20200A'), NULL, 2) = '  ' || chr(10) AS result;
SELECT utl_encode.text_decode(utl_raw.cast_to_varchar2('20200D0A'), NULL, 2) = chr(13) || chr(10) AS result;

-- 5 check Unix to Unix
-- 5.1 default type=1, filename='uuencode.txt', permission='0'
-- 626567696E2030207575656E636F64652E7478740D0A24303429230D0A0A656E64
SELECT utl_encode.uuencode('414243') FROM dual;
-- 626567696E2030207575656E636F64652E7478740D0A24303429230D0A0A656E64
SELECT utl_encode.uuencode('414243', 1) FROM dual;
-- 626567696E2030207575656E636F64652E7478740D0A24303429230D0A
SELECT utl_encode.uuencode('414243', 2) FROM dual;
-- 24303429230D0A
SELECT utl_encode.uuencode('414243', 3) FROM dual;
-- 24303429230D0A0A656E64
SELECT utl_encode.uuencode('414243', 4) FROM dual;

-- 5.2 not enough 3 bytes, add 0x00
-- 22303020200D0A
SELECT utl_encode.uuencode('41', 3) FROM dual;
-- 23303428200D0A
SELECT utl_encode.uuencode('4142', 3) FROM dual;
-- 24303429230D0A
SELECT utl_encode.uuencode('414243', 3) FROM dual;
-- 2830342923312435260D0A
SELECT utl_encode.uuencode('414243444546', 3) FROM dual;
-- 2C3034292331243526315441290D0A
SELECT utl_encode.uuencode('414243444546474849', 3) FROM dual;

-- 5.3 57bytes with 0D0A, length is 79,79,86,158,165
SELECT utl_raw.length(utl_encode.uuencode(utl_raw.copies('41', 57), 3)) FROM dual;
SELECT utl_raw.length(utl_encode.uuencode(utl_raw.copies('41', 56), 3)) FROM dual;
SELECT utl_raw.length(utl_encode.uuencode(utl_raw.copies('41', 58), 3)) FROM dual;
SELECT utl_raw.length(utl_encode.uuencode(utl_raw.copies('41', 114), 3)) FROM dual;
SELECT utl_raw.length(utl_encode.uuencode(utl_raw.copies('41', 115), 3)) FROM dual;

-- 5.4 check begin line
-- 626567696E20363434206120622E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, 'a b.txt', '644') FROM dual;
-- 626567696E2036343420612020622E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, 'a  b.txt', '644') FROM dual;
-- 626567696E203634342020612E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, ' a.txt', '644') FROM dual;
-- 626567696E203620343420612E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, 'a.txt', '6 44') FROM dual;
-- 626567696E20363434200D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, '', '644') FROM dual;
-- 626567696E2020612E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, 'a.txt', '') FROM dual;
-- 626567696E20363434200D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, NULL, '644') FROM dual;
-- 626567696E2036343420C3A92E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', 1, 'é.txt', '644') FROM dual;

-- 5.5 invalid type, raw is null or '' with error
-- 626567696E2036343420612E7478740D0A22303020200D0A0A656E64
SELECT utl_encode.uuencode('41', NULL, 'a.txt', '644') FROM dual;
SELECT utl_encode.uuencode('41', 5, 'a.txt', '644') FROM dual;
SELECT utl_encode.uuencode('41', -7, 'a.txt', '644') FROM dual;
SELECT utl_encode.uuencode(NULL) FROM dual;
SELECT utl_encode.uuencode('') FROM dual;
SELECT CASE WHEN utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode('48656C6C6F')), '48656C6C6F') = 0
            THEN 'OK' ELSE 'FAIL' END FROM dual;
SELECT CASE WHEN utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode(utl_raw.copies('41', 100))), utl_raw.copies('41', 100)) = 0
            THEN 'OK' ELSE 'FAIL' END FROM dual;
SELECT CASE WHEN utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode(utl_raw.copies('42', 1000))), utl_raw.copies('42', 1000)) = 0
            THEN 'OK' ELSE 'FAIL' END FROM dual;

-- 5.6 decode unix
-- AB
SELECT utl_encode.uudecode('224A5C574F') FROM dual;
-- ABCD
SELECT utl_encode.uudecode('234A5C574F') FROM dual;
-- ABCDEF
SELECT utl_encode.uudecode('244A5C574F') FROM dual;
-- ABCDEFAB
SELECT utl_encode.uudecode('264A5C574F4A5C574F') FROM dual;
-- ABCDEFABCDEFAB
SELECT utl_encode.uudecode('2A4A5C574F4A5C574F4A5C574F') FROM dual;
-- ABCDEFABCDEFABCD
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A234A5C574F') FROM dual;
-- ABCDEFABCD
SELECT utl_encode.uudecode('224A5C574F0D0A234A5C574F') FROM dual;
-- ABCDEFABCDEFAB
SELECT utl_encode.uudecode('234A5C574F0D0A264A5C574F4A5C574F') FROM dual;
-- ABCDEFABCDEFABCD
SELECT utl_encode.uudecode('234A5C574F0D0A234A5C574F0D0A234A5C574F') FROM dual;

-- 5.7 invalid length byte will check more bytes
-- B=0x21, D=1, L=0 → NULL
SELECT utl_encode.uudecode('214A5C574F') FROM dual;
-- B=0x25, D=5, L=3, M=8，
SELECT utl_raw.compare(utl_encode.uudecode('254A5C574F4A5C574F'), 'ABCDEF') FROM dual;
-- B=0x25 less then 8 bytes ERROR
SELECT utl_encode.uudecode('254A5C574F') FROM dual;
-- B=0x29, D=9, L=6, M=12，need 16bytes
SELECT utl_raw.compare(utl_encode.uudecode('294A5C574F4A5C574F4A5C574F4A5C574F'), 'ABCDEFABCDEF') FROM dual;
-- B=0x2D, D=13, L=9, M=16，need 16bytes
SELECT utl_raw.compare(utl_encode.uudecode('2D4A5C574F4A5C574F4A5C574F4A5C574F'), 'ABCDEFABCDEFABCDEF') FROM dual;
-- B=0x31, D=17, L=12, M=20，less then 16 bytes ERROR
SELECT utl_encode.uudecode('314A5C574F4A5C574F4A5C574F4A5C574F') FROM dual;

-- 5.8 remove begin, end
-- AB
SELECT utl_encode.uudecode('626567696E2036343420612E7478740D0A224A5C574F') FROM dual;
SELECT utl_encode.uudecode('626567696E2036343420612E7478740A224A5C574F') FROM dual;
SELECT utl_encode.uudecode('626567696E2036343420612E7478740D224A5C574F') FROM dual;
SELECT utl_encode.uudecode('626567696E2036343420612E7478740D0D224A5C574F') FROM dual;
-- ABCDEFAB
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A656E64') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F0A656E64') FROM dual;
-- 0D end is error
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D656E64') FROM dual;
-- ABCDEFAB
SELECT utl_encode.uudecode('264A5C574F4A5C574F656E64') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A0A656E64') FROM dual;
-- ABCDEFABCDEFABCD
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A234A5C574F0D0A656E64') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A234A5C574F0D0A0A656E64') FROM dual;

-- 5.9 data line with 0D 0A:
-- ABCDEFAB
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F0A') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C574F') FROM dual;
-- ABCDEF
SELECT utl_encode.uudecode('244A5C574F0D0D') FROM dual;
SELECT utl_encode.uudecode('244A5C574F0D0D0D') FROM dual;
-- ABCDEFABCDEFABCD
SELECT utl_encode.uudecode('264A5C574F4A5C574F0D0A234A5C574F0D0A') FROM dual;

-- 0D0D0D0D K=3 ERROR
SELECT utl_encode.uudecode('244A5C574F0D0D0D0D') FROM dual;
-- 0D0D0D0A K=3 ERROR
SELECT utl_encode.uudecode('244A5C574F0D0D0D0A') FROM dual;

SELECT utl_raw.compare(utl_encode.uudecode('626567696E2036343420612E7478740D0A224A5C574F0A656E64'), 'AB') FROM dual;
SELECT utl_raw.compare(utl_encode.uudecode('626567696E2036343420612E7478740D0A264A5C574F4A5C574F0D0A234A5C574F0D0A656E64'), 'ABCDEFABCDEFABCD') FROM dual;

-- 5.10 too much 0D 0A, null, '', not enough bytes with error
SELECT utl_encode.uudecode('234A5C574F0D0D0D234A5C574F') FROM dual;
SELECT utl_raw.compare(utl_encode.uudecode('234A5C574F0D0D0D'), 'ABCD') FROM dual;
SELECT utl_encode.uudecode('234A5C574F0D0A0D0A234A5C574F') FROM dual;
SELECT utl_encode.uudecode(NULL) FROM dual;
SELECT utl_encode.uudecode('') FROM dual;
SELECT utl_encode.uudecode('264A5C574F4A5C') FROM dual;

SELECT utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode('48656C6C6F')), '48656C6C6F') FROM dual;
SELECT utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode(utl_raw.copies('41', 100))), utl_raw.copies('41', 100)) FROM dual;
SELECT utl_raw.compare(utl_encode.uudecode(utl_encode.uuencode(utl_raw.copies('42', 1000))), utl_raw.copies('42', 1000)) FROM dual;

RESET SESSION AUTHORIZATION;
DROP USER u_utl_encode CASCADE;