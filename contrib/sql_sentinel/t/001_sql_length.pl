# Copyright (c) 2021-2026, PostgreSQL Global Development Group

# Verify the sql_sentinel.max_sql_length GUC and the enforcement of the SQL
# statement length limit provided by the sql_sentinel extension.

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

my $node = get_new_node('main');
$node->init;

# Load the sql_sentinel extension at server startup so its GUC and
# post_parse_analyze_hook take effect.
$node->append_conf('postgresql.conf',
	"shared_preload_libraries = 'sql_sentinel'");
$node->start;

# The parameter defaults to its maximum value, 1GB.
is($node->safe_psql('postgres', 'SHOW sql_sentinel.max_sql_length'),
	'1GB', 'default sql_sentinel.max_sql_length is 1GB');

# Sanity-check the parameter bounds and context.
is($node->safe_psql('postgres',
		q{SELECT min_val, max_val, context, unit
		   FROM pg_settings WHERE name = 'sql_sentinel.max_sql_length'}),
	'100|1073741824|superuser|B', 'min/max/context/unit are as expected');

# An oversized SQL statement must be rejected with a clear error.
{
	my $long_sql = 'SELECT ' . ("'" . ('x' x 500) . "'");

	my ($ret, $stdout, $stderr) = $node->psql(
		'postgres',
		"SET sql_sentinel.max_sql_length = '100B'; $long_sql",
		on_error_stop => 0);

	like($stderr, qr/SQL statement is too long/,
		'oversized SQL statement is rejected');
}

# A statement under the configured limit still executes normally.
like($node->safe_psql('postgres',
		"SET sql_sentinel.max_sql_length = '1MB'; SELECT 1;"),
	qr/^1$/m, 'normal SQL is accepted under the limit');

# RESET restores the compiled-in default.
$node->safe_psql('postgres', 'RESET sql_sentinel.max_sql_length');
is($node->safe_psql('postgres', 'SHOW sql_sentinel.max_sql_length'),
	'1GB', 'RESET restores the default');

$node->stop('fast');

done_testing();
