alter system set credcheck.enable to on;
select pg_reload_conf();
select pg_sleep(1);

show credcheck.password_expiration;

create user test_password_expiration_u1 with password 'Test123456@' VALID UNTIL '2222-12-12';

alter system set credcheck.password_expiration = 71817;
select pg_reload_conf();
select pg_sleep(1);
show credcheck.password_expiration;
--SELECT hg_security.user_expiration_info('test_password_expiration_u1');
SELECT rolename from hg_security.user_expiration_info('test_password_expiration_u1'); --test_password_expiration_u1


alter system set credcheck.password_expiration = 1;
select pg_reload_conf();
select pg_sleep(1);
--SELECT hg_security.user_expiration_info('test_password_expiration_u1');
SELECT rolename from hg_security.user_expiration_info('test_password_expiration_u1'); 

drop user test_password_expiration_u1;