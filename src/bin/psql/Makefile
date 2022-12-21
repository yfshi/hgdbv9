#-------------------------------------------------------------------------
#
# Makefile for src/bin/psql
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/bin/psql/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "psql - the PostgreSQL interactive terminal"
PGAPPICON=win32

subdir = src/bin/psql
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# make this available to TAP test scripts
export with_readline

REFDOCDIR= $(top_srcdir)/doc/src/sgml/ref

override CPPFLAGS := -I. -I$(srcdir) -I$(libpq_srcdir) $(CPPFLAGS)
LDFLAGS_INTERNAL += -L$(top_builddir)/src/oracle_fe_utils -lorafeutils -L$(top_builddir)/src/fe_utils -lpgfeutils $(libpq_pgport) # ReqID:SRS-PSQL-PARSER

OBJS = \
	$(WIN32RES) \
	command.o \
	common.o \
	copy.o \
	crosstabview.o \
	describe.o \
	help.o \
	input.o \
	large_obj.o \
	mainloop.o \
	ora_psqlscanslash.o \
	prompt.o \
	psqlplusparse.o \
	ora_prompt.o\
	psqlscanslash.o \
	sql_help.o \
	startup.o \
	stringutils.o \
	tab-complete.o \
	variables.o
# ReqID:SRS-PSQL-PARSER
# ReqID:SRS-SQL-ESCAPECHAR

all: psql

psql: $(OBJS) | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

help.o: sql_help.h

# ReqID:SRS-CMD-PSQL
psqlplusparse.o: psqlplusscan.c

# See notes in src/backend/parser/Makefile about the following two rules
sql_help.c: sql_help.h
	touch $@

sql_help.h: create_help.pl $(wildcard $(REFDOCDIR)/*.sgml)
	$(PERL) $< $(REFDOCDIR) $*

psqlscanslash.c: FLEXFLAGS = -Cfe -p -p
psqlscanslash.c: FLEX_NO_BACKUP=yes
psqlscanslash.c: FLEX_FIX_WARNING=yes

/* Begin - ReqID:SRS-PSQL-PARSER */
ora_psqlscanslash.c: FLEXFLAGS = -Cfe -p -p
ora_psqlscanslash.c: FLEX_NO_BACKUP=yes
ora_psqlscanslash.c: FLEX_FIX_WARNING=yes
/* End - ReqID:SRS-PSQL-PARSER */

# ReqID:SRS-CMD-PSQL
distprep: ora_psqlscanslash.c sql_help.h sql_help.c psqlscanslash.c psqlplusscan.c psqlplusparse.c

install: all installdirs
	$(INSTALL_PROGRAM) psql$(X) '$(DESTDIR)$(bindir)/psql$(X)'
	$(INSTALL_DATA) $(srcdir)/psqlrc.sample '$(DESTDIR)$(datadir)/psqlrc.sample'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)' '$(DESTDIR)$(datadir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/psql$(X)' '$(DESTDIR)$(datadir)/psqlrc.sample'

clean distclean:
	rm -f psql$(X) $(OBJS) lex.backup ora_psqlscanslash.c
	rm -rf tmp_check

# files removed here are supposed to be in the distribution tarball,
# so do not clean them in the clean/distclean rules
# ReqID:SRS-CMD-PSQL
maintainer-clean: distclean
	rm -f ora_psqlscanslash.c sql_help.h sql_help.c psqlscanslash.c #ReqID:SRS-PSQL-PARSER
	rm -f psqlplusscan.c psqlplusparse.c

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)
