#-------------------------------------------------------------------------
#
# Makefile for src/oracle_test/authentication
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/oracle_test/authentication/Makefile
#
# add the file for requirement "SQL PARSER"
#
#-------------------------------------------------------------------------

subdir = src/oracle_test/authentication
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

oracle-check:
	$(oracle_prove_check)

oracle-installcheck:
	$(oracle_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
