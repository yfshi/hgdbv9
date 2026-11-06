package tecc_perf_utils;

# tecc_perf_utils.pm — TECC性能测试公共模块
# 提供: psql_timing, threshold检查, 结果汇总
#
# 使用方式:
#   use tecc_perf_utils;
#   my $result = psql_timing($node, $sql, runs => 5);

use strict;
use warnings;
use Exporter 'import';
use IPC::Run qw(run);
use Time::HiRes qw(gettimeofday tv_interval);
use JSON::PP;
use TestLib;

# 不使用File::Slurp（非核心模块），用标准IO代替
sub _slurp {
    my ($file) = @_;
    open(my $fh, '<', $file) or die "Cannot read $file: $!";
    local $/;  # slurp mode
    my $content = <$fh>;
    close $fh;
    return $content;
}

our @EXPORT = qw(
    psql_timing
    load_thresholds
    check_threshold
    print_perf_result
    print_perf_summary
    print_perf_table
    save_perf_result
    save_perf_summary
    discover_perf_sql_files
    get_tecc_test_filter
    init_encrypted_data
);

# 获取TAP输出句柄
my $TAP;
sub _tap { $TAP //= Test::More->builder->output; }

# ============ 单SQL时序测量（libpq） ============
#
# 执行指定SQL N次，返回每次的耗时(ms)
# 第1次为预热（warmup），后续为正式测量
#
# 用单psql会话执行所有run（多个-c选项共享同一连接），确保TECC缓存
# 只初始化一次。消除了每次spawn进程的启动+缓存开销，更贴近连接池真实场景。
#
# 参数:
#   $node    - PostgresNode对象
#   $sql     - 要执行的SQL语句
#   runs     - 总执行次数（含预热，默认5）
#
# 返回:
#   { runs_ms => [...], avg_ms => N, min_ms => N, max_ms => N, warmup_ms => N }

sub psql_timing {
    my ($node, $sql, %opts) = @_;
    my $runs = $opts{runs} // 5;
    my $psql = $node->installed_command('psql');
    my $connstr = $node->connstr('postgres');

    # 多个-c选项在同一连接中顺序执行，psql的\timing输出每条的耗时
    # 重要: 用单引号避免Perl将\t解释为制表符
    my @cmd = ($psql, '-X', '-At', '-d', $connstr, '-c', '\timing on');
    push @cmd, ('-c', $sql) for (1 .. $runs);

    my ($stdout, $stderr);
    my $ok = run(\@cmd, '>', \$stdout, '2>', \$stderr);

    # 从 stdout 和 stderr 解析 Time: 行（psql 可能将 \timing 输出到任意流）
    my @all_times_ms;
    for my $line (split /\n/, $stdout) {
        if ($line =~ /Time:\s*([\d.]+)\s*ms/i) {
            push @all_times_ms, int($1 + 0.5);
        }
    }
    for my $line (split /\n/, ($stderr // '')) {
        if ($line =~ /Time:\s*([\d.]+)\s*ms/i) {
            push @all_times_ms, int($1 + 0.5);
        }
    }

    # 检测错误
    my @errors;
    my $has_error = !$ok || ($stderr && ($stderr =~ /ERROR:/ || $stderr =~ /FATAL:/ || $stderr =~ /PANIC:/));

    if ($has_error) {
        my $seen = scalar @all_times_ms;
        if ($seen < $runs) {
            # 部分查询中断（FATAL或psql终止），后续无Time:行
            for my $i (($seen + 1) .. $runs) {
                my $err_text = $stderr || '(unknown error)';
                chomp $err_text;
                # Keep first 20 lines of error for debugging
                my @lines = split /\n/, $err_text;
                my $short_err = join("\n", @lines[0..19]);
                push @errors, { run => $i, elapsed_ms => 0, error => $short_err };
            }
            @all_times_ms = splice(@all_times_ms, 0, $seen);
        } else {
            # 所有查询都有Time:行但stderr有错误
            my $err_text = $stderr || '(unknown error)';
            chomp $err_text;
            my @lines = split /\n/, $err_text;
            my $short_err = join("\n", @lines[0..19]);
            push @errors, { run => 1, elapsed_ms => 0, error => $short_err };
        }
    }

    my $total_ok = scalar @all_times_ms;
    my $total_err = scalar @errors;
    my $warmup_ms = shift @all_times_ms;
    my $avg_ms = @all_times_ms ? (int((sum(@all_times_ms) / @all_times_ms) + 0.5)) : 0;
    my $min_ms = @all_times_ms ? ((sort { $a <=> $b } @all_times_ms)[0]) : 0;
    my $max_ms = @all_times_ms ? ((sort { $a <=> $b } @all_times_ms)[-1]) : 0;

    return {
        runs_ms      => \@all_times_ms,
        avg_ms       => $avg_ms,
        min_ms       => $min_ms,
        max_ms       => $max_ms,
        warmup_ms    => $warmup_ms,
        total_ok     => $total_ok,
        total_err    => $total_err,
        errors       => \@errors,
    };
}

sub sum {
    my $s = 0;
    $s += $_ for @_;
    return $s;
}

# ============ 加载阈值配置 ============
#
# 从 perf/thresholds.json 读取阈值
# 返回: { 'int4_scan' => { threshold_ms => 8000, desc => '...' }, ... }

sub load_thresholds {
    my ($module_dir) = @_;
    my $file = "$module_dir/perf/thresholds.json";
    return {} unless -f $file;

    my $json = _slurp($file);
    my $data = decode_json($json);
    return $data;
}

# ============ 阈值校验 ============
#
# 对比实际耗时与阈值，返回是否通过

sub check_threshold {
    my ($result, $threshold_info) = @_;

    # 有错误则直接标记为失败
    return 0 if ($result->{total_err} // 0) > 0;

    return 1 unless $threshold_info;  # 无阈值定义，默认通过

    my $threshold = $threshold_info->{threshold_ms} // 0;
    return 1 if $threshold <= 0;

    return $result->{avg_ms} <= $threshold;
}

# ============ TAP输出 ============

sub print_perf_result {
    my ($test_name, $result, $threshold_info) = @_;

    my $threshold = $threshold_info ? ($threshold_info->{threshold_ms} // 0) : 0;
    my $desc = $threshold_info ? ($threshold_info->{desc} // '') : '';
    my $tc = $threshold_info ? ($threshold_info->{tc} // '') : '';

    my $passed = check_threshold($result, $threshold_info);
    my $status = $passed ? 'PASS' : 'FAIL';

    my $runs_str = join(', ', @{$result->{runs_ms}});
    my $dev_pct = 0;
    if ($threshold > 0 && $result->{avg_ms} > 0) {
        $dev_pct = ($result->{avg_ms} - $threshold) / $threshold * 100;
    }

    my $tap = _tap();

    # 有错误时的特殊输出
    if (($result->{total_err} // 0) > 0) {
        my $status_line = sprintf("%-20s ERRORS=%d/%d runs %s",
            $test_name, ($result->{total_err} // 0),
            ($result->{total_err} // 0) + ($result->{total_ok} // 0), $status);
        print $tap "not ok - $status_line\n";
        print $tap "#   $desc\n" if $desc;
        print $tap "#   TC: $tc\n" if $tc;
        for my $e (@{$result->{errors}}) {
            my @err_lines = split /\n/, $e->{error};
            my $first = shift @err_lines;
            print $tap "#   [run $e->{run}] ${e}ms: $first\n";
            for my $l (@err_lines) {
                print $tap "#     $l\n";
            }
        }
        return;
    }

    my $status_line = sprintf("%-20s avg=%dms threshold=%dms dev=%+.1f%% %s",
        $test_name, $result->{avg_ms}, $threshold, $dev_pct, $status);
    if ($passed) {
        print $tap "ok - $status_line\n";
    } else {
        print $tap "not ok - $status_line\n";
    }

    # 详细时序（注释行，TAP解析器跳过）
    print $tap "#   $desc\n" if $desc;
    print $tap "#   TC: $tc\n" if $tc;
    print $tap "#   runs: [$runs_str] ms (warmup discarded)\n";
    print $tap "#   min=$result->{min_ms}ms  max=$result->{max_ms}ms\n";
}

# ============ 保存JSON结果 ============

sub save_perf_result {
    my ($module_dir, $test_name, $result, $threshold_info, $driver) = @_;

    my $results_dir = "$module_dir/perf/results";
    mkdir $results_dir unless -d $results_dir;

    my $json = encode_json({
        test         => $test_name,
        driver       => $driver,
        threshold_ms => $threshold_info ? ($threshold_info->{threshold_ms} // 0) : 0,
        desc         => $threshold_info ? ($threshold_info->{desc} // '') : '',
        runs_ms      => $result->{runs_ms},
        avg_ms       => $result->{avg_ms},
        min_ms       => $result->{min_ms},
        max_ms       => $result->{max_ms},
        warmup_ms    => $result->{warmup_ms} // -1,
        total_ok     => $result->{total_ok} // 0,
        total_err    => $result->{total_err} // 0,
        errors       => $result->{errors},
        pass         => check_threshold($result, $threshold_info),
    });

    my $outfile = "$results_dir/${test_name}.json";
    open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
    if ($fh) {
        print $fh $json;
        close $fh;
    }
}

# ============ 发现SQL文件 ============

sub discover_perf_sql_files {
    my ($sql_dir) = @_;
    my $filter_str = get_tecc_test_filter();
    my @filters = $filter_str ? split(/,/, $filter_str) : ();

    opendir(my $dh, $sql_dir) or return ();
    my @files = sort grep {
        my $f = $_;
        $f =~ /\.sql$/ && $f ne 'perf_ddl.sql' && $f ne 'perf_multiply.sql'
        && (!$filter_str || grep { $f eq $_ } @filters)
    } readdir($dh);
    closedir($dh);

    return @files;
}

sub get_tecc_test_filter {
    my $filter = $ENV{'TECC_TEST'} || '';
    return $filter;
}

# ============ 打印汇总 ============

sub print_perf_summary {
    my ($results) = @_;  # [ { name => 'int4_scan', passed => 1, ... }, ... ]

    my $tap = _tap();
    my $total = scalar @$results;
    my $passed = scalar grep { $_->{passed} } @$results;
    my $failed = $total - $passed;
    my $errors = scalar grep { $_->{errors} && $_->{errors} > 0 } @$results;

    print $tap sprintf("\n# %s\n", '=' x 42);
    print $tap sprintf("# 性能测试汇总\n");
    print $tap sprintf("# %s\n", '=' x 42);
    print $tap sprintf("# 通过：%d / %d  (%.1f%%)\n", $passed, $total,
        $total > 0 ? ($passed * 100 / $total) : 0);
    print $tap sprintf("# 失败：%d / %d  (%.1f%%)\n", $failed, $total,
        $total > 0 ? ($failed * 100 / $total) : 0);
    print $tap sprintf("# ERRORS: %d tests had SQL errors\n", $errors) if $errors > 0;
    print STDERR sprintf("ERRORS: %d tests had SQL errors\n", $errors) if $errors > 0;

    my @failed_details = grep { !$_->{passed} } @$results;
    if (@failed_details) {
        print $tap sprintf("# %s\n", '=' x 42);
        print $tap sprintf("# 失败详情：\n");
        for my $r (@failed_details) {
            if ($r->{errors} && $r->{errors} > 0) {
                print $tap sprintf("#   %-20s SQL 错误 (%d 个)\n",
                    $r->{name}, $r->{errors});
            } else {
                my $dev = $r->{threshold} > 0
                    ? sprintf("%+.1f%%", ($r->{avg} - $r->{threshold}) / $r->{threshold} * 100)
                    : 'N/A';
                print $tap sprintf("#   %-20s 平均=%dms > 阈值=%dms (%s)\n",
                    $r->{name}, $r->{avg}, $r->{threshold}, $dev);
            }
        }
    }
    print $tap sprintf("# %s\n", '=' x 42);
}

sub ensure_csv_data {
    my $dir = '/tmp';
    my @csvs = ('perf_int.csv', 'perf_decimal.csv', 'perf_text.csv');
    for my $csv (@csvs) {
        return if -f "$dir/$csv";
    }
    # CSV文件缺失，自动生成
    my $tap = _tap();
    print $tap sprintf("# perf_data: generating CSV test data (1M rows each)...\n");

    # 写入CSV - 批量写入提高性能
    open(my $fh_int, '>', "$dir/perf_int.csv") or die "Cannot write perf_int.csv: $!";
    open(my $fh_dec, '>', "$dir/perf_decimal.csv") or die "Cannot write perf_decimal.csv: $!";
    open(my $fh_txt, '>', "$dir/perf_text.csv") or die "Cannot write perf_text.csv: $!";
    for my $i (1 .. 1000000) {
        print $fh_int "$i\n";
        printf $fh_dec "%.12f\n", $i * 0.000001;
        print $fh_txt "这是第 $i 条测试记录\n";
    }
    close $fh_int; close $fh_dec; close $fh_txt;
    print $tap sprintf("# perf_data: CSV generation complete (3 files, 1M rows each)\n");
}

# ============ 密文数据初始化（种子+倍增模式）============
#
# 策略：客户端加密少量种子行（SEED=1000），然后服务端 INSERT...SELECT
# 倍增密文到目标 PERF_ROWS。密文是普通字节，服务端复制不触发客户端加密。
#
# 参数: 通过环境变量控制 PERF_ROWS（默认 1000000）/ PERF_SEED（种子上限，默认 100000）

sub init_encrypted_data {
    my ($node, $module_dir) = @_;

    my $connstr = $node->connstr('postgres');
    my $tap = _tap();
    my $psql = $node->installed_command('psql');
    my $target_rows = $ENV{PERF_ROWS} // 1000000;
    my $seed_max = $ENV{PERF_SEED} // 100000;
    my $seed = $target_rows < $seed_max ? $target_rows : $seed_max;

    # ===== Phase 1: 客户端加密种子数据（3表并行）=====
    my $t_start = gettimeofday();
    my $msg = sprintf("# perf_data: seeding %d rows via client encryption (target=%d, parallel)\n",
        $seed, $target_rows);
    print $tap $msg;
    print STDERR $msg;
    my $t0 = gettimeofday();

    my $batch_size = 2000;
    my %stage_tables = (
        int4   => { table => 'stage_int4',   cols => 'val_det, val_ope', encrypts => 2 },
        float8 => { table => 'stage_float8', cols => 'val_det, val_ope', encrypts => 2 },
        text   => { table => 'stage_text',   cols => 'val_det, val_ope, val_search', encrypts => 3 },
    );

    my %sql_files;
    my %pids;
    my $total_encrypts = 0;

    # 为每张表生成独立的 VALUES SQL 文件
    for my $t (keys %stage_tables) {
        my $tbl = $stage_tables{$t}{table};
        my $sqlfile = "/tmp/tecc_perf_seed_${t}_$$.sql";
        $sql_files{$t} = $sqlfile;
        $total_encrypts += $seed * $stage_tables{$t}{encrypts};

        open(my $fh, '>', $sqlfile) or die "Cannot write $sqlfile: $!";
        if ($t eq 'int4') {
            for (my $start = 1; $start <= $seed; $start += $batch_size) {
                my $end = $start + $batch_size - 1;
                $end = $seed if $end > $seed;
                my @vals;
                for my $i ($start .. $end) { push @vals, "($i,$i)"; }
                print $fh "INSERT INTO $tbl (val_det, val_ope) VALUES "
                    . join(", ", @vals) . ";\n";
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
                print $fh "INSERT INTO $tbl (val_det, val_ope) VALUES "
                    . join(", ", @vals) . ";\n";
            }
        } elsif ($t eq 'text') {
            for (my $start = 1; $start <= $seed; $start += $batch_size) {
                my $end = $start + $batch_size - 1;
                $end = $seed if $end > $seed;
                my @vals;
                for my $i ($start .. $end) {
                    push @vals, "('第${i}条测试记录','第${i}条测试记录','第${i}条测试记录')";
                }
                print $fh "INSERT INTO $tbl (val_det, val_ope, val_search) VALUES "
                    . join(", ", @vals) . ";\n";
            }
        }
        close $fh;
    }

    # fork 3 个 psql 并行执行
    for my $t (keys %sql_files) {
        my $pid = fork();
        if (!defined $pid) {
            die "fork failed: $!";
        }
        if ($pid == 0) {
            # 子进程
            exec($psql, '-X', '-At', '-d', $connstr, '-f', $sql_files{$t});
            exit(1);  # exec 失败才走到这里
        }
        $pids{$t} = $pid;
    }

    # 等待所有子进程完成
    my $seed_failed = 0;
    for my $t (keys %pids) {
        waitpid($pids{$t}, 0);
        my $exit_code = $? >> 8;
        if ($exit_code != 0) {
            print $tap "# WARNING: seed for $t failed (exit=$exit_code)\n";
            print STDERR "WARNING: seed for $t failed (exit=$exit_code)\n";
            $seed_failed = 1;
        }
        unlink $sql_files{$t};
    }

    if ($seed_failed) {
        return;
    }
    my $t1 = gettimeofday();
    my $seed_elapsed = $t1 - $t0;
    print $tap sprintf("# perf_data: seeded %d rows into stage tables (elapsed=%.1fs, encryptions=%d, parallel)\n",
        $seed, $seed_elapsed, $total_encrypts);

    # ===== Phase 2: 从 stage 线性复制到 target（psql -C off，纯服务端字节拷贝）=====
    # 读取共享的 perf_multiply.sql 作为单轮倍增模板
    my $mult_sql_file = "$module_dir/perf/sql/perf_multiply.sql";
    open(my $mh_tpl, '<', $mult_sql_file) or die "Cannot read $mult_sql_file: $!";
    local $/;
    my $mult_one_iter = <$mh_tpl>;
    close $mh_tpl;
    # 去掉注释行，只保留 SQL 语句
    $mult_one_iter =~ s/--[^\n]*\n//g;
    $mult_one_iter =~ s/^\s+|\s+$//g;

    if ($target_rows <= $seed) {
        # 目标行数不超过种子数，从 stage 直接复制（加 LIMIT）
        my $mult_sql = $mult_one_iter =~ s/;/ LIMIT $target_rows;/gr;
        my $mult_tmpfile = "/tmp/tecc_perf_mult_$$.sql";
        open(my $mh, '>', $mult_tmpfile) or die "Cannot write $mult_tmpfile: $!";
        print $mh "$mult_sql\n";
        close $mh;
        my ($mout, $merr);
        my $mok = run([$psql, '-k', 'off', '-X', '-At', '-d', $connstr, '-f', $mult_tmpfile],
            '>', \$mout, '2>', \$merr);
        unlink $mult_tmpfile;
        my $total_elapsed = gettimeofday() - $t_start;
        print $tap sprintf("# perf_data: copy from stage, total init=%.1fs\n", $total_elapsed);
        return;
    }

    my $t_mult_start = gettimeofday();

    my $full_iters = int($target_rows / $seed);
    my $remainder = $target_rows % $seed;

    my $mult_msg = sprintf("# perf_data: copying %d rows/iter from stage to target, %d iters",
        $seed, $full_iters);
    $mult_msg .= sprintf(" + %d remainder", $remainder) if $remainder > 0;
    $mult_msg .= " (psql -C off)\n";
    print $tap $mult_msg;
    print STDERR $mult_msg;

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
    open(my $mh, '>', $mult_tmpfile) or die "Cannot write $mult_tmpfile: $!";
    print $mh $mult_sql;
    close $mh;

    my ($mout, $merr);
    my $mok = run([$psql, '-k', 'off', '-X', '-At', '-d', $connstr, '-f', $mult_tmpfile],
        '>', \$mout, '2>', \$merr);
    unlink $mult_tmpfile;

    if (!$mok) {
        my $e = $merr || '(unknown)';
        chomp $e;
        (my $s = $e) =~ s/\n.*//s;
        print $tap "# WARNING: multiplication had errors: $s\n";
        print STDERR "WARNING: multiplication had errors: $s\n";
    } else {
        # 验证最终行数
        my ($cnt_out, $cnt_err);
        run([$psql, '-X', '-At', '-d', $connstr, '-c',
            "SELECT 'enc_int4='||count(*)||' enc_float8='||(SELECT count(*) FROM enc_float8)"
            . "||' enc_text='||(SELECT count(*) FROM enc_text) FROM enc_int4"],
            '>', \$cnt_out, '2>', \$cnt_err);
        chomp $cnt_out if $cnt_out;
        my $t_now = gettimeofday();
        my $total_elapsed = $t_now - $t_start;
        my $mult_elapsed = $t_now - $t_mult_start;
        print $tap "# perf_data: final row counts: $cnt_out\n";
        print $tap sprintf("# perf_data: seed=%.1fs, multiply=%.1fs, total_init=%.1fs\n",
            $seed_elapsed, $mult_elapsed, $total_elapsed);
    }
}

# ============ 格式化表格输出 ============

sub print_perf_table {
    my ($results, $opts) = @_;
    $opts //= {};
    my $tap = _tap();

    my $total = scalar @$results;
    return unless $total;

    my $passed = scalar grep { $_->{passed} } @$results;
    my $failed = $total - $passed;

    my $name_w = 22;
    print $tap "\n";
    print $tap "+" . ("-" x $name_w) . "+--------+----------+------+--------+\n";
    print $tap "| " . sprintf("%-${name_w}s", "测试名称") . " | 平均(ms) | 阈值(ms) | 结果 |  阈值占比  |\n";
    print $tap "+" . ("-" x $name_w) . "+--------+----------+------+--------+\n";

    for my $r (sort { $a->{name} cmp $b->{name} } @$results) {
        my $name = $r->{name};
        my $avg = $r->{avg} // 0;
        my $th = $r->{threshold} // 0;
        my $ok = $r->{passed} ? "\033[32m通过\033[0m" : "\033[31m失败\033[0m";
        my $pct = $th > 0 ? sprintf("%.0f%%", $avg / $th * 100) : "N/A";

        print $tap sprintf("| %-${name_w}s | %6d | %8d | %-4s | %13s |\n",
            $name, $avg, $th, $ok, $pct);
    }
    print $tap "+" . ("-" x $name_w) . "+--------+----------+------+--------+\n";
    print $tap sprintf("通过：%d/%d  失败：%d/%d\n", $passed, $total, $failed, $total);
}

# ============ 保存完整汇总文件 ============

sub save_perf_summary {
    my ($module_dir, $results, $opts) = @_;
    $opts //= {};

    my $results_dir = "$module_dir/perf/results";
    mkdir $results_dir unless -d $results_dir;

    my $now = scalar localtime;
    my $cmdline = join(' ', $0, @ARGV);
    my $perf_rows = $ENV{PERF_ROWS} // 1000000;
    my $perf_runs = $ENV{PERF_RUNS} // 5;
    my $perf_seed = $ENV{PERF_SEED} // 100000;
    my $filter = $ENV{TECC_TEST} // '(all)';
    my $driver = $opts->{driver} // 'libpq';

    my ($sec, $min, $hour, $mday, $mon, $year) = localtime;
    my $ts = sprintf("%04d%02d%02d_%02d%02d%02d", $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
    my $summary_file = "$results_dir/SUMMARY_${ts}.txt";

    open(my $fh, '>', $summary_file) or do {
        my $tap = _tap();
        print $tap "# WARNING: cannot write summary file: $summary_file\n";
        print STDERR "WARNING: cannot write summary file: $summary_file\n";
        return undef;
    };

    # ---- Header ----
    print $fh "======================================================================\n";
    print $fh "  TECC 性能测试报告\n";
    print $fh "======================================================================\n";
    print $fh "时间：        $now\n";
    print $fh "驱动：        $driver\n";
    print $fh "命令：        $cmdline\n";
    print $fh "环境：        PERF_ROWS=$perf_rows PERF_SEED=$perf_seed PERF_RUNS=$perf_runs TECC_TEST=$filter\n";
    print $fh "说明：阈值按100万行数据设定，未经同意不得修改\n";
    print $fh "\n";

    # ---- Results Table ----
    my $total = scalar @$results;
    if ($total) {
        my $passed = scalar grep { $_->{passed} } @$results;
        my $failed = $total - $passed;

        my $name_w = 22;
        my $hdr = sprintf("%-${name_w}s | 平均(ms) | 阈值(ms) | 结果 | 阈值占比", "测试名称");
        print $fh "$hdr\n";
        print $fh "-" x length($hdr) . "\n";

        for my $r (sort { $a->{name} cmp $b->{name} } @$results) {
            my $name = $r->{name};
            my $avg = $r->{avg} // 0;
            my $th = $r->{threshold} // 0;
            my $ok = $r->{passed} ? "通过" : "失败";
            my $pct = $th > 0 ? sprintf("%.0f%%", $avg / $th * 100) : "N/A";

            printf $fh "%-${name_w}s | %7d | %9d | %-6s | %s\n",
                $name, $avg, $th, $ok, $pct;
        }
        print $fh "\n汇总：通过=$passed  失败=$failed  总计=$total\n";
    }
    print $fh "\n";

    # ---- Per-test Details (from JSON files) ----
    print $fh "======================================================================\n";
    print $fh "  详细结果\n";
    print $fh "======================================================================\n";

    opendir(my $dh, $results_dir) or return $summary_file;
    my @json_files = sort grep { /\.json$/ && -f "$results_dir/$_" } readdir($dh);
    closedir($dh);

    for my $jf (@json_files) {
        my $name = $jf;
        $name =~ s/\.json$//;
        my $json_text = _slurp("$results_dir/$jf");
        my $d;
        eval { $d = decode_json($json_text); };
        next unless $d;

        print $fh "\n--- $name ---\n";
        print $fh "  描述：        " . ($d->{desc} || '') . "\n";
        print $fh "  驱动：        " . ($d->{driver} || '') . "\n";
        printf $fh "  平均 (ms):    %d\n", ($d->{avg_ms} // 0);
        printf $fh "  最小 (ms):    %d\n", ($d->{min_ms} // 0);
        printf $fh "  最大 (ms):    %d\n", ($d->{max_ms} // 0);
        printf $fh "  预热 (ms):    %d\n", ($d->{warmup_ms} // 0);
        printf $fh "  阈值 (ms):    %d\n", ($d->{threshold_ms} // 0);
        print $fh "  时序 (ms):    [" . join(", ", @{$d->{runs_ms}}) . "]\n";
        printf $fh "  成功/失败:    %d/%d\n", ($d->{total_ok} // 0), ($d->{total_err} // 0);
        print $fh "  结果：        " . ($d->{pass} ? "通过" : "失败") . "\n";
        if ($d->{errors} && @{$d->{errors}}) {
            print $fh "  Errors:\n";
            for my $e (@{$d->{errors}}) {
                print $fh "    [run $e->{run}] $e->{error}\n";
            }
        }
    }

    close $fh;

    my $tap = _tap();
    print $tap "# \n";
    print $tap "# 汇总报告：${summary_file}\n";

    return $summary_file;
}

1;
