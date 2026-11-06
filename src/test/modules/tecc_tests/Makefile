# src/test/modules/tecc_tests/Makefile
#
# TECC全密态回归测试模块
# 包含SQL回归测试、libpq(psql -C)全链路测试、JDBC全链路测试
#
# 环境变量:
#   TECC_JDBC_HOME - JDBC驱动项目根目录（可选，设了才跑JDBC测试）

PGFILEDESC = "tecc_tests - TECC encryption regression tests"

# TAP tests驱动libpq(psql -C)和JDBC测试
TAP_TESTS = 1

PG_CPPFLAGS = -I$(libpq_srcdir)

# JDBC驱动jar构建（TECC_JDBC_HOME 指向 pgjdbc 项目根目录）
# 每次都委托 Gradle 做增量编译，源码不变秒回 UP-TO-DATE
JAVA_HOME_JDBC ?= $(error JAVA_HOME_JDBC is not set. Set it to JDK root, e.g. /usr/lib/jvm/java-11-openjdk-amd64)
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
	cd "$(TECC_JDBC_HOME)" && ./gradlew :postgresql:shadowJar -Dorg.gradle.java.home=$(JAVA_HOME_JDBC)

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
	cd "$(TECC_JDBC_HOME)" && ./gradlew :postgresql:compileTestJava -x test -PskipCheckstyle -PskipAutostyle -Dorg.gradle.java.home=$(JAVA_HOME_JDBC)

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

# MODE 选择（透传到 TAP 测试脚本）：
#   默认 MODE=both（raw + common 都比较）
#   可在顶级目录用 make check-tecc-libpq{-raw,-common} 选择
#   也可直接 MODE=raw make check 在当前目录运行
#
# 临时存放目标：仅显式调用才执行
check-tecc-temp: PROVE_TESTS = temp/temp_test.pl
check-tecc-temp:
	$(prove_check)

# 同 check，但 prove exit=2（TAP plan警告）不视为失败
check-p:
	-$(prove_check)

# 同 check，但 prove exit=2（TAP plan缺失/多余）不视为失败
check-p:
	-$(prove_check)
