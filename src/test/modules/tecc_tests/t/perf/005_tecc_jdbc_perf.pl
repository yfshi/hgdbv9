#!/usr/bin/perl

# 005_tecc_jdbc_perf.pl
#
# TECC JDBC 性能测试
# 所有数据操作（DDL、数据初始化、性能测量）全部通过 JDBC TeccSqlTestRunner 执行。
# 数据生成策略（种子+倍增）与 libpq 侧相同，但通过 JDBC 加密。
#
# 执行:
#   1. 启动PG节点
#   2. DDL（perf_ddl.sql，通过 TeccSqlTestRunner）
#   3. 数据初始化（生成 SQL 文件，通过 TeccSqlTestRunner 执行 JDBC 插入）
#   4. TeccSqlTestRunner --perf-mode 测量所有测例
#   5. 解析TSV输出 → 阈值校验 → TAP输出

use strict;
use warnings;
use Config;
use PostgresNode;
use TestLib;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);
use POSIX ();

use FindBin;
use lib "$FindBin::Bin/..";
use tecc_perf_utils;

my $jdbc_home = $ENV{'TECC_JDBC_HOME'} || '';
if (!$jdbc_home) {
    plan skip_all => "TECC_JDBC_HOME not set, skipping JDBC perf tests";
}

my $TAP = Test::More->builder->output;
my $module_dir = "$FindBin::Bin/../..";

# 定位JDBC jar和测试类
my @jars = glob("$jdbc_home/pgjdbc/build/libs/postgresql*.jar");
if (!@jars) {
    plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first";
}

my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
if (!-d $test_classes) {
    plan skip_all => "JDBC test classes not found; run 'make jdbc-compile' first";
}

my $jdbc_jar = $jars[0];

# 构造 java 命令（执行 SQL 文件的基础参数）
sub build_java_cmd {
    my ($port, $extra_args) = @_;
    my @cmd = (
        'java', '-cp', "${test_classes}:${jdbc_jar}",
        'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
        '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
        '--user', 'postgres', '--password', '',
        '--enable-tecc', '1',
    );
    push @cmd, @$extra_args if $extra_args;
    return @cmd;
}

# 通过 Java runner 执行单个 SQL 文件（simple 模式，静默输出）
sub run_sql_via_jdbc {
    my ($port, $sql_file, $label) = @_;
    $label //= $sql_file;
    print $TAP "#   [jdbc] ${label} ... ";
    my @cmd = build_java_cmd($port, [
        '--sql-file', $sql_file,
        '--modes', 'simple',
        '--output', '/dev/null',
        '--output-format', 'psql'
    ]);
    my ($stdout, $stderr) = run_command(\@cmd);
    # 检查 stderr 中的关键错误
    my $has_error = ($stderr && ($stderr =~ /ERROR|Exception|at org\./));
    if ($has_error) {
        print $TAP "失败\n";
        for my $line (split /\n/, $stderr) {
            print $TAP "#     $line\n" if $line =~ /ERROR|Exception/;
        }
        return 0;
    }
    print $TAP "完成\n";
    return 1;
}

# ============ 启动PG ============
print $TAP "# 启动 PostgreSQL 临时节点...\n";
my $node = get_new_node('tecc_jdbc_perf');
$ENV{LC_ALL} = 'zh_CN.UTF-8';
$node->init;
$node->append_conf('postgresql.conf', "listen_addresses = 'localhost'\n");
$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
$node->start;
my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;

$node->psql('postgres', "CREATE ROLE postgres LOGIN SUPERUSER;");

my $psql = $node->installed_command('psql');
my $port = $node->port;
my $sql_dir = "$module_dir/perf/sql";

# ============ 打印环境信息，方便手工连接分析 ============
print $TAP sprintf "\n# %s\n", '=' x 60;
print $TAP sprintf "# TECC Perf 数据库环境信息\n";
print $TAP sprintf "# %s\n", '-' x 60;
print $TAP sprintf "#  数据目录 : %s\n", $node->data_dir;
print $TAP sprintf "#  端口     : %s\n", $node->port;
print $TAP sprintf "#  连接串   : %s\n", $node->connstr('postgres');
print $TAP sprintf "#  psql     : %s\n", $psql;
print $TAP sprintf "#  KEK 目录 : %s\n", $key_dir;
print $TAP sprintf "# \n";
print $TAP sprintf "#  手工连接: %s -d '%s'\n", $psql, $node->connstr('postgres');
print $TAP sprintf "# %s\n\n", '=' x 60;

my $target_rows = $ENV{PERF_ROWS} // 1000000;
my $seed_max = $ENV{PERF_SEED} // 100000;
my $seed = $target_rows < $seed_max ? $target_rows : $seed_max;

# ============ 1. DDL（通过 JDBC runner）============
print $TAP "\n# === 阶段 1：建表 + KEK/DEK（通过 JDBC） ===\n";
run_sql_via_jdbc($port, "$sql_dir/perf_ddl.sql", 'perf_ddl.sql');
print $TAP "# 建表完成\n";

# ============ 2. 数据初始化（通过 JDBC runner 执行 INSERT）============
print $TAP "\n# === 阶段 2：数据初始化（通过 JDBC 加密） ===\n";
my $t_start = gettimeofday();
my $total_encrypts = 0;

# Phase 2a: 生成并执行种子 INSERT SQL 文件
my $batch_size = 2000;
my @seed_files;
my %stage_tables = (
    int4   => { table => 'stage_int4',   cols => 'val_det, val_ope',          encrypts => 2 },
    float8 => { table => 'stage_float8', cols => 'val_det, val_ope',          encrypts => 2 },
    text   => { table => 'stage_text',   cols => 'val_det, val_ope, val_search', encrypts => 3 },
);

for my $t (keys %stage_tables) {
    my $tbl = $stage_tables{$t}{table};
    my $cols = $stage_tables{$t}{cols};
    $total_encrypts += $seed * $stage_tables{$t}{encrypts};

    my $sqlfile = "/tmp/tecc_perf_seed_${t}_$$.sql";
    push @seed_files, $sqlfile;

    open(my $fh, '>', $sqlfile) or die "Cannot write $sqlfile: $!";
    if ($t eq 'int4') {
        for (my $start = 1; $start <= $seed; $start += $batch_size) {
            my $end = $start + $batch_size - 1;
            $end = $seed if $end > $seed;
            my @vals;
            for my $i ($start .. $end) { push @vals, "($i,$i)"; }
            print $fh "INSERT INTO $tbl ($cols) VALUES " . join(", ", @vals) . ";\n";
        }
    } elsif ($t eq 'float8') {
        for (my $start = 1; $start <= $seed; $start += $batch_size) {
            my $end = $start + $batch_size - 1;
            $end = $seed if $end > $seed;
            my @vals;
            for my $i ($start .. $end) {
                my $f = sprintf("%.6f", $i * 0.000001);
                push @vals, "($f,$f)";
            }
            print $fh "INSERT INTO $tbl ($cols) VALUES " . join(", ", @vals) . ";\n";
        }
    } elsif ($t eq 'text') {
        for (my $start = 1; $start <= $seed; $start += $batch_size) {
            my $end = $start + $batch_size - 1;
            $end = $seed if $end > $seed;
            my @vals;
            for my $i ($start .. $end) {
                push @vals, "('第${i}条测试记录','第${i}条测试记录','第${i}条测试记录')";
            }
            print $fh "INSERT INTO $tbl ($cols) VALUES " . join(", ", @vals) . ";\n";
        }
    }
    close $fh;
}

my $msg = sprintf("#   种子 %d 行（目标 %d 行，%d 次客户端加密）\n",
    $seed, $target_rows, $total_encrypts);
print $TAP $msg;

# 串行执行种子 INSERT（通过 JDBC runner）
for my $t (sort keys %stage_tables) {
    my $label = "seed_$t (${seed} rows)";
    run_sql_via_jdbc($port, "/tmp/tecc_perf_seed_${t}_$$.sql", $label);
}

# 清理种子 SQL 文件
for my $f (@seed_files) { unlink $f; }

# Phase 2b: 倍增——重复执行共享的 perf_multiply.sql
my $t_seed_done = gettimeofday();
my $seed_elapsed = $t_seed_done - $t_start;

open(my $mh, '<', "$sql_dir/perf_multiply.sql") or die "Cannot read perf_multiply.sql: $!";
local $/;
my $mult_one_iter = <$mh>;
close $mh;

if ($target_rows > $seed) {
    my $full_iters = int($target_rows / $seed);
    my $remainder = $target_rows % $seed;

    print $TAP "#   倍增 ${seed} 行/轮，共 ${full_iters} 轮";
    print $TAP " + 余数 ${remainder}\n" if $remainder;
    print $TAP "\n" unless $remainder;

    my $mult_sql = $mult_one_iter x $full_iters;
    if ($remainder > 0) {
        $mult_sql .= "INSERT INTO enc_int4 (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_int4 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_float8 (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_float8 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_text (val_det, val_ope, val_search) "
            . "SELECT val_det, val_ope, val_search FROM stage_text LIMIT $remainder;\n";
    }

    my $mult_tmpfile = "/tmp/tecc_perf_mult_$$.sql";
    open(my $mh2, '>', $mult_tmpfile) or die "Cannot write $mult_tmpfile: $!";
    print $mh2 $mult_sql;
    close $mh2;

    run_sql_via_jdbc($port, $mult_tmpfile, 'multiply');
    unlink $mult_tmpfile;
} else {
    # 目标行数不超过种子数，从 stage 直接复制
    my $copy_sql = $mult_one_iter =~ s/;/ LIMIT $target_rows;/gr;
    my $copy_tmpfile = "/tmp/tecc_perf_copy_$$.sql";
    open(my $ch, '>', $copy_tmpfile) or die "Cannot write $copy_tmpfile: $!";
    print $ch $copy_sql;
    close $ch;

    run_sql_via_jdbc($port, $copy_tmpfile, 'copy_from_stage');
    unlink $copy_tmpfile;
}

my $t_init_done = gettimeofday();
my $s_sec = sprintf("%.1f", $seed_elapsed);
my $t_sec = sprintf("%.1f", $t_init_done - $t_start);
print $TAP "#   初始化完成：种子=${s_sec}s，总计=${t_sec}s\n";

# 输出数据表信息
my $data_dir = $node->data_dir;
my $host_dir = $node->host;
my $conn_info = "psql -h ${host_dir} -p ${port} -U postgres -d postgres";
print $TAP "#\n";
print $TAP "# 预置数据表：\n";
print $TAP "#   enc_int4   (val_det DET, val_ope OPE)                 — ${target_rows} 行\n";
print $TAP "#   enc_float8 (val_det DET, val_ope OPE)                 — ${target_rows} 行\n";
print $TAP "#   enc_text   (val_det DET, val_ope OPE, val_search SEARCH) — ${target_rows} 行\n";
print $TAP "# 数据目录：${data_dir}\n";
print $TAP "# 连接命令：${conn_info}\n";

# ============ 3. 加载阈值 ============
my $thresholds = load_thresholds($module_dir);

# ============ 4. 发现SQL文件 ============
my @sql_files = discover_perf_sql_files($sql_dir);
my $total = scalar @sql_files;
print $TAP "\n# === 阶段 3：JDBC 性能测量（$total 个 SQL 文件） ===\n";

# 清空上次测试残留的 JSON 结果
my $results_dir = "$module_dir/perf/results";
if (-d $results_dir) {
    unlink grep { /\.json$/ } glob "$results_dir/*.json";
}

# ============ 5. 通过TeccSqlTestRunner执行性能测试 ============
my $perf_runs = $ENV{PERF_RUNS} // 5;

my @jfr_args = ();
if (my $jfr_file = $ENV{JFR_FILE}) {
    @jfr_args = ('-XX:StartFlightRecording=filename=' . $jfr_file . ',settings=default,dumponexit=true');
}
my @java_cmd = (
    'java', @jfr_args, '-cp', "${test_classes}:${jdbc_jar}",
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', 'postgres', '--password', '',
    '--enable-tecc', '1',
    '--perf-mode', '--perf-runs', $perf_runs, '--perf-warmup', '1',
    '--sql-dir', $sql_dir,
    '--sql-files', join(',', @sql_files)
);

print $TAP "# 运行：java ... TeccSqlTestRunner --perf-mode\n";
my ($stdout, $stderr) = run_command(\@java_cmd);

# 输出调试信息（stderr 可能包含连接错误等）
if ($stderr && length($stderr) > 0) {
    for my $line (split /\n/, $stderr) {
        print $TAP "# [java-stderr] $line\n";
    }
}

# ============ 6. 解析输出 ============
my @results;
my @lines = split /\n/, ($stdout // '');
my $current_file = '';
my %runs_ms_for_file;

for my $line (@lines) {
    chomp $line;
    # 跳过表头
    if ($line =~ /^# sql_file/) { next; }
    # 文件名注释行: # int4_eq: runs=5
    if ($line =~ /^#\s+(\S+):\s+runs=(\d+)/) {
        $current_file = $1;
        next;
    }
    # runs_ms行: runs_ms: 13, 11, 23
    if ($line =~ /^runs_ms:\s+(.+)$/) {
        my $vals = $1;
        $runs_ms_for_file{$current_file} = [ map { int($_) } split(/\s*,\s*/, $vals) ];
        next;
    }
    # 跳过 perf_ddl.sql 的数据行
    if ($current_file eq 'perf_ddl') { next; }
    # 数据行: int4_eq	5	118	112	125
    if ($line =~ /^(\S+)\t(\d+)\t(\d+)\t(\d+)\t(\d+)$/) {
        my ($name, $runs, $avg, $min, $max) = ($1, $2, $3, $4, $5);
        my $th = $thresholds->{$name};
        my $runs_arr = $runs_ms_for_file{$name} // [];

        my $result = {
            runs_ms   => $runs_arr,
            avg_ms    => $avg,
            min_ms    => $min,
            max_ms    => $max,
            warmup_ms => -1,    # JDBC runner 不单独输出预热时间
            total_ok  => scalar(@$runs_arr),
            total_err => 0,
        };

        print_perf_result($name, $result, $th);
        save_perf_result($module_dir, $name, $result, $th, 'jdbc');

        push @results, {
            name      => $name,
            passed    => check_threshold($result, $th),
            avg       => $avg,
            threshold => $th ? ($th->{threshold_ms} // 0) : 0,
        };
    }
}

# ============ 7. 汇总 ============
print $TAP "\n";
print_perf_table(\@results);
print_perf_summary(\@results);
my $summary_file = save_perf_summary($module_dir, \@results, { driver => 'jdbc' });
my $plan_count = scalar @results;
print $TAP "1..${plan_count}\n";

# ============ 8. 直接退出跳过析构，下次运行时 make check 清理旧数据 ============
POSIX::_exit(0);
