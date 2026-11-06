package tecc_test_utils;

#
# TECC回归测试公共工具模块
# 提供: PG节点管理、SQL执行比对、JDBC构建等公共函数
#

use strict;
use warnings;
use Exporter 'import';
use PostgresNode;
use TestLib;
use Test::More;

# Set default PG_REGRESS path if not already set — PostgresNode.pm
# uses this for --config-auth during node initialization.
# Without it, TestLib::system_or_bail receives undef → "system  failed".
# Dynamically locate pg_regress via pg_config so this works on any machine.
if (!defined $ENV{PG_REGRESS}) {
    chomp(my $pg_config = `which pg_config 2>/dev/null`);
    if ($pg_config) {
        chomp(my $pgxsdir = `pg_config --pgxs 2>/dev/null`);
        if ($pgxsdir) {
            $ENV{PG_REGRESS} = "$pgxsdir/src/test/regress/pg_regress";
        }
    }
}
use Cwd;
use Time::HiRes qw(gettimeofday tv_interval);
use File::Copy;

# 获取TAP输出句柄（TestLib重定向了STDOUT，TAP输出必须走这里）
my $TAP = Test::More->builder->output;

our @EXPORT = qw(
    run_sql_diff_test
    run_sql_dir_diff
    get_tecc_test_filter
    normalize_psql_output
    run_sql_dir_libpq_dual_diff
    normalize_common_output
    print_failure_summary
    setup_tecc_regress_env
    setup_swsds_ini
    restore_swxa_tests
);


# 测试名 → SQL 相对路径映射（供 print_failure_summary 使用）
our %test_sql_label;

# 获取单用例过滤（TECC_TEST=foo.sql 时只跑这一个）
sub get_tecc_test_filter {
    my $filter = $ENV{'TECC_TEST'} || '';
    return $filter;
}

# ---- 临时目录与环境变量设置 ----
# 创建 tmp_check/keys (KEK密钥) 和 tmp_check/tmp (导出文件)
# 设置 TECC_KEK_PATH 供C层覆盖 KEY_PATH 目录
sub setup_tecc_regress_env {
    my $key_dir = "$TestLib::tmp_check/keys";
    my $tmp_dir = "$TestLib::tmp_check/tmp";

    mkdir $key_dir unless -d $key_dir;
    mkdir $tmp_dir unless -d $tmp_dir;

    $ENV{TECC_KEK_PATH} = $key_dir;
    return ($key_dir, $tmp_dir);
}

# ---- 导出文件绝对路径检查 ----
# SQL脚本中 COPY/\\copy/\\i/\\o 的文件路径必须是相对路径(相对于 tmp_check/tmp)
# KEY_PATH 豁免(C层 TECC_KEK_PATH 覆盖)
sub validate_no_absolute_paths {
    my ($sql_content, $script_name) = @_;
    my @violations;
    my @lines = split /\n/, $sql_content;

    for my $i (0 .. $#lines) {
        my $lineno = $i + 1;
        my $line = $lines[$i];
        next unless defined $line;
        next if $line =~ /^\s*--/;
        next if $line =~ /^\s*$/;

        # KEY_PATH 豁免 — C层 TECC_KEK_PATH 覆盖
        next if $line =~ /KEY_PATH\s*=/i;

        # Server COPY TO|FROM 豁免 — PG 要求绝对路径
        next if $line =~ /\bCOPY\b.*\b(TO|FROM)\s+'\/[^']+'/i;

        # \! 中读取 server COPY 输出文件的路径豁免 (head/cat /tmp/srv_* 等)
        next if $line =~ /\\!\s*(head|cat)\s+.*\/tmp\//;

        # psql \copy ... TO|FROM '/...'
        if ($line =~ /\\copy\b.*\b(TO|FROM)\s+'(\/[^']+)'/i) {
            push @violations, "$script_name:$lineno: \\copy $1 '$2' (must be relative path)";
            next;
        }

        # \i '/...' or \i /...
        if ($line =~ /\\i\s+'(\/[^']+)'/) {
            push @violations, "$script_name:$lineno: \\i '$1' (must be relative path)";
            next;
        }
        if ($line =~ /\\i\s+(\/\S+)/) {
            push @violations, "$script_name:$lineno: \\i $1 (must be relative path)";
            next;
        }

        # \o '/...' or \o /...
        if ($line =~ /\\o\s+'(\/[^']+)'/) {
            push @violations, "$script_name:$lineno: \\o '$1' (must be relative path)";
            next;
        }
        if ($line =~ /\\o\s+(\/\S+)/) {
            push @violations, "$script_name:$lineno: \\o $1 (must be relative path)";
            next;
        }
    }

    return @violations;
}

# 执行单个SQL文件并diff期望输出
# $canonical=1 时用规范格式（-A -t -q），去除客户端格式差异，仅保留服务端数据
# 返回值: 1=通过, 0=失败
sub run_sql_diff_test {
    my ($node, $sql_file, $expected_file, $test_name, $canonical, $sql_base) = @_;

    $test_name //= $sql_file;

    my $sql_fullpath;
    if ($sql_file =~ m{^/} || $sql_file =~ m{^[a-zA-Z]:/}) {
        $sql_fullpath = $sql_file;
    } else {
        $sql_fullpath = "$TestLib::tmp_check/$sql_file";
    }

    my $expected_fullpath;
    if ($expected_file =~ m{^/} || $expected_file =~ m{^[a-zA-Z]:/}) {
        $expected_fullpath = $expected_file;
    } else {
        $expected_fullpath = "$TestLib::tmp_check/$expected_file";
    }

    # Fallback to _1.out, _2.out ... when expected file not found
    # (platform-specific variants, e.g. Windows vs Linux output).
    if (! -f $expected_fullpath) {
        for my $n (1 .. 9) {
            my $alt = $expected_fullpath;
            $alt =~ s/\.out$/_$n.out/;
            if (-f $alt) {
                $expected_fullpath = $alt;
                last;
            }
        }
    }

    if (! -f $sql_fullpath) {
        printf "not ok - test %-20s ... FAILED  (SQL file not found)\n", $test_name;
        return 0;
    }

    # ---- 导出文件绝对路径检查 ----
    my $sql_content = slurp_file($sql_fullpath);
    my @violations = validate_no_absolute_paths($sql_content, $sql_file);
    if (@violations) {
        my $pad = " " x (35 - length($test_name));
        print $TAP "not ok - $test_name${pad}... FAILED  (absolute paths not allowed)\n";
        for my $v (@violations) {
            print $TAP "#   $v\n";
        }
        print $TAP "#   导出文件路径请使用相对路径 (CWD = tmp_check/tmp)\n";
        return 0;
    }

    my $connstr = $node->connstr('postgres');
    my @psql_opts = $canonical
        ? ('-X', '-A', '-t')
        : ('-X');

    my $start = [gettimeofday];
    local %ENV = $node->_get_env();
    my ($key_dir, $tmp_dir) = setup_tecc_regress_env();

    # 写入临时 SQL 文件，切换 CWD 到 tmp_check/tmp
    #   - \\copy, \\i, \\o, \\! 相对路径 → 相对于 tmp_check/tmp
    #   - server COPY 相对路径 → 相对于 PG 数据目录(仍在tmp_check下)
    my $pp_file = "$tmp_dir/_pp_$sql_base.sql";
    open(my $pp_fh, '>', $pp_file) or warn "Cannot write $pp_file: $!";
    print $pp_fh $sql_content;
    close($pp_fh);

    use Cwd;
    my $saved_cwd = getcwd();
    chdir($tmp_dir);

    my ($stdout, $stderr) = run_command([
        $node->installed_command('psql'),
        @psql_opts, '-d', $connstr, '-f', $pp_file
    ]);

    chdir($saved_cwd);

    # 恢复原始文件名路径(psql NOTICE中显示临时文件名)
    (my $pp_basename = $pp_file) =~ s{.*/}{};
    $stdout =~ s|\Q$pp_basename\E|$sql_file|g if defined $stdout;
    $stderr =~ s|\Q$pp_basename\E|$sql_file|g if defined $stderr;
    my $elapsed = int(tv_interval($start) * 1000);

    my $expected = -f $expected_fullpath ? slurp_file($expected_fullpath) : '';

    # 规范化换行符
    $stdout =~ s/\r\n/\n/g if defined $stdout;

    # 将stderr合并到stdout
    my $actual = $stdout;
    if (defined $stderr && $stderr ne '') {
        $stderr =~ s/\r\n/\n/g;
        $actual .= "\n" . $stderr;
    }

    # 去除尾部空行
    $actual =~ s/\n*\z// if defined $actual;
    $expected =~ s/\n*\z//;

    # 归一化hex密文（每次运行加密结果不同），处理both actual和expected
    # 匹配两种形式:
    #   1) E'\\x{hex}  — 完整的hex字面量
    #   2) {hex}'::type_enc — psql LINE截断后的形式（E'\\x被截掉）
    my @enc_types = qw(int2_enc int4_enc int8_enc float4_enc float8_enc
                       numeric_enc text_enc varchar_enc bpchar_enc);
    my $enc_types_alt = join('|', @enc_types);
    my $hex_full_re = qr/E'\\x[0-9a-fA-F]+/;
    my $hex_bare_re  = qr/\\x[0-9a-fA-F]+/;
    my $hex_trunc_re = qr/([0-9a-fA-F]{16,})'::($enc_types_alt)/;
    my $bare_replacement = "\\x被隐藏的HEX输出";
    my $replacement = "E'\\\\x被隐藏的HEX输出";

    # EXPLAIN Filter格式: \x<hex>E'\x<hex>'::type — 先整体替换避免拆开处理
    my $explain_hex_re = qr/\\x[0-9a-fA-F]+E'\\x[0-9a-fA-F]+/;
    $actual =~ s/$explain_hex_re/$replacement/ge;
    $expected =~ s/$explain_hex_re/$replacement/ge;

    $actual =~ s/$hex_full_re/$replacement/ge;
    $actual =~ s/$hex_bare_re/$bare_replacement/g;
    $actual =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $expected =~ s/$hex_full_re/$replacement/ge;
    $expected =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $expected =~ s/$hex_bare_re/$bare_replacement/g;

    # LINE error context: "...{hex}' WHERE ..." (hex truncated by psql LINE display)
    my $hex_inline_re = qr/([0-9a-fA-F]{16,})' WHERE /;
    $actual =~ s/$hex_inline_re/HEX_STR' WHERE /g;
    $expected =~ s/$hex_inline_re/HEX_STR' WHERE /g;

    # LINE error context: '...{hex}...' (psql truncates with ... after hex)
    # e.g. LINE 1: ... WHERE name LIKE '310000000000010...
    # e.g. LINE 2: WHERE name COLLATE "C" LIKE '2400000000000100a5...
    my $hex_trunc_ellipsis_re = qr/'([0-9a-fA-F]{4,})\.\.\./;
    $actual =~ s/$hex_trunc_ellipsis_re/'HEX_STR.../g;
    $expected =~ s/$hex_trunc_ellipsis_re/'HEX_STR.../g;

    # 归一化psql NOTICE/WARNING/ERROR中的文件路径（不同机器路径不同）
    $actual =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;
    $expected =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;

    # 归一化端口号（每次运行随机分配，出现在 \connect 失败错误中）
    $actual =~ s/port \d+ failed/port NNNN failed/g;
    $expected =~ s/port \d+ failed/port NNNN failed/g;

    # 归一化tecc_key_log输出的时间戳（`[2026-05-25 09:45:24]`，每次运行都不同）
    $actual =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;
    $expected =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;

    # 归一化 pg_dump \restrict / \unrestrict 随机 token
    $actual =~ s/^\\(restrict|unrestrict) [A-Za-z0-9]+$/\\$1 HIDDEN/mg;
    $expected =~ s/^\\(restrict|unrestrict) [A-Za-z0-9]+$/\\$1 HIDDEN/mg;

    # 归一化 pg_restore TOC entry OID (每次运行不同)
    $actual =~ s/from TOC entry \d+; \d+ \d+/from TOC entry NNN; NNN NNN/g;
    $expected =~ s/from TOC entry \d+; \d+ \d+/from TOC entry NNN; NNN NNN/g;

    # 归一化相对路径的psql NOTICE (\\i 使用相对路径时不带前导/)
    $actual =~ s|psql:([^/\s][^:\s]*\.\w+:\d+: )|psql:/.../$1|g;
    $expected =~ s|psql:([^/\s][^:\s]*\.\w+:\d+: )|psql:/.../$1|g;

    if ($actual ne $expected) {
        my $base = $test_name;
        $base =~ s/[:\/]/_/g;
        $base =~ s/\s+//g;
        # 记录 SQL 相对路径（从绝对路径中提取 tecc_tests/... 部分）
        {
            my $rel = $sql_file;
            $rel =~ s{/[^/]+/\.\./}{/}g;   # 规范化 ../
            $rel =~ s/^.*\/tecc_tests\///;
            $tecc_test_utils::test_sql_label{$base} = $rel;
        }
        my $outdir = "$TestLib::tmp_check/results";
        mkdir $outdir unless -d $outdir;
        my $outfile = "$outdir/${base}.out";
        open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
        if ($fh) {
            print $fh $actual;
            close $fh;
        }
        my $diff_file = undef;
        my $diff = `diff -u "$expected_fullpath" "$outfile" 2>/dev/null || true`;
        if ($diff ne '') {
            $diff_file = "$outdir/${base}.diff";
            open(my $df, '>', $diff_file) or warn "Cannot write $diff_file: $!";
            if ($df) {
                print $df $diff;
                close $df;
            }
        }
        my $pad = " " x (35 - length($test_name));
        print $TAP "not ok - $test_name${pad}... FAILED  $elapsed ms\n";
        print $TAP "#       实际输出: $outfile\n";
        print $TAP "#       差异对比: $diff_file\n" if defined $diff_file;
        return 0;
    } else {
        my $pad = " " x (35 - length($test_name));
        print $TAP "ok - $test_name${pad}... ok      $elapsed ms\n";
        return 1;
    }
}

# 遍历SQL目录批量执行diff测试
# 如果 TECC_TEST 环境变量设了（如 TECC_TEST=test_ope.sql），只跑这一个
# $canonical=1 时使用规范格式对比（-A -t），去客户端展示差异
# 返回值: 通过的测试数, 总测试数
sub run_sql_dir_diff {
    my ($node, $sql_dir, $expected_dir, $test_prefix, $canonical) = @_;

    $test_prefix //= '';

    my $filter = get_tecc_test_filter();

    opendir(my $dh, $sql_dir) or do {
        printf "not ok - test %-25s ... FAILED  (cannot open directory)\n", $test_prefix;
        return (0, 0);
    };

    my @sql_files = sort grep {
        my $f = $_;
        $f =~ /\.sql$/ && (!$filter || $f eq $filter)
    } readdir($dh);
    closedir($dh);

    my $passed = 0;
    my $total = scalar @sql_files;

    foreach my $sql_file (@sql_files) {
        my $base = $sql_file;
        $base =~ s/\.sql$//;
        my $expected_file = "$expected_dir/${base}.out";
        # Prefer _1.out (Windows platform variant) if it exists
        my $expected_alt = "$expected_dir/${base}_1.out";
        $expected_file = $expected_alt if -f $expected_alt;
        my $test_name = $test_prefix ? "${test_prefix}: ${base}" : $base;

        $passed += run_sql_diff_test($node,
            "$sql_dir/$sql_file",
            $expected_file,
            $test_name,
            $canonical);
    }

    return ($passed, $total);
}

