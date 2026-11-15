#!/usr/bin/perl

#
# 004_tecc_kek_cleanup_test.pl
#
# TECC 密钥文件生命周期回归测试
#
# 核心策略：密钥文件由 tecc_ktool 管理（generate 生成、delete 覆写销毁），
# SQL DROP KEK / DROP SCHEMA|OWNED ... CASCADE 只清理目录元数据 + 客户端
# 元数据缓存，【不再 unlink 本地密钥文件】。因此无论 enable_tecc 开/关、
# 无论 DROP KEK 是否 CASCADE，密钥文件都必须保留；销毁职责唯一落在
# tecc_ktool delete 上。
#
#   DROP KEK（TECC 开/关）  → 文件保留（服务端与客户端都不删）
#   DROP KEK ... CASCADE    → 文件保留
#   DROP SCHEMA ... CASCADE → 相关 KEK 文件保留、无关 KEK 不受影响
#   DROP TABLE  ... CASCADE → 不误触 KEK 文件
#   tecc_ktool delete       → 文件被覆写销毁（唯一的销毁入口）
#

use strict;
use warnings;
use Config;
use Test::More;
use PostgresNode;
use TestLib;
use Digest::SHA qw(sha256_hex);

use FindBin;
use lib $FindBin::Bin;

my $TAP = Test::More->builder->output;

note("");
note("╔═════════════════════════════════╗");
note("║  TECC 密钥文件生命周期回归测试   ║");
note("╚═════════════════════════════════╝");

my $node = get_new_node('tecc_kek_cleanup');
$node->init;
$node->start;

my $basedir = "$TestLib::tmp_check";
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
my $kek_ktool_del = "$basedir/kek_ktool_del.key";

sub file_exists { return -f $_[0]; }

# 文件内容 SHA-256（二进制安全），用于验证 CREATE KEK 未改写已存在密钥文件
sub _file_sha256 {
    my ($path) = @_;
    open(my $fh, '<', $path) or die "Cannot read $path: $!";
    binmode($fh);
    local $/;   # slurp
    my $data = <$fh>;
    close($fh);
    return sha256_hex($data);
}

# helper: 带 enable_tecc 参数的 psql 执行
sub psql_tecc {
    my ($sql, $tecc_val, %params) = @_;
    my $c = "$connstr enable_tecc=$tecc_val";
    $node->safe_psql('postgres', $sql,
        connstr => $c,
        %params,
    );
}

# ReqID:TECC4.0 - CREATE KEK 不生成密钥文件（Review 问题10 需求对齐），
# 密钥文件必须由 tecc_ktool generate 预生成。
my $ktool;
for my $p (split /:/, $ENV{PATH}) {
    next unless $p && $p ne '';
    if (-x "$p/tecc_ktool") { $ktool = "$p/tecc_ktool"; last; }
}
die "tecc_ktool not found in PATH\n" unless $ktool;

sub gen_kek {
    my ($path) = @_;
    (my $dir = $path) =~ s{/[^/]*$}{};
    (my $kname = $path) =~ s{^.*/}{};
    $kname =~ s/\.[^.]*$//;
    my $rc = system($ktool, 'generate', '--key-name', $kname,
                    '--key-algo', 'sm4', '--key-path', $dir);
    die "tecc_ktool generate $path failed (rc=$rc)\n" if $rc != 0;
}

# 预生成本脚本全部 KEK 文件
for my $k (qw(kek_prove_off kek_prove_on kek_explicit kek_cascade
              kek_s1_a kek_s1_b kek_s2 kek_keep kek_dek_test kek_new_conn
              kek_ktool_del)) {
    gen_kek("$basedir/$k.key");
}

# 仅 CREATE KEK 必须用 enable_tecc=1（客户端预生成校验/登记）
# DROP KEK 控制 enable_tecc 来验证：无论开/关都不删文件
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
# 测试0: DROP KEK 不删文件 —— 服务端与客户端都不删（TECC 开/关一致）
# ============================================================
note("\n=== 测试0: DROP KEK 不删文件（TECC 开/关一致）===");

# 注：CREATE KEK 服务端要求 enable_tecc=on，两个 KEK 都须在 TECC 开启下创建；
# 区别仅在 DROP 时的 enable_tecc —— 关闭时客户端完全不拦截（服务端本就不删文件），
# 开启时客户端拦截但按新策略也不再 unlink → 两种路径文件都必须保留。
psql_tecc_on(qq{
    CREATE KEK kek_prove_off WITH (
        key_store = 'localkms',
        key_path = '$kek_prove_off'
    );
});
v("创建KEK(TECC ON)后文件存在", file_exists($kek_prove_off));

# TECC 关闭时 DROP：客户端不拦截 → 文件必然保留
psql_tecc_off("DROP KEK kek_prove_off;");
v("TECC关闭时DROP KEK → 文件保留（服务端本就不删）",
    file_exists($kek_prove_off));

# TECC 开启时 DROP：客户端不再 unlink → 文件同样保留
psql_tecc_on(qq{
    CREATE KEK kek_prove_on WITH (
        key_store = 'localkms',
        key_path = '$kek_prove_on'
    );
});
v("创建KEK(TECC ON)后文件存在", file_exists($kek_prove_on));
psql_tecc_on("DROP KEK kek_prove_on;");
v("TECC开启时DROP KEK → 文件保留（不再客户端删除）",
    file_exists($kek_prove_on));

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
v("显式DROP: DROP KEK后文件保留", file_exists($kek_explicit));

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
v("DROP KEK CASCADE: 删除后文件保留", file_exists($kek_cascade));

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
v("DROP SCHEMA CASCADE: s1/kek_s1_a文件保留", file_exists($kek_s1_a));
v("DROP SCHEMA CASCADE: s1/kek_s1_b文件保留", file_exists($kek_s1_b));

# ============================================================
# 测试 4: 其他schema KEK 不受 CASCADE 影响
# ============================================================
note("\n=== 测试4: CASCADE 不影响其他 KEK ===");

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
v("其他KEK: s2/kek_s2文件存在", file_exists($kek_s2));
v("其他KEK: public/kek_keep文件存在", file_exists($kek_keep));
psql_tecc_on("DROP SCHEMA s2 CASCADE;");
v("其他KEK: s2 CASCADE后s2/kek_s2文件保留", file_exists($kek_s2));
v("其他KEK: public/kek_keep未受波及", file_exists($kek_keep));

# ============================================================
# 测试 5: DROP TABLE CASCADE 不应影响 KEK 文件
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
v("DROP TABLE CASCADE: KEK文件存在", file_exists($kek_dek_test));
psql_tecc_on("DROP TABLE t_test CASCADE;");
v("DROP TABLE CASCADE后KEK文件保留", file_exists($kek_dek_test));
psql_tecc_on("DROP KEK kek_dek_test CASCADE;");
v("显式清理DEK的KEK后文件保留", file_exists($kek_dek_test));

# ============================================================
# 测试 6: 新连接 DROP KEK 同样不删文件
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
v("新连接: DROP KEK后文件保留", file_exists($kek_new_conn));

# ============================================================
# 测试 7: CREATE KEK 不生成文件（Review 问题10 需求对齐）
# ============================================================
note("\n=== 测试7: CREATE KEK 不生成文件 ===");

# 7a. 指向不存在的文件 → CREATE KEK 必须报错（不自动生成）
my $kek_noautogen = "$basedir/kek_noautogen.key";
my ($no_rc, $no_out, $no_err) = $node->psql('postgres',
    qq{CREATE KEK kek_noautogen WITH (
        key_store = 'localkms',
        key_path = '$kek_noautogen'
    );},
    connstr => "$connstr enable_tecc=1");
v("CREATE KEK 指向不存在文件 → 报错（不自动生成）",
    $no_rc != 0 && $no_err =~ /key file not found/);
v("CREATE KEK 报错后未生成文件", !file_exists($kek_noautogen));

# 7b. 指向已存在的 tecc_ktool 文件 → 成功且不新增/不改写文件（只登记元数据）
my $kek_preexist = "$basedir/kek_preexist.key";
gen_kek($kek_preexist);
my $pre_hash = _file_sha256($kek_preexist);
my $files_before = scalar(grep { /\.key$/ } glob("$basedir/*.key"));
psql_tecc_on(qq{
    CREATE KEK kek_preexist WITH (
        key_store = 'localkms',
        key_path = '$kek_preexist'
    );
});
my $files_after = scalar(grep { /\.key$/ } glob("$basedir/*.key"));
v("CREATE KEK 指向已存在文件 → 成功（仅登记元数据）", 1);
v("CREATE KEK 未新增任何 .key 文件", $files_after == $files_before);
v("CREATE KEK 未改写已存在文件内容", _file_sha256($kek_preexist) eq $pre_hash);
psql_tecc_on("DROP KEK kek_preexist;");

# 7c. 指向存在的空文件 → 报错（密钥文件必须非空，不允许以空文件登记）
my $kek_empty = "$basedir/kek_empty.key";
open(my $efh, '>', $kek_empty) or die "Cannot create empty file $kek_empty: $!";
close($efh);
my ($ec_rc, $ec_out, $ec_err) = $node->psql('postgres',
    qq{CREATE KEK kek_empty WITH (
        key_store = 'localkms',
        key_path = '$kek_empty'
    );},
    connstr => "$connstr enable_tecc=1");
v("CREATE KEK 指向空文件 → 报错（文件必须非空）",
    $ec_rc != 0 && $ec_err =~ /empty/);
v("CREATE KEK 报错后空文件保持为空", -s $kek_empty == 0);
unlink($kek_empty);

# ============================================================
# 测试 8: 销毁职责唯一在 tecc_ktool delete（覆写销毁）
# ============================================================
note("\n=== 测试8: tecc_ktool delete 是唯一销毁入口 ===");

psql_tecc_on(qq{
    CREATE KEK kek_ktool_del WITH (
        key_store = 'localkms',
        key_path = '$kek_ktool_del'
    );
});
v("ktool delete: 创建后文件存在", file_exists($kek_ktool_del));
my $del_rc = system($ktool, 'delete', '--key-path', $kek_ktool_del);
v("ktool delete 执行成功（rc=0）", $del_rc == 0);
v("ktool delete 后文件已被销毁", !file_exists($kek_ktool_del));

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
print $TAP "# $ok_cnt of $total KEK file-lifecycle tests passed\n";
print $TAP "# ====================================\n";
print $TAP "1..$total\n";
