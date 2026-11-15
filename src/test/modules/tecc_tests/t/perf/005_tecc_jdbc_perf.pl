#!/usr/bin/perl

# 005_tecc_jdbc_perf.pl
#
# TECC JDBC 性能测试
# 所有数据操作（DDL、数据初始化、性能测量）全部通过 JDBC TeccSqlTestRunner 执行。
# 数据生成策略（种子+倍增）与 libpq 侧相同，但通过 JDBC 加密。
#
# 用例集合与 libpq 侧（004）一致：perf/sql/ 下 23 个共享 SQL 用例 +
# 4 个由 seed 阶段顺带测量的 INSERT 用例（int4/float8/text/he_int8_insert），
# 共 27 个，与 thresholds.json 的键一一对应。
#
# 与 libpq 侧的既有差异（仅数据规模，用例与口径不变）：he_int8 的种子按用例
# 实际读取行数取（PERF_HE_ROWS，默认 10000），原因是 JDBC 同态客户端加密约
# 38ms/值、比 libpq 慢约 40 倍，照 libpq 的 10 万行种子要跑近 2 小时。
#
# 执行:
#   1. 启动PG节点
#   2. DDL（perf_ddl.sql，通过 TeccSqlTestRunner）
#   3. 数据初始化（按表按 run 生成 SQL 文件，通过 TeccSqlTestRunner 执行 JDBC 插入）
#   4. 倍增 stage→target（--enable-tecc 0，对齐 libpq 的 psql -k off 纯服务端字节拷贝）
#   5. TeccSqlTestRunner --perf-mode 测量所有 SQL 用例
#   6. 解析TSV输出 → 阈值校验 → TAP输出
#
# 硬加密模式（ReqID:TECC4.0）——与功能回归同一套开关语义，外加 005 的 localkms 基线：
#   TECC_SDF_LIB 已设                 → SoftSDF（软件 SDF）：KEK=swxakms，
#                                       classpath 首位挂 JCE 门面 libteccjce.jar
#   TECC_SDF_LIB 未设 且 TECC_PERF_HSM=1 → 真实加密机：KEK=swxakms，
#                                       --driver-dir 挂厂商 JAR，CWD 放真 swsds.ini
#   其余（默认）                      → localkms，不影响既有 localkms 报告可复现
# SoftSDF/真机 的主开关仍是 TECC_SDF_LIB（设=SoftSDF，与 psql/libpq 一致）；
# 005 默认跑 localkms，故真机须显式 TECC_PERF_HSM=1 打开，避免 HSM 在线时
# 把 localkms 基线悄悄测成硬加密。两者同时设置直接 die。

use strict;
use warnings;
use Config;
use Cwd qw(getcwd);
use PostgresNode;
use TestLib;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);
use POSIX ();

use FindBin;
use lib "$FindBin::Bin/..";
use tecc_perf_utils;
use tecc_jdbc_jar;
use tecc_test_utils;

my $jdbc_home = $ENV{'TECC_JDBC_HOME'} || '';
if (!$jdbc_home) {
    plan skip_all => "TECC_JDBC_HOME not set, skipping JDBC perf tests";
}

my $TAP = Test::More->builder->output;
my $module_dir = "$FindBin::Bin/../..";

# 定位JDBC jar和测试类
# ReqID:TECC4.0 - 驱动 JAR 必须按版本选（build/libs 会累积历次构建产物），
# 选取规则见 t/tecc_jdbc_jar.pm —— 不能再用 glob 的字典序结果。
my @jars = jdbc_jar_candidates($jdbc_home);
if (!@jars) {
    plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first";
}

my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
if (!-d $test_classes) {
    plan skip_all => "JDBC test classes not found; run 'make jdbc-compile' first";
}

my $jdbc_jar = $jars[0];
print $TAP "# 驱动 JAR: " . jdbc_jar_selection_note(\@jars) . "\n";

# ReqID:TECC4.0 - 硬加密模式前置校验，在 initdb 之前 die，避免跑完一轮才发现环境不全。
# SoftSDF：libteccsdf.so + 预置 KEK + JCE 门面；真机：厂商 JAR 目录 + 可达 swsds.ini。
my $softsdf = $ENV{TECC_SDF_LIB} ? 1 : 0;
my $real_hsm = 0;
if (!$softsdf && (($ENV{TECC_PERF_HSM} // '') =~ /^(1|true|yes|hsm|swxa)$/i)) {
    $real_hsm = 1;
}
die "perf: TECC_SDF_LIB（SoftSDF）与 TECC_PERF_HSM=1（真机）不能同时设置\n"
    if $softsdf && $real_hsm;
my $use_hsm = $softsdf || $real_hsm;   # KEK 走 key_store='swxakms'
# ReqID:TECC4.0 - 阈值口径：005 默认保持判红（历史口径不变），可用
# TECC_PERF_RECORD_ONLY=1 切成"只记录"、TECC_PERF_ENFORCE=1 显式判红。
set_perf_record_only(perf_record_only_env() // 0);
my $jce_jar;       # 仅 SoftSDF：JCE 门面（真机绝不能进 classpath，会顶掉厂商同名类）
my $vendor_dir;    # 仅真机：厂商 JAR 目录（--driver-dir + classpath 前缀）
my $vendor_cp;     # 仅真机：厂商 JAR 的 classpath 串（必须排在驱动 JAR 之前）
my $runner_cwd;    # 硬加密下 runner 必须在 tmp_check/tmp 执行（SoftSDF 读预置密钥 / 真机读 swsds.ini）
my $hsm_props;     # setup_swsds_ini 生成的 hsm.properties（--tecc-config）
if ($softsdf) {
    # ReqID:TECC4.0 - 这里必须调 init_sdf_mode：它把 TECC_SDF_LIB 校验/归一化成绝对
    # 路径并置 $sdf_lib_path，后面 setup_swsds_ini 靠 sdf_soft_mode() 判 SoftSDF 分支
    # （铺预置 kek-*/sm2* 到 tmp_check/tmp + 写占位 hsm.properties）。缺了它，
    # setup_swsds_ini 会错走真机分支（软模式没预置密钥 → KEK 建不出来）。
    # 历史上这里写的是不存在的 check_softsdf_env()，SoftSDF 入口直接 die。
    init_sdf_mode($module_dir);
    $jce_jar = $ENV{TECC_SDF_JCE_JAR}
        || "$module_dir/../../../../tecc_kms/bin/libteccjce.jar";
    if (!-f $jce_jar) {
        die "SoftSDF JCE facade not found: $jce_jar\n"
          . "Build it with: make -C src/test/modules/tecc_tests tecc-sdf-jce-jar\n";
    }
    print $TAP "# SoftSDF 模式: TECC_SDF_LIB=$ENV{TECC_SDF_LIB}\n";
    print $TAP "# SoftSDF JCE 门面: $jce_jar\n";
} elsif ($real_hsm) {
    # 对齐 002：厂商身份由 KEK 的 key_store 决定；目录默认 pgjdbc/libs，
    # 仓库外用 TECC_HSM_VENDOR_DIR 覆盖（覆盖项，不是模式开关）。
    # 注意：仅 --driver-dir 不够 —— 驱动会先从自身 JAR 加载 SwxaProvider 空壳 stub，
    # 连通性检查报「vendor JAR not found on classpath」；厂商 JAR 必须进 -cp
    # 且排在驱动 JAR 之前（与 SoftSDF 把门面放首位同一规则）。
    $vendor_dir = $ENV{TECC_HSM_VENDOR_DIR} || "$jdbc_home/pgjdbc/libs";
    if (!-d $vendor_dir) {
        die "perf: 真机模式缺少厂商 JAR 目录: $vendor_dir\n"
          . "Set TECC_HSM_VENDOR_DIR to the vendor JAR directory.\n";
    }
    my @vjars = sort glob("$vendor_dir/*.jar");
    die "perf: 真机模式厂商 JAR 目录为空: $vendor_dir\n" unless @vjars;
    $vendor_cp = join(':', @vjars);
    print $TAP "# 真机 HSM 模式: TECC_PERF_HSM=$ENV{TECC_PERF_HSM}\n";
    print $TAP "# 真机模式: 厂商 JAR 目录 -> $vendor_dir\n";
    print $TAP "# 真机模式: vendor classpath 首位 -> $vjars[0]\n";
}

# runner 的 markdown 报告目录。让 runner 落盘报告（而不是 --output-format psql
# 把结果丢到 stdout）有两个原因：
#   1) 语句失败只体现在报告里（状态 ❌ + 错误消息），stderr 为空时看不出失败 ——
#      旧脚本只看 stderr，DDL 静默失败也报"完成"；
#   2) 报告汇总表带「每语句耗时」，seed 阶段的 INSERT 用例由此取数（同 libpq 的 \timing）。
my $report_dir = "$TestLib::tmp_check/jdbc_reports";
mkdir $report_dir unless -d $report_dir;

# seed 的 run 文件目录。runner 的 --sql-files 只接受「相对 --sql-dir 的文件名」
# （内部按 new File(sqlDir, name) 解析，绝对路径拼不上），故 run 文件统一落在这里。
my $seed_dir = "$TestLib::tmp_check/jdbc_seed";
mkdir $seed_dir unless -d $seed_dir;

# classpath 规则：SoftSDF 门面 / 真机厂商 JAR 必须排在驱动 JAR 之前，
# 否则命中驱动内 SwxaProvider stub（连通性检查直接失败）。
sub _runner_classpath {
    return $jce_jar
        ? "${jce_jar}:${test_classes}:${jdbc_jar}"
        : $vendor_cp
            ? "${vendor_cp}:${test_classes}:${jdbc_jar}"
            : "${test_classes}:${jdbc_jar}";
}

# 构造 java 命令（执行 SQL 文件的基础参数）
# tecc => 0 时关闭客户端 TECC 处理器（对齐 libpq 侧的 psql -k off：倍增纯服务端字节拷贝）
# SoftSDF/真机 都传 --tecc-config（HSM KEK 必填，对齐 002）；真机另传 --driver-dir。
sub build_java_cmd {
    my ($port, $extra_args, $tecc) = @_;
    my $classpath = _runner_classpath();
    my @cmd = (
        'java', '-cp', $classpath,
        'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
        '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
        '--user', 'postgres', '--password', '',
        '--enable-tecc', (defined $tecc ? $tecc : 1),
    );
    push @cmd, ('--tecc-config', $hsm_props)
        if $use_hsm && $hsm_props;
    push @cmd, ('--driver-dir', $vendor_dir)
        if $real_hsm && $vendor_dir;
    push @cmd, @$extra_args if $extra_args;
    return @cmd;
}

# 硬加密下在 runner_cwd 执行 java（SoftSDF 按相对 CWD 读 kek-*/sm2*；
# 真机厂商 JCE 按 JVM 工作目录读 swsds.ini）；localkms 直接 run_command。
# 返回 (stdout, stderr)。
sub _run_java {
    my ($cmd) = @_;
    if ($use_hsm && $runner_cwd) {
        my $saved = getcwd();
        chdir($runner_cwd)
            or die "perf: chdir $runner_cwd failed: $!";
        my @r = run_command($cmd);
        chdir($saved)
            or die "perf: chdir back to $saved failed: $!";
        return @r;
    }
    return run_command($cmd);
}

# 读取 runner 的 markdown 报告；不存在返回 undef
sub _read_report {
    my ($path) = @_;
    open(my $fh, '<', $path) or return undef;
    local $/;
    my $text = <$fh>;
    close $fh;
    return $text;
}

# 解析报告汇总表行：| he_ins.sql | 1 | INSERT | ✅ | 2488ms |
# 返回 (失败行列表, 全部语句明细)
sub _parse_jdbc_report {
    my ($text) = @_;
    my (@stmts, @failed);
    for my $line (split /\n/, $text) {
        next unless $line =~ /^\|\s*([^|]+?)\s*\|\s*(\d+)\s*\|\s*([^|]*?)\s*\|\s*([^|]*?)\s*\|\s*(\d+)ms\s*\|/;
        my $stmt = {
            file   => $1,
            idx    => int($2),
            type   => $3,
            status => $4,
            ms     => int($5),
        };
        push @stmts, $stmt;
        push @failed, $stmt if $stmt->{status} !~ /✅/;
    }
    my @msgs;
    while ($text =~ /^\*\*错误消息\*\*: (.*)$/mg) {
        push @msgs, $1;
    }
    return (\@failed, \@stmts, \@msgs);
}

# 文件路径 → 报告基名（runner 用 SQL 文件基名命名报告）
sub _report_base {
    my ($path) = @_;
    my ($b) = $path =~ m{([^/]+)$};
    $b =~ s/\.sql$//;
    return $b;
}

# 调 runner 执行一批 SQL 文件并收集报告。
# 任一句失败即 die（DDL/种子/倍增失败必须立刻中止，否则后续测量会在空表上
# 得出"跑得飞快"的假数据）。
# tecc 传 0 表示关闭客户端 TECC 处理器（对齐 libpq 的 psql -k off）。
# 返回 { <报告基名> => [ 语句耗时明细 ] }
sub _run_runner_and_collect {
    my ($port, $file_args, $bases, $label, $tecc) = @_;
    print $TAP "#   [jdbc] ${label} ... ";

    my @cmd = build_java_cmd($port, [
        @$file_args,
        '--modes', 'simple',
        '--output', $report_dir,
    ], $tecc);
    my ($stdout, $stderr) = _run_java(\@cmd);

    # 进程级异常（连接失败、驱动异常）不会进报告，需单独判
    my @proc_err = grep { /ERROR|Exception|at org\./ }
        split /\n/, ($stderr // '') . "\n" . ($stdout // '');

    my %result;
    for my $base (@$bases) {
        my $text = _read_report("$report_dir/${base}.md");
        if (!$text) {
            print $TAP "失败（$base 未生成报告）\n";
            print $TAP "#     $_\n" for @proc_err;
            die "perf: $label 未生成 runner 报告（$base.md）\n";
        }
        my ($failed, $stmts, $msgs) = _parse_jdbc_report($text);
        if (@$failed or @proc_err) {
            print $TAP "失败（$base）\n";
            print $TAP "#     语句 #$_->{idx} ($_->{type}) 未通过\n" for @$failed;
            print $TAP "#     错误消息: $_\n" for @$msgs;
            print $TAP "#     $_\n" for @proc_err;
            die "perf: $label 执行失败（$base）\n";
        }
        $result{$base} = $stmts;
    }

    my @all = map { @$_ } values %result;
    my $total_ms = 0;
    $total_ms += $_->{ms} for @all;
    # 注意：$TAP 是本框架的 SimpleTee 句柄，只支持 print，不能用 printf
    print $TAP sprintf("完成（%d 个文件/%d 条语句，合计 %d ms）\n",
        scalar @$bases, scalar @all, $total_ms);
    return \%result;
}

# 执行单个 SQL 文件（DDL / 倍增），返回该文件内的语句耗时明细
sub run_sql_via_jdbc {
    my ($port, $sql_file, $label, $tecc) = @_;
    my $base = _report_base($sql_file);
    my $res = _run_runner_and_collect($port, ['--sql-file', $sql_file],
        [$base], $label // $base, $tecc);
    return $res->{$base};
}

# 执行同一目录下的一组 SQL 文件（seed 的 runs 个 run 文件）：
# 一次调用 = 一次 JVM + 一次连接，对齐 libpq 侧「单 psql 进程连跑 runs 个 run 文件」
# 的口径（启动/连接/TECC 缓存初始化只付一次）。
# 返回 { <文件基名> => [ 语句耗时明细 ] }
sub run_sql_files_in_dir_via_jdbc {
    my ($port, $dir, $names, $label, $tecc) = @_;
    return _run_runner_and_collect($port,
        ['--sql-dir', $dir, '--sql-files', join(',', @$names)],
        [map { _report_base($_) } @$names], $label, $tecc);
}

# 生成一个批次的 VALUES 元组（与 libpq 侧 init_encrypted_data 的种子形状一致）
sub _seed_values {
    my ($t, $start, $end) = @_;
    my @vals;
    if ($t eq 'int4') {
        for my $i ($start .. $end) { push @vals, "($i,$i)"; }
    } elsif ($t eq 'float8') {
        for my $i ($start .. $end) {
            my $f = sprintf("%.6f", $i * 0.000001);
            push @vals, "($f,$f)";
        }
    } elsif ($t eq 'text') {
        # val_search 用 5 个 5 字符前缀轮换（各 20% 命中率），保证模糊查询
        # 前缀匹配（LIKE '第一数据组%'）扫描 5 倍行数命中 1 行
        my @like_prefix = ('第一数据组', '第二数据组', '第三数据组',
                           '第四数据组', '第五数据组');
        for my $i ($start .. $end) {
            my $pfx = $like_prefix[($i - 1) % 5];
            my $suffix = sprintf("%06d", $i);
            push @vals, "('第${i}条测试记录','第${i}条测试记录','${pfx}${suffix}')";
        }
    } elsif ($t eq 'he_int8') {
        # 同态表：val_add 递增整型（供 he_add_10000/he_add_sum_10k）；
        # val_mul 固定为 1（供 he_mul_pow_10000：1^10000=1 不溢出 INT8）
        for my $i ($start .. $end) { push @vals, "($i,1)"; }
    }
    return join(", ", @vals);
}

# ============ 启动PG ============
print $TAP "# 启动 PostgreSQL 临时节点...\n";
my $node = get_new_node('tecc_jdbc_perf');
$ENV{LC_ALL} = 'zh_CN.UTF-8';
$node->init;
# SoftSDF/真机：与 002 相同，在 init 后、start 前做 setup_swsds_ini
# （SoftSDF 预置 kek-*/sm2* 并写占位 hsm.properties；真机拷 swsds.ini +
# 写指向真机的 hsm.properties），并 chdir 到 tmp_check/tmp。
# 密码机不可用时 setup_swsds_ini 返回 0，本入口直接中止 —— perf 的用例都要走硬加密 KEK。
if ($use_hsm) {
    my $hsm_ok = setup_swsds_ini($node, $module_dir);
    die "perf: " . ($softsdf ? 'SoftSDF' : '真机 HSM')
        . " 模式下 setup_swsds_ini 未就绪（swxa 不可用）\n"
        unless $hsm_ok;
    $hsm_props = $tecc_test_utils::hsm_properties_path;
    die "perf: " . ($softsdf ? 'SoftSDF' : '真机 HSM')
        . " 模式下 setup_swsds_ini 未生成 hsm.properties\n"
        unless $hsm_props && -f $hsm_props;
    $runner_cwd = "$TestLib::tmp_check/tmp";
    die "perf: runner CWD 不存在: $runner_cwd\n" unless -d $runner_cwd;
    if ($real_hsm) {
        die "perf: 真机模式 runner CWD 缺少 swsds.ini: $runner_cwd/swsds.ini\n"
            unless -f "$runner_cwd/swsds.ini";
    }
    print $TAP "# " . ($softsdf ? 'SoftSDF' : '真机 HSM')
        . " hsm.properties: $hsm_props\n";
    print $TAP "# runner CWD: $runner_cwd\n";
}
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

# perf_ddl.sql 建出的全部表（DDL 静默失败时后续测量全是假数据，故每步都校验行数）
my @required_tables = qw(
    enc_int4 enc_int4_b enc_float8 enc_text he_int8
    stage_int4 stage_float8 stage_text stage_he_int8
);

# 校验各表行数；不符即 die 并打印实际值
# 注意：psql -c 一次只回显**最后一个**结果集，故必须拼成单条语句
# （标量子查询 + 字符串连接），否则前面几张表的计数根本看不到。
sub check_table_rows {
    my ($label, $expect) = @_;
    my $sql = 'SELECT ' . join(" || ' ' || ",
        map { "'$_=' || (SELECT count(*) FROM $_)" } @required_tables);
    my ($out, $err) = run_command([
        $psql, '-h', '127.0.0.1', '-p', $port, '-U', 'postgres',
        '-d', 'postgres', '-X', '-A', '-t', '-c', $sql
    ]);
    my %got;
    while (($out // '') =~ /(\w+)=(\d+)/g) {
        $got{$1} = int($2);
    }
    my @bad = grep { !exists $got{$_} || $got{$_} != $expect->{$_} } @required_tables;
    if (@bad) {
        print $TAP "#   FAIL: $label\n";
        print $TAP "#     $err\n" if $err;
        die "perf: $label 失败: "
            . join(', ', map { "$_=" . ($got{$_} // 'missing') . "(期望 $expect->{$_})" } @bad) . "\n";
    }
    print $TAP sprintf "#   OK: %s（%s）\n", $label,
        join(', ', map { "$_=$got{$_}" } @required_tables);
}

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
my $runs = $ENV{PERF_RUNS} // 5;
# 批大小可用 PERF_BATCH_SIZE 覆盖（默认 2000）。不再要求 seed 是
# batch×runs 的整数倍：按 run 尽量均分，批内不足 batch 的末批照常插入。
my $batch_size = $ENV{PERF_BATCH_SIZE} // 2000;
$runs = 1 if $runs < 1;
$batch_size = 1 if $batch_size < 1;
die "perf: PERF_ROWS=$target_rows / PERF_SEED=$seed_max 计算后种子为 $seed 行（需 >= 1）\n"
    if $seed < 1;

# 把 $total 行均分到 $runs 个 run（前 remainder 个 run 多 1 行）。
# 返回数组ref，元素为该 run 的行数；总和恒等于 $total。
sub _split_rows_across_runs {
    my ($total, $nruns) = @_;
    my @parts;
    my $base = int($total / $nruns);
    my $rem = $total % $nruns;
    for my $i (1 .. $nruns) {
        push @parts, $base + ($i <= $rem ? 1 : 0);
    }
    return \@parts;
}

# ============ 1. DDL（通过 JDBC runner）============
print $TAP "\n# === 阶段 1：建表 + KEK/DEK（通过 JDBC） ===\n";
# 1a/1b：localkms 需预生成密钥文件；SoftSDF/真机改走设备索引 hsm:id=N，跳过 ktool。
my $ddl_content;
if ($use_hsm) {
    print $TAP "#   KEK: key_store=swxakms（对称 hsm:id=1，同态 SM2 hsm:id=3，"
        . ($softsdf ? 'SoftSDF' : '真机') . "）\n";
    open(my $dh, '<', "$sql_dir/perf_ddl.sql") or die "Cannot read perf_ddl.sql: $!";
    $ddl_content = do { local $/; <$dh> };
    close $dh;
    # 与 swxa_enc.sql 一致：对称用设备索引 1、HE 非对称用设备索引 3（预置密钥/真机槽位）
    my $n1 = ($ddl_content =~ s/key_store = 'localkms',\s*\n\s*key_path = '%KEKPATH%\/kek_perf\.key'/key_store = 'swxakms',\n    key_path = 'hsm:id=1'/);
    my $n2 = ($ddl_content =~ s/key_store = 'localkms',\s*\n\s*key_algo = 'sm2',\s*\n\s*key_path = '%KEKPATH%\/kek_perf_he\.priv'/key_store = 'swxakms',\n    key_algo = 'sm2',\n    key_path = 'hsm:id=3'/);
    die "perf: 改写 perf_ddl.sql 的 CREATE KEK 失败（对称命中 $n1，HE 命中 $n2）\n"
        unless $n1 == 1 && $n2 == 1;
} else {
    # 1a. 预生成对称密钥（localkms 对称 KEK 需要，CREATE KEK 不再自行生成密钥文件）
    my $ktool = $node->installed_command('tecc_ktool');
    my $sym_key = "$key_dir/kek_perf.key";
    if (!-f $sym_key) {
        print $TAP "#   [tecc_ktool] generate sym key -> $sym_key\n";
        run_command([$ktool, 'generate', '--key-name', 'kek_perf',
                     '--key-algo', 'sm4', '--key-path', $key_dir]);
        die "perf: 生成对称密钥 $sym_key 失败\n" unless -f $sym_key;
    }
    # 1b. 预生成非对称密钥对（同态 KEK 需要）
    my $he_priv = "$key_dir/kek_perf_he.priv";
    if (!-f $he_priv) {
        print $TAP "#   [tecc_ktool] generate asym keypair -> $he_priv\n";
        run_command([$ktool, 'generate', '--key-name', 'kek_perf_he',
                     '--key-algo', 'sm2', '--key-path', $key_dir]);
        die "perf: 生成非对称密钥 $he_priv 失败\n" unless -f $he_priv;
    }
    # 1c. 替换 %KEKPATH% 占位符
    open(my $dh, '<', "$sql_dir/perf_ddl.sql") or die "Cannot read perf_ddl.sql: $!";
    $ddl_content = do { local $/; <$dh> };
    close $dh;
    $ddl_content =~ s/%KEKPATH%/$key_dir/g;
}
my $ddl_tmp = "/tmp/tecc_perf_ddl_$$.sql";
open(my $dfh, '>', $ddl_tmp) or die "Cannot write $ddl_tmp: $!";
print $dfh $ddl_content;
close $dfh;
run_sql_via_jdbc($port, $ddl_tmp, 'perf_ddl.sql');
unlink $ddl_tmp;
check_table_rows('DDL 后所有表存在且为空', { map { $_ => 0 } @required_tables });
print $TAP "# 建表完成\n";

# ============ 2. 数据初始化（通过 JDBC runner 执行 INSERT）============
print $TAP "\n# === 阶段 2：数据初始化（通过 JDBC 加密） ===\n";
my $t_start = gettimeofday();
my $total_encrypts = 0;

# he_int8 的种子规模：JDBC 侧同态客户端加密约 38ms/值（libpq 约 0.94ms/值），
# 按 libpq 的 10 万行种子要跑近 2 小时，故按「用例实际读取的行数」取种子 ——
# he_* 四个用例只有 LIMIT 10000 与 WHERE id <= 10000 两种写法，读的就是前 10000 行；
# 之后照常从 stage 倍增到 target，表规模与 libpq 侧一致。
# 需要与 libpq 完全同口径时用 PERF_HE_ROWS 覆盖（代价见上）。
# 不再要求 PERF_HE_ROWS 是 batch×runs 的整数倍：按 run 尽量均分即可。
my $he_seed_rows = $ENV{PERF_HE_ROWS} // ($batch_size * $runs);
die "perf: PERF_HE_ROWS=$he_seed_rows 必须 >= 1\n" if $he_seed_rows < 1;

my %stage_tables = (
    int4    => { table => 'stage_int4',    cols => 'val_det, val_ope',             encrypts => 2, rows => $seed },
    float8  => { table => 'stage_float8',  cols => 'val_det, val_ope',             encrypts => 2, rows => $seed },
    text    => { table => 'stage_text',    cols => 'val_det, val_ope, val_search', encrypts => 3, rows => $seed },
    he_int8 => { table => 'stage_he_int8', cols => 'val_add, val_mul',             encrypts => 2, rows => $he_seed_rows },
);

$total_encrypts += $stage_tables{$_}{rows} * $stage_tables{$_}{encrypts} for keys %stage_tables;
print $TAP sprintf(
    "#   种子（目标 %d 行，%d 次客户端加密），每表按 %d run 尽量均分：\n",
    $target_rows, $total_encrypts, $runs);
for my $t (sort keys %stage_tables) {
    print $TAP sprintf("#     %-8s %d 行种子 → 倍增到 %d 行\n",
        $t, $stage_tables{$t}{rows}, $target_rows);
}

# 每表每 run 一个 SQL 文件（批大小 = batch_size，末批可不足；行跨 run 连续编号）。
# 同表 runs 个文件一次执行（一次 JVM/连接），报告里每语句耗时按 run 分组求和，
# 供 int4_insert / float8_insert / text_insert / he_int8_insert 取数
# （首 run 预热）。各 run 行数尽量均分，允许相差 1 行 —— 不再强制 seed 对齐。
my %insert_results;
for my $t (sort keys %stage_tables) {
    my $tbl = $stage_tables{$t}{table};
    my $cols = $stage_tables{$t}{cols};
    my $rows = $stage_tables{$t}{rows};
    my $run_parts = _split_rows_across_runs($rows, $runs);

    my @names;
    my @batches_per_run;
    my $global_pos = 1;
    for my $run (1 .. $runs) {
        my $name = "seed_${t}_run${run}.sql";
        push @names, $name;
        my $nrows = $run_parts->[$run - 1];
        my $sqlfile = "$seed_dir/$name";
        open(my $fh, '>', $sqlfile) or die "Cannot write $sqlfile: $!";
        my $left = $nrows;
        my $nbatches = 0;
        while ($left > 0) {
            my $take = $left > $batch_size ? $batch_size : $left;
            my $end = $global_pos + $take - 1;
            print $fh "INSERT INTO $tbl ($cols) VALUES "
                . _seed_values($t, $global_pos, $end) . ";\n";
            $global_pos = $end + 1;
            $left -= $take;
            $nbatches++;
        }
        close $fh;
        push @batches_per_run, $nbatches;
    }

    my $by_file = run_sql_files_in_dir_via_jdbc($port, $seed_dir, \@names, "seed_${t}");
    my @run_times;
    for my $run (1 .. $runs) {
        my $base = _report_base("seed_${t}_run${run}.sql");
        my $run_ms = 0;
        $run_ms += $_->{ms} for @{$by_file->{$base} // []};
        push @run_times, $run_ms;
    }
    unlink "$seed_dir/$_" for @names;

    $insert_results{$t} = build_insert_result(\@run_times);
    print $TAP sprintf("#   种子 %s: 行/run=%s（共 %d 行，%d run），各 run 耗时 %s ms（首 run 预热）\n",
        $t, join('/', @$run_parts), $rows, $runs, join(', ', @run_times));
}

# Phase 2b: 倍增——重复执行共享的 perf_multiply.sql
my $t_seed_done = gettimeofday();
my $seed_elapsed = $t_seed_done - $t_start;

open(my $mh, '<', "$sql_dir/perf_multiply.sql") or die "Cannot read perf_multiply.sql: $!";
my $mult_one_iter = do { local $/; <$mh> };
close $mh;
# 去掉注释行：多轮倍增时注释会重复堆叠，也避免注释里的分号干扰下面的 LIMIT 改写
$mult_one_iter =~ s/--[^\n]*\n//g;
$mult_one_iter =~ s/^\s+|\s+$//g;

my $mult_tmpfile = "/tmp/tecc_perf_mult_$$.sql";

# 拆分共享的 perf_multiply.sql：对称表（enc_int4/enc_int4_b/enc_float8/enc_text）与
# he_int8 分开处理 —— he_int8 的种子行数与对称表不同（见 $he_seed_rows 说明），
# 按同一个「轮数」倍增会凑不出 target 行数，故单独按自己的轮数拼。
my @mult_stmts = grep { /\S/ } split /(?<=;)/, $mult_one_iter;
my @sym_stmts  = grep { !/stage_he_int8/ } @mult_stmts;
my ($he_stmt)  = grep {  /stage_he_int8/ } @mult_stmts;
die "perf: perf_multiply.sql 中未找到 he_int8 的 stage→target 语句\n" unless $he_stmt;

my $he_iters = int($target_rows / $he_seed_rows);
my $he_remainder = $target_rows % $he_seed_rows;

my $mult_sql = '';
if ($target_rows > $seed) {
    my $full_iters = int($target_rows / $seed);
    my $remainder = $target_rows % $seed;

    print $TAP "#   倍增：对称表 ${seed} 行/轮 × ${full_iters} 轮";
    print $TAP " + 余数 ${remainder}" if $remainder;
    print $TAP "；he_int8 ${he_seed_rows} 行/轮 × ${he_iters} 轮";
    print $TAP " + 余数 ${he_remainder}" if $he_remainder;
    print $TAP "\n";

    $mult_sql = join('', @sym_stmts) x $full_iters;
    if ($remainder > 0) {
        $mult_sql .= "INSERT INTO enc_int4 (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_int4 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_int4_b (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_int4 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_float8 (val_det, val_ope) "
            . "SELECT val_det, val_ope FROM stage_float8 LIMIT $remainder;\n";
        $mult_sql .= "INSERT INTO enc_text (val_det, val_ope, val_search) "
            . "SELECT val_det, val_ope, val_search FROM stage_text LIMIT $remainder;\n";
    }
} else {
    # 目标行数不超过种子数，从 stage 直接复制
    print $TAP "#   从 stage 直接复制：对称表每表 LIMIT ${target_rows} 行；"
        . "he_int8 ${he_seed_rows} 行/轮 × ${he_iters} 轮\n";
    $mult_sql = join('', map { my $s = $_; $s =~ s/;/ LIMIT $target_rows;/r } @sym_stmts);
}

$mult_sql .= $he_stmt x $he_iters if $he_iters > 0;
if ($he_remainder > 0) {
    $mult_sql .= "INSERT INTO he_int8 (val_add, val_mul) "
        . "SELECT val_add, val_mul FROM stage_he_int8 LIMIT $he_remainder;\n";
}

open(my $mh2, '>', $mult_tmpfile) or die "Cannot write $mult_tmpfile: $!";
print $mh2 $mult_sql;
close $mh2;

# tecc => 0：关闭客户端 TECC 处理器，让服务端直接搬运密文字节
# （对齐 libpq 侧 init_encrypted_data 用 psql -k off 做倍增的做法）
run_sql_via_jdbc($port, $mult_tmpfile, 'multiply', 0);
unlink $mult_tmpfile;

# 校验最终行数：target 表 = 目标行数，stage 表 = 各自种子行数
# （旧脚本不校验，倍增失败会让用例在空表上"跑得飞快"而全绿）
check_table_rows('初始化后行数', {
    map({ $_ => $target_rows } qw(enc_int4 enc_int4_b enc_float8 enc_text he_int8)),
    map({ $_ => $seed } qw(stage_int4 stage_float8 stage_text)),
    stage_he_int8 => $he_seed_rows,
});

my $t_init_done = gettimeofday();
print $TAP sprintf "#   初始化完成：种子=%.1fs，总计=%.1fs\n",
    $seed_elapsed, $t_init_done - $t_start;

# 输出数据表信息
my $data_dir = $node->data_dir;
my $host_dir = $node->host;
my $conn_info = "psql -h ${host_dir} -p ${port} -U postgres -d postgres";
print $TAP "#\n";
print $TAP "# 预置数据表：\n";
print $TAP "#   enc_int4   (val_det DET, val_ope OPE)                 — ${target_rows} 行\n";
print $TAP "#   enc_float8 (val_det DET, val_ope OPE)                 — ${target_rows} 行\n";
print $TAP "#   enc_text   (val_det DET, val_ope OPE, val_search SEARCH) — ${target_rows} 行\n";
print $TAP "#   he_int8    (val_add he-add, val_mul he-mul)           — ${target_rows} 行\n";
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
my @jfr_args = ();
if (my $jfr_file = $ENV{JFR_FILE}) {
    @jfr_args = ('-XX:StartFlightRecording=filename=' . $jfr_file . ',settings=default,dumponexit=true');
}
my $meas_classpath = _runner_classpath();
my @java_cmd = (
    'java', @jfr_args, '-cp', $meas_classpath,
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', 'postgres', '--password', '',
    '--enable-tecc', '1',
    '--perf-mode', '--perf-runs', $runs, '--perf-warmup', '1',
    '--sql-dir', $sql_dir,
    '--sql-files', join(',', @sql_files)
);
push @java_cmd, ('--tecc-config', $hsm_props) if $use_hsm && $hsm_props;
push @java_cmd, ('--driver-dir', $vendor_dir) if $real_hsm && $vendor_dir;

print $TAP "# 运行：java ... TeccSqlTestRunner --perf-mode\n";
my ($stdout, $stderr) = _run_java(\@java_cmd);

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
my %seen;

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
        my ($name, $runs_ok, $avg, $min, $max) = ($1, $2, $3, $4, $5);
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

        $seen{$name} = 1;
        # req_threshold 必须传：汇总表的「需求阈值/需求达标」列读 $r->{req_threshold}，
        # 漏传会显示 0/N/A（004/libpq 侧有传，005 曾漏，导致 JDBC 报告看不出是否达需求）
        push @results, {
            name         => $name,
            passed       => check_threshold($result, $th),
            avg          => $avg,
            threshold    => $th ? ($th->{threshold_ms} // 0) : 0,
            req_threshold => $th ? ($th->{requirement_threshold_ms} // 0) : 0,
        };
    }
}

# 未产出结果的用例（runner 报错后跳过）显式记为失败：
# 否则"少跑了几个用例"会和"全部通过"长得一模一样。
for my $sql_file (@sql_files) {
    my $name = $sql_file;
    $name =~ s/\.sql$//;
    next if $seen{$name};
    my $th = $thresholds->{$name};
    print $TAP "not ok - $name 未产出测量结果（见上方 [java-stderr]）\n";
    push @results, {
        name         => $name,
        passed       => 0,
        avg          => 0,
        threshold    => $th ? ($th->{threshold_ms} // 0) : 0,
        req_threshold => $th ? ($th->{requirement_threshold_ms} // 0) : 0,
    };
}

# ============ 6b. INSERT 用例（seed 阶段顺带测量）============
# 与 libpq 侧（004）同一组用例名，数据源为上面的 seed 测量，不额外造数。
my @insert_cases = (
    [ 'int4_insert',    'int4'    ],
    [ 'float8_insert',  'float8'  ],
    [ 'text_insert',    'text'    ],
    [ 'he_int8_insert', 'he_int8' ],
);
for my $case (@insert_cases) {
    my ($name, $t) = @$case;
    my $res = $insert_results{$t} // next;
    my $th = $thresholds->{$name};
    print_perf_result($name, $res, $th);
    save_perf_result($module_dir, $name, $res, $th, 'jdbc');
    $seen{$name} = 1;
    push @results, {
        name         => $name,
        passed       => check_threshold($res, $th),
        avg          => $res->{avg_ms},
        threshold    => $th ? ($th->{threshold_ms} // 0) : 0,
        req_threshold => $th ? ($th->{requirement_threshold_ms} // 0) : 0,
    };
}

# ============ 7. 汇总 ============
print $TAP "\n";
print_perf_table(\@results);
print_perf_summary(\@results);
# ReqID:TECC4.0 - 报告头写明加密后端：$softsdf / $real_hsm 为本轮入口判定
# （设 TECC_SDF_LIB → SoftSDF；TECC_PERF_HSM=1 → 真机；否则 localkms）
my $summary_file = save_perf_summary($module_dir, \@results, {
    driver    => 'jdbc',
    crypto    => perf_crypto_backend(),
    key_store => $use_hsm ? 'swxakms' : 'localkms',
});
my $plan_count = scalar @results;
print $TAP "1..${plan_count}\n";

# ============ 8. 直接退出跳过析构，下次运行时 make check 清理旧数据 ============
POSIX::_exit(0);
