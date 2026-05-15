#-------------------------------------------------------------------------
#
# Makefile for src/oracle_test/kerberos
#
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# src/oracle_test/kerberos/Makefile
#
# add the file for requirement "SQL PARSER"
#
#-------------------------------------------------------------------------

subdir = src/oracle_test/kerberos
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

export with_gssapi with_krb_srvnam

oracle-check:
	$(oracle_prove_check)

oracle-installcheck:
	$(oracle_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
