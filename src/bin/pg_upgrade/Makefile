# src/bin/pg_upgrade/Makefile

PGFILEDESC = "pg_upgrade - an in-place binary upgrade utility"
PGAPPICON = win32

subdir = src/bin/pg_upgrade
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	$(WIN32RES) \
	check.o \
	controldata.o \
	dump.o \
	exec.o \
	file.o \
	function.o \
	info.o \
	option.o \
	parallel.o \
	pg_upgrade.o \
	relfilenode.o \
	server.o \
	tablespace.o \
	util.o \
	version.o

#BEGIN - ReqID:SRS-CMD-pg2highgo
pg2highgo_OBJS = \
	$(WIN32RES) \
	check.o \
	pg2hg_controldata.o \
	dump.o \
	exec.o \
	file.o \
	function.o \
	info.o \
	option.o \
	parallel.o \
	pg2highgo.o \
	relfilenode.o \
	server.o \
	pg2hg_tablespace.o \
	util.o \
	version.o
#END - ReqID:SRS-CMD-pg2highgo

#BEGIN - ReqID:SRS-CMD-highgo2pg
highgo2pg_OBJS = \
	$(WIN32RES) \
	check.o \
	hg2pg_controldata.o \
	hg2pg_dump.o \
	hg2pg_exec.o \
	file.o \
	function.o \
	info.o \
	option.o \
	parallel.o \
	highgo2pg.o \
	relfilenode.o \
	server.o \
	hg2pg_tablespace.o \
	util.o \
	version.o
#END - ReqID:SRS-CMD-highgo2pg

override CPPFLAGS := -DDLSUFFIX=\"$(DLSUFFIX)\" -I$(srcdir) -I$(libpq_srcdir) $(CPPFLAGS)
LDFLAGS_INTERNAL += -L$(top_builddir)/src/fe_utils -lpgfeutils $(libpq_pgport)

#BEGIN - ReqID:SRS-CMD-pg2highgo
all: create_link_pg2hg create_link_hg2pg pg_upgrade pg2highgo highgo2pg
#END - ReqID:SRS-CMD-pg2highgo

pg_upgrade: $(OBJS) | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

#BEGIN - ReqID:SRS-CMD-pg2highgo
PG2HG_LINK_FILE := pg2highgo.c
CONTROL_LINK_FILE := pg2hg_controldata.c
PG2HG_SPACE_LINK_FILE := pg2hg_tablespace.c

create_link_pg2hg:
	@if [ ! -e $(PG2HG_LINK_FILE) ]; then \
		ln -s pg_upgrade.c $(PG2HG_LINK_FILE) ;\
	fi
	@if [ ! -e $(CONTROL_LINK_FILE) ]; then \
		ln -s controldata.c $(CONTROL_LINK_FILE) ;\
	fi
	@if [ ! -e $(PG2HG_SPACE_LINK_FILE) ]; then \
		ln -s tablespace.c $(PG2HG_SPACE_LINK_FILE) ;\
	fi


pg2highgo_FLAGS := -DPG2HG

pg2highgo.c: $(create_link_pg2hg)
pg2hg_controldata.c:$(create_link_pg2hg)
pg2hg_tablespace.c:$(create_link_pg2hg)

pg2highgo.o: pg2highgo.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(pg2highgo_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@

pg2hg_controldata.o: pg2hg_controldata.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(pg2highgo_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@

pg2hg_tablespace.o: pg2hg_tablespace.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(pg2highgo_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@

pg2highgo: $(pg2highgo_OBJS) | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(pg2highgo_FLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)
#END - ReqID:SRS-CMD-pg2highgo

#BEGIN - ReqID:SRS-CMD-highgo2pg
HG2PG_LINK_FILE := highgo2pg.c
DUMP_LINK_FILE := hg2pg_dump.c
EXEC_LINK_FILE := hg2pg_exec.c
HG_CONTROL_LINK_FILE := hg2pg_controldata.c
HG2PG_SPACE_LINK_FILE := hg2pg_tablespace.c

create_link_hg2pg:
	@if [ ! -e $(HG2PG_LINK_FILE) ]; then \
		ln -s pg_upgrade.c $(HG2PG_LINK_FILE) ;\
	fi
	@if [ ! -e $(DUMP_LINK_FILE) ]; then \
		ln -s dump.c $(DUMP_LINK_FILE) ;\
	fi
	@if [ ! -e $(EXEC_LINK_FILE) ]; then \
		ln -s exec.c $(EXEC_LINK_FILE) ;\
	fi
	@if [ ! -e $(HG_CONTROL_LINK_FILE) ]; then \
		ln -s controldata.c $(HG_CONTROL_LINK_FILE) ;\
	fi
	@if [ ! -e $(HG2PG_SPACE_LINK_FILE) ]; then \
		ln -s tablespace.c $(HG2PG_SPACE_LINK_FILE) ;\
	fi


highgo2pg_FLAGS := -DHG2PG

highgo2pg.c: $(create_link_hg2pg)
hg2pg_dump.c: $(create_link_hg2pg)
hg2pg_exec.c: $(create_link_hg2pg)
hg2pg_controldata.c: $(create_link_hg2pg)
hg2pg_tablespace.c: $(create_link_hg2pg)

highgo2pg.o: highgo2pg.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@
hg2pg_dump.o: hg2pg_dump.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@
hg2pg_exec.o: hg2pg_exec.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@
hg2pg_controldata.o: hg2pg_controldata.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@
hg2pg_tablespace.o: hg2pg_tablespace.c | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) $(LDFLAGS_INTERNAL) $(CPPFLAGS) -c $< -o $@

highgo2pg: $(highgo2pg_OBJS) | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(highgo2pg_FLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)
#END - ReqID:SRS-CMD-highgo2pg

install: all installdirs
	$(INSTALL_PROGRAM) pg_upgrade$(X) '$(DESTDIR)$(bindir)/pg_upgrade$(X)'
	#BEGIN - ReqID:SRS-CMD-pg2highgo
	$(INSTALL_PROGRAM) pg2highgo$(X) '$(DESTDIR)$(bindir)/pg2highgo$(X)'
	$(INSTALL_PROGRAM) highgo2pg$(X) '$(DESTDIR)$(bindir)/highgo2pg$(X)'
	#END - ReqID:SRS-CMD-pg2highgo

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_upgrade$(X)'
	rm -f '$(DESTDIR)$(bindir)/pg2highgo$(X)'
	rm -f '$(DESTDIR)$(bindir)/highgo2pg$(X)'

clean distclean maintainer-clean:
	rm -f pg_upgrade$(X) $(OBJS)
	rm -rf delete_old_cluster.sh log/ tmp_check/ \
	       loadable_libraries.txt reindex_hash.sql \
	       pg_upgrade_dump_globals.sql \
	       pg_upgrade_dump_*.custom pg_upgrade_*.log
	rm -f pg2highgo* highgo2pg* hg2pg_dump.c hg2pg_exec.c pg2hg_controldata.c hg2pg_dump.o hg2pg_exec.o pg2hg_controldata.o \
		hg2pg_controldata.o  pg2hg_controldata.c hg2pg_controldata.c hg2pg_tablespace.o hg2pg_tablespace.c pg2hg_tablespace.c pg2hg_tablespace.o

# When $(MAKE) is present, make automatically infers that this is a
# recursive make. which is not actually what we want here, as that
# e.g. prevents output synchronization from working (as make thinks
# that the subsidiary make knows how to deal with that itself, but
# we're invoking a shell script that doesn't know). Referencing
# $(MAKE) indirectly avoids that behaviour.
# See https://www.gnu.org/software/make/manual/html_node/MAKE-Variable.html#MAKE-Variable
NOTSUBMAKEMAKE=$(MAKE)

check: pro2pro_check pg2hg_check hg2pg_check

pro2pro_check: test.sh all temp-install
	MAKE=$(NOTSUBMAKEMAKE) $(with_temp_install) bindir=$(abs_top_builddir)/tmp_install/$(bindir) EXTRA_REGRESS_OPTS="$(EXTRA_REGRESS_OPTS)" $(SHELL) $<
pg2hg_check: pg2hg_test.sh all temp-install
	MAKE=$(NOTSUBMAKEMAKE) $(with_temp_install) bindir=$(abs_top_builddir)/tmp_install/$(bindir) EXTRA_REGRESS_OPTS="$(EXTRA_REGRESS_OPTS)" $(SHELL) $<
hg2pg_check: hg2pg_test.sh all temp-install
	MAKE=$(NOTSUBMAKEMAKE) $(with_temp_install) bindir=$(abs_top_builddir)/tmp_install/$(bindir) EXTRA_REGRESS_OPTS="$(EXTRA_REGRESS_OPTS)" $(SHELL) $<

#BEGIN - SQL PARSER
oracle-check: oracle_test.sh all temp-install
	MAKE=$(NOTSUBMAKEMAKE) $(with_temp_install) bindir=$(abs_top_builddir)/tmp_install/$(bindir) EXTRA_REGRESS_OPTS="$(EXTRA_REGRESS_OPTS)" $(SHELL) $<
#END - SQL PARSER
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
mysql-check: mysql_test.sh all temp-install
	MAKE=$(NOTSUBMAKEMAKE) $(with_temp_install) bindir=$(abs_top_builddir)/tmp_install/$(bindir) EXTRA_REGRESS_OPTS="$(EXTRA_REGRESS_OPTS)" $(SHELL) $<
#End - ReqID:SRS-MISC-MYSQL-FRAME

# installcheck is not supported because there's no meaningful way to test
# pg_upgrade against a single already-running server