# ============ psql -X 输出归一化 ============

sub normalize_psql_output {
    my ($output) = @_;
    my @lines = split /\n/, $output;
    my @result;

    for my $i (0 .. $#lines) {
        my $line = $lines[$i];
        next unless defined $line;

        # 分隔线：全行仅含 - + | 和空白，且至少含 3 个连续 dash
        if ($line =~ /^[-+|\s]+$/ && $line =~ /-{3,}/) {
            pop @result if @result;
            next;
        }

        # 结果集间的空行（psql -X 输出中跨结果集的空行，-A -t 无此输出）
        if ($line =~ /^\s*$/) {
            next;
        }

        # 数据行：trim 每个 | 段的对齐空格（| 分割的保留列对齐）
        # 单列无 | 的数据行也全局 trim，去掉 psql 列对齐的空格填充
        if ($line =~ /\|/) {
            my @parts = split /\|/, $line;
            for my $part (@parts) {
                $part =~ s/^\s+//;
                $part =~ s/\s+$//;
            }
            $line = join '|', @parts;
        } else {
            $line =~ s/^\s+//;
            $line =~ s/\s+$//;
        }

        push @result, $line;
    }

    my $normalized = join "\n", @result;
    $normalized =~ s/\n*\z//;
    return $normalized;
}

# ============ SEARCH非确定性行剥离 ============
#
# ============ libpq 双比较（raw + common） ============

sub run_sql_dir_libpq_dual_diff {
    my ($node, $sql_dir, $raw_expected_dir, $common_expected_dir, $test_prefix) = @_;

    my $filter = get_tecc_test_filter();
    my $mode = $ENV{'MODE'} // 'both';

    $test_prefix //= '';

    opendir(my $dh, $sql_dir) or do {
        printf "not ok - test %-25s ... FAILED  (cannot open directory)\n", $test_prefix;
        return (0, 0);
    };

    my @sql_files = sort grep {
        my $f = $_;
        $f =~ /\.sql$/ && (!$filter || $f eq $filter)
    } readdir($dh);
    closedir($dh);

    my $tp_passed = 0;
    my $tp_total = 0;
    my $connstr = $node->connstr('postgres');

    my $TAP = Test::More->builder->output;

    # 第一遍：收集每个 SQL 的输出，同时保存元数据
    my @tasks;
    foreach my $sql_file (@sql_files) {
        my $base = $sql_file;
        $base =~ s/\.sql$//;
        my $sql_fullpath = "$sql_dir/$sql_file";
        my $raw_expected_file = "$raw_expected_dir/${base}.out";
        if (! -f $raw_expected_file) {
            my $raw_expected_alt = "$raw_expected_dir/${base}_1.out";
            $raw_expected_file = $raw_expected_alt if -f $raw_expected_alt;
        }
        my $common_expected_file = "$common_expected_dir/${base}.out";

        # ---- 导出文件绝对路径检查 ----
        my $sql_content = slurp_file($sql_fullpath);
        my @violations = validate_no_absolute_paths($sql_content, $sql_file);
        if (@violations) {
            my $base = $sql_file;
            $base =~ s/\.sql$//;
            my $test_name = $test_prefix ? "${test_prefix}: ${base}" : $base;
            my $pad = " " x (35 - length($test_name));
            print $TAP "not ok - $test_name${pad}... FAILED  (absolute paths)\n";
            for my $v (@violations) {
                print $TAP "#   $v\n";
            }
            push @tasks, { base => $base, sql_fullpath => $sql_fullpath,
                raw_expected => $raw_expected_file,
                common_expected => $common_expected_file,
                raw_output => "ABSOLUTE_PATH_VIOLATION",
                common_output => "ABSOLUTE_PATH_VIOLATION",
                skip => 1 };
            next;
        }

        local %ENV = $node->_get_env();
        my ($key_dir, $tmp_dir) = setup_tecc_regress_env();

        my $pp_file = "$tmp_dir/_pp_${base}.sql";
        open(my $pp_fh, '>', $pp_file) or warn "Cannot write $pp_file: $!";
        print $pp_fh $sql_content;
        close($pp_fh);

        use Cwd;
        my $saved_cwd = getcwd();
        chdir($tmp_dir);

        my ($stdout, $stderr) = run_command([
            $node->installed_command('psql'),
            '-X', '-d', $connstr, '-f', $pp_file
        ]);

        chdir($saved_cwd);

        # 恢复原始文件名
        (my $pp_basename = $pp_file) =~ s{.*/}{};
        $stdout =~ s|\Q$pp_basename\E|$sql_file|g if defined $stdout;
        $stderr =~ s|\Q$pp_basename\E|$sql_file|g if defined $stderr;

        my $raw_out = $stdout;
        if (defined $stderr && $stderr ne '') {
            $raw_out .= "\n" . $stderr;
        }

        push @tasks, {
            base         => $base,
            sql_fullpath => $sql_fullpath,
            raw_expected => $raw_expected_file,
            common_expected => $common_expected_file,
            raw_output   => $raw_out,
            common_output => normalize_psql_output($raw_out),
        };
    }

    # raw 比较段
    if ($mode eq 'both' || $mode eq 'raw') {
        note("\n=== ${test_prefix}用例libpq原始输出比对 ===");
        foreach my $t (@tasks) {
            my $base = $t->{base};
            $tp_total++;
            my $test_name = $test_prefix ? "${test_prefix}_raw: ${base}" : "raw: ${base}";
            if ($t->{skip}) { $tp_total++; next; }
            if (run_sql_diff_test($node, $t->{sql_fullpath}, $t->{raw_expected}, $test_name, 0, $base)) {
                $tp_passed++;
            }
        }
    }

    # common 比较段
    if ($mode eq 'both' || $mode eq 'common') {
        note("\n=== ${test_prefix}用例libpq规范化输出比对 ===");
        foreach my $t (@tasks) {
            my $base = $t->{base};
            $tp_total++;
            my $test_name = $test_prefix ? "${test_prefix}_common: ${base}" : "common: ${base}";
            if ($t->{skip}) { next; }
            my $expected = '';
            my $common_expected_path = $t->{common_expected};
            if (! -f $common_expected_path) {
                my $alt = $common_expected_path;
                $alt =~ s/\.out$/_1.out/;
                $common_expected_path = $alt if -f $alt;
            }
            if (-f $common_expected_path) {
                $expected = slurp_file($common_expected_path);
            }
            $expected =~ s/\r\n/\n/g;
            $expected =~ s/\n*\z//;

            my $common_output = $t->{common_output};
            $common_output =~ s/\r\n/\n/g;
            $common_output = normalize_common_output($common_output, $base);
            $expected = normalize_common_output($expected, $base);

            if ($common_output ne $expected) {
                    my $outdir = "$TestLib::tmp_check/results";
                    mkdir $outdir unless -d $outdir;
                    my $stem = "${test_prefix}_${base}_common";
                    my $outfile = "$outdir/${stem}.out";
                    {
                        my $rel = $t->{sql_fullpath};
                        $rel =~ s{/[^/]+/\.\./}{/}g;   # 规范化 ../
            $rel =~ s/^.*\/tecc_tests\///;
                        $tecc_test_utils::test_sql_label{$stem} = $rel;
                    }
                    open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
                    print $fh $common_output if $fh;
                    close $fh if $fh;
                    my $diff = `diff -u "$common_expected_path" "$outfile" 2>/dev/null || true`;
                    my $diff_file;
                    if ($diff ne '') {
                        $diff_file = "$outdir/${stem}.diff";
                        open(my $df, '>', $diff_file) or warn "Cannot write $diff_file: $!";
                        if ($df) {
                            print $df $diff;
                            close $df;
                        }
                    }
                    print $TAP "not ok - $test_name ... FAILED\n";
                    print $TAP "#       实际输出: $outfile\n";
                    print $TAP "#       差异对比: $diff_file\n" if defined $diff_file;
                } else {
                    print $TAP "ok - $test_name ... ok\n";
                    $tp_passed++;
                }
        }
    }

    return ($tp_passed, $tp_total);
}

