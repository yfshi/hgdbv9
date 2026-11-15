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
use Encode qw(encode_utf8);
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
    threshold_met
    set_perf_record_only
    perf_record_only_env
    print_perf_result
    print_perf_summary
    print_perf_table
    save_perf_result
    save_perf_summary
    save_perf_resource_report
    discover_perf_sql_files
    get_tecc_test_filter
    init_encrypted_data
    build_insert_result
    perf_crypto_backend
);

# 获取TAP输出句柄
my $TAP;
sub _tap { $TAP //= Test::More->builder->output; }

# ============ 记录模式（阈值不判红）============
#
# ReqID:TECC4.0 - 硬加密（SoftSDF / 真机）轮次的耗时是设备带宽决定的，thresholds.json
# 里的候选阈值按 localkms 标定，整片判红没有信息量。故硬加密轮次默认**只记录不判红**：
# 仍然逐用例打印平均耗时与候选阈值偏差、仍写 JSON，但不产生 not ok。
#
# 由入口脚本决定后调用 set_perf_record_only(1)（环境变量语义见 t/perf/004_*.pl 头注释）。
#
# 注意：SQL 错误（total_err>0）**不受记录模式影响**，一律判红 —— 那说明用例根本没跑成，
# 属于功能/环境问题，不是性能阈值判断。
our $perf_record_only = 0;

sub set_perf_record_only {
    my ($flag) = @_;

    $perf_record_only = $flag ? 1 : 0;
    return $perf_record_only;
}

# 记录模式的环境变量口径（两个入口共用）：
#   TECC_PERF_RECORD_ONLY=1 → 强制只记录；TECC_PERF_ENFORCE=1 → 强制判红。
# 都没设返回 undef —— 默认值由入口按自身模式决定（004 硬加密默认只记录；
# 005 保持既有判红口径，需要时用 TECC_PERF_RECORD_ONLY=1 打开）。
sub perf_record_only_env {
    my $force = (($ENV{TECC_PERF_RECORD_ONLY} // '') =~ /^(1|true|yes|on)$/i) ? 1 : 0;
    my $keep  = (($ENV{TECC_PERF_ENFORCE} // '') =~ /^(1|true|yes|on)$/i) ? 1 : 0;

    return undef unless $force || $keep;
    return $force ? 1 : 0;
}

# 单用例判定（TAP/表格口径）：
#   SQL 错误 → fail；记录模式 → 按原始阈值比较标 达标/未达标；否则 → 按通过/失败。
sub _perf_verdict {
    my ($r) = @_;

    my $th = $r->{threshold} // 0;
    my $avg = $r->{avg} // 0;

    return 'fail' if ($r->{errors} // 0) > 0;
    return $perf_record_only
        ? (($th > 0 && $avg > $th) ? 'fail' : 'pass')
        : ($r->{passed} ? 'pass' : 'fail');
}

# ============ 性能报告：加密后端标识 ============
#
# ReqID:TECC4.0 - 汇总报告必须能区分「本地软加密 / SoftSDF 软件密码机 / 真机 HSM」，
# 否则同一 SUMMARY 文件名无法判断测的是 localkms 还是硬加密路径。
#
# 判定（与功能回归 TECC_SDF_LIB 开关一致；005 真机另用 TECC_PERF_HSM 打开）：
#   - 设了 TECC_SDF_LIB           → SoftSDF（软件 SDF，硬加密路径）
#   - 未设且 TECC_PERF_HSM=1      → HSM:swxa（真实加密机；入口也可 crypto => 'HSM:...' 覆盖）
#   - 其余                        → localkms（本地软加密）
sub perf_crypto_backend {
    my ($override) = @_;
    return $override if defined $override && $override ne '';

    if (my $sdf = $ENV{TECC_SDF_LIB}) {
        my $base = $sdf;
        $base =~ s{.*/}{};
        return "SoftSDF ($base)";
    }
    if (($ENV{TECC_PERF_HSM} // '') =~ /^(1|true|yes|hsm|swxa)$/i) {
        return 'HSM:swxa (真实加密机)';
    }
    return 'localkms (本地软加密)';
}

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
    my $tecc_client_mode = $ENV{TECC_PERF_CLIENT_MODE} // 'on';
    my $resource_file;
    my $resource_info;

    # 多个-c选项在同一连接中顺序执行，psql的\timing输出每条的耗时
    # 重要: 用单引号避免Perl将\t解释为制表符
    my @cmd = ($psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-c', '\timing on');
    push @cmd, ('-c', $sql) for (1 .. $runs);

    $resource_info = _prepare_client_resource_wrapper(\@cmd);
    $resource_file = $resource_info->{file} if $resource_info->{enabled};

    my ($stdout, $stderr);
    my $t0 = [gettimeofday()];
    my $ok = run(\@cmd, '>', \$stdout, '2>', \$stderr);
    my $elapsed_ms = int(tv_interval($t0) * 1000 + 0.5);

    # 从 stdout 和 stderr 解析 Time: 行（psql 可能将 \timing 输出到任意流）
    my @all_times_ms;
    for my $line (split /\n/, $stdout) {
        my $ms = _parse_psql_timing_line($line);
        push @all_times_ms, $ms if defined $ms;
    }
    for my $line (split /\n/, ($stderr // '')) {
        my $ms = _parse_psql_timing_line($line);
        push @all_times_ms, $ms if defined $ms;
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

    if (!$has_error && scalar(@all_times_ms) == 0 && $runs > 0) {
        my $fallback_ms = int($elapsed_ms / $runs + 0.5);
        @all_times_ms = (($fallback_ms) x $runs);
    }

    my $total_ok = scalar @all_times_ms;
    my $total_err = scalar @errors;
    my $warmup_ms = shift @all_times_ms;
    my $avg_ms = @all_times_ms ? (int((sum(@all_times_ms) / @all_times_ms) + 0.5)) : 0;
    my $min_ms = @all_times_ms ? ((sort { $a <=> $b } @all_times_ms)[0]) : 0;
    my $max_ms = @all_times_ms ? ((sort { $a <=> $b } @all_times_ms)[-1]) : 0;
    my $resource = _finish_client_resource_sample($resource_info, $resource_file);

    return {
        runs_ms      => \@all_times_ms,
        avg_ms       => $avg_ms,
        min_ms       => $min_ms,
        max_ms       => $max_ms,
        warmup_ms    => $warmup_ms,
        total_ok     => $total_ok,
        total_err    => $total_err,
        errors       => \@errors,
        resource      => $resource,
    };
}

sub sum {
    my $s = 0;
    $s += $_ for @_;
    return $s;
}

# ============ 客户端资源采集 ============
#
# 使用 GNU time 包裹 psql 进程，采集客户端 CPU、最大 RSS、缺页和上下文切换。
# 采集范围包含同一个 psql 进程中的预热和正式执行次数，便于观察 libpq 加解密整体开销。

sub _parse_psql_timing_line {
    my ($line) = @_;

    return undef unless defined $line;
    if ($line =~ /(?:Time|时间)\s*[:：]\s*([\d.]+)\s*ms/i) {
        return int($1 + 0.5);
    }
    return undef;
}

sub _client_resource_enabled {
    my $v = $ENV{PERF_CLIENT_RESOURCE};
    return 1 unless defined $v;
    return 0 if $v eq '0' || lc($v) eq 'false' || lc($v) eq 'off';
    return 1;
}

sub _find_gnu_time {
    my $out;
    my $err;
    for my $cmd ('/usr/bin/time', '/bin/time') {
        next unless -x $cmd;
        if (run([$cmd, '-v', '-o', '/dev/null', 'true'], '>', \$out, '2>', \$err)) {
            return $cmd;
        }
    }
    return undef;
}

sub _prepare_client_resource_wrapper {
    my ($cmdref) = @_;
    my $time_cmd;
    my $file;

    return { enabled => 0, available => 0, reason => 'PERF_CLIENT_RESOURCE=0' }
        unless _client_resource_enabled();

    $time_cmd = _find_gnu_time();
    return { enabled => 0, available => 0, reason => '未找到支持 -v -o 的 GNU time' }
        unless $time_cmd;

    $file = "/tmp/tecc_perf_resource_$$.txt";
    unshift @$cmdref, ($time_cmd, '-v', '-o', $file);

    return {
        enabled => 1,
        available => 0,
        file => $file,
        collector => $time_cmd,
    };
}

sub _finish_client_resource_sample {
    my ($resource_info, $resource_file) = @_;
    my $text;

    return $resource_info unless $resource_info && $resource_info->{enabled};
    return { %$resource_info, available => 0, reason => '资源采集文件缺失' }
        unless $resource_file && -f $resource_file;

    $text = _slurp($resource_file);
    unlink $resource_file;

    return _parse_gnu_time_verbose($text, $resource_info);
}

sub _parse_hms_ms {
    my ($value) = @_;
    my @parts;
    my $seconds;

    return undef unless defined $value;

    @parts = split /:/, $value;
    $seconds = 0;

    if (@parts == 3) {
        $seconds = $parts[0] * 3600 + $parts[1] * 60 + $parts[2];
    } elsif (@parts == 2) {
        $seconds = $parts[0] * 60 + $parts[1];
    } else {
        $seconds = $parts[0];
    }

    return int($seconds * 1000 + 0.5);
}

sub _parse_gnu_time_verbose {
    my ($text, $base) = @_;
    my %r = %$base;

    $r{available} = 1;
    for my $line (split /\n/, $text) {
        if ($line =~ /User time \(seconds\):\s*([\d.]+)/) {
            $r{user_cpu_sec} = 0 + $1;
        } elsif ($line =~ /System time \(seconds\):\s*([\d.]+)/) {
            $r{sys_cpu_sec} = 0 + $1;
        } elsif ($line =~ /Percent of CPU this job got:\s*([\d.]+)%/) {
            $r{cpu_percent} = 0 + $1;
        } elsif ($line =~ /Elapsed \(wall clock\) time.*:\s*(\S+)/) {
            $r{elapsed_ms} = _parse_hms_ms($1);
        } elsif ($line =~ /Maximum resident set size \(kbytes\):\s*(\d+)/) {
            $r{max_rss_kb} = 0 + $1;
        } elsif ($line =~ /Major \(requiring I\/O\) page faults:\s*(\d+)/) {
            $r{major_faults} = 0 + $1;
        } elsif ($line =~ /Minor \(reclaiming a frame\) page faults:\s*(\d+)/) {
            $r{minor_faults} = 0 + $1;
        } elsif ($line =~ /Voluntary context switches:\s*(\d+)/) {
            $r{voluntary_ctxt_switches} = 0 + $1;
        } elsif ($line =~ /Involuntary context switches:\s*(\d+)/) {
            $r{involuntary_ctxt_switches} = 0 + $1;
        } elsif ($line =~ /Exit status:\s*(\d+)/) {
            $r{exit_status} = 0 + $1;
        }
    }

    $r{cpu_total_sec} = ($r{user_cpu_sec} // 0) + ($r{sys_cpu_sec} // 0);
    return \%r;
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

    # 有错误则直接标记为失败（记录模式下也判红：用例没跑成，与阈值无关）
    return 0 if ($result->{total_err} // 0) > 0;

    # ReqID:TECC4.0 - 记录模式：阈值不判红，是否达标由 threshold_met 单独记录
    return 1 if $perf_record_only;

    return threshold_met($result, $threshold_info);
}

# 原始阈值比较（不看记录模式）：记录模式下仍写入 JSON 的 threshold_met 字段，
# 便于事后统计"硬加密到底有多少用例未达候选阈值"。
sub threshold_met {
    my ($result, $threshold_info) = @_;

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
    # thresholds.json 经 decode_json 得到的是字符串，而本文件无 use utf8、
    # 中文字面量是字节串。混拼会让结果带 UTF-8 标志，打印到无编码层的
    # TAP 句柄（SimpleTee）时逐条刷 "Wide character in print" 告警。
    # 统一转成 UTF-8 字节后再拼，内容不变、告警消失。
    my $desc = $threshold_info ? encode_utf8($threshold_info->{desc} // '') : '';
    my $tc = $threshold_info ? encode_utf8($threshold_info->{tc} // '') : '';

    my $passed = check_threshold($result, $threshold_info);
    # ReqID:TECC4.0 - 记录模式：阈值不判红，状态标 RECORD（SQL 错误仍标 FAIL）
    my $has_err = ($result->{total_err} // 0) > 0;
    my $status = $has_err ? 'FAIL'
        : $perf_record_only ? 'RECORD'
        : ($passed ? 'PASS' : 'FAIL');

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
    print $tap "#   阈值口径: 记录模式（不判红），候选阈值仅作对照；是否达标见 JSON 的 threshold_met\n"
        if $perf_record_only && !$has_err;
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
    if ($result->{resource} && $result->{resource}{available}) {
        my $r = $result->{resource};
        print $tap sprintf("#   客户端资源: CPU(user/sys)=%.2f/%.2fs CPU%%=%s RSS=%sKB\n",
            ($r->{user_cpu_sec} // 0), ($r->{sys_cpu_sec} // 0),
            defined $r->{cpu_percent} ? $r->{cpu_percent} : 'N/A',
            defined $r->{max_rss_kb} ? $r->{max_rss_kb} : 'N/A');
    } elsif ($result->{resource} && $result->{resource}{reason}) {
        print $tap "#   客户端资源: 未采集（$result->{resource}{reason}）\n";
    }
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
        req_threshold_ms => $threshold_info ? ($threshold_info->{requirement_threshold_ms} // 0) : 0,
        desc         => $threshold_info ? ($threshold_info->{desc} // '') : '',
        runs_ms      => $result->{runs_ms},
        avg_ms       => $result->{avg_ms},
        min_ms       => $result->{min_ms},
        max_ms       => $result->{max_ms},
        warmup_ms    => $result->{warmup_ms} // -1,
        total_ok     => $result->{total_ok} // 0,
        total_err    => $result->{total_err} // 0,
        errors       => $result->{errors},
        resource      => $result->{resource},
        pass         => check_threshold($result, $threshold_info),
        # ReqID:TECC4.0 - 记录模式下 pass 恒真（不判红），真实达标情况另存
        # threshold_met；record_only 标明本轮阈值是否参与判定。
        threshold_met => threshold_met($result, $threshold_info),
        record_only  => $perf_record_only ? 1 : 0,
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
    my $passed = scalar grep { _perf_verdict($_) eq 'pass' } @$results;
    my $failed = $total - $passed;
    my $errors = scalar grep { $_->{errors} && $_->{errors} > 0 } @$results;

    print $tap sprintf("\n# %s\n", '=' x 42);
    print $tap sprintf("# 性能测试汇总\n");
    print $tap sprintf("# %s\n", '=' x 42);
    if ($perf_record_only) {
        print $tap sprintf("# 阈值口径：记录模式（不判红）—— 候选阈值仅作对照，SQL 错误仍判红\n");
        print $tap sprintf("# 达标：%d / %d  (%.1f%%)\n", $passed, $total,
            $total > 0 ? ($passed * 100 / $total) : 0);
        print $tap sprintf("# 未达标：%d / %d  (%.1f%%)\n", $failed, $total,
            $total > 0 ? ($failed * 100 / $total) : 0);
    } else {
        print $tap sprintf("# 通过：%d / %d  (%.1f%%)\n", $passed, $total,
            $total > 0 ? ($passed * 100 / $total) : 0);
        print $tap sprintf("# 失败：%d / %d  (%.1f%%)\n", $failed, $total,
            $total > 0 ? ($failed * 100 / $total) : 0);
    }
    print $tap sprintf("# ERRORS: %d tests had SQL errors\n", $errors) if $errors > 0;
    print STDERR sprintf("ERRORS: %d tests had SQL errors\n", $errors) if $errors > 0;

    my @failed_details = grep { _perf_verdict($_) ne 'pass' } @$results;
    if (@failed_details) {
        print $tap sprintf("# %s\n", '=' x 42);
        print $tap sprintf("# %s：\n",
            $perf_record_only ? '未达标详情（仅记录，不判红）' : '失败详情');
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
    my $tecc_client_mode = $ENV{TECC_PERF_CLIENT_MODE} // 'on';
    my $target_rows = $ENV{PERF_ROWS} // 1000000;
    my $seed_max = $ENV{PERF_SEED} // 100000;
    my $seed = $target_rows < $seed_max ? $target_rows : $seed_max;

    # ===== Phase 1: 客户端加密种子数据（顺序/并行由 TECC_PERF_SEED_PARALLEL 控制）=====
    # 顺带测量各密态类型 INSERT 加密耗时（4 个代表类型：int4/float8/text/he_int8）
    # 每表按 run 拆分（第 1 run 预热，后 runs-1 run 测量取平均），详见 _run_table_seed
    my $t_start = gettimeofday();
    my $parallel = $ENV{TECC_PERF_SEED_PARALLEL} // 0;
    my $msg = sprintf("# perf_data: seeding %d rows via client encryption (target=%d, %s)\n",
        $seed, $target_rows, $parallel ? 'parallel' : 'serial');
    print $tap $msg;
    print STDERR $msg;
    my $t0 = gettimeofday();

    my $batch_size = $ENV{PERF_BATCH_SIZE} // 2000;
    $batch_size = 1 if $batch_size < 1;
    my $runs = $ENV{PERF_RUNS} // 5;
    $runs = 1 if $runs < 1;
    my %stage_tables = (
        int4   => { table => 'stage_int4',   cols => 'val_det, val_ope', encrypts => 2 },
        float8 => { table => 'stage_float8', cols => 'val_det, val_ope', encrypts => 2 },
        text   => { table => 'stage_text',   cols => 'val_det, val_ope, val_search', encrypts => 3 },
        he_int8 => { table => 'stage_he_int8', cols => 'val_add, val_mul', encrypts => 2 },
    );

    # 不再强制 seed 对齐到 batch_size*runs：按 run 尽量均分（前 remainder 个
    # run 多 1 行），批内末批可不足 batch_size。各 run 行数允许相差 1 行。
    die "perf_data: seed=$seed 必须 >= 1\n" if $seed < 1;
    my $seed_base = int($seed / $runs);
    my $seed_rem = $seed % $runs;

    my %run_files;
    my %run_batches;    # $t => [每 run 的批数]
    my $total_encrypts = 0;

    # 为每张表生成 runs 个 run SQL 文件（行跨 run 连续编号 1..seed）
    for my $t (keys %stage_tables) {
        my $tbl = $stage_tables{$t}{table};
        my $cols = $stage_tables{$t}{cols};
        $total_encrypts += $seed * $stage_tables{$t}{encrypts};
        my @files;
        my @batches_per_run;
        my $global_pos = 1;
        for my $run (1 .. $runs) {
            my $nrows = $seed_base + ($run <= $seed_rem ? 1 : 0);
            my $sqlfile = "/tmp/tecc_perf_seed_${t}_run${run}_$$.sql";
            open(my $fh, '>', $sqlfile) or die "Cannot write $sqlfile: $!";
            my $left = $nrows;
            my $nbatches = 0;
            while ($left > 0) {
                my $take = $left > $batch_size ? $batch_size : $left;
                my $start = $global_pos;
                my $end = $global_pos + $take - 1;
                my @vals;
                if ($t eq 'int4') {
                    for my $i ($start .. $end) { push @vals, "($i,$i)"; }
                    print $fh "INSERT INTO $tbl (val_det, val_ope) VALUES "
                        . join(", ", @vals) . ";\n";
                } elsif ($t eq 'float8') {
                    for my $i ($start .. $end) {
                        my $f = sprintf("%.6f", $i * 0.000001);
                        push @vals, "($f,$f)";
                    }
                    print $fh "INSERT INTO $tbl (val_det, val_ope) VALUES "
                        . join(", ", @vals) . ";\n";
                } elsif ($t eq 'text') {
                    # val_search 用 5 个 5 字符前缀轮换（各 20% 命中率），保证模糊查询
                    # 前缀匹配（LIKE 'P1%'）扫描 5 倍行数命中 1 行（命中率 20%）
                    my @like_prefix = ('第一数据组', '第二数据组', '第三数据组',
                                       '第四数据组', '第五数据组');
                    for my $i ($start .. $end) {
                        my $pfx = $like_prefix[($i - 1) % 5];
                        my $suffix = sprintf("%06d", $i);
                        push @vals, "('第${i}条测试记录','第${i}条测试记录','${pfx}${suffix}')";
                    }
                    print $fh "INSERT INTO $tbl (val_det, val_ope, val_search) VALUES "
                        . join(", ", @vals) . ";\n";
                } elsif ($t eq 'he_int8') {
                    # 同态表：val_add（he-add）+ val_mul（he-mul）
                    # val_add 用递增整型种子（供 he_add_10000/he_add_sum_10k）
                    # val_mul 固定为 1（供 he_mul_pow_10000：1^10000=1 不溢出 INT8）
                    # 若 val_mul 用大值，a^10000 mod p 超 INT8 范围会被 range check 拒绝
                    for my $i ($start .. $end) { push @vals, "($i,1)"; }
                    print $fh "INSERT INTO $tbl (val_add, val_mul) VALUES "
                        . join(", ", @vals) . ";\n";
                }
                $global_pos = $end + 1;
                $left -= $take;
                $nbatches++;
            }
            close $fh;
            push @files, $sqlfile;
            push @batches_per_run, $nbatches;
        }
        $run_files{$t} = \@files;
        $run_batches{$t} = \@batches_per_run;
    }

    # 执行种子 INSERT 并测量各表耗时（并行：表间 fork+exec bash；顺序：逐表执行）
    # 注意：fork 后子进程里 $$ 会变为子进程 PID（Perl 5.34 行为），
    # 故文件名用 fork 前保存的 $seed_token（父 PID），保证父子拼名一致。
    my %insert_results;
    my $seed_failed = 0;
    my $seed_token = "$$";
    if ($parallel) {
        # 并行：fork + exec bash（子进程不调用 IPC::Run，避免 fork 后 run() 管道问题）
        my %pids;
        for my $t (keys %run_files) {
            my $pid = fork();
            if (!defined $pid) {
                die "fork failed: $!";
            }
            if ($pid == 0) {
                my @cmd = ($psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-c', '\timing on');
                push @cmd, ('-f', $_) for @{$run_files{$t}};
                my $cmd = join(' ', map { "'$_'" } @cmd);
                my $outfile = "/tmp/tecc_perf_seed_out_${t}_${seed_token}.txt";
                exec('/bin/bash', '-c', "$cmd > '$outfile' 2>&1");
                exit(1);  # exec 失败才走到这里
            }
            $pids{$t} = $pid;
        }
        for my $t (keys %pids) {
            waitpid($pids{$t}, 0);
            my $exit_code = $? >> 8;
            if ($exit_code != 0) {
                print $tap "# WARNING: seed for $t failed (exit=$exit_code)\n";
                print STDERR "WARNING: seed for $t failed (exit=$exit_code)\n";
                $seed_failed = 1;
            }
        }
        # 解析各表输出：\timing 行按每 run 实际批数分组求和（各 run 批数可不同）
        for my $t (keys %run_files) {
            my $outfile = "/tmp/tecc_perf_seed_out_${t}_${seed_token}.txt";
            if (-f $outfile) {
                my $content = _slurp($outfile);
                $insert_results{$t} = _parse_timing_grouped($content, $runs,
                    $run_batches{$t} // []);
                _print_seed_error_excerpt("seed $t", $content)
                    if (($insert_results{$t}{total_err} // 0) > 0);
                unlink $outfile;
            }
        }
        # 清理 run 文件（子进程 exec 后不删除，父进程统一删）
        for my $t (keys %run_files) {
            unlink $_ for @{$run_files{$t}};
        }
    } else {
        # 顺序：逐表执行。每表一个 psql 进程顺序执行全部 run 文件（含预热），
        # 与并行模式测量口径一致（psql 启动 + TECC 缓存初始化每表仅一次）。
        for my $t (keys %run_files) {
            my @cmd = ($psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-c', '\timing on');
            push @cmd, ('-f', $_) for @{$run_files{$t}};
            my ($stdout, $stderr);
            run(\@cmd, '>', \$stdout, '2>', \$stderr);
            my $content = ($stdout // '') . ($stderr // '');
            $insert_results{$t} = _parse_timing_grouped($content, $runs,
                $run_batches{$t} // []);
            _print_seed_error_excerpt("seed $t", $content)
                if (($insert_results{$t}{total_err} // 0) > 0);
            unlink $_ for @{$run_files{$t}};
            if (($insert_results{$t}{total_err} // 0) > 0) {
                $seed_failed = 1;
            }
        }
    }

    if ($seed_failed) {
        die "perf_data: seed INSERT failed; see psql error excerpt above\n";
    }
    my $t1 = gettimeofday();
    my $seed_elapsed = $t1 - $t0;
    print $tap sprintf("# perf_data: seeded %d rows into stage tables (elapsed=%.1fs, encryptions=%d, %s)\n",
        $seed, $seed_elapsed, $total_encrypts, $parallel ? 'parallel' : 'serial');

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
        my $mok = run([$psql, '-k', 'off', '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-f', $mult_tmpfile],
            '>', \$mout, '2>', \$merr);
        unlink $mult_tmpfile;
        if (!$mok) {
            _print_seed_error_excerpt('copy from stage', ($mout // '') . ($merr // ''));
            die "perf_data: copy from stage failed; see psql error excerpt above\n";
        }
        my $total_elapsed = gettimeofday() - $t_start;
        print $tap sprintf("# perf_data: copy from stage, total init=%.1fs\n", $total_elapsed);
        return \%insert_results;
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
        $mult_sql .= "INSERT INTO enc_int4_b (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_int4 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_float8 (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_float8 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_text (val_det, val_ope, val_search) "
            . "SELECT val_det, val_ope, val_search FROM stage_text LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO he_int8 (val_add, val_mul) "
            . "SELECT val_add, val_mul FROM stage_he_int8 LIMIT $remainder;\n";
    }

    my $mult_tmpfile = "/tmp/tecc_perf_mult_$$.sql";
    open(my $mh, '>', $mult_tmpfile) or die "Cannot write $mult_tmpfile: $!";
    print $mh $mult_sql;
    close $mh;

    my ($mout, $merr);
    my $mok = run([$psql, '-k', 'off', '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-f', $mult_tmpfile],
        '>', \$mout, '2>', \$merr);
    unlink $mult_tmpfile;

    if (!$mok) {
        my $e = $merr || '(unknown)';
        chomp $e;
        (my $s = $e) =~ s/\n.*//s;
        print $tap "# WARNING: multiplication had errors: $s\n";
        print STDERR "WARNING: multiplication had errors: $s\n";
        _print_seed_error_excerpt('multiply from stage', ($mout // '') . ($merr // ''));
        die "perf_data: multiply from stage failed; see psql error excerpt above\n";
    } else {
        # 验证最终行数
        my ($cnt_out, $cnt_err);
        run([$psql, '-k', $tecc_client_mode, '-X', '-At', '-d', $connstr, '-c',
            "SELECT 'enc_int4='||count(*)||' enc_float8='||(SELECT count(*) FROM enc_float8)"
            . "||' enc_text='||(SELECT count(*) FROM enc_text)"
            . "||' enc_int4_b='||(SELECT count(*) FROM enc_int4_b) FROM enc_int4"],
            '>', \$cnt_out, '2>', \$cnt_err);
        chomp $cnt_out if $cnt_out;
        my $t_now = gettimeofday();
        my $total_elapsed = $t_now - $t_start;
        my $mult_elapsed = $t_now - $t_mult_start;
        print $tap "# perf_data: final row counts: $cnt_out\n";
        print $tap sprintf("# perf_data: seed=%.1fs, multiply=%.1fs, total_init=%.1fs\n",
            $seed_elapsed, $mult_elapsed, $total_elapsed);
    }

    # 返回各表种子 INSERT 耗时（供 perf 用例：int4_insert/float8_insert/text_insert/he_int8_insert）
    return \%insert_results;
}

# ============ 种子 INSERT 输出解析 ============
# 单个 psql 进程顺序执行 runs 个 run 文件，\timing 输出所有 Time 行；
# seed 不再强制对齐，各 run 的 INSERT 批数可以不同，按每 run 实际批数分组求和。
# $batches_per_run：数组ref（每 run 批数）；标量时各 run 同批数（兼容旧行为）。
# 返回与 psql_timing 相同结构的 result hash。

sub _parse_timing_grouped {
    my ($content, $runs, $batches_per_run) = @_;
    my @all_times;
    for my $line (split /\n/, $content) {
        my $ms = _parse_psql_timing_line($line);
        push @all_times, $ms if defined $ms;
    }
    my @run_batch_counts;
    if (ref $batches_per_run eq 'ARRAY') {
        @run_batch_counts = @$batches_per_run;
        push @run_batch_counts, (0) while @run_batch_counts < $runs;
    } else {
        my $n = $batches_per_run // 0;
        @run_batch_counts = ($n) x $runs;
    }
    my @run_times;
    my @errors;
    my $has_error = $content =~ /ERROR:|FATAL:/;
    my $cursor = 0;
    for my $i (0 .. $runs - 1) {
        my $sum = 0;
        my $nbatches = $run_batch_counts[$i] // 0;
        for my $j (0 .. $nbatches - 1) {
            last if $cursor >= @all_times;
            $sum += $all_times[$cursor];
            $cursor++;
        }
        push @run_times, $sum;
        if ($has_error) {
            push @errors, { run => $i + 1, elapsed_ms => 0, error => '(seed SQL error)' };
        }
    }
    return build_insert_result(\@run_times, \@errors);
}

# 由「每 run 耗时列表」构造 INSERT 用例结果（与 libpq 侧同口径：
# 第 1 run 计为预热并从统计中丢弃，avg/min/max 取其余 run）。
# 005（JDBC）侧把 runner 报告的每批 INSERT 耗时按 run 分组后同样走这里，
# 保证两种驱动的 int4_insert/float8_insert/text_insert/he_int8_insert 数字可比。
sub build_insert_result {
    my ($run_times, $errors) = @_;
    my @runs = @{$run_times // []};
    my @errors = @{$errors // []};
    my $warmup_ms = @runs ? shift @runs : -1;
    my $avg_ms = @runs ? int(sum(@runs) / @runs + 0.5) : 0;
    my $min_ms = @runs ? ((sort { $a <=> $b } @runs)[0]) : 0;
    my $max_ms = @runs ? ((sort { $a <=> $b } @runs)[-1]) : 0;
    return {
        runs_ms   => \@runs,
        avg_ms    => $avg_ms,
        min_ms    => $min_ms,
        max_ms    => $max_ms,
        warmup_ms => $warmup_ms,
        total_ok  => scalar @runs,
        total_err => scalar @errors,
        errors    => \@errors,
    };
}

sub _print_seed_error_excerpt {
    my ($label, $content) = @_;
    my $tap = _tap();
    my @lines;
    my $limit = 40;

    return unless defined $content && length $content;

    @lines = split /\n/, $content;
    print $tap "# perf_data: $label 失败输出摘要：\n";
    for my $i (0 .. $#lines) {
        last if $i >= $limit;
        print $tap "#   $lines[$i]\n";
    }
    print $tap "#   ...\n" if @lines > $limit;
}

# ============ 格式化表格输出 ============

sub print_perf_table {
    my ($results, $opts) = @_;
    $opts //= {};
    my $tap = _tap();

    my $total = scalar @$results;
    return unless $total;

    my $passed = scalar grep { _perf_verdict($_) eq 'pass' } @$results;
    my $failed = $total - $passed;

    my $name_w = 22;
    print $tap "\n";
    print $tap "+" . ("-" x $name_w) . "+--------+------------+------+------------+----------+\n";
    print $tap "| " . sprintf("%-${name_w}s", "测试名称") . " | 平均(ms) | 实际阈值 | 结果 | 需求阈值 | 需求达标 |\n";
    print $tap "+" . ("-" x $name_w) . "+--------+------------+------+------------+----------+\n";

    for my $r (sort { $a->{name} cmp $b->{name} } @$results) {
        my $name = $r->{name};
        my $avg = $r->{avg} // 0;
        my $th = $r->{threshold} // 0;
        my $req = $r->{req_threshold} // 0;
        my $verdict = _perf_verdict($r);
        my $ok = $verdict eq 'pass'
            ? "\033[32m" . ($perf_record_only ? '达标' : '通过') . "\033[0m"
            : "\033[31m" . ($perf_record_only ? (($r->{errors} // 0) > 0 ? '失败' : '未达标') : '失败') . "\033[0m";
        my $req_ok = ($req > 0 && $avg <= $req) ? "\033[32m达标\033[0m" : ($req > 0 ? "\033[31m未达标\033[0m" : "N/A");

        print $tap sprintf("| %-${name_w}s | %6d | %9d | %-4s | %9d | %-5s |\n",
            $name, $avg, $th, $ok, $req, $req_ok);
    }
    print $tap "+" . ("-" x $name_w) . "+--------+------------+------+------------+----------+\n";
    if ($perf_record_only) {
        print $tap "阈值口径：记录模式（不判红）——“未达标”仅为未达候选阈值，供对照\n";
        print $tap sprintf("达标：%d/%d  未达标：%d/%d\n", $passed, $total, $failed, $total);
    } else {
        print $tap sprintf("通过：%d/%d  失败：%d/%d\n", $passed, $total, $failed, $total);
    }
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
    my $crypto = perf_crypto_backend($opts->{crypto});
    my $key_store = $opts->{key_store};
    $key_store = 'swxakms' if !defined $key_store && $crypto =~ /^(SoftSDF|HSM)/;
    $key_store = 'localkms' if !defined $key_store;

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
    print $fh "加密后端：    $crypto\n";
    print $fh "KEK存储：     $key_store\n";
    print $fh "命令：        $cmdline\n";
    print $fh "环境：        PERF_ROWS=$perf_rows PERF_SEED=$perf_seed PERF_RUNS=$perf_runs TECC_TEST=$filter\n";
    print $fh "环境：        TECC_SDF_LIB=" . ($ENV{TECC_SDF_LIB} // '(unset)') . "\n";
    print $fh "环境：        TECC_PERF_HSM=" . ($ENV{TECC_PERF_HSM} // '(unset)') . "\n";
    # ReqID:TECC4.0 - 记录模式（硬加密轮次默认）下阈值不参与判定，报告头必须写明，
    # 否则"结果=失败"会被误读成本轮回归红了。
    print $fh "阈值口径：    " . ($perf_record_only
        ? '记录模式（不判红）：候选阈值仅作对照，SQL 错误仍判红'
        : '判红：平均耗时 > 候选阈值即失败') . "\n";
    print $fh "说明：阈值按100万行数据设定，未经同意不得修改\n";
    print $fh "\n";

    # ---- Results Table ----
    my $total = scalar @$results;
    if ($total) {
        my $passed = scalar grep { _perf_verdict($_) eq 'pass' } @$results;
        my $failed = $total - $passed;

        my $name_w = 22;
        my $hdr = sprintf("%-${name_w}s | 平均(ms) | 实际阈值 | 结果 | 需求阈值 | 需求达标", "测试名称");
        print $fh "$hdr\n";
        print $fh "-" x length($hdr) . "\n";

        for my $r (sort { $a->{name} cmp $b->{name} } @$results) {
            my $name = $r->{name};
            my $avg = $r->{avg} // 0;
            my $th = $r->{threshold} // 0;
            my $req = $r->{req_threshold} // 0;
            my $verdict = _perf_verdict($r);
            my $ok = $verdict eq 'pass'
                ? ($perf_record_only ? "达标" : "通过")
                : ($perf_record_only && ($r->{errors} // 0) == 0 ? "未达标" : "失败");
            my $req_ok = ($req > 0 && $avg <= $req) ? "达标" : ($req > 0 ? "未达标" : "N/A");

            printf $fh "%-${name_w}s | %7d | %9d | %-6s | %9d | %s\n",
                $name, $avg, $th, $ok, $req, $req_ok;
        }
        if ($perf_record_only) {
            print $fh "\n汇总（记录模式，不判红）：达标=$passed  未达标=$failed  总计=$total\n";
        } else {
            print $fh "\n汇总：通过=$passed  失败=$failed  总计=$total\n";
        }
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
        # 同 print_perf_result：decode_json 返回字符串，与本文件的字节字面量
        # "描述：" 相拼会二次编码成乱码，故先转 UTF-8 字节。
        print $fh "  描述：        " . encode_utf8($d->{desc} || '') . "\n";
        print $fh "  驱动：        " . encode_utf8($d->{driver} || '') . "\n";
        printf $fh "  平均 (ms):    %d\n", ($d->{avg_ms} // 0);
        printf $fh "  最小 (ms):    %d\n", ($d->{min_ms} // 0);
        printf $fh "  最大 (ms):    %d\n", ($d->{max_ms} // 0);
        printf $fh "  预热 (ms):    %d\n", ($d->{warmup_ms} // 0);
        printf $fh "  阈值 (ms):    %d\n", ($d->{threshold_ms} // 0);
        printf $fh "  需求阈值 (ms): %d\n", ($d->{req_threshold_ms} // 0);
        print $fh "  时序 (ms):    [" . join(", ", @{$d->{runs_ms}}) . "]\n";
        printf $fh "  成功/失败:    %d/%d\n", ($d->{total_ok} // 0), ($d->{total_err} // 0);
        # ReqID:TECC4.0 - 记录模式下 pass 恒真，达标情况看 threshold_met
        # （旧 JSON 无该字段时回落到 pass）
        my $met = defined $d->{threshold_met} ? $d->{threshold_met} : $d->{pass};
        print $fh "  结果：        " . ($d->{record_only}
            ? ($met ? '达标（记录模式，未判红）' : '未达标（记录模式，未判红）')
            : ($d->{pass} ? "通过" : "失败")) . "\n";
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
    print $tap "# 驱动=$driver 加密后端=$crypto KEK存储=$key_store\n";
    print $tap "# 汇总报告：${summary_file}\n";

    return $summary_file;
}

sub _resource_field {
    my ($resource, $key) = @_;

    return '' unless $resource && $resource->{available};
    return defined $resource->{$key} ? $resource->{$key} : '';
}

sub _result_avg_ms {
    my ($result, $perf_runs) = @_;
    my $avg = $result->{avg} // 0;
    my $resource = $result->{resource};

    return $avg if $avg > 0;
    return $avg unless $resource && $resource->{available};
    return $avg unless ($resource->{elapsed_ms} // 0) > 0;
    return $avg unless ($perf_runs // 0) > 0;

    return int(($resource->{elapsed_ms} / $perf_runs) + 0.5);
}

sub save_perf_resource_report {
    my ($module_dir, $results, $opts) = @_;
    my $results_dir;
    my $driver;
    my $perf_rows;
    my $perf_runs;
    my $perf_seed;
    my $filter;
    my $now;
    my $ts;
    my $md_file;
    my $csv_file;
    my $md;
    my $csv;
    my @with_resource;
    my @missing_resource;

    $opts //= {};
    $results_dir = "$module_dir/perf/results";
    mkdir $results_dir unless -d $results_dir;

    $driver = $opts->{driver} // 'libpq';
    my $crypto = perf_crypto_backend($opts->{crypto});
    $perf_rows = $ENV{PERF_ROWS} // 1000000;
    $perf_runs = $ENV{PERF_RUNS} // 5;
    $perf_seed = $ENV{PERF_SEED} // 100000;
    $filter = $ENV{TECC_TEST} // '(all)';
    $now = scalar localtime;

    my ($sec, $min, $hour, $mday, $mon, $year) = localtime;
    $ts = sprintf("%04d%02d%02d_%02d%02d%02d", $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
    $md_file = "$results_dir/RESOURCE_${ts}.md";
    $csv_file = "$results_dir/RESOURCE_${ts}.csv";

    @with_resource = grep { $_->{resource} && $_->{resource}{available} } @$results;
    @missing_resource = grep { !($_->{resource} && $_->{resource}{available}) } @$results;

    open($md, '>', $md_file) or do {
        my $tap = _tap();
        print $tap "# WARNING: cannot write resource report: $md_file\n";
        return undef;
    };

    print $md "# TECC 客户端资源性能报告\n\n";
    print $md "- 时间：$now\n";
    print $md "- 驱动：$driver\n";
    print $md "- 加密后端：$crypto\n";
    print $md "- 环境：PERF_ROWS=$perf_rows PERF_SEED=$perf_seed PERF_RUNS=$perf_runs TECC_TEST=$filter\n";
    print $md "- 环境：TECC_SDF_LIB=" . ($ENV{TECC_SDF_LIB} // '(unset)') . "\n";
    print $md "- 环境：TECC_PERF_HSM=" . ($ENV{TECC_PERF_HSM} // '(unset)') . "\n";
    print $md "- 采集口径：GNU time 包裹客户端进程，覆盖同一进程内的预热和正式执行次数。\n\n";

    print $md "## 客户端资源汇总\n\n";
    print $md "| 用例 | 平均耗时(ms) | user CPU(s) | sys CPU(s) | CPU(%) | 最大RSS(KB) | 主缺页 | 次缺页 | 主动切换 | 被动切换 |\n";
    print $md "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    for my $r (sort { (($b->{resource}{cpu_total_sec} // 0) <=> ($a->{resource}{cpu_total_sec} // 0)) } @with_resource) {
        my $res = $r->{resource};
        my $avg_ms = _result_avg_ms($r, $perf_runs);
        print $md sprintf("| %s | %d | %.2f | %.2f | %s | %s | %s | %s | %s | %s |\n",
            $r->{name},
            $avg_ms,
            ($res->{user_cpu_sec} // 0),
            ($res->{sys_cpu_sec} // 0),
            defined $res->{cpu_percent} ? $res->{cpu_percent} : '',
            defined $res->{max_rss_kb} ? $res->{max_rss_kb} : '',
            defined $res->{major_faults} ? $res->{major_faults} : '',
            defined $res->{minor_faults} ? $res->{minor_faults} : '',
            defined $res->{voluntary_ctxt_switches} ? $res->{voluntary_ctxt_switches} : '',
            defined $res->{involuntary_ctxt_switches} ? $res->{involuntary_ctxt_switches} : '');
    }

    if (@missing_resource) {
        print $md "\n## 未采集资源的用例\n\n";
        for my $r (@missing_resource) {
            my $reason = $r->{resource} ? ($r->{resource}{reason} // '无资源数据') : '无资源数据';
            print $md "- `$r->{name}`：$reason\n";
        }
    }
    close $md;

    open($csv, '>', $csv_file) or do {
        my $tap = _tap();
        print $tap "# WARNING: cannot write resource csv: $csv_file\n";
        return $md_file;
    };
    print $csv "test,avg_ms,user_cpu_sec,sys_cpu_sec,cpu_total_sec,cpu_percent,max_rss_kb,major_faults,minor_faults,voluntary_ctxt_switches,involuntary_ctxt_switches\n";
    for my $r (sort { $a->{name} cmp $b->{name} } @with_resource) {
        my $avg_ms = _result_avg_ms($r, $perf_runs);
        print $csv join(',',
            $r->{name},
            $avg_ms,
            _resource_field($r, 'user_cpu_sec'),
            _resource_field($r, 'sys_cpu_sec'),
            _resource_field($r, 'cpu_total_sec'),
            _resource_field($r, 'cpu_percent'),
            _resource_field($r, 'max_rss_kb'),
            _resource_field($r, 'major_faults'),
            _resource_field($r, 'minor_faults'),
            _resource_field($r, 'voluntary_ctxt_switches'),
            _resource_field($r, 'involuntary_ctxt_switches')) . "\n";
    }
    close $csv;

    my $tap = _tap();
    print $tap "# 客户端资源报告：${md_file}\n";
    print $tap "# 客户端资源CSV：${csv_file}\n";

    return $md_file;
}

1;
