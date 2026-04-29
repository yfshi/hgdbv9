--==============================================================================================--
-- test user schema antitamper
--==============================================================================================--
create user ttt;
create database tbase;
\c tbase;
grant all on DATABASE tbase to ttt;
set role ttt;
create schema ddd with antitamper;
create table  ddd.t1 (id int, name text);
insert into ddd.t1 values(1, 'alex'), (2, 'bob'), (3, 'peter');
select hg_security.antitamper_hist_check('ddd','t1');
select hg_security.antitamper_hist_repair('ddd','t1');
select hg_security.antitamper_gchain_check('ddd','t1');
select hg_security.antitamper_gchain_repair('ddd','t1');
select hg_security.antitamper_gchain_archive('ddd','t1');
drop table ddd.t1;
drop schema ddd;
reset role;

\c postgres;
revoke all on DATABASE postgres from ttt;
drop database tbase;
drop user ttt;
