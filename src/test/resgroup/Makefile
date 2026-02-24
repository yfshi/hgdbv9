#-------------------------------------------------------------------------
#
# Makefile for src/test/resgroup
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/test/resgroup/Makefile
#
#-------------------------------------------------------------------------

subdir = src/test/resgroup
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# required for 017_shm.pl
REGRESS_SHLIB=$(abs_top_builddir)/src/test/regress/regress$(DLSUFFIX)
export REGRESS_SHLIB
REGRESS_OPTS += --REGEX

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
