# Agent Development Guide — offlineTimeTableCpp

## Project Structure

```
timetable/         — Core timetable grid model (Timetable.h/.cpp)
models/            — Domain structs (Teacher, Subject, SchoolClass, Room, Lesson, FixedEvent, etc.)
services/          — Business logic (solvers, evaluators, DB, export, analytics)
gui/               — Qt5 GUI widgets (MainWindow, dashboard, ribbon, sidebar, dialogs)
web/               — Flask web server (app.py, static/, templates/)
tests/             — Unit tests (test_runner.cpp)
utils/             — Utility helpers (PathUtil.h)
data/              — SQLite database (gitignored)
```

## Build Commands

| Command            | Purpose                               |
|--------------------|---------------------------------------|
| `make`             | Release build                         |
| `make BUILD=debug` | Debug build with ASan+UBSan           |
| `make test`        | Build and run tests                   |
| `make clean`       | Remove build artifacts                |
| `make format`      | Run clang-format on all source files  |
| `cmake -B build_cmake -S . && cmake --build build_cmake -j` | CMake build |
| `./build_cmake/test_runner` | Run test binary directly    |

## Code Conventions

- **Naming:** Classes `PascalCase`, functions `camelBack`, variables `camelBack`, constants `UPPER_CASE`.
- **Headers:** `#pragma once`, no include guards.
- **Types:** Prefer `int` for IDs; use `std::string` for text; use `std::vector` for collections.
- **No raw loops** over vectors for lookups — use `std::unordered_map` when adding new lookup-heavy code.
- **Qt:** Use `QString::fromStdString()`/`.toStdString()` at boundary only; keep domain code Qt-free.

## Testing

Tests live in `tests/test_runner.cpp`. Each test function is registered manually in `main()`.
To add a test:
1. Write a `void runMyFeatureTest()` function.
2. Call it from `main()` inside the `try` block.

## Common Pitfalls

- `DataManager` is a god object — don't grow it further; create focused repositories.
- Days/periods are in-memory only (never persisted to SQLite).
- The C++ solver and Flask server share a SQLite DB — ensure `FLASK_DB_PATH` is consistent.
- Fixed events use `RecurrenceType` enum; the 1-based day/period IDs must match the DB.
