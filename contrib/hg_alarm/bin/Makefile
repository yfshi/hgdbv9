PGFILEDESC = "hg_alarm - examine the file structure"

PROGRAM = hg_alarm
OBJS = \
	$(WIN32RES) \
	hg_alarm.o \
	logger_fe.o \
	parse_int.o \
	json_opt.o \
	pqfe.o \
	memory.o \
	datadir.o \
	database.o \
	relation.o

# install a template conf file
DOCS = hg_alarm_conf.example

PG_CPPFLAGS = -I../include -I$(libpq_srcdir) -DFRONTED
PG_LIBS_INTERNAL = $(libpq_pgport)

ifdef PGSQL
PG_CPPFLAGS += -D_PG_ALARM
endif

# enable debug info output
ifdef HGA_DEBUG
PG_CPPFLAGS += -DHGA_DEBUG
endif

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
# import static library 'libpgfeutils',
#	PGXS case: 'make install' already done, headers and .a can be found in '$(includedir)' and '$(libdir)'
# PG_CPPFLAGS += $(includedir)
# PG_LIBS_INTERNAL += -L$(libdir) -lpgfeutils
else
subdir = contrib/hg_alarm
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
# import static library 'libpgfeutils',
#	no PGXS case: 'make install' not done yet, headers and .a can only be found in source directory
# PG_CPPFLAGS += $(top_srcdir)/src/fe_utils
# PG_LIBS_INTERNAL += -L$(top_builddir)/src/fe_utils -lpgfeutils
endif
pqfe.o : ../common/pqfe.o
	mv $< $@
json_opt.o : ../common/json_opt.o
	mv $< $@
parse_int.o : ../common/parse_int.o
	mv $< $@
memory.o : ../common/memory.o
	mv $< $@
datadir.o : ../common/datadir.o
	mv $< $@
database.o : ../common/database.o
	mv $< $@
relation.o : ../common/relation.o
	mv $< $@