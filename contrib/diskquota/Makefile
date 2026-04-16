# contrib/diskquota/Makefile

MODULE_big = diskquota

OBJS = diskquota.o enforcement.o quotamodel.o activetable.o pg_utils.o

EXTENSION = diskquota
DATA = diskquota--1.0.sql
PGFILEDESC = "diskquota - diskquota set role or schema"

REGRESS = test_diskquota_info test_min_quota test_role test_schema test_rename test_insert_after_drop test_column test_schema_transaction test_role_transaction test_toast test_partition test_extension
REGRESS_OPTS = --temp-config=test_diskquota.conf

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/diskquota
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

