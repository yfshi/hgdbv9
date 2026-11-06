# Begin - ReqID:TECC4.0
#-------------------------------------------------------------------------
#
# Makefile for TECC common types module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
# Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# src/interfaces/libpq/tecc_common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_common
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# 编译标志
override CPPFLAGS := -DFRONTEND_PARSER -DFRONTEND -I$(srcdir) -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq -I../tecc_processor -I../tecc_libpq_integration $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)

# 对象文件
OBJS = tecc_frontend_list.o tecc_copy_state.o tecc_logging.o

# 默认目标
all: $(OBJS)

# 清理
clean:
	rm -f $(OBJS)

# 安装
install: all

.PHONY: all clean install
# End - ReqID:TECC4.0