# Begin - ReqID:TECC4.0
#-------------------------------------------------------------------------
#
# Makefile for TECC data fetcher module tests
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
# Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# src/interfaces/libpq/tecc_data_fetcher/test/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_data_fetcher/test
top_builddir = ../../../../..
include $(top_builddir)/src/Makefile.global

# 测试程序
PROGRAMS = test_tecc_data_fetcher

# 编译标志
override CPPFLAGS := -DFRONTEND -I$(srcdir) -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq -I../include $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)

# 测试程序依赖
test_tecc_data_fetcher_OBJS = \
    test_tecc_data_fetcher.o \
    ../src/tecc_data_fetcher.o \
    ../src/tecc_libpq_cursor.o \
    ../src/tecc_metadata_query.o \
    ../src/tecc_permission_check.o \
    ../src/tecc_bitmask_parser.o \
    ../src/tecc_key_args_parser.o \
    ../src/tecc_error_handler.o \
    ../src/tecc_connection_pool.o \
    ../src/tecc_query_cache.o

# 链接库
LIBS := $(libpq) $(LIBS)

# 默认目标
all: $(PROGRAMS)

# 测试程序编译规则
test_tecc_data_fetcher: $(test_tecc_data_fetcher_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

# 测试规则
check: all
	./test_tecc_data_fetcher

# 清理
clean:
	rm -f $(PROGRAMS) $(test_tecc_data_fetcher_OBJS) *.o *.d

distclean: clean

maintainer-clean: distclean

.PHONY: all check clean distclean maintainer-clean
# End - ReqID:TECC4.0