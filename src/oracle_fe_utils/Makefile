#-------------------------------------------------------------------------
#
# Makefile for src/oracle_fe_utils
#
# This makefile generates a static library, liborafeutils.a,
# for use by client applications
#
# Portions Copyright (c) 2019, KingbaseES
#
# IDENTIFICATION
#    src/oracle_fe_utils/Makefile
#
# add the file for requirement "SQL PARSER"
#
#-------------------------------------------------------------------------

subdir = src/oracle_fe_utils
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -DFRONTEND -I$(libpq_srcdir) $(CPPFLAGS)

OBJS = ora_string_utils.o \
	ora_psqlscan.o \
	ora_number_utils.o
# ReqID:SRS-PSQL-PARSER

all: liborafeutils.a

liborafeutils.a: $(OBJS)
	rm -f $@
	$(AR) $(AROPT) $@ $^

# Begin - ReqID:SRS-PSQL-PARSER
ora_psqlscan.c: FLEXFLAGS = -Cfe -p -p
ora_psqlscan.c: FLEX_NO_BACKUP=yes
ora_psqlscan.c: FLEX_FIX_WARNING=yes
# End - ReqID:SRS-PSQL-PARSER

# liborafeutils could be useful to contrib, so install it
install: all installdirs
	$(INSTALL_STLIB) liborafeutils.a '$(DESTDIR)$(libdir)/liborafeutils.a'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(libdir)'

uninstall:
	rm -f '$(DESTDIR)$(libdir)/liborafeutils.a'

clean distclean:
	rm -f liborafeutils.a $(OBJS) lex.backup ora_psqlscan.c

maintainer-clean: distclean
	rm -f ora_psqlscan.c ora_string_utils.o	# ReqID:SRS-PSQL-PARSER
	rm -f ora_number_utils.o	# ReqID:SRS-SQL-SYSFUNC
