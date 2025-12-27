# Integration Test Runbook (Quick)

## Run
```bash
make test                      # build + run all
make tests                     # build only
./build/test_integration_comprehensive   # 41-case suite
./build/test_roundtrip_extended          # round-trip focus
./build/test_integration                 # legacy lexer+parser
./build/test_scope                       # scope tests
./build/test_expressions                 # expressions
./build/test_statements                  # statements
```

## What Is Covered
- Interfaces across lexer/parser/scope/edit/codegen.
- Snapshot-backed round-trip cases.
- Error handling: recovery, empty source, null options.

## Snapshots
- Location: `test/snapshots/`
- Update: `SNAPSHOT_UPDATE=1 make test`

## Add a New Integration Test
1) Write a test in `test/test_integration_comprehensive.c`.
2) Register with `run_test_case` in main.
3) For snapshot tests, add a baseline in `test/snapshots/`.
4) Build and run: `make test_integration_comprehensive && ./build/test_integration_comprehensive`.

## Helpful APIs
- Assertions/snapshots: `test/test_framework_extended.h`
- Mocks: `test/mock_modules.{h,c}`
- Interfaces: `include/quickjsflow/module_interfaces.h`
