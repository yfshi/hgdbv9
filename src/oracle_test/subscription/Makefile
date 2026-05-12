#-------------------------------------------------------------------------
#
# Makefile for src/oracle_test/subscription
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/oracle_test/subscription/Makefile
#
# add the file for requirement "SQL PARSER"
#
#-------------------------------------------------------------------------

subdir = src/oracle_test/subscription
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

EXTRA_INSTALL = contrib/hstore

export with_icu

oracle-check:
	$(oracle_prove_check)

oracle-installcheck:
	$(oracle_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
