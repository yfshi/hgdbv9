# contrib/pg_sysmac/Makefile

MODULE_big = hg_sysmac

OBJS = \
	$(WIN32RES) \
	label.o \
	hooks.o \
	dml.o	\
	row_level.o


EXTENSION = hg_sysmac
DATA = hg_sysmac--1.0.sql
PGFILEDESC = "hg_sysmac"

REGRESS = mac inherit partition

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_sysmac
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
