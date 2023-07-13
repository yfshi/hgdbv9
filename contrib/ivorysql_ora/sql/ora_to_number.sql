--
-- SHOW / SET system_variable value
--
show num numw numwi numwid numwidt numwidth

set num 1
show num

set numw 2
show num

set numwi 3
show num

set numwid 4
show num

set numwidt 5
show num

set numwidth 6
show num

set num 2 numw 3 numwi 4 numwid 5 numwidt 6 numwidth 7
show num

set num -1

set schema 'test';
show search_path;
reset search_path;

--
-- TO_NUMBER()
--
set numw 10

select to_number('') from dual;
select to_number(null) from dual;

select to_number(1E-130) from dual;
select to_number(1E-131) from dual;

select to_number(9.999E125) from dual;
select to_number(9.9999E125) from dual;
select to_number(1E126) from dual;

select to_number(0.987654321) from dual;
select to_number(-0.987654321) from dual;

select to_number(cast(BINARY_FLOAT_NAN as binary_float)) from dual;
select to_number(cast(BINARY_FLOAT_INFINITY as binary_float)) from dual;

select to_number(cast(1.23456-38F as binary_float)) from dual;
select to_number(cast(1.23456E+39F as binary_float)) from dual;

select to_number(cast(987654.321 as binary_float)) from dual;
select to_number(cast(-987654.321 as binary_float)) from dual;

select to_number(cast(BINARY_DOUBLE_NAN as binary_double)) from dual;
select to_number(cast(BINARY_DOUBLE_INFINITY as binary_double)) from dual;

select to_number(cast(1.23456E-308D as binary_double)) from dual;
select to_number(cast(1.23456E+308D as binary_double)) from dual;

select to_number(cast(987654.321 as binary_double)) from dual;
select to_number(cast(-987654.321 as binary_double)) from dual;

select to_number(cast('9876543211' as char(10))) from dual;
select to_number(cast('98765432112' as char(11))) from dual;

select to_number(cast('9876543211' as char(11))) from dual;
select to_number(cast('98765432112' as char(12))) from dual;

--
-- TO_NUMBER(expr DEFAULT return_value ON CONVERSION ERROR)
--
select to_number('1,2' default null on conversion error) from dual;
select to_number('1,2' default '0' on conversion error) from dual;
select to_number('1,2' default '0,0' on conversion error) from dual;

--
-- TO_NUMBER(expr, fmt)
--
select to_number(123, '999') from dual;
select to_number(' 123', '999') from dual;
select to_number('123 ', '999') from dual;
select to_number('123 ', '999 ') from dual;

select to_number('+123', '999') from dual;
select to_number('-123', '999') from dual;

select to_number(cast('234' as binary_float),'999')  from dual;
select to_number(cast('234' as binary_double),'999')  from dual;


-- PR MI S
select to_number('123', '999PR') from dual;
select to_number('<123>', '999PR') from dual;
select to_number('<123', '999PR') from dual;

select to_number('123', '999MI') from dual;
select to_number('123 ', '999MI') from dual;
select to_number('123+', '999MI') from dual;
select to_number('123-', '999MI') from dual;

select to_number('123+', '999S') from dual;
select to_number('123-', '999S') from dual;
select to_number('123 ', '999S') from dual;
select to_number('123', '999S') from dual;
select to_number('+123', 'S999') from dual;
select to_number('-123', 'S999') from dual;
select to_number('123', 'S999') from dual;

-- $ C L U
select to_number('$123', '$999') from dual;
select to_number('$123', '99$9') from dual;
select to_number('1$23', '9$99') from dual;

alter session set nls_iso_currency = CHINA;
select to_number('CNY123', 'C999') from dual;
select to_number('cny123', 'C999') from dual;
select to_number('CNY123', '9C99') from dual;
select to_number('1CNY23', '9C99') from dual;
select to_number('1cny23', '9C99') from dual;
select to_number('123CNY', '999C') from dual;
select to_number('123cny', '999C') from dual;

alter session set nls_currency = abcabcabcabc;
select to_number('abcabcabcabc123', 'L999') from dual;
select to_number('abcabcabca123', 'L999') from dual;
select to_number('1ABCABCABCA23', '9L99') from dual;
reset nls_currency;

alter session set nls_dual_currency = abcabcabc;
select to_number('abcabcabc123', 'U999') from dual;
select to_number('abcabca123', 'U999') from dual;
select to_number('1ABCABCA23', '9U99') from dual;
reset nls_dual_currency;

-- EEEE
select to_number('1.23e-04', '9.99eEEE') from dual;
select to_number('1.23e-04', '9.99EEEE') from dual;
select to_number('1.23e-04 ', '9.99EEEEMI') from dual;
select to_number('1.23e-04+', '9.99EEEEMI') from dual;
select to_number('1.23e-04-', '9.99EEEEMI') from dual;
select to_number('12.34e-04', '99.99EEEE') from dual;
select to_number('1￥23e-04', '9L99EEEE') from dual;
select to_number('-1￥23e-04', 'S9L99EEEE') from dual;
select to_number('1￥23e-04+', '9L99EEEES') from dual;
select to_number('1￥23e-04-', '9L99EEEES') from dual;
select to_number('1￥23e-04', '9L99EEEEPR') from dual;
select to_number('<1￥23e-04>', '9L99EEEEPR') from dual;
select to_number('<1￥23e-04', '9L99EEEEPR') from dual;
select to_number('1￥23e-04 ', '9L99EEEEMI') from dual;
select to_number('1￥23e-04+', '9L99EEEEMI') from dual;
select to_number('1￥23e-04+', '9L99EEEEMI') from dual;
select to_number('1,2.3e-04', '9,9.9EEEE') from dual;

-- , G
select to_number('1,2,34,567,890', '9,9,99,999,999') from dual;
select to_number('123,456,7890', '999,9999999') from dual;
select to_number('123,456,7890', '999999,9999') from dual;
select to_number('12,345,678,,,,,,90', '999,999,999,,,,,,99') from dual;
select to_number('12,345,678,,,,,,90', '999,999,999,99') from dual;
select to_number('12,345,,678,,,,,,90', '999,999,999,,,,,,99') from dual;
select to_number('1,,,,,,2.3', '9,,,,,,9.9') from dual;
select to_number('1,,,,,,2.3', '9gggggg9d9') from dual;
select to_number(',123', ',999') from dual;
select to_number('1.2,3', '9.9,9') from dual;
select to_number('1,2.3', '9g9.9') from dual;
select to_number('1,2.3', '9,9d9') from dual;

-- X
select to_number('FFFF', 'XXXX') from dual;
select to_number('F,,,F,,F,F', 'XXXX') from dual;

--
-- TO_NUMBER(expr DEFAULT return_value ON CONVERSION ERROR, fmt)
--
select to_number('1,2.3,4' default '1,2.3' on conversion error, '9,9.9') from dual;
select to_number('9,8.7' default '1,2.3' on conversion error, '9,9.9') from dual;

--
-- TO_NUMBER(expr, fmt, 'nlsparam')
--
select to_number('1b2a3', '9g9d9', 'nls_numeric_characters = ab') from dual;
select to_number('1b2a3', '9g9d9', 'nls_numeric_characters = AB') from dual;
select to_number('1 2.3', '9g9d9', 'nls_numeric_characters = ''. ''') from dual;
select to_number('1 2.3', '9g9d9', 'nls_numeric_characters = . ') from dual;
select to_number('1 2.3', '9g9d9', 'nls_numeric_characters =. ') from dual;
select to_number('1a2"3', '9g9d9', 'nls_numeric_characters = "a"') from dual;
select to_number('a''123', 'L999', 'nls_currency = ''a''''''') from dual;
select to_number('''''123', 'L999', 'nls_currency = ''''''''''''') from dual;
select to_number('''a123', 'L999', 'nls_currency = ''''a''') from dual;
select to_number('''a123', 'L999', 'nls_currency = ''''''a''') from dual;
select to_number('1USD23', '9C99', 'nls_iso_currency = america') from dual;
select to_number('--123', 'd999', 'nls_numeric_characters = ''-,''') from dual;
select to_number('--123', 'sd999', 'nls_numeric_characters = ''-,''') from dual;
select to_number('++123', 'd999', 'nls_numeric_characters = ''+,''') from dual;
select to_number('++123', 'sd999', 'nls_numeric_characters = ''+,''') from dual;
select to_number('-123-', 'd999mi', 'nls_numeric_characters = -,') from dual;
select to_number('--123-', 'd999mi', 'nls_numeric_characters = -,') from dual;
select to_number('-123-', 'd999mi', 'nls_numeric_characters = -,') from dual;
select to_number('+1+123', '9d999', 'nls_numeric_characters = +,') from dual;
select to_number('+1+123', 's9d999', 'nls_numeric_characters = +,') from dual;
select to_number('-1+123', '9d999', 'nls_numeric_characters = +,') from dual;
select to_number('-1+123', 's9d999', 'nls_numeric_characters = +,') from dual;
select to_number('1.23e+04', '9.99eeee', 'nls_numeric_characters=.+') from dual;
select to_number('5432112', 'L99', 'nls_currency = 54321') from dual;
select to_number('1254321', '99L', 'nls_currency = 54321') from dual;
select to_number('1543212', '9L9', 'nls_currency = 54321') from dual;
select to_number('12a54321', '99L', 'nls_currency = a54321') from dual;
select to_number('1a543212', '9L9', 'nls_currency = a54321') from dual;

--
-- NLS parameters
--
alter session set nls_numeric_characters = '+a';
alter session set nls_numeric_characters = '-a';
alter session set nls_numeric_characters = '<a';
alter session set nls_numeric_characters = '>a';
alter session set nls_numeric_characters = '1a';
alter session set nls_numeric_characters = ' ,';
show nls_numeric_characters;
alter session set nls_numeric_characters = 'abaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
alter session set nls_numeric_characters = 'abaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';

set numw -1
