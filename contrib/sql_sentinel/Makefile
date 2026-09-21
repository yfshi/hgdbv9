# contrib/sql_sentinel/Makefile

MODULES = sql_sentinel

EXTENSION = sql_sentinel
DATA = sql_sentinel--1.0.sql
PGFILEDESC = "sql_sentinel - a SQL security filtering extension"

TAP_TESTS = 1

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/sql_sentinel/sql_sentinel.conf
REGRESS = sql_sentinel

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/sql_sentinel
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif


TAP_TESTS = 1

SHLIB_LINK += -lxml2


