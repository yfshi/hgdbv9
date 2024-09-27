# contrib/dict_int/Makefile

MODULE_big = dict_int
OBJS = \
	$(WIN32RES) \
	dict_int.o

EXTENSION = dict_int
DATA = dict_int--1.0.sql
PGFILEDESC = "dict_int - add-on dictionary template for full-text search"

REGRESS = dict_int
ORA_REGRESS = dict_int
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_REGRESS = mys_dict_int
#End - ReqID:SRS-MISC-MYSQL-FRAME

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/dict_int
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
