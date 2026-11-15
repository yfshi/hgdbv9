#-------------------------------------------------------------------------
#
# Makefile
#    Makefile for src/common
#
# These files are used by the Postgres backend, and also by frontend
# programs.  These files provide common functionality that isn't directly
# concerned with portability and thus doesn't belong in src/port.
#
# This makefile generates three outputs:
#
#	libpgcommon.a - contains object files with FRONTEND defined,
#		for use by client applications
#
#	libpgcommon_shlib.a - contains object files with FRONTEND defined,
#		built suitably for use in shared libraries; for use
#		by frontend libraries
#
#	libpgcommon_srv.a - contains object files without FRONTEND defined,
#		for use only by the backend
#
# IDENTIFICATION
#    src/common/Makefile
#
#-------------------------------------------------------------------------

subdir = src/common
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

# Ensure DEPDIR is always defined (src/Makefile.global only sets it
# inside ifeq ($(autodepend),yes), but our subdirectory rules use it).
DEPDIR ?= .deps

# don't include subdirectory-path-dependent -I and -L switches
STD_CPPFLAGS := $(filter-out -I$(top_srcdir)/src/include -I$(top_builddir)/src/include,$(CPPFLAGS))
STD_LDFLAGS := $(filter-out -L$(top_builddir)/src/common -L$(top_builddir)/src/port,$(LDFLAGS))
override CPPFLAGS += -DVAL_CC="\"$(CC)\""
override CPPFLAGS += -DVAL_CPPFLAGS="\"$(STD_CPPFLAGS)\""
override CPPFLAGS += -DVAL_CFLAGS="\"$(CFLAGS)\""
override CPPFLAGS += -DVAL_CFLAGS_SL="\"$(CFLAGS_SL)\""
override CPPFLAGS += -DVAL_LDFLAGS="\"$(STD_LDFLAGS)\""
override CPPFLAGS += -DVAL_LDFLAGS_EX="\"$(LDFLAGS_EX)\""
override CPPFLAGS += -DVAL_LDFLAGS_SL="\"$(LDFLAGS_SL)\""
override CPPFLAGS += -DVAL_LIBS="\"$(LIBS)\""

override CPPFLAGS := -DFRONTEND -I. -I$(top_srcdir)/src/common $(CPPFLAGS)
LIBS += $(PTHREAD_LIBS)

# If you add objects here, see also src/tools/msvc/Mkvcbuild.pm

OBJS_COMMON = \
	archive.o \
	base64.o \
	checksum_helper.o \
	config_info.o \
	controldata_utils.o \
	d2s.o \
	encnames.o \
	exec.o \
	f2s.o \
	file_perm.o \
	file_utils.o \
	hashfn.o \
	ip.o \
	jsonapi.o \
	keywords.o \
	kmgr_utils.o \
	kwlookup.o \
	link-canary.o \
	md5_common.o \
	pg_get_line.o \
	pg_lzcompress.o \
	pgfnames.o \
	psprintf.o \
	relpath.o \
	rmtree.o \
	saslprep.o \
	scram-common.o \
	string.o \
	stringinfo.o \
	unicode_norm.o \
	username.o \
	wait_error.o \
	wchar.o	 \
	mac.o	\
	hkdf.o	\
	pass_crypt.o \
	hex.o \
	plsqlwrap_crypto.o \
	wchar.o \
	tecc_algorithm/tecc_hkdf_sm3.o \
	tecc_algorithm/tecc_sm4_rnd.o \
	tecc_algorithm/tecc_det_sm4.o \
	sm4_adapter.o \
	tecc_algorithm/tecc_sm4_ope.o \
	tecc_algorithm/tecc_search_sm4.o \
	tecc_algorithm/tecc_sm4_ope.o \
	tecc_algorithm/tecc_search_sm4.o \
	tecc_algorithm/tecc_sm2_algo.o \
	tecc_algorithm/tecc_sm3_internal.o \
	tecc_algorithm/tecc_sm4_internal.o \
	tecc_algorithm/tecc_pattern.o \
	tecc_algorithm/tecc_algorithm_api.o \
	tecc_algorithm/tecc_ec_elgamal.o \
	tecc_algorithm/tecc_bsgs_tables_data.o \
	tecc_algorithm/tecc_elgamal.o \
	tecc_algorithm/tecc_byteorder.o \
	tecc_key/high_level_api.o \
	tecc_key/key_manager.o \
	tecc_key/kms_adapter.o \
	tecc_key/server_interface.o \
	tecc_key/security.o \
	tecc_key/config.o \
	tecc_key/tecc_sm2.o \
	tecc_key/utils.o


