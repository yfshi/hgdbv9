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
use Cwd;
use Test::More;
use PostgresNode;
use TestLib;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;
use tecc_jdbc_jar;

my $module_dir = "$FindBin::Bin/..";
my $sql_dir     = "$module_dir/cross/sql";
my $exp_dir     = "$module_dir/cross/expected";

# TAP 输出句柄（TestLib 重定向了 STDOUT，诊断信息必须走这里）
my $TAP = Test::More->builder->output;

# 校验 JDBC 环境
my $jdbc_home = $ENV{'TECC_JDBC_HOME'} or plan skip_all => "TECC_JDBC_HOME not set, required for cross test";
# ReqID:TECC4.0 - 驱动 JAR 必须按版本选（build/libs 会累积历次构建产物），
# 选取规则见 t/tecc_jdbc_jar.pm —— 不能再用 glob 的字典序结果。
my @jars = jdbc_jar_candidates($jdbc_home);
plan skip_all => "JDBC jar not found; run 'make jdbc-jar' first" unless @jars;
my $test_classes = "$jdbc_home/pgjdbc/build/classes/java/test";
plan skip_all => "JDBC test classes not found; run 'make jdbc-compile' first" unless -d $test_classes;
my $jdbc_jar = $jars[0];

# ReqID:TECC4.0 - 解析 SDF 模式（SoftSDF 默认 / TECC_SDF_LIB=off 走真机），
# 必须排在下面 JCE 门面与厂商目录的取舍之前（那两处按模式二选一）。
init_sdf_mode($module_dir);

# ReqID:TECC4.0 - 硬加密（HSM）用例的接线，与 002 保持一致：
#   ① SoftSDF 模式：JCE 门面 JAR 放 classpath 首位（驱动按全限定名反射加载
#      com.sansec.jce.provider.SwxaProvider，门面提供同名类与同算法集）；
#   ② 真机模式：--driver-dir 指向厂商 JAR 目录（HSM 真机上 002 用同一做法）。
# 见 tecc_kms/jce/README.md 与 AGENTS.md「HSM / SoftSDF 回归」。
my $jce_jar;
if (sdf_soft_mode()) {
    $jce_jar = $ENV{TECC_SDF_JCE_JAR} || "$module_dir/../../../../tecc_kms/bin/libteccjce.jar";
    if (!-f $jce_jar) {
        die "SoftSDF JCE facade not found: $jce_jar\n"
          . "Build it with: make -C src/test/modules/tecc_tests tecc-sdf-jce-jar\n";
    }
}

my $vendor_dir;
if (!sdf_soft_mode()) {
    $vendor_dir = $ENV{TECC_HSM_VENDOR_DIR} || "$jdbc_home/pgjdbc/libs";
    $vendor_dir = undef unless -d $vendor_dir;
}

# 启动PG节点（两个驱动共享同一个实例）
note("驱动 JAR: " . jdbc_jar_selection_note(\@jars));
note("Starting PostgreSQL node for cross-driver tests...");
my $node = get_new_node('tecc_cross');
# ReqID:TECC4.0 - 强制消息语言为英文，保留 UTF-8 排序与字符集(说明同 001 脚本)。
delete $ENV{LC_ALL};                # LC_ALL 优先级最高，必须先删，否则覆盖下面单独设置
$ENV{LC_CTYPE}    = 'zh_CN.UTF-8';  # 字符分类/编码(中文数据)
$ENV{LC_COLLATE}  = 'zh_CN.UTF-8';  # 排序规则保持原 zh_CN.UTF-8
$ENV{LC_MESSAGES} = 'C';            # 报错/提示消息固定英文
$ENV{LANG}        = 'C';            # 兜底(已被上面显式设置覆盖)
$node->init;
setup_swsds_ini($node, $module_dir);
$node->append_conf('postgresql.conf', "listen_addresses = 'localhost'\n");
$node->append_conf('pg_hba.conf', "host all all 127.0.0.1/32 trust\n");
$node->start;
my $key_dir = "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;
# ReqID:TECC4.0 - localkms KEK 文件预生成（说明同 002）：CREATE KEK 不再自行写盘，
# 密钥文件须由 tecc_ktool 提供，否则 libpq 侧直接 `ERROR: key file not found:
# <name>.key` + `Failed to setup local KEK`，后续 DEK/表创建全线失败。
# 第三个参数=1：JDBC 步由 runner 执行（runner 只实现 \c/\copy，SQL 里的
# `\! tecc_ktool generate` 会被丢弃），故自带生成的用例同样必须预生成。
pregen_localkms_kek_files($module_dir, $key_dir, 1);

my $connstr = $node->connstr('postgres');
my $port = $node->port;

# ReqID:TECC4.0 - 硬加密路径的执行工作目录（说明同 002）：SoftSDF 动态库与 JCE 门面
# 都按**相对 CWD** 读 kek-<index>.key / sm2enc-<index>.pem，setup_swsds_ini 已把预置
# 密钥复制到 tmp_check/tmp。两个驱动的 psql/JVM 子进程都从这里启动；本用例的
# SQL 与期望文件全部用绝对路径访问，故切目录不影响其余行为。
my $tmp_dir = "$TestLib::tmp_check/tmp";
mkdir $tmp_dir unless -d $tmp_dir;
# ReqID:TECC4.0 - 兜底清除源码目录里上一次运行遗留的用例产物（见 tecc_test_utils.pm）
cleanup_stray_test_artifacts();

# ReqID:TECC4.0 - 读清单之后、执行之前剔除硬加密用例（密码机不可用时；名单见
# tecc_test_utils.pm 的 %HSM_CASE），故用例数与下面的计划数都随之收缩。
opendir(my $dh, $sql_dir) or plan skip_all => "cross/sql not found";
my @sql_files = sort grep { /^\d+.*\.sql$/ } readdir($dh);
closedir($dh);
@sql_files = exclude_hsm_cases(@sql_files);

my $n = scalar @sql_files;
if ($n == 0) {
    # ReqID:TECC4.0 - 用例被剔空时把原因写进报告（plan skip_all 会让报告显示该原因）
    plan skip_all => empty_run_reason('');
}

# 总用例数 = 正向 n 个 + 反向 n 个
plan tests => $n * 2;
note("cross cases: " . join(', ', @sql_files));

# TeccSqlTestRunner 基础参数（使用当前 OS 用户连接）
my $os_user = $ENV{USER} || $ENV{LOGNAME} || 'postgres';
# 真机模式必须把厂商 JAR 放到 **classpath** 上（理由同 002：驱动侧 HsmVendorClassLoader
# 按 java.class.path 快照扫 vendor JAR，--driver-dir 只挂 TCCL 而 TCCL 是 parent-first）。
my $vendor_cp = defined $vendor_dir ? "${vendor_dir}/*:" : '';
my $classpath = $jce_jar
    ? "${jce_jar}:${test_classes}:${jdbc_jar}"
    : "${vendor_cp}${test_classes}:${jdbc_jar}";
my $runner_base = ['java', '-cp', $classpath,
    'org.postgresql.test.clientlogic.tools.TeccSqlTestRunner',
    '--url', "jdbc:postgresql://127.0.0.1:${port}/postgres",
    '--user', $os_user, '--password', '',
    '--enable-tecc', '1', '--modes', 'simple'];

# ReqID:TECC4.0 - 硬加密 KEK（key_store='swxakms'）要求 tecc_config 指向 HSM 连接
# 配置文件，否则驱动建 KEK 时报 "failed to load HSM connection config"（真机模式下
# 会退回 runner 内置的 Windows 默认路径）。该文件由 setup_swsds_ini 在软模式与真机
# 模式都生成到 tmp_check/tmp，故条件只看路径有无（说明同 002）。
if ($tecc_test_utils::hsm_properties_path) {
    push @$runner_base, ('--tecc-config', $tecc_test_utils::hsm_properties_path);
}

if (defined $vendor_dir) {
    push @$runner_base, ('--driver-dir', $vendor_dir);
    note("真机模式: 厂商 JAR 目录 -> $vendor_dir");
} elsif (!sdf_soft_mode()) {
    note("真机模式: 未找到厂商 JAR 目录（TECC_HSM_VENDOR_DIR 未设、"
        . "$jdbc_home/pgjdbc/libs 不存在）—— HSM 用例会因加载到驱动内 stub 而失败");
}

