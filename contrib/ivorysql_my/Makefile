# contrib/ivorysql_my/Makefile

MODULE_big = ivorysql_my

OBJS = \
	$(WIN32RES) \
	src/ivorysql_my.o

EXTENSION = ivorysql_my

# Begin - ReqID:SRS-MISC-SQLMERGE
# SQL(s) which need to be generated only if which is specified here.
DATA = ivorysql_my--1.0.sql

all: gensql

clean: cleansql

# SQL(s) merge using perl
gensql: gensql.pl
	$(PERL) $< $(patsubst ivorysql_my--%.sql, %, $(DATA))

# Clean the generated SQL(s).
cleansql:
	rm -f $(DATA)
# End - ReqID:SRS-MISC-SQLMERGE

# REGRESS_OPTS mainly used for installcheck or debug.
#REGRESS_OPTS = --port=3306
MYS_REGRESS = multi_update \
			  insert_update \

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/ivorysql_my
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif



