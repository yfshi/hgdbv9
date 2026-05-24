# src/mysql_test/modules/test_mys_case/Makefile
# Begin - ReqID:SRS-MYSQL-CASE

PGFILEDESC = "test_mys_case - regression tests for mys_case_switch"

EXTENSION = mys_ext_case_off mys_ext_case_on
DATA = mys_ext_case_off--1.0.sql mys_ext_case_on--1.0.sql

REGRESS = test_mys_case

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = src/mysql_test/modules/test_mys_case
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# End - ReqID:SRS-MYSQL-CASE
