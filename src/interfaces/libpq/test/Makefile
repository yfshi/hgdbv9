subdir = src/interfaces/libpq/test
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

ifeq ($(PORTNAME), win32)
LDFLAGS += -lws2_32
endif

override CPPFLAGS := -I$(libpq_srcdir) $(CPPFLAGS)
LDFLAGS_INTERNAL += $(libpq_pgport)

PROGS = uri-regress

# TAP tests for the removed ssl_* connection options (see t/tlcp_compatible.pl)
TAP_TESTS = 1

# Export so the TAP test can gate itself on the --enable-compatible-ak build
# flag via $ENV{enable_compatible_ak}.
export enable_compatible_ak

all: $(PROGS)

installcheck: all
	SRCDIR='$(top_srcdir)' SUBDIR='$(subdir)' \
		   $(PERL) $(top_srcdir)/$(subdir)/regress.pl
ifdef TAP_TESTS
	$(prove_installcheck)
endif

#BEGIN - SQL PARSER
oracle-installcheck: all
	SRCDIR='$(top_srcdir)' SUBDIR='$(subdir)' \
		   $(PERL) $(top_srcdir)/$(subdir)/regress.pl
ifdef TAP_TESTS
	$(prove_installcheck)
endif
#END - SQL PARSER

check: all
ifdef TAP_TESTS
	$(prove_check)
endif

clean distclean maintainer-clean:
	rm -f $(PROGS) *.o
	rm -f regress.out regress.diff
