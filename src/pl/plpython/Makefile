# src/pl/plpython/Makefile

subdir = src/pl/plpython
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global


# On Windows we have to remove -lpython from the link since we are
# building our own
ifeq ($(PORTNAME), win32)
override python_libspec =
endif

override CPPFLAGS := -I. -I$(srcdir) $(CPPFLAGS) $(python_includespec)

rpathdir = $(python_libdir)

PGFILEDESC = "PL/Python - procedural language"

NAME = plpython$(python_majorversion)

OBJS = \
	$(WIN32RES) \
	plpy_cursorobject.o \
	plpy_elog.o \
	plpy_exec.o \
	plpy_main.o \
	plpy_planobject.o \
	plpy_plpymodule.o \
	plpy_procedure.o \
	plpy_resultobject.o \
	plpy_spi.o \
	plpy_subxactobject.o \
	plpy_typeio.o \
	plpy_util.o

DATA = $(NAME)u.control $(NAME)u--1.0.sql
ifeq ($(python_majorversion),2)
DATA += plpythonu.control plpythonu--1.0.sql
endif

# header files to install - it's not clear which of these might be needed
# so install them all.
INCS = 	plpython.h \
	plpython_system.h \
	plpy_cursorobject.h \
	plpy_elog.h \
	plpy_exec.h \
	plpy_main.h \
	plpy_planobject.h \
	plpy_plpymodule.h \
	plpy_procedure.h \
	plpy_resultobject.h \
	plpy_spi.h \
	plpy_subxactobject.h \
	plpy_typeio.h \
	plpy_util.h

# Python on win32 ships with import libraries only for Microsoft Visual C++,
# which are not compatible with mingw gcc. Therefore we need to build a
# new import library to link with.
ifeq ($(PORTNAME), win32)

pytverstr=$(subst .,,${python_version})
PYTHONDLL=$(subst \,/,$(WINDIR))/system32/python${pytverstr}.dll

OBJS += libpython${pytverstr}.a

libpython${pytverstr}.a: python${pytverstr}.def
	dlltool --dllname python${pytverstr}.dll --def python${pytverstr}.def --output-lib libpython${pytverstr}.a

python${pytverstr}.def:
	gendef - $(PYTHONDLL) > $@

endif # win32


SHLIB_LINK = $(python_libspec) $(python_additional_libs) $(filter -lintl,$(LIBS))

REGRESS_OPTS = --dbname=$(PL_TESTDB)
# Only load plpythonu with Python 2.  The test files themselves load
# the versioned language plpython(2|3)u.
ifeq ($(python_majorversion),2)
REGRESS_OPTS += --load-extension=plpythonu
ORACLE_REGRESS_OPTS += --load-extension=plpythonu	# BUGM0000301
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYSQL_REGRESS_OPTS += --load-extension=plpythonu
#End - ReqID:SRS-MISC-MYSQL-FRAME
endif

REGRESS = \
	plpython_schema \
	plpython_populate \
	plpython_test \
	plpython_do \
	plpython_global \
	plpython_import \
	plpython_spi \
	plpython_newline \
	plpython_void \
	plpython_call \
	plpython_params \
	plpython_setof \
	plpython_record \
	plpython_trigger \
	plpython_types \
	plpython_error \
	plpython_ereport \
	plpython_unicode \
	plpython_quote \
	plpython_composite \
	plpython_subtransaction \
	plpython_transaction \
	plpython_drop

REGRESS_PLPYTHON3_MANGLE := $(REGRESS)
# Begin - BUGM0000301
ORA_REGRESS = \
	plpython_schema \
	plpython_populate \
	ivy_plpython_test \
	plpython_do \
	ivy_plpython_global \
	ivy_plpython_import \
	ivy_plpython_spi \
	ivy_plpython_newline \
	ivy_plpython_void \
	ivy_plpython_call \
	ivy_plpython_params \
	ivy_plpython_setof \
	ivy_plpython_record \
	ivy_plpython_trigger \
	ivy_plpython_types \
	ivy_plpython_error \
	ivy_plpython_ereport \
	ivy_plpython_unicode \
	ivy_plpython_quote \
	ivy_plpython_composite \
	ivy_plpython_subtransaction \
	ivy_plpython_transaction \
	plpython_drop

ORA_REGRESS_PLPYTHON3_MANGLE := $(ORA_REGRESS)
# End - BUGM0000301
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_REGRESS = \
	mys_plpython_schema \
	mys_plpython_populate \
	mys_plpython_test \
	mys_plpython_do \
	mys_plpython_global \
	mys_plpython_import \
	mys_plpython_spi \
	mys_plpython_newline \
	mys_plpython_void \
	mys_plpython_call \
	mys_plpython_params \
	mys_plpython_setof \
	mys_plpython_record \
	mys_plpython_trigger \
	mys_plpython_types \
	mys_plpython_error \
	mys_plpython_ereport \
	mys_plpython_unicode \
	mys_plpython_quote \
	mys_plpython_composite \
	mys_plpython_subtransaction \
	mys_plpython_transaction \
	mys_plpython_drop

MYS_REGRESS_PLPYTHON3_MANGLE := $(MYS_REGRESS)
#End - ReqID:SRS-MISC-MYSQL-FRAME
include $(top_srcdir)/src/Makefile.shlib

all: all-lib

# Ensure parallel safety if a build is started in this directory
$(OBJS): | submake-generated-headers

install: all install-lib install-data

installdirs: installdirs-lib
	$(MKDIR_P) '$(DESTDIR)$(datadir)/extension' '$(DESTDIR)$(includedir_server)' '$(DESTDIR)$(pgxsdir)/src/pl/plpython'

uninstall: uninstall-lib uninstall-data

install-data: installdirs
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(DATA)) '$(DESTDIR)$(datadir)/extension/'
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(INCS)) '$(DESTDIR)$(includedir_server)'
	$(INSTALL_DATA) $(srcdir)/regress-python3-mangle.mk '$(DESTDIR)$(pgxsdir)/src/pl/plpython'

uninstall-data:
	rm -f $(addprefix '$(DESTDIR)$(datadir)/extension'/, $(notdir $(DATA)))
	rm -f $(addprefix '$(DESTDIR)$(includedir_server)'/, $(INCS))

.PHONY: install-data uninstall-data


include $(srcdir)/regress-python3-mangle.mk


check: submake-pg-regress
	$(pg_regress_check) $(REGRESS_OPTS) $(REGRESS)

installcheck: submake-pg-regress
	$(pg_regress_installcheck) $(REGRESS_OPTS) $(REGRESS)

# Begin - BUGM0000301
oracle-check: submake_ora
	$(oracle_regress_check) $(ORACLE_REGRESS_OPTS) $(ORA_REGRESS)

oracle-installcheck: submake_ora
	$(oracle_regress_installcheck) $(ORACLE_REGRESS_OPTS) $(ORA_REGRESS)

#Begin - ReqID:SRS-MISC-MYSQL-FRAME
mysql-check: submake_mys
	$(mysql_regress_check) $(MYSQL_REGRESS_OPTS) $(MYS_REGRESS)

mysql-installcheck: submake_mys
	$(mysql_regress_installcheck) $(MYSQL_REGRESS_OPTS) $(MYS_REGRESS)
#End - ReqID:SRS-MISC-MYSQL-FRAME

.PHONY: submake-pg-regress	submake_ora
# End - BUGM0000301
submake-pg-regress: | submake-generated-headers
	$(MAKE) -C $(top_builddir)/src/test/regress pg_regress$(X)
# Begin - BUGM0000301
submake_ora: | submake-generated-headers
	$(MAKE) -C $(top_builddir)/src/oracle_test/regress pg_regress$(X)
# End - BUGM0000301
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
submake_mys: | submake-generated-headers
	$(MAKE) -C $(top_builddir)/src/mysql_test/regress pg_regress$(X)
#End - ReqID:SRS-MISC-MYSQL-FRAME
clean distclean: clean-lib
	rm -f $(OBJS)
	rm -rf $(pg_regress_clean_files)
ifeq ($(PORTNAME), win32)
	rm -f python${pytverstr}.def
endif


# Force this dependency to be known even without dependency info built:
plpy_plpymodule.o: spiexceptions.h

spiexceptions.h: $(top_srcdir)/src/backend/utils/errcodes.txt generate-spiexceptions.pl
	$(PERL) $(srcdir)/generate-spiexceptions.pl $< > $@

distprep: spiexceptions.h

maintainer-clean: distclean
	rm -f spiexceptions.h
