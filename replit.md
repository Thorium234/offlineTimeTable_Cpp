# Timetable Generator

A C++17 constraint-based school timetable scheduling application with both a GUI (Qt5) and CLI mode.

## Overview

Automates generation of conflict-free school schedules satisfying:
- No class double-bookings in the same time slot
- No teacher double-bookings in the same time slot
- Required weekly periods per lesson are met

## Build

The project uses CMake. Build artifacts go into `build_cmake/`:

```bash
mkdir -p build_cmake && cd build_cmake && cmake .. && make -j$(nproc)
```

## Run

```bash
bash run.sh
```

This sets the Qt plugin path for SQLite and runs the app in CLI mode (Replit is headless — no display server).

## Architecture

- `main.cpp` — Entry point; auto-detects headless env and falls back to CLI mode
- `models/` — Domain objects (Teacher, Subject, SchoolClass, Room, Lesson)
- `services/` — Core engine (BacktrackingSolver, GreedySolver, TimetableEngine, SQLiteService, etc.)
- `gui/` — Qt5 Widgets GUI (MainWindow, dialogs, ribbon, sidebar, timetable view)
- `timetable/` — Grid model and display
- `utils/` — CLI menu and path utilities
- `data/` — SQLite database file
- `tests/` — Unit/integration tests (test_runner)

## Dependencies

Installed via Nix:
- `cmake` — build system
- `pkg-config` — library detection
- `qt5.qtbase` — Qt5 Widgets + Sql
- `qt5.qttools` — Qt tools
- `qt5.full` — includes the QSQLITE driver plugin

## Notes

- The QSQLITE driver is in `qt-full`'s plugin directory; `run.sh` sets `QT_PLUGIN_PATH` at runtime.
- In headless Replit, the app auto-switches to CLI mode (no `DISPLAY`/`WAYLAND_DISPLAY`).
- Build output is in `build_cmake/` (not the repo's `build/` directory).

## User Preferences

(None yet)
