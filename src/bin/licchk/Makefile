# src/bin/licchk/Makefile

PGFILEDESC = "licchk - check and print licenses info."
PGAPPICON = win32

subdir = src/bin/licchk
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# We remove liblicense.so from LIBS, using liblicense.a
ifeq ($(with_license),yes)
LIBS := $(subst -llicense, -l:liblicense.a, $(LIBS))
endif

OBJS = \
	$(WIN32RES) \
	licchk.o

all: licchk

licchk: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

install: all installdirs
	$(INSTALL_PROGRAM) licchk$(X) '$(DESTDIR)$(bindir)/licchk$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/licchk$(X)'

clean distclean maintainer-clean:
	rm -f licchk$(X) $(OBJS)
	rm -rf tmp_check

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)
