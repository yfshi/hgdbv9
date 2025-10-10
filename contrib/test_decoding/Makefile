# contrib/test_decoding/Makefile

MODULES = test_decoding
PGFILEDESC = "test_decoding - example of a logical decoding output plugin"

REGRESS = ddl xact rewrite toast permissions decoding_in_xact \
	decoding_into_rel binary prepared replorigin time messages \
	spill slot truncate stream stats twophase twophase_stream


ORA_REGRESS = ivy_ddl xact ivy_rewrite ivy_toast permissions decoding_in_xact \
       ivy_decoding_into_rel binary prepared replorigin ivy_time messages \
       spill ivy_slot truncate stream ivy_stats twophase twophase_stream

#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_REGRESS = mys_ddl mys_xact mys_rewrite mys_toast mys_permissions mys_decoding_in_xact \
	mys_decoding_into_rel mys_binary mys_prepared mys_replorigin mys_time mys_messages \
	mys_spill mys_slot mys_truncate mys_stream mys_stats mys_twophase mys_twophase_stream
#End - ReqID:SRS-MISC-MYSQL-FRAME

ISOLATION = mxact delayed_startup ondisk_startup concurrent_ddl_dml \
	oldest_xmin snapshot_transfer subxact_without_top concurrent_stream \
	twophase_snapshot catalog_change_snapshot skip_snapshot_restore \
	invalidation_distribution
ORA_ISOLATION = mxact delayed_startup ondisk_startup \
       oldest_xmin snapshot_transfer subxact_without_top concurrent_stream \
       twophase_snapshot catalog_change_snapshot skip_snapshot_restore
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_ISOLATION = mxact delayed_startup ondisk_startup mys_concurrent_ddl_dml \
	oldest_xmin snapshot_transfer subxact_without_top concurrent_stream \
	twophase_snapshot catalog_change_snapshot skip_snapshot_restore
#End - ReqID:SRS-MISC-MYSQL-FRAME

ISOLATION_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
ORA_ISOLATION_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_ISOLATION_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
#End - ReqID:SRS-MISC-MYSQL-FRAME

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
#BEGIN - SQL PARSER
ORACLE_REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
#END - SQL PARSER
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYSQL_REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/test_decoding/logical.conf
#End - ReqID:SRS-MISC-MYSQL-FRAME

# Disabled because these tests require "wal_level=logical", which
# typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/test_decoding
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# But it can nonetheless be very helpful to run tests on preexisting
# installation, allow to do so, but only if requested explicitly.
installcheck-force:
	$(pg_regress_installcheck) $(REGRESS)
	$(pg_isolation_regress_installcheck) $(ISOLATION)
