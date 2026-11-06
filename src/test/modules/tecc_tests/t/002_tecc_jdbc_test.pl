#!/usr/bin/perl

# 002_tecc_jdbc_test.pl
#
# TECC JDBC 全链路回归测试

use strict;
use warnings;
use Config;
use PostgresNode;
use TestLib;
use Test::More;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;

my $jdbc_home = $ENV{'TECC_JDBC_HOME'} || '';
if (!$jdbc_home) {
    plan skip_all => "TECC_JDBC_HOME not set, skipping JDBC tests";
}

my @jars = glob("$jdbc_home/pgjdbc/build/libs/postgresql*.jar");
if (!@jars) {
    plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first";
}

my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
if (!-d $test_classes) {
    plan skip_all => "JDBC test classes not found; run './gradlew :postgresql:compileTestJava' first";
}

my $module_dir = "$FindBin::Bin/..";
my $mode = $ENV{'MODE'} // 'both';
my $filter = get_tecc_test_filter();

my $do_raw    = ($mode eq 'both' || $mode eq 'raw');
my $do_common = ($mode eq 'both' || $mode eq 'common');

note("");
note("╔═════════════════════════════╗");
note("║  TECC JDBC  全链路回归测试  ║");
note("╚═════════════════════════════╝");
note("");
note("Starting PostgreSQL node for TECC JDBC tests...");
my $node = get_new_node('tecc_jdbc');
$ENV{LC_ALL} = 'zh_CN.UTF-8';
$node->init;
setup_swsds_ini($node, $module_dir);
$node->append_conf('postgresql.conf', "listen_addresses = 'localhost'\n");
$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
$node->start;

my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;

my ($setup_out, $setup_err) = $node->psql('postgres',
    "CREATE ROLE postgres LOGIN SUPERUSER;");

my $connstr = $node->connstr('postgres');
my $port = $node->port;
my $TAP = Test::More->builder->output;

# ============ 共享用例 ============
note("=== shared用例JDBC原始输出比对 + 规范化输出比对 ===");

my @shared_sql_files;
if ($filter) {
    @shared_sql_files = ("$module_dir/shared/sql/$filter");
} else {
    opendir(my $dh, "$module_dir/shared/sql") or die "Cannot open shared/sql: $!";
    @shared_sql_files = sort map { "$module_dir/shared/sql/$_" } grep { /\.sql$/ } readdir($dh);
    closedir($dh);
}

my $jdbc_jar = $jars[0];
my $tp_passed = 0;
my $tp_total = 0;
my $runner_base = ['java', '-cp', "${test_classes}:${jdbc_jar}",
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', 'postgres', '--password', '',
    '--enable-tecc', '1', '--modes', 'simple'];

foreach my $sql_file (@shared_sql_files) {
    my $base = (split '/', $sql_file)[-1];
    $base =~ s/\.sql$//;
    next unless -f $sql_file;

    note("  SQL: $base");

    # raw 比较
    if ($do_raw) {
        $tp_total++;
        my $test_name = "jdbc_raw: ${base}";
        my $raw_expected = "$module_dir/shared/expected_jdbc/${base}.out";
        my $raw_cmd = [@$runner_base, '--output-format', 'raw', '--sql-file', $sql_file];
        my ($r_stdout, $r_stderr) = run_command($raw_cmd);
        my $raw_out = $r_stdout // '';
        if (defined $r_stderr && $r_stderr ne '') {
            $raw_out .= "\n" . $r_stderr;
        }
        $raw_out =~ s/\r\n/\n/g;
        $raw_out =~ s/\n*\z//;

        my $expected = '';
        $expected = slurp_file($raw_expected) if -f $raw_expected;
        $expected =~ s/\r\n/\n/g;
        $expected =~ s/\n*\z//;
        if ($raw_out ne $expected) {
            my $outdir = "$TestLib::tmp_check/results";
            mkdir $outdir unless -d $outdir;
            my $outfile = "$outdir/jdbc_raw_${base}.out";
            open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
            print $fh $raw_out if $fh;
            close $fh if $fh;
            my $diff = `diff -u "$raw_expected" "$outfile" 2>/dev/null || true`;
            print $TAP "not ok - $test_name ... FAILED\n";
            print $TAP "#       实际输出: $outfile\n";
            print $TAP "#       差异对比: $diff\n" if $diff;
        } else {
            print $TAP "ok - $test_name ... ok\n";
            $tp_passed++;
        }
    }

    # common 比较
    if ($do_common) {
        $tp_total++;
        my $test_name = "jdbc_common: ${base}";
        my $common_expected = "$module_dir/shared/expected_common/${base}.out";
        my $psql_cmd = [@$runner_base, '--output-format', 'psql', '--sql-file', $sql_file];
        my ($p_stdout, $p_stderr) = run_command($psql_cmd);
        my $psql_raw = $p_stdout // '';
        if (defined $p_stderr && $p_stderr ne '') {
            $psql_raw .= "\n" . $p_stderr;
        }

        my @lines = split /\n/, $psql_raw;
        my $in_file = 0;
        my $common_out = '';
        for my $line (@lines) {
            if ($line =~ /^>>>\s+(.+)$/) {
                $in_file = ($1 eq "${base}.sql" || $1 eq $base);
                next;
            }
            if ($in_file) {
                $common_out .= $line . "\n";
            }
        }
        $common_out =~ s/\n*\z//;

        my $expected = '';
        $expected = slurp_file($common_expected) if -f $common_expected;
        $expected =~ s/\r\n/\n/g;

        $common_out = normalize_common_output($common_out);
        $expected = normalize_common_output($expected);

        if ($common_out ne $expected) {
                my $outdir = "$TestLib::tmp_check/results";
                mkdir $outdir unless -d $outdir;
                my $outfile = "$outdir/jdbc_common_${base}.out";
                open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
                print $fh $common_out if $fh;
                close $fh if $fh;
                my $diff = `diff -u "$common_expected" "$outfile" 2>/dev/null || true`;
                print $TAP "not ok - $test_name ... FAILED\n";
                print $TAP "#       实际输出: $outfile\n";
                print $TAP "#       差异对比: $diff\n" if $diff;
            } else {
                print $TAP "ok - $test_name ... ok\n";
                $tp_passed++;
            }
        }
    }

# ============ JDBC 独有 SQL ============

note("=== JDBC独有用例 ===");
my ($lp, $lt) = (0, 0);
if (-d "$module_dir/jdbc/sql") {
    ($lp, $lt) = run_sql_dir_diff($node,
        "$module_dir/jdbc/sql",
        "$module_dir/jdbc/expected",
        'jdbc');
}

restore_swxa_tests($module_dir);
$node->stop('fast');

my $t_pass = $tp_passed + $lp;
my $t_total = $tp_total + $lt;
my $t_fail = $t_total - $t_pass;
print $TAP "\n# ====================================\n";
print $TAP "# $t_pass of $t_total tests passed, $t_fail failed\n";
print $TAP "# ====================================\n";
if ($t_fail > 0) {
    print $TAP "\n# [结果查看] 差异文件和实际输出保存在:\n";
    print $TAP "#     $TestLib::tmp_check/results/\n";
    print $TAP "#   查看所有失败测试的实际输出:\n";
    print $TAP "#       ls $TestLib::tmp_check/results/*.out\n";
    print $TAP "#   查看所有失败测试的差异对比:\n";
    print $TAP "#       ls $TestLib::tmp_check/results/*.diff\n";
    print $TAP "#   查看完整测试日志:\n";
    print $TAP "#       cat $TestLib::tmp_check/regress_log_002_tecc_jdbc_test\n";
    print $TAP "\n";
}
print_failure_summary();
print $TAP "1..$t_total\n";
