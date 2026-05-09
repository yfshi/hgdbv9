EXTENSION = credcheck
EXTVERSION = $(shell grep default_version $(EXTENSION).control | \
	       sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

# Uncomment the following two lines to enable cracklib support, adapt the path
# to the cracklib dictionary following your distribution
#PG_CPPFLAGS = -DUSE_CRACKLIB '-DCRACKLIB_DICTPATH="/usr/lib/cracklib_dict"'
#SHLIB_LINK = -lcrack

#PG_CPPFLAGS += -Wno-ignored-attributes

MODULE_big = credcheck
OBJS = credcheck.o logininfo.o $(WIN32RES)
PGFILEDESC = "credcheck - postgresql credential checker"

#DATA = $(wildcard updates/*--*.sql) $(EXTENSION)--$(EXTVERSION).sql
DATA = credcheck--2.8.0.sql

REGRESS_OPTS  = --inputdir=test --load-extension=credcheck --max-connections=1
TESTS = 01_username 02_password 03_rename 04_alter_pwd \
	05_reuse_history 06_reuse_interval 07_valid_until \
	08_creat_set fix_bug_login_display password_expiration_reminder

REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/credcheck
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
