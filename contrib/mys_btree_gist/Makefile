# contrib/mys_btree_gist/Makefile

MODULE_big = mys_btree_gist

OBJS =  \
	$(WIN32RES) \
	btree_gist.o \
	btree_utils_num.o \
	btree_year.o

EXTENSION = mys_btree_gist
DATA = mys_btree_gist--1.0.sql
PGFILEDESC = "mys_btree_gist - B-tree equivalent GiST operator classes"

MYS_REGRESS = init year
EXTRA_INSTALL = contrib/ivorysql_my

SHLIB_LINK += $(filter -lm, $(LIBS))

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/mys_btree_gist
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
