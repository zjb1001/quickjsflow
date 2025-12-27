# CI Status Badges

Add these to your main README.md:

```markdown
# QuickJSFlow

[![CI](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/quickjsflow/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/quickjsflow)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
```

## Setting Up Codecov

1. Visit https://codecov.io/
2. Sign in with your GitHub account
3. Add your repository
4. Get your upload token (if repository is private)
5. Add token to GitHub Secrets:
   - Go to repository Settings → Secrets → Actions
   - Add new secret: `CODECOV_TOKEN`

## Customizing Badges

Replace `YOUR_USERNAME` with your GitHub username or organization name.

## Additional Badges

You can add more badges for:
- Version/Release
- Downloads
- Contributors
- Last Commit
- Language Stats

Visit https://shields.io/ for more badge options.
