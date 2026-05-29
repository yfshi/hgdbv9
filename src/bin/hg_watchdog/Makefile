#-------------------------------------------------------------------------
#
# Makefile for src/bin/hg_watchdog
#
#
# src/bin/hg_watchdog/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "hg_watchdog - the Highgo Program Shell"

subdir = src/bin/hg_watchdog

top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -I$(libpq_srcdir) $(CPPFLAGS)
OBJS = hg_watchdog.o

all: hg_watchdog

SHLIB_LINK := -lsqlite3

hg_watchdog: $(OBJS) | submake-libpgport $(SUBMAKE_LIBPQ)
	$(CC) $(OBJS) $(SHLIB_LINK) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) hg_watchdog$(X) '$(DESTDIR)$(bindir)/hg_watchdog$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'
# Begin - BUG#Z8011
ifeq ($(enable_dky_beta_feature), yes)
	$(INSTALL_DATA) hg_watchdog.conf.sample '$(DESTDIR)$(datadir)/hg_watchdog.conf.sample'
	$(INSTALL_DATA) hg_watchdog.service.sample '$(DESTDIR)$(datadir)/hg_watchdog.service.sample'
endif
# End - BUG#Z8011

clean distclean maintainer-clean:
	rm -rf $(OBJS) hg_watchdog
