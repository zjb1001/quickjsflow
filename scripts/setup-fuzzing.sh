#!/bin/bash
# Fuzzing script for QuickJSFlow

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZ_DIR="$PROJECT_ROOT/build/fuzz"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "QuickJSFlow Fuzzing Setup"
echo "========================================="

# Create fuzz directories
mkdir -p "$FUZZ_DIR"/{input,output,crashes,hangs}

# Generate seed corpus
echo -e "\n${YELLOW}Generating seed corpus...${NC}"
cat > "$FUZZ_DIR/input/test1.js" << 'EOF'
function test() { return 42; }
EOF

cat > "$FUZZ_DIR/input/test2.js" << 'EOF'
const x = 10;
let y = 20;
var z = x + y;
EOF

cat > "$FUZZ_DIR/input/test3.js" << 'EOF'
class MyClass {
  constructor(x) {
    this.x = x;
  }
  method() {
    return this.x * 2;
  }
}
EOF

cat > "$FUZZ_DIR/input/test4.js" << 'EOF'
if (true) {
  console.log("yes");
} else {
  console.log("no");
}
EOF

cat > "$FUZZ_DIR/input/test5.js" << 'EOF'
for (let i = 0; i < 10; i++) {
  console.log(i);
}
EOF

# Copy examples as seed corpus
if [ -d "$PROJECT_ROOT/examples" ]; then
    echo "Copying examples to seed corpus..."
    cp "$PROJECT_ROOT/examples"/*.js "$FUZZ_DIR/input/" 2>/dev/null || true
fi

echo -e "${GREEN}Seed corpus generated in $FUZZ_DIR/input${NC}"
echo "Files:"
ls -lh "$FUZZ_DIR/input"

# Check for AFL++
if command -v afl-fuzz >/dev/null 2>&1; then
    echo -e "\n${GREEN}✓ AFL++ is installed${NC}"
    AFL_VERSION=$(afl-fuzz --version 2>&1 | head -n1 || echo "Unknown")
    echo "  Version: $AFL_VERSION"
else
    echo -e "\n${YELLOW}⚠ AFL++ not found${NC}"
    echo "Install with: sudo apt-get install afl++ (Ubuntu/Debian)"
    echo "           or: brew install afl++ (macOS)"
fi

echo -e "\n${GREEN}Fuzzing setup complete!${NC}"
echo -e "\nTo start fuzzing:"
echo -e "  1. Build fuzzer:    ${YELLOW}make fuzz-build${NC}"
echo -e "  2. Run fuzzer:      ${YELLOW}make fuzz-test${NC}"
echo -e "  3. Or manually:     ${YELLOW}afl-fuzz -i $FUZZ_DIR/input -o $FUZZ_DIR/output -- $FUZZ_DIR/fuzz_target @@${NC}"
