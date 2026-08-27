# contrib/pg_audit/Makefile

MODULE_big = pg_audit
OBJS = pg_audit.o  $(WIN32RES)

TAP_TESTS = 1

EXTENSION = pg_audit

PGFILEDESC = "pg_audit - hgdb audit control"

DATA = pg_audit--1.0.sql pg_audit.control \
	pg_audit--1.1.sql pg_audit--1.0--1.1-sqlite.sql \
	pg_audit--1.2.sql pg_audit--1.1--1.2-sqlite.sql \
	sqlite.db

REGRESS = pg_audit
ORA_REGRESS = ivy_audit

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

#SHLIB_LINK += -L$(top_builddir)/src/port -lpgport -lpgport_shlib -lpgport_srv

#REGRESS = pg_audit
#ORA_REGRESS = pg_audit

TAP_TESTS = 1

ifeq ($(with_sqlite), yes)
SQLITE_LIB = sqlite3

SHLIB_LINK := -lsqlite3
endif



#ifeq ($(PORTNAME), win32)
#SHLIB_LINK_INTERNAL += $(sort $(wildcard ../../src/port/libpgport.a))
#else
#SHLIB_LINK += -L$(top_builddir)/src/port -lpgport
#endif

