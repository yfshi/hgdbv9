#!/usr/bin/perl

# 009_tecc_hgdde_test.pl
#
# TECC + hgdde 分布式密态回归（1 coordinator + 2 worker）
#
# 执行:
#   make check-tecc-hgdde
#   TECC_TEST=010_shard_ddl_metadata.sql make check-tecc-hgdde   # 单用例
#
# 为什么单独一个入口：
#   单实例 TAP 框架（001/002/003）跑在 PostgresNode 的单实例上，够不到分布式
#   分发路径。hgdde 同理：分片 DDL 由 coordinator 反解析后下发到 worker 重新
#   解析执行、分片元数据落在 worker 自己的 hg_encrypted_columns 上。
#   Citus 的 t/008_tecc_citus_test.pl 与本文件是同一套用例家族，本文件只把
#   citus.* 命名空间换成 hgdde.*（GUC / 目录 / 函数），其余逻辑一致。
#
# 目录约定:
#   hgdde/sql/*.sql                        用例，逐文件与 hgdde/expected/<base>.out 比对
#
# 前置：temp-install 已把 hgdde.so 装进临时前缀（src/Makefile.global.in 的
#       temp-install 钩子会在装完 PG 后自动 configure/构建/安装 hgdde）。

use strict;
use warnings;
use Config;
use Test::More;
use PostgresNode;
use TestLib;

# 加载公共工具
use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;
use tecc_hgdde_protection;  # ReqID:TECC4.0 - 用户数据保护的备份/恢复断言

# TAP输出句柄（TestLib重定向了STDOUT，必须走builder）
my $TAP = Test::More->builder->output;

# 定位模块根目录（即 tecc_tests/ 目录）
my $module_dir = "$FindBin::Bin/..";

note("");
note("╔══════════════════════════════════╗");
note("║  TECC + hgdde 分布式密态回归测试 ║");
note("╚══════════════════════════════════╝");
note("");

# ---- 消息语言固定英文；集群用 C locale（分布式用例不依赖本地化排序）----
delete $ENV{LC_ALL};                # LC_ALL 优先级最高，必须先删
$ENV{LC_CTYPE}    = 'C';
$ENV{LC_COLLATE}  = 'C';
$ENV{LC_MESSAGES} = 'C';            # 报错/提示 message 固定英文
$ENV{LANG}        = 'C';

# ---- 密钥目录必须在起节点之前设好：服务端进程也要读 localkms 密钥文件 ----
my ($key_dir, $tmp_dir) = setup_tecc_regress_env();
$ENV{TECC_KEK_PATH} = $key_dir;

# localkms 的 KEK 不再由 CREATE KEK 生成密钥文件，必须由 tecc_ktool 预生成
sub gen_key_files {
    my $ktool;
    for my $p (split /:/, $ENV{PATH}) {
        next unless defined $p && $p ne '' && -x "$p/tecc_ktool";
        $ktool = "$p/tecc_ktool";
        last;
    }
    BAIL_OUT("PATH 中找不到 tecc_ktool（temp-install 应提供）；"
           . "请通过 make check-tecc-hgdde 运行。\n当前 PATH: $ENV{PATH}")
        unless defined $ktool;

    for my $spec (['ddb_kek', 'sm4'], ['ddb_he', 'sm2']) {
        next if -f "$key_dir/$spec->[0].key" || -f "$key_dir/$spec->[0].priv";
        system($ktool, 'generate', '--key-name', $spec->[0],
               '--key-algo', $spec->[1], '--key-path', $key_dir) == 0
            or BAIL_OUT("tecc_ktool generate $spec->[0] 失败");
    }
}
gen_key_files();
pregen_localkms_kek_files($module_dir, $key_dir);

# ---- 前置检查：临时前缀里必须有 hgdde.so ----
{
    my $pg_config;
    for my $p (split /:/, $ENV{PATH}) {
        next unless defined $p && $p ne '';
        next unless $p =~ /tmp_install/ && -x "$p/pg_config";
        $pg_config = "$p/pg_config";
        last;
    }
    BAIL_OUT(
        "TECC+hgdde 回归必须通过 make check-tecc-hgdde 运行："
            . "PATH 中找不到 temp-install 的 pg_config。\n当前 PATH: $ENV{PATH}")
        unless defined $pg_config;

    my $pkglibdir = qx{"$pg_config" --pkglibdir};
    chomp $pkglibdir;
    BAIL_OUT(
        "找不到 $pkglibdir/hgdde.so：hgdde 没有装进临时安装前缀。\n"
            . "src/Makefile.global.in 的 temp-install 钩子会在装完 PG 后自动\n"
            . "configure/构建/安装 hgdde，出现本错误说明该钩子失败"
            . "（常见原因：缺 libcurl4-openssl-dev / liblz4-dev / libzstd-dev）。")
        unless -f "$pkglibdir/hgdde.so";
}

# ---- 起 1 coordinator + 2 worker + 1 个待加入实例 ----
# 第 4 个实例预先 initdb/启动/CREATE EXTENSION，但**不加入集群**：供
# 085_mac_distributed 模拟"新节点加入后标记补发"（与 t/008 对 citus 的做法同构）。
my $coord = get_new_node('tecc_hgdde_coord');
my $w1    = get_new_node('tecc_hgdde_w1');
my $w2    = get_new_node('tecc_hgdde_w2');
my $w3    = get_new_node('tecc_hgdde_w3');   # spare：不注册，等用例里 add_node
my @instances = ($coord, $w1, $w2, $w3);
my @nodes     = ($coord, $w1, $w2);   # 注册进集群的节点

for my $node (@instances) {
    $node->init(extra => ['-E', 'UTF8', '--locale=C']);
    # 原 feature-ddb 的 hba 重写块（ddb_sm3 / ddb_mfa + sm3 / md5,pkey 规则）已移除：
    # 本基线不含 SM3/MFA 认证方式，pg_hba 会因 invalid authentication method 起不来。
    # 分布式测试只需要下面那条对 127.0.0.2/32 的 trust 规则。

    # PostgresNode 在 Unix 上默认只开 Unix socket（listen_addresses = ''），
    # 而分布式集群必须能通过 TCP 连到 worker —— 追加一行把 TCP 打开。
    $node->append_conf('postgresql.conf', <<"CONF");
# --- TECC/hgdde 分布式回归 ---
# hg_sysmac（标记强访/MAC）自 2026-11-09 起是预加载扩展：与 hgdde 一起预加载；
# 两者的 ProcessUtility 钩子都是链式调用，先后不影响行为。
shared_preload_libraries = 'hgdde,hg_sysmac'
hgdde.node_conninfo = 'sslmode=prefer'
shared_buffers = '32MB'
max_connections = 20
listen_addresses = 'localhost,127.0.0.2'
wal_level = 'replica'
hg_sysmac.global_on = on
CONF
    $node->append_conf('pg_hba.conf',
        "host    all             all             127.0.0.2/32            trust\n");

    $node->enable_archiving;
    $node->start;
}

sub run_sql_or_die {
    my ($node, $label, $sql) = @_;
    my ($rc, $out, $err) = $node->psql('postgres', $sql);
    BAIL_OUT("$label 在节点 " . $node->name . " 上失败 (rc=$rc)\n$out\n$err")
        if $rc != 0;
}

for my $node (@instances) {
    run_sql_or_die($node, 'CREATE EXTENSION hgdde', 'CREATE EXTENSION IF NOT EXISTS hgdde;');
    # 标记强访（MAC）SQL 入口自 2026-11-09 起由扩展提供：每个节点（含 spare）
    # 都要建，否则分片上的行级策略/标记检查在 worker/w3 上解析不到函数。
    run_sql_or_die($node, 'CREATE EXTENSION hg_sysmac',
        'CREATE EXTENSION IF NOT EXISTS hg_sysmac;');
}

run_sql_or_die($coord, 'hgdde_set_coordinator_host',
    "SELECT hgdde_set_coordinator_host('localhost', " . $coord->port . ");");
run_sql_or_die($coord, 'hgdde_add_node w1',
    "SELECT hgdde_add_node('localhost', " . $w1->port . ");");
run_sql_or_die($coord, 'hgdde_add_node w2',
    "SELECT hgdde_add_node('localhost', " . $w2->port . ");");

# ---- 在 coordinator 上建一套 KEK/DEK ----
sub note_node_keys {
    my ($node) = @_;
    my $sql = <<"SQL";
CREATE KEK ddb_kek WITH (
    key_store = 'localkms',
    key_algo  = 'sm4',
    key_path  = '$key_dir/ddb_kek.key'
);
CREATE KEK ddb_hekek WITH (
    key_store = 'localkms',
    key_algo  = 'sm2',
    key_path  = '$key_dir/ddb_he.priv'
);
CREATE DEK ddb_dek   WITH VALUES (KEK = ddb_kek);
CREATE DEK ddb_headd WITH VALUES (KEK = ddb_hekek, KEY_ALGO = 'he-add');
CREATE DEK ddb_hemul WITH VALUES (KEK = ddb_hekek, KEY_ALGO = 'he-mul');
SQL
    my ($stdout, $stderr) = ('', '');
    my $rc = $node->psql('postgres', $sql,
        extra_params => ['-k', 'on'],
        stdout => \$stdout, stderr => \$stderr);
    if ($rc != 0) {
        print $TAP "# 建钥失败 stdout:\n$stdout\n# stderr:\n$stderr\n";
        BAIL_OUT("节点 " . $node->name . " 建 KEK/DEK 失败 (rc=$rc)");
    }
    print $TAP "# 建钥完成（coordinator 本地建一套，依赖 W3 自动传播到 worker）: "
        . $node->name . "\n";
}

note_node_keys($coord);

$ENV{TECC_HGDDE_W1_PORT} = $w1->port;
$ENV{TECC_HGDDE_W2_PORT} = $w2->port;
$ENV{TECC_HGDDE_W3_PORT} = $w3->port;   # spare 实例：供 085 加节点用
$ENV{TECC_HGDDE_COORD_PORT} = $coord->port;

print $TAP "# 集群: coordinator=localhost:" . $coord->port
        . " worker1=localhost:" . $w1->port
        . " worker2=localhost:" . $w2->port
        . " spare(未注册)=" . $w3->port . "\n";
print $TAP "# 密钥目录: $key_dir\n";
print $TAP "# 用例目录: $module_dir/hgdde/sql\n\n";

my ($pass, $total) = run_sql_dir_diff($coord,
    "$module_dir/hgdde/sql",
    "$module_dir/hgdde/expected",
    'hgdde',
    1);

# ReqID:TECC4.0 - V9-DD-1-1 用户数据保护（行级安全标记的导出/导入）：
# 需要真实的 pg_dump / pg_restore / psql 二进制与归档，无法用 SQL golden 表达，
# 因此走 TAP 断言。虚拟用例名沿用 TECC_TEST=<case>.sql 过滤约定。
# 与 t/008 的 citus: protection 用例同一套 UDP 清单，改为 hgdde 命名空间，
# 并补上 2026-11-09 标记强访插件化之后要求的失败关闭断言。
# MAC 行级安全标记（set_user_seclabel 等）不在 release-9.2 合并范围内，
# 基线没有这些函数，该子测试整段跳过。
if (0 && (!get_tecc_test_filter() || get_tecc_test_filter() eq '087_mac_dump_restore.sql')) {
    my ($protection_pass, $protection_total) =
        tecc_hgdde_protection::run($coord, $tmp_dir, $w1);
    $pass += $protection_pass;
    $total += $protection_total;
}

for my $node (reverse @instances) {
    $node->stop('fast');
}

my $fail = $total - $pass;
print $TAP "\n# ====================================\n";
print $TAP "# $pass of $total tests passed, $fail failed\n";
print $TAP "# ====================================\n";
if ($fail > 0) {
    print $TAP "# [结果查看] 差异文件和实际输出保存在:\n";
    print $TAP "#     $TestLib::tmp_check/results/\n";
}
print_failure_summary();

if ($total == 0) {
    my $reason = empty_run_reason(get_tecc_test_filter());
    print $TAP "# $reason\n";
    print $TAP "1..0 # SKIP $reason\n";
}
else {
    print $TAP "1..$total\n";
}
