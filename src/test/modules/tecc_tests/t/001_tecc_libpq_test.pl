#!/usr/bin/perl

# 001_tecc_libpq_test.pl

#
# TECC libpq 全链路回归测试
# 使用 psql 执行SQL，验证加密→存储→解密端到端链路正确性。
#
# 执行:
#   1. 启动具有 TECC 支持的 PG 实例
#   2. 执行 公用SQL → 对比 expected_libpwq/ + expected_common/
#   3. 执行 libpq独有SQL → 对比 libpq/expected/（含 pg_dump）

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

# TAP输出句柄（TestLib重定向了STDOUT，必须走builder）
my $TAP = Test::More->builder->output;

# 定位模块根目录（即 tecc_tests/ 目录）
my $module_dir = "$FindBin::Bin/..";

note("");
note("╔═════════════════════════════╗");
note("║  TECC libpq 全链路回归测试  ║");
note("╚═════════════════════════════╝");
note("");
# 启动PG节点
print $TAP "# Starting PostgreSQL node for TECC libpq tests...\n";
my $node = get_new_node('tecc_libpq');

delete $ENV{LC_ALL};
$ENV{LC_CTYPE} = 'zh_CN.UTF-8';
$ENV{LC_COLLATE} = 'zh_CN.UTF-8';
$ENV{LC_MESSAGES} = 'C';
$ENV{LANG} = 'C';
#$ENV{LC_ALL} = 'zh_CN.UTF-8';

init_sdf_mode($module_dir);
$node->init;
setup_swsds_ini($node, $module_dir);
$node->start;

# 创建 postgres 超级用户角色（部分测试用例需要）
my ($setup_rc, $setup_out, $setup_err) = $node->psql('postgres',
    "CREATE ROLE postgres LOGIN SUPERUSER;");
if ($setup_err) {
    # postgres 角色可能已存在，忽略错误
}

# ---- 打印 TECC 临时目录信息 ----
{
    my ($key_dir, $tmp_dir) = setup_tecc_regress_env();
    # ReqID:TECC4.0 - 预生成 localkms KEK 文件（CREATE KEK 不生成文件，
    # 密钥文件须由 tecc_ktool generate 提供；见 Review 问题10）
    pregen_localkms_kek_files($module_dir, $key_dir);
    print $TAP "# \n";
    print $TAP "# TECC 回归测试临时目录:\n";
    print $TAP "#   KEK key 目录:  $key_dir\n";
    print $TAP "#   (C层 TECC_KEK_PATH 覆盖 CREATE KEK 的 KEY_PATH 目录)\n";
    print $TAP "#   导出文件目录:  $tmp_dir\n";
    print $TAP "#   (SQL脚本中 COPY/\\copy/\\i/\\o 使用相对路径, CWD 指向该目录)\n";
    print $TAP "# \n";
}

my ($sp, $st) = run_sql_dir_libpq_dual_diff($node,
    "$module_dir/shared/sql",
    "$module_dir/shared/expected_libpq",
    "$module_dir/shared/expected_common",
    'shared');

my ($lp, $lt) = (0, 0);
if (-d "$module_dir/libpq/sql") {
    note("\n=== libpq独有用例 ===");
    ($lp, $lt) = run_sql_dir_diff($node,
        "$module_dir/libpq/sql",
        "$module_dir/libpq/expected",
        'libpq',
        1);
}

$node->stop('fast');

my $t_pass = $sp + $lp;
my $t_total = $st + $lt;
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
    print $TAP "#       cat $TestLib::tmp_check/regress_log_001_tecc_libpq_test\n";
    print $TAP "\n";
}
print_failure_summary();

# ReqID:TECC4.0 - 一个用例都没跑时，把原因打进报告（裸 `1..0` 只会显示
# "skipped: (no reason given)"，看不出是被跳过还是 TECC_TEST 没匹配上）
if ($t_total == 0) {
    my $reason = empty_run_reason(get_tecc_test_filter());
    print $TAP "# $reason\n";
    print $TAP "1..0 # SKIP $reason\n";
}
else {
    print $TAP "1..$t_total\n";
}
