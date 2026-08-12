# contrib/hg_profiler/Makefile

MODULE_big = hg_profiler
OBJS = \
	$(WIN32RES) \
	hg_profiler.o

EXTENSION = hg_profiler
DATA = hg_profiler--1.0.sql
PGFILEDESC = "hg_profiler - per-query SQL execution profiler"

# Module hooks into shared memory and the executor; loading via
# shared_preload_libraries is required, so installcheck cannot exercise
# the trace path.
NO_INSTALLCHECK = 1

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_profiler
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
