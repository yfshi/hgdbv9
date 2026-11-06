#-------------------------------------------------------------------------
#
# Makefile for TECC cache module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
# Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# src/interfaces/libpq/tecc_cache/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_cache
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# 编译标志
override CPPFLAGS := -DFRONTEND_PARSER -DFRONTEND -I$(srcdir) -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq -I../frontend_parser $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)

# 对象文件 - 基于简化设计的完整实现
OBJS = tecc_cache_manager.o tecc_column_cache.o \
       tecc_kek_cache.o tecc_dek_cache.o tecc_data_fetcher_adapter.o \
       fe_hash_table.o

# 默认目标
all: $(OBJS)

# 编译规则
tecc_cache_manager.o: tecc_cache_manager.c tecc_metadata_cache.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_column_cache.o: tecc_column_cache.c tecc_metadata_cache.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_kek_cache.o: tecc_kek_cache.c tecc_metadata_cache.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_dek_cache.o: tecc_dek_cache.c tecc_metadata_cache.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_data_fetcher_adapter.o: tecc_data_fetcher_adapter.c tecc_metadata_cache.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -I../tecc_data_fetcher/include -I../tecc_libpq_integration -c -o $@ $<


fe_hash_table.o: fe_hash_table.c fe_hash_table.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


# 清理
clean:
	rm -f $(OBJS) *.d

distclean: clean

maintainer-clean: distclean

.PHONY: all clean distclean maintainer-clean