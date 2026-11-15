package tecc_test_utils;

#
# TECC回归测试公共工具模块
# 提供: PG节点管理、SQL执行比对、JDBC构建等公共函数
#

use strict;
use warnings;
use Exporter 'import';
use PostgresNode;
use TestLib;
use Test::More;

# Set default PG_REGRESS path if not already set — PostgresNode.pm
# uses this for --config-auth during node initialization.
# Without it, TestLib::system_or_bail receives undef → "system  failed".
# Dynamically locate pg_regress via pg_config so this works on any machine.
if (!defined $ENV{PG_REGRESS}) {
    chomp(my $pg_config = `which pg_config 2>/dev/null`);
    if ($pg_config) {
        chomp(my $pgxsdir = `pg_config --pgxs 2>/dev/null`);
        if ($pgxsdir) {
            $ENV{PG_REGRESS} = "$pgxsdir/src/test/regress/pg_regress";
        }
    }
    # Begin - ReqID:TECC4.0
    # pg_regress 是源码树 build 产物（安装目录不含），make check-tecc-*
    # 时 prove_check 已用源码树路径显式设置 PG_REGRESS，不会走到这里。
    # 手动 prove 时若推导路径不存在，直接报错，避免 PostgresNode
    # --config-auth 使用错误版本的 pg_regress。
    if (defined $ENV{PG_REGRESS} && !-f $ENV{PG_REGRESS}) {
        die "PG_REGRESS 指向不存在的文件: $ENV{PG_REGRESS}\n"
            . "请通过 make check-tecc-* 运行回归测试（prove_check 会自动设置 PG_REGRESS）。\n";
    }
    # End - ReqID:TECC4.0
}
use Cwd;
use Time::HiRes qw(gettimeofday tv_interval);
use File::Copy;
use IO::Socket::INET;
use File::Find;
use File::Basename;

# 获取TAP输出句柄（TestLib重定向了STDOUT，TAP输出必须走这里）
my $TAP = Test::More->builder->output;

our @EXPORT = qw(
    run_sql_diff_test
    run_sql_dir_diff
    get_tecc_test_filter
    normalize_psql_output
    run_sql_dir_libpq_dual_diff
    normalize_common_output
    normalize_jdbc_runner_output
    print_failure_summary
    setup_tecc_regress_env
    pregen_localkms_kek_files
    find_stray_test_artifacts
    cleanup_stray_test_artifacts
    setup_swsds_ini
    init_sdf_mode
    sdf_soft_mode
    exclude_hsm_cases
    exclude_jdbc_unsupported_cases
    empty_run_reason
);


# 测试名 → SQL 相对路径映射（供 print_failure_summary 使用）
our %test_sql_label;

# setup_swsds_ini 生成的 HSM 连接配置（hsm.properties）路径。
# SoftSDF 与真实加密机两条路径都会写：JDBC 侧 runner 需要它（tecc_config 连接属性）；
# libpq 侧走 TECC_SDF_LIB / swsds.ini 直接加载动态库，不用该文件。
our $hsm_properties_path;

# 回归的 SDF 模式，由 init_sdf_mode 在 initdb 之前解析一次（判定一律用 sdf_soft_mode()）：
#   非空 → SoftSDF，值为传给 dlopen 的 .so 路径
#   undef → 真实加密机（子进程不带 TECC_SDF_LIB，C 层回落厂商库并探测 swsds.ini）
our $sdf_lib_path;

# ---- 本轮不执行的用例（硬加密用例 + JDBC 测试程序跑不了的用例） ----
#
# ReqID:TECC4.0 - 硬加密（HSM，key_store='swxakms'）用例名单：**唯一**一份。
# 密码机不可用时（未启用 SoftSDF，且真机 swsds.ini/可达性探测失败），这些用例整体
# 不参与执行——由 exclude_hsm_cases 在"读用例清单"处剔除并打印说明。
# 旧实现是把文件改名成 `.sql.skip` 让扫描不到，代价是：运行期间会改动工作区、中断则
# 留下残留、同一份名单要在 setup_swsds_ini 的改名/恢复两处与 restore_swxa_tests 再抄
# 三遍（漏一处就静默失效）。新增硬加密用例时只在本名单加一行即可。
#
# （TZWY/同智伟业 hsmkms 用例已退役：tzhsm_enc.sql 与 swxa_enc.sql 首表逐列重复，
#   已归档到 archive/hsm_tzwy_cases_20260918/；原 jdbc 侧 hsmkms 全覆盖用例已合并为
#   swxa 版，2026-09-20 随 jdbc/ 目录解散迁入 shared/）
our %HSM_CASE = map { $_ => 1 } qw(
    swxa_enc.sql
    swxa_search.sql
    swxa_ope.sql
    swxa_alter_kek.sql
    swxa_concurrent.sql
    swxa_alter_kek_he.sql
    swxa_enc_full_coverage.sql
    swxa_he_parallel.sql
    swxa_multi_kek.sql
    swxa_key_cache.sql
    bug9384_coalesce_hsm.sql
    005_swxa_cross.sql
    008_swxa_he_cross.sql
    009_swxa_he_cross_ops.sql
    010_swxa_he_cross_recheck.sql
);

# JDBC 测试程序（TeccSqlTestRunner）跑不了、只能跳过的用例：{ 文件名 => 原因 }。
# 仅 002（JDBC 入口）在读清单时剔除；同一文件由 psql 执行（001）时不受影响。
our %JDBC_UNSUPPORTED_CASE = (
    'reconnect_schema_dek.sql' =>
        'runner 拆句误把注释中的 \\c 当重连命令（FATAL: role "DEK"）',
);

# 密码机是否可用，由 setup_swsds_ini 判定后置位（1/0）；不可用时 %HSM_CASE 中的用例被剔除。
our $hsm_available = 0;

# 密码机不可用的原因，用于跳过说明；由 setup_swsds_ini 置位，正常路径下为空串。
our $hsm_skip_reason = '';

# 本轮被剔除的用例汇总（形如"用例跳过 3 个（原因）: a.sql, b.sql"），由 _exclude_cases
# 逐次追加；供 empty_run_reason 在清单为空时给报告一句能看懂的原因。
our $skipped_cases_summary = '';

# 内部：按 { 文件名 => 原因 } 从用例清单里剔除不该执行的用例，按原因归并打印一行说明，
# 返回保留的清单。参数可以是纯文件名，也可以是含目录的路径（按文件名末段匹配）。
sub _exclude_cases {
    my ($cases, @files) = @_;

    my (@kept, %skipped);
    for my $f (@files) {
        my $name = basename($f);
        my $reason = $cases->{$name};
        if (defined $reason) {
            push @{ $skipped{$reason} }, $name;
        } else {
            push @kept, $f;
        }
    }

    my $TAP = Test::More->builder->output;
    for my $reason (sort keys %skipped) {
        my @names = @{ $skipped{$reason} };
        my $line = "用例跳过 " . scalar(@names) . " 个（$reason）: "
            . join(', ', @names);
        print $TAP "# $line\n";
        $skipped_cases_summary .= '; ' if $skipped_cases_summary ne '';
        $skipped_cases_summary .= $line;
    }

    return @kept;
}

# ReqID:TECC4.0 - 硬加密用例：密码机不可用时整体从清单中剔除（清单见 %HSM_CASE）。
# 四个入口都必须在"读出用例清单之后、执行之前"调用它。
sub exclude_hsm_cases {
    my (@files) = @_;

    return @files if $hsm_available;

    my %cases = map { $_ => ($hsm_skip_reason || 'HSM unavailable') } keys %HSM_CASE;
    return _exclude_cases(\%cases, @files);
}

# ReqID:TECC4.0 - JDBC 测试程序跑不了的用例（清单见 %JDBC_UNSUPPORTED_CASE）：
# 仅 002 在读清单时调用；同一用例由 psql 执行时不剔除。
sub exclude_jdbc_unsupported_cases {
    my (@files) = @_;

    return _exclude_cases(\%JDBC_UNSUPPORTED_CASE, @files);
}

# ReqID:TECC4.0 - 用例清单被剔空时，给报告一句能看懂的原因。各入口把它打成
# `1..0 # SKIP <原因>`（裸 `1..0` 在报告里只会显示"skipped: (no reason given)"）。
sub empty_run_reason {
    my ($filter) = @_;

    return "无用例可执行：$skipped_cases_summary" if $skipped_cases_summary ne '';
    $filter //= '';
    return "无用例可执行：TECC_TEST=$filter 未匹配到用例文件" if $filter ne '';
    return '无用例可执行：用例目录里没有 .sql 文件';
}

# 获取单用例过滤（TECC_TEST=foo.sql 时只跑这一个）
sub get_tecc_test_filter {
    my $filter = $ENV{'TECC_TEST'} || '';
    return $filter;
}

# ---- 源码目录残留测试产物探测与清理 ----
# 用例里的 \copy TO / pg_dump -f / `> file` 一律使用相对路径，正常由
# run_case_sql_via_psql（以及 JDBC runner）切到 $tmp_check/tmp 落地，
# 且每次运行开头 prove_check 都会 rm -rf tmp_check。
# 一旦切目录逻辑失效（历史上 F-04 重构曾把 tecc_test_utils.pm 的 chdir 整段覆盖掉），
# 这些产物会静默落到模块源码目录，长期污染 git status。故：
#   find_stray_test_artifacts    —— 探测源码目录残留，run_case_sql_via_psql 据此硬失败
#   cleanup_stray_test_artifacts —— 运行初始化时兜底清除上次遗留
# 模式只覆盖用例自己写出的固定文件名，不做通配扩散。
# - ReqID:TECC4.0
our @STRAY_ARTIFACT_PATTERNS = (
    'conc_bg.out',
    'pg_dump_*.sql',
    'copy_*.bin',
    'copy_*.txt',
    'tecc_*.dump',
);