ifeq ($(with_gmssl),yes)
OBJS_COMMON += sm3_common.o
endif

ifeq ($(with_ssl),openssl)
OBJS_COMMON += \
	protocol_openssl.o \
	cryptohash_openssl.o \
	hmac_openssl.o \
	cipher_openssl.o
else
OBJS_COMMON += \
	cryptohash.o \
	hmac.o \
	md5.o \
	sha1.o \
	sha2.o \
	cipher.o
endif

# A few files are currently only built for frontend, not server
# (Mkvcbuild.pm has a copy of this list, too).  logging.c is excluded
# from OBJS_FRONTEND_SHLIB (shared library) as a matter of policy,
# because it is not appropriate for general purpose libraries such
# as libpq to report errors directly.
OBJS_FRONTEND_SHLIB = \
	$(OBJS_COMMON) \
	fe_memutils.o \
	restricted_token.o \
	sprompt.o
OBJS_FRONTEND = \
	$(OBJS_FRONTEND_SHLIB) \
	logging.o

# foo.o, foo_shlib.o, and foo_srv.o are all built from foo.c
OBJS_SHLIB = $(OBJS_FRONTEND_SHLIB:%.o=%_shlib.o)
OBJS_SRV = $(OBJS_COMMON:%.o=%_srv.o)

# where to find gen_keywordlist.pl and subsidiary files
TOOLSDIR = $(top_srcdir)/src/tools
GEN_KEYWORDLIST = $(PERL) -I $(TOOLSDIR) $(TOOLSDIR)/gen_keywordlist.pl
GEN_KEYWORDLIST_DEPS = $(TOOLSDIR)/gen_keywordlist.pl $(TOOLSDIR)/PerfectHash.pm

all: libpgcommon.a libpgcommon_shlib.a libpgcommon_srv.a

distprep: kwlist_d.h

# libpgcommon is needed by some contrib
install: all installdirs
	$(INSTALL_STLIB) libpgcommon.a '$(DESTDIR)$(libdir)/libpgcommon.a'
	$(INSTALL_STLIB) libpgcommon_shlib.a '$(DESTDIR)$(libdir)/libpgcommon_shlib.a'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(libdir)'

uninstall:
	rm -f '$(DESTDIR)$(libdir)/libpgcommon.a'
	rm -f '$(DESTDIR)$(libdir)/libpgcommon_shlib.a'

libpgcommon.a: $(OBJS_FRONTEND)
	rm -f $@
	$(AR) $(AROPT) $@ $^

#
# Shared library versions of object files
#

libpgcommon_shlib.a: $(OBJS_SHLIB)
	rm -f $@
	$(AR) $(AROPT) $@ $^

# Because this uses its own compilation rule, it doesn't use the
# dependency tracking logic from Makefile.global.  To make sure that
# dependency tracking works anyway for the *_shlib.o files, depend on
# their *.o siblings as well, which do have proper dependencies.  It's
# a hack that might fail someday if there is a *_shlib.o without a
# corresponding *.o, but there seems little reason for that.
%_shlib.o: %.c %.o
	$(CC) $(CFLAGS) $(CFLAGS_SL) $(CPPFLAGS) -c $< -o $@

#
# Server versions of object files
#

libpgcommon_srv.a: $(OBJS_SRV)
	rm -f $@
	$(AR) $(AROPT) $@ $^

# Because this uses its own compilation rule, it doesn't use the
# dependency tracking logic from Makefile.global.  To make sure that
# dependency tracking works anyway for the *_srv.o files, depend on
# their *.o siblings as well, which do have proper dependencies.  It's
# a hack that might fail someday if there is a *_srv.o without a
# corresponding *.o, but it works for now.
%_srv.o: %.c %.o
	$(CC) $(CFLAGS) $(subst -DFRONTEND,, $(CPPFLAGS)) -c $< -o $@

# generate SQL keyword lookup table to be included into keywords*.o.
kwlist_d.h: $(top_srcdir)/src/include/parser/kwlist.h $(GEN_KEYWORDLIST_DEPS)
	$(GEN_KEYWORDLIST) --extern $<

# Dependencies of keywords*.o need to be managed explicitly to make sure
# that you don't get broken parsing code, even in a non-enable-depend build.
keywords.o keywords_shlib.o keywords_srv.o: kwlist_d.h

# The code imported from Ryu gets a pass on declaration-after-statement,
# in order to keep it more closely aligned with its upstream.
RYU_FILES = d2s.o f2s.o
RYU_OBJS = $(RYU_FILES) $(RYU_FILES:%.o=%_shlib.o) $(RYU_FILES:%.o=%_srv.o)

$(RYU_OBJS): CFLAGS += $(PERMIT_DECLARATION_AFTER_STATEMENT)

# Compile tecc_key files
tecc_algorithm/%.o: tecc_algorithm/%.c
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F).Po

tecc_algorithm/%_shlib.o: tecc_algorithm/%.c tecc_algorithm/%.o
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(CFLAGS_SL) $(CPPFLAGS) -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F)_shlib.Po
 
tecc_algorithm/%_srv.o: tecc_algorithm/%.c
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(filter-out -DFRONTEND,$(CPPFLAGS)) -DFRONTEND=0 -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F)_srv.Po

# Compile tecc_key files
tecc_key/%.o: tecc_key/%.c
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F).Po

tecc_key/%_shlib.o: tecc_key/%.c tecc_key/%.o
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(CFLAGS_SL) $(CPPFLAGS) -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F)_shlib.Po

tecc_key/%_srv.o: tecc_key/%.c
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(filter-out -DFRONTEND,$(CPPFLAGS)) -DFRONTEND=0 -c $< -o $@ -MMD -MP -MF $(DEPDIR)/$(*F)_srv.Po

# ---- ReqID:TECC4.0 - BSGS 静态表数据生成（保证与核心算法一致）----
# 生成程序直接 include tecc_ec_elgamal.c 复用核心建表逻辑；核心算法
# （tecc_ec_elgamal.c）变化时自动重新编译生成程序并重新生成表数据，
# 使静态表数据始终与算法同源（详见 tecc_gen_bsgs_table.c 文件头注释）。
tecc_algorithm/tecc_gen_bsgs_table: tecc_algorithm/tecc_gen_bsgs_table.c tecc_algorithm/tecc_ec_elgamal.c
	@if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi
	$(CC) $(CFLAGS) $(PTHREAD_CFLAGS) -I$(top_srcdir)/src/include -I$(srcdir)/tecc_algorithm \
		-o $@ $(srcdir)/tecc_algorithm/tecc_gen_bsgs_table.c $(LDFLAGS) -lssl -lcrypto -lz $(PTHREAD_LIBS)

tecc_algorithm/tecc_bsgs_tables_data.c tecc_algorithm/tecc_bsgs_tables_data.h: tecc_algorithm/tecc_gen_bsgs_table tecc_algorithm/tecc_ec_elgamal.c
	cd $(srcdir)/tecc_algorithm && ./tecc_gen_bsgs_table

tecc_algorithm/tecc_ec_elgamal.o tecc_algorithm/tecc_ec_elgamal_shlib.o tecc_algorithm/tecc_ec_elgamal_srv.o: tecc_algorithm/tecc_bsgs_tables_data.h

# kwlist_d.h is in the distribution tarball, so it is not cleaned here.
clean distclean:
	rm -f libpgcommon.a libpgcommon_shlib.a libpgcommon_srv.a
	rm -f $(OBJS_FRONTEND) $(OBJS_SHLIB) $(OBJS_SRV)
	rm -f tecc_algorithm/*.o tecc_algorithm/*_shlib.o tecc_algorithm/*_srv.o
	rm -f tecc_algorithm/tecc_gen_bsgs_table
	rm -f tecc_key/*.o tecc_key/*_shlib.o tecc_key/*_srv.o

maintainer-clean: distclean
	rm -f kwlist_d.h
