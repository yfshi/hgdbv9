# contrib/ora_synonym/Makefile

MODULE_big = ora_synonym
OBJS = \
	$(WIN32RES) \
	synonym.o

EXTENSION = ora_synonym
DATA = ora_synonym--1.0.sql
PGFILEDESC = "ora_synonym - compatible oracle synonym"

HEADERS = synonym.h

ORA_REGRESS = synonym

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/ora_synonym
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