# 驱动执行子过程
sub run_step {
    my ($sql_file, $exp_file, $label, $driver) = @_;

    # ReqID:TECC4.0 - 与 001/002 一致的用例可移植化：%PGDATA% / %KEKPATH% 占位符
    # 在执行前替换（说明同共享用例：asym KEK 建 KEK 时直接 stat key_path，不走
    # TECC_KEK_PATH 覆盖，必须替换成 tmp_check/keys 绝对路径）。
    # 只对含占位符的用例写临时副本，其余仍直接用原路径——临时文件名会出现在
    # psql 的 `psql:<file>:<line>` 前缀里，既有用例的输出不能受影响。
    my $sql_path = $sql_file;
    my $sql_content = slurp_file($sql_file);
    if (defined $sql_content && $sql_content =~ /%PGDATA%|%KEKPATH%/) {
        $sql_content =~ s/%PGDATA%/$node->data_dir/g;
        $sql_content =~ s/%KEKPATH%/$key_dir/g;
        (my $pp_base = $sql_file) =~ s{.*/}{};
        $sql_path = "$tmp_dir/$pp_base";
        open(my $pp_fh, '>', $sql_path) or die "Cannot write $sql_path: $!";
        print $pp_fh $sql_content;
        close($pp_fh);
    }

    # 两个驱动的子进程都在 tmp_check/tmp 下执行（SoftSDF 按相对 CWD 取预置密钥）
    my $saved_cwd = getcwd();
    chdir($tmp_dir) or die "chdir $tmp_dir: $!";

    my $output;
    if ($driver eq 'jdbc') {
        # JDBC 执行：通过 TeccSqlTestRunner 以 psql 兼容格式输出
        my $cmd = [@$runner_base, '--output-format', 'psql', '--sql-file', $sql_path];
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
            $output = `$psql_bin -X -A -t -d '$connstr' -f '$sql_path' 2>&1`;
            $output //= '';
        }
    }

    chdir($saved_cwd) or die "chdir $saved_cwd: $!";

    $output =~ s/\r\n/\n/g;
    # JDBC 步先剔除 runner/JUL 工具噪音（说明同 002）：java.util.logging 表头 + 类名
    # （带本地化时间戳）、报告目录提示、HsmVendorClassLoader 的 INFO 行等，都是工具输出
    # 且随环境变化，不能进跨驱动期望。libpq 侧无这类输出，故只对 JDBC 侧应用。
    $output = normalize_jdbc_runner_output($output) if $driver eq 'jdbc';
    $output = normalize_common_output($output);

    my $expected = '';
    $expected = slurp_file($exp_file) if -f $exp_file;
    $expected =~ s/\r\n/\n/g;
    $expected = normalize_common_output($expected);

    # ReqID:TECC4.0 - bpchar 定长列的**尾部空格**是驱动呈现差异：JDBC 驱动按声明长度补齐
    # （`BPCHAR_ENC(50)` 的 'CROSS_BP' 输出为 'CROSS_BP' + 42 空格），libpq 侧原样输出。
    # 该差异属已登记的固有格式差异（shared/jdbc_vs_libpq_diff.md 的 bpchar 空格语义一族，
    # 共享用例因此各自维护 expected_libpq / expected_jdbc 两套期望）。跨驱动套件只有一份
    # 期望，故在本脚本内把补齐用的连续空格（≥2）归一化掉；**单个尾部空格保留** ——
    # shared/sql/bpchar_trailing_space.sql 正是用它验证比较语义的。
    # 注意：规则只在本脚本生效，不影响共享用例两套期望里记录的补齐形态。
    $output   =~ s/ {2,}(?=\||$)//mg;
    $expected =~ s/ {2,}(?=\||$)//mg;

    my $ok = is($output, $expected, $label);

    # ReqID:TECC4.0 - 失败时把归一化后的实际输出落盘：本脚本不做 run_sql_dir_diff，
    # 原先失败只留下 "not ok"，无法取证（tmp_check 下次运行即删）。落盘文件名带驱动名，
    # 便于直接对比"同一用例在两个驱动下的输出是否一致"（跨驱动一致正是本套件的前提，
    # 期望文件必须同时满足正/反两轮的两个驱动）。
    if (!$ok) {
        my $outdir = "$TestLib::tmp_check/results";
        mkdir $outdir unless -d $outdir;
        (my $base = $exp_file) =~ s{.*/}{};
        $base =~ s/\.out$//;
        my $outfile = "$outdir/cross_${base}.${driver}.out";
        open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
        print $fh $output if $fh;
        close $fh if $fh;
        print $TAP "#       实际输出($driver): $outfile\n";
    }
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

$node->stop('fast');

done_testing();

print_failure_summary();
