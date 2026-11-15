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
#
# 硬加密模式（ReqID:TECC4.0）——与功能回归（001/002/003）和 JDBC perf（005）同一套语义：
#   TECC_SDF_LIB=<path>                    → SoftSDF（软件密码机），KEK=swxakms
#   TECC_SDF_LIB=off（同 real/none/no/0）   → 真实加密机（同 001/002/003）
#   TECC_PERF_HSM=1（且 TECC_SDF_LIB 未设） → 真实加密机（同 005）
#   其余（默认）                            → localkms，既有基线口径不变
# 硬加密下：perf_ddl.sql 的两把 KEK 由本脚本改写为 key_store='swxakms'
# （对称 hsm:id=1、同态 SM2 hsm:id=3，与 swxa_enc.sql / 005 一致）、跳过 tecc_ktool
# 预生成密钥文件，并由 setup_swsds_ini 把 swsds.ini（真机）或预置设备密钥（SoftSDF）
# 铺到 tmp_check/tmp，psql 子进程在该目录下执行（厂商库/软件 SDF 按相对 CWD 取密钥）。
# 真机模式**不设** TECC_SDF_LIB（子进程必须不带它，否则 C 层 dlopen 成 SoftSDF）。
#
# 阈值口径（环境变量控制）：
#   硬加密轮次默认**只记录不判红** —— thresholds.json 按 localkms 标定，硬加密耗时
#   由设备带宽决定，整片判红没有信息量；TAP 仍逐条打印耗时/偏差，JSON 另存
#   threshold_met 供事后统计。SQL 错误（用例没跑成）不受此影响，照旧判红。
#   TECC_PERF_ENFORCE=1      硬加密也按阈值判红
#   TECC_PERF_RECORD_ONLY=1  软加密也只记录（不判红）

use strict;
use warnings;
use Config;
use Cwd qw(abs_path);
use PostgresNode;
use TestLib;
use Test::More;
use IPC::Run;
use POSIX ();

use FindBin;
use lib "$FindBin::Bin/..";
use tecc_perf_utils;
use tecc_test_utils;

my $TAP = Test::More->builder->output;
my $module_dir = "$FindBin::Bin/../..";

# ============ 硬加密模式解析（环境变量控制）============
#
# 三态解析放在 initdb 之前：SoftSDF 的 .so / 预置设备密钥缺失立即 die，
# 不浪费一轮 initdb/start（与 001/002/003 一致）。
my $sdf_val  = $ENV{TECC_SDF_LIB};
my $sdf_set  = (defined $sdf_val && $sdf_val ne '') ? 1 : 0;
my $sdf_off  = ($sdf_set && $sdf_val =~ /\A(?:off|no|none|real|0)\z/i) ? 1 : 0;
my $perf_hsm = (($ENV{TECC_PERF_HSM} // '') =~ /^(1|true|yes|hsm|swxa)$/i) ? 1 : 0;

die "perf: TECC_SDF_LIB（SoftSDF）与 TECC_PERF_HSM=1（真机）不能同时设置\n"
    if $sdf_set && !$sdf_off && $perf_hsm;

my $softsdf  = ($sdf_set && !$sdf_off) ? 1 : 0;
my $real_hsm = ($sdf_off || (!$sdf_set && $perf_hsm)) ? 1 : 0;
my $use_hsm  = $softsdf || $real_hsm;

if ($use_hsm) {
    # init_sdf_mode 负责 SoftSDF 侧校验（.so/预置密钥）并把 TECC_SDF_LIB 归一化成
    # 绝对路径；真机分支会**删除**该变量。TECC_PERF_HSM=1 这种"不带 TECC_SDF_LIB"
    # 的写法先补成 off，让 init_sdf_mode 走真机分支，语义与 001/002/003 统一。
    $ENV{TECC_SDF_LIB} = 'off' if $real_hsm && !$sdf_off;
    tecc_test_utils::init_sdf_mode($module_dir);
    $softsdf  = tecc_test_utils::sdf_soft_mode() ? 1 : 0;
    $real_hsm = $softsdf ? 0 : 1;
}

# ============ 阈值口径：记录模式 ============
# 环境变量优先（TECC_PERF_RECORD_ONLY / TECC_PERF_ENFORCE，见 tecc_perf_utils）；
# 都没设时：硬加密默认只记录，localkms 默认判红。
my $env_rec     = perf_record_only_env();
my $record_only = defined $env_rec ? $env_rec : ($use_hsm ? 1 : 0);
set_perf_record_only($record_only);

print $TAP sprintf "# 加密模式: %s\n", $use_hsm
    ? ($softsdf ? "SoftSDF（软件密码机，KEK=swxakms）"
                : "真机 HSM（KEK=swxakms）")
    : "localkms（本地软加密）";
print $TAP sprintf "# 阈值口径: %s\n", $record_only
    ? "记录模式（不判红；TECC_PERF_ENFORCE=1 可打开判红）"
    : "判红（TECC_PERF_RECORD_ONLY=1 可改为只记录）";

# 实时进度输出（直接写 stderr，不被 prove 缓冲）
sub _progress { print STDERR $_[0]; print $TAP $_[0]; }

{
    package TeccExternalPerfNode;

    sub connstr {
        my ($self, $dbname) = @_;
        return $self->{connstr};
    }

    sub installed_command {
        my ($self, $command) = @_;
        return $self->{psql} if $command eq 'psql';
        return $self->{ktool} if $command eq 'tecc_ktool';
        return $command;
    }

    sub data_dir {
        my ($self) = @_;
        return $self->{data_dir};
    }

    sub port {
        my ($self) = @_;
        return $self->{port};
    }
}

sub _print_command_output {
    my ($label, $stdout, $stderr) = @_;
    my $limit = 80;

    print $TAP "# $label stdout:\n" if defined $stdout && length $stdout;
    if (defined $stdout && length $stdout) {
        my @lines = split /\n/, $stdout;
        for my $i (0 .. $#lines) {
            last if $i >= $limit;
            print $TAP "#   $lines[$i]\n";
        }
        print $TAP "#   ...\n" if @lines > $limit;
    }

    print $TAP "# $label stderr:\n" if defined $stderr && length $stderr;
    if (defined $stderr && length $stderr) {
        my @lines = split /\n/, $stderr;
        for my $i (0 .. $#lines) {
            last if $i >= $limit;
            print $TAP "#   $lines[$i]\n";
        }
        print $TAP "#   ...\n" if @lines > $limit;
    }
}

sub run_checked {
    my ($cmd, $label) = @_;
    my ($stdout, $stderr);
    my $ok = IPC::Run::run($cmd, '>', \$stdout, '2>', \$stderr);

    if (!$ok) {
        _print_command_output($label, $stdout, $stderr);
        die "$label failed\n";
    }

    return ($stdout, $stderr);
}

# ============ 启动PG ============
_progress "# 启动 PostgreSQL 节点...\n";
my $external_connstr = $ENV{TECC_PERF_EXTERNAL_CONNSTR};
my $node;
if ($external_connstr) {
    my $external_psql = $ENV{TECC_PERF_EXTERNAL_PSQL} // 'psql';
    my $external_ktool = $ENV{TECC_PERF_EXTERNAL_KTOOL} // 'tecc_ktool';
    my $external_port = $ENV{TECC_PERF_EXTERNAL_PORT} // '(external)';
    my $external_data_dir = $ENV{TECC_PERF_EXTERNAL_DATADIR} // '(external)';

    $node = {
        connstr  => $external_connstr,
        psql     => $external_psql,
        ktool    => $external_ktool,
        port     => $external_port,
        data_dir => $external_data_dir,
    };
    bless $node, 'TeccExternalPerfNode';
} else {
    $node = get_new_node('tecc_libpq_perf');
}
$ENV{LC_ALL} = 'zh_CN.UTF-8';
# 硬加密 + 外部实例：客户端侧的 swsds.ini / 预置设备密钥必须由 setup_swsds_ini 铺到
# 本轮的 tmp_check/tmp（它需要自管节点的 data_dir），此处不支持，明确报错而不是
# 悄悄退化成"KEK 建不出来 → 后面全是假数据"。
die "perf: 硬加密模式暂不支持 TECC_PERF_EXTERNAL_CONNSTR（需要 setup_swsds_ini 铺 tmp_check/tmp）\n"
    if $use_hsm && $external_connstr;
if (!$external_connstr) {
    $node->init;
    # SoftSDF/真机：与 002/005 相同，在 init 后、start 前准备 HSM 环境
    # （SoftSDF 铺预置 kek-*/sm2*；真机拷 swsds.ini），密码机/库不可用直接中止。
    if ($use_hsm) {
        my $hsm_ok = setup_swsds_ini($node, $module_dir);
        die "perf: " . ($softsdf ? 'SoftSDF' : '真机 HSM')
            . " 模式下 setup_swsds_ini 未就绪（密码机/软件 SDF 不可用）\n"
            unless $hsm_ok;
    }
    $node->start;
}
my $key_dir = $ENV{TECC_KEK_PATH} // "$TestLib::tmp_check/keys";
mkdir $key_dir unless -d $key_dir;
$ENV{TECC_KEK_PATH} = $key_dir;

# 创建postgres超级用户角色
if (!$external_connstr) {
    $node->psql('postgres', "CREATE ROLE postgres LOGIN SUPERUSER;");
}

# 硬加密下把 psql 子进程的工作目录切到 tmp_check/tmp：厂商库（真机，读 swsds.ini）
# 与软件 SDF（SoftSDF，按相对路径读 kek-<index>.key / sm2enc-<index>.pem）都按
# **相对 CWD** 取配置与设备密钥。后续所有路径（module_dir/sql_dir/key_dir/
# TestLib::tmp_check）都是绝对路径，chdir 不影响；脚本末尾 POSIX::_exit 退出，
# 无需切回。
if ($use_hsm) {
    my $work_dir = "$TestLib::tmp_check/tmp";
    die "perf: 硬加密工作目录不存在: $work_dir\n" unless -d $work_dir;
    die "perf: 真机模式工作目录缺少 swsds.ini: $work_dir/swsds.ini\n"
        if $real_hsm && !-f "$work_dir/swsds.ini";
    my $abs = abs_path($work_dir);
    $work_dir = $abs if defined $abs;
    chdir($work_dir) or die "perf: chdir $work_dir failed: $!\n";
    print $TAP "# 硬加密: psql 工作目录 -> $work_dir\n";
}

my $connstr = $node->connstr('postgres');
my $psql = $node->installed_command('psql');
my $sql_dir = "$module_dir/perf/sql";
my $tecc_client_mode = $ENV{TECC_PERF_CLIENT_MODE} // 'on';

if ($external_connstr) {
    run_checked([$psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr,
                 '-c', 'SELECT 1'],
                'external PostgreSQL connection check');
}

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
print $TAP sprintf "#  手工连接: %s -k %s -d '%s'\n", $psql, $tecc_client_mode, $connstr;
print $TAP sprintf "# %s\n\n", '=' x 60;

# ============ 1. DDL ============
print $TAP "\n# === 阶段 1：建表 + KEK/DEK + 索引 ===\n";
my $ddl_content;
if ($use_hsm) {
    # 硬加密：KEK 改走设备索引（对称 hsm:id=1、同态 SM2 hsm:id=3），
    # 无需 tecc_ktool 预生成密钥文件，也不再有 %KEKPATH% 占位符。
    print $TAP "#   KEK: key_store=swxakms（对称 hsm:id=1，同态 SM2 hsm:id=3，"
        . ($softsdf ? 'SoftSDF' : '真机') . "）\n";
    open(my $dh, '<', "$sql_dir/perf_ddl.sql") or die "Cannot read perf_ddl.sql: $!";
    $ddl_content = do { local $/; <$dh> };
    close $dh;
    my $n1 = ($ddl_content =~ s/key_store = 'localkms',\s*\n\s*key_path = '%KEKPATH%\/kek_perf\.key'/key_store = 'swxakms',\n    key_path = 'hsm:id=1'/);
    my $n2 = ($ddl_content =~ s/key_store = 'localkms',\s*\n\s*key_algo = 'sm2',\s*\n\s*key_path = '%KEKPATH%\/kek_perf_he\.priv'/key_store = 'swxakms',\n    key_algo = 'sm2',\n    key_path = 'hsm:id=3'/);
    die "perf: 改写 perf_ddl.sql 的 CREATE KEK 失败（对称命中 $n1，HE 命中 $n2）\n"
        unless $n1 == 1 && $n2 == 1;
} else {
    # 1a. 预生成对称密钥（localkms KEK 需要，CREATE KEK 不再自行生成密钥文件）
    my $ktool = $node->installed_command('tecc_ktool');
    my $sym_key = "$key_dir/kek_perf.key";
    if (!-f $sym_key) {
        print $TAP "#   [tecc_ktool] generate sym key -> $sym_key\n";
        run_checked([$ktool, 'generate', '--key-name', 'kek_perf',
                     '--key-algo', 'sm4', '--key-path', $key_dir],
                    'tecc_ktool generate sym key');
    }
    # 1b. 预生成非对称密钥对（同态 KEK 需要，key_path 用 %KEKPATH% 占位符由脚本替换）
    # 与回归测试 homomorphic_enc.sql 的 asym KEK 机制一致
    my $he_priv = "$key_dir/kek_perf_he.priv";
    if (!-f $he_priv) {
        print $TAP "#   [tecc_ktool] generate asym keypair -> $he_priv\n";
        run_checked([$ktool, 'generate', '--key-name', 'kek_perf_he',
                     '--key-algo', 'sm2', '--key-path', $key_dir],
                    'tecc_ktool generate asym keypair');
    }
    # 1b. 替换 %KEKPATH% 占位符后执行 DDL（psql 不展开环境变量，需文本替换）
    open(my $dh, '<', "$sql_dir/perf_ddl.sql") or die "Cannot read perf_ddl.sql: $!";
    $ddl_content = do { local $/; <$dh> };
    close $dh;
    $ddl_content =~ s/%KEKPATH%/$key_dir/g;
}
my $ddl_tmp = "/tmp/tecc_perf_ddl_$$.sql";
open(my $dfh, '>', $ddl_tmp) or die "Cannot write $ddl_tmp: $!";
print $dfh $ddl_content;
close $dfh;
run_checked([$psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-d', $connstr, '-f', $ddl_tmp],
            'perf DDL');
unlink $ddl_tmp;
run_checked([$psql, '-k', $tecc_client_mode, '-v', 'ON_ERROR_STOP=1', '-X', '-At', '-d', $connstr, '-c',
   "SELECT count(*) FROM enc_int4; "
    . "SELECT count(*) FROM enc_int4_b; "
    . "SELECT count(*) FROM enc_float8; "
    . "SELECT count(*) FROM enc_text; "
    . "SELECT count(*) FROM he_int8; "
    . "SELECT count(*) FROM stage_int4; "
    . "SELECT count(*) FROM stage_float8; "
    . "SELECT count(*) FROM stage_text; "
    . "SELECT count(*) FROM stage_he_int8;"],
    'perf required table check');
_progress "# 建表完成\n";

# ============ 2. 数据初始化 ============
_progress "\n# === 阶段 2：数据初始化 ===\n";
my $insert_results = init_encrypted_data($node, $module_dir);
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
IPC::Run::run([$psql, '-k', $tecc_client_mode, '-X', '-At', '-d', $connstr, '-c', "SELECT current_schema()"],
    '>', \$cs_out, '2>', \$cs_err);
chomp $cs_out if defined $cs_out;
print $TAP "# DIAG: current_schema = '$cs_out'\n";
my ($sp_out, $sp_err);
IPC::Run::run([$psql, '-k', $tecc_client_mode, '-X', '-At', '-d', $connstr, '-c', "SHOW search_path"],
    '>', \$sp_out, '2>', \$sp_err);
chomp $sp_out if defined $sp_out;
print $TAP "# DIAG: search_path = '$sp_out'\n";
# 诊断：检查current_schema是否存在
my ($ns_out, $ns_err);
IPC::Run::run([$psql, '-k', $tecc_client_mode, '-X', '-At', '-d', $connstr, '-c',
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
        req_threshold => $th ? ($th->{requirement_threshold_ms} // 0) : 0,
        errors    => $result->{total_err},
        resource  => $result->{resource},
    };
}

# ============ 5b. INSERT 用例（seed 阶段顺带测量）============
# seed 过程中已按 4 个代表类型测量客户端加密 INSERT 耗时，
# 作为独立 perf 用例纳入阈值检查与汇总（数据源：预置数据生成，不额外造数）。
my @insert_cases = (
    [ 'int4_insert',    'int4'    ],
    [ 'float8_insert',  'float8'  ],
    [ 'text_insert',    'text'    ],
    [ 'he_int8_insert', 'he_int8' ],
);
for my $case (@insert_cases) {
    my ($name, $t) = @$case;
    my $res = $insert_results->{$t} // next;
    my $th = $thresholds->{$name};
    _progress sprintf("# [%2d/%2d] %s ...\n", scalar(@results) + 1, $total + scalar(@insert_cases), $name);
    print_perf_result($name, $res, $th);
    save_perf_result($module_dir, $name, $res, $th, 'libpq');
    push @results, {
        name      => $name,
        passed    => check_threshold($res, $th),
        avg       => $res->{avg_ms},
        threshold => $th ? ($th->{threshold_ms} // 0) : 0,
        req_threshold => $th ? ($th->{requirement_threshold_ms} // 0) : 0,
        errors    => $res->{total_err},
        resource  => $res->{resource},
    };
}

# ============ 6. 汇总 ============
_progress "\n# === 阶段 4：汇总 ===\n";
print $TAP "\n";
print_perf_table(\@results);
print_perf_summary(\@results);
# ReqID:TECC4.0 - 报告头按**本轮实际执行的模式**标注，不再只按环境变量猜：
# 真机有三种写法（TECC_PERF_HSM=1 / TECC_SDF_LIB=off / TECC_SDF_LIB=real 等），
# 走 TECC_PERF_HSM 时 perf_crypto_backend 认不出，KEK 存储则由 perf_crypto_backend
# 反推——旧写法在真机上会标成 localkms，属误标。此处直接给出实测量。
my $sdf_base = $ENV{TECC_SDF_LIB} // '';
$sdf_base =~ s{.*/}{};
my $report_opts = {
    driver    => 'libpq',
    crypto    => $use_hsm
        ? ($softsdf ? "SoftSDF ($sdf_base)" : 'HSM:swxa (真实加密机)')
        : 'localkms (本地软加密)',
    key_store => $use_hsm ? 'swxakms' : 'localkms',
};
my $summary_file = save_perf_summary($module_dir, \@results, $report_opts);
save_perf_resource_report($module_dir, \@results, $report_opts);
print $TAP sprintf("1..%d\n", scalar @results);
_progress "# 测试完成\n";

# ============ 7. 直接退出跳过析构，保留数据目录，下次运行时 make check 清理旧数据 ============
POSIX::_exit(0);

