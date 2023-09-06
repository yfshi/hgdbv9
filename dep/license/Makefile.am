NO_GENERATED_HEADERS=yes

subdir = dep/license/x86_64/lib
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

license_dat = $(top_builddir)/dep/license/x86_64/lib/license.dat

NAME = license
SO_MAJOR_VERSION=1
SO_MINOR_VERSION=3

include $(top_builddir)/src/Makefile.shlib 

install: install-lib-shared install-license

install-license:
	$(MKDIR_P) '$(DESTDIR)${prefix}/license'
	$(INSTALL_DATA) $(license_dat) '$(DESTDIR)${prefix}/license/'

uninstall: uninstall-lib uninstall-license

uninstall-license:
	rm -f '$(DESTDIR)${prefix}/license'/$(notdir $(license_dat))
