#-------------------------------------------------------------------------
#
# Makefile for src/fe_utils
#
# This makefile generates a static library, libpgfeutils.a,
# for use by client applications
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# IDENTIFICATION
#    src/fe_utils/Makefile
#
#-------------------------------------------------------------------------

subdir = src/fe_utils
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

# override CPPFLAGS := -DFRONTEND -I$(libpq_srcdir) $(CPPFLAGS)
override CPPFLAGS := -DFRONTEND -I$(libpq_srcdir) -I$(top_builddir)/src/include/oracle_fe_utils -I$(top_builddir)/src/include/mysql_fe_utils $(CPPFLAGS)	# ReqID:SRS-MISC-MYSQL-FRAME

OBJS = \
	archive.o \
	cancel.o \
	conditional.o \
	connect_utils.o \
	mbprint.o \
	option_utils.o \
	parallel_slot.o \
	print.o \
	psqlscan.o \
	query_utils.o \
	recovery_gen.o \
	simple_list.o \
	string_utils.o \
	ora_keywords.o \
	ora_string_utils.o\
	mys_keywords.o \
	mys_string_utils.o
# ReqID:SRS-MISC-MYSQL-FRAME

# where to find ora_gen_keywordlist.pl and subsidiary files
TOOLSDIR = $(top_srcdir)/src/tools
GEN_KEYWORDLIST = $(PERL) -I $(TOOLSDIR) $(TOOLSDIR)/ora_gen_keywordlist.pl
GEN_KEYWORDLIST_DEPS = $(TOOLSDIR)/ora_gen_keywordlist.pl $(TOOLSDIR)/PerfectHash.pm
# Begin - ReqID:SRS-MISC-MYSQL-FRAME
GEN_KEYWORDLIST1 = $(PERL) -I $(TOOLSDIR) $(TOOLSDIR)/mys_gen_keywordlist.pl
GEN_KEYWORDLIST_DEPS1 = $(TOOLSDIR)/mys_gen_keywordlist.pl $(TOOLSDIR)/PerfectHash.pm
#End - ReqID:SRS-MISC-MYSQL-FRAME

all: libpgfeutils.a

libpgfeutils.a: $(OBJS)
	rm -f $@
	$(AR) $(AROPT) $@ $^

psqlscan.c: FLEXFLAGS = -Cfe -p -p
psqlscan.c: FLEX_NO_BACKUP=yes
psqlscan.c: FLEX_FIX_WARNING=yes

# generate SQL keyword lookup table to be included into ora_keywords*.o.
ora_kwlist_d.h: $(top_srcdir)/src/include/oracle_parser/ora_kwlist.h $(GEN_KEYWORDLIST_DEPS)
	$(GEN_KEYWORDLIST) --extern $<

# Dependencies of ora_keywords*.o need to be managed explicitly to make sure
# that you don't get broken parsing code, even in a non-enable-depend build.
ora_keywords.o: ora_kwlist_d.h

distprep: psqlscan.c ora_keywords.c ora_string_utils.c

ora_keywords.c: % : $(top_srcdir)/src/backend/oracle_parser/%
	rm -f $@ && $(LN_S) $< .

ora_string_utils.c: % : $(top_srcdir)/src/oracle_fe_utils/%
	rm -f $@ && $(LN_S) $< .

# Begin - ReqID:SRS-MISC-MYSQL-FRAME
# generate SQL keyword lookup table to be included into mys_keywords*.o.
mys_kwlist_d.h: $(top_srcdir)/src/include/mysql_parser/mys_kwlist.h $(GEN_KEYWORDLIST_DEPS1)
	$(GEN_KEYWORDLIST1) --extern $<

# Dependencies of mys_keywords*.o need to be managed explicitly to make sure
# that you don't get broken parsing code, even in a non-enable-depend build.
mys_keywords.o: mys_kwlist_d.h

distprep: psqlscan.c mysql_keywords.c mys_string_utils.c

mys_keywords.c: % : $(top_srcdir)/src/backend/mysql_parser/%
	rm -f $@ && $(LN_S) $< .

mys_string_utils.c: % : $(top_srcdir)/src/mysql_fe_utils/%
	rm -f $@ && $(LN_S) $< .
#End - ReqID:SRS-MISC-MYSQL-FRAME

# libpgfeutils could be useful to contrib, so install it
install: all installdirs
	$(INSTALL_STLIB) libpgfeutils.a '$(DESTDIR)$(libdir)/libpgfeutils.a'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(libdir)'

uninstall:
	rm -f '$(DESTDIR)$(libdir)/libpgfeutils.a'

clean distclean:
	rm -f libpgfeutils.a $(OBJS) lex.backup

# psqlscan.c is supposed to be in the distribution tarball,
# so do not clean it in the clean/distclean rules
maintainer-clean: distclean
	rm -f psqlscan.c ora_keywords.c ora_string_utils.c ora_kwlist_d.h mys_keywords.c mys_string_utils.c mys_kwlist_d.h	# ReqID:SRS-MISC-MYSQL-FRAME

