# contrib/file_fdw/Makefile

MODULES = file_fdw

EXTENSION = file_fdw
DATA = file_fdw--1.0.sql
PGFILEDESC = "file_fdw - foreign data wrapper for files"

REGRESS = file_fdw
ORA_REGRESS = ivy_file_fdw
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_REGRESS = mys_file_fdw
#End - ReqID:SRS-MISC-MYSQL-FRAME

EXTRA_CLEAN = sql/file_fdw.sql expected/file_fdw.out sql/ivy_file_fdw.sql expected/ivy_file_fdw.out		# BUGM0000266

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/file_fdw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
