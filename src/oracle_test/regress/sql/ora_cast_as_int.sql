/* Begin - BUG#M0000194 */
select cast('' as int2);
select cast('' as int4);
select cast('' as int8);

select cast(' ' as int2);
select cast(' ' as int4);
select cast(' ' as int8);

select cast('32767' as int2);
select cast('2147483647' as int4);
select cast('9223372036854775807' as int8);

select cast('-32768' as int2);
select cast('-2147483648' as int4);
select cast('-9223372036854775808' as int8);

select cast('32768' as int2);
select cast('2147483648' as int4);
select cast('9223372036854775808' as int8);

select cast('32767.4' as int2);
select cast('2147483647.4' as int4);
select cast('9223372036854775807.4' as int8);

select cast('- 32768' as int2);
select cast('- 2147483648' as int4);
select cast('- 9223372036854775808' as int8);

select cast('3.27E+4' as int2);
select cast('2.14E+9' as int4);
select cast('9.22E+18' as int8);

set ivorysql.cast_int_like_oracle to true;

select cast('' as int2);
select cast('' as int4);
select cast('' as int8);

select cast(' ' as int2);
select cast(' ' as int4);
select cast(' ' as int8);

select cast('32767' as int2);
select cast('2147483647' as int4);
select cast('9223372036854775807' as int8);

select cast('-32768' as int2);
select cast('-2147483648' as int4);
select cast('-9223372036854775808' as int8);

select cast('32768' as int2);
select cast('2147483648' as int4);
select cast('9223372036854775808' as int8);

select cast('32767.4' as int2);
select cast('2147483647.4' as int4);
select cast('9223372036854775807.4' as int8);

select cast('32767.5' as int2);
select cast('2147483647.5' as int4);
select cast('9223372036854775807.5' as int8);

select cast(' 32767.4 ' as int2);
select cast(' 2147483647.4 ' as int4);
select cast(' 9223372036854775807.4 ' as int8);

select cast('-32769' as int2);
select cast('-2147483649' as int4);
select cast('-9223372036854775809' as int8);

select cast('- 32768' as int2);
select cast('- 2147483648' as int4);
select cast('- 9223372036854775808' as int8);

select cast('-32768.4' as int2);
select cast('-2147483648.4' as int4);
select cast('-9223372036854775808.4' as int8);

select cast('-32768.5' as int2);
select cast('-2147483648.5' as int4);
select cast('-9223372036854775808.5' as int8);

select cast('3.27E+4' as int2);
select cast('2.14E+9' as int4);
select cast('9.22E+18' as int8);

select cast('3.27674E+4' as int2);
select cast('2.1474836474E+9' as int4);
select cast('9.2233720368547758074E+18' as int8);

select cast('3.27675E+4' as int2);
select cast('2.1474836475E+9' as int4);
select cast('9.2233720368547758075E+18' as int8);
/* End - BUG#M0000194 */
