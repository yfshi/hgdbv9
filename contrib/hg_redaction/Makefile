# contrib/hg_redaction/Makefile

MODULE_big = hg_redaction
OBJS = hg_redaction.o RedactionPolicyCmd.o RedactionPolicyAnalyze.o RedactionPolicyTableCmd.o $(WIN32RES)

EXTENSION = hg_redaction

DATA = hg_redaction--1.0.sql

PGFILEDESC = "hg_redaction - make data support redaction"

LDFLAGS_SL += $(filter -lm, $(LIBS))

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/hg_redaction/hg_redaction.conf
REGRESS = hg_redaction
# Disabled because these tests require "shared_preload_libraries="hg_redaction",
# which typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

ifdef USE_PGXS
	PG_CONFIG = pg_config
	PGXS := $(shell $(PG_CONFIG) --pgxs)
	include $(PGXS)
else
	subdir = contrib/hg_redaction
	top_builddir = ../..
	include $(top_builddir)/src/Makefile.global
	include $(top_srcdir)/contrib/contrib-global.mk
endif
