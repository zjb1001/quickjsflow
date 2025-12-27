# QuickJSFlow CLI Quick Reference

## Installation

```bash
make clean && make
```

## Basic Commands

| Command | Description | Example |
|---------|-------------|---------|
| `lex` | Tokenize JavaScript | `./build/quickjsflow lex file.js` |
| `parse` | Generate AST | `./build/quickjsflow parse file.js` |
| `generate` | Generate code | `./build/quickjsflow generate file.js` |
| `check` | Syntax & scope check | `./build/quickjsflow check file.js` |
| `cfg` | Control flow graph | `./build/quickjsflow cfg file.js [json\|dot\|mermaid]` |
| `run` | Transform with plugin | `./build/quickjsflow run file.js --plugin <name>` |

## Built-in Plugins

| Plugin | Description | Usage |
|--------|-------------|-------|
| `remove-console` | Remove console.log() | `--plugin remove-console` |
| `remove-debugger` | Remove debugger | `--plugin remove-debugger` |

## Quick Examples

### Remove Debug Code
```bash
# Remove console.log statements
./build/quickjsflow run examples/with_console.js --plugin remove-console > clean.js

# Remove debugger statements
./build/quickjsflow run examples/with_debugger.js --plugin remove-debugger > clean.js
```

### Parse and Analyze
```bash
# Get AST
./build/quickjsflow parse examples/sample.js > ast.json

# Check for errors
./build/quickjsflow check examples/bad.js
```

### Control Flow Analysis
```bash
# JSON format (default)
./build/quickjsflow cfg examples/fib.js

# Graphviz DOT
./build/quickjsflow cfg examples/fib.js dot | dot -Tpng > cfg.png

# Mermaid diagram
./build/quickjsflow cfg examples/fib.js mermaid
```

## Exit Codes

- `0` - Success
- `1` - Usage error or command not found
- `2` - File read error

## See Also

- Full documentation: `docs/CLI_PLUGIN_GUIDE.md`
- Plugin development: `include/quickjsflow/plugin.h`
- Examples: `examples/`
