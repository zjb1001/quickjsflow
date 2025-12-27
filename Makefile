CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=

SRC := src/main.c src/lexer.c src/parser.c src/ast_print.c
INC := -Iinclude

BIN := build/quickjsflow
TEST_BINS := build/test_integration build/test_roundtrip build/test_expressions build/test_statements build/test_phase1_full

.PHONY: all clean run tests test

all: $(BIN)

tests: $(TEST_BINS)

$(BIN): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(LDFLAGS)

build/test_integration: test/test_integration.c src/lexer.c src/parser.c src/ast_print.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_integration.c src/lexer.c src/parser.c src/ast_print.c $(LDFLAGS)

build/test_roundtrip: test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c $(LDFLAGS)

build/test_expressions: test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c $(LDFLAGS)

build/test_statements: test/test_statements.c src/lexer.c src/parser.c src/ast_print.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_statements.c src/lexer.c src/parser.c src/ast_print.c $(LDFLAGS)

build/test_phase1_full: test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c $(LDFLAGS)

test: tests
	./build/test_integration
	./build/test_roundtrip
	./build/test_expressions
	./build/test_statements
	./build/test_phase1_full

clean:
	rm -rf build

run: $(BIN)
	./$(BIN)
