#-------------------------------------------------------------------------
#
# Makefile for the PL/iSQL procedural language
#
# src/pl/plisql/src/Makefile
#
# add the file for requirement "SQL PARSER"
#
#-------------------------------------------------------------------------

subdir = src/pl/plisql/src
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

PGFILEDESC = "PL/iSQL - procedural language"

# Shared library parameters
NAME= plisql

# Use header files from parser/oracle firstly
override CPPFLAGS := -I. -I$(srcdir) -I$(top_srcdir)/src/include/oracle_parser $(CPPFLAGS)
SHLIB_LINK = $(filter -lintl, $(LIBS))
rpath =

OBJS = \
	$(WIN32RES) \
	pl_comp.o \
	pl_exec.o \
	pl_funcs.o \
	pl_label.o \
	pl_gram.o \
	pl_handler.o \
	pl_scanner.o \
	pl_subproc_function.o \
	pl_package.o \
	pl_udt_type.o \
	pl_ora_record.o \
	pl_udt_exception.o \
	pl_ora_collection.o

DATA = plisql.control plisql--1.0.sql

ORACLE_REGRESS_OPTS = --dbname=$(PL_TESTDB)
# Begin - BUGM0000301
ORA_REGRESS = plisql_array plisql_call plisql_control plisql_copy plisql_domain \
	plisql_record plisql_cache plisql_simple plisql_transaction \
	plisql_trap plisql_trigger plisql_varprops plisql_nested_subproc \
	plisql_nested_subproc2 plisql_out_parameter plisql_type_rowtype \
	plisql_ora_cursor plisql_ora_cursor2 plisql_ora_record plisql_ora_exception \
	plisql_autonomous_transaction plisql_goto
# End - BUGM0000301

# where to find ora_gen_keywordlist.pl and subsidiary files
TOOLSDIR = $(top_srcdir)/src/tools
GEN_KEYWORDLIST = $(PERL) -I $(TOOLSDIR) $(TOOLSDIR)/ora_gen_keywordlist.pl
GEN_KEYWORDLIST_DEPS = $(TOOLSDIR)/ora_gen_keywordlist.pl $(TOOLSDIR)/PerfectHash.pm

# Test input and expected files.  These are created by pg_regress itself, so we
# don't have a rule to create them.  We do need rules to clean them however.
input_files = $(patsubst $(srcdir)/input/%.source,sql/%.sql, $(wildcard $(srcdir)/input/*.source))
output_files := $(patsubst $(srcdir)/output/%.source,expected/%.out, $(wildcard $(srcdir)/output/*.source))

all: all-lib

# Shared library stuff
include $(top_srcdir)/src/Makefile.shlib


install: all install-lib install-data install-headers

installdirs: installdirs-lib
	$(MKDIR_P) '$(DESTDIR)$(datadir)/extension'
	$(MKDIR_P) '$(DESTDIR)$(includedir_server)'

uninstall: uninstall-lib uninstall-data uninstall-headers

install-data: installdirs
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(DATA)) '$(DESTDIR)$(datadir)/extension/'

# The plisql.h header file is needed by instrumentation plugins
install-headers: installdirs
	$(INSTALL_DATA) '$(srcdir)/plisql.h' '$(DESTDIR)$(includedir_server)'
	$(INSTALL_DATA) '$(srcdir)/pl_subproc_function.h' '$(DESTDIR)$(includedir_server)'
	$(INSTALL_DATA) '$(srcdir)/pl_package.h' '$(DESTDIR)$(includedir_server)'
	$(INSTALL_DATA) '$(srcdir)/pl_udt_type.h' '$(DESTDIR)$(includedir_server)'
	$(INSTALL_DATA) '$(srcdir)/pl_udt_exception.h' '$(DESTDIR)$(includedir_server)'

uninstall-data:
	rm -f $(addprefix '$(DESTDIR)$(datadir)/extension'/, $(notdir $(DATA)))

uninstall-headers:
	rm -f '$(DESTDIR)$(includedir_server)/plisql.h'
	rm -f '$(DESTDIR)$(includedir_server)/pl_subproc_function.h'
	rm -f '$(DESTDIR)$(includedir_server)/pl_package.h'
	rm -f '$(DESTDIR)$(includedir_server)/pl_udt_type.h'
	rm -f '$(DESTDIR)$(includedir_server)/pl_udt_exception.h'

.PHONY: install-data install-headers uninstall-data uninstall-headers


# Force these dependencies to be known even without dependency info built:
pl_gram.o pl_handler.o pl_comp.o pl_exec.o pl_funcs.o pl_scanner.o: plisql.h pl_gram.h plerrcodes.h pl_subproc_function.h pl_package.h pl_udt_type.h pl_udt_exception.h
pl_scanner.o: pl_reserved_kwlist_d.h pl_unreserved_kwlist_d.h

# See notes in src/backend/parser/Makefile about the following two rules
pl_gram.h: pl_gram.c
	touch $@

pl_gram.c: BISONFLAGS += -d

# generate plerrcodes.h from src/backend/utils/ora_errcodes.txt
plerrcodes.h: $(top_srcdir)/src/backend/utils/ora_errcodes.txt generate-plerrcodes.pl
	$(PERL) $(srcdir)/generate-plerrcodes.pl $< > $@

# generate keyword headers for the scanner
pl_reserved_kwlist_d.h: pl_reserved_kwlist.h $(GEN_KEYWORDLIST_DEPS)
	$(GEN_KEYWORDLIST) --varname ReservedPLKeywords $<

pl_unreserved_kwlist_d.h: pl_unreserved_kwlist.h $(GEN_KEYWORDLIST_DEPS)
	$(GEN_KEYWORDLIST) --varname UnreservedPLKeywords $<

# Begin - BUGM0000301
oracle-check: submake
	$(oracle_regress_check) $(ORACLE_REGRESS_OPTS) $(ORA_REGRESS)

oracle-installcheck: submake
	$(oracle_regress_installcheck) $(ORACLE_REGRESS_OPTS) $(ORA_REGRESS)
# End - BUGM0000301
.PHONY: submake
submake:
	$(MAKE) -C $(top_builddir)/src/oracle_test/regress pg_regress$(X)


distprep: pl_gram.h pl_gram.c plerrcodes.h pl_reserved_kwlist_d.h pl_unreserved_kwlist_d.h

# pl_gram.c, pl_gram.h, plerrcodes.h, pl_reserved_kwlist_d.h, and
# pl_unreserved_kwlist_d.h are in the distribution tarball, so they
# are not cleaned here.
clean distclean: clean-lib
	rm -f $(OBJS)
	rm -f $(output_files) $(input_files)
	rm -rf $(oracle_regress_clean_files)

maintainer-clean: distclean
	rm -f pl_gram.c pl_gram.h plerrcodes.h pl_reserved_kwlist_d.h pl_unreserved_kwlist_d.h
