MODULE_big = hg_flashback_db
OBJS = hg_flashback_db.o bgw.o restore_point.o pgbackrest.o

EXTENSION = hg_flashback_db
DATA = hg_flashback_db--1.0.sql

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_flashback_db
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
