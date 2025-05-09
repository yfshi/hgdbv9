#-------------------------------------------------------------------------
#
# Makefile for src/bin/pg_pgpass
#
# Copyright (c) 1998-2021, PostgreSQL Global Development Group
#
# src/bin/pg_pgpass/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "pg_pgpass - Encrypt or decrypt the.pgpass file"
PGAPPICON=win32

subdir = src/bin/pg_pgpass
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	$(WIN32RES) \
	pg_pgpass.o

all: pg_pgpass

pg_pgpass: $(OBJS) | submake-libpgport
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)

install: all installdirs
	$(INSTALL_PROGRAM) pg_pgpass$(X) '$(DESTDIR)$(bindir)/pg_pgpass$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_pgpass$(X)'

clean distclean maintainer-clean:
	rm -f pg_pgpass$(X) $(OBJS)
	rm -rf tmp_check

