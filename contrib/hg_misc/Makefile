# contrib/hg_misc/Makefile

MODULE_big = hg_misc
OBJS = \
	$(WIN32RES) \
	hg_misc.o lic_views.o
	
EXTENSION = hg_misc
DATA = hg_misc--1.0.sql
SRCS = hg_misc.c lic_views.c
PGFILEDESC = "hg_misc"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_misc
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