# ============ common 比较归一化（hex隐藏 + NOTICE路径归一化） ============
# 同时适用于libpq和JDBC输出，跨驱动比对的公共归一化函数

sub normalize_common_output {
    my ($text, $sql_base) = @_;
    return '' unless defined $text;

    my $result = $text;

    # 1. Hex 密文隐藏 — 与 raw normalizer 保持能力统一
    my @enc_types = qw(int2_enc int4_enc int8_enc float4_enc float8_enc
                       numeric_enc text_enc varchar_enc bpchar_enc);
    my $enc_types_alt = join('|', @enc_types);
    my $hex_full_re   = qr/E'\\\\x[0-9a-fA-F]+/;
    my $hex_bare_re   = qr/\\\\x[0-9a-fA-F]+/;
    my $hex_trunc_re  = qr/([0-9a-fA-F]{16,})'::($enc_types_alt)/;
    my $replacement   = "E'\\\\x被隐藏的HEX输出";
    my $bare_replacement = "\\\\x被隐藏的HEX输出";
    # LIKE 模式中的裸 hex (单引号内长 hex 串，无 E'\x 前缀)
    my $hex_like_re   = qr/'([0-9a-fA-F]{16,})'/;

    # EXPLAIN Filter格式: \x<hex>E'\x<hex>'::type — 先整体替换避免拆开处理
    my $explain_hex_re = qr/\\x[0-9a-fA-F]+E'\\\\x[0-9a-fA-F]+/;
    $result =~ s/$explain_hex_re/$replacement/ge;

    $result =~ s/$hex_full_re/$replacement/ge;
    $result =~ s/$hex_bare_re/$bare_replacement/g;
    $result =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $result =~ s/$hex_like_re/"'被隐藏的HEX输出'"/ge;


    # LINE error context: psql truncates with ... after hex
    $result =~ s/'([0-9a-fA-F]{4,})\.\.\./'HEX_STR.../g;

    # psql LINE error context: "...{hex}' WHERE ..." (hex truncated by psql LINE display)
    my $hex_inline_re = qr/([0-9a-fA-F]{16,})' (WHERE|ORDER|LIKE|AND|OR|GROUP|LIMIT|HAVING) /;
    $result =~ s/$hex_inline_re/被隐藏的HEX输出' $2 /g;

    # 2. psql 完整路径缩短为 /.../ 形式
    $result =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;

    # 3. NOTICE/WARNING/ERROR 路径前缀归一化
    $result =~ s/^psql:\/\.\.\.\/[^\s]+:\d+:\s*(NOTICE|WARNING|ERROR):\s+/$1: /gm;
    $result =~ s/^(NOTICE|WARNING|ERROR):\s{2,}/$1: /gm;

    # 4. 时间戳归一化（`[2026-05-25 09:45:24]`，每次运行都不同）
    $result =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;

    # 5. 去除尾部空行
    $result =~ s/\n*\z//;


    return $result;
}

# ============ 失败汇总（TAP # 诊断格式，prove -v 可见）============

sub print_failure_summary {
    my $results_dir = "$TestLib::tmp_check/results";
    return unless -d $results_dir;

    my @out_files  = sort glob("$results_dir/*.out");
    my @diff_files = sort glob("$results_dir/*.diff");
    return unless @out_files || @diff_files;

    my %groups;
    foreach my $f (@out_files, @diff_files) {
        my $basename = $f;
        $basename =~ s{.*/}{};
        my ($group) = $basename =~ /^(.+)\.(out|diff)$/;
        next unless $group;
        $groups{$group}{$f =~ /\.([^.]+)$/ ? $1 : ''} = $f;
    }

    my $TAP = Test::More->builder->output;
    print $TAP "\n# ╔═════════════════════════════╗\n";
    print $TAP "# ║  TECC 回归测试 — 失败汇总   ║\n";
    print $TAP "# ╚═════════════════════════════╝\n";
    print $TAP "#\n";

    foreach my $group (sort keys %groups) {
        my $sql_path = $tecc_test_utils::test_sql_label{$group} // '';
        print $TAP "# ==== ${group} ====\n";
        if ($sql_path) {
            print $TAP "#   问题用例: ${sql_path}\n";
        }
        if ($groups{$group}{out}) {
            print $TAP "#   实际输出: $groups{$group}{out}\n";
        }
        if ($groups{$group}{diff}) {
            print $TAP "#   差异对比: $groups{$group}{diff}\n";
        }
        print $TAP "#\n";
    }
}

#
# setup_swsds_ini - Setup HSM test environment (SWXA + TZHSM)
#
# SWXA (三未信安): copies swsds.ini to PGDATA/ and ./
# TZHSM (同智伟业): checks /usr/lib64/cfg/client_config.xml
# 
# If HSM config not found, corresponding .sql files are renamed to .skip
# so the test framework won't discover them.
# Returns 1 if any HSM is available, 0 if all skipped.
sub setup_swsds_ini {
    my ($node, $module_dir) = @_;
    my $TAP = Test::More->builder->output;

    # Disable all HSM tests initially
    for my $f ("$module_dir/shared/sql/swxa_enc.sql",
               "$module_dir/shared/sql/swxa_search.sql",
               "$module_dir/shared/sql/tzhsm_enc.sql",
               "$module_dir/cross/sql/005_swxa_cross.sql")
    {
        my $skip = "$f.skip";
        rename($f, $skip) if -f $f;
    }

    # Check SWXA config
    my $swxa_ok = 0;
    for my $src ('/etc/swsds.ini', './swsds.ini', '/home/why/pg14/data/swsds.ini') {
        next unless -f $src;
        my $datadir = $node->data_dir;
        copy($src, "$datadir/swsds.ini") or warn "copy fail: $!";
        copy($src, "swsds.ini") or warn "copy fail: $!";
        print $TAP "# swsds.ini: $src -> PGDATA/ and ./
";
        $swxa_ok = 1;
        last;
    }

    # TZHSM (同智伟业) password machine currently unavailable
    my $tzhsm_ok = 0;

    # Restore available HSM tests
    if ($swxa_ok) {
        for my $f ("$module_dir/shared/sql/swxa_enc.sql",
                   "$module_dir/shared/sql/swxa_search.sql",
                   "$module_dir/cross/sql/005_swxa_cross.sql")
        {
            my $skip = "$f.skip";
            rename($skip, $f) if -f $skip;
        }
    } else {
        print $TAP "# SWXA tests skipped (swsds.ini not found)
";
    }

    if ($tzhsm_ok) {
        my $skip = "$module_dir/shared/sql/tzhsm_enc.sql.skip";
        my $restore = "$module_dir/shared/sql/tzhsm_enc.sql";
        rename($skip, $restore) if -f $skip;
    } else {
        print $TAP "# TZHSM tests skipped (password machine unavailable)
";
    }

    return ($swxa_ok || $tzhsm_ok) ? 1 : 0;
}

sub restore_swxa_tests {
    my ($module_dir) = @_;
    for my $f ("$module_dir/shared/sql/swxa_enc.sql",
               "$module_dir/shared/sql/swxa_search.sql",
               "$module_dir/shared/sql/tzhsm_enc.sql",
               "$module_dir/cross/sql/005_swxa_cross.sql")
    {
        my $skip = "$f.skip";
        rename($skip, $f) if -f $skip;
    }
}

1;
