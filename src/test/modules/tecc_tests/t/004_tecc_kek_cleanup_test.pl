#!/usr/bin/perl

#
# 004_tecc_kek_cleanup_test.pl
#
# TECC KEK 文件客户端删除回归测试
#
# 核心验证策略：服务端已移除所有 delete_kek_file() 调用，
# 因此 .key 文件被删除的唯一途径是客户端 libpq 的
# fe-tecc-integration.c 中的 unlink()。测试通过对比
# 「TECC关闭时 DROP」vs「TECC开启时 DROP」来证明：
#
#   enable_tecc=0: 客户端不拦截，文件残留（证明服务端不删）
#   enable_tecc=1: 客户端拦截并 unlink，文件消失（客户端在删）
#

use strict;
use warnings;
use Config;
use PostgresNode;
use TestLib;
use Test::More;

use FindBin;
use lib $FindBin::Bin;
use tecc_test_utils;

my $TAP = Test::More->builder->output;

note("");
note("╔═════════════════════════════════╗");
note("║  TECC KEK 文件删除回归测试       ║");
note("╚═════════════════════════════════╝");

my $node = get_new_node('tecc_kek_cleanup');
$node->init;
$node->start;

my ($key_dir, $tmp_dir) = setup_tecc_regress_env();

my $basedir = $key_dir;
my $connstr = $node->connstr('postgres');

# 所有 KEK 文件路径（放tmp_check下，测试框架自动清理）
my $kek_prove_off = "$basedir/kek_prove_off.key";
my $kek_prove_on  = "$basedir/kek_prove_on.key";
my $kek_explicit  = "$basedir/kek_explicit.key";
my $kek_cascade   = "$basedir/kek_cascade.key";
my $kek_s1_a      = "$basedir/kek_s1_a.key";
my $kek_s1_b      = "$basedir/kek_s1_b.key";
my $kek_s2        = "$basedir/kek_s2.key";
my $kek_keep      = "$basedir/kek_keep.key";
my $kek_dek_test  = "$basedir/kek_dek_test.key";
my $kek_new_conn  = "$basedir/kek_new_conn.key";

sub file_exists { return -f $_[0]; }

# helper: 带 enable_tecc 参数的 psql 执行
sub psql_tecc {
    my ($sql, $tecc_val, %params) = @_;
    my $c = "$connstr enable_tecc=$tecc_val";
    $node->safe_psql('postgres', $sql,
        connstr => $c,
        %params,
    );
}

# 仅 CREATE KEK 必须用 enable_tecc=1（客户端生成 key 文件）
# DROP KEK 控制 enable_tecc 来验证文件行为
sub psql_tecc_on  { psql_tecc($_[0], 1, @_); }   # 启用客户端拦截
sub psql_tecc_off { psql_tecc($_[0], 0, @_); }   # 不启用客户端拦截

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

# ============================================================
# 测试0: 证明文件删除是客户端行为（而非服务端）
# ============================================================
note("\n=== 测试0: 证明文件删除来自客户端（服务端已不删）===");

psql_tecc_on(qq{
    CREATE KEK kek_prove_off WITH (
        key_store = 'localkms',
        key_path = '$kek_prove_off'
    );
});
v("创建KEK后文件存在", file_exists($kek_prove_off));

# TECC 关闭时 DROP：客户端不拦截，看文件是否残留
psql_tecc_off("DROP KEK kek_prove_off;");
v("TECC关闭时DROP KEK → 文件残留（服务端不删）",
    file_exists($kek_prove_off));

# 重新创建，TECC 开启时 DROP：客户端拦截，文件应消失
psql_tecc_on(qq{
    CREATE KEK kek_prove_on WITH (
        key_store = 'localkms',
        key_path = '$kek_prove_on'
    );
});
v("再次创建KEK后文件存在", file_exists($kek_prove_on));

psql_tecc_on("DROP KEK kek_prove_on;");
v("TECC开启时DROP KEK → 文件已删（客户端删的）",
    !file_exists($kek_prove_on));

# 清理文件残留
unlink($kek_prove_off);

# ============================================================
# 测试 1: 显式 DROP KEK
# ============================================================
note("\n=== 测试1: 显式 DROP KEK ===");

psql_tecc_on(qq{
    CREATE KEK kek_explicit WITH (
        key_store = 'localkms',
        key_path = '$kek_explicit'
    );
});
v("显式DROP: 创建后文件存在", file_exists($kek_explicit));
psql_tecc_on("DROP KEK kek_explicit;");
v("显式DROP: DROP KEK后文件已删除", !file_exists($kek_explicit));

# ============================================================
# 测试 2: DROP KEK CASCADE
# ============================================================
note("\n=== 测试2: DROP KEK CASCADE ===");

psql_tecc_on(qq{
    CREATE KEK kek_cascade WITH (
        key_store = 'localkms',
        key_path = '$kek_cascade'
    );
});
v("DROP KEK CASCADE: 创建后文件存在", file_exists($kek_cascade));
psql_tecc_on("DROP KEK kek_cascade CASCADE;");
v("DROP KEK CASCADE: 删除后文件已删除", !file_exists($kek_cascade));

# ============================================================
# 测试 3: DROP SCHEMA CASCADE（多KEK）
# ============================================================
note("\n=== 测试3: DROP SCHEMA CASCADE ===");

psql_tecc_on(qq{
    CREATE SCHEMA s1;
    SET search_path TO s1;
    CREATE KEK kek_s1_a WITH (
        key_store = 'localkms',
        key_path = '$kek_s1_a'
    );
    CREATE KEK kek_s1_b WITH (
        key_store = 'localkms',
        key_path = '$kek_s1_b'
    );
    RESET search_path;
});
v("DROP SCHEMA CASCADE: s1/kek_s1_a文件存在", file_exists($kek_s1_a));
v("DROP SCHEMA CASCADE: s1/kek_s1_b文件存在", file_exists($kek_s1_b));
psql_tecc_on("DROP SCHEMA s1 CASCADE;");
v("DROP SCHEMA CASCADE: s1/kek_s1_a已删除", !file_exists($kek_s1_a));
v("DROP SCHEMA CASCADE: s1/kek_s1_b已删除", !file_exists($kek_s1_b));

# 这里也验证：用 TECC 关闭再查一次，确保文件真的不在了（不是缓存问题）
psql_tecc_off("SELECT 1;");  # just to prove connection works without TECC
v("DROP SCHEMA CASCADE后文件确实不在了", !file_exists($kek_s1_a));

# ============================================================
# 测试 4: 其他schema KEK 不受 CASCADE 影响（误删防护）
# ============================================================
note("\n=== 测试4: CASCADE误删防护 ===");

psql_tecc_on(qq{
    CREATE SCHEMA s2;
    SET search_path TO s2;
    CREATE KEK kek_s2 WITH (
        key_store = 'localkms',
        key_path = '$kek_s2'
    );
    RESET search_path;
    CREATE KEK kek_keep WITH (
        key_store = 'localkms',
        key_path = '$kek_keep'
    );
});
v("误删防护: s2/kek_s2文件存在", file_exists($kek_s2));
v("误删防护: public/kek_keep文件存在", file_exists($kek_keep));
psql_tecc_on("DROP SCHEMA s2 CASCADE;");
v("误删防护: s2 CASCADE后s2/kek_s2已删除", !file_exists($kek_s2));
v("误删防护: s2 CASCADE后public/kek_keep未受波及", file_exists($kek_keep));
psql_tecc_off("SELECT 1;");
v("误删防护: 用TECC关闭确认keep仍存在", file_exists($kek_keep));

# ============================================================
# 测试 5: DROP TABLE CASCADE 不应触发 KEK 文件删除
# ============================================================
note("\n=== 测试5: DROP TABLE CASCADE不误触 ===");

psql_tecc_on(qq{
    CREATE KEK kek_dek_test WITH (
        key_store = 'localkms',
        key_path = '$kek_dek_test'
    );
    CREATE DEK dek_test WITH VALUES(KEK = kek_dek_test);
    CREATE TABLE t_test (id INT4_ENC ENCRYPTED WITH (DEK = dek_test));
});
v("DROP TABLE CASCADE: KEK文件未被误删", file_exists($kek_dek_test));
psql_tecc_on("DROP TABLE t_test CASCADE;");
v("DROP TABLE CASCADE后KEK文件不受影响", file_exists($kek_dek_test));
psql_tecc_on("DROP KEK kek_dek_test CASCADE;");
v("正确清理DEK的KEK文件", !file_exists($kek_dek_test));

# ============================================================
# 测试 6: 新连接也能正确删除 KEK 文件
# ============================================================
note("\n=== 测试6: 新连接 ===");

psql_tecc_on(qq{
    CREATE KEK kek_new_conn WITH (
        key_store = 'localkms',
        key_path = '$kek_new_conn'
    );
});
v("新连接: 创建后文件存在", file_exists($kek_new_conn));
psql_tecc_on("DROP KEK kek_new_conn;");
v("新连接: DROP KEK后文件已删除", !file_exists($kek_new_conn));

# ============================================================
# 清理
# ============================================================
psql_tecc_on(qq{
    DROP KEK IF EXISTS kek_keep CASCADE;
    DROP SCHEMA IF EXISTS s1 CASCADE;
    DROP SCHEMA IF EXISTS s2 CASCADE;
});

$node->stop('fast');

print $TAP "\n# ====================================\n";
print $TAP "# $ok_cnt of $total KEK cleanup tests passed\n";
print $TAP "# ====================================\n";
print STDOUT "1..$total\n";
