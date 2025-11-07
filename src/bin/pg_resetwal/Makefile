#-------------------------------------------------------------------------
#
# Makefile for src/bin/pg_resetwal
#
# Copyright (c) 1998-2021, PostgreSQL Global Development Group
#
# src/bin/pg_resetwal/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "pg_resetwal - reset PostgreSQL WAL log"
PGAPPICON=win32

subdir = src/bin/pg_resetwal
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	$(WIN32RES) \
	pg_resetwal.o \
	walenc.o

override CPPFLAGS := -I$(libpq_srcdir) -DFRONTEND $(CPPFLAGS)

all: pg_resetwal

pg_resetwal: $(OBJS) | submake-libpgport
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

walenc.c: % : $(top_srcdir)/src/backend/crypto/%
	rm -f $@ && $(LN_S) $< .

install: all installdirs
	$(INSTALL_PROGRAM) pg_resetwal$(X) '$(DESTDIR)$(bindir)/pg_resetwal$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_resetwal$(X)'

clean distclean maintainer-clean:
	rm -f pg_resetwal$(X) $(OBJS) walenc.c
	rm -rf tmp_check

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)

#BEGIN - SQL PARSER
oracle-check:
	$(oracle_prove_check)

oracle-installcheck:
	$(oracle_prove_installcheck)
#END - SQL PARSER
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
mysql-check:
	$(mysql_prove_check)

mysql-installcheck:
	$(mysql_prove_installcheck)
#End - ReqID:SRS-MISC-MYSQL-FRAME
