MODULE_big = hg_alarm

# Exported so the TAP test can gate itself on the build flag.
export enable_compatible_ak

OBJS = \
	$(WIN32RES) \
	hg_alarm.o \
	parse_int.o \
	json_opt.o \
	pqfe.o \
	memory.o \
	datadir.o \
	database.o \
	relation.o

EXTENSION = hg_alarm
DATA = hg_alarm--1.0.sql
PGFILEDESC = "hg_alarm"

# The alarm API (hg_alarm_check / hg_alarm_stat) always connects to the
# "highgo" database and reads a JSON config file from disk, so it cannot be
# exercised by a plain SQL regression test.  It is covered by the TAP test in
# t/001_hg_alarm.pl instead.
TAP_TESTS = 1

PG_CPPFLAGS = -I../include -I$(libpq_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

ifdef PGSQL
PG_CPPFLAGS += -D_PG_ALARM
endif

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
SHLIB_PREREQS = submake-libpq
subdir = contrib/hg_alarm
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

pqfe.o : ../common/pqfe.o
	mv $< $@
json_opt.o : ../common/json_opt.o
	mv $< $@
parse_int.o : ../common/parse_int.o
	mv $< $@
memory.o : ../common/memory.o
	mv $< $@
datadir.o : ../common/datadir.o
	mv $< $@
database.o : ../common/database.o
	mv $< $@
relation.o : ../common/relation.o
	mv $< $@
