# contrib/pgcrypto/Makefile

INT_SRCS = internal.c internal-sha2.c internal-sm3.c blf.c rijndael.c sm4.c \
		pgp-mpi-internal.c imath.c
INT_TESTS = sha2

OSSL_SRCS = openssl.c pgp-mpi-openssl.c
OSSL_TESTS = sha2 des 3des cast5

ZLIB_TST = pgp-compression
ZLIB_OFF_TST = pgp-zlib-DISABLED

CF_SRCS = $(if $(subst openssl,,$(with_ssl)), $(INT_SRCS), $(OSSL_SRCS))
CF_TESTS = $(if $(subst openssl,,$(with_ssl)), $(INT_TESTS), $(OSSL_TESTS))
CF_PGP_TESTS = $(if $(subst no,,$(with_zlib)), $(ZLIB_TST), $(ZLIB_OFF_TST))

MIN_OPENSSL_VERSION := 1.1.1

OPENSSL_VERSION := $(shell pkg-config --modversion openssl)

OPENSSL_VERSION_CHECK := $(shell \
    printf "%s\n%s" "$(OPENSSL_VERSION)" "$(MIN_OPENSSL_VERSION)" | sort -V | head -n1)

ifneq ($(OPENSSL_VERSION_CHECK),$(MIN_OPENSSL_VERSION))
CF_SRCS = $(INT_SRCS)
CF_TESTS = $(INT_TESTS)
endif

SRCS = \
	$(CF_SRCS) \
	crypt-blowfish.c \
	crypt-des.c \
	crypt-gensalt.c \
	crypt-md5.c \
	mbuf.c \
	pgcrypto.c \
	pgp-armor.c \
	pgp-cfb.c \
	pgp-compress.c \
	pgp-decrypt.c \
	pgp-encrypt.c \
	pgp-info.c \
	pgp-mpi.c \
	pgp-pgsql.c \
	pgp-pubdec.c \
	pgp-pubenc.c \
	pgp-pubkey.c \
	pgp-s2k.c \
	pgp.c \
	px-crypt.c \
	px-hmac.c \
	px.c

MODULE_big	= pgcrypto
OBJS = \
	$(SRCS:.c=.o) \
	$(WIN32RES)

EXTENSION = pgcrypto
DATA = pgcrypto--1.3.sql pgcrypto--1.2--1.3.sql pgcrypto--1.1--1.2.sql \
	pgcrypto--1.0--1.1.sql
PGFILEDESC = "pgcrypto - cryptographic functions"

REGRESS = init md5 sha1 sm3 hmac-md5 hmac-sha1 hmac-sm3 blowfish rijndael sm4 \
	$(CF_TESTS) \
	crypt-des crypt-md5 crypt-blowfish crypt-xdes \
	pgp-armor pgp-decrypt pgp-encrypt $(CF_PGP_TESTS) \
	pgp-pubkey-decrypt pgp-pubkey-encrypt pgp-info
ORA_REGRESS = init md5 sha1 sm3 hmac-md5 hmac-sha1 hmac-sm3 blowfish rijndael sm4 \
	$(CF_TESTS) \
	crypt-des crypt-md5 crypt-blowfish crypt-xdes \
	pgp-armor pgp-decrypt pgp-encrypt $(CF_PGP_TESTS) \
	pgp-pubkey-decrypt pgp-pubkey-encrypt pgp-info
#Begin - ReqID:SRS-MISC-MYSQL-FRAME
MYS_REGRESS = mys_init mys_md5 mys_sha1 sm3 mys_hmac-md5 mys_hmac-sha1 hmac-sm3 mys_blowfish mys_rijndael sm4 \
	$(CF_TESTS) \
	mys_crypt-des mys_crypt-md5 mys_crypt-blowfish mys_crypt-xdes \
	mys_pgp-armor mys_pgp-decrypt mys_pgp-encrypt $(CF_PGP_TESTS) \
	mys_pgp-pubkey-decrypt mys_pgp-pubkey-encrypt mys_pgp-info
#End - ReqID:SRS-MISC-MYSQL-FRAME

EXTRA_CLEAN = gen-rtab

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pgcrypto
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# Add libraries that pgcrypto depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)
SHLIB_LINK += $(filter -lcrypto -lz, $(LIBS))
ifeq ($(PORTNAME), win32)
SHLIB_LINK += $(filter -leay32, $(LIBS))
# those must be at the end
SHLIB_LINK += -lws2_32
endif

# Upstream uses a larger subset of C99.
imath.o: CFLAGS+=$(PERMIT_DECLARATION_AFTER_STATEMENT)

rijndael.o: rijndael.tbl

rijndael.tbl:
	$(CC) $(CPPFLAGS) $(CFLAGS) -DPRINT_TABS rijndael.c -o gen-rtab
	./gen-rtab > rijndael.tbl
