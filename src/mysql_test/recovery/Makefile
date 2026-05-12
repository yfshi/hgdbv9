#-------------------------------------------------------------------------
#
# Makefile for src/mysql_test/recovery
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/mysql_test/recovery/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

EXTRA_INSTALL=contrib/test_decoding

subdir = src/mysql_test/recovery
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# required for 017_shm.pl
REGRESS_SHLIB=$(abs_top_builddir)/src/mysql_test/regress/mysregress$(DLSUFFIX)
export REGRESS_SHLIB

mysql-check:
	$(mysql_prove_check)

mysql-installcheck:
	$(mysql_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
