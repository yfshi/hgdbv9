MODULE_big = hg_ddos_guard
OBJS = \
       $(WIN32RES) \
       hg_ddos_guard.o

EXTENSION = hg_ddos_guard
DATA = hg_ddos_guard--1.0.sql
PGFILEDESC = "hg_ddos_guard - client login and SQL rate limiting"

LDFLAGS_SL += $(filter -lm, $(LIBS))

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/hg_ddos_guard/hg_ddos_guard.conf
REGRESS = hg_ddos_guard
NO_INSTALLCHECK = 1

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_ddos_guard
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
