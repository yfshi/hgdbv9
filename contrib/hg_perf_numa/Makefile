# contrib/hg_perf_numa/Makefile

MODULE_big = hg_perf_numa
OBJS = \
	$(WIN32RES) \
	numa.o

EXTENSION = hg_perf_numa
DATA = hg_perf_numa--1.0.sql
PGFILEDESC = "hg_perf_numa"

SHLIB_LINK += -lnuma

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_perf_numa
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif