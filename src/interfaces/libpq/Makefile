#-------------------------------------------------------------------------
#
# Makefile for src/interfaces/libpq library
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/interfaces/libpq/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global


PGFILEDESC = "PostgreSQL Access Library"

# shared library parameters
NAME= pq
SO_MAJOR_VERSION= 5
SO_MINOR_VERSION= $(MAJORVERSION)

override CPPFLAGS := -DFRONTEND -DUNSAFE_STAT_OK -I$(srcdir) -I$(top_builddir)/src/port -I$(top_srcdir)/src/port -I$(srcdir)/frontend_parser -I$(srcdir)/tecc_parser -I$(srcdir)/tecc_processor -I$(srcdir)/tecc_cache -I$(srcdir)/tecc_rewriter -I$(srcdir)/tecc_data_fetcher/include -I$(srcdir)/tecc_hsm_adapter -I$(srcdir)/tecc_common -I$(srcdir)/tecc_formatter -I$(srcdir)/tecc_scheduler -I$(top_srcdir)/src/include/hsm $(CPPFLAGS)

# Auto-dependency tracking: -MMD -MP generates .d files alongside .o files.
# PG's implicit %.o : %.c rule handles all compilation; no explicit rules needed
# for subdirectories since all -I paths are covered above.
override CFLAGS += -MMD -MP

ifneq ($(PORTNAME), win32)
override CFLAGS += $(PTHREAD_CFLAGS)
endif

# The MSVC build system scrapes OBJS from this file.  If you change any of
# the conditional additions of files to OBJS, update Mkvcbuild.pm to match.

OBJS = \
	$(WIN32RES) \
	fe-auth-scram.o \
	fe-connect.o \
	fe-exec.o \
	fe-lobj.o \
	fe-misc.o \
	fe-print.o \
	fe-protocol3.o \
	fe-secure.o \
	fe-trace.o \
	legacy-pqsignal.o \
	libpq-events.o \
	pqexpbuffer.o \
	fe-auth.o \
	ivy_sema.o \
	ivy-exec.o \
	mfa_common.o \
	tecc_hsm_adapter/fe-hsm-adapter.o \
	tecc_hsm_adapter/fe-swxa-adapter.o \
	tecc_common/tecc_frontend_list.o \
	tecc_common/tecc_copy_state.o \
	tecc_common/tecc_logging.o \
	tecc_processor/tecc_processor.o \
	tecc_processor/stmt_processor.o \
	tecc_processor/values_processor.o \
	tecc_processor/where_clause_processor.o \
	tecc_processor/create_stmt_processor.o \
	tecc_processor/create_kek_processor.o \
	tecc_processor/raw_value.o \
	tecc_processor/raw_values_list.o \
	tecc_processor/internal/tecc_processor_internal.o \
	tecc_processor/column_ref_data.o \
	tecc_processor/expr_processor.o \
	tecc_processor/tecc_expression_walker.o \
	tecc_processor/pre_stmt_processor.o \
	tecc_processor/post_stmt_processor.o \
	tecc_processor/processor_utils.o \
	tecc_processor/func_processor.o \
	tecc_processor/func_hardcoded_values.o \
	tecc_processor/prepared_statement.o \
	tecc_processor/key_management.o \
	tecc_processor/param_expression_processor.o \
	tecc_processor/alter_stmt_processor.o \
	tecc_scheduler/tecc_crypto_scheduler.o \
	tecc_scheduler/tecc_crypto_context.o \
	tecc_scheduler/tecc_algorithm_scheduler.o \
	tecc_scheduler/tecc_key_manager_adapter.o \
	tecc_formatter/tecc_formatter.o \
	tecc_formatter/tecc_normalizer.o \
	tecc_formatter/tecc_text_normalizer.o \
	tecc_formatter/tecc_data_validator.o \
	tecc_formatter/tecc_type_converter.o \
	tecc_formatter/tecc_hex_encoder.o \
	tecc_formatter/tecc_buffer_manager.o \
	tecc_formatter/tecc_formatter_utils.o \
	tecc_formatter/tecc_icu_normalizer.o \
	tecc_formatter/tecc_type_output.o \
	tecc_formatter/tecc_normalization_common.o \
	tecc_data_fetcher/tecc_data_fetcher.o \
	tecc_data_fetcher/tecc_libpq_cursor.o \
	tecc_data_fetcher/tecc_metadata_query.o \
	tecc_data_fetcher/tecc_permission_check.o \
	tecc_data_fetcher/tecc_bitmask_parser.o \
	tecc_data_fetcher/tecc_key_args_parser.o \
	tecc_data_fetcher/tecc_error_handler.o \
	tecc_data_fetcher/tecc_metadata_serializer.o \
	tecc_data_fetcher/tecc_bytea_decoder.o \
	tecc_cache/tecc_cache_manager.o \
	tecc_cache/tecc_column_cache.o \
	tecc_cache/tecc_data_fetcher_adapter.o \
	tecc_cache/tecc_dek_cache.o \
	tecc_cache/tecc_kek_cache.o \
	tecc_cache/fe_hash_table.o \
	tecc_libpq_integration/fe-tecc-integration.o \
	tecc_libpq_integration/fe-tecc-copy.o \
	frontend_parser/fe_adapter.o \
	frontend_parser/tecc_parser.o \
	frontend_parser/tecc_perf.o \
	frontend_parser/tecc_error.o \
	frontend_parser/tecc_nodes.o \
	frontend_parser/tecc_grammar.o \
	frontend_parser/lex.tecc_yy.o \
	frontend_parser/tecc_location.o \
	frontend_parser/tecc_semantic.o \
	frontend_parser/parser.o \
	frontend_parser/feparser_memutils.o

# File shared across all SSL implementations supported.
ifneq ($(with_ssl),no)
OBJS += \
	fe-secure-common.o
endif

ifeq ($(with_ssl),openssl)
OBJS += \
	fe-secure-openssl.o
endif

#Begin - ReqID:SRS-SECURITY-TLCP
ifeq ($(with_gmssl),yes)
OBJS += \
        fe-secure-tlcp.o
endif
#End - ReqID:SRS-SECURITY-TLCP

ifeq ($(with_gssapi),yes)
OBJS += \
	fe-gssapi-common.o \
	fe-secure-gssapi.o
endif

ifeq ($(PORTNAME), cygwin)
override shlib = cyg$(NAME)$(DLSUFFIX)
endif

ifeq ($(PORTNAME), win32)
OBJS += \
	win32.o

ifeq ($(enable_thread_safety), yes)
OBJS += pthread-win32.o
endif
endif


# Add libraries that libpq depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)  Note that we filter out -lpgcommon and -lpgport from LIBS and
# instead link with -lpgcommon_shlib and -lpgport_shlib, to get object files
# that are built correctly for use in a shlib.
SHLIB_LINK_INTERNAL = -lpgcommon_shlib -lpgport_shlib
ifneq ($(PORTNAME), win32)
ifeq ($(with_gmssl), yes)
SHLIB_LINK += $(filter -lcrypt -ldes -lcom_err -lcrypto -lgmssl -lk5crypto -lkrb5 -lgssapi_krb5 -lgss -lgssapi -lssl -lsocket -lnsl -lresolv -lintl -lm, $(LIBS)) $(LDAP_LIBS_FE) $(PTHREAD_LIBS) -lstdc++ $(ICU_LIBS) -L/usr/lib64 -lsdf.client -ldl
else
SHLIB_LINK += $(filter -lcrypt -ldes -lcom_err -lcrypto -lk5crypto -lkrb5 -lgssapi_krb5 -lgss -lgssapi -lssl -lsocket -lnsl -lresolv -lintl -lm, $(LIBS)) $(LDAP_LIBS_FE) $(PTHREAD_LIBS) -lstdc++ $(ICU_LIBS) -L/usr/lib64 -lsdf.client -ldl
endif
SHLIB_LINK += $(filter -lcrypt -ldes -lcom_err -lcrypto -lk5crypto -lkrb5 -lgssapi32 -lssl -lsocket -lnsl -lresolv -lintl -lm $(PTHREAD_LIBS), $(LIBS)) $(LDAP_LIBS_FE)
endif

# HSM library paths (only for libpq, not globally, to avoid interfering with ICU link order)
ifneq ($(tzwy_sdf_libdir),)
SHLIB_LINK += -L'$(tzwy_sdf_libdir)'
endif
ifneq ($(swxa_sdf_libdir),)
SHLIB_LINK += -L'$(swxa_sdf_libdir)'
endif

ifeq ($(PORTNAME), win32)
SHLIB_LINK += -lshell32 -lws2_32 -lsecur32 $(filter -leay32 -lssleay32 -lcomerr32 -lkrb5_32, $(LIBS))
endif
SHLIB_PREREQS = submake-libpgport

SHLIB_EXPORTS = exports.txt

ifeq ($(with_ssl),openssl)
PKG_CONFIG_REQUIRES_PRIVATE = libssl libcrypto
endif

all: all-lib

# Shared library stuff
include $(top_srcdir)/src/Makefile.shlib

# Make libpq.so depend on libpgcommon_shlib.a and libpgport_shlib.a as real
# file dependencies (not just order-only), so that changes to tecc_key or
# other common/port files trigger re-linking of libpq.so.
$(shlib): $(top_builddir)/src/common/libpgcommon_shlib.a $(top_builddir)/src/port/libpgport_shlib.a

backend_src = $(top_srcdir)/src/backend


# Make dependencies on pg_config_paths.h visible in all builds.
fe-connect.o: fe-connect.c $(top_builddir)/src/port/pg_config_paths.h
fe-misc.o: fe-misc.c $(top_builddir)/src/port/pg_config_paths.h

$(top_builddir)/src/port/pg_config_paths.h:
	$(MAKE) -C $(top_builddir)/src/port pg_config_paths.h

install: all installdirs install-lib
	$(INSTALL_DATA) $(srcdir)/libpq-fe.h '$(DESTDIR)$(includedir)'
	$(INSTALL_DATA) $(srcdir)/libpq-ivy.h '$(DESTDIR)$(includedir)'
	$(INSTALL_DATA) $(srcdir)/ivy_sema.h  '$(DESTDIR)$(includedir)'
	$(INSTALL_DATA) $(srcdir)/libpq-events.h '$(DESTDIR)$(includedir)'
	$(INSTALL_DATA) $(srcdir)/libpq-int.h '$(DESTDIR)$(includedir_internal)'
	$(INSTALL_DATA) $(srcdir)/pqexpbuffer.h '$(DESTDIR)$(includedir_internal)'
	$(INSTALL_DATA) $(srcdir)/pg_service.conf.sample '$(DESTDIR)$(datadir)/pg_service.conf.sample'

installcheck:
	$(MAKE) -C test $@

#BEGIN - SQL PARSER
oracle-installcheck:
	$(MAKE) -C test $@
#END - SQL PARSER

installdirs: installdirs-lib
	$(MKDIR_P) '$(DESTDIR)$(includedir)' '$(DESTDIR)$(includedir_internal)' '$(DESTDIR)$(datadir)'

uninstall: uninstall-lib
	rm -f '$(DESTDIR)$(includedir)/libpq-fe.h'
	rm -f '$(DESTDIR)$(includedir)/libpq-ivy.h'
	rm -f '$(DESTDIR)$(includedir)/ivy_sema.h'
	rm -f '$(DESTDIR)$(includedir)/libpq-events.h'
	rm -f '$(DESTDIR)$(includedir_internal)/libpq-int.h'
	rm -f '$(DESTDIR)$(includedir_internal)/pqexpbuffer.h'
	rm -f '$(DESTDIR)$(datadir)/pg_service.conf.sample'

# TECC grammar: auto-regenerate when .y changes
frontend_parser/tecc_grammar.c frontend_parser/tecc_grammar.h: frontend_parser/tecc_grammar.y
	cd frontend_parser && bison -d -o tecc_grammar.c tecc_grammar.y
frontend_parser/lex.tecc_yy.c: frontend_parser/tecc_scanner.h frontend_parser/tecc_scanner.l frontend_parser/tecc_grammar.h
	cd frontend_parser && flex -o lex.tecc_yy.c tecc_scanner.l

# Build-order deps for bison/flex-generated headers.  On first build, .d files
# don't exist yet and -MMD -MP can't express these cross-file dependencies.
frontend_parser/tecc_parser.o: frontend_parser/tecc_grammar.h
frontend_parser/tecc_grammar.o: frontend_parser/tecc_grammar.h
frontend_parser/lex.tecc_yy.o: frontend_parser/tecc_grammar.h
frontend_parser/fe_adapter.o: frontend_parser/fe_adapter.h

clean distclean maintainer-clean: clean-lib clean-deps
	$(MAKE) -C test $@
	$(MAKE) -C ivytest $@
	rm -f $(OBJS) pthread.h
# Might be left over from a Win32 client-only build
	rm -f pg_config_paths.h
	rm -f frontend_parser/tecc_grammar.c
	rm -f frontend_parser/tecc_grammar.h 
	rm -f frontend_parser/lex.tecc_yy.c
	rm -f tecc_formatter/.depend

# Auto-dependency tracking: -MMD -MP generates .d files alongside .o files.
# PG's implicit %.o : %.c rule handles compilation; .d files are included here
# so header changes trigger rebuild of dependent .c files.
d_files := $(wildcard *.d */*.d */*/*.d */*/*/*.d)
ifneq (,$(d_files))
include $(d_files)
endif

.PHONY: clean-deps
clean-deps:
	@rm -f *.d */*.d */*/*.d */*/*/*.d



