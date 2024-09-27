#-------------------------------------------------------------------------
#
# Makefile for src/mysql_test/kerberos
#
# Portions Copyright (c) 2023, IvorySQL-2.0
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
