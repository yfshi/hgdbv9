#!/usr/bin/perl

# 003_tecc_cross_test.pl
#
# 跨驱动交替一致性测试
# libpq和JDBC交替执行同一组用例，验证两端数据一致。
#
# 用例文件放在 cross/sql/，以数字前缀命名控制顺序：
#   001_create.sql  002_insert.sql  003_select.sql  ...
# 不标注驱动，由本脚本按交替规则分配驱动执行。
#
# 执行两轮交替：
#   正向：001→libpq, 002→jdbc, 003→libpq, 004→jdbc ...
#   反向：001→jdbc,  002→libpq, 003→jdbc,  004→libpq ...
# 每步结果均经过归一化后与 cross/expected/<同名>.out 对比

use strict;
use warnings;
use PostgresNode;
use TestLib;
use Test::More;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;

my $module_dir = "$FindBin::Bin/..";
my $sql_dir     = "$module_dir/cross/sql";
my $exp_dir     = "$module_dir/cross/expected";

# 读取用例文件（数字前缀 .sql）
opendir(my $dh, $sql_dir) or plan skip_all => "cross/sql not found";
my @sql_files = sort grep { /^\d+.*\.sql$/ } readdir($dh);
closedir($dh);

my $n = scalar @sql_files;
if ($n == 0) {
    plan skip_all => "No cross test SQL files found";
}

# 校验 JDBC 环境
my $jdbc_home = $ENV{'TECC_JDBC_HOME'} or plan skip_all => "TECC_JDBC_HOME not set, required for cross test";
my @jars = glob("$jdbc_home/pgjdbc/build/libs/postgresql*.jar");
plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first" unless @jars;
my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
plan skip_all => "JDBC test classes not found; run 'make jdbc-compile' first" unless -d $test_classes;
my $jdbc_jar = $jars[0];

# 总用例数 = 正向 n 个 + 反向 n 个
plan tests => $n * 2;

# 启动PG节点（两个驱动共享同一个实例）
note("Starting PostgreSQL node for cross-driver tests ($n files)...");
my $node = get_new_node('tecc_cross');
$ENV{LC_ALL} = 'zh_CN.UTF-8';
$node->init;
setup_swsds_ini($node, $module_dir);
$node->append_conf('postgresql.conf', "listen_addresses = 'localhost'\n");
$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
$node->start;
my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;
my $connstr = $node->connstr('postgres');
my $port = $node->port;

# TeccSqlTestRunner 基础参数（使用当前 OS 用户连接）
my $os_user = $ENV{USER} || $ENV{LOGNAME} || 'postgres';
my $runner_base = ['java', '-cp', "${test_classes}:${jdbc_jar}",
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', $os_user, '--password', '',
    '--enable-tecc', '1', '--modes', 'simple'];

# 驱动执行子过程
sub run_step {
    my ($sql_file, $exp_file, $label, $driver) = @_;

    my $output;
    if ($driver eq 'jdbc') {
        # JDBC 执行：通过 TeccSqlTestRunner 以 psql 兼容格式输出
        my $cmd = [@$runner_base, '--output-format', 'psql', '--sql-file', $sql_file];
        my ($stdout, $stderr) = run_command($cmd);
        $output = $stdout // '';
        # 合并 stderr（NOTICE/WARNING/ERROR）
        if (defined $stderr && $stderr ne '') {
            $stderr =~ s/\r\n/\n/g;
            $output .= "\n" . $stderr;
        }
        # 提取 >>> filename.sql 标记之后的内容（过滤前面的进度输出）
        my @lines = split /\n/, $output;
        $output = '';
        my $in_file = 0;
        for my $line (@lines) {
            if ($line =~ /^>>>\s+(.+)$/) {
                $in_file = 1;
                next;
            }
            if ($in_file) {
                $output .= $line . "\n";
            }
        }
        $output =~ s/\n*\z//;
    } else {
        # libpq 执行：psql -A -t，2>&1 合并 stderr 保持服务端返回时序
        # （NOTICE 在对应 DROP 语句前，与 JDBC 行为一致）
        {
            my $psql_bin = $node->installed_command('psql');
            $output = `$psql_bin -X -A -t -d '$connstr' -f '$sql_file' 2>&1`;
            $output //= '';
        }
    }

    $output =~ s/\r\n/\n/g;
    $output = normalize_common_output($output);

    my $expected = '';
    $expected = slurp_file($exp_file) if -f $exp_file;
    $expected =~ s/\r\n/\n/g;
    $expected = normalize_common_output($expected);

    is($output, $expected, $label);
}

# ========== 第一轮：正向交替 ==========
# 001→libpq, 002→jdbc, 003→libpq, 004→jdbc ...
note("--- Round 1: forward alternating (libpq→jdbc→libpq→jdbc...) ---");
for my $i (0 .. $n - 1) {
    my $sql = $sql_files[$i];
    (my $base = $sql) =~ s/\.sql$//;
    my $driver = ($i % 2 == 0) ? 'libpq' : 'jdbc';
    run_step("$sql_dir/$sql", "$exp_dir/$base.out",
             "fwd[$i] $base via $driver", $driver);
}

# ========== 第二轮：反向交替 ==========
# 001→jdbc, 002→libpq, 003→jdbc, 004→libpq ...
note("--- Round 2: reverse alternating (jdbc→libpq→jdbc→libpq...) ---");
for my $i (0 .. $n - 1) {
    my $sql = $sql_files[$i];
    (my $base = $sql) =~ s/\.sql$//;
    my $driver = ($i % 2 == 1) ? 'libpq' : 'jdbc';  # 反向
    run_step("$sql_dir/$sql", "$exp_dir/$base.out",
             "rev[$i] $base via $driver", $driver);
}

restore_swxa_tests($module_dir);
$node->stop('fast');

done_testing();

print_failure_summary();
