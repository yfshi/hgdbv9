#-------------------------------------------------------------------------
#
# Makefile for src/mysql_test/kerberos
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/oracle_test/kerberos/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

subdir = src/mysql_test/kerberos
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

export with_gssapi with_krb_srvnam

mysql-check:
	$(mysql_prove_check)

mysql-installcheck:
	$(mysql_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
