#-------------------------------------------------------------------------
#
# Makefile for parser
#
# 
# Copyright:
# Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
#
# IDENTIFICATION
#    src/backend/mysql_parser/Makefile
#
# add the file for requirement "SRS-MISC-MYSQL-FRAME"
#
#-------------------------------------------------------------------------

subdir = src/backend/parser
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

PGFILEDESC = "libmysql_parser - raw parser for mysql"
NAME = libmysql_parser

override CPPFLAGS := -I. -I$(srcdir) $(CPPFLAGS)

OBJS = mys_keywords.o mys_gram.o mys_scan.o libmysql_parser.o $(WIN32RES)

# where to find mys_gen_keywordlist.pl and subsidiary files
TOOLSDIR = $(top_srcdir)/src/tools
GEN_KEYWORDLIST = $(PERL) -I $(TOOLSDIR) $(TOOLSDIR)/mys_gen_keywordlist.pl
GEN_KEYWORDLIST_DEPS = $(TOOLSDIR)/mys_gen_keywordlist.pl $(TOOLSDIR)/PerfectHash.pm

all: all-shared-lib

include $(top_srcdir)/src/Makefile.shlib

install: all installdirs install-lib

installdirs: installdirs-lib

uninstall: uninstall-lib

# There is no correct way to write a rule that generates two files.
# Rules with two targets don't have that meaning, they are merely
# shorthand for two otherwise separate rules.  If we have an action
# that in fact generates two or more files, we must choose one of them
# as primary and show it as the action's output, then make all of the
# other output files dependent on the primary, like this.  Furthermore,
# the "touch" action is essential, because it ensures that gram.h is
# marked as newer than (or at least no older than) gram.c.  Without that,
# make is likely to try to rebuild gram.h in subsequent runs, which causes
# failures in VPATH builds from tarballs.

mys_gram.h: mys_gram.c
	touch $@

mys_gram.c: BISONFLAGS += -d
mys_gram.c: BISON_CHECK_CMD = $(PERL) $(srcdir)/../parser/check_keywords.pl $< $(srcdir)/../../include/mysql_parser/mys_kwlist.h


mys_scan.c: FLEXFLAGS = -CF -p -p
mys_scan.c: FLEX_NO_BACKUP=yes
mys_scan.c: FLEX_FIX_WARNING=yes


# Force these dependencies to be known even without dependency info built:
mys_gram.o mys_scan.o mys_keywords.o libmysql_parser.o: mys_gram.h

# generate SQL keyword lookup table to be included into ora_keywords*.o.
mys_kwlist_d.h: $(top_srcdir)/src/include/mysql_parser/mys_kwlist.h $(GEN_KEYWORDLIST_DEPS)
	$(GEN_KEYWORDLIST) --extern $<

# Dependencies of mys_keywords*.o need to be managed explicitly to make sure
# that you don't get broken parsing code, even in a non-enable-depend build.
mys_keywords.o : mys_kwlist_d.h


distprep: mys_scan.c mys_gram.c mys_kwlist_d.h

# mys_gram.c, mys_gram.h, and mys_scan.c are in the distribution tarball, so they
# are not cleaned here.
clean distclean: clean-lib
	rm -f lex.backup $(OBJS)

maintainer-clean: clean
	rm -f mys_scan.c mys_gram.c mys_kwlist_d.h
