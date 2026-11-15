#-------------------------------------------------------------------------
#
# Makefile for src/bin/tecc_ktool
#
# TECC Key Management Tool — offline KEK file generator
#
#-------------------------------------------------------------------------

PGFILEDESC = "tecc_ktool - TECC key management tool"
PGAPPICON=win32

subdir = src/bin/tecc_ktool
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

override CPPFLAGS := -I$(libpq_srcdir) -I$(top_srcdir)/src/common/tecc_key \
	-I$(top_srcdir)/src/include/common/tecc_key $(CPPFLAGS)

KEK_FILE_C = $(top_srcdir)/src/common/tecc_key/kek_file.c

OBJS = \
	$(WIN32RES) \
	tecc_ktool.o \
	cmd_generate.o \
	cmd_list.o \
	cmd_info.o \
	cmd_delete.o \
	cmd_validate.o \
	sm2_pub.o \
	kek_file.o

all: tecc_ktool

kek_file.o: $(KEK_FILE_C) $(top_srcdir)/src/include/common/tecc_key/kek_file.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(KEK_FILE_C) -o $@

tecc_ktool: $(OBJS) | submake-libpq submake-libpgport
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(libpq_pgport) $(LIBS) -o $@$(X)

install: all
	$(INSTALL_PROGRAM) tecc_ktool$(X) '$(DESTDIR)$(bindir)'/tecc_ktool$(X)

clean distclean maintainer-clean:
	rm -f tecc_ktool$(X) $(OBJS) *.o
