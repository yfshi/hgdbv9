#!/usr/bin/perl

#
# 007_tecc_ktool_cli_test.pl
#
# tecc_ktool 命令行回归测试（纯文件操作，不依赖加密表/SQL 前端）。
#
# 覆盖 0901 批次 6 条 tecc_ktool CLI bug：
#
#   L8   list 目录参数：位置参数 / --dir=<path> / --dir <path> 三形态须等效，
#        多余参数、--dir 与位置参数并用、非法选项必须明确报错 rc=2（不得静默
#        回退当前目录）。
#   L100 日志级别：默认（未设 TECC_LOG_LEVEL）stderr 零 DEBUG 刷屏；
#        TECC_LOG_LEVEL=DEBUG 时按需出现 [DEBUG] ... PBKDF2: 日志。
#   L141 delete 独立性：delete .priv 只毁 .priv（.pub 保留），delete .pub 只毁
#        .pub（.priv 保留），不再"删 .priv 连带 .pub"；报 Destroyed 覆写提示。
#   L177 info 字段：.key/.priv 严格 4 字段 key_algo/Algorithm/Created/Format，
#        无 Size/Key Type/File 残留行；sm2/sm4 的算法行均为文件保护算法
#        (SM4-CBC + PBKDF2-HMAC-SHA256)。
#   L539 list 对齐：size 数值左边界与 "Size" 表头同列；分隔虚线与整行同宽。
#   L590 validate/info .pub：公钥按裸 SM2 点(65B/0x04/on-curve)分流校验，不再
#        报 "bad magic"；validate 对篡改返回 rc=1 与具体原因；info 正常展示。
#
# 说明：本用例只用 CLI 对密钥文件操作，无需 psql/SQL 对象；仍按模块惯例创建
#       node 以获得 tmp_check 工作目录与 tmp_install 的 PATH 环境。

use strict;
use warnings;
use Config;
use Test::More;
use PostgresNode;
use TestLib;
use IPC::Open3;
use Symbol 'gensym';
use File::Path qw(rmtree);
use File::Copy qw(copy);

use FindBin;
use lib $FindBin::Bin;

my $TAP = Test::More->builder->output;

note("");
note("╔══════════════════════════════════════╗");
note("║  tecc_ktool CLI 回归测试             ║");
note("╚══════════════════════════════════════╝");

my $node = get_new_node('tecc_ktool_cli');
$node->init;
$node->start;

my $basedir = "$TestLib::tmp_check";
my $kdir = "$basedir/kdir007";     # list / info / validate fixtures
my $kdel = "$basedir/kdel007";     # delete 独立性 fixtures
my $kdbg = "$basedir/kdbg007";     # DEBUG 日志 fixtures
for my $d ($kdir, $kdel, $kdbg) { rmtree($d) if -d $d; mkdir $d or die "mkdir $d: $!"; }

# 定位 tecc_ktool：PATH 里第一个（tmp_install 排最前）
my $ktool;
for my $p (split /:/, $ENV{PATH}) {
    next unless $p && $p ne '';
    if (-x "$p/tecc_ktool") { $ktool = "$p/tecc_ktool"; last; }
}
die "tecc_ktool not found in PATH\n" unless $ktool;

# ---- 工具函数 ----
sub gen {
    my ($algo, $name, $dir) = @_;
    my $rc = system($ktool, 'generate', '--key-name', $name,
                    '--key-algo', $algo, '--key-path', $dir);
    die "tecc_ktool generate $name (algo=$algo) failed rc=$rc\n" if $rc != 0;
}

# 运行 tecc_ktool，返回 (rc, stdout, stderr)
sub run_ktool {
    my (@args) = @_;
    my $err = gensym;
    my $pid = open3(my $ci, my $co, $err, $ktool, @args);
    close($ci);
    my $o = do { local $/; <$co> } // '';
    my $e = do { local $/; <$err> } // '';
    waitpid($pid, 0);
    return (($? >> 8), $o, $e);
}

my $total = 0;
my $ok_cnt = 0;
sub v {
    my ($desc, $cond) = @_;
    $total++;
    if ($cond) {
        print $TAP "ok $total - $desc\n";
        $ok_cnt++;
    } else {
        print $TAP "not ok $total - $desc\n";
    }
}

# 共享断言块文本（stdout+stderr 合并，通道无关）
sub out_err { my ($o, $e) = @_; return "$o$e"; }

# ============================================================
# 测试 1 (L8): list 目录参数三形态 + 错误收敛
# ============================================================
note("\n=== 测试1: list 目录参数（L8）===");
gen('sm4', 'kek_sm4', $kdir);
gen('sm2', 'kek_sm2', $kdir);

my ($rc, $o, $e) = run_ktool('list', $kdir);
my $t = out_err($o, $e);
v("list <dir>（位置参数）跨目录可列",
    $rc == 0 && $t =~ /kek_sm4/ && $t =~ /kek_sm2/ && $t =~ /Total: 2 KEK file/);

($rc, $o, $e) = run_ktool('list', "--dir=$kdir");
$t = out_err($o, $e);
v("list --dir=<dir>（=号形式）等效",
    $rc == 0 && $t =~ /kek_sm4/ && $t =~ /Total: 2 KEK file/);

($rc, $o, $e) = run_ktool('list', '--dir', $kdir);
$t = out_err($o, $e);
v("list --dir <dir>（紧邻形式）保留可用",
    $rc == 0 && $t =~ /kek_sm4/ && $t =~ /Total: 2 KEK file/);

($rc, $o, $e) = run_ktool('list', $kdir, 'extra');
$t = out_err($o, $e);
v("list 多余参数 → rc=2 + too many arguments",
    $rc == 2 && $t =~ /too many arguments/);

($rc, $o, $e) = run_ktool('list', '--dir', $kdir, $kdir);
$t = out_err($o, $e);
v("list --dir 与位置参数并用 → rc=2 明确互斥报错",
    $rc == 2 && $t =~ /both --dir and a directory argument/);

($rc, $o, $e) = run_ktool('list', '--bogus');
$t = out_err($o, $e);
v("list 非法选项 → rc=2 + unrecognized option",
    $rc == 2 && $t =~ /unrecognized option/);

# ============================================================
# 测试 2 (L539): list 表格对齐 —— size 与表头同列、虚线与整行同宽
# ============================================================
note("\n=== 测试2: list 表格对齐（L539）===");
($rc, $o, $e) = run_ktool('list', '--dir', $kdir);
my @L = split /\n/, $o;
my $hdr  = $L[0];
my $dash = $L[1];
my $hsize = index($hdr, 'Size');
v("表头含 Name/Type/Size/Modified",
    $hdr =~ /Name\s+Type\s+Size\s+Modified/);
v("Size 表头列下标 > 0", $hsize > 0);
v("分隔虚线为纯 '-' 且与表头同宽",
    $dash =~ /^-+$/ && length($dash) == length($hdr));

my $align_ok = 1;
for my $row (@L[2 .. $#L]) {
    last unless $row =~ /^kek_/;
    my $num = ($row =~ /\bsym\b/) ? '148' : '228';
    if (index($row, $num) != $hsize) { $align_ok = 0; last; }
}
v("数据 size 数值左边界与 'Size' 表头同列",
    $align_ok);

# ============================================================
# 测试 3 (L100): 默认安静 / TECC_LOG_LEVEL=DEBUG 按需刷日志
# ============================================================
note("\n=== 测试3: 日志级别默认 DEBUG 关闭（L100）===");
my ($drc, $do, $de);
{
    local $ENV{TECC_LOG_LEVEL};
    delete $ENV{TECC_LOG_LEVEL};
    ($drc, $do, $de) = run_ktool('generate', '--key-name', 'dbg_q',
                                 '--key-algo', 'sm4', '--key-path', $kdbg);
}
v("默认 generate rc=0 且文件生成", $drc == 0 && -f "$kdbg/dbg_q.key");
v("默认 generate stderr 无任何 DEBUG 刷屏", $de !~ /DEBUG/ && $de eq '');

{
    local $ENV{TECC_LOG_LEVEL} = 'DEBUG';
    ($drc, $do, $de) = run_ktool('generate', '--key-name', 'dbg_l',
                                 '--key-algo', 'sm4', '--key-path', $kdbg);
}
v("DEBUG generate rc=0 且文件生成", $drc == 0 && -f "$kdbg/dbg_l.key");
v("DEBUG stderr 出现 [DEBUG] ... PBKDF2 日志",
    $de =~ /\[DEBUG\].*PBKDF2:/);

# ============================================================
# 测试 4 (L177): info 严格四字段（key_algo/Algorithm/Created/Format）
# ============================================================
note("\n=== 测试4: info 字段对齐需求（L177）===");
($rc, $o, $e) = run_ktool('info', '--key-path', "$kdir/kek_sm4.key");
$t = out_err($o, $e);
v("info .key: key_algo=sm4", $rc == 0 && $t =~ /key_algo:\s+sm4/);
v("info .key: Algorithm=SM4-CBC+PBKDF2",
    $t =~ /Algorithm:\s+SM4-CBC \+ PBKDF2-HMAC-SHA256/);
v("info .key: Created 时间戳", $t =~ /Created:\s+\d{4}-\d\d-\d\d/);
v("info .key: Format=TECCKEK v1", $t =~ /Format:\s+TECCKEK v1/);
v("info .key: 无 Size/Key Type/File 残留行",
    $t !~ /^Size:/m && $t !~ /Key Type/ && $t !~ /^File:/m);

($rc, $o, $e) = run_ktool('info', '--key-path', "$kdir/kek_sm2.priv");
$t = out_err($o, $e);
v("info .priv: key_algo=sm2", $rc == 0 && $t =~ /key_algo:\s+sm2/);
v("info .priv: 算法同为文件保护算法",
    $t =~ /Algorithm:\s+SM4-CBC \+ PBKDF2-HMAC-SHA256/);
v("info .priv: Format=TECCKEK v1", $t =~ /Format:\s+TECCKEK v1/);

# ============================================================
# 测试 5 (L590): validate/info 支持 .pub 裸 SM2 公钥
# ============================================================
note("\n=== 测试5: .pub 公钥 validate/info（L590）===");
($rc, $o, $e) = run_ktool('validate', '--key-path', "$kdir/kek_sm2.pub");
$t = out_err($o, $e);
v("validate .pub 完好 → rc=0", $rc == 0);
v("validate .pub 报 Format/SM2 明文 + on-curve",
    $t =~ /Format: SM2 public key \(plain, no HMAC\)/ && $t =~ /on-curve/);

($rc, $o, $e) = run_ktool('validate', '--key-path', "$kdir/kek_sm4.key");
$t = out_err($o, $e);
v("validate .key（信封）仍走 HMAC 校验", $rc == 0 && $t =~ /HMAC:\s+verified/);

($rc, $o, $e) = run_ktool('info', '--key-path', "$kdir/kek_sm2.pub");
$t = out_err($o, $e);
v("info .pub rc=0 且 key_algo=sm2 (public)",
    $rc == 0 && $t =~ /key_algo:\s+sm2 \(public\)/);
v("info .pub: Algorithm=n/a 明文公钥",
    $t =~ /Algorithm:\s+n\/a — public key, plain text \(no HMAC\)/);
v("info .pub: Format=裸点 + Size 65",
    $t =~ /Format:\s+SM2 public key \(uncompressed point\)/ && $t =~ /Size:\s+65 bytes/);

# 篡改 .pub：X 坐标翻转 1 比特 → 出曲线
my $flip_pub = "$kdir/flip.pub";
copy("$kdir/kek_sm2.pub", $flip_pub) or die "copy: $!";
{
    open(my $fh, '+<', $flip_pub) or die "open $flip_pub: $!";
    binmode($fh);
    seek($fh, 20, 0);
    my $byte;
    read($fh, $byte, 1) == 1 or die "read: $!";
    my $flipped = chr(ord($byte) ^ 0x01);
    seek($fh, 20, 0);
    print $fh $flipped;
    close($fh);
}
($rc, $o, $e) = run_ktool('validate', '--key-path', $flip_pub);
$t = out_err($o, $e);
v("validate .pub 等长比特翻转 → rc=1 报出曲线",
    $rc == 1 && $t =~ /point is not on the sm2p256v1 curve \(file tampered\?\)/);

# 篡改 .pub：覆写为短文本 → bad length
my $short_pub = "$kdir/short.pub";
open(my $sfh, '>', $short_pub) or die "open $short_pub: $!";
print $sfh 'hello world';
close($sfh);
($rc, $o, $e) = run_ktool('validate', '--key-path', $short_pub);
$t = out_err($o, $e);
v("validate .pub 覆写短文本 → rc=1 报 bad length",
    $rc == 1 && $t =~ /bad length \(11 bytes, expected 65-byte uncompressed SM2 point\)/);

# 篡改 .key：尾部追加 → HMAC 仍能检出（信封路径行为不变）
my $app_key = "$kdir/app.key";
copy("$kdir/kek_sm4.key", $app_key) or die "copy: $!";
open(my $afh, '>>', $app_key) or die "open $app_key: $!";
print $afh 'X';
close($afh);
($rc, $o, $e) = run_ktool('validate', '--key-path', $app_key);
$t = out_err($o, $e);
v("validate .key 尾部追加 → HMAC unverified",
    $t =~ /HMAC:\s+unverified/);

# ============================================================
# 测试 6 (L141): delete 各自独立销毁，不再连带兄弟文件
# ============================================================
note("\n=== 测试6: delete 独立性（L141）===");
gen('sm2', 'p1', $kdel);
gen('sm2', 'p2', $kdel);
gen('sm4', 's1', $kdel);

($rc, $o, $e) = run_ktool('delete', '--key-path', "$kdel/p1.priv");
$t = out_err($o, $e);
v("delete .priv rc=0 + Destroyed 覆写提示",
    $rc == 0 && $t =~ /Destroyed: .*overwritten with 4 passes before removal/);
v("delete .priv 后 .priv 消失", ! -f "$kdel/p1.priv");
v("delete .priv 后 .pub 保留（不再连带）", -f "$kdel/p1.pub");

($rc, $o, $e) = run_ktool('delete', '--key-path', "$kdel/p2.pub");
$t = out_err($o, $e);
v("delete .pub rc=0 + Destroyed 覆写提示",
    $rc == 0 && $t =~ /Destroyed: .*overwritten with 4 passes before removal/);
v("delete .pub 后 .pub 消失", ! -f "$kdel/p2.pub");
v("delete .pub 后 .priv 保留（不对称连带已去除）", -f "$kdel/p2.priv");

($rc, $o, $e) = run_ktool('delete', '--key-path', "$kdel/s1.key");
$t = out_err($o, $e);
v("delete .key rc=0 且文件消失", $rc == 0 && ! -f "$kdel/s1.key");

($rc, $o, $e) = run_ktool('delete', '--key-path', "$kdel/nope.key");
$t = out_err($o, $e);
v("delete 不存在文件 → rc=1 明确报错", $rc == 1 && $t =~ /No such file or directory/);

# ============================================================
# 清理
# ============================================================
$node->stop('fast');

print $TAP "\n# ====================================\n";
print $TAP "# $ok_cnt of $total tecc_ktool CLI tests passed\n";
print $TAP "# ====================================\n";
print $TAP "1..$total\n";
