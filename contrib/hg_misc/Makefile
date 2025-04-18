# contrib/hg_misc/Makefile

# Load feature control via configure
include ./hg_misc.global
# SQL marcos are used to generate hg_misc--1.0.sql.
# If you add a new module, don't forget add corresponding here.
SQL_MARCO = -P

MODULE_big = hg_misc
OBJS = \
	$(WIN32RES) \
	src/hg_misc.o 

# if --with-license, for hg_misc/license
ifeq ($(with_license),yes)
OBJS += src/license/lic_views.o
SQL_MARCO += -DUSE_LICENSE
endif

# if --enable-internal-guc, for hg_misc/internal_guc
# ReqID:SRS-Performance-GUC
ifeq ($(enable_internal_guc),yes)
OBJS += src/internal_guc/internal.o
SQL_MARCO += -DMISC_INTERNAL
endif
	
EXTENSION = hg_misc
DATA = hg_misc--1.0.sql
SRC = src/hg_misc.c src/license/lic_views.c
PGFILEDESC = "hg_misc"

# Clean the generated SQL(s).
EXTRA_CLEAN = $(DATA)

all: hg_misc--1.0.sql

maintainer-clean:
	rm hg_misc.global

# SQL(s) merge
hg_misc--1.0.sql: src/sql/hg_misc.in
	cpp $(SQL_MARCO) $< | sed 's/^# /-- /g' > $@


ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hg_misc
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif