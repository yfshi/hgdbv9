# contrib/restricted_dba/Makefile

MODULE_big = restricted_dba
OBJS = \
	$(WIN32RES) \
	alter.o		\
	hooks.o		\
	user.o		\
	aclchk.o	\
	database.o	\
	instance.o	\
	helper.o	\
	tablespace.o\
	schema.o	\
	relation.o	\
	superuser.o	\
	proc.o

EXTENSION = restricted_dba
DATA = restricted_dba--1.0.sql
PGFILEDESC = "restricted_dba - restrict superuser privilege from dba"

REGRESS = role helper tablespace database schema table view sequence index		\
			function procedure routine aclchk guc misc 
# ORA_REGRESS: for oracle grammer sequence function/procedure/routine is different
ORA_REGRESS = role helper tablespace database schema table view sequence_ora 	\
			index function_ora procedure_ora routine_ora aclchk misc_ora
# MYS_REGRESS: add for latter
# MYS_REGRESS = role helper tablespace database schema table view sequence index		\
# 			function procedure routine aclchk misc

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/restricted_dba
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
