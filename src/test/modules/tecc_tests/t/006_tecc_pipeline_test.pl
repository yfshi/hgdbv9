#!/usr/bin/perl

#
# 006_tecc_pipeline_test.pl
#
# TECC libpq 管道模式（pipeline mode）回归测试
#
# 验证 Review 问题 1 修复后，TECC 在 libpq 管道模式下透明加解密可用：
#   1. PQsendQueryParams（0 参数扩展协议）批量 INSERT/SELECT —— 文本改写 + 结果解密
#   2. PQsendQueryParams（$N 参数化）INSERT —— 参数加密
#   3. PQsendPrepare + PQsendQueryPrepared —— prepared 语句参数加密
#   4. 非管道模式对照（确保行为不变）
#
# 管道模式是 libpq C API 特性，psql 无法表达，故用 C 测试程序驱动
# （t/libpq_pipeline_test.c），TAP 校验其退出码与汇总输出。
#

use strict;
use warnings;
use Config;
use Test::More;
use PostgresNode;
use TestLib;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;

my $TAP = Test::More->builder->output;

note("");
note("╔═════════════════════════════════╗");
note("║  TECC libpq 管道模式回归测试     ║");
note("╚═════════════════════════════════╝");

my $node = get_new_node('tecc_pipeline');
# ReqID:TECC4.0 - 消息语言固定英文（与其它用例一致）
delete $ENV{LC_ALL};
$ENV{LC_CTYPE}    = 'zh_CN.UTF-8';
$ENV{LC_COLLATE}  = 'zh_CN.UTF-8';
$ENV{LC_MESSAGES} = 'C';
$ENV{LANG}        = 'C';
init_sdf_mode($FindBin::Bin . '/..');
$node->init;
setup_swsds_ini($node, $FindBin::Bin . '/..');
$node->start;

my ($key_dir, $tmp_dir) = setup_tecc_regress_env();
pregen_localkms_kek_files($FindBin::Bin . '/..', $key_dir);

# C 测试程序使用的 KEK（libpq_pipeline_test.c 中 key_path=/tmp/kek_pipe.key，
# 经 TECC_KEK_PATH 覆盖到 $key_dir）——C 程序不在 .sql 中，pregen 扫描不到，
# 这里显式用 tecc_ktool generate 预生成（幂等：已存在则跳过）。失败直接中止，
# 不计入 TAP 测试点。
note("\n=== 预生成 pipeline 测试 KEK ===");
{
    my $kek_path = "$key_dir/kek_pipe.key";
    if (-f $kek_path) {
        note("kek_pipe.key already exists, skip generate");
    }
    else {
        my $gen = system("tecc_ktool generate --key-name kek_pipe --key-algo sm4 --key-path $key_dir >/dev/null 2>&1");
        BAIL_OUT("tecc_ktool generate kek_pipe failed") if $gen != 0;
        note("kek_pipe.key generated");
    }
}

my $connstr = $node->connstr('postgres');
# 附加 enable_tecc=1 客户端参数启用 TECC
my $tecc_connstr = $connstr . " enable_tecc=1";

# ---- 编译 C 测试程序（使用当前源码构建的 libpq） ----
note("\n=== 编译 libpq_pipeline_test ===");
my $srcdir = "$FindBin::Bin/../..";          # .../tecc_tests
my $pgroot = "$srcdir/../../..";             # PG 源码根
my $libpq_dir = "$pgroot/src/interfaces/libpq";
my $c_src = "$FindBin::Bin/libpq_pipeline_test.c";
my $c_bin = "$TestLib::tmp_check/libpq_pipeline_test";
my $rpath = $libpq_dir;

my $cc = $Config{cc} || 'gcc';
my $compile_cmd = "$cc -o $c_bin $c_src "
    . "-I$pgroot/src/include -I$libpq_dir "
    . "-L$libpq_dir -lpq -Wl,-rpath,$rpath";
print $TAP "# compile: $compile_cmd\n";
my $c_rc = system($compile_cmd);
is($c_rc, 0, "compile libpq_pipeline_test") or BAIL_OUT("compile failed");

# 测试程序需要 TECC_KEK_PATH 指向预生成 KEK 目录
$ENV{TECC_KEK_PATH} = $key_dir;
# LD_LIBRARY_PATH 指向当前源码构建的 libpq（优先于系统 libpq）
$ENV{LD_LIBRARY_PATH} = join(':', grep { defined && length } (
    $libpq_dir,
    $ENV{LD_LIBRARY_PATH} || ''));

# ---- 运行管道模式测试 ----
note("\n=== 运行 libpq_pipeline_test ===");
my $run_out = `$c_bin "$tecc_connstr" 2>&1`;
my $run_rc = $? >> 8;
print $TAP "# ---- 输出 ----\n";
for my $line (split /\n/, $run_out) {
    print $TAP "# $line\n";
}
print $TAP "# ----------------\n";

is($run_rc, 0, "libpq_pipeline_test exit code 0")
    or diag("test binary returned $run_rc");

# 汇总校验：总计/通过/失败 数值
my ($total) = $run_out =~ /总计:\s*(\d+)/;
my ($passed) = $run_out =~ /通过:\s*(\d+)/;
my ($failed) = $run_out =~ /失败:\s*(\d+)/;

ok(defined($total) && defined($passed) && defined($failed),
   "pipeline test summary parsed (total=$total passed=$passed failed=$failed)");
if (defined($failed)) {
    is($failed, 0, "pipeline test: 0 failures")
        or diag("$failed of $total tests failed");
}
if (defined($total) && defined($passed)) {
    is($passed, $total, "pipeline test: all $total passed");
}

$node->stop('fast');

done_testing(5);
