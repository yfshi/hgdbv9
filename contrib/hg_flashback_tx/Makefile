# contrib/hg_flashback_tx/Makefile

MODULES = hg_flashback_tx flashback_decoder
EXTENSION = hg_flashback_tx
DATA = hg_flashback_tx--1.0.sql
TAP_TESTS = 1

PGFILEDESC = "hg_flashback_tx - flashback functionality for PostgreSQL"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_flashback_tx
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
