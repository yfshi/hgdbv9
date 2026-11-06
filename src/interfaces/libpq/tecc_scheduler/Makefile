# Begin - ReqID:TECC4.0
#
# Makefile for TECC crypto scheduler module
#

subdir = src/interfaces/libpq/tecc_scheduler
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

# Source files
OBJS = \
    tecc_crypto_scheduler.o \
    tecc_crypto_context.o \
    tecc_algorithm_scheduler.o \
    tecc_key_manager_adapter.o

# Include paths
override CPPFLAGS := -I$(libpq_srcdir) -I$(srcdir) -I../../../common/tecc_key $(CPPFLAGS)

# TECC modules use C99 standard, remove C90-specific warning flags for TECC
override CFLAGS := $(filter-out -Wdeclaration-after-statement,$(CFLAGS)) -std=c99

# Build rules
all: $(OBJS)

# Dependencies
tecc_crypto_scheduler.o: \
    tecc_crypto_scheduler.h \
    tecc_crypto_context.h \
    tecc_algorithm_scheduler.h \
    tecc_key_manager_adapter.h \
    ../tecc_processor/internal/tecc_processor_internal.h \
    ../tecc_cache/tecc_metadata_cache.h

tecc_crypto_context.o: \
    tecc_crypto_context.h \
    tecc_crypto_scheduler.h \
    ../tecc_processor/internal/tecc_processor_internal.h

tecc_algorithm_scheduler.o: \
    tecc_algorithm_scheduler.h \
    tecc_crypto_context.h \
    ../tecc_processor/internal/tecc_processor_internal.h

tecc_key_manager_adapter.o: \
    tecc_key_manager_adapter.h \
    ../tecc_processor/internal/tecc_processor_internal.h \
    ../../../common/tecc_key/high_level_api.h \
    ../../../common/tecc_key/key_manager.h


# Clean
clean:
	rm -f $(OBJS)

# Install (headers only)
install: all
	$(INSTALL_DATA) $(srcdir)/tecc_crypto_scheduler.h '$(DESTDIR)$(includedir_server)'

# Uninstall
uninstall:
	rm -f '$(DESTDIR)$(includedir_server)/tecc_crypto_scheduler.h'

# End - ReqID:TECC4.0