# Top-level Makefile for TECC tests
# Authored by <wangxp@highgo.com>, 20260413
# This Makefile delegates to Makefile.all for all operations

# Default target
all:
	$(MAKE) -f Makefile.all all

# Build algorithm tests
algorithm:
	$(MAKE) -f Makefile.all algorithm

# Build processor tests
processor:
	$(MAKE) -f Makefile.all processor

# Build formatter tests
formatter:
	$(MAKE) -f Makefile.all formatter

# Build parser tests
parser:
	$(MAKE) -f Makefile.all parser

# Run all tests
test:
	$(MAKE) -f Makefile.all test

# Clean all
clean:
	$(MAKE) -f Makefile.all clean

.PHONY: all algorithm processor formatter test clean