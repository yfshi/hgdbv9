#!/usr/bin/perl

#
# 005_tecc_priv_deletion_test.pl
#
# TECC 非对称 KEK 私钥（.priv）删除回归测试
#
# 需求（V4.0.1 非对称密钥管理）：创建 DEK（含同态 HE DEK）要求所绑非对称 KEK
# 私钥可用——软加密 localkms 下 .priv 文件缺失（被删除/丢失）时 CREATE DEK 报错。
#
# 核心验证策略：
#   1. tecc_ktool 生成 sm2 密钥对（.priv + .pub），CREATE KEK 登记元数据
#   2. 基线：.priv 存在时 CREATE DEK (he-add) 成功
#   3. 删除 .priv → 新连接（空缓存）CREATE DEK (he-add) 报错
#   4. 重新生成 .priv → 新连接 CREATE DEK 再次成功（证明报错确因 .priv 缺失）
#
# 注：每次 psql 调用都是独立连接、独立 KEK 明文缓存，因此删除 .priv 后
# 新连接的 CREATE DEK 必然重新读文件，能稳定复现缺失报错。
#

use strict;
use warnings;
use Config;
use Test::More;
use PostgresNode;
use TestLib;

use FindBin;
use lib $FindBin::Bin;

my $TAP = Test::More->builder->output;

note("");
note("╔═════════════════════════════════════════╗");
note("║  TECC 非对称 KEK 私钥(.priv)删除测试    ║");
note("╚═════════════════════════════════════════╝");

my $node = get_new_node('tecc_priv_deletion');
$node->init;
$node->start;

my $basedir = "$TestLib::tmp_check";
my $connstr = $node->connstr('postgres');

my $kek_sm2_priv = "$basedir/kek_he.priv";
my $kek_sm2_pub  = "$basedir/kek_he.pub";

# 定位 tecc_ktool（with_temp_install 已将其装入 PATH）
my $ktool;
for my $p (split /:/, $ENV{PATH}) {
    next unless $p && $p ne '';
    if (-x "$p/tecc_ktool") { $ktool = "$p/tecc_ktool"; last; }
}
die "tecc_ktool not found in PATH\n" unless $ktool;

# 生成 sm2 密钥对（.priv 私钥 + .pub 公钥）
sub gen_sm2_kek {
    my $rc = system($ktool, 'generate', '--key-name', 'kek_he',
                    '--key-algo', 'sm2', '--key-path', $basedir);
    die "tecc_ktool generate sm2 failed (rc=$rc)\n" if $rc != 0;
}
gen_sm2_kek();
die "sm2 .priv not generated\n" unless -f $kek_sm2_priv;
die "sm2 .pub not generated\n" unless -f $kek_sm2_pub;

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

# 带 enable_tecc 的 psql 执行（每次调用都是新连接、新 KEK 明文缓存）。
# 返回 (rc, stdout, stderr)；错误时 rc != 0（ON_ERROR_STOP）。
sub psql_tecc {
    my ($sql, $tecc_val) = @_;
    return $node->psql('postgres', $sql,
        connstr => "$connstr enable_tecc=$tecc_val");
}

# ============================================================
# 测试1: 基线——.priv 存在时 CREATE DEK (he-add) 成功
# ============================================================
note("\n=== 测试1: 基线 .priv 存在 → CREATE DEK 成功 ===");

my ($rc, $out, $err) = psql_tecc(qq{
    CREATE KEK kek_he WITH (
        key_store = 'localkms',
        key_algo  = 'sm2',
        key_path  = '$kek_sm2_priv'
    );
}, 1);
v("CREATE KEK (sm2) 成功", $rc == 0);

($rc, $out, $err) = psql_tecc(qq{
    CREATE DEK he_dek WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
}, 1);
v("基线: CREATE DEK (he-add) 成功", $rc == 0);

# ============================================================
# 测试2: 删除 .priv → 新连接 CREATE DEK 报错
# ============================================================
note("\n=== 测试2: 删除 .priv → CREATE DEK 报错 ===");

unlink($kek_sm2_priv);
v(".priv 文件已删除", !-f $kek_sm2_priv);

($rc, $out, $err) = psql_tecc(qq{
    CREATE DEK he_dek2 WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
}, 1);
v("删除 .priv 后 CREATE DEK 报错", $rc != 0);
v("报错信息含 'private key is unavailable'",
    defined($err) && $err =~ /private key is unavailable/);

# ============================================================
# 测试3: 重新生成 .priv → 新连接 CREATE DEK 再次成功
# ============================================================
note("\n=== 测试3: 恢复 .priv → CREATE DEK 再次成功 ===");

# tecc_ktool 拒绝覆盖已存在文件，需先删掉残留的 .pub 再重新生成
unlink($kek_sm2_pub);
gen_sm2_kek();
v(".priv 已恢复", -f $kek_sm2_priv);

($rc, $out, $err) = psql_tecc(qq{
    CREATE DEK he_dek3 WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
}, 1);
v("恢复 .priv 后 CREATE DEK 成功", $rc == 0);

# ============================================================
# 测试4: 同一连接内删除 .priv → CREATE DEK 报错（绕过连接级缓存）
# ============================================================
note("\n=== 测试4: 同一连接内删除 .priv → CREATE DEK 报错（绕过缓存） ===");

# 用同一个持久连接：先成功 CREATE DEK 使 KEK 明文进入连接级缓存，
# 再在同一连接里删除 .priv 后 CREATE DEK——必须报错（强制重读文件），
# 不允许命中缓存继续创建出无法解密的 DEK。
my $bg = $node->background_psql('postgres',
    extra_params => ['-d', "$connstr enable_tecc=1"],
    on_error_stop => 0);
$bg->query_safe(qq{
    CREATE DEK d_same1 WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
});
v("基线: 同连接 CREATE DEK 成功（KEK 明文已缓存）", 1);

unlink($kek_sm2_priv);
v("同一连接内 .priv 已删除", !-f $kek_sm2_priv);

my ($out4) = $bg->query(qq{
    CREATE DEK d_same2 WITH VALUES(KEK = kek_he, KEY_ALGO = 'he-add');
});
my $err4 = $bg->{stderr};
v("同连接删除 .priv 后 CREATE DEK 报错（绕过缓存）",
    defined($err4) && $err4 =~ /private key is unavailable/);
$bg->quit;

# ============================================================
# 清理
# ============================================================
psql_tecc(qq{
    DROP DEK IF EXISTS he_dek CASCADE;
    DROP DEK IF EXISTS he_dek2 CASCADE;
    DROP DEK IF EXISTS he_dek3 CASCADE;
    DROP DEK IF EXISTS d_same1 CASCADE;
    DROP DEK IF EXISTS d_same2 CASCADE;
    DROP KEK IF EXISTS kek_he CASCADE;
}, 1);

$node->stop('fast');

print $TAP "\n# ====================================\n";
print $TAP "# $ok_cnt of $total priv-deletion tests passed\n";
print $TAP "# ====================================\n";
print $TAP "1..$total\n";
