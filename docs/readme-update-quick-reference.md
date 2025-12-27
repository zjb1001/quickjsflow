# README Update Quick Reference

## Goals
- Add testing/quality and performance sections to the root README.
- Include CI/coverage badges with the correct owner/org.

## Testing & Quality Block
```markdown
## Testing and Quality
make test              # run all tests
make coverage-report   # coverage report
make run-benchmark     # performance benchmarks
make fuzz-test-ci      # quick fuzz run
```
- Link to detailed docs if present (e.g., testing quickstart, quality guide).

## CI Badges
Use badges from `docs/ci-badges-quick-reference.md` and replace `YOUR_USERNAME`.

## Quality Targets
- Coverage >= 80% overall; core modules >= 90%.
- CI turnaround < 5 minutes; long fuzzing stable for 24h.

## Performance Section
- Show sample benchmark table (lexer/parser/pipeline) if available.
- Point to `make run-benchmark`.

## Dependencies Block
Add platform-specific deps if needed:
- Ubuntu/Debian: `sudo apt-get install build-essential lcov valgrind afl++`
- macOS: `brew install lcov afl++`

## Checklist
- New sections render near the top of README.
- Badges point to the correct repo.
- Commands match Makefile targets.
