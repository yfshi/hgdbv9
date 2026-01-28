#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for utils/sqlresourcelimit
#
# IDENTIFICATION
#    src/backend/utils/sqlresourcelimit/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/utils/sqlresourcelimit
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -I. -I$(srcdir) $(CPPFLAGS)

OBJS = \
	sqlcpulimit_v1.o \
	sqlcpulimit_v2.o \
	sqlcpulimitcommon.o \
	sqlresourcelimit.o

include $(top_srcdir)/src/backend/common.mk
