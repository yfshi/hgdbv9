#-------------------------------------------------------------------------
#
# Makefile for src/bin/pg_alterckey
#
# Portions Copyright (c) 2025, IvorySQL-2.0
#
# src/bin/pg_alterckey/Makefile
#
# add the file for requirement "SRS-SECURITY-TDE"
#-------------------------------------------------------------------------

PGFILEDESC = "pg_alterckey - alter the cluster key"
PGAPPICON=win32

subdir = src/bin/pg_alterckey
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	$(WIN32RES) \
	pg_alterckey.o

all: pg_alterckey

pg_alterckey: $(OBJS) | submake-libpgport
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) pg_alterckey$(X) '$(DESTDIR)$(bindir)/pg_alterckey$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_alterckey$(X)'

clean distclean maintainer-clean:
	rm -f pg_alterckey$(X) $(OBJS)
	rm -rf tmp_check
