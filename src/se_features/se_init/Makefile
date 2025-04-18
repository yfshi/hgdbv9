#-------------------------------------------------------------------------
#
# Makefile for backend/catalog
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/backend/catalog/Makefile
#
#-------------------------------------------------------------------------

subdir = src/SEfeatures/SE_init
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = \
	SE_init.o

%: %.c

%: %.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)
# all: 
# 	@echo "we are building data..."
# OBJS_COM = $(OBJS_COMMON:%.o=%_srv.o)

# %_srv.o: %.c %.o
# 	$(CC) $(CFLAGS) $(subst -DFRONTEND,, $(CPPFLAGS)) -c $< -o $@

# libSEfeatures.a: $(OBJS_COM)
# 	rm -f $@
# 	$(AR) $(AROPT) $@ $^

# Note: installation of generated headers is handled elsewhere
.PHONY: install-data
install-data: installdirs
	$(INSTALL_DATA) $(srcdir)/security_roles.sql '$(DESTDIR)$(datadir)/security_roles.sql'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(datadir)'

.PHONY: uninstall-data
uninstall-data:
	rm -f $(addprefix '$(DESTDIR)$(datadir)'/, security_roles.sql)

# postgres.bki, system_constraints.sql, and the generated headers are
# in the distribution tarball, so they are not cleaned here.
# clean:

# maintainer-clean: clean
# 	rm -f bki-stamp postgres.bki system_constraints.sql $(GENERATED_HEADERS)