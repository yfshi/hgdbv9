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

set mysql.at_or_atat = 0;