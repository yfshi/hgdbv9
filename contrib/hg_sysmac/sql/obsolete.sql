-- Run with make installcheck against an isolated server preloading hg_sysmac.
-- Use an account authorized to execute ALTER SYSTEM (instance superuser).
-- Errors are expected: do not enable ON_ERROR_STOP or use a transaction.
\set ON_ERROR_STOP off
\set VERBOSITY default

-- SET: every explicit value must report the obsolete parameter and migration hint.
SET hg_sysmac.enable = on;
SET hg_sysmac.enable = off;
SET hg_sysmac.enable = '';
SET hg_sysmac.enable = 'unknown_old_value';

SET hg_sysmac.default_label = on;
SET hg_sysmac.default_label = off;
SET hg_sysmac.default_label = '';
SET hg_sysmac.default_label = 'unknown_old_value';

SET hg_sysmac.labelmode = on;
SET hg_sysmac.labelmode = off;
SET hg_sysmac.labelmode = '';
SET hg_sysmac.labelmode = 'unknown_old_value';

SET hg_sysmac.label_impexp = on;
SET hg_sysmac.label_impexp = off;
SET hg_sysmac.label_impexp = '';
SET hg_sysmac.label_impexp = 'unknown_old_value';

-- set_config(): SQL function assignments must report the same diagnostics.
SELECT set_config('hg_sysmac.enable', 'on', false);
SELECT set_config('hg_sysmac.default_label', 'on', false);
SELECT set_config('hg_sysmac.labelmode', 'on', false);
SELECT set_config('hg_sysmac.label_impexp', 'on', false);

-- ALTER SYSTEM SET: reject obsolete parameters before writing auto configuration.
ALTER SYSTEM SET hg_sysmac.enable = on;
ALTER SYSTEM SET hg_sysmac.default_label = 'test_label';
ALTER SYSTEM SET hg_sysmac.labelmode = 'unknown_old_value';
ALTER SYSTEM SET hg_sysmac.label_impexp = on;

-- All ten obsolete names must produce migration diagnostics.
SELECT mac_create_label_policy('p1', 's1:c1023');
SELECT mac_alter_label_policy('p1', 's2:c1023');
SELECT mac_drop_label_policy('p1');
SELECT mac_set_label_policy('p1', 'enable');
SELECT mac_label_to_char(10);
SELECT mac_char_to_label('s1:c1023');
SELECT mac_apply_user_policy('u1', 'p1');
SELECT mac_apply_table_policy('tb1', 'p1');
SELECT mac_remove_user_policy('p1', 'u1');
SELECT mac_remove_table_policy('p1', 'sch1', 'tb1', false);
