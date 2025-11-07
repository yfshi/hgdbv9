#-------------------------------------------------------------------------
#
# Makefile for src/bin/pg_alterckey
#
# Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
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
	pg_alterckey.o \
	walenc.o \
	bufenc.o

override CPPFLAGS := -DFRONTEND $(CPPFLAGS)

all: pg_alterckey test_libpgtde_demo

pg_alterckey: $(OBJS) | submake-libpgport
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

walenc.c: % : $(top_srcdir)/src/backend/crypto/%
	rm -f $@ && $(LN_S) $< .

bufenc.c: % : $(top_srcdir)/src/backend/crypto/%
	rm -f $@ && $(LN_S) $< .

libpgtde.a: walenc.o bufenc.o
	$(AR) $(AROPT) $@ $^

test_libpgtde_demo: test_libpgtde_demo.o libpgtde.a
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -L. -lpgtde -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) pg_alterckey$(X) '$(DESTDIR)$(bindir)/pg_alterckey$(X)'
	$(INSTALL_DATA) libpgtde.a  '$(DESTDIR)$(libdir)'
	$(INSTALL_DATA) $(top_srcdir)/src/include/crypto/walenc.h  '$(DESTDIR)$(includedir)'
	$(INSTALL_DATA) $(top_srcdir)/src/include/crypto/bufenc.h  '$(DESTDIR)$(includedir)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_alterckey$(X)'

clean distclean maintainer-clean:
	rm -f pg_alterckey$(X) $(OBJS) walenc.c bufenc.c libpgtde.a
	rm -f test_libpgtde_demo test_libpgtde_demo.o
	rm -rf tmp_check

check:
	$(prove_check)
