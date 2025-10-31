# contrib/conn_restrict/Makefile

MODULE_big = conn_restrict
OBJS = conn_restrict.o $(WIN32RES)

EXTENSION = conn_restrict
DATA = conn_restrict--1.0.sql
PGFILEDESC = "conn_restrict support"

#REGRESS = conn_restrict

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/conn_restrict
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
