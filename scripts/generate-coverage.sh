#!/bin/bash
# Generate coverage report script

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COVERAGE_DIR="$PROJECT_ROOT/build/coverage"

echo "========================================="
echo "QuickJSFlow Coverage Report Generator"
echo "========================================="

# Check for lcov
if ! command -v lcov >/dev/null 2>&1; then
    echo "Error: lcov not found. Install with:"
    echo "  Ubuntu/Debian: sudo apt-get install lcov"
    echo "  macOS: brew install lcov"
    exit 1
fi

# Build with coverage
echo -e "\n[1/4] Building with coverage instrumentation..."
make coverage

# Run tests
echo -e "\n[2/4] Running tests..."
make test-coverage

# Generate report
echo -e "\n[3/4] Generating coverage data..."
cd "$COVERAGE_DIR"

# Capture coverage
lcov --capture --directory . --output-file coverage.info

# Remove system files and tests
lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info

# Generate HTML
genhtml coverage.info --output-directory html

# Calculate coverage percentage
COVERAGE=$(lcov --summary coverage.info 2>&1 | grep "lines" | awk '{print $2}')

echo -e "\n[4/4] Coverage report generated!"
echo "========================================="
echo "Coverage: $COVERAGE"
echo "HTML Report: $COVERAGE_DIR/html/index.html"
echo "========================================="

# Open report if on macOS or Linux with xdg-open
if command -v open >/dev/null 2>&1; then
    echo -e "\nOpening report in browser..."
    open "$COVERAGE_DIR/html/index.html"
elif command -v xdg-open >/dev/null 2>&1; then
    echo -e "\nOpening report in browser..."
    xdg-open "$COVERAGE_DIR/html/index.html"
else
    echo -e "\nTo view the report, open: file://$COVERAGE_DIR/html/index.html"
fi
