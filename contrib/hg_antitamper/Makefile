# contrib/hg_antitamper/Makefile

MODULE_big = hg_antitamper

OBJS = \
	$(WIN32RES) \
	hg_antitamper.o


EXTENSION = hg_antitamper
DATA = hg_antitamper--1.0.sql
PGFILEDESC = "hg_antitamper"

REGRESS = hg_antitamper
MYS_REGRESS = hg_antitamper
ORA_REGRESS = hg_antitamper

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_antitamper
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
