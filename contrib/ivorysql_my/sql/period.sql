/*PERIOD_ADD*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_add(''202101'', ''10'')', period_add('202101', '10'));
insert into func_test(functionName, result) values ('period_add(''202101.4'', ''10.5'')', period_add('202101.4', '10.5'));
insert into func_test(functionName, result) values ('period_add(202101.4, 10.5)', period_add(202101.4, 10.5));
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_add(0.0, 1)', period_add(0, 1)); -- p == 0
insert into func_test(functionName, result) values ('period_add(200000, 0)', period_add(200000, 0)); 
insert into func_test(functionName, result) values ('period_add(-202205, 1)', period_add(-202205, 1)); -- p < 0
insert into func_test(functionName, result) values ('period_add(2222222222222222222222222202209, 1)', period_add(2222222222222222222222222202209, 1));
insert into func_test(functionName, result) values ('period_add(202211, 100)', period_add(202211, 100)); 
insert into func_test(functionName, result) values ('period_add(197005, 7)', period_add(197005, 7));
insert into func_test(functionName, result) values ('period_add(202205, -12)', period_add(202205, -12)); -- N < 0 
insert into func_test(functionName, result) values ('period_add(1,12)', period_add(1,12)); 
insert into func_test(functionName, result) values ('period_add(6901,12)', period_add(6901,12));
insert into func_test(functionName, result) values ('period_add(7001,12)', period_add(7001,12)); 
insert into func_test(functionName, result) values ('period_add(10001,12)', period_add(10001,12)); 
insert into func_test(functionName, result) values ('period_add(10001, -12)', period_add(10001, -12)); 
insert into func_test(functionName, result) values ('period_add(10001, -12*30-1)', period_add(10001, -361));
select * from func_test;
drop table func_test;
/*PERIOD_DIFF*/
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_diff(''202101'', ''202102'')', period_diff('202101', '202102')); 
insert into func_test(functionName, result) values ('period_diff(''202101.4'', ''202102.5'')', period_diff('202101.4', '202102.5')); 
insert into func_test(functionName, result) values ('period_diff(202101.4, 202102.5)', period_diff(202101.4, 202102.5)); 
select * from func_test;
drop table func_test;
create table func_test(functionName varchar(256),result varchar(256));
insert into func_test(functionName, result) values ('period_diff(NULL, 200001)', period_diff(NULL, 200001));
insert into func_test(functionName, result) values ('period_diff(202011, NULL)', period_diff(202011, NULL));
insert into func_test(functionName, result) values ('period_diff(-202011, 200001)', period_diff(-202011, 200001)); -- P1<0
insert into func_test(functionName, result) values ('period_diff(202011, -200001)', period_diff(202011, -200001)); -- P2<0
insert into func_test(functionName, result) values ('period_diff(707712, 202201)', period_diff(707712, 202201));
insert into func_test(functionName, result) values ('period_diff(6912, 10001)', period_diff(6912, 10001));
insert into func_test(functionName, result) values ('period_diff(7001, 10001)', period_diff(7001, 10001));
insert into func_test(functionName, result) values ('period_diff(10002, 6901)', period_diff(10002, 6901));
insert into func_test(functionName, result) values ('period_diff(10002, 7001)', period_diff(10002, 7001));
insert into func_test(functionName, result) values ('period_diff(0, 30)', period_diff(0, 30)); -- P1 == 0
insert into func_test(functionName, result) values ('period_diff(30, 0)', period_diff(30, 0)); -- P2 == 0
select * from func_test;
drop table func_test;


select period_diff('!@#', '!@#');   /* bug#z5589 */
