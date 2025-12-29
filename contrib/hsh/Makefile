# contrib/hsh/Makefile

MODULES = hsh

EXTENSION = hsh
DATA = hsh--1.0.sql
PGFILEDESC = "HSH - Highgo Session History"

# Disabled because these tests require "shared_preload_libraries=hsh",
# which typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hsh
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

