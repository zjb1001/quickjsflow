CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=

SRC := src/main.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
INC := -Iinclude

BIN := build/quickjsflow
TEST_BINS := build/test_integration build/test_roundtrip build/test_expressions build/test_statements build/test_phase1_full build/test_scope build/test_edit

.PHONY: all clean run tests test

all: $(BIN)

tests: $(TEST_BINS)

$(BIN): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(LDFLAGS)

build/test_integration: test/test_integration.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_integration.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_roundtrip: test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_expressions: test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_statements: test/test_statements.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_statements.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_phase1_full: test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_scope: test/test_scope.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_scope.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_edit: test/test_edit.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_edit.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

test: tests
	./build/test_integration
	./build/test_roundtrip
	./build/test_expressions
	./build/test_statements
	./build/test_phase1_full
	./build/test_scope
	./build/test_edit

clean:
	rm -rf build

run: $(BIN)
	./$(BIN)
