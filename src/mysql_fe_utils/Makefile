#-------------------------------------------------------------------------
#
# Makefile for src/mysql_fe_utils
#
# This makefile generates a static library, libmysfeutils.a,
# for use by client applications
#
# Portions Copyright (c) 2023, IvorySQL2.0
#
# IDENTIFICATION
#    src/mysql_fe_utils/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

subdir = src/oracle_fe_utils
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -DFRONTEND -I$(libpq_srcdir) $(CPPFLAGS)

OBJS = mys_string_utils.o \
	mys_psqlscan.o

all: libmysfeutils.a

libmysfeutils.a: $(OBJS)
	rm -f $@
	$(AR) $(AROPT) $@ $^

mys_psqlscan.c: FLEXFLAGS = -Cfe -p -p
mys_psqlscan.c: FLEX_NO_BACKUP=yes
mys_psqlscan.c: FLEX_FIX_WARNING=yes

# libmysfeutils could be useful to contrib, so install it
install: all installdirs
	$(INSTALL_STLIB) libmysfeutils.a '$(DESTDIR)$(libdir)/libmysfeutils.a'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(libdir)'

uninstall:
	rm -f '$(DESTDIR)$(libdir)/libmysfeutils.a'

clean distclean:
	rm -f libmysfeutils.a $(OBJS) lex.backup mys_psqlscan.c

maintainer-clean: distclean
	rm -f mys_psqlscan.c mys_string_utils.o
