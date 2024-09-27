#-------------------------------------------------------------------------
#
# Makefile for src/mysql_test/ldap
#
# Portions Copyright (c) 2022, IvorySQL-2.0
#
# src/mysql_test/ldap/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

subdir = src/mysql_test/ldap
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

export with_ldap

mysql-check:
	$(mysql_prove_check)

mysql-installcheck:
	$(mysql_prove_installcheck)

clean distclean maintainer-clean:
	rm -rf tmp_check
