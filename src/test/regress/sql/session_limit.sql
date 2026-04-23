show session_limit;
create user testuser1;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser1';

create user testuser2;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser2';

create user testuser3;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser3';

alter system set session_limit to 0;
select pg_reload_conf();
select pg_sleep(1);

show session_limit;
create user testuser4;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser4';

alter system set session_limit to 100;
select pg_reload_conf();
select pg_sleep(1);

show session_limit;
create user testuser5;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser5';


alter system set session_limit to 10;
select pg_reload_conf();
select pg_sleep(1);
create user testuser6;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser6';

alter system set session_limit to -1;
select pg_reload_conf();
select pg_sleep(1);

show session_limit;
create user testuser7;
select rolname, rolconnlimit from pg_authid where rolname = 'testuser7';
select rolname, rolconnlimit from pg_authid where rolname = 'testuser1' or rolname = 'testuser2' or rolname = 'testuser3' or rolname = 'testuser4' or rolname = 'testuser5' or rolname = 'testuser6' or rolname = 'testuser7' order by rolname;

drop user testuser1;
drop user testuser2;
drop user testuser3;
drop user testuser4;
drop user testuser5;
drop user testuser6;
drop user testuser7;


alter system set session_limit to -2;
alter system set session_limit to 1025;

--privilege test
create user usr1;
create user session_limit_super superuser;
create user dba_user;
create user sso_user;
create user sao_user;
grant sysdba_role to dba_user;
grant syssso_role to sso_user;
grant syssao_role to sao_user;

\c - usr1
alter system set session_limit to -1; --error

\c - session_limit_super
alter system set session_limit to -1;

\c - dba_user
alter system set session_limit to -1;

\c - sso_user
alter system set session_limit to -1; --error

\c - sao_user
alter system set session_limit to -1; --error

--three privilege enable
\c - session_limit_super
set se_feature.mode to on;
alter system set session_limit to -1; --error

\c - dba_user
set se_feature.mode to on;
alter system set session_limit to -1;

\c - sso_user
set se_feature.mode to on;
alter system set session_limit to -1; --error

\c - sao_user
set se_feature.mode to on;
alter system set session_limit to -1; --error

\c - session_limit_super
drop user usr1;
drop user dba_user;
drop user sso_user;
drop user sao_user;
