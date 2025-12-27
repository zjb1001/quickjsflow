# CI Badges Quick Reference

## Scope
- Add CI, coverage, and license badges to the top of README.md.
- Replace `YOUR_USERNAME` with the GitHub owner or org.

## Badge Snippet (copy/paste)
```markdown
[![CI](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/quickjsflow/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/quickjsflow)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
```

## Codecov Setup (GitHub)
1) Sign in at https://codecov.io/ and add the repo.
2) If private, grab the upload token.
3) In GitHub: Settings → Secrets → Actions → New secret `CODECOV_TOKEN` (only if private).
4) Ensure CI uploads coverage (e.g., `bash <(curl -s https://codecov.io/bash)` or official action).

## Optional Badges
- Release/version, downloads, contributors, last commit, language stats.
- See https://shields.io/ for options.

## Checklist
- Badges render in README.
- Owner/org name replaced.
- Coverage upload succeeds in CI.
