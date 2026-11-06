#-------------------------------------------------------------------------
#
# Makefile
#    Makefile for TECC key management module
#
# Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/common/tecc_key/Makefile
#
# ReqID:TECC4.0 - Key management module build
# Authored by <wanghaoyan@highgo.com>, 20260408.
# Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
#
#-------------------------------------------------------------------------

MODULE_big = tecc_key
OBJS = \
    high_level_api.o \
    key_manager.o \
    kms_adapter.o \
    server_interface.o \
    security.o \
    config.o \
    utils.o

PGFILEDESC = "TECC Key Management Module"

# PostgreSQL build system integration
# In a real PostgreSQL build, this would use PGXS

# Default target - just list object files
all:
	@echo "Object files to build:"
	@echo "  $(OBJS)"
	@echo ""
	@echo "Note: This Makefile is for reference only."
	@echo "Actual compilation is done through PostgreSQL build system."

# Clean target
clean:
	rm -f $(OBJS) *.dll *.so *.a *.o

# Dependencies (for reference)
high_level_api.o: high_level_api.c high_level_api.h key_manager.h security.h utils.h
key_manager.o: key_manager.c key_manager.h server_interface.h kms_adapter.h utils.h
kms_adapter.o: kms_adapter.c kms_adapter.h security.h utils.h
server_interface.o: server_interface.c server_interface.h utils.h
security.o: security.c security.h utils.h
config.o: config.c config.h utils.h
utils.o: utils.c utils.h

.PHONY: all clean