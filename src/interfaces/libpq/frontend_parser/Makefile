#-------------------------------------------------------------------------
#
# Makefile for frontend parser library
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/interfaces/libpq/frontend_parser/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/frontend_parser
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# 前端解析器编译标志
override CPPFLAGS := -DFRONTEND_PARSER -DFRONTEND -I$(srcdir) -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)
override CXXFLAGS := -fPIC $(CXXFLAGS)

# Bison/Flex生成的文件
BISON_OUTPUT = tecc_grammar.c tecc_grammar.h
FLEX_OUTPUT = lex.tecc_yy.c
PARSER_SOURCES = tecc_grammar.c lex.tecc_yy.c

# 对象文件 - 基础解析器组件（包含Bison/Flex生成的文件）
OBJS = \
	parser.o \
	feparser_memutils.o \
	tecc_nodes.o \
	tecc_parser.o \
	tecc_grammar.o \
	lex.tecc_yy.o \
	tecc_location.o \
	tecc_semantic.o \
	tecc_error.o \
	tecc_perf.o \
	fe_adapter.o

# TECC模块对象文件（在子目录中）
# TECC_PARSER_OBJS = ../tecc_parser/tecc_parser.o  # 不再使用子目录中的tecc_parser.cpp
TECC_PROCESSOR_OBJS = ../tecc_processor/tecc_processor.o
TECC_CACHE_OBJS = ../tecc_cache/tecc_metadata_cache.o
# TECC_REWRITER_OBJS = ../tecc_rewriter/tecc_rewriter.o  # 目录不存在，暂时注释掉

# 所有对象文件
ALL_OBJS = $(OBJS) $(TECC_PROCESSOR_OBJS) $(TECC_CACHE_OBJS) # $(TECC_REWRITER_OBJS)

# 测试程序
TEST_PROG = test_tecc_parser
INTEGRATION_TEST_PROG = test_integration
QUICK_TEST_PROG = test_quick

# 默认目标
all: check_parser_deps $(BISON_OUTPUT) $(FLEX_OUTPUT) $(OBJS) tecc_modules

# 检查解析器依赖
check_parser_deps:
	@if ! command -v bison >/dev/null 2>&1 || ! command -v flex >/dev/null 2>&1; then \
		echo "Warning: bison or flex not found. Generating stub parser files..."; \
		if [ -f generate_stub_parser.sh ]; then \
			./generate_stub_parser.sh; \
		else \
			echo "Error: generate_stub_parser.sh not found."; \
			exit 1; \
		fi; \
	fi

# 编译TECC模块
tecc_modules:
	# $(MAKE) -C ../tecc_parser  # 不再编译tecc_parser子目录
	$(MAKE) -C ../tecc_processor
	$(MAKE) -C ../tecc_cache
	# $(MAKE) -C ../tecc_rewriter  # 目录不存在，暂时注释掉

# Bison/Flex构建规则
tecc_grammar.c tecc_grammar.h: tecc_grammar.y
	@if command -v bison >/dev/null 2>&1; then \
		echo "Generating parser with bison..."; \
		bison -d -o tecc_grammar.c tecc_grammar.y; \
	else \
		echo "Warning: bison not found. Using pre-generated parser if available."; \
		if [ ! -f tecc_grammar.c ]; then \
			echo "Error: tecc_grammar.c not found and bison is not available."; \
			exit 1; \
		fi; \
	fi

lex.tecc_yy.c: tecc_scanner.l tecc_grammar.h
	@if command -v flex >/dev/null 2>&1; then \
		echo "Generating scanner with flex..."; \
		flex -o lex.tecc_yy.c tecc_scanner.l; \
	else \
		echo "Warning: flex not found. Using pre-generated scanner if available."; \
		if [ ! -f lex.tecc_yy.c ]; then \
			echo "Error: lex.tecc_yy.c not found and flex is not available."; \
			exit 1; \
		fi; \
	fi

# 编译规则
parser.o: parser.c feparser_memutils.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

feparser_memutils.o: feparser_memutils.c feparser_memutils.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_nodes.o: tecc_nodes.c tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_parser.o: tecc_parser.c tecc_parser.h tecc_nodes.h tecc_grammar.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_grammar.o: tecc_grammar.c tecc_grammar.h tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

lex.tecc_yy.o: lex.tecc_yy.c tecc_grammar.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_location.o: tecc_location.c tecc_location.h tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_semantic.o: tecc_semantic.c tecc_semantic.h tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_error.o: tecc_error.c tecc_error.h tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

tecc_perf.o: tecc_perf.c tecc_perf.h tecc_nodes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

fe_adapter.o: fe_adapter.c fe_adapter.h tecc_nodes.h feparser_memutils.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -Wno-error -c -o $@ $<

# 测试程序编译
$(TEST_PROG): test_tecc_parser.c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ test_tecc_parser.c $(OBJS) $(TECC_PROCESSOR_OBJS) $(TECC_CACHE_OBJS) -L../../../../src/port -L../../../../src/common -lpgcommon -lpgport

$(INTEGRATION_TEST_PROG): test_integration.c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ test_integration.c $(OBJS) $(TECC_PROCESSOR_OBJS) $(TECC_CACHE_OBJS) -L../../../../src/port -L../../../../src/common -lpgcommon -lpgport

$(QUICK_TEST_PROG): test_quick.c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ test_quick.c $(OBJS) $(TECC_PROCESSOR_OBJS) $(TECC_CACHE_OBJS) -L../../../../src/port -L../../../../src/common -lpgcommon -lpgport

# 清理
clean:
	rm -f $(OBJS) *.d $(TEST_PROG) $(INTEGRATION_TEST_PROG) $(QUICK_TEST_PROG) $(BISON_OUTPUT) $(FLEX_OUTPUT) *.output tecc_grammar.tab.c tecc_grammar.tab.h

distclean: clean

maintainer-clean: distclean

.PHONY: all clean distclean maintainer-clean