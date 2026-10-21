
# Copyright (c) 2021, PostgreSQL Global Development Group

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

if ($ENV{enable_compatible_ak} ne 'yes')
{
	plan skip_all => 'compatible ak not supported by this build';
}
else
{
	plan tests => 21;
}

my $node = get_new_node('test');
$node->init;

# The legacy hg_audit.* GUCs are registered in _PG_init, which only runs when
# pg_audit is loaded via shared_preload_libraries.
$node->append_conf('postgresql.conf', "shared_preload_libraries = 'pg_audit'");

$node->start;

# The legacy hg_* function names are intercepted by the undefined-function
# hook, which only fires when audit is enabled.
$node->safe_psql('postgres', "ALTER SYSTEM SET audit.enable = on;");
$node->stop;
$node->start;

# Each of the legacy hg_audit.* parameters must be rejected with the expected
# error message when a user tries to set it.
my ($ret, $stdout, $stderr);

($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_audit.alarm = on;");
like($stderr, qr/parameter "hg_audit.alarm" has been removed/,
	 'hg_audit.alarm reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_audit.alarmemail = 'a\@b.com';");
like($stderr, qr/parameter "hg_audit.arlarmemail" has been removed/,
	 'hg_audit.alarmemail reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_audit.logencryp = 'SM4';");
like($stderr, qr/parameter "hg_audit.logencryp" has been removed/,
	 'hg_audit.logencryp reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_audit.logsize = 16;");
like($stderr, qr/parameter "hg_audit.logsize" has been removed/,
	 'hg_audit.logsize reports removed');

# Each of the legacy hg_* function names must be rejected with the expected
# error message when a user tries to call it.
($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_object('insert','table','t');");
like($stderr, qr/function hg_audit_object has been renamed/,
	 'hg_audit_object reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_noaudit_object('insert','table','t');");
like($stderr, qr/function hg_noaudit_object has been renamed/,
	 'hg_noaudit_object reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_stmt('create table');");
like($stderr, qr/function hg_audit_stmt has been renamed/,
	 'hg_audit_stmt reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_noaudit_stmt('create table');");
like($stderr, qr/function hg_noaudit_stmt has been renamed/,
	 'hg_noaudit_stmt reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_showlog();");
like($stderr, qr/function hg_audit_showlog has been removed/,
	 'hg_audit_showlog reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_file_archive();");
like($stderr, qr/function hg_audit_file_archive has been renamed/,
	 'hg_audit_file_archive reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_add_analyze_rule('rule');");
like($stderr, qr/function hg_audit_add_analyze_rule has been removed/,
	 'hg_audit_add_analyze_rule reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_delete_analyze_rule('rule');");
like($stderr, qr/function hg_audit_delete_analyze_rule has been removed/,
	 'hg_audit_delete_analyze_rule reports removed');

($ret, $stdout, $stderr) = $node->psql('postgres', "SELECT hg_audit_file_delete('file');");
like($stderr, qr/function hg_audit_file_delete has been removed/,
	 'hg_audit_file_delete reports removed');

# The remaining legacy hg_audit.* parameters are intercepted by check hooks
# that report the parameter has been renamed. hg_audit.enable is PGC_USERSET,
# so a session SET reaches the hook.
($ret, $stdout, $stderr) = $node->psql('postgres', "SET hg_audit.enable = on;");
like($stderr, qr/parameter "hg_audit.enable" has been renamed/,
	 'hg_audit.enable reports renamed');

# The rest are PGC_POSTMASTER (or PGC_SIGHUP for fullmode), so a session SET
# is blocked by the context gate before the hook runs. ALTER SYSTEM validates
# the value directly (PGC_S_FILE) and reaches the check hook.
($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.fullmode = 2;");
like($stderr, qr/parameter "hg_audit.fullmode" has been renamed "audit.logpolicy"/,
	 'hg_audit.fullmode reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.retentionpolicy = 'time';");
like($stderr, qr/parameter "hg_audit.retentionpolicy" has been renamed "audit.retentionpolicy"/,
	 'hg_audit.retentionpolicy reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.retentiondays = 30;");
like($stderr, qr/parameter "hg_audit.retentiondays" has been renamed/,
	 'hg_audit.retentiondays reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.spacelimit = 10240;");
like($stderr, qr/parameter "hg_audit.spacelimit" has been renamed "audit.spacelimit"/,
	 'hg_audit.spacelimit reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.spacealarm = '0.8';");
like($stderr, qr/parameter "hg_audit.spacealarm" has been renamed "audit.spacealarm"/,
	 'hg_audit.spacealarm reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.logarchdir = '/tmp/arch';");
like($stderr, qr/parameter "hg_audit.logarchdir" has been renamed "audit.archdir"/,
	 'hg_audit.logarchdir reports renamed');

($ret, $stdout, $stderr) = $node->psql('postgres', "ALTER SYSTEM SET hg_audit.systemopers = 'all';");
like($stderr, qr/parameter "hg_audit.systemopers" has been renamed "audit.systemopers"/,
	 'hg_audit.systemopers reports renamed');

$node->stop;

done_testing();
