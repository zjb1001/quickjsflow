CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=

SRC := src/main.c src/lexer.c src/parser.c src/ast_print.c
INC := -Iinclude

BIN := build/quickjsflow

.PHONY: all clean run

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(LDFLAGS)

clean:
	rm -rf build

run: $(BIN)
	./$(BIN)
