# contrib/pg_object/Makefile

MODULE_big = pg_object
OBJS = \
	$(WIN32RES) \
	src/pg_object.o \
        src/pg_object_interface.o \
	src/pg_object_to_relcache.o

EXTENSION = pg_object
DATA = pg_object--1.0.sql
PGFILEDESC = "pg_object - Store objects in Oracle or MySQL schema"

#clean:
#	rm -rf log results tmp_check
#	rm -r regression.diffs

#ORA_REGRESS = pg_object

#MYS_REGRESS = pg_object 

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_object
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
