# Begin - ReqID:TECC4.0
#-------------------------------------------------------------------------
#
# Makefile for TECC processor module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
# Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
# src/interfaces/libpq/tecc_processor/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_processor
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# 编译标志
override CPPFLAGS := -DFRONTEND_PARSER -DFRONTEND -I$(srcdir) -I$(top_builddir)/src/include -I$(top_builddir)/src/interfaces/libpq -I../frontend_parser -I../tecc_scheduler -I../tecc_formatter $(CPPFLAGS)
override CFLAGS := -fPIC $(CFLAGS)

# 对象文件 - 新的模块化实现 (v4.0)
OBJS = \
    tecc_processor.o \
    stmt_processor.o \
    values_processor.o \
    where_clause_processor.o \
    create_stmt_processor.o \
    raw_value.o \
    raw_values_list.o \

    internal/tecc_processor_internal.o \
    column_ref_data.o \
    func_processor.o \
    func_hardcoded_values.o \
    prepared_statement.o \
    expr_processor.o \
    tecc_expression_walker.o \
    processor_utils.o \
    key_management.o \
    alter_stmt_processor.o
    # prepared_statements_list.o \
    # pre_stmt_processor.o \
    # post_stmt_processor.o

# 默认目标
all: $(OBJS)

# 编译规则
tecc_processor.o: tecc_processor.c tecc_processor.h tecc_processor_types.h internal/tecc_processor_internal.h stmt_processor.h values_processor.h where_clause_processor.h create_stmt_processor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

stmt_processor.o: stmt_processor.c stmt_processor.h internal/tecc_processor_internal.h values_processor.h where_clause_processor.h create_stmt_processor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

values_processor.o: values_processor.c values_processor.h raw_value.h raw_values_list.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

where_clause_processor.o: where_clause_processor.c where_clause_processor.h internal/tecc_processor_internal.h values_processor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

create_stmt_processor.o: create_stmt_processor.c create_stmt_processor.h internal/tecc_processor_internal.h

alter_stmt_processor.o: alter_stmt_processor.c alter_stmt_processor.h internal/tecc_processor_internal.h create_stmt_processor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

raw_value.o: raw_value.c raw_value.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

raw_values_list.o: raw_values_list.c raw_values_list.h raw_value.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# sql_rewriter.o removed in v4.0 - SQL rewrite functionality distributed to submodules


internal/tecc_processor_internal.o: internal/tecc_processor_internal.c internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

column_ref_data.o: column_ref_data.c column_ref_data.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

expr_processor.o: expr_processor.c expr_processor.h column_ref_data.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

tecc_expression_walker.o: tecc_expression_walker.c tecc_expression_walker.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

func_processor.o: func_processor.c func_processor.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

func_hardcoded_values.o: func_hardcoded_values.c func_hardcoded_values.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

prepared_statement.o: prepared_statement.c prepared_statement.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

processor_utils.o: processor_utils.c processor_utils.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

key_management.o: key_management.c internal/tecc_processor_internal.h

alter_stmt_processor.o: alter_stmt_processor.c alter_stmt_processor.h internal/tecc_processor_internal.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


# prepared_statements_list.o: prepared_statements_list.c prepared_statements_list.h internal/tecc_processor_internal.h
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# pre_stmt_processor.o: pre_stmt_processor.c pre_stmt_processor.h internal/tecc_processor_internal.h
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# post_stmt_processor.o: post_stmt_processor.c post_stmt_processor.h internal/tecc_processor_internal.h
#	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# 清理
clean:
	rm -f $(OBJS) internal/*.o *.d

distclean: clean

maintainer-clean: distclean

.PHONY: all clean distclean maintainer-clean
# End - ReqID:TECC4.0