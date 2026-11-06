# Makefile for TECC frontend parser test programs
# Authored by <wangxp@highgo.com>, 20260413

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wno-unused-function -Wno-deprecated-declarations \
         -I../../../../../src/include \
         -I../../../../../src/interfaces/libpq \
         -I../../../../../src/interfaces/libpq/frontend_parser \
         -D_GNU_SOURCE -g -DFRONTEND -DFRONTEND_PARSER

# Parser test source files
PARSER_TEST_SRCS = test_parser_basic.c \
                   test_parser_nodes.c \
                   test_parser_integration.c \
                   test_parser_performance.c \
                   test_parser_error.c \
                   test_collate.c

PARSER_TEST_OBJS = $(addsuffix .o, $(PARSER_TEST_TARGETS))
PARSER_TEST_TARGETS = test_parser_basic \
                       test_parser_nodes \
                       test_parser_integration \
                       test_parser_performance \
                       test_parser_error

# Frontend parser source files
PARSER_SRC_DIR = ../../frontend_parser
PARSER_SRCS = $(PARSER_SRC_DIR)/tecc_parser.c \
              $(PARSER_SRC_DIR)/fe_adapter.c \
              $(PARSER_SRC_DIR)/tecc_nodes.c \
              $(PARSER_SRC_DIR)/tecc_error.c \
              $(PARSER_SRC_DIR)/tecc_grammar.c \
              $(PARSER_SRC_DIR)/lex.tecc_yy.c \
              $(PARSER_SRC_DIR)/tecc_perf.c \
              $(PARSER_SRC_DIR)/tecc_semantic.c \
              ../../tecc_common/tecc_frontend_list.c \
              ../../tecc_common/tecc_logging.c

# Adapter for PostgreSQL functions in test environment
ADAPTER_SRCS = fe_memutils_adapter.c

PARSER_OBJS = $(PARSER_SRCS:.c=.o) $(ADAPTER_SRCS:.c=.o)

# Default target
all: $(PARSER_TEST_TARGETS)

# Build individual test programs
test_parser_basic: test_parser_basic.o $(PARSER_OBJS)
	$(CC) -o $@ $^

test_parser_nodes: test_parser_nodes.o $(PARSER_OBJS)
	$(CC) -o $@ $^

test_parser_integration: test_parser_integration.o $(PARSER_OBJS)
	$(CC) -o $@ $^

test_parser_performance: test_parser_performance.o $(PARSER_OBJS)
	$(CC) -o $@ $^

test_parser_error: test_parser_error.o $(PARSER_OBJS)
	$(CC) -o $@ $^

test_collate: test_collate.o $(PARSER_OBJS)
	$(CC) -o $@ $^

# Compile test programs
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Regenerate grammar and scanner sources from .y/.l files
$(PARSER_SRC_DIR)/tecc_grammar.c $(PARSER_SRC_DIR)/tecc_grammar.h: $(PARSER_SRC_DIR)/tecc_grammar.y
	cd $(PARSER_SRC_DIR) && bison -d -o tecc_grammar.c tecc_grammar.y

$(PARSER_SRC_DIR)/lex.tecc_yy.c: $(PARSER_SRC_DIR)/tecc_scanner.l $(PARSER_SRC_DIR)/tecc_grammar.h
	cd $(PARSER_SRC_DIR) && flex -o lex.tecc_yy.c tecc_scanner.l

# Compile parser source files
tecc_parser.o: $(PARSER_SRC_DIR)/tecc_parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

fe_adapter.o: $(PARSER_SRC_DIR)/fe_adapter.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_nodes.o: $(PARSER_SRC_DIR)/tecc_nodes.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_error.o: $(PARSER_SRC_DIR)/tecc_error.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_frontend_list.o: ../../tecc_common/tecc_frontend_list.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_grammar.o: $(PARSER_SRC_DIR)/tecc_grammar.c $(PARSER_SRC_DIR)/tecc_grammar.h
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_logging.o: ../../tecc_common/tecc_logging.c
	$(CC) $(CFLAGS) -c -o $@ $<

lex.tecc_yy.o: $(PARSER_SRC_DIR)/lex.tecc_yy.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_perf.o: $(PARSER_SRC_DIR)/tecc_perf.c
	$(CC) $(CFLAGS) -c -o $@ $<

tecc_semantic.o: $(PARSER_SRC_DIR)/tecc_semantic.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Disable implicit yacc rules (use explicit bison rules above)
%.c: %.y
%.h: %.y
%.c: %.l

# Compile adapter files
fe_memutils_adapter.o: fe_memutils_adapter.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run all parser tests
test: all
	@echo "Running parser tests..."
	@echo ""
	@for test in $(PARSER_TEST_TARGETS); do \
		echo "=== Running $$test ==="; \
		./$$test || exit 1; \
		echo ""; \
	done
	@echo "All parser tests passed!"

# Clean up
clean:
	rm -f $(PARSER_TEST_OBJS) $(PARSER_TEST_TARGETS)
	rm -f $(PARSER_OBJS)

.PHONY: all test clean