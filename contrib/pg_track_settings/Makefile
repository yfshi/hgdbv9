MODULE_big = pg_track_settings
OBJS = pg_track_settings.o  $(WIN32RES)

EXTENSION    = pg_track_settings
EXTVERSION   = $(shell grep default_version $(EXTENSION).control | sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")
#TESTS        = $(wildcard test/sql/*.sql)
#REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
#REGRESS_OPTS = --inputdir=test
#DOCS         = $(wildcard README.md)
REGRESS = pg_track_settings

#DATA         = $(wildcard *--*.sql)
DATA         = pg_track_settings--1.0.0.sql pg_track_settings--1.0.0--1.0.1.sql \
				pg_track_settings--1.0.1.sql pg_track_settings--1.0.1--1.1.0.sql \
				pg_track_settings--1.1.0.sql pg_track_settings--2.0.0.sql \
				pg_track_settings--2.0.0--2.0.1.sql pg_track_settings--2.0.1.sql \
				pg_track_settings--2.0.1--2.1.0.sql pg_track_settings--2.1.0.sql \
				pg_track_settings--2.1.0--2.1.2.sql pg_track_settings--2.1.2.sql
				
				
ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_track_settings
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

all:

release-zip: all
	git archive --format zip --prefix=${EXTENSION}-${EXTVERSION}/ --output ./${EXTENSION}-${EXTVERSION}.zip HEAD
	unzip ./${EXTENSION}-$(EXTVERSION).zip
	rm ./${EXTENSION}-$(EXTVERSION).zip
	rm ./${EXTENSION}-$(EXTVERSION)/.gitignore
	sed -i -e "s/__VERSION__/$(EXTVERSION)/g"  ./${EXTENSION}-$(EXTVERSION)/META.json
	zip -r ./${EXTENSION}-$(EXTVERSION).zip ./${EXTENSION}-$(EXTVERSION)/
	rm ./${EXTENSION}-$(EXTVERSION) -rf
