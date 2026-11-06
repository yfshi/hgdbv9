#
# Makefile for utils/tecc
#
# src/backend/utils/tecc/Makefile
#

subdir = src/backend/utils/tecc
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -I. -I$(srcdir) $(CPPFLAGS)

# TECC utility functions
OBJS = \
	cipherdatatype.o \
	cipherdata_operators.o \
	tecc_typeicu.o \
	tecc_type_mapping.o \
	tecc_typecast.o \
	tecc_index.o \
	cipherdata_hex.o \
	tecc_error.o

include $(top_srcdir)/src/backend/common.mk

# TECC后端文件头文件依赖：修改核心头文件时触发重编译
TECC_HEADERS = $(top_srcdir)/src/include/utils/cipherdatatype.h \
               $(top_srcdir)/src/include/utils/tecc/cipherdata_hex.h \
               $(top_srcdir)/src/include/utils/tecc/tecc_error.h \
               $(top_srcdir)/src/include/utils/tecc/tecc_typecast.h \
               $(top_srcdir)/src/include/utils/tecc/tecc_typeicu.h \
               $(top_srcdir)/src/include/utils/tecc/tecc_type_mapping.h \
               $(top_srcdir)/src/include/utils/tecc/tecc_index.h

$(OBJS): $(TECC_HEADERS)