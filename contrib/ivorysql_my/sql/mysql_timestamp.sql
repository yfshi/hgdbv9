-- #bug 8339
CREATE TABLE timestamp_01(dt TIMESTAMP);
-- error, ubuntu timezone is UTC not CST, not test.
-- insert into timestamp_01 values('1970-01-01 00:00:01.000000');
-- error
insert into timestamp_01 values('1969-12-31 23:59:59.000000');
-- error
insert into timestamp_01 values('2038-01-29 03:14:08.999999');

insert into timestamp_01 values('1970-01-01 08:00:01.000000');
insert into timestamp_01 values('2038-01-19 03:14:07.999999');
insert into timestamp_01 values('2020-12-12 23:29:59.569853');
select * from timestamp_01;