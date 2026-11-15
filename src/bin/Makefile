#-------------------------------------------------------------------------
#
# Makefile for src/bin (client programs)
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/bin/Makefile
#
#-------------------------------------------------------------------------

subdir = src/bin
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

SUBDIRS = \
	initdb \
	pg_amcheck \
	pg_archivecleanup \
	pg_basebackup \
	pg_checksums \
	pg_config \
	pg_controldata \
	pg_ctl \
	pg_dump \
	pg_resetwal \
	pg_rewind \
	pg_test_fsync \
	pg_test_timing \
	pg_upgrade \
	pg_verifybackup \
	pg_waldump \
	pgbench \
	psql \
	wrap \
	scripts \
        tecc_ktool

# Begin - ReqID:SRS-SECURITY-IDENTITYAUTH
SUBDIRS += pg_pgpass

ifeq ($(PORTNAME), win32)
SUBDIRS += pgevent
else
ALWAYS_SUBDIRS += pgevent
endif

ifeq ($(with_license), yes)
SUBDIRS += licchk
endif

ifeq ($(with_ssl),openssl)
SUBDIRS += pg_alterckey
endif

ifeq ($(enable_dky_beta_feature), yes)
SUBDIRS += hg_watchdog
endif

$(recurse)
$(recurse_always)
