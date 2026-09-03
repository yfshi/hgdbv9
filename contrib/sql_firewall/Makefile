# contrib/sql_firewall/Makefile

MODULE_big = sql_firewall
OBJS = sql_firewall.o $(WIN32RES)

EXTENSION = sql_firewall
DATA = sql_firewall--1.0.sql

PGFILEDESC = "sql_firewall - support sql firewall"

LDFLAGS_SL += $(filter -lm, $(LIBS))

EXTRA_CLEAN = -r $(pg_regress_clean_files) temp \
	expected/import-and-export-test.out \
	expected/import-black-and-white-list-for-all-user.out \
	expected/import-black-and-white-list-for-single-user.out \
	expected/all-functions-and-relations-permissions.out \
	sql/import-and-export-test.sql \
	sql/import-black-and-white-list-for-all-user.sql \
	sql/import-black-and-white-list-for-single-user.sql \
	sql/all-functions-and-relations-permissions.sql

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/sql_firewall/sql_firewall.conf
REGRESS = learning-permissive-enforcing-disable \
	import-black-and-white-list-for-all-user \
	import-black-and-white-list-for-single-user \
	import_single_sql_of_white_list \
	import_single_sql_of_black_list \
	import-and-export-test \
	other-functions \
	all-functions-and-relations-permissions

# Disabled because these tests require "shared_preload_libraries="sql_firewall",
# # which typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

REGRESS_PREP = temp

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/sql_firewall
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

.PHONY: temp
temp:
	$(MKDIR_P) $@
