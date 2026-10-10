CREATE EXTENSION hg_ddos_guard;
ALTER SYSTEM SET ddos.max_login_entries = 20000;
ALTER SYSTEM SET ddos.max_sql_entries = 20000;
ALTER SYSTEM SET ddos.enable = 'true';
ALTER SYSTEM SET ddos.login_enable = 'yes';
ALTER SYSTEM SET ddos.sql_enable = '1';
ALTER SYSTEM SET ddos.login_window = '100ms';
ALTER SYSTEM SET ddos.sql_window = '100ms';
ALTER SYSTEM SET ddos.enable = 'on';
ALTER SYSTEM SET ddos.login_enable = 'on';
ALTER SYSTEM SET ddos.sql_enable = 'on';
SELECT pg_reload_conf();
SELECT pg_sleep(1) IS NULL;
SELECT pg_sleep(2) IS NULL;
SELECT hg_security.hg_ddos_unlock_login('127.0.0.1', 'nobody');
SELECT hg_security.hg_ddos_unlock_login_ip('127.0.0.1', 'only');
SELECT hg_security.hg_ddos_unlock_sql('127.0.0.1', 'nobody');
SELECT count(*) >= 0 FROM hg_security.hg_ddos_login_status();
SELECT count(*) >= 0 FROM hg_security.hg_ddos_sql_status();
SELECT count(*) >= 0 FROM hg_security.hg_ddos_login_status_view;
SELECT count(*) >= 0 FROM hg_security.hg_ddos_login_ip_status_view;
SELECT count(*) >= 0 FROM hg_security.hg_ddos_sql_status_view;
SELECT login_hash_entries >= 0 AND sql_hash_entries >= 0
  FROM hg_security.hg_ddos_stats();
SELECT login_hash_entries >= 0 AND sql_hash_entries >= 0
  FROM hg_security.hg_ddos_stats_view;
SELECT login_hash_entries = 0 FROM hg_security.hg_ddos_stats();
SELECT hg_security.hg_ddos_unlock_sql('local', current_user::name);
DROP EXTENSION hg_ddos_guard CASCADE;
