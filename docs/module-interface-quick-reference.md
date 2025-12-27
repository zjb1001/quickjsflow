# Module Interface Quick Reference

## Purpose
Contracts between modules to keep Lexer → Parser → Scope → Edit → Codegen stable and testable.

## Interfaces
- **Lexer → Parser (TokenStream)**
  - Tokens carry type, lexeme, start/end positions, error flag.
  - Stability: token enums are backward compatible.
  - Validate: token count/positions/lexeme accuracy tests.

- **Parser → ScopeManager (AST Structure)**
  - `AstNode { type, start, end, refcount, data }` with ESTree-compatible types.
  - Invariants: valid enum, start <= end, child spans inside parent, data matches type.
  - Validate with `ast_verify()` / `ast_verify_tree()`.

- **ScopeManager → Edit API (Scope Query)**
  - Lookups: `scope_lookup_local`, `scope_resolve`, `scope_of_node`.
  - Behavior: chain resolution, shadowing awareness, bindings for all decls, TDZ for let/const.
  - Tests: lookup, chain traversal, shadowing, TDZ detection.

- **Edit API → Codegen (Immutability)**
  - Planned edits: replace/remove/insert return new AST copies; refcount manages sharing.
  - Positions stay consistent; original AST unchanged.
  - Tests: replacements do not mutate originals; refcounts updated.

- **Codegen Interface**
  - `codegen_generate` returns deterministic code (and optional source map).
  - Preserve identifiers and literals; source maps must be valid JSON.
  - Tests: deterministic output, identifier preservation, sourcemap validity.

- **End-to-End Round-Trip**
  - Flow: source → tokens → AST1 → code → AST2; AST1 ≈ AST2.
  - Checks: node types/counts, identifiers, literals.

## Versioning
- Interfaces expose version helpers (e.g., `ast_get_version`) to guard compatibility.

## Files
- Definitions: `include/quickjsflow/module_interfaces.h`
- Tests: `test/test_integration_comprehensive.c`, `test/test_roundtrip_extended.c`
- Mocks: `test/mock_modules.{h,c}`
