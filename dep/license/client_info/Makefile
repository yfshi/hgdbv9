NO_GENERATED_HEADERS=yes

subdir = dep/license/client_info
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

client_info_dat = $(top_builddir)/dep/license/client_info/client_info.dat

install: install-client-info

install-client-info:
	$(MKDIR_P) '$(DESTDIR)${prefix}/license'
	$(INSTALL_DATA) $(client_info_dat) '$(DESTDIR)${prefix}/license/'

uninstall: uninstall-client-info

uninstall-client-info:
	rm -f '$(DESTDIR)${prefix}/license'/$(notdir $(client_info_dat))
