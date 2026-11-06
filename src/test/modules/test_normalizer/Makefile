# Makefile for testing TECC normalizer
# ReqID:TECC4.0

MODULE_big = test_normalizer
OBJS = test_normalizer_basic.o

PGFILEDESC = "test_normalizer - test TECC normalization interface layer"

REGRESS = test_normalizer_basic

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = src/test/modules/test_normalizer
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_builddir)/contrib/contrib-global.mk
endif