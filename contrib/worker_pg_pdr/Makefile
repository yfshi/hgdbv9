# contrib/worker_pg_pdr/Makefilei

MODULES = worker_pg_pdr

EXTENSION = worker_pg_pdr
DATA = worker_pg_pdr--1.0.sql pg_pdr.html
PGFILEDESC = "worker_pg_pdr - performance diagnosis collector"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/worker_pg_pdr
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
