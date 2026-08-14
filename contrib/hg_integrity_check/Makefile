# contrib/hg_integrity_check/Makefile

MODULE_big = hg_integrity_check

OBJS = \
	$(WIN32RES) \
	hg_integrity_check.o


EXTENSION = hg_integrity_check
DATA = hg_integrity_check--1.0.sql hg_integrity_check--1.0--1.1.sql
PGFILEDESC = "hg_integrity_check"

TAP_TESTS = 1

#REGRESS = hg_integrity_check
##MYS_REGRESS = hg_integrity_check
#ORA_REGRESS = hg_integrity_check

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_integrity_check
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
