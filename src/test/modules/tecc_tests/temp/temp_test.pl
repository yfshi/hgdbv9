#!/usr/bin/perl

# temp_test.pl — 安装测试用例临时存放
# 不包含在 check-tecc 中，仅通过 make check-tecc-temp 显式执行
# 测试用例从安装测试工作目录直接拷贝，暂不改造

use strict;
use warnings;
use Config;
use PostgresNode;
use TestLib;
use Test::More;

use FindBin;
use lib "$FindBin::Bin/../t";
use tecc_test_utils;

my $TAP = Test::More->builder->output;

my $module_dir = "$FindBin::Bin/..";
my $sql_dir    = "$module_dir/temp/sql";
my $exp_dir    = "$module_dir/temp/expected";

print $TAP "# Starting PostgreSQL node for TECC temp tests...\n";
my $node = get_new_node('tecc_temp');
$node->init;
$node->start;

my ($setup_out, $setup_err) = $node->psql('postgres',
    "CREATE ROLE postgres LOGIN SUPERUSER;");

print $TAP "\n# === temp: installation test cases ===\n";
my ($passed, $total) = run_sql_dir_diff($node,
    $sql_dir, $exp_dir, 'temp', 1);

$node->stop('fast');

my $failed = $total - $passed;
print $TAP "\n# ====================================\n";
print $TAP "# $passed of $total tests passed, $failed failed\n";
print $TAP "# ====================================\n";
if ($failed > 0) {
    print $TAP "\n# [结果查看] 差异文件和实际输出保存在:\n";
    print $TAP "#     $TestLib::tmp_check/results/\n";
    print $TAP "#   查看所有失败测试的实际输出:\n";
    print $TAP "#       ls $TestLib::tmp_check/results/*.out\n";
    print $TAP "#   查看所有失败测试的差异对比:\n";
    print $TAP "#       ls $TestLib::tmp_check/results/*.diff\n";
    print $TAP "#   查看完整测试日志:\n";
    print $TAP "#       cat $TestLib::tmp_check/regress_log_004_tecc_temp_test\n";
    print $TAP "\n";
}
print $TAP "1..$total\n";
