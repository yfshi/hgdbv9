# contrib/ivorysql_ora/Makefile

MODULE_big = ivorysql_ora

OBJS = \
	$(WIN32RES) \
	src/ivorysql_ora.o \
	src/datatype/oracharchar.o \
	src/datatype/oracharbyte.o \
	src/datatype/oravarcharchar.o \
	src/datatype/oravarcharbyte.o \
	src/datatype/oradate.o \
	src/datatype/oratimestamp.o \
	src/datatype/oratimestamptz.o \
	src/datatype/oratimestampltz.o \
	src/datatype/yminterval.o \
	src/datatype/dsinterval.o \
	src/datatype/compatible_oracle_precedence.o \
	src/datatype/common_datatypes.o \
	src/datatype/raw_long.o \
	src/datatype/binary_float.o \
	src/datatype/binary_double.o \
	src/dmcompatible/dmchartype_compatible.o \
	src/builtin_functions/character_datatype_functions.o \
	src/builtin_functions/datetime_datatype_functions.o \
	src/builtin_functions/numeric_datatype_functions.o \
	src/builtin_functions/misc_functions.o	\
	src/insert/insert_alias.o \
	src/merge/ora_merge.o \
	src/sysview/sysview_functions.o \
	src/xml_functions/ora_xml_functions.o \
	src/nlssort/nlssort.o \
	src/orarefcursor/orarefcursor.o \
	src/builtin_packages/dbms_job/ora_dbms_job.o \
	src/builtin_packages/dbms_job/ora_dbms_jobparse.o \
	src/datatype/orachar_varchar.o \
	src/builtin_packages/dbms_output/dbms_output.o \
	src/ora_trigger_pred/ora_trigger_pred.o  \
	src/collection/collection.o \
	src/tstl2cl/c_algo.o \
	src/tstl2cl/c_function.o \
	src/tstl2cl/c_list.o \
	src/tstl2cl/c_memory.o \
	src/tstl2cl/c_vector.o \
	src/tstl2cl/c_pair.o \
	src/tstl2cl/c_tree.o \
	src/tstl2cl/c_set.o \
	src/tstl2cl/c_map.o \
	src/tstl2cl/c_queue.o \
	src/tstl2cl/c_stack.o \
	src/tstl2cl/c_iterator.o

EXTENSION = ivorysql_ora

# Begin - ReqID:SRS-MISC-SQLMERGE
# SQL(s) which need to be generated only if which is specified here.
DATA = ivorysql_ora--1.0.sql

all: gensql

src/builtin_packages/dbms_job/ora_dbms_jobparse.o: src/builtin_packages/dbms_job/ora_dbms_jobscan.c # ReqID:SRS-PLSQL-DBMS_JOB

install: install-data  install-dbms-job	# BUG#M0000195 ReqID:SRS-PLSQL-DBMS_JOB

clean: cleansql

# SQL(s) merge using perl
gensql: gensql.pl
	$(PERL) $< $(patsubst ivorysql_ora--%.sql, %, $(DATA))

# Begin - BUG#M0000195
# Preload the compatible oracle misc sql script
install-data:
	$(INSTALL_DATA) $(srcdir)/preload_ora_misc.sql '$(DESTDIR)$(datadir)/preload_ora_misc.sql'
# End - BUG#M0000195

# Begin - ReqID:SRS-PLSQL-DBMS_JOB
install-dbms-job:
	$(INSTALL_DATA) $(srcdir)/src/builtin_packages/dbms_job/dbms_job_pgagent/* '$(DESTDIR)$(datadir)/extension/'
# End - ReqID:SRS-PLSQL-DBMS_JOB

# Clean the generated SQL(s).
cleansql:
	rm -f $(DATA)
# End - ReqID:SRS-MISC-SQLMERGE

# REGRESS_OPTS mainly used for installcheck or debug.
#REGRESS_OPTS = --port=1521
ORA_REGRESS = \
	ora_character \
	ora_datetime \
	ora_interval \
	ora_number \
	ora_binary_float \
	ora_binary_double \
	ora_raw_long \
	ora_character_datatype_functions \
	ora_datetime_datatype_functions \
	ora_misc_functions \
	ora_insert_alias \
	ora_merge \
	datatype_and_func_bugs \
	ora_sysview \
	ora_xml_functions \
	ora_to_number \
	ora_order_by_null \
	ora_nlssort \
	ora_nlssort_utf8 \
	ora_nlssort_gbk \
	ora_nlssort_gb18030 \
	ora_nlssort_bug \
	ora_default_collation \
	orarefcursor \
	dm_compatible \
	ora_dbms_job \
	decode \
	ora_dbms_metadata \
	aggregates_without_group_by \
	ora_dbms_output \
	ora_collection \
	ora_buildin_func \
	ora_bulk_collect \
	ora_wmsys_wmconcat

#/* Begin - ReqID:SRS-PLSQL-Collection */
PG_CPPFLAGS = -I$../src/tstl2cl/include -I$../src/tstl2cl -I$(srcdir) -I$../src/include
#/* End - ReqID:SRS-PLSQL-Collection */

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/ivorysql_ora
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# Begin - ReqID:SRS-PLSQL-DBMS_JOB
maintainer-clean:
	rm -f ./src/builtin_packages/dbms_job/ora_dbms_jobparse.c ./src/builtin_packages/dbms_job/ora_dbms_jobscan.c
# End - ReqID:SRS-PLSQL-DBMS_JOB
