#-------------------------------------------------------------------------
#
# Makefile for src/bin/wrap
#
#-------------------------------------------------------------------------

PGFILEDESC = "wrap - wrap PL/SQL source"
PGAPPICON = win32

subdir = src/bin/wrap
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -DFRONTEND $(CPPFLAGS)
LDFLAGS_INTERNAL += -L$(top_builddir)/src/common -lpgcommon \
					-L$(top_builddir)/src/port -lpgport

OBJS = \
	$(WIN32RES) \
	wrap.o

TAP_TESTS = 1

all: wrap

wrap: $(OBJS) | submake-libpgport
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) wrap$(X) '$(DESTDIR)$(bindir)/wrap$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'
	$(MKDIR_P) '$(DESTDIR)$(libdir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/wrap$(X)'

clean distclean maintainer-clean:
	rm -f wrap$(X) $(OBJS)
	rm -rf tmp_check

oracle-check:
	$(prove_check)

installcheck:
	$(prove_installcheck)
