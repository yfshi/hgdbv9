# src/test/modules/test_sm4/Makefile

MODULE_big = test_sm4
OBJS = \
	$(WIN32RES) \
	test_sm4_basic.o

PGFILEDESC = "test_sm4 - SM4 algorithm test module"

REGRESS = test_sm4

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = src/test/modules/test_sm4
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_builddir)/contrib/contrib-global.mk
endif

# Link with OpenSSL for SM4/SM3 support
LIBS += -lssl -lcrypto