#!/usr/bin/perl

# 004_tecc_libpq_perf.pl
#
# TECC libpq 性能测试
# 测量包含客户端加解密的完整往返时间（非EXPLAIN ANALYZE服务端时间）
#
# 数据生成策略:
#   首次运行: \copy 导入明文（客户端加密）→ COPY TO 导出密文
#   后续运行: COPY FROM 直接灌密文（跳过加密）
#
# 执行:
#   1. 启动PG节点
#   2. 执行 perf_ddl.sql（建表+KEK/DEK+索引）
#   3. 初始化加密数据（首次/快速路径自动判断）
#   4. 对每个SQL文件：预热1次 → 测量5次 → 阈值校验
#   5. 输出汇总

use strict;
use warnings;
use Config;
use PostgresNode;
use TestLib;
use Test::More;
use IPC::Run;
use POSIX ();

use FindBin;
use lib "$FindBin::Bin/..";
use tecc_perf_utils;

my $TAP = Test::More->builder->output;
my $module_dir = "$FindBin::Bin/../..";

# 实时进度输出（直接写 stderr，不被 prove 缓冲）
sub _progress { print STDERR $_[0]; print $TAP $_[0]; }

# ============ 启动PG ============
_progress "# 启动 PostgreSQL 节点...\n";
my $node = get_new_node('tecc_libpq_perf');
$ENV{LC_ALL} = 'zh_CN.UTF-8';
$node->init;
$node->start;
my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;

# 创建postgres超级用户角色
$node->psql('postgres', "CREATE ROLE postgres LOGIN SUPERUSER;");

my $connstr = $node->connstr('postgres');
my $psql = $node->installed_command('psql');
my $sql_dir = "$module_dir/perf/sql";

# ============ 打印环境信息，方便手工连接分析 ============
print $TAP sprintf "\n# %s\n", '=' x 60;
print $TAP sprintf "# TECC Perf 数据库环境信息\n";
print $TAP sprintf "# %s\n", '-' x 60;
print $TAP sprintf "#  数据目录 : %s\n", $node->data_dir;
print $TAP sprintf "#  端口     : %s\n", $node->port;
print $TAP sprintf "#  连接串   : %s\n", $connstr;
print $TAP sprintf "#  psql     : %s\n", $psql;
print $TAP sprintf "#  KEK 目录 : %s\n", $key_dir;
print $TAP sprintf "# \n";
print $TAP sprintf "#  手工连接: %s -d '%s'\n", $psql, $connstr;
print $TAP sprintf "# %s\n\n", '=' x 60;

# ============ 1. DDL ============
print $TAP "\n# === 阶段 1：建表 + KEK/DEK + 索引 ===\n";
run_command([$psql, '-X', '-d', $connstr, '-f', "$sql_dir/perf_ddl.sql"]);
_progress "# 建表完成\n";

# ============ 2. 数据初始化 ============
_progress "\n# === 阶段 2：数据初始化 ===\n";
init_encrypted_data($node, $module_dir);
_progress "# 数据初始化完成\n";

# ============ 3. 加载阈值 ============
my $thresholds = load_thresholds($module_dir);

# ============ 4. 发现SQL文件 ============
# 清空上次测试残留的 JSON 结果
my $results_dir = "$module_dir/perf/results";
if (-d $results_dir) {
    unlink grep { /\.json$/ } glob "$results_dir/*.json";
}
my @sql_files = discover_perf_sql_files($sql_dir);
my $total = scalar @sql_files;
_progress sprintf("\n# === 阶段 3：性能测量（%d 个 SQL 文件） ===\n", $total);

# 诊断：检查current_schema和search_path
my ($cs_out, $cs_err);
IPC::Run::run([$psql, '-X', '-At', '-d', $connstr, '-c', "SELECT current_schema()"],
    '>', \$cs_out, '2>', \$cs_err);
chomp $cs_out if defined $cs_out;
print $TAP "# DIAG: current_schema = '$cs_out'\n";
my ($sp_out, $sp_err);
IPC::Run::run([$psql, '-X', '-At', '-d', $connstr, '-c', "SHOW search_path"],
    '>', \$sp_out, '2>', \$sp_err);
chomp $sp_out if defined $sp_out;
print $TAP "# DIAG: search_path = '$sp_out'\n";
# 诊断：检查current_schema是否存在
my ($ns_out, $ns_err);
IPC::Run::run([$psql, '-X', '-At', '-d', $connstr, '-c',
    "SELECT count(*) FROM pg_namespace WHERE nspname = current_schema()"],
    '>', \$ns_out, '2>', \$ns_err);
chomp $ns_out if defined $ns_out;
print $TAP "# DIAG: schema_exists = '$ns_out'\n";

# ============ 5. 执行性能测试 ============
my @results;
my $runs = $ENV{PERF_RUNS} // 5;  # 可通过环境变量覆盖

for my $sql_file (@sql_files) {
    my $base = $sql_file;
    $base =~ s/\.sql$//;
    my $sql_path = "$sql_dir/$sql_file";

    # 读取SQL内容
    open(my $fh, '<', $sql_path) or do {
        print $TAP "not ok - $base ... FAILED (cannot read SQL file)\n";
        push @results, { name => $base, passed => 0, avg => 0, threshold => 0 };
        next;
    };
    my @lines = grep { !/^--/ && !/^\s*$/ } <$fh>;
    close $fh;
    my $sql = join(' ', @lines);
    $sql =~ s/^\s+|\s+$//g;

    # 获取阈值
    my $th = $thresholds->{$base};

    # 测量时序（预热+5次正式）
    _progress sprintf("# [%2d/%2d] %s ...\n", scalar(@results) + 1, $total, $base);
    my $result = psql_timing($node, $sql, runs => $runs);

    # 输出TAP结果
    print_perf_result($base, $result, $th);

    # 保存JSON结果
    save_perf_result($module_dir, $base, $result, $th, 'libpq');

    push @results, {
        name      => $base,
        passed    => check_threshold($result, $th),
        avg       => $result->{avg_ms},
        threshold => $th ? ($th->{threshold_ms} // 0) : 0,
        errors    => $result->{total_err},
    };
}

# ============ 6. 汇总 ============
_progress "\n# === 阶段 4：汇总 ===\n";
print $TAP "\n";
print_perf_table(\@results);
print_perf_summary(\@results);
my $summary_file = save_perf_summary($module_dir, \@results, { driver => 'libpq' });
print $TAP sprintf("1..%d\n", scalar @results);
_progress "# 测试完成\n";

