# contrib/pg_wait_sampling/Makefile

MODULE_big = pg_wait_sampling
OBJS = pg_wait_sampling.o collector.o $(WIN32RES)

EXTENSION = pg_wait_sampling
DATA = pg_wait_sampling--1.1.sql pg_wait_sampling--1.0--1.1.sql
PGFILEDESC = "pg_wait_sampling - sampling based statistics of wait events"

REGRESS = load queries

EXTRA_REGRESS_OPTS=--temp-config=$(top_srcdir)/$(subdir)/conf.add

# Begin - ReqID:SRS-MISC-PDR
# Disabled because these tests require "shared_preload_libraries=pg_wait_sampling",
# which typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1
# End - ReqID:SRS-MISC-PDR

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_wait_sampling
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
