# src/test/modules/tecc_tests/Makefile
#
# TECC全密态回归测试模块
# 包含SQL回归测试、libpq(psql -C)全链路测试、JDBC全链路测试
#
# 环境变量:
#   TECC_JDBC_HOME - JDBC驱动项目根目录（可选，设了才跑JDBC测试）
#   TECC_SDF_LIB   - SDF 库：不设=SoftSDF（默认，取 tecc_kms/bin/libteccsdf.so）；
#                    指定路径=用该 SoftSDF 库；=off 走真实加密机（swsds.ini+TCP 探测）

PGFILEDESC = "tecc_tests - TECC encryption regression tests"

# TAP tests驱动libpq(psql -C)和JDBC测试
TAP_TESTS = 1

# tecc: TAP 用例会直接调用 pg_regress；通用 prove_check 不设置该变量，
# 缺失时用例会退回到 `pg_config --pgxs` 拼出错误路径而直接 die。
PG_REGRESS = $(abs_top_builddir)/src/test/regress/pg_regress
export PG_REGRESS
# tecc: 用例里还有 `use PostgresNode;` 等老式模块，统一走 PERL5LIB，
# 避免依赖各 check 目标是否传了 -I。
PERL5LIB = $(abs_top_srcdir)/src/test/perl:$(abs_srcdir):$(abs_srcdir)/t
export PERL5LIB
# tecc: 老式 PostgresNode.pm 用 PG_TEST_PORT_DIR 做端口锁目录；
# 不设时在本树会被算成空串，进而尝试写 "/<port>.rsv" 而失败。
PG_TEST_PORT_DIR = $(CURDIR)/tmp_check/portlock
export PG_TEST_PORT_DIR

PG_CPPFLAGS = -I$(libpq_srcdir)

# JDBC驱动jar构建（TECC_JDBC_HOME 指向 pgjdbc 项目根目录）
# 每次都委托 Gradle 做增量编译，源码不变秒回 UP-TO-DATE
#
# JDK 根目录：优先 JAVA_HOME_JDBC，其次 JAVA_HOME，最后从 PATH 上的 java 反推
# （pgjdbc 仓库的 gradlew 在 git 中无执行位，故统一用 `sh ./gradlew` 调用）
JAVA_HOME_JDBC ?= $(if $(JAVA_HOME),$(JAVA_HOME),$(shell readlink -f "$$(command -v java)" 2>/dev/null | sed -e 's:/bin/java$$::'))
jdbc-jar:
	@if [ -z "$(JAVA_HOME_JDBC)" ]; then \
	    echo "Error: JAVA_HOME_JDBC not set. Set it to JDK root (e.g. /usr/lib/jvm/java-11-openjdk-amd64)."; \
	    exit 1; \
	fi
	@if [ -z "$(TECC_JDBC_HOME)" ]; then \
	    echo "Error: TECC_JDBC_HOME not set. Point it to pgjdbc project root."; \
	    exit 1; \
	fi
	@if ! java -version >/dev/null 2>&1; then \
	    echo "Error: Java not found. JDBC tests require JDK (JAVA_HOME or java in PATH)."; \
	    exit 1; \
	fi
	@echo "Building JDBC driver from $(TECC_JDBC_HOME)..."
	cd "$(TECC_JDBC_HOME)" && sh ./gradlew :postgresql:shadowJar -Dorg.gradle.java.home=$(JAVA_HOME_JDBC)

# JDBC 测试类编译（供 TeccSqlTestRunner 使用）
jdbc-compile:
	@if [ -z "$(JAVA_HOME_JDBC)" ]; then \
	    echo "Error: JAVA_HOME_JDBC not set. Set it to JDK root (e.g. /usr/lib/jvm/java-11-openjdk-amd64)."; \
	    exit 1; \
	fi
	@if [ -z "$(TECC_JDBC_HOME)" ]; then \
	    echo "Error: TECC_JDBC_HOME not set."; \
	    exit 1; \
	fi
	cd "$(TECC_JDBC_HOME)" && sh ./gradlew :postgresql:compileTestJava -x test -PskipCheckstyle -PskipAutostyle -Dorg.gradle.java.home=$(JAVA_HOME_JDBC)

# SoftSDF 的 JCE 门面 JAR（tecc_kms/jce，回归测试专用，非产品件）
#
# 用途：让 JDBC 的 SWXA（swxakms）路径在无真实加密机的 Linux 上跑起来 —— 驱动按全限定名
# 反射加载 com.sansec.jce.provider.SwxaProvider，本 JAR 提供同名类与同算法集，后端读
# SoftSDF 的 kek-<index>.key / sm2enc-<index>.pem，顶替真实厂商 JAR。
#
# 编译期需要驱动的 Sm4Cipher/Sm2Cipher，故必须在 jdbc-jar 之后执行。
#
# 驱动 JAR 的选取走 t/tecc_jdbc_jar.pm（与各回归入口脚本同一份规则）：pgjdbc 的
# build/libs/ 会累积历次构建产物，按字典序取第一个会挑到**旧版本驱动** —— 旧 JAR
# 里还没有 Sm2Cipher，本目标会直接报"找不到符号"。
TECC_KMS_DIR ?= $(abspath $(CURDIR)/../../../../tecc_kms)
TECC_JCE_JAR = $(TECC_KMS_DIR)/bin/libteccjce.jar
tecc-sdf-jce-jar:
	@if [ -z "$(JAVA_HOME_JDBC)" ]; then \
	    echo "Error: JAVA_HOME_JDBC not set. Set it to JDK root (e.g. /usr/lib/jvm/java-11-openjdk-amd64)."; \
	    exit 1; \
	fi
	@driver_jar=$$(perl -I "$(CURDIR)/t" -Mtecc_jdbc_jar=pick_jdbc_jar \
	    -e 'print(pick_jdbc_jar($$ARGV[0]) || "")' "$(TECC_JDBC_HOME)"); \
	if [ -z "$$driver_jar" ]; then \
	    echo "Error: JDBC driver jar not found; run 'make jdbc-jar' first."; \
	    exit 1; \
	fi; \
	echo "Using JDBC driver: $$driver_jar"; \
	mkdir -p "$(TECC_KMS_DIR)/jce/build/classes"; \
	"$(JAVA_HOME_JDBC)/bin/javac" -nowarn -cp "$$driver_jar" \
	    -d "$(TECC_KMS_DIR)/jce/build/classes" \
	    `find "$(TECC_KMS_DIR)/jce/src" -name '*.java'` || exit 1; \
	"$(JAVA_HOME_JDBC)/bin/jar" cf "$(TECC_JCE_JAR)" \
	    -C "$(TECC_KMS_DIR)/jce/build/classes" . || exit 1; \
	echo "Built SoftSDF JCE facade: $(TECC_JCE_JAR)"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = src/test/modules/tecc_tests
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_builddir)/contrib/contrib-global.mk
endif

# prove 详细输出（必须在 include 之后，否则被 Makefile.global 覆盖）
PROVE_FLAGS = -v

# ReqID:TECC4.0 - PERF_EXT_SKIP_INIT reuses both the existing database and the
# KEK files under tmp_check. The global prove_check always removes tmp_check,
# so perf_ext needs a local wrapper that preserves it when initialization is
# explicitly skipped.
define perf_ext_prove_check
if [ -n "$$PERF_EXT_SKIP_INIT" ] && [ "$$PERF_EXT_SKIP_INIT" != "0" ]; then \
	echo "PERF_EXT_SKIP_INIT is set; preserving '$(CURDIR)/tmp_check'"; \
else \
	rm -rf '$(CURDIR)'/tmp_check; \
fi
$(MKDIR_P) '$(CURDIR)'/tmp_check
cd $(srcdir) && \
   TESTDIR='$(CURDIR)' $(with_temp_install) PGPORT='6$(DEF_PGPORT)' \
   top_builddir='$(CURDIR)/$(top_builddir)' \
   PG_REGRESS='$(CURDIR)/$(top_builddir)/src/test/regress/pg_regress' \
   $(PROVE) $(PG_PROVE_FLAGS) $(PROVE_FLAGS) $(if $(PROVE_TESTS),$(PROVE_TESTS),t/*.pl)
endef

# MODE 选择（透传到 TAP 测试脚本）：
#   默认 MODE=both（raw + common 都比较）
#   可在顶级目录用 make check-tecc-libpq{-raw,-common} 选择
#   也可直接 MODE=raw make check 在当前目录运行
#
# 临时存放目标：仅显式调用才执行
check-tecc-temp: PROVE_TESTS = temp/temp_test.pl
check-tecc-temp:
	$(prove_check)
check-tecc-perf-libpq: PROVE_TESTS = t/perf/004_tecc_libpq_perf.pl
check-tecc-perf-libpq:
	$(prove_check)
check-tecc-perf-jdbc: PROVE_TESTS = t/perf/005_tecc_jdbc_perf.pl
check-tecc-perf-jdbc:
	$(prove_check)
check-tecc-perf: check-tecc-perf-libpq check-tecc-perf-jdbc
check-tecc-perf-ext: PROVE_TESTS = t/perf_ext/006_tecc_perf_ext.pl
check-tecc-perf-ext:
	$(perf_ext_prove_check)
check-tecc-perf-ext-he: PROVE_TESTS = t/perf_ext_he/007_tecc_perf_ext_he.pl
check-tecc-perf-ext-he:
	$(perf_ext_prove_check)
# 同 check，但 prove exit=2（TAP plan警告）不视为失败
check-p:
	-$(prove_check)

# 同 check，但 prove exit=2（TAP plan缺失/多余）不视为失败
check-p:
	-$(prove_check)

# 回归用例的相对路径产物（\copy TO / pg_dump -f / `> file`）正常落在 tmp_check/tmp，
# 每次运行开头 prove_check 都会 rm -rf tmp_check。若 CWD 跑偏（历史上 F-04 重构曾把
# harness 的 chdir 整段覆盖掉），这些产物会落到本目录并污染 git status。
# 运行期由 t/tecc_test_utils.pm 的 cleanup_stray_test_artifacts 兜底清理，
# 这里给出显式的手工清理入口。
TECC_STRAY_ARTIFACT_GLOBS = conc_bg.out pg_dump_*.sql copy_*.bin copy_*.txt tecc_*.dump
clean distclean maintainer-clean: clean-tecc-stray-artifacts
clean-tecc-stray-artifacts:
	rm -f $(TECC_STRAY_ARTIFACT_GLOBS)
