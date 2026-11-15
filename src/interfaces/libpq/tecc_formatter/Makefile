#-------------------------------------------------------------------------
#
# Makefile for src/interfaces/libpq/tecc_formatter module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/interfaces/libpq/tecc_formatter/Makefile
#
#-------------------------------------------------------------------------

subdir = src/interfaces/libpq/tecc_formatter

# Include parent directory's Makefile.global
include $(top_builddir)/src/Makefile.global

# Module name
NAME = tecc_formatter

# Source files
OBJS = \
    tecc_formatter.o \
    tecc_normalizer.o \
    tecc_text_normalizer.o \
    tecc_data_validator.o \
    tecc_type_converter.o \
    tecc_hex_encoder.o \
    tecc_buffer_manager.o \
    tecc_formatter_utils.o \
    tecc_icu_normalizer.o \
    tecc_type_output.o

# Include directories
override CPPFLAGS := -I$(srcdir) -I$(srcdir)/../ -I$(srcdir)/../tecc_scheduler $(CPPFLAGS)

# ICU is mandatory dependency for TECC data formatter module
# Check if ICU headers exist by testing file existence
ifeq (,$(wildcard /usr/include/unicode/ucol.h))
$(error ICU library not found. ICU is required for TECC data formatter module. Install libicu-dev package.)
endif

# Always include ICU normalizer
OBJS += tecc_icu_normalizer.o
CPPFLAGS += -DUSE_ICU -DFRONTEND -I/usr/include

# Library dependencies
SHLIB_LINK += -licuuc -licui18n

# Build rules
all: all-lib

# Pattern rules for object files
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Special dependencies
tecc_formatter.o: tecc_formatter.c tecc_formatter.h
tecc_normalizer.o: tecc_normalizer.c tecc_normalizer.h tecc_formatter.h
tecc_text_normalizer.o: tecc_text_normalizer.c tecc_text_normalizer.h tecc_formatter.h
tecc_data_validator.o: tecc_data_validator.c tecc_data_validator.h tecc_formatter.h
tecc_type_converter.o: tecc_type_converter.c tecc_type_converter.h tecc_formatter.h
tecc_hex_encoder.o: tecc_hex_encoder.c tecc_hex_encoder.h tecc_formatter.h
tecc_buffer_manager.o: tecc_buffer_manager.c tecc_buffer_manager.h tecc_formatter.h
tecc_formatter_utils.o: tecc_formatter_utils.c tecc_formatter_utils.h tecc_formatter.h
tecc_icu_normalizer.o: tecc_icu_normalizer.c tecc_icu_normalizer.h tecc_formatter.h
tecc_type_output.o: tecc_type_output.c tecc_type_output.h tecc_formatter.h tecc_formatter_internal.h
# Note: tecc_formatter_guc module has been removed

# Clean rules
clean:
	rm -f $(OBJS)

distclean: clean

# Install rules (formatter is part of libpq, no separate installation)
install: all

# Check dependencies
deps:
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) *.c > .depend

# Include dependency file if it exists
ifneq ($(wildcard .depend),.depend)
include .depend
endif