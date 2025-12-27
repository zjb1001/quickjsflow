# CLI Quick Reference

## Install
```bash
make clean && make
```

## Commands
| Command | Purpose | Example |
|---------|---------|---------|
| `lex` | Tokenize JavaScript | `./build/quickjsflow lex file.js` |
| `parse` | Emit AST (ESTree JSON) | `./build/quickjsflow parse file.js` |
| `generate` | Generate code from AST | `./build/quickjsflow generate file.js` |
| `check` | Syntax + scope checks | `./build/quickjsflow check file.js` |
| `cfg` | Control-flow graph output | `./build/quickjsflow cfg file.js [json|dot|mermaid]` |
| `run` | Apply plugin transform | `./build/quickjsflow run file.js --plugin <name>` |

## Built-in Plugins
| Plugin | Description | Usage |
|--------|-------------|-------|
| `remove-console` | Strip `console.log` | `--plugin remove-console` |
| `remove-debugger` | Strip `debugger` | `--plugin remove-debugger` |

## Examples
```bash
# Remove debug code
./build/quickjsflow run examples/with_console.js --plugin remove-console > clean.js
./build/quickjsflow run examples/with_debugger.js --plugin remove-debugger > clean.js

# Parse and check
./build/quickjsflow parse examples/sample.js > ast.json
./build/quickjsflow check examples/bad.js

# Control flow
./build/quickjsflow cfg examples/fib.js           # JSON
./build/quickjsflow cfg examples/fib.js dot | dot -Tpng > cfg.png
./build/quickjsflow cfg examples/fib.js mermaid   # Mermaid
```

## Exit Codes
- 0 success
- 1 usage error / command missing
- 2 file read error

## Pointers
- Plugins: `include/quickjsflow/plugin.h`
- Examples: `examples/`
