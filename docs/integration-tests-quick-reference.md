# Integration Tests Quick Reference

## Scope
- End-to-end coverage of Lexer → Parser → Scope → Edit API → Codegen.
- Interfaces tracked in `include/quickjsflow/module_interfaces.h`.

## Core Components
- Interfaces: TokenStream, AST integrity, scope queries, edit immutability (planned), codegen contracts.
- Test framework: `test/test_framework_extended.h` (assertions, snapshots, AST compare).
- Mocks: `test/mock_modules.{h,c}` for lexer/parser/scope/codegen isolation.
- Snapshots: `test/snapshots/*.snapshot` for round-trip baselines.

## Suites (41 cases)
1) Lexer → Parser interface (token stream, consumption, round-trip)
2) Parser → Scope (positions, structure, declarations)
3) Scope → Edit (global scope, lookup, chain resolution)
4) Edit → Codegen (deterministic output, identifier preservation)
5) Full pipeline round-trip (simple, multi-stmt, string literal, detailed)
6) AST comparison (identical/different/program)
7) Error handling (recovery, empty source, null options)

## Commands
```bash
make test                          # build + run all
./build/test_integration_comprehensive   # run comprehensive suite
./build/test_roundtrip_extended          # round-trip framework
```

## Pass Criteria
- 41/41 integration cases green.
- Interfaces validated per contracts.
- Snapshots up to date (set `SNAPSHOT_UPDATE=1` to refresh).

## References
- Contracts: `docs/module-interface-quick-reference.md`
- Runbook: `docs/integration-test-runbook-quick-reference.md`
