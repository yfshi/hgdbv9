# contrib/dblink/Makefile

MODULE_big = dblink
OBJS = \
	$(WIN32RES) \
	dblink.o
PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

EXTENSION = dblink
DATA = dblink--1.2.sql dblink--1.1--1.2.sql dblink--1.0--1.1.sql
PGFILEDESC = "dblink - connect to other PostgreSQL databases"

REGRESS = paths dblink
REGRESS_OPTS = --dlpath=$(top_builddir)/src/test/regress
# Begin - BUGM0000294
ORA_REGRESS = paths	ivy_dblink
# End - BUGM0000294
# BEGIN - SQL PARSER
ORACLE_REGRESS_OPTS = --dlpath=$(top_builddir)/src/oracle_test/regress
# Begin - ReqID:SRS-MISC-MYSQL-FRAME 
MYS_REGRESS = paths mys_dblink
MYSQL_REGRESS_OPTS = --dlpath=$(top_builddir)/src/mysql_test/regress
# End - ReqID:SRS-MISC-MYSQL-FRAME
# END - SQL PARSER

EXTRA_CLEAN = sql/paths.sql expected/paths.out

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
SHLIB_PREREQS = submake-libpq
subdir = contrib/dblink
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

all: ../../src/oracle_test/regress/regress$(DLSUFFIX) ../../src/mysql_test/regress/regress$(DLSUFFIX)

../../src/oracle_test/regress/egress$(DLSUFFIX): ../../src/oracle_test/regress/oraregress$(DLSUFFIX)
../../src/mysql_test/regress/egress$(DLSUFFIX): ../../src/mysql_test/regress/mysregress$(DLSUFFIX)
	        cp $< $@