# 模块源码目录 = tmp_check 的父目录
# （prove_check 注入的 TESTDIR 是绝对路径；直接 prove 时为相对路径 "tmp_check"，
#   dirname 得 "."，恰好也是启动目录）
sub _stray_artifact_dir {
    return dirname($TestLib::tmp_check);
}

# 返回源码目录里残留的产物绝对路径（无残留则返回空列表）
sub find_stray_test_artifacts {
    my $dir = _stray_artifact_dir();
    return grep { -f $_ } map { glob("$dir/$_") } @STRAY_ARTIFACT_PATTERNS;
}

# 兜底清除源码目录残留产物，并修复历史遗留的用例文件名；返回处理个数
sub cleanup_stray_test_artifacts {
    my $restored = restore_leftover_skips();

    my @stray = find_stray_test_artifacts();
    return $restored unless @stray;

    my $n = unlink @stray;
    print $TAP "# 清理源码目录残留测试产物 $n 个: "
        . join(', ', map { basename($_) } @stray)
        . "\n";
    return $n + $restored;
}

# 修复历史遗留的用例文件名：旧实现（2026-06 ~ 2026-09）在密码机不可用时把硬加密用例
# 改名成 `xxx.sql.skip`，运行中断就会留在树里；而用例扫描只认 `.sql`，这些用例会永远
# 不执行。这里把三处用例目录下的 `*.sql.skip` 改回 `.sql`。
# 注意是**恢复、不删除**——`.skip` 文件里装的就是用例本体，删掉即丢失用例。
sub restore_leftover_skips {
    my $dir = _stray_artifact_dir();
    my @restored;

    for my $sub (qw(shared/sql libpq/sql cross/sql)) {
        for my $f (glob("$dir/$sub/*.sql.skip")) {
            (my $target = $f) =~ s/\.skip$//;
            next if -e $target;    # 目标文件名已被占用则不覆盖
            push @restored, basename($target) if rename($f, $target);
        }
    }

    print $TAP "# 恢复历史遗留用例文件名 " . scalar(@restored) . " 个（*.sql.skip -> *.sql）: "
        . join(', ', @restored) . "\n"
        if @restored;
    return scalar @restored;
}

# ---- 临时目录与环境变量设置 ----
# 创建 tmp_check/keys (KEK密钥) 和 tmp_check/tmp (导出文件)
# 设置 TECC_KEK_PATH 供C层覆盖 KEY_PATH 目录
sub setup_tecc_regress_env {
    my $key_dir = "$TestLib::tmp_check/keys";
    my $tmp_dir = "$TestLib::tmp_check/tmp";

    cleanup_stray_test_artifacts();

    mkdir $key_dir unless -d $key_dir;
    mkdir $tmp_dir unless -d $tmp_dir;

    $ENV{TECC_KEK_PATH} = $key_dir;

    # Start - ReqID:TECC4.0 (ivorysql 适配)
    # 内网版本 libpq 的 enable_tecc 编译默认值为 false（psql -k 默认 off），
    # 与 tecc 原生 PG14 版（默认 true）不同。用例的 CREATE KEK/DEK 依赖前端
    # 注入 _TECC_PROCESSED，这里显式打开，等价于 psql -k on。
    $ENV{PGENABLETECC} = 'true';
    # End - ReqID:TECC4.0 (ivorysql 适配)

    # Begin - ReqID:TECC4.0
    # 强制使用 temp-install 中由当前源码编译的 pg_dump/pg_restore/psql。
    # make check-tecc-* 时 src/Makefile.global 的 with_temp_install 已把
    # "$(abs_top_builddir)/tmp_install$(bindir)" 注入 PATH 头部。
    # 此处校验并从 PATH 提取 temp-install bin；找不到绝不 fallback 到
    # 其他安装目录，否则回归结果不能反映当前代码。
    my @path_entries = split /:/, $ENV{PATH};
    my $tmp_bin;
    for my $p (@path_entries) {
        next unless defined $p && $p ne '';
        next unless $p =~ /tmp_install/;
        if (-x "$p/pg_dump" && -x "$p/pg_restore" && -x "$p/psql") {
            $tmp_bin = $p;
            last;
        }
    }
    BAIL_OUT(
        "TECC 回归测试必须通过 make check-tecc-* 运行："
            . "PATH 中未找到 temp-install 的 bin 目录"
            . "（需含 tmp_install 且包含 pg_dump/pg_restore/psql）。\n"
            . "请使用: make check-tecc-libpq / make check-tecc-jdbc / make check-tecc-perf-*\n"
            . "当前 PATH: $ENV{PATH}")
        unless defined $tmp_bin;

    # 确保 temp-install/bin 在 PATH 最前（去重），使 psql 内部 \\! 调用的
    # pg_dump/pg_restore 同样使用 temp-install 版本
    $ENV{PATH} = $tmp_bin . ':' . join(':', grep { $_ ne $tmp_bin } @path_entries);
    # End - ReqID:TECC4.0

    return ($key_dir, $tmp_dir);
}

# ---- localkms KEK 文件预生成（需求对齐 Review 问题10）----
# CREATE KEK 不再生成密钥文件：对称（sm4/默认）分支仅校验文件已存在并登记
# 元数据，密钥文件必须由 tecc_ktool generate 预生成。回归用例普遍使用裸
# 绝对路径（/tmp/xxx.key），C 层加载时经 TECC_KEK_PATH 覆盖到 $key_dir/xxx.key，
# CREATE KEK 的存在性校验同样在覆盖路径上（create_kek_processor 的
# effective_path 解析），因此这里扫描 module_dir 下所有 .sql 中
# CREATE/ALTER KEK ... key_store='localkms' ... key_path='...' 的路径，
# 用 tecc_ktool 预生成到 $key_dir。
# 幂等：目标文件已存在则跳过（tecc_ktool generate 拒绝覆盖）。
# 用 %KEKPATH% 占位符的用例（如 homomorphic_enc.sql）自带 \! tecc_ktool
# generate，这里跳过，避免双份生成。
#
# $include_self_gen：JDBC runner 只实现 \c / \copy 两个元命令
# （TeccSqlTestRunner.java:2415），SQL 里自带的 \! tecc_ktool generate 会被
# 静默丢弃。JDBC 路径必须传 1，强制把这类密钥也预生成，否则 CREATE KEK
# 会因缺密钥文件整例分叉（he_acl / he_kek_privkey / homomorphic_enc）。
sub pregen_localkms_kek_files {
    my ($module_dir, $key_dir, $include_self_gen) = @_;

    return unless defined $module_dir && -d $module_dir;
    return unless defined $key_dir;

    # 定位 tecc_ktool（temp-install/bin 在 PATH 头部，见 setup_tecc_regress_env）
    my $ktool;
    for my $p (split /:/, $ENV{PATH}) {
        next unless $p && $p ne '';
        if (-x "$p/tecc_ktool") {
            $ktool = "$p/tecc_ktool";
            last;
        }
    }
    if (!$ktool) {
        die "pregen_localkms_kek_files: tecc_ktool not found in PATH.\n"
            . "CREATE KEK 修复后密钥文件必须由 tecc_ktool generate 预生成，"
            . "请先 make install 或把 tecc_ktool 加入 PATH。\n";
    }

    my @sql_files;
    find(sub { push @sql_files, $File::Find::name if /\.sql$/ }, $module_dir);

    my %seen;
    my $generated = 0;

    for my $f (@sql_files) {
        next unless -f $f;
        my $content = slurp_file($f);
        next unless defined $content;

        # 提取 CREATE/ALTER KEK 块（到首个 ; 结束），块内查 localkms + key_path
        while ($content =~ /\b(?:CREATE|ALTER)\s+KEK\b(.*?);/gis) {
            my $blk = $1;
            next unless $blk =~ /key_store\s*=\s*['"]?localkms['"]?/i;
            next unless $blk =~ /key_path\s*=\s*['"]([^'"]+)['"]/i;
            my $kp = $1;

            (my $base = $kp) =~ s{^.*[\\/]}{};
            next unless $base =~ /\S/;

            # 自足用例（SQL 内自带 \! tecc_ktool generate 同一 key-name）默认跳过，
            # 避免双份生成；其余（含 %KEKPATH% 占位符、无引号 localkms）由框架
            # 预生成，确保 CREATE KEK 修复后（不再自行写盘）密钥文件始终存在。
            # $include_self_gen（JDBC 路径）时不再跳过——runner 丢弃 \!，
            # 不预生成就没有密钥文件。
            (my $kname = $base) =~ s/\.[^.]*$//;   # 去扩展名 → key-name
            next if !$include_self_gen
                && $content =~ /tecc_ktool\s+generate[^\n]*--key-name\s+\Q$kname\E\b/i;

            next if $seen{$base}++;

            next if -f "$key_dir/$base";   # 已生成（幂等）

            # 算法：块内显式 key_algo='sm2' → 非对称，否则对称 sm4
            my $algo = 'sm4';
            if ($blk =~ /key_algo\s*=\s*['"]([^'"]+)['"]/i) {
                $algo = lc($1);
                $algo = 'sm4' if $algo ne 'sm2';
            }

            my $rc = system($ktool, 'generate', '--key-name', $kname,
                            '--key-algo', $algo, '--key-path', $key_dir);
            if ($rc != 0) {
                # tecc_ktool 拒绝覆盖已存在的密钥。同名前缀的其它后缀
                # （.key / .priv / .pub）指向同一份密钥材料：命中即视为已就绪，
                # 不中断预生成（JDBC 路径强制预生成时会同时遇到 .key 与 .pub 两种
                # key_path 写法）。
                next if grep { -f "$key_dir/$kname.$_" } qw(key priv pub);
                die "pregen_localkms_kek_files: tecc_ktool generate "
                    . "($base, algo=$algo) failed (rc=$rc)\n";
            }
            $generated++;
        }
    }

    if ($generated) {
        my $TAP = Test::More->builder->output;
        print $TAP "# pregen localkms KEK: $generated files -> $key_dir\n";
    }
}

# ---- 导出文件绝对路径检查 ----
# SQL脚本中 COPY/\\copy/\\i/\\o 的文件路径必须是相对路径(相对于 tmp_check/tmp)
# KEY_PATH 豁免(C层 TECC_KEK_PATH 覆盖)
sub validate_no_absolute_paths {
    my ($sql_content, $script_name) = @_;
    my @violations;
    my @lines = split /\n/, $sql_content;

    for my $i (0 .. $#lines) {
        my $lineno = $i + 1;
        my $line = $lines[$i];
        next unless defined $line;
        next if $line =~ /^\s*--/;
        next if $line =~ /^\s*$/;

        # KEY_PATH 豁免 — C层 TECC_KEK_PATH 覆盖
        next if $line =~ /KEY_PATH\s*=/i;

        # Server COPY TO|FROM 豁免 — PG 要求绝对路径
        next if $line =~ /\bCOPY\b.*\b(TO|FROM)\s+'\/[^']+'/i;

        # \! 中读取 server COPY 输出文件的路径豁免 (head/cat /tmp/srv_* 等)
        next if $line =~ /\\!\s*(head|cat)\s+.*\/tmp\//;

        # psql \copy ... TO|FROM '/...'
        if ($line =~ /\\copy\b.*\b(TO|FROM)\s+'(\/[^']+)'/i) {
            push @violations, "$script_name:$lineno: \\copy $1 '$2' (must be relative path)";
            next;
        }

        # \i '/...' or \i /...
        if ($line =~ /\\i\s+'(\/[^']+)'/) {
            push @violations, "$script_name:$lineno: \\i '$1' (must be relative path)";
            next;
        }
        if ($line =~ /\\i\s+(\/\S+)/) {
            push @violations, "$script_name:$lineno: \\i $1 (must be relative path)";
            next;
        }

        # \o '/...' or \o /...
        if ($line =~ /\\o\s+'(\/[^']+)'/) {
            push @violations, "$script_name:$lineno: \\o '$1' (must be relative path)";
            next;
        }
        if ($line =~ /\\o\s+(\/\S+)/) {
            push @violations, "$script_name:$lineno: \\o $1 (must be relative path)";
            next;
        }
    }

    return @violations;
}

# 执行一个 SQL 用例：写占位符替换后的临时文件 → 切 CWD 到 tmp_check/tmp 跑 psql →
# 把 psql 输出里的临时文件名还原成用例名。
#   - \\copy, \\i, \\o, \\! 相对路径 → 相对于 tmp_check/tmp
#   - server COPY 相对路径 → 相对于 PG 数据目录(仍在tmp_check下)
#   - %PGDATA% 占位符 → 替换为实例数据目录（server COPY 与 \! 读文件均指向
#     PGDATA，保证不同用户/不同回归运行实例之间互不干扰）
#   - %KEKPATH% → 本回归独立 KEK 目录（TECC_KEK_PATH=tmp_check/keys）。
#     供 CREATE KEK 的 key_path 使用（asym KEK 直接 stat key_path，不做
#     TECC_KEK_PATH 覆盖），保证跨用户/跨机器通用。
# 共享用例（shared/sql/）的执行处与 libpq 单独用例（libpq/sql/）都走这里，
# 保证"只执行一遍"的输出与原先逐字一致。
# 返回 ($stdout, $stderr, $elapsed_ms) - ReqID:TECC4.0
sub run_case_sql_via_psql {
    my ($node, $connstr, $psql_opts, $sql_content, $sql_base, $sql_file) = @_;

    local %ENV = $node->_get_env();
    my ($key_dir, $tmp_dir) = setup_tecc_regress_env();

    my $pp_file = "$tmp_dir/_pp_${sql_base}.sql";
    my $pp_sql = $sql_content;
    my $pgdata = $node->data_dir;
    # 换行符规范化：Windows 检出（core.autocrlf=true）下 .sql 是 CRLF，而 psql 的 \! 元命令
    # 会把行尾的 CR 一并交给 /bin/sh —— 于是 `2>/dev/null` 变成 `2>/dev/null\r`
    # （报 "cannot create /dev/null: Permission denied"）、`>/dev/null 2>&1` 变成 `2>&1\r`
    # （报 "Syntax error: Bad fd number"），密钥生成整段失败并级联到后续 KEK/DEK 用例。
    # 执行前统一成 LF，与 Linux 检出（真正的回归环境）保持一致。
    $pp_sql =~ s/\r\n/\n/g if defined $pp_sql;
    $pp_sql =~ s/%PGDATA%/$pgdata/g if defined $pp_sql;
    $pp_sql =~ s/%KEKPATH%/$key_dir/g if defined $pp_sql;
    open(my $pp_fh, '>', $pp_file) or warn "Cannot write $pp_file: $!";
    print $pp_fh $pp_sql;
    close($pp_fh);

    my $saved_cwd = getcwd();
    # Begin - ReqID:TECC4.0
    # 切目录失败（目录不存在等）必须硬失败：否则 psql 的 CWD 会留在源码目录，
    # 用例里的相对路径产物（\copy TO / pg_dump -f / `> file`）直接写进源码树。
    chdir($tmp_dir)
        or BAIL_OUT("无法切换到回归临时目录 \"$tmp_dir\": $!");
    # End - ReqID:TECC4.0

    my $start = [gettimeofday];
    my ($stdout, $stderr) = run_command([
        $node->installed_command('psql'),
        @$psql_opts, '-d', $connstr, '-f', $pp_file
    ]);
    my $elapsed = int(tv_interval($start) * 1000);

    chdir($saved_cwd);

    # Begin - ReqID:TECC4.0
    # 用例产物必须落在 $tmp_dir。出现在源码目录即说明 psql 的工作目录没生效，
    # 立即失败并报出文件名，避免源码树被持续污染。
    my @stray = find_stray_test_artifacts();
    BAIL_OUT(
        "用例 $sql_file 的相对路径产物落到了源码目录 "
            . "(" . _stray_artifact_dir() . ") 而非 $tmp_dir: "
            . join(', ', map { basename($_) } @stray)
            . "\npsql 工作目录未生效，请检查 run_case_sql_via_psql 的 chdir 逻辑")
        if @stray;
    # End - ReqID:TECC4.0

    # 恢复原始文件名路径(psql NOTICE中显示临时文件名)
    (my $pp_basename = $pp_file) =~ s{.*/}{};
    $stdout =~ s|\Q$pp_basename\E|$sql_file|g if defined $stdout;
    $stderr =~ s|\Q$pp_basename\E|$sql_file|g if defined $stderr;

    return ($stdout, $stderr, $elapsed);
}

# 执行单个SQL文件并diff期望输出
# $canonical=1 时用规范格式（-A -t -q），去除客户端格式差异，仅保留服务端数据
# $precomputed: 可选 hashref { output => 已合并 stdout+stderr 的原始输出, elapsed => 毫秒 }
#   调用方已执行过该 SQL 时传入，直接复用其输出做比较，不再重复执行。 - ReqID:TECC4.0
# 返回值: 1=通过, 0=失败
sub run_sql_diff_test {
    my ($node, $sql_file, $expected_file, $test_name, $canonical, $sql_base, $precomputed) = @_;

    $test_name //= $sql_file;

    my $sql_fullpath;
    if ($sql_file =~ m{^/} || $sql_file =~ m{^[a-zA-Z]:/}) {
        $sql_fullpath = $sql_file;
    } else {
        $sql_fullpath = "$TestLib::tmp_check/$sql_file";
    }

    my $expected_fullpath;
    if ($expected_file =~ m{^/} || $expected_file =~ m{^[a-zA-Z]:/}) {
        $expected_fullpath = $expected_file;
    } else {
        $expected_fullpath = "$TestLib::tmp_check/$expected_file";
    }

    # Fallback to _1.out, _2.out ... when expected file not found
    # (platform-specific variants, e.g. Windows vs Linux output).
    if (! -f $expected_fullpath) {
        for my $n (1 .. 9) {
            my $alt = $expected_fullpath;
            $alt =~ s/\.out$/_$n.out/;
            if (-f $alt) {
                $expected_fullpath = $alt;
                last;
            }
        }
    }

    if (! -f $sql_fullpath) {
        printf "not ok - test %-20s ... FAILED  (SQL file not found)\n", $test_name;
        return 0;
    }

    # ---- 导出文件绝对路径检查 ----
    my $sql_content = slurp_file($sql_fullpath);
    my @violations = validate_no_absolute_paths($sql_content, $sql_file);
    if (@violations) {
        my $pad = " " x (35 - length($test_name));
        print $TAP "not ok - $test_name${pad}... FAILED  (absolute paths not allowed)\n";
        for my $v (@violations) {
            print $TAP "#   $v\n";
        }
        print $TAP "#   导出文件路径请使用相对路径 (CWD = tmp_check/tmp)\n";
        return 0;
    }

    my $connstr = $node->connstr('postgres');
    my @psql_opts = $canonical
        ? ('-X', '-A', '-t')
        : ('-X');

    local %ENV = $node->_get_env();
    my ($key_dir, $tmp_dir) = setup_tecc_regress_env();

    # Begin - ReqID:TECC4.0
    # $precomputed：调用方（run_sql_dir_libpq_dual_diff 里执行共享用例的地方）已经
    # 执行过该 SQL，直接复用其输出与耗时，
    # 不再重复执行 —— 共享用例每次回归只跑一遍。
    my ($stdout, $stderr, $elapsed);
    if (defined $precomputed) {
        $stdout  = $precomputed->{output};
        $elapsed = $precomputed->{elapsed} // 0;
    } else {
        ($stdout, $stderr, $elapsed) =
            run_case_sql_via_psql($node, $connstr, \@psql_opts, $sql_content, $sql_base, $sql_file);
    }
    # End - ReqID:TECC4.0

    my $expected = -f $expected_fullpath ? slurp_file($expected_fullpath) : '';

    # 规范化换行符
    $stdout =~ s/\r\n/\n/g if defined $stdout;
    # expected 侧同样规范化：Windows 检出（core.autocrlf=true）下 golden 文件是 CRLF，而 actual 统一为 LF；
    # 只规范化单侧会让**每个** CRLF golden 在 raw 比较里整体判不等（100+ 假失败）。
    # common 路径（normalize_psql_output 的调用点）与 JDBC 路径（002_tecc_jdbc_test.pl）早已做同样处理，此处补齐。
    $expected =~ s/\r\n/\n/g;

    # 将stderr合并到stdout
    my $actual = $stdout;
    if (defined $stderr && $stderr ne '') {
        $stderr =~ s/\r\n/\n/g;
        $actual .= "\n" . $stderr;
    }

    # 去除尾部空行
    $actual =~ s/\n*\z// if defined $actual;
    $expected =~ s/\n*\z//;

    # 归一化hex密文（每次运行加密结果不同），处理both actual和expected
    # 匹配两种形式:
    #   1) E'\\x{hex}  — 完整的hex字面量
    #   2) {hex}'::type_enc — psql LINE截断后的形式（E'\\x被截掉）
    my @enc_types = qw(int2_enc int4_enc int8_enc float4_enc float8_enc
                       numeric_enc text_enc varchar_enc bpchar_enc);
    my $enc_types_alt = join('|', @enc_types);
    my $hex_full_re = qr/E'\\x[0-9a-fA-F]+/;
    my $hex_bare_re  = qr/\\x[0-9a-fA-F]+/;
    my $hex_trunc_re = qr/([0-9a-fA-F]{16,})'::($enc_types_alt)/;
    my $bare_replacement = "\\x被隐藏的HEX输出";
    my $replacement = "E'\\\\x被隐藏的HEX输出";

    # EXPLAIN Filter格式: \x<hex>E'\x<hex>'::type — 先整体替换避免拆开处理
    my $explain_hex_re = qr/\\x[0-9a-fA-F]+E'\\x[0-9a-fA-F]+/;
    $actual =~ s/$explain_hex_re/$replacement/ge;
    $expected =~ s/$explain_hex_re/$replacement/ge;

    $actual =~ s/$hex_full_re/$replacement/ge;
    $actual =~ s/$hex_bare_re/$bare_replacement/g;
    $actual =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $expected =~ s/$hex_full_re/$replacement/ge;
    $expected =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $expected =~ s/$hex_bare_re/$bare_replacement/g;

    # LINE error context: "...{hex}' WHERE ..." (hex truncated by psql LINE display)
    my $hex_inline_re = qr/([0-9a-fA-F]{16,})' WHERE /;
    $actual =~ s/$hex_inline_re/HEX_STR' WHERE /g;
    $expected =~ s/$hex_inline_re/HEX_STR' WHERE /g;

    # LINE error context: '...{hex}...' (psql truncates with ... after hex)
    # e.g. LINE 1: ... WHERE name LIKE '310000000000010...
    # e.g. LINE 2: WHERE name COLLATE "C" LIKE '2400000000000100a5...
    # psql may truncate the hex to as few as 3 chars before "..."
    # (e.g. "LIKE '240..."), so match {3,} to cover both short and long.
    my $hex_trunc_ellipsis_re = qr/'([0-9a-fA-F]{3,})\.\.\./;
    $actual =~ s/$hex_trunc_ellipsis_re/'HEX_STR.../g;
    $expected =~ s/$hex_trunc_ellipsis_re/'HEX_STR.../g;

    # 归一化psql NOTICE/WARNING/ERROR中的文件路径（不同机器路径不同）
    $actual =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;
    $expected =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;

    # 归一化端口号（每次运行随机分配，出现在 \connect 失败错误中）
    $actual =~ s/port \d+ failed/port NNNN failed/g;
    $expected =~ s/port \d+ failed/port NNNN failed/g;

    # 归一化tecc_key_log输出的时间戳（`[2026-05-25 09:45:24]`，每次运行都不同）
    $actual =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;
    $expected =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;

    # 归一化 KEK 文件绝对路径（不同机器/用户路径不同，出现在 tecc_key 日志的
    # local_kms_load_key FAILED 中：path='/home/<user>/.../kek_xxx.priv'）
    $actual =~ s/path='\/[^']*'/path='PATH'/g;
    $expected =~ s/path='\/[^']*'/path='PATH'/g;

    # ReqID:TECC4.0 - 归一化 TECC ERROR 中的 DEK/KEK OID
    # (dek_oid=16391 每次回归数据库对象 OID 不同，取决于前面用例创建的对象数)
    $actual =~ s/dek_oid=\d+/dek_oid=OID/g;
    $expected =~ s/dek_oid=\d+/dek_oid=OID/g;
    $actual =~ s/kek_oid=\d+/kek_oid=OID/g;
    $expected =~ s/kek_oid=\d+/kek_oid=OID/g;

    # 归一化 pg_dump \restrict / \unrestrict 随机 token
    $actual =~ s/^\\(restrict|unrestrict) [A-Za-z0-9]+$/\\$1 HIDDEN/mg;
    $expected =~ s/^\\(restrict|unrestrict) [A-Za-z0-9]+$/\\$1 HIDDEN/mg;

    # 归一化 pg_restore TOC entry OID (每次运行不同)
    $actual =~ s/from TOC entry \d+; \d+ \d+/from TOC entry NNN; NNN NNN/g;
    $expected =~ s/from TOC entry \d+; \d+ \d+/from TOC entry NNN; NNN NNN/g;

    # Begin - ReqID:TECC4.0
    # 归一化 OS 用户名（pg_dump/pg_restore 输出含对象 Owner，随运行用户变化）
    $actual =~ s/Owner: \S+/Owner: USER/g;
    $expected =~ s/Owner: \S+/Owner: USER/g;
    $actual =~ s/OWNER TO \S+/OWNER TO USER/g;
    $expected =~ s/OWNER TO \S+/OWNER TO USER/g;
    $actual =~ s/member of role "\S+"/member of role "USER"/g;
    $expected =~ s/member of role "\S+"/member of role "USER"/g;
    # pg_restore TOC 条目尾部 owner（OID 已归一化为 NNN; NNN NNN）
    $actual =~ s/^(pg_restore: from TOC entry NNN; NNN NNN .+) (\S+)$/$1 USER/mg;
    $expected =~ s/^(pg_restore: from TOC entry NNN; NNN NNN .+) (\S+)$/$1 USER/mg;
    # End - ReqID:TECC4.0

    # 归一化相对路径的psql NOTICE (\\i 使用相对路径时不带前导/)
    $actual =~ s|psql:([^/\s][^:\s]*\.\w+:\d+: )|psql:/.../$1|g;
    $expected =~ s|psql:([^/\s][^:\s]*\.\w+:\d+: )|psql:/.../$1|g;

    # 归一化回归 KEK 密钥目录绝对路径（不同用户/构建目录不同）。部分前端
    # 错误消息（如 CREATE DEK 的 "public key file is unavailable (localkms:
    # <abs-path> missing)"）打印真实 key_path，该路径 = TECC_KEK_PATH =
    # $TestLib::tmp_check/keys，前缀随机器/用户变化 → 必须归一化后才能落库
    # 期望。现有 expected 均不含该路径，此规则是纯新增、对称应用到 actual/
    # expected，不影响既有用例。
    if (defined $key_dir && $key_dir ne '') {
        $actual   =~ s/\Q$key_dir\E/KEKDIR/g if defined $actual;
        $expected =~ s/\Q$key_dir\E/KEKDIR/g if defined $expected;
    }

    if ($actual ne $expected) {
        my $base = $test_name;
        $base =~ s/[:\/]/_/g;
        $base =~ s/\s+//g;
        # 记录 SQL 相对路径（从绝对路径中提取 tecc_tests/... 部分）
        {
            my $rel = $sql_file;
            $rel =~ s{/[^/]+/\.\./}{/}g;   # 规范化 ../
            $rel =~ s/^.*\/tecc_tests\///;
            $tecc_test_utils::test_sql_label{$base} = $rel;
        }
        my $outdir = "$TestLib::tmp_check/results";
        mkdir $outdir unless -d $outdir;
        my $outfile = "$outdir/${base}.out";
        open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
        if ($fh) {
            print $fh $actual;
            close $fh;
        }
        my $diff_file = undef;
        my $diff = `diff -u "$expected_fullpath" "$outfile" 2>/dev/null || true`;
        if ($diff ne '') {
            $diff_file = "$outdir/${base}.diff";
            open(my $df, '>', $diff_file) or warn "Cannot write $diff_file: $!";
            if ($df) {
                print $df $diff;
                close $df;
            }
        }
        my $pad = " " x (35 - length($test_name));
        print $TAP "not ok - $test_name${pad}... FAILED  $elapsed ms\n";
        print $TAP "#       实际输出: $outfile\n";
        print $TAP "#       差异对比: $diff_file\n" if defined $diff_file;
        return 0;
    } else {
        my $pad = " " x (35 - length($test_name));
        print $TAP "ok - $test_name${pad}... ok      $elapsed ms\n";
        return 1;
    }
}

# 遍历SQL目录批量执行diff测试
# 如果 TECC_TEST 环境变量设了（如 TECC_TEST=test_ope.sql），只跑这一个
# $canonical=1 时使用规范格式对比（-A -t），去客户端展示差异
# 返回值: 通过的测试数, 总测试数
sub run_sql_dir_diff {
    my ($node, $sql_dir, $expected_dir, $test_prefix, $canonical) = @_;

    $test_prefix //= '';

    my $filter = get_tecc_test_filter();

    opendir(my $dh, $sql_dir) or do {
        printf "not ok - test %-25s ... FAILED  (cannot open directory)\n", $test_prefix;
        return (0, 0);
    };

    my @sql_files = sort grep {
        my $f = $_;
        $f =~ /\.sql$/ && (!$filter || $f eq $filter)
    } readdir($dh);
    closedir($dh);

    # ReqID:TECC4.0 - 密码机不可用时硬加密用例整体不执行（名单见 %HSM_CASE）
    @sql_files = exclude_hsm_cases(@sql_files);

# MAC 行级安全标记（set_user_seclabel / 强访）不在 release-9.2 的合并范围内：
# 这组用例依赖基线不存在的 SQL 函数，整组排除，不参与比对。
our %OUT_OF_SCOPE_CASE = map { $_ => 1 } qw(
    085_mac_distributed.sql
    086_mac_copy_label.sql
    089_mac_table_column_shard.sql
);
@sql_files = grep { !$OUT_OF_SCOPE_CASE{$_} } @sql_files;

    my $passed = 0;
    my $total = scalar @sql_files;

    foreach my $sql_file (@sql_files) {
        my $base = $sql_file;
        $base =~ s/\.sql$//;
        my $expected_file = "$expected_dir/${base}.out";
        # Prefer _1.out (Windows platform variant) if it exists
        my $expected_alt = "$expected_dir/${base}_1.out";
        $expected_file = $expected_alt if -f $expected_alt;
        my $test_name = $test_prefix ? "${test_prefix}: ${base}" : $base;

        $passed += run_sql_diff_test($node,
            "$sql_dir/$sql_file",
            $expected_file,
            $test_name,
            $canonical,
            $base);
    }

    return ($passed, $total);
}

# ============ psql -X 输出归一化 ============

sub normalize_psql_output {
    my ($output) = @_;
    my @lines = split /\n/, $output;
    my @result;

    for my $i (0 .. $#lines) {
        my $line = $lines[$i];
        next unless defined $line;

        # 分隔线：全行仅含 - + | 和空白，且至少含 3 个连续 dash
        if ($line =~ /^[-+|\s]+$/ && $line =~ /-{3,}/) {
            pop @result if @result;
            next;
        }

        # 结果集间的空行（psql -X 输出中跨结果集的空行，-A -t 无此输出）
        if ($line =~ /^\s*$/) {
            next;
        }

        # 数据行：trim 每个 | 段的对齐空格（| 分割的保留列对齐）
        # 单列无 | 的数据行也全局 trim，去掉 psql 列对齐的空格填充
        if ($line =~ /\|/) {
            my @parts = split /\|/, $line;
            for my $part (@parts) {
                $part =~ s/^\s+//;
                $part =~ s/\s+$//;
            }
            $line = join '|', @parts;
        } else {
            $line =~ s/^\s+//;
            $line =~ s/\s+$//;
        }

        push @result, $line;
    }

    my $normalized = join "\n", @result;
    $normalized =~ s/\n*\z//;
    return $normalized;
}

# ============ SEARCH非确定性行剥离 ============
#
# ============ libpq 双比较（raw + common） ============

sub run_sql_dir_libpq_dual_diff {
    my ($node, $sql_dir, $raw_expected_dir, $common_expected_dir, $test_prefix) = @_;

    my $filter = get_tecc_test_filter();
    # ReqID:TECC4.0 - 共享用例每次回归只执行一遍（下面是唯一的执行点），其输出同时供
    # raw 与 common 两种比较使用，不再重复执行。MODE 只控制"比较哪几范式"：
    #   raw（默认）| common | both  —— MODE=both make check-tecc-libpq 恢复双范式比较。
    my $mode = $ENV{'MODE'} // 'raw';

    $test_prefix //= '';

    opendir(my $dh, $sql_dir) or do {
        printf "not ok - test %-25s ... FAILED  (cannot open directory)\n", $test_prefix;
        return (0, 0);
    };

    # ReqID:TECC4.0 - tecc_full_test_suite.sql 是 JDBC 版（Windows 路径
    # E:/tmp/...），仅用于 JDBC 测试，不适用于 libpq TAP（Linux 绝对路径
    # 校验会拒绝 Windows 路径）。从 libpq 用例排除。
    my @sql_files = sort grep {
        my $f = $_;
        $f =~ /\.sql$/ && $f ne 'tecc_full_test_suite.sql' &&
        (!$filter || $f eq $filter)
    } readdir($dh);
    closedir($dh);

    # ReqID:TECC4.0 - 密码机不可用时硬加密用例整体不执行（名单见 %HSM_CASE）
    @sql_files = exclude_hsm_cases(@sql_files);

    my $tp_passed = 0;
    my $tp_total = 0;
    my $connstr = $node->connstr('postgres');

    my $TAP = Test::More->builder->output;

    # 逐个共享用例执行一遍，保存输出与元数据（raw/common 两种比较都用这一份输出）
    my @tasks;
    foreach my $sql_file (@sql_files) {
        my $base = $sql_file;
        $base =~ s/\.sql$//;
        my $sql_fullpath = "$sql_dir/$sql_file";
        my $raw_expected_file = "$raw_expected_dir/${base}.out";
        if (! -f $raw_expected_file) {
            my $raw_expected_alt = "$raw_expected_dir/${base}_1.out";
            $raw_expected_file = $raw_expected_alt if -f $raw_expected_alt;
        }
        my $common_expected_file = "$common_expected_dir/${base}.out";

        # ---- 导出文件绝对路径检查 ----
        my $sql_content = slurp_file($sql_fullpath);
        my @violations = validate_no_absolute_paths($sql_content, $sql_file);
        if (@violations) {
            my $base = $sql_file;
            $base =~ s/\.sql$//;
            my $test_name = $test_prefix ? "${test_prefix}: ${base}" : $base;
            my $pad = " " x (35 - length($test_name));
            print $TAP "not ok - $test_name${pad}... FAILED  (absolute paths)\n";
            for my $v (@violations) {
                print $TAP "#   $v\n";
            }
            push @tasks, { base => $base, sql_fullpath => $sql_fullpath,
                raw_expected => $raw_expected_file,
                common_expected => $common_expected_file,
                raw_output => "ABSOLUTE_PATH_VIOLATION",
                common_output => "ABSOLUTE_PATH_VIOLATION",
                skip => 1 };
            next;
        }

        # Begin - ReqID:TECC4.0
        # 每个共享用例只执行一遍；执行所得的输出按 MODE 需要保留：
        #   raw（默认）→ 只与 expected_libpq/ 比较（$precomputed 复用下面的 raw_output）
        #   common/both → 额外做一次规范化，与 expected_common/ 比较
        my ($stdout, $stderr, $elapsed) =
            run_case_sql_via_psql($node, $connstr, ['-X'], $sql_content, $base, $sql_file);
        # End - ReqID:TECC4.0

        my $raw_out = $stdout;
        if (defined $stderr && $stderr ne '') {
            $raw_out .= "\n" . $stderr;
        }

        my $task = {
            base         => $base,
            sql_fullpath => $sql_fullpath,
            raw_expected => $raw_expected_file,
            common_expected => $common_expected_file,
            raw_output   => $raw_out,
            elapsed      => $elapsed,
        };
        # 只在需要比较 common 时才做规范化（MODE=raw 时不产生 common 输出）
        $task->{common_output} = normalize_psql_output($raw_out)
            if $mode eq 'both' || $mode eq 'common';

        push @tasks, $task;
    }

    # raw 比较段
    if ($mode eq 'both' || $mode eq 'raw') {
        note("\n=== ${test_prefix}用例libpq原始输出比对 ===");
        foreach my $t (@tasks) {
            my $base = $t->{base};
            $tp_total++;
            my $test_name = $test_prefix ? "${test_prefix}_raw: ${base}" : "raw: ${base}";
            if ($t->{skip}) { $tp_total++; next; }
            if (run_sql_diff_test($node, $t->{sql_fullpath}, $t->{raw_expected}, $test_name, 0, $base,
                    { output => $t->{raw_output}, elapsed => $t->{elapsed} })) {
                $tp_passed++;
            }
        }
    }

    # common 比较段
    if ($mode eq 'both' || $mode eq 'common') {
        note("\n=== ${test_prefix}用例libpq规范化输出比对 ===");
        foreach my $t (@tasks) {
            my $base = $t->{base};
            $tp_total++;
            my $test_name = $test_prefix ? "${test_prefix}_common: ${base}" : "common: ${base}";
            if ($t->{skip}) { next; }
            my $expected = '';
            my $common_expected_path = $t->{common_expected};
            if (! -f $common_expected_path) {
                my $alt = $common_expected_path;
                $alt =~ s/\.out$/_1.out/;
                $common_expected_path = $alt if -f $alt;
            }
            if (-f $common_expected_path) {
                $expected = slurp_file($common_expected_path);
            }
            $expected =~ s/\r\n/\n/g;
            $expected =~ s/\n*\z//;

            my $common_output = $t->{common_output};
            $common_output =~ s/\r\n/\n/g;
            $common_output = normalize_common_output($common_output, $base);
            $expected = normalize_common_output($expected, $base);

            if ($common_output ne $expected) {
                    my $outdir = "$TestLib::tmp_check/results";
                    mkdir $outdir unless -d $outdir;
                    my $stem = "${test_prefix}_${base}_common";
                    my $outfile = "$outdir/${stem}.out";
                    {
                        my $rel = $t->{sql_fullpath};
                        $rel =~ s{/[^/]+/\.\./}{/}g;   # 规范化 ../
            $rel =~ s/^.*\/tecc_tests\///;
                        $tecc_test_utils::test_sql_label{$stem} = $rel;
                    }
                    open(my $fh, '>', $outfile) or warn "Cannot write $outfile: $!";
                    print $fh $common_output if $fh;
                    close $fh if $fh;
                    my $diff = `diff -u "$common_expected_path" "$outfile" 2>/dev/null || true`;
                    my $diff_file;
                    if ($diff ne '') {
                        $diff_file = "$outdir/${stem}.diff";
                        open(my $df, '>', $diff_file) or warn "Cannot write $diff_file: $!";
                        if ($df) {
                            print $df $diff;
                            close $df;
                        }
                    }
                    print $TAP "not ok - $test_name ... FAILED\n";
                    print $TAP "#       实际输出: $outfile\n";
                    print $TAP "#       差异对比: $diff_file\n" if defined $diff_file;
                } else {
                    print $TAP "ok - $test_name ... ok\n";
                    $tp_passed++;
                }
        }
    }

    return ($tp_passed, $tp_total);
}

# ============ common 比较归一化（hex隐藏 + NOTICE路径归一化） ============
# 同时适用于libpq和JDBC输出，跨驱动比对的公共归一化函数

sub normalize_common_output {
    my ($text, $sql_base) = @_;
    return '' unless defined $text;

    my $result = $text;

    # 1. Hex 密文隐藏 — 与 raw normalizer 保持能力统一
    my @enc_types = qw(int2_enc int4_enc int8_enc float4_enc float8_enc
                       numeric_enc text_enc varchar_enc bpchar_enc);
    my $enc_types_alt = join('|', @enc_types);
    my $hex_full_re   = qr/E'\\x[0-9a-fA-F]+/;
    my $hex_bare_re   = qr/\\x[0-9a-fA-F]+/;
    my $hex_trunc_re  = qr/([0-9a-fA-F]{16,})'::($enc_types_alt)/;
    my $replacement   = "E'\\\\x被隐藏的HEX输出";
    my $bare_replacement = "\\x被隐藏的HEX输出";
    # LIKE 模式中的裸 hex (单引号内长 hex 串，无 E'\x 前缀)
    my $hex_like_re   = qr/'([0-9a-fA-F]{16,})'/;

    # EXPLAIN Filter格式: \x<hex>E'\x<hex>'::type — 先整体替换避免拆开处理
    my $explain_hex_re = qr/\\x[0-9a-fA-F]+E'\\\\x[0-9a-fA-F]+/;
    $result =~ s/$explain_hex_re/$replacement/ge;

    $result =~ s/$hex_full_re/$replacement/ge;
    $result =~ s/$hex_bare_re/$bare_replacement/g;
    $result =~ s/$hex_trunc_re/"$replacement'::" . lc($2)/ge;
    $result =~ s/$hex_like_re/"'被隐藏的HEX输出'"/ge;


    # LINE error context: psql truncates with ... after hex
    $result =~ s/'([0-9a-fA-F]{3,})\.\.\./'HEX_STR.../g;

    # psql LINE error context: "...{hex}' WHERE ..." (hex truncated by psql LINE display)
    my $hex_inline_re = qr/([0-9a-fA-F]{16,})' (WHERE|ORDER|LIKE|AND|OR|GROUP|LIMIT|HAVING) /;
    $result =~ s/$hex_inline_re/被隐藏的HEX输出' $2 /g;

    # 2. psql 完整路径缩短为 /.../ 形式
    $result =~ s|psql:/\S+?/([^/]+\.\w+:\d+: )|psql:/.../$1|g;

    # 3. NOTICE/WARNING/ERROR 路径前缀归一化
    $result =~ s/^psql:\/\.\.\.\/[^\s]+:\d+:\s*(NOTICE|WARNING|ERROR):\s+/$1: /gm;
    $result =~ s/^(NOTICE|WARNING|ERROR):\s{2,}/$1: /gm;

    # 4. 时间戳归一化（`[2026-05-25 09:45:24]`，每次运行都不同）
    $result =~ s/\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]/[TIMESTAMP]/g;

    # 4.0.1 归一化 KEK 文件绝对路径（不同机器/用户路径不同，出现在 tecc_key 日志的
    # local_kms_load_key FAILED 中：path='/home/<user>/.../kek_xxx.priv'）
    $result =~ s/path='\/[^']*'/path='PATH'/g;

    # Begin - ReqID:TECC4.0
    # 4.1 OS 用户名归一化（pg_dump/pg_restore 输出含对象 Owner，随运行用户变化）
    $result =~ s/Owner: \S+/Owner: USER/g;
    $result =~ s/OWNER TO \S+/OWNER TO USER/g;
    $result =~ s/member of role "\S+"/member of role "USER"/g;
    $result =~ s/^(pg_restore: from TOC entry .+) (\S+)$/$1 USER/mg;
    # End - ReqID:TECC4.0

    # ReqID:TECC4.0 - 归一化 TECC ERROR 中的 DEK/KEK OID
    # (每次回归数据库对象 OID 不同，取决于前面用例创建的对象数)
    $result =~ s/dek_oid=\d+/dek_oid=OID/g;
    $result =~ s/kek_oid=\d+/kek_oid=OID/g;
    # 同上的括号形态：ALTER KEK 的 "KEK (oid=N) not found in database"。
    # 同一用例两次运行可以是 16385 / 16480（取决于实例里已分配的对象数），
    # 期望文件里写死具体值就换了实例必红。
    $result =~ s/\(oid=\d+\)/(oid=OID)/g;

    # ReqID:TECC4.0 - ALTER TYPE 命中加密列时的依赖报错有两个合法分支
    # (依赖对象先被识别为"视图/规则"还是"加密列"，取决于目录扫描顺序，
    #  同一驱动不同次运行之间都会变) → 归一化为同一形态
    $result =~ s/cannot alter type of an encrypted column/cannot alter type of a column/g;
    $result =~ s/cannot alter type of a column used by a view or rule/cannot alter type of a column/g;
    $result =~ s/^(\s*detail:\s*)(?:rule \S+ on view \S+|encrypted column \S+) depends on column /$1DEPENDENCY depends on column /gim;

    # ReqID:TECC4.0 - DROP DEK 依赖校验的 DETAIL 明细本身不确定
    # (报哪个依赖列、表 OID 是多少，取决于目录扫描顺序与对象分配；
    #  同一驱动不同次运行之间都会变，见 libpq 侧 expected_libpq 里
    #  "name"/16392、"v_det"/16574、"v"/17228 三种历史取值)
    $result =~ s/Encrypted column "\S+" in table OID \d+ uses this DEK\./Encrypted column "COL" in table OID OID uses this DEK./g;

    # ReqID:TECC4.0 - 剔除前端兜底噪音行
    # (TECC query preprocessing failed: Processor failed to process query)
    # libpq 处理器在真实 ERROR/HINT 已 fprintf 后仍回传空错误信息，fe-exec 兜底打印该
    # 泛化行；JDBC(Java) 前端无此兜底 → 两驱动在“前端拦截类”错误上文本不一致。共享
    # common 期望必须驱动无关，故把该噪音行从两侧对称剔除（真实错误行已保留）。
    $result =~ s/^[^\n]*?TECC query preprocessing failed: Processor failed to process query[^\n]*\n//gm;
    $result =~ s/^[^\n]*?TECC query preprocessing failed: Processor failed to process query[^\n]*\z//m;

    # 注：libpq 前端在硬加密 KEK 初始化时的那几行进度提示（"Setting up SWXA KEK: ..."、
    # "SUCCESS: [SM2 ]key found in ..."）原先打 stdout、需要在此剔除，现已改由
    # TECC_LOG 输出（src/interfaces/libpq/tecc_common/tecc_logging.h，默认级别 ERROR
    # 即静默），不再出现在任何用例输出里，故这里的剔除规则已删除。

    # 5. 去除尾部空行
    $result =~ s/\n*\z//;


    return $result;
}

# ============ JDBC runner 输出归一化（仅 JDBC 侧调用）============
#
# TeccSqlTestRunner 是 Java 工具，输出中掺有与数据无关的非确定性内容：
#   1. java.util.logging 默认 SimpleFormatter 的表头行带本地化时间戳与类名/方法名
#      （如 `9月 18, 2026 1:18:04 下午 org.postgresql.tecc.xxx yyy`），每次运行都不同；
#      随后的 `WARNING: <正文>` 行是驱动真实行为，保留。
#   2. `报告输出目录: <绝对路径>` 随 checkout 位置变化。
#   3. 回归 KEK 目录的绝对路径（= TECC_KEK_PATH = $TestLib::tmp_check/keys）会直接
#      出现在驱动侧错误消息里（如 "LOCAL KMS key file not found:
#      /home/<user>/.../tmp_check/keys/kek_priv.priv"），该前缀随用户/机器/checkout
#      变化，期望文件里写死具体路径就换个人必红 —— 归一化为 KEKDIR/
#      （记号与 libpq 侧 run_sql_dir_libpq_dual_diff 里的同名规则一致）。
# 归一化只剔除这三类噪音，不改动任何数据行（数据行的处理交给
# normalize_common_output，与 libpq 侧共用同一套规则）。
sub normalize_jdbc_runner_output {
    my ($text) = @_;
    return '' unless defined $text;

    my $result = $text;

    # JUL SimpleFormatter 表头行（中文/英文 locale 两种形态）
    $result =~ s/^\s*\S{0,10}\s*\d{1,2}, \d{4} \d{1,2}:\d{2}:\d{2}\s*(?:上午|下午|AM|PM)?\s+org\.postgresql\S*.*$//gm;
    $result =~ s/^\s*(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \d{1,2}, \d{4} \d{1,2}:\d{2}:\d{2}\s*(?:AM|PM)?\s+org\.postgresql\S*.*$//gm;

    # runner 自身的报告目录提示行
    $result =~ s/^报告输出目录:.*$//gm;
    # 驱动 HSM 类加载器的 INFO 行：候选 vendor JAR 数与 classpath 条目数随环境变化
    # （SoftSDF 模式下会多出 tecc_kms/jce 的门面 JAR），属工具噪音而非业务输出。
    # 注意：必须用 `.*`（不匹配 \n）而非 `[^:]*`——Perl 的否定字符类**匹配换行**，
    # 会让整条规则跨行吞掉前面的输出。
    $result =~ s/^.*HsmVendorClassLoader: found \d+ potential vendor JAR.*$//gm;
    # runner 自身的进度/汇总噪音行
    $result =~ s/^\s*已注册驱动（自动发现）:.*$//gm;
    $result =~ s/^\s*发现 \d+ 个 SQL 文件.*$//gm;
    $result =~ s/^\s*\[simple\] 完成.*$//gm;
    $result =~ s/^\s*语句总数:.*$//gm;
    $result =~ s/^\s*全部完成:.*$//gm;
    $result =~ s/^\s*报告已生成:.*$//gm;

    # 回归 KEK 目录绝对路径 → KEKDIR/（见本函数头部说明第 3 条）。
    # 用 \Q..\E 精确匹配实际目录，避免正则误吃消息前文（如 path='...'）。
    $result =~ s{\Q$TestLib::tmp_check/keys\E}{KEKDIR}g if defined $TestLib::tmp_check;

    $result =~ s/\n{2,}/\n/g;

    return $result;
}

# ============ 失败汇总（TAP # 诊断格式，prove -v 可见）============

sub print_failure_summary {
    my $results_dir = "$TestLib::tmp_check/results";
    return unless -d $results_dir;

    my @out_files  = sort glob("$results_dir/*.out");
    my @diff_files = sort glob("$results_dir/*.diff");
    return unless @out_files || @diff_files;

    my %groups;
    foreach my $f (@out_files, @diff_files) {
        my $basename = $f;
        $basename =~ s{.*/}{};
        my ($group) = $basename =~ /^(.+)\.(out|diff)$/;
        next unless $group;
        $groups{$group}{$f =~ /\.([^.]+)$/ ? $1 : ''} = $f;
    }

    my $TAP = Test::More->builder->output;
    print $TAP "\n# ╔═════════════════════════════╗\n";
    print $TAP "# ║  TECC 回归测试 — 失败汇总   ║\n";
    print $TAP "# ╚═════════════════════════════╝\n";
    print $TAP "#\n";

    foreach my $group (sort keys %groups) {
        my $sql_path = $tecc_test_utils::test_sql_label{$group} // '';
        print $TAP "# ==== ${group} ====\n";
        if ($sql_path) {
            print $TAP "#   问题用例: ${sql_path}\n";
        }
        if ($groups{$group}{out}) {
            print $TAP "#   实际输出: $groups{$group}{out}\n";
        }
        if ($groups{$group}{diff}) {
            print $TAP "#   差异对比: $groups{$group}{diff}\n";
        }
        print $TAP "#\n";
    }
}

#
# setup_swsds_ini - Setup HSM test environment (SWXA + TZHSM)
#
# 按 init_sdf_mode 定下的模式二选一（调用前必须先跑 init_sdf_mode）：
#   SoftSDF（默认）: 铺预置设备密钥 + hsm.properties 到 tmp_check/tmp，不需要 swsds.ini
#   TECC_SDF_LIB=off: copies swsds.ini to PGDATA/ and ./，并要求密码机 TCP 可达
# TZHSM (同智伟业): checks /usr/lib64/cfg/client_config.xml
#
# 密码机不可用时**不改动任何文件**：只把 $hsm_available 置 0 并记下 $hsm_skip_reason，
# 由各入口读用例清单时的 exclude_hsm_cases 剔除 %HSM_CASE 中的用例并打印说明。
# Returns 1 if any HSM is available, 0 if all skipped.

# ---- SDF 模式解析（SoftSDF / 真实加密机） ----
#
# 产品侧语义不变：`TECC_SDF_LIB` 非空 → swxa 适配器 dlopen 该路径；生产不设。
# 回归框架在它之上包一层，**默认走 SoftSDF**（软件密码机，不依赖任何真实加密机）：
#
#   TECC_SDF_LIB 未设/为空  → SoftSDF（默认），库路径取 tecc_kms/bin/libteccsdf.so
#   TECC_SDF_LIB=<path>     → SoftSDF，用指定库（覆盖默认路径）
#   TECC_SDF_LIB=off        → 真实加密机（同义 real / none / no / 0）：
#                             子进程**不带**该变量，C 层回落厂商 libswsds.so，
#                             setup_swsds_ini 转而探测 swsds.ini + TCP 可达性
#
# 解析结果落两份：$sdf_lib_path（Perl 侧判模式）与 $ENV{TECC_SDF_LIB}（子进程按它
# 取库；真机模式必须 unset，否则会被 dlopen 成 SoftSDF）。校验一律在 initdb 之前
# 做完——库或预置密钥缺失立即 die，避免浪费一轮 initdb/start。
sub init_sdf_mode {
    my ($module_dir) = @_;
    my $TAP = Test::More->builder->output;
    my $default_lib = "$module_dir/../../../../tecc_kms/bin/libteccsdf.so";
    my $preset_kek  = "$module_dir/../../../../tecc_kms/keys/kek-1.key";
    my $val = $ENV{TECC_SDF_LIB};
    my $explicit = (defined $val && $val ne '') ? 1 : 0;
    my $lib;

    if ($explicit && $val =~ /\A(?:off|no|none|real|0)\z/i) {
        delete $ENV{TECC_SDF_LIB};
        $sdf_lib_path = undef;
        print $TAP "# SDF 模式: 真实加密机（TECC_SDF_LIB=$val）—— 走 swsds.ini 探测\n";
        return;
    }

    $lib = $explicit ? $val : $default_lib;
    if (!-f $lib) {
        die "SoftSDF 库不存在: $lib\n"
          . ($explicit
             ? "（TECC_SDF_LIB 指定的路径）\n"
             : "（TECC_SDF_LIB 未设时回归默认走 SoftSDF，库路径即此处）\n")
          . "构建方式见 tecc_kms/README.md：\n"
          . "    cd tecc_kms/build && cmake -DGmSSL_ROOT_DIR=\$HOME/gmssl-install . && make -j\n"
          . "本机若只有真实加密机、没有 SoftSDF，用 TECC_SDF_LIB=off 走真机模式。\n";
    }
    if (!-f $preset_kek) {
        die "SoftSDF 预置 KEK 不存在: $preset_kek\n";
    }

    # 规范成绝对路径：子进程（psql / JVM runner）会 chdir 到 tmp_check/tmp，
    # 相对路径到那时就 dlopen 不到了。
    my $abs = Cwd::abs_path($lib);
    $lib = $abs if defined $abs;

    $ENV{TECC_SDF_LIB} = $lib;
    $sdf_lib_path = $lib;
    print $TAP "# SDF 模式: SoftSDF"
        . ($explicit ? "（TECC_SDF_LIB 指定）" : "（默认）")
        . ": $lib\n";

    # ReqID:TECC4.0 - 产物过期检测：`bin/libteccsdf.so` 不入库，源码改了它不会自动重建，
    # 于是回归会拿着旧库跑 HSM 用例。真实踩过：`ed28e2f712b` 把授权索引改成真机形态
    # （<10000 返回 SDR_KEYNOTEXIST），客户端同步改为传 key_index+10000，而当时树里的
    # .so 还是改前的产物 ⇒ HE 私钥授权失败（SDF_GetPrivateKeyAccessRight），表现为
    # HE-only 列解不开、露出密文 hex，误判成"HE 链路坏了"。只告警不中断——.so 也可能
    # 构建在别处（工作副本、安装目录），判定权交给看日志的人。
    my @sdf_sources = (glob("$module_dir/../../../../tecc_kms/src/*.c"),
                       glob("$module_dir/../../../../tecc_kms/src/*.h"),
                       glob("$module_dir/../../../../tecc_kms/include/*.h"));
    my $lib_mtime = (stat($lib))[9];
    my @stale = grep { defined $lib_mtime && (stat($_))[9] > $lib_mtime } @sdf_sources;
    if (@stale) {
        my $sample = Cwd::abs_path($stale[0]);
        $sample = $stale[0] unless defined $sample;
        print $TAP "# ⚠ SoftSDF 产物可能过期：库比 " . scalar(@stale) . " 个源码文件旧\n";
        print $TAP "#   库:   $lib\n";
        print $TAP "#   例:   $sample\n";
        print $TAP "#   重建: cd tecc_kms/build && cmake -DGmSSL_ROOT_DIR=\$HOME/gmssl-install . && make -j\n";
        print $TAP "#   过期 .so 典型症状：SDF_GetPrivateKeyAccessRight 失败 → HE 列解不开、露出密文 hex\n";
    }
}

# True = SoftSDF 模式（init_sdf_mode 已解析出库路径）
sub sdf_soft_mode {
    return defined $sdf_lib_path && $sdf_lib_path ne '';
}

# _swxa_hsm_reachable - Probe whether any HSM listed in swsds.ini is reachable
# over TCP.  A swsds.ini may be present in the dev tree (untracked fixture)
# while the password machine is off the network; the suite must skip instead
# of hard-failing with "SDF_OpenDeviceWithPathAndName failed".
sub _swxa_hsm_reachable {
    my ($ini_file) = @_;
    my @targets;
    my ($ip, $port);
    my $flush = sub {
        if (defined $ip) {
            push @targets, { ip => $ip, port => $port || 8078 };
        }
        ($ip, $port) = (undef, undef);
    };

    open(my $fh, '<', $ini_file) or return 0;
    while (<$fh>) {
        if (/^\s*\[/) { $flush->(); next; }
        if (/^\s*ip\s*=\s*([^\s]+)/i)      { $ip = $1; }
        elsif (/^\s*port\s*=\s*(\d+)/i)    { $port = $1; }
    }
    close($fh);
    $flush->();

    return 0 unless @targets;
    for my $t (@targets) {
        return 1 if _tcp_probe($t->{ip}, $t->{port}, 2);
    }
    return 0;
}

# _tcp_probe - TCP connect with a short timeout; returns 1 on success
sub _tcp_probe {
    my ($host, $port, $timeout) = @_;
    my $sock = IO::Socket::INET->new(
        PeerHost => $host,
        PeerPort => $port,
        Proto    => 'tcp',
        Timeout  => $timeout,
    );
    if ($sock) {
        close($sock);
        return 1;
    }
    return 0;
}

sub setup_swsds_ini {
    my ($node, $module_dir) = @_;
    my $TAP = Test::More->builder->output;
    my $swxa_skip_reason = 'swsds.ini not found';

    # Check SWXA config
    my $swxa_ok = 0;

    # ReqID:TECC4.0 - SoftSDF 模式（**默认**，模式由 init_sdf_mode 解析）：用软件
    # SDF stub（libteccsdf.so）替代真实 swxa 加密机，无需 swsds.ini。把固定预置
    # KEK（kek-1.key）复制到 psql 工作目录（tmp_check/tmp），供
    # SDF_GetSymmKeyHandle(1) 按相对路径读取。
    if (sdf_soft_mode()) {
        my ($key_dir, $tmp_dir) = setup_tecc_regress_env();
        # ReqID:TECC4.0 - 预置设备密钥：kek-1/2 为对称 SM4 KEK（swxa_enc 等
        # 用例）；kek-3 与 sm2*-3.pem 为**无访问口令**的 SM2 密钥对，用于
        # 硬加密 × HE 场景（swxa_enc.sql 的 HE 段）——真机密码机内托管的就是
        # 这种无口令设备密钥，客户端须先授权（空口令）才能解 HE 私钥信封。
        my @preset = ("kek-1.key", "kek-2.key", "kek-3.key",
                      "sm2sign-3.pem", "sm2signpub-3.pem",
                      "sm2enc-3.pem", "sm2encpub-3.pem",
                      # ReqID:TECC4.0 - 索引 4：硬加密非对称 KEK 轮换用例
                      # （swxa_alter_kek_he.sql）的**新**设备密钥，与索引 3
                      # 构成"轮换前/后"两把独立设备 SM2 密钥
                      "kek-4.key", "sm2sign-4.pem", "sm2signpub-4.pem",
                      "sm2enc-4.pem", "sm2encpub-4.pem");
        for my $f (@preset) {
            my $src = "$module_dir/../../../../tecc_kms/keys/$f";
            copy($src, "$tmp_dir/$f") or die "copy fail ($f): $!";
        }
        # ReqID:TECC4.0 - JDBC 侧（JVM runner）走 JCE Provider 路线，需要 tecc_config 指定的
        # HSM 连接配置文件。swxa.ini.path 只做**存在性**校验（HsmConfig.parseSwxaConnectionSection），
        # 内容由厂商类自行读取；SoftSDF 模式下驱动加载的是 tecc_kms/jce 的门面 Provider，
        # 不碰真机，故此处写占位文件即可。
        my $swsds_ini = "$tmp_dir/swsds.ini";
        open(my $si, '>', $swsds_ini) or die "write $swsds_ini: $!";
        print $si "# SoftSDF 模式占位配置：驱动仅校验本文件存在，不解析其内容\n";
        close($si);
        my $hsm_props = "$tmp_dir/hsm.properties";
        open(my $hp, '>', $hsm_props) or die "write $hsm_props: $!";
        print $hp "hsm.vendor = swxa\n";
        print $hp "swxa.ini.path = $swsds_ini\n";
        close($hp);
        $hsm_properties_path = $hsm_props;
        print $TAP "# SoftSDF: TECC_SDF_LIB=$sdf_lib_path, preset keys (kek-1/2/3/4 + sm2*-3/4) -> tmp_check/tmp/\n";
        $swxa_ok = 1;
    } else {
        # 真实加密机模式（TECC_SDF_LIB=off）：找厂商 swsds.ini 并确认密码机可达，
        # 二者缺一即判定密码机不可用、硬加密用例整体不执行（真机不在网络内属正常
        # 开发机形态；由读清单处的 exclude_hsm_cases 剔除并打印说明）。
        for my $src ('/etc/swsds.ini', './swsds.ini', '/home/why/pg14/data/swsds.ini',
                     # ReqID:TECC4.0 - 厂商部署形态：swsds.ini 与 libswsds.so 同目录
                     # （客户端适配器同样会搜该目录，见 fe-swxa-adapter.c swxa_init）
                     '/usr/lib64/swsds.ini') {
            next unless -f $src;
            # ReqID:TECC4.0 - swsds.ini 存在不代表密码机在线：开发机上常驻
            # swsds.ini 但加密机不在网络内时，探测失败应跳过而非硬失败。
            unless (_swxa_hsm_reachable($src)) {
                $swxa_skip_reason = "HSM unreachable via $src";
                last;
            }
            my $datadir = $node->data_dir;
            copy($src, "$datadir/swsds.ini") or warn "copy fail: $!";
            copy($src, "swsds.ini") or warn "copy fail: $!";
            # ReqID:TECC4.0 - JDBC 真机路径同样需要 tecc_config（hsm.properties），
            # 且厂商 JCE 按 **JVM 工作目录** 读 swsds.ini（不解析 swxa.ini.path 内容），
            # 故把 ini 与 properties 一并落到 tmp_check/tmp，供 runner chdir 后使用。
            my ($key_dir_r, $tmp_dir_r) = setup_tecc_regress_env();
            copy($src, "$tmp_dir_r/swsds.ini") or warn "copy fail: $!";
            my $hsm_props = "$tmp_dir_r/hsm.properties";
            open(my $hp, '>', $hsm_props) or die "write $hsm_props: $!";
            print $hp "hsm.vendor = swxa\n";
            print $hp "swxa.ini.path = $tmp_dir_r/swsds.ini\n";
            close $hp;
            $hsm_properties_path = $hsm_props;
            print $TAP "# swsds.ini: $src -> PGDATA/ and ./\n";
            print $TAP "# real HSM hsm.properties: $hsm_props (swsds.ini -> tmp_check/tmp)\n";
            $swxa_ok = 1;
            last;
        }
    }

    # ReqID:TECC4.0 - 记录密码机可用性与不可用原因。四个入口在读用例清单时会据此剔除
    # %HSM_CASE 中的硬加密用例（exclude_hsm_cases），跳过情况在那边打印。
    $hsm_available = $swxa_ok;
    $hsm_skip_reason = $swxa_ok ? '' : $swxa_skip_reason;

    return $swxa_ok ? 1 : 0;
}

# ---- JDBC 路径专用跳过项 ----
#
# JDBC 路径因**工具/已登记缺陷**原因无法执行的用例，在 002 读清单时由
# exclude_jdbc_unsupported_cases 剔除并打印原因（名单见 %JDBC_UNSUPPORTED_CASE）：
#
#   shared/sql/reconnect_schema_dek.sql —— TeccSqlTestRunner.splitStatements
#   遇到任意反斜杠即切分语句（不区分注释），把文件头部注释里的 `\c 重连...`
#   当成重连命令，以 role "DEK" 登录 → FATAL: role "DEK" does not exist，
#   整个文件中止、零输出（详见 shared/jdbc_vs_libpq_diff.md）。
#
# 注：shared/sql/swxa_concurrent.sql 曾因 C 侧"多 KEK 时首个 KEK 的 kms_type 被
#   后续 KEK 的 KEY_STORE 覆盖"缺陷在此跳过；该缺陷已修复（KEK 列表改为逐行按
#   kek_oid 分组），本用例已重新启用。
#
# 注：硬加密（HSM）用例不在此列——它们由 %HSM_CASE + exclude_hsm_cases 统一处理
#   （SoftSDF 为默认即启用；真机需 TECC_SDF_LIB=off 且 swsds.ini 可达）。

1;
