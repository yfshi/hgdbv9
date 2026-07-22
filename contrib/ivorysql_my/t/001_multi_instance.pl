use strict;
use warnings;

use PostgresNode;
use TestLib;
use Test::More tests => 13;

my $shared_preload = "libmysql_parser, ivorysql_my";

sub test_show_value {
    my ($node, $db, $guc, $expected, $desc) = @_;
    my $val = $node->safe_psql($db, "SHOW $guc");
    is($val, $expected, $desc);
}

# Helper: start node with given GUC values, return 0 on success
sub try_start {
    my ($node, %gucs) = @_;
    while (my ($k, $v) = each %gucs) {
        $node->append_conf('postgresql.conf', "$k = '$v'");
    }
    my $cmd = $node->installed_command('pg_ctl');
    my $ret = system("$cmd start -D " . $node->data_dir .
                     " -l " . $node->logfile .
                     " -o \"-p " . $node->port . "\" > /dev/null 2>&1");
    return $ret == 0;
}

# ======================================================
# Test 1: Single port + single database (backward compat)
# ======================================================
{
    my $node = get_new_node('legacy_single');
    $node->init;
    my $port = get_free_port();

    $node->append_conf('postgresql.conf', "shared_preload_libraries = '$shared_preload'");
    $node->append_conf('postgresql.conf', "mysql.port = '$port'");
    $node->append_conf('postgresql.conf', "mysql.default_database = 'postgres'");

    $node->start;
    pass("legacy single port starts successfully");

    test_show_value($node, 'postgres', 'mysql.port', $port,
        "legacy: mysql.port = $port");
    test_show_value($node, 'postgres', 'mysql.default_database', 'postgres',
        "legacy: mysql.default_database = postgres");

    $node->stop;
}

# ======================================================
# Test 2: Two ports, two databases
# ======================================================
{
    my $node = get_new_node('multi_two');
    $node->init;
    my $port1 = get_free_port();
    my $port2 = get_free_port();

    $node->append_conf('postgresql.conf',
        "shared_preload_libraries = '$shared_preload'");
    $node->append_conf('postgresql.conf', "mysql.port = '$port1, $port2'");
    $node->append_conf('postgresql.conf',
        "mysql.default_database = 'postgres, postgres'");

    $node->start;
    pass("two ports + two databases starts successfully");

    my $port_val = $node->safe_psql('postgres', "SHOW mysql.port");
    like($port_val, qr/$port1/, "mysql.port contains first port");
    like($port_val, qr/$port2/, "mysql.port contains second port");

    $node->stop;
}

# ======================================================
# Test 3: Mismatched list lengths => startup failure
# ======================================================
{
    my $node = get_new_node('mismatch');
    $node->init;
    my $p1 = get_free_port();
    my $p2 = get_free_port();
    my $p3 = get_free_port();

    my $ok = try_start($node,
        shared_preload_libraries => $shared_preload,
        'mysql.port'             => '$p1, $p2, $p3',
        'mysql.default_database' => 'db1, db2');
    ok(!$ok, "mismatched lengths: startup FATAL");
}


# ======================================================
# Test 4: Empty port list => startup failure
# ======================================================
{
    my $node = get_new_node('empty_port');
    $node->init;

    my $ok = try_start($node,
        shared_preload_libraries => $shared_preload,
        'mysql.port'             => '',
        'mysql.default_database' => 'postgres');
    ok(!$ok, "empty port with non-empty database: startup FATAL");
}

# ======================================================
# Test 5: Same schema name in two databases
# ======================================================
{
    my $node = get_new_node('same_schema');
    $node->init;
    my $p1 = get_free_port();
    my $p2 = get_free_port();

    $node->append_conf('postgresql.conf',
        "shared_preload_libraries = '$shared_preload'");
    $node->append_conf('postgresql.conf', "mysql.port = '$p1, $p2'");
    $node->append_conf('postgresql.conf',
        "mysql.default_database = 'multi_db_a, multi_db_b'");

    $node->start;
    pass("same-schema test: instance started");

    # Create databases
    $node->safe_psql('postgres', "CREATE DATABASE multi_db_a");
    $node->safe_psql('postgres', "CREATE DATABASE multi_db_b");

    # Create same-named schema in each
    $node->safe_psql('multi_db_a', "CREATE SCHEMA sales");
    $node->safe_psql('multi_db_b', "CREATE SCHEMA sales");

    # Create same-named table with different data
    $node->safe_psql('multi_db_a',
        "CREATE TABLE sales.t (val int); INSERT INTO sales.t VALUES (1)");
    $node->safe_psql('multi_db_b',
        "CREATE TABLE sales.t (val int); INSERT INTO sales.t VALUES (2)");

    # Verify isolation
    my $r1 = $node->safe_psql('multi_db_a', "SELECT val FROM sales.t");
    is($r1, '1', "db_a: sales.t has value 1");

    my $r2 = $node->safe_psql('multi_db_b', "SELECT val FROM sales.t");
    is($r2, '2', "db_b: sales.t has value 2");

    # Verify schemas are database-local
    my $sc1 = $node->safe_psql('multi_db_a',
        "SELECT schema_name FROM information_schema.schemata WHERE schema_name='sales'");
    is($sc1, 'sales', "sales schema visible in multi_db_a");

    my $sc2 = $node->safe_psql('multi_db_b',
        "SELECT schema_name FROM information_schema.schemata WHERE schema_name='sales'");
    is($sc2, 'sales', "sales schema visible in multi_db_b");

    # Cleanup
    $node->safe_psql('multi_db_a', "DROP SCHEMA sales CASCADE");
    $node->safe_psql('multi_db_b', "DROP SCHEMA sales CASCADE");
    $node->safe_psql('postgres', "DROP DATABASE multi_db_a");
    $node->safe_psql('postgres', "DROP DATABASE multi_db_b");

    $node->stop;
}