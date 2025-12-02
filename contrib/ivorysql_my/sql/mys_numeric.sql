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