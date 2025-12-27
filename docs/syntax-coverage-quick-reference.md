# Syntax Coverage Quick Reference

## Coverage Snapshot (Issue 12)
- Phase 1 (Essential): ~85% implemented. Gaps: boolean/null/undefined/regexp literals; import default/namespace forms.
- Phase 2 (Modern): ~80% implemented. Gaps: async/await parsing, verify object shorthand.
- Phase 3+ (Advanced): 0% (planned).

## High-Priority Fixes
1) Literals: add boolean/null/undefined/regexp kinds; extend lexer/parser; add tests.
2) Imports: support default and namespace specifiers; mixed default+named imports.

## Phase 2 Follow-ups
- Implement async functions and await expressions.
- Confirm object shorthand `{ x }` and method shorthand `x() {}` parsing.

## Phase 3 Roadmap (suggested order)
- Optional chaining `?.` and nullish coalescing `??`.
- Generator functions `function*` and `yield`.
- Logical assignment `&&= ||= ??=` and dynamic `import()`.
- Later: BigInt literals, other built-ins.

## Testing Additions (examples)
- `test/test_literals_complete.c` for missing literals.
- `test/test_import_complete.c` for import forms.
- `test/test_async_await.c`, `test/test_object_shorthand.c`.
- Future: optional chaining/nullish coalescing/generator suites.

## References
- Source matrix: `issues/12-ESTree-AST语法完整性矩阵.md`
- Interfaces: `include/quickjsflow/ast.h`, parser/lexer/codegen sources.
