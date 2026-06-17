# Source Hygiene — foliage_un06

**Date:** 2026-06-17
**Phase:** 1 (completed)

## What Was Found

### Build artifacts in workspace root
Before cleanup: 13 `.obj` files, `core_entry.lib`, `core_entry.exp` at `C:\mont-ed-plg\foliage_un06\`.
After cleanup: removed. `.gitignore` updated to cover `*.lib`, `*.exp`, `*.ilk`, `*_export.json`, `reports/`.

### .gitignore status
Updated from basic (7 patterns) to comprehensive (22 patterns):
- `build/`, `bin/`, `out/`
- `*.obj`, `*.pdb`, `*.dll`, `*.exe`, `*.lib`, `*.exp`, `*.ilk`, `*.log`
- `*_export.json`, `reports/`, `__pycache__/`, etc.

## What Can Be Committed
- `src/**/*.h`, `src/**/*.cpp`
- `plugin.monted.json`
- `build.ps1`, `test.ps1`, `verify_build.ps1`, `build_verify.ps1`
- `*.md` docs
- `schemas/` (once created)
- `demo/payloads/` (once created)

## What Cannot Be Committed
- `build/` directory contents (generated)
- Any `.obj`, `.lib`, `.exp`, `.dll`, `.exe`, `.pdb`
- Temporary export/import files like `test_export.json`
- `reports/` directory
- IDE config `.vs/`

## Encoding
- Source files: UTF-8 without BOM (default MSVC `cl` behavior)
- Documentation: UTF-8 (Russian comments acceptable in docs, not in C++ identifiers)
- Line endings: CRLF (Windows default for MSVC)

## Git Readiness
Workspace is ready for `git init` and push to separate GitHub repo. No engine source dependencies.
