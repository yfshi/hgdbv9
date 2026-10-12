# Copyright (c) 2021, PostgreSQL Global Development Group
#
# TAP test for the hg_alarm extension alarm API (hg_alarm_check /
# hg_alarm_stat).
#
# The alarm API functions always open a connection to the "highgo" database
# (see valid_connection_param() in common/pqfe.c), so that database must
# exist for the internal connection to succeed.

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;
use File::Temp qw(tempdir tempfile);
use IPC::Run;

if ($ENV{enable_compatible_ak} ne 'yes')
{
	plan skip_all => 'compatible ak not supported by this build';
}
else
{
	plan tests => 12;
}

my $node = get_new_node('test');
$node->init;
$node->start;

# The hg_alarm API always connects to the "highgo" database, so it must
# exist.  Create it and install the extension there.
$node->safe_psql('postgres', 'CREATE DATABASE highgo;');
$node->safe_psql('highgo', 'CREATE EXTENSION hg_alarm;');

# The config's "username" is validated against an existing role that must be a
# member of sysdba_role (see hg_alarm_check in lib/hg_alarm.c: get_role_oid()
# errors if the role does not exist).  The value is only validated, not used
# for the connection (which uses the current session user), so create a
# dedicated sysdba_role member and reference it in the configs.
$node->safe_psql('postgres', 'CREATE ROLE hg_alarm_dba;');
$node->safe_psql('postgres', 'GRANT sysdba_role TO hg_alarm_dba;');

# Config files must live at an absolute path: the backend opens them from its
# own working directory (the data dir), not the test's CWD, so a relative
# filename would not be found.
my $cfgdir = tempdir(CLEANUP => 1);

# Write a JSON configuration file and return its (absolute) path.  The
# host/port are embedded so the API's internal connection reaches this node.
sub write_cfg
{
	my ($host, $port, $content) = @_;
	my ($fh, $path) =
	  tempfile('hg_alarm_cfg_XXXXXX', SUFFIX => '.json', DIR => $cfgdir);
	print $fh $content;
	close $fh;
	return $path;
}

my $host = $node->host;
my $port = $node->port;

# All limits unlimited -> every check passes.
my $cfg_unlimited = write_cfg($host, $port, <<"EOF");
{
    "host" : "$host",
    "port" : "$port",
    "database" : "highgo",
    "username" : "hg_alarm_dba",
    "memory_limit" : "-1",
    "datadir_limit" : "-1"
}
EOF

# A zero memory limit is always exceeded by a live server.
my $cfg_tiny = write_cfg($host, $port, <<"EOF");
{
    "host" : "$host",
    "port" : "$port",
    "database" : "highgo",
    "username" : "hg_alarm_dba",
    "memory_limit" : "0"
}
EOF

# Concrete limits, used to verify hg_alarm_stat reports the configured
# thresholds.
my $cfg_stat = write_cfg($host, $port, <<"EOF");
{
    "host" : "$host",
    "port" : "$port",
    "database" : "highgo",
    "username" : "hg_alarm_dba",
    "memory_limit" : "100MB",
    "datadir_limit" : "200MB"
}
EOF

my ($ret, $stdout, $stderr);

# 1-arg form checks every configured parameter; with all limits unlimited
# the result must be true.
($ret, $stdout, $stderr) =
  $node->psql('highgo', "SELECT hg_alarm_check('$cfg_unlimited');");
is($stdout, 't',
	'hg_alarm_check (all) returns true when limits are unlimited');

# 2-arg form checks a single parameter.
($ret, $stdout, $stderr) =
  $node->psql('highgo',
	"SELECT hg_alarm_check('$cfg_unlimited', 'memory_limit');");
is($stdout, 't',
	'hg_alarm_check (memory_limit) returns true when unlimited');

# A zero memory limit is exceeded by a running server, so the check fails.
($ret, $stdout, $stderr) =
  $node->psql('highgo', "SELECT hg_alarm_check('$cfg_tiny', 'memory_limit');");
is($stdout, 'f',
	'hg_alarm_check (memory_limit) returns false when limit exceeded');

# hg_alarm_stat reports the configured thresholds.  The OUT columns are of
# type cstring (an internal type with no '=' operator), so the rows cannot be
# filtered in SQL; select them all and match in Perl instead.  psql unaligned
# mode uses '|' as the field separator.
($ret, $stdout, $stderr) =
  $node->psql('highgo', "SELECT * FROM hg_alarm_stat('$cfg_stat');");
like($stdout, qr/memory_limit\|[^|]*\|100 MB/,
	'hg_alarm_stat reports memory_limit threshold');

like($stdout, qr/datadir_limit\|[^|]*\|200 MB/,
	'hg_alarm_stat reports datadir_limit threshold');

# Only the sysdba (superuser) role may call the alarm API.  Use SET ROLE in
# the same session to drop superuser privileges, avoiding a second connection.
$node->safe_psql('postgres', 'CREATE ROLE alarm_tester;');
($ret, $stdout, $stderr) =
  $node->psql('highgo',
	"SET ROLE alarm_tester; SELECT hg_alarm_check('$cfg_unlimited');");
like($stderr, qr/can only be called by sysdba_role user/,
	'hg_alarm_check rejects non-superuser');

# A missing configuration file is reported as an open error.
($ret, $stdout, $stderr) =
  $node->psql('highgo',
	"SELECT hg_alarm_check('/nonexistent/hg_alarm_cfg.json');");
like($stderr, qr/open file failed/,
	'hg_alarm_check reports missing config file');

# An unknown parameter name is rejected.
($ret, $stdout, $stderr) =
  $node->psql('highgo',
	"SELECT hg_alarm_check('$cfg_unlimited', 'bogus_type');");
like($stderr, qr/unknown type/, 'hg_alarm_check rejects unknown parameter');

# The hg_alarm executable reports its version and usage.
my ($version_out, $version_err) = ('', '');
my $version_ret = IPC::Run::run(['hg_alarm', '--version'],
	'>', \$version_out, '2>', \$version_err);
ok($version_ret, 'hg_alarm --version runs');
like($version_out, qr/hg_alarm-v1\.0/, 'hg_alarm --version prints version');

my ($help_out, $help_err) = ('', '');
my $help_ret = IPC::Run::run(['hg_alarm', '--help'],
	'>', \$help_out, '2>', \$help_err);
ok($help_ret, 'hg_alarm --help runs');
like($help_out, qr/Usage:/, 'hg_alarm --help prints usage');

unlink $cfg_unlimited, $cfg_tiny, $cfg_stat;
$node->stop;
done_testing();
