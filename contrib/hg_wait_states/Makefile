# contrib/hg_wait_states/Makefile

MODULE_big = hg_wait_states
OBJS = \
	$(WIN32RES) \
	hg_wait_states.o

EXTENSION = hg_wait_states
DATA = \
	hg_wait_states--1.0.sql
PGFILEDESC = "hg_wait_states - background wait event sampling"

# hg_wait_states must be loaded via shared_preload_libraries so installcheck
# (which connects to an external cluster) cannot exercise the collector;
# the TAP suite under t/ spins up a dedicated cluster instead.
NO_INSTALLCHECK = 1
TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_wait_states
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
