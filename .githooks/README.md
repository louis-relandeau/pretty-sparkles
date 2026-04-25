Pre-commit hooks (clang-format)
--------------------------------

This directory contains a `pre-commit` hook that autoformats staged C/C++ files using `clang-format`.

Enable the hook for this repository with:

```sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-commit
```

Notes:
- Ensure `clang-format` is installed and available on `PATH`.
- The hook uses `--style=file`, so place a `.clang-format` at the repository root to customize style.
- The hook formats only staged files with C/C++ extensions and re-adds them to the index.
