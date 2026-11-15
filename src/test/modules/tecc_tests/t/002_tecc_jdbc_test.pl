#!/usr/bin/perl

# 002_tecc_jdbc_test.pl
#
# TECC JDBC 全链路回归测试
#
# 范式说明（与 libpq 侧 001 不同）：
#   JDBC 侧只跑**一个范式** —— TeccSqlTestRunner 的 psql 输出格式，经
#   normalize_jdbc_runner_output + normalize_common_output 归一化后，与 JDBC 自有的
#   shared/expected_jdbc/<case>.out 比较。
#
#   为什么不与 libpq 共用 expected_common：两驱动的输出存在**固有**格式差异
#   （布尔 true/false vs t/f、浮点 1.0E10 vs 1e+10、psql 行数页脚、命令 tag 文本、
#   NOTICE 流位置等），强行共用归一化期望只会同时削弱两侧的比较强度。因此
#   JDBC 与 libpq 只共享**用例**（shared/sql/），各自维护期望文件；两侧同一用例的
#   输出差异逐条登记在 shared/jdbc_vs_libpq_diff.md 中备查。

use strict;
use warnings;
use Config;
use Cwd;
use File::Basename;
use Test::More;
use PostgresNode;
use TestLib;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;
use tecc_jdbc_jar;

my $jdbc_home = $ENV{'TECC_JDBC_HOME'} || '';
if (!$jdbc_home) {
    plan skip_all => "TECC_JDBC_HOME not set, skipping JDBC tests";
}

# ReqID:TECC4.0 - 驱动 JAR 必须按版本选（build/libs 会累积历次构建产物），
# 选取规则见 t/tecc_jdbc_jar.pm —— 不能再用 glob 的字典序结果。
my @jars = jdbc_jar_candidates($jdbc_home);
if (!@jars) {
    plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first";
}

my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
if (!-d $test_classes) {
    plan skip_all => "JDBC test classes not found; run './gradlew :postgresql:compileTestJava' first";
}

my $module_dir = "$FindBin::Bin/..";
my $filter = get_tecc_test_filter();

note("");
note("╔═════════════════════════════╗");
note("║  TECC JDBC  全链路回归测试  ║");
note("╚═════════════════════════════╝");
note("");
note("Starting PostgreSQL node for TECC JDBC tests...");
my $node = get_new_node('tecc_jdbc');
# ReqID:TECC4.0 - 强制消息语言为英文，保留 UTF-8 排序与字符集(说明同 001 脚本)。
delete $ENV{LC_ALL};                # LC_ALL 优先级最高，必须先删，否则覆盖下面单独设置
$ENV{LC_CTYPE}    = 'zh_CN.UTF-8';  # 字符分类/编码(中文数据)
$ENV{LC_COLLATE}  = 'zh_CN.UTF-8';  # 排序规则保持原 zh_CN.UTF-8
$ENV{LC_MESSAGES} = 'C';            # 报错/提示消息固定英文
$ENV{LANG}        = 'C';            # 兜底(已被上面显式设置覆盖)
# ReqID:TECC4.0 - 解析 SDF 模式（SoftSDF 默认 / TECC_SDF_LIB=off 走真机），
# 必须排在下面 JCE 门面与厂商目录的取舍之前（那两处按模式二选一）。
init_sdf_mode($module_dir);

# ReqID:TECC4.0 - SoftSDF 模式下的 JDBC 侧 JCE 门面（详见 tecc_kms/jce/README.md）：
# 驱动按全限定名反射加载 com.sansec.jce.provider.SwxaProvider，本 JAR 提供同名类与
# 同算法集顶替真实厂商 JAR，后端读 SoftSDF 的 kek-<index>.key / sm2enc-<index>.pem。
# 在 initdb 之前校验，避免跑完一轮才发现环境不全。
# 非 SoftSDF 模式（真实加密机）**不得**把本 JAR 放进 classpath，否则会遮蔽真厂商实现。
my $jce_jar;
if (sdf_soft_mode()) {
    $jce_jar = $ENV{TECC_SDF_JCE_JAR} || "$module_dir/../../../../tecc_kms/bin/libteccjce.jar";
    if (!-f $jce_jar) {
        die "SoftSDF JCE facade not found: $jce_jar\n"
          . "Build it with: make -C src/test/modules/tecc_tests tecc-sdf-jce-jar\n";
    }
}

# ReqID:TECC4.0 - 真机模式（TECC_SDF_LIB=off）下，把厂商 HSM 的 JAR 目录交给 runner。
# 驱动用 HsmVendorClassLoader 按"非驱动 .jar"扫 classpath 与其线程上下文类加载器来找
# JCE Provider 实现（SWXA 的 com.sansec.jce.provider.SwxaProvider、TZWY 的 com.tzwy.*）；
# 厂商 JAR 不在场时会加载到驱动 JAR 内的空壳 stub 并由驱动抛错（"vendor JAR not found"）。
# 注意：厂商身份由 KEK 的 key_store 决定、**不是**由目录区分，故不同厂商的 JAR 可以放在同一目录。
#
# 默认目录取 $TECC_JDBC_HOME/pgjdbc/libs，与 runner 用法示例里的 `--driver-dir pgjdbc/libs`
# 及 pgjdbc build.gradle.kts 里 hsmVendorDir 的兜底目录一致。厂商 JAR 不入版本库
# （见 build.gradle.kts "Do not commit vendor binaries to git"），放在仓库外时用
# TECC_HSM_VENDOR_DIR 指向该目录（这是"目录不在默认位置"的覆盖项，不是模式开关）。
#
# SoftSDF 模式**不传**：门面 JAR 与厂商 JAR 同名类，同时在场时谁先被扫到不确定。
my $vendor_dir;
if (!sdf_soft_mode()) {
    $vendor_dir = $ENV{TECC_HSM_VENDOR_DIR} || "$jdbc_home/pgjdbc/libs";
    $vendor_dir = undef unless -d $vendor_dir;
}

$node->init;
setup_swsds_ini($node, $module_dir);
$node->append_conf('postgresql.conf', "listen_addresses = 'localhost'\n");
$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
$node->start;

my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;
# ReqID:TECC4.0 - 预生成 localkms KEK 文件（CREATE KEK 不生成文件，
# 密钥文件须由 tecc_ktool generate 提供；见 Review 问题10）。
# 第三个参数=1（JDBC 路径专用）：runner 只实现 \c / \copy，SQL 里自带的
# `\! tecc_ktool generate` 会被静默丢弃，故自带生成的用例也必须由框架预生成。
pregen_localkms_kek_files($module_dir, $key_dir, 1);

# runner 的工作目录：SQL 中的相对路径（server COPY / \copy / \i 等）与 libpq 侧
# 保持一致的可写位置（libpq 侧同样以 tmp_check/tmp 为 CWD）。
my $tmp_dir = "$TestLib::tmp_check/tmp";
mkdir $tmp_dir unless -d $tmp_dir;
# ReqID:TECC4.0 - 兜底清除源码目录里上一次运行遗留的用例产物（见 tecc_test_utils.pm）
cleanup_stray_test_artifacts();

my ($setup_rc, $setup_out, $setup_err) = $node->psql('postgres',
    "CREATE ROLE postgres LOGIN SUPERUSER;");

my $port = $node->port;
my $TAP = Test::More->builder->output;

# ============ 共享用例 ============
note("=== shared用例JDBC执行 + 归一化输出比对 ===");

my @shared_sql_files;
if ($filter) {
    @shared_sql_files = ("$module_dir/shared/sql/$filter");
} else {
    opendir(my $dh, "$module_dir/shared/sql") or die "Cannot open shared/sql: $!";
    @shared_sql_files = sort map { "$module_dir/shared/sql/$_" } grep { /\.sql$/ } readdir($dh);
    closedir($dh);
}

# ReqID:TECC4.0 - 读清单之后、执行之前剔除本轮不该执行的用例，并打印一行说明：
#   ① 硬加密用例（密码机不可用时；见 tecc_test_utils.pm 的 %HSM_CASE）
#   ② JDBC 测试程序跑不了的用例（框架/工具/已登记缺陷原因；见 %JDBC_UNSUPPORTED_CASE，
#      例如 reconnect_schema_dek.sql 被 runner 拆句误判注释里的 \c）
@shared_sql_files = exclude_hsm_cases(@shared_sql_files);
@shared_sql_files = exclude_jdbc_unsupported_cases(@shared_sql_files);

# ReqID:TECC4.0 - java.util.logging 默认格式带本地化时间戳（每次运行都不同），
# 统一改为 `level: message`，使 JDBC 输出可以做 golden file 比较。
my $jdbc_jar = $jars[0];
# 实际选用的 JAR 必须打印：旧版本留在 build/libs 里时，混搭运行的表象
# （invalid CipherDataType structure、后端崩溃）极易被误判为用例本身的缺陷。
print $TAP "# 驱动 JAR: " . jdbc_jar_selection_note(\@jars) . "\n";
# SoftSDF 模式把 JCE 门面放在 classpath 首位（HsmVendorClassLoader 扫 classpath 时按序取
# 非驱动 JAR；放首位同时避免任何按名直接加载的场景命中驱动 JAR 内的 stub）。
# ReqID:TECC4.0 - 真机模式同理必须把厂商 JAR 放到 **classpath** 上：驱动侧
# HsmVendorClassLoader 是按 java.class.path 快照扫 vendor JAR 的，只给 --driver-dir 不够
# ——那只是把 JAR 挂到上下文类加载器，而 TCCL 加载是 parent-first，父加载器里 pgjdbc
# shadow JAR 的同名 stub 会先命中（表现为 "provider loaded from pgjdbc stub"）。
my $vendor_cp = defined $vendor_dir ? "${vendor_dir}/*:" : '';
my $classpath = $jce_jar
    ? "${jce_jar}:${test_classes}:${jdbc_jar}"
    : "${vendor_cp}${test_classes}:${jdbc_jar}";
my $runner_base = ['java',
    '-Djava.util.logging.SimpleFormatter.format=%4$s: %5$s%n',
    '-cp', $classpath,
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', 'postgres', '--password', '',
    '--enable-tecc', '1', '--modes', 'simple',
    '--key-dir', $key_dir,
    '--output-format', 'psql'];

# ReqID:TECC4.0 - HSM KEK（key_store='swxakms'）要求 tecc_config 指向 HSM 连接配置文件，
# 否则驱动建 KEK 时报 "failed to load HSM connection config"——真机模式下驱动会退回
# runner 内置的 Windows 默认路径（E:/newWorkSpace/jdbc/hsm.properties），必然失败。
# 该文件（hsm.properties）由 setup_swsds_ini 在**软模式与真机模式都**生成到
# tmp_check/tmp，故这里只看路径有没有，不绑定 sdf_soft_mode()。
if ($tecc_test_utils::hsm_properties_path) {
    push @$runner_base, ('--tecc-config', $tecc_test_utils::hsm_properties_path);
}

# 真机模式：把厂商 JAR 目录挂到 runner（详见脚本上方的说明）。
if (defined $vendor_dir) {
    push @$runner_base, ('--driver-dir', $vendor_dir);
    print $TAP "# 真机模式: 厂商 JAR 目录 -> $vendor_dir\n";
} elsif (!sdf_soft_mode()) {
    # 到这一步说明既没设 TECC_HSM_VENDOR_DIR、默认目录也不存在，HSM 用例会因加载到
    # 驱动内的 stub 而失败，提前把原因打出来
    print $TAP "# 真机模式: 未找到厂商 JAR 目录（TECC_HSM_VENDOR_DIR 未设、"
        . "$jdbc_home/pgjdbc/libs 不存在）—— HSM 用例会因加载到驱动内 stub 而失败\n";
}

my $tp_passed = 0;
my $tp_total = 0;

foreach my $sql_file (@shared_sql_files) {
    my $base = (split '/', $sql_file)[-1];
    $base =~ s/\.sql$//;
    next unless -f $sql_file;

    $tp_total++;
    my $test_name = "jdbc: ${base}";
    my $expected_file = "$module_dir/shared/expected_jdbc/${base}.out";
    $tecc_test_utils::test_sql_label{$base} = "shared/sql/${base}.sql";

    # ---- 与 libpq 侧一致的可移植化：%PGDATA% / %KEKPATH% 占位符替换 ----
    # 分段执行时以同名 <base>.sql 落到 tmp_check/tmp，使 runner 回显的文件名
    # 与 psql 侧一致（libpq 侧写 _pp_<base>.sql 后再改名回显）。
    my $pp_sql = slurp_file($sql_file);
    my $pgdata = $node->data_dir;
    $pp_sql =~ s/%PGDATA%/$pgdata/g if defined $pp_sql;
    $pp_sql =~ s/%KEKPATH%/$key_dir/g if defined $pp_sql;

    # ---- 执行（CWD = tmp_dir，与 libpq 侧一致）----
    # SQL 中的 \! shell 命令由框架代 runner 执行，见 run_jdbc_case_with_shell。
    my $saved_cwd = getcwd();
    # Begin - ReqID:TECC4.0
    # 切目录失败必须硬失败，否则相对路径产物会写进源码目录（同 libpq 侧防护）
    chdir($tmp_dir)
        or BAIL_OUT("无法切换到回归临时目录 \"$tmp_dir\": $!");
    # End - ReqID:TECC4.0
    my ($actual, $raw_all) = run_jdbc_case_with_shell($runner_base, $tmp_dir, $base, $pp_sql, $node);
    chdir($saved_cwd);

    # Begin - ReqID:TECC4.0
    my @stray = find_stray_test_artifacts();
    BAIL_OUT(
        "用例 ${base}.sql 的相对路径产物落到了源码目录 "
            . "(" . dirname($TestLib::tmp_check) . ") 而非 $tmp_dir: "
            . join(', ', map { basename($_) } @stray)
            . "\nrunner 工作目录未生效，请检查 002 的 chdir 逻辑")
        if @stray;
    # End - ReqID:TECC4.0

    # 抽取结果为空说明 runner 没有按 `>>> <file>` 分段输出（例如提前报错退出），
    # 或内容被归一化规则整体吃掉；两种情况都把原始输出落盘，便于定位
    # （否则只有"空 == 空"的假象）。
    $actual = normalize_common_output(normalize_jdbc_runner_output($actual), $base);

    if ($actual !~ /\S/) {
        my $outdir = "$TestLib::tmp_check/results";
        mkdir $outdir unless -d $outdir;
        my $rawfile = "$outdir/jdbc_${base}.raw";
        open(my $rf, '>', $rawfile) or warn "Cannot write $rawfile: $!";
        print $rf $raw_all if $rf;
        close $rf if $rf;
        print $TAP "# [诊断] $test_name 归一化后无内容，runner 原始输出见 jdbc_${base}.raw\n";
    }

    my $expected = '';
    $expected = slurp_file($expected_file) if -f $expected_file;
    $expected =~ s/\r\n/\n/g;
    $expected = normalize_common_output(normalize_jdbc_runner_output($expected), $base);

    # 缺少期望文件必须显式失败：否则 expected 为空串，会把"抽取不到任何输出"
    # （例如 runner 未按 `>>> <file>` 分段）误判为通过。
    my $missing_expected = !-f $expected_file;

    if ($missing_expected || $actual ne $expected) {
        my $outdir = "$TestLib::tmp_check/results";
        mkdir $outdir unless -d $outdir;
        my $outfile = "$outdir/jdbc_${base}.out";
        open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
        print $fh $actual if $fh;
        close $fh if $fh;
        my $diff = `diff -u "$expected_file" "$outfile" 2>/dev/null || true`;
        print $TAP "not ok - $test_name ... FAILED\n";
        print $TAP "#       缺少期望文件: $expected_file\n" if $missing_expected;
        print $TAP "#       实际输出: $outfile\n";
        print $TAP "#       差异对比: $diff\n" if $diff;
    } else {
        print $TAP "ok - $test_name ... ok\n";
        $tp_passed++;
    }
}

$node->stop('fast');

my $t_pass = $tp_passed;
my $t_total = $tp_total;
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

# ReqID:TECC4.0 - 一个用例都没跑时，把原因打进报告（裸 `1..0` 只会显示
# "skipped: (no reason given)"，看不出是被跳过还是 TECC_TEST 没匹配上）
if ($t_total == 0) {
    my $reason = empty_run_reason($filter);
    print $TAP "# $reason\n";
    print $TAP "1..0 # SKIP $reason\n";
}
else {
    print $TAP "1..$t_total\n";
}

# ============ 用例执行辅助 ============

# 按 psql 的 \! 行分段执行 SQL。
#
# 为什么需要：TeccSqlTestRunner 只实现 `\c ` 和 `\copy ` 两个元命令
# （TeccSqlTestRunner.java:2415），其余元命令静默丢弃。而 \! 执行的是 shell 命令
# （tecc_ktool generate 生成密钥、mv 移走私钥等），对用例语义是必需的——
# 例如 he_kek_privkey.sql 用 `\! mv .../kek_priv.priv .../kek_priv.priv.bak`
# 模拟"客户端拿不到 KEK 私钥"的场景。缺了它，该场景在 JDBC 侧变成了"私钥可用"，
# 用例覆盖点丢失。故由框架代 runner 执行 \!：逐段调用 runner，段间执行 shell 命令，
# 输出按原顺序拼接。
#
# 代价：每段一次 JVM 启动；段之间不保留会话级状态（SET/临时表/GUC）。仅含 \!
# 的用例（he_acl / he_kek_privkey / homomorphic_enc / swxa_concurrent）走多段，
# 这些用例的 \! 之间没有会话状态依赖。
#
# 两处必须与 psql 的 \! 语义对齐，否则用例会静默失去覆盖：
#   1) 环境：psql 的 \! 子进程只继承环境，不继承 psql 命令行的 -h/-p；而该节点用
#      自定义端口，故必须把节点的 PGHOST/PGPORT 注入（与 $node->psql 口径一致）。
#      否则 swxa_concurrent 里 `\! psql ...` 起的后台进程会连默认 5432 端口而失败，
#      用例看起来"通过"实则后台进程从未运行。
#   2) stdout：psql 把 \! 命令的 stdout 直接写进自己的 stdout（与 SQL 结果按序交错），
#      故框架代执行时要把 shell 段 stdout 按序拼回用例输出。除 swxa_concurrent 的
#      `\! cat conc_bg.out` 外，本仓库其余 \! 命令均静默或已重定向，故不受影响。
sub run_jdbc_case_with_shell {
    my ($runner_base, $tmp_dir, $base, $sql_text, $node) = @_;

    return '' unless defined $sql_text;

    # 换行符规范化（与 libpq 侧 run_case_sql_via_psql 同一原因）：Windows 检出下 .sql 是 CRLF，
    # 框架代执行 \! 时行尾 CR 会进入 shell 命令（`2>/dev/null\r` 等），导致密钥生成失败并级联。
    $sql_text =~ s/\r\n/\n/g;

    # 切段：\! 行独立成"shell 段"，其余按序为"SQL 段"
    my @parts;
    my $cur = '';
    for my $line (split /\n/, $sql_text, -1) {
        if ($line =~ /^\\!\s*(.*)$/) {
            push @parts, { sql => $cur };
            $cur = '';
            push @parts, { shell => $1 };
        } else {
            $cur .= $line . "\n";
        }
    }
    push @parts, { sql => $cur };

    my $out = '';
    my $raw_all = '';
    for my $part (@parts) {
        if (defined $part->{shell}) {
            # psql 的 \! 经 shell 执行，继承 psql 的环境与 CWD
            # （TECC_KEK_PATH 已由本脚本设置为 $key_dir；PGHOST/PGPORT 见上方说明）。
            my $sh_out;
            {
                local %ENV = (%ENV,
                    PGHOST => $node->host,
                    PGPORT => $node->port);
                ($sh_out) = run_command(['/bin/sh', '-c', $part->{shell}]);
            }
            $sh_out = '' unless defined $sh_out;
            $sh_out =~ s/\r\n/\n/g;
            if ($sh_out ne '') {
                # run_command 会 chomp，补回换行以保持与 psql 输出同样的行结构
                $out .= $sh_out . "\n";
                $raw_all .= $sh_out . "\n";
            }
            next;
        }

        my $seg = $part->{sql} // '';
        # 去掉纯注释/空行后若无可执行内容则跳过（避免 runner 空跑一次）
        (my $effective = $seg) =~ s/^\s*--[^\n]*$//gm;
        next unless $effective =~ /\S/;

        my $seg_file = "$tmp_dir/${base}.sql";
        open(my $fh, '>', $seg_file) or warn "Cannot write $seg_file: $!";
        print $fh $seg if $fh;
        close $fh if $fh;

        my ($stdout, $stderr) = run_command([@$runner_base, '--sql-file', $seg_file]);
        my $raw = $stdout // '';
        $raw .= "\n" . $stderr if defined $stderr && $stderr ne '';
        $raw =~ s/\r\n/\n/g;

        $raw_all .= $raw;
        $out .= extract_jdbc_case_section($raw, $base);
    }

    return ($out, $raw_all);
}

# 抽取 runner 输出中本用例的段落（runner 以 `>>> <file>` 分隔每个 SQL 文件）。
sub extract_jdbc_case_section {
    my ($raw, $base) = @_;

    my $out = '';
    my $in_file = 0;
    for my $line (split /\n/, $raw) {
        if ($line =~ /^>>>\s+(.+)$/) {
            my $marker = $1;
            $marker =~ s/^执行文件:\s*//;
            $in_file = ($marker eq "${base}.sql" || $marker eq $base);
            next;
        }
        $out .= $line . "\n" if $in_file;
    }
    return $out;
}

