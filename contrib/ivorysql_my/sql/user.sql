set mysql.at_or_atat = 1;

create user 'username'@%; -- fail

create user 'username'@127.0.0.1;
drop user 'username'@127.0.0.1;

create user 'username'@'%';
drop user 'username'@'%';

create user 'username'@'127.0.0.1';
drop user 'username'@'127.0.0.1';

create user username@127.0.0.1;
drop user username@127.0.0.1;

-- check mysql_native_password
set password_encryption = 'mysql_native_password';
create user mysql_user password 'qwe123';
select rolpassword from pg_authid where rolname = 'mysql_user';
drop user mysql_user;

set mysql.at_or_atat = 0;