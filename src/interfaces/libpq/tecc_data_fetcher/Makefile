# Begin - ReqID:TECC4.0
#-------------------------------------------------------------------------
#
# Makefile for TECC data fetcher module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
# Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# src/interfaces/libpq/tecc_data_fetcher/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_data_fetcher
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# 编译标志
override CPPFLAGS := -DFRONTEND -I$(srcdir) -I$(srcdir)/include -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq -I../tecc_cache -I../tecc_common -I../frontend_parser $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)

# 对象文件
OBJS = \
    tecc_data_fetcher.o \
    tecc_libpq_cursor.o \
    tecc_metadata_query.o \
    tecc_permission_check.o \
    tecc_bitmask_parser.o \
    tecc_key_args_parser.o \
    tecc_error_handler.o \
    tecc_bytea_decoder.o

# 默认目标
all: $(OBJS)

# 源文件路径
SRCDIR = .

# 编译规则
tecc_data_fetcher.o: $(SRCDIR)/tecc_data_fetcher.c include/tecc_data_fetcher.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_libpq_cursor.o: $(SRCDIR)/tecc_libpq_cursor.c include/tecc_libpq_cursor.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_metadata_query.o: $(SRCDIR)/tecc_metadata_query.c include/tecc_metadata_query.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_permission_check.o: $(SRCDIR)/tecc_permission_check.c include/tecc_permission_check.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_bitmask_parser.o: $(SRCDIR)/tecc_bitmask_parser.c include/tecc_bitmask_parser.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_key_args_parser.o: $(SRCDIR)/tecc_key_args_parser.c include/tecc_key_args_parser.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_error_handler.o: $(SRCDIR)/tecc_error_handler.c include/tecc_error_handler.h include/tecc_data_fetcher_types.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_bytea_decoder.o: $(SRCDIR)/tecc_bytea_decoder.c include/tecc_bytea_decoder.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


# 清理
clean:
	rm -f $(OBJS) *.d

distclean: clean

maintainer-clean: distclean

.PHONY: all clean distclean maintainer-clean
# End - ReqID:TECC4.0