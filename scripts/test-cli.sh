#!/bin/bash
# Test script for QuickJSFlow CLI and Plugin System

set -e

echo "================================================"
echo "QuickJSFlow CLI & Plugin System Test Suite"
echo "================================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

CLI="./build/quickjsflow"

if [ ! -f "$CLI" ]; then
    echo "Error: quickjsflow binary not found. Run 'make' first."
    exit 1
fi

test_command() {
    local name=$1
    shift
    echo -e "${BLUE}Testing:${NC} $name"
    echo -e "${YELLOW}Command:${NC} $*"
    if "$@"; then
        echo -e "${GREEN}✓ PASSED${NC}"
    else
        echo -e "\033[0;31m✗ FAILED${NC}"
        exit 1
    fi
    echo ""
}

echo "=== 1. Lexer Test ==="
test_command "Tokenize sample.js" \
    bash -c "$CLI lex examples/sample.js | head -5"

echo "=== 2. Parser Test ==="
test_command "Parse and generate AST" \
    bash -c "$CLI parse examples/sample.js | head -20"

echo "=== 3. Code Generation Test ==="
test_command "Generate code from AST" \
    bash -c "$CLI generate examples/sample.js | head -10"

echo "=== 4. Syntax Check Test ==="
test_command "Check valid file" \
    bash -c "$CLI check examples/sample.js"

echo "=== 5. Plugin Test: Remove Console ==="
echo "Input file (examples/with_console.js):"
echo "---"
head -10 examples/with_console.js
echo "---"
echo ""
echo "After applying remove-console plugin:"
echo "---"
test_command "Remove console.log statements" \
    bash -c "$CLI run examples/with_console.js --plugin remove-console | head -15"
echo "---"

echo "=== 6. Plugin Test: Remove Debugger ==="
echo "Input file (examples/with_debugger.js):"
echo "---"
head -10 examples/with_debugger.js
echo "---"
echo ""
echo "After applying remove-debugger plugin:"
echo "---"
test_command "Remove debugger statements" \
    bash -c "$CLI run examples/with_debugger.js --plugin remove-debugger | head -15"
echo "---"

echo "=== 7. CFG Test ==="
test_command "Generate CFG in JSON format" \
    bash -c "$CLI cfg examples/fib.js | head -20"

echo "=== 8. Help Test ==="
test_command "Display help" \
    bash -c "$CLI 2>&1 | grep -q 'Usage'"

echo ""
echo "================================================"
echo -e "${GREEN}All CLI tests passed!${NC}"
echo "================================================"
echo ""
echo "Additional manual tests you can run:"
echo "  1. $CLI cfg examples/fib.js dot > cfg.dot"
echo "  2. $CLI cfg examples/fib.js mermaid > cfg.mmd"
echo "  3. $CLI run examples/with_console.js --plugin remove-console > clean.js"
echo "  4. $CLI check examples/bad.js  # Should show errors"
echo ""
