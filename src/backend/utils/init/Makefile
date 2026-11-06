#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for utils/init
#
# IDENTIFICATION
#    src/backend/utils/init/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/utils/init
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	globals.o \
	miscinit.o \
	postinit.o \
	tecc_global_mark.o

include $(top_srcdir)/src/backend/common.mk

# TECC后端文件头文件依赖
tecc_global_mark.o: $(top_srcdir)/src/include/utils/tecc_global_mark.h
