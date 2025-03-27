# contrib/ivorysql_my/Makefile

MODULE_big = ivorysql_my

OBJS = \
	$(WIN32RES) \
	src/ivorysql_my.o \
	src/case_sensitive/casesensitive.o \
	src/case_sensitive/casefunc.o \
	src/datatype/myyear.o

EXTENSION = ivorysql_my

# Begin - ReqID:SRS-MISC-SQLMERGE
# SQL(s) which need to be generated only if which is specified here.
DATA = ivorysql_my--1.0.sql

all: gensql

# Begin - ReqID:SRS-MYSQL-CASE
install: install-data
# End - ReqID:SRS-MYSQL-CASE

clean: cleansql

# SQL(s) merge using perl
gensql: gensql.pl
	$(PERL) $< $(patsubst ivorysql_my--%.sql, %, $(DATA))

# Begin - ReqID:SRS-MYSQL-CASE
install-data:
	$(INSTALL_DATA) $(srcdir)/preload_mys_misc.sql '$(DESTDIR)$(datadir)/preload_mys_misc.sql'
# End- ReqID:SRS-MYSQL-CASE

# Clean the generated SQL(s).
cleansql:
	rm -f $(DATA)
# End - ReqID:SRS-MISC-SQLMERGE

# REGRESS_OPTS mainly used for installcheck or debug.
#REGRESS_OPTS = --port=3306
MYS_REGRESS = multi_update \
			  insert_update \
			  multi_delete \
			  replace \
			  multi_partition \
			  casesensitive \
			  casesensitive0 \
			  casesensitive1 \
			  casesensitive2 \
			  insert_updatecase \
			  multi_updatecase \
			  multi_deletecase \
			  mys_use_case \
			  mys_year

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
