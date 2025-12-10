/* Begin - bug#z5409 */
create table numeric_test(val numeric(10, 6));
drop table numeric_test;

create table numeric_test(val decimal(10, 6));
drop table numeric_test;

create table numeric_test(val dec(10, 6));
drop table numeric_test;

create table numeric_test(val fixed(10, 6));
drop table numeric_test;
/* End - bug#z5409 */

/* Begin - bug#z6342 */
create table test_priority(a int1, b tinyint default 10, c tinyint signed );
insert into test_priority (a,c) values (5,2);
select a/b+c from test_priority ;
select 10/5 + 1::smallint;
select 10/5 + 1::tinyint;
drop table test_priority;
/* End - bug#z6342 */

/* Begin - bug#z6503 */
select 
convert(1000.00, decimal(5,2)) , 
convert(12345.67, decimal(5,2)), 
convert(-1000.00, decimal(5,2));

select convert(123.456, decimal(10,10)) , 
convert(1234.567, decimal(5,2));
/* End - bug#z6503 */

/* Begin - bug#6506 */
create table tttt (a decimal(0, 0));
desc tttt;
drop table tttt;

select convert(123.456, decimal(0,0));
/* End - bug#6506 */

/* Begin - bug#6526 */
select CAST('1.8446744e19' AS decimal(65,30));
select convert(CAST('1.8446744e19' AS decimal(65,30)), signed);
select convert(CAST('-1.8446744e19' AS decimal(65,30)), signed);
/* End - bug#6526 */