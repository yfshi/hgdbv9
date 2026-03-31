# contrib/hg_rolstatus/Makefile

MODULE_big = hg_rolstatus

OBJS = \
	$(WIN32RES) \
	hg_rolstatus.o


EXTENSION = hg_rolstatus
DATA = hg_rolstatus--1.0.sql
PGFILEDESC = "hg_rolstatus"

REGRESS = hg_rolstatus
MYS_REGRESS = mys_hg_rolstatus
ORA_REGRESS = hg_rolstatus

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_rolstatus
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
