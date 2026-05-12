#-------------------------------------------------------------------------
#
# Makefile for src/test/authentication
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/mysql_test/authentication/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

subdir = src/mysql_test/authentication
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

mysql-check:
	$(mysql_prove_check)

mysql-installcheck:
	$(mysql_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
