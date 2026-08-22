# Contributing to HolyScreen

## Workflow

1. Create a short-lived branch from `main`.
2. Confirm the baseline before changing code.
3. Work in baby steps with TDD: assertion failure, minimal implementation, green, and refactoring.
4. Do not mix unrelated changes.
5. Describe the behavior, risks, and test evidence in the pull request.

## Required verification

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
git diff --check
```

Visual changes also require validation in the real application. Output changes must cover the operator display, external displays, hot-plugging, and mixed DPI when applicable.

## Architecture

- The UI sends commands; it does not contain domain rules.
- The application layer orchestrates use cases.
- The domain does not depend on QML, SQLite, or hardware.
- Adapters implement networking, persistence, files, audio, video, and displays.
- Copyrighted Bible content must never be embedded in the repository.

By contributing, you agree that your contribution will be distributed under GPLv3.
