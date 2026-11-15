package tecc_jdbc_jar;

#
# TECC回归测试——JDBC驱动JAR选取
#
# 背景：TECC JDBC用例必须用**与服务端同版本**的驱动JAR跑，否则表现为大批
#   "invalid CipherDataType structure"、服务端 double free 崩溃，极易被误判为
#   被测代码的缺陷（复盘见 shared/jdbc_vs_libpq_diff.md 的"过期驱动 JAR"一节）。
#
# pgjdbc 的 build/libs/ 是 Gradle 生成物，历次构建的 JAR 会**累积**在同一目录里
#   （如 tecc-4.0.0.0 与 tecc-4.0.1.0 并存），而 glob() 按**字典序**返回，
#   tecc-4.0.0.0 恰好排在前面 —— 旧的写法 `(glob ...)[0]` 因此会恒取旧驱动。
#
# 本模块统一选取规则，供回归入口脚本与 Makefile 共用，避免各处规则漂移：
#   1. 只考虑 postgresql*.jar，排除 -sources/-javadoc 之类的旁挂产物；
#   2. 优先 `-all.jar`（shadowJar 产物，自带依赖；用例 classpath 只有驱动JAR，
#      非 -all 的瘦 JAR 会缺 jsqlparser 等运行期依赖）；
#   3. 同组内按版本号（数字段逐段比较）取最高，而非字典序。
#
# 用法：
#   use tecc_jdbc_jar;
#   my @jars = jdbc_jar_candidates($jdbc_home);  # 全部候选，最佳在前；空列表表示未找到
#   print $TAP "# 驱动 JAR: " . jdbc_jar_selection_note(\@jars) . "\n";
#
#   # Makefile 等只要单值的场合：
#   perl -Mtecc_jdbc_jar=pick_jdbc_jar -e 'print pick_jdbc_jar($ARGV[0]) // ""' <home>
#

use strict;
use warnings;
use Exporter 'import';

our @EXPORT    = qw(jdbc_jar_candidates jdbc_jar_selection_note);
our @EXPORT_OK = qw(pick_jdbc_jar);

# 候选 JAR 列表，最佳者在前；未找到返回空列表。
sub jdbc_jar_candidates {
    my ($jdbc_home) = @_;

    return () if !defined $jdbc_home || $jdbc_home eq '';

    my @jars = grep { !/-sources\.jar$|-javadoc\.jar$/ }
        glob("$jdbc_home/pgjdbc/build/libs/postgresql*.jar");
    return () if !@jars;

    # 优先 shadowJar 产物；没有 -all 时退化为全部候选（后缀命名变化时不至于直接取不到）
    my @shadow = grep { /-all\.jar$/ } @jars;
    @jars = @shadow if @shadow;

    # 版本降序：数字段逐段比较（字典序会把 4.0.10.0 判成小于 4.0.9.0），
    # 同版本再按全名比较，保证结果与目录读取顺序无关。
    return sort { _version_key($b) cmp _version_key($a) || $b cmp $a } @jars;
}

sub pick_jdbc_jar {
    my ($jdbc_home) = @_;
    my @jars = jdbc_jar_candidates($jdbc_home);
    return @jars ? $jars[0] : undef;
}

# 本次选用的 JAR 及其被忽略的旧版本，供入口脚本打进 TAP 诊断：同一份代码配不同驱动
# JAR 会给出**不同**的回归结果，实际选用项必须留在输出里。
sub jdbc_jar_selection_note {
    my ($jars) = @_;

    return undef if !$jars || !@$jars;
    return $jars->[0] if @$jars == 1;

    my @ignored = map { (my $n = $_) =~ s{.*/}{}; $n } @$jars[1 .. $#$jars];
    return "$jars->[0]（已忽略旧版本: " . join(', ', @ignored) . "）";
}

# 版本键：把文件名里的连续数字段零填充后串起来，使字符串比较等价于版本比较。
sub _version_key {
    my ($jar) = @_;
    (my $name = $jar) =~ s{.*/}{};
    $name =~ s{\.jar$}{};
    return join('.', map { sprintf('%09d', $_) } ($name =~ /(\d+)/g));
}

1;
