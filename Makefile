CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=
COVERAGE_FLAGS := -fprofile-arcs -ftest-coverage --coverage
AFL_CC ?= afl-gcc

SRC := src/main.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c src/cfg.c
INC := -Iinclude

BIN := build/quickjsflow
TEST_BINS := build/test_integration build/test_roundtrip build/test_expressions build/test_statements build/test_phase1_full build/test_scope build/test_edit build/test_cfg
BENCHMARK_BIN := build/benchmark/benchmark
FUZZ_BIN := build/fuzz/fuzz_target

.PHONY: all clean run tests test coverage test-coverage coverage-report benchmark run-benchmark fuzz-build fuzz-test fuzz-test-ci help

all: $(BIN)

tests: $(TEST_BINS)

# Help target
help:
	@echo "QuickJSFlow Build System"
	@echo "========================"
	@echo ""
	@echo "Build targets:"
	@echo "  make all              - Build main executable"
	@echo "  make tests            - Build all test binaries"
	@echo "  make test             - Build and run all tests"
	@echo ""
	@echo "Coverage targets:"
	@echo "  make coverage         - Build with coverage instrumentation"
	@echo "  make test-coverage    - Run tests with coverage collection"
	@echo "  make coverage-report  - Generate HTML coverage report"
	@echo ""
	@echo "Benchmark targets:"
	@echo "  make benchmark        - Build benchmark suite"
	@echo "  make run-benchmark    - Run performance benchmarks"
	@echo ""
	@echo "Fuzzing targets:"
	@echo "  make fuzz-build       - Build fuzzer with AFL"
	@echo "  make fuzz-test        - Run fuzzing (interactive)"
	@echo "  make fuzz-test-ci     - Run fuzzing for CI (5 min)"
	@echo ""
	@echo "Utility targets:"
	@echo "  make clean            - Remove all build artifacts"
	@echo "  make run              - Build and run main executable"


$(BIN): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(LDFLAGS)

build/test_integration: test/test_integration.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_integration.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_roundtrip: test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_roundtrip.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_expressions: test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_expressions.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_statements: test/test_statements.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_statements.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_phase1_full: test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_phase1_full.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c $(LDFLAGS)

build/test_scope: test/test_scope.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_scope.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_edit: test/test_edit.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_edit.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

build/test_cfg: test/test_cfg.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c src/cfg.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ test/test_cfg.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c src/cfg.c $(LDFLAGS)

test: tests
	./build/test_integration
	./build/test_cfg
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

# Coverage targets
coverage:
	@mkdir -p build/coverage
	$(CC) $(CFLAGS) $(COVERAGE_FLAGS) $(INC) -o build/coverage/test_all \
		test/test_integration.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c src/cfg.c

test-coverage: coverage
	@echo "Running tests with coverage..."
	cd build/coverage && ./test_all
	@echo "Coverage data collected"

coverage-report: test-coverage
	@echo "Generating coverage report..."
	lcov --capture --directory build/coverage --output-file build/coverage/coverage.info
	lcov --remove build/coverage/coverage.info '/usr/*' --output-file build/coverage/coverage.info
	lcov --list build/coverage/coverage.info
	genhtml build/coverage/coverage.info --output-directory build/coverage/html
	@echo "HTML coverage report: build/coverage/html/index.html"

# Benchmark targets
benchmark: $(BENCHMARK_BIN)

$(BENCHMARK_BIN): test/benchmark.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c
	@mkdir -p build/benchmark
	$(CC) $(CFLAGS) $(INC) -o $@ test/benchmark.c src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)

run-benchmark: benchmark
	@mkdir -p build/benchmark
	./$(BENCHMARK_BIN)

# Fuzzing targets
fuzz-build:
	@echo "Setting up fuzzing environment..."
	@bash scripts/setup-fuzzing.sh
	@echo "Building fuzzer with AFL..."
	@mkdir -p build/fuzz
	$(AFL_CC) $(CFLAGS) $(INC) -o $(FUZZ_BIN) test/fuzz_target.c \
		src/lexer.c src/parser.c src/ast_print.c src/scope.c src/edit.c src/codegen.c $(LDFLAGS)
	@echo "Fuzzer built: $(FUZZ_BIN)"

fuzz-test: fuzz-build
	@echo "Starting AFL fuzzer (Press Ctrl+C to stop)..."
	afl-fuzz -i build/fuzz/input -o build/fuzz/output -- $(FUZZ_BIN) @@

fuzz-test-ci: fuzz-build
	@echo "Running fuzzer for 5 minutes (CI mode)..."
	timeout 300 afl-fuzz -i build/fuzz/input -o build/fuzz/output -V 300 -- $(FUZZ_BIN) @@ || true
	@if [ -d "build/fuzz/output/default/crashes" ] && [ "$$(ls -A build/fuzz/output/default/crashes)" ]; then \
		echo "Crashes found!"; \
		ls -la build/fuzz/output/default/crashes; \
		exit 1; \
	fi

