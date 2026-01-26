# contrib/Makefile

subdir = contrib
top_builddir = ..
include $(top_builddir)/src/Makefile.global

SUBDIRS = \
		adminpack	\
		amcheck		\
		auth_delay	\
		auto_explain	\
		bloom		\
		btree_gin	\
		btree_gist	\
		citext		\
		cube		\
		dblink		\
		dict_int	\
		dict_xsyn	\
		earthdistance	\
		file_fdw	\
		fuzzystrmatch	\
		hg_sysmac       \
		hg_antitamper   \
		hstore		\
		intagg		\
		intarray	\
		isn		\
		lo		\
		ltree		\
		oid2name	\
		old_snapshot	\
		pageinspect	\
		passwordcheck	\
		pg_buffercache	\
		pg_freespacemap \
		pg_prewarm	\
		pg_stat_statements \
		pg_surgery	\
		pg_trgm		\
		pgcrypto	\
		pgrowlocks	\
		pgstattuple	\
		pg_visibility	\
		postgres_fdw	\
		seg		\
		spi		\
		tablefunc	\
		tcn		\
		test_decoding	\
		tsm_system_rows \
		tsm_system_time \
		unaccent	\
		vacuumlo	\
		gb18030_2022	\
		ivorysql_ora	\
		ora_btree_gist	\
		ora_btree_gin	\
		pg_object	\
		ivorysql_my	\
		mys_btree_gist \
		mys_btree_gin	\
		uuid-ossp \
		hg_misc	\
		ora_synonym	\
		hsh		\
		hg_redaction	\
		conn_restrict

ifeq ($(with_ssl),openssl)
SUBDIRS += sslinfo
else
ALWAYS_SUBDIRS += sslinfo
endif

#ifneq ($(with_uuid),no)
#SUBDIRS += uuid-ossp
#else
#ALWAYS_SUBDIRS += uuid-ossp
#endif

ifeq ($(with_libxml),yes)
SUBDIRS += xml2
else
ALWAYS_SUBDIRS += xml2
endif

ifeq ($(with_selinux),yes)
SUBDIRS += sepgsql
else
ALWAYS_SUBDIRS += sepgsql
endif

ifeq ($(with_perl),yes)
SUBDIRS += bool_plperl hstore_plperl jsonb_plperl
else
ALWAYS_SUBDIRS += bool_plperl hstore_plperl jsonb_plperl
endif

ifeq ($(with_python),yes)
SUBDIRS += hstore_plpython jsonb_plpython ltree_plpython
else
ALWAYS_SUBDIRS += hstore_plpython jsonb_plpython ltree_plpython
endif

# /* Begin - ReqID:SRS-SECURITY-POWERS */
#always add restricted_dba extension
SUBDIRS += restricted_dba
# /* End - ReqID:SRS-SECURITY-POWERS */

#Begin - ReqID:SRS-SECURITY-AUDIT
SUBDIRS += pg_audit pg_track_settings
ifeq ($(with_sqlite), yes)
SUBDIRS += sqlite_fdw
endif
#End - ReqID:SRS-SECURITY-AUDIT
# Begin - ReqID:SRS-SECURITY-IDENTITYAUTH
SUBDIRS += credcheck
# End - ReqID:SRS-SECURITY-IDENTITYAUTH

# /* Begin - ReqID:SRS-MISC-PDR */
SUBDIRS += pg_wait_sampling worker_pg_pdr
# /* End - ReqID:SRS-MISC-PDR */

# Missing:
#		start-scripts	\ (does not have a makefile)


$(recurse)
$(recurse_always)
