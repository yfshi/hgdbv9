# contrib/mys_btree_gin/Makefile

MODULE_big = mys_btree_gin
OBJS = \
	$(WIN32RES) \
	mys_btree_gin.o \


EXTENSION = mys_btree_gin
DATA = mys_btree_gin--1.0.sql

PGFILEDESC = "mys_btree_gin - B-tree equivalent GIN operator classes"

MYS_REGRESS = init year date datetime timestamp time

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/mys_btree_gin
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
