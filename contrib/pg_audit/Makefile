# contrib/pg_audit/Makefile

MODULE_big = pg_audit
OBJS = pg_audit.o  $(WIN32RES)

EXTENSION = pg_audit
DATA = pg_audit--1.0.sql sqlite.db
PGFILEDESC = "pg_audit - hgdb audit control"
#SHLIB_LINK += -L$(top_builddir)/src/port -lpgport -lpgport_shlib -lpgport_srv

REGRESS = pg_audit
ORA_REGRESS = pg_audit

TAP_TESTS = 1

SQLITE_LIB = sqlite3

SHLIB_LINK := -lsqlite3

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_audit
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif


#ifeq ($(PORTNAME), win32)
#SHLIB_LINK_INTERNAL += $(sort $(wildcard ../../src/port/libpgport.a))
#else
#SHLIB_LINK += -L$(top_builddir)/src/port -lpgport
#endif

