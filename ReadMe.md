# Thorium234 — School Timetable Generator

A **dual-stack** school timetable generator combining a **Qt5 C++ GUI** with a **Flask + Vanilla JS** web interface, sharing an **SQLite** database. Includes two independent solver pipelines — a high-performance C++ backtracking/MRV solver for uniform period grids, and a Python greedy solver with **per-class dynamic timelines** for schools that need different lesson durations across streams.

---

## Features

- **Two solver engines**: C++ BacktrackingSolver (MRV + LCV + domain propagation) and GreedySolver; Python GreedySolver with dynamic timelines
- **Per-class lesson durations**: Form 4 can use 60-minute periods while Form 1 uses 40-minute periods, all under the same break schedule
- **Configurable time blocks**: Define assemblies, morning tea, lunch, games — any break type, any duration, per day of the week
- **Full constraint set**: `maxPerDay`, `blockSize` (double periods), `secondTeacherId`, week types (A/B/Every), teacher max consecutive, class period windows, room capacity/type
- **Fixed events**: Lock specific (day, period) slots for recurring events (lunch, assembly)
- **Teacher preferences**: Mark slots as BLOCKED/PREFERRED/UNDESIRABLE per teacher
- **Substitutions**: Request, suggest, approve, and track substitute teacher assignments
- **Divisions**: Group classes for parallel scheduling
- **Versioning**: Save, compare, and restore timetable solutions (aSc-style)
- **Drag & drop**: Manual slot adjustment with undo/redo and conflict checking
- **Export**: Professional aSc-style PDF (ReportLab), HTML, CSV
- **Analytics**: Teacher load distribution, room utilization, gap statistics
- **Flask web API**: 60+ REST endpoints — full CRUD, generate, export, analytics
- **Custom Timetable modal**: Load sample Kenyan school data, adjust breaks & durations, generate PDF in one click
- **Docker support**: Containerized deployment

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                    C++ Binary (timetableGen)                        │
│                                                                     │
│  main.cpp ──→ QApplication ──→ MainWindow ──→ Qt5 GUI              │
│      │                         (16 widget classes)                  │
│      ├──→ startFlaskWebServer() ──→ python3 web/app.py             │
│      │                            (port 5000-5009)                  │
│      └──→ --solve-from-db ──→ DataManager ──→ TimetableEngine      │
│                                                ├── BacktrackingSolver│
│                                                └── GreedySolver     │
│                                                                     │
│  DataManager ──→ SQLiteService ──→ timetable.db                    │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                     Flask Server (web/app.py)                       │
│                                                                     │
│  60+ API endpoints: CRUD, generate, export, analytics, versions    │
│                                                                     │
│  /api/generate (POST) ──→ C++ binary --solve-from-db (subprocess)  │
│  /api/generate-timetable (POST) ──→ Python greedy solver + PDF     │
│  /api/export/pdf (GET) ──→ HTML→pdfkit or ReportLab                │
│                                                                     │
│  timeline_gen.py ──→ per-class dynamic timelines + greedy solver   │
│  pdf_generator.py ──→ ReportLab aSc-style PDF (direct canvas)      │
│                                                                     │
│  templates/index.html ──→ SPA shell                                 │
│  static/js/app.js    ──→ 1665 lines vanilla JS                      │
│  static/css/asc.css  ──→ 290 lines                                  │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
offlineTimeTableCpp/
├── main.cpp                 # Entry point — GUI mode or --solve-from-db
├── Makefile                 # GNU Make build
├── CMakeLists.txt           # CMake build (alternative)
├── pyproject.toml           # Python dependencies
├── AGENTS.md                # Developer guide
│
├── models/                  # Domain structs (17 headers)
│   ├── Teacher.h, Subject.h, SchoolClass.h, Room.h, Lesson.h
│   ├── FixedEvent.h, TimeBlock.h, LessonUnit.h
│   ├── Day.h, Period.h, RoomType.h, SubjectRequirement.h
│   ├── Division.h, Substitution.h
│   └── TeacherConstraint.h, TeacherPreference.h, ConstraintViolation.h
│
├── services/                # Business logic (19 .cpp/.h pairs)
│   ├── DataManager.h/.cpp         # Central data hub + CRUD
│   ├── SQLiteService.h/.cpp       # All database operations
│   ├── TimetableEngine.h/.cpp     # Solver orchestrator
│   ├── BacktrackingSolver.h/.cpp  # MRV + LCV + domain propagation
│   ├── GreedySolver.h/.cpp        # Heuristic fallback
│   ├── FeasibilityChecker.h/.cpp  # Pre-solve validation
│   ├── ConflictChecker.h/.cpp     # Post-placement checks
│   ├── TimetableEvaluator.h/.cpp  # Score computation
│   ├── AnalyticsService.h/.cpp    # Statistics
│   ├── ExportService.h/.cpp       # CSV / HTML export
│   ├── PdfReportService.h/.cpp    # Qt5 PDF export
│   ├── TimelineGenerator.h/.cpp   # C++ dynamic timeline
│   └── ...                        # Benchmark, UndoRedo, ConstraintExplain
│
├── timetable/               # Grid model
│   └── Timetable.h/.cpp     # TimetableCell, 2D grid storage
│
├── gui/                     # Qt5 GUI widgets (16 files)
│   ├── MainWindow.cpp
│   ├── ribbon/RibbonToolbar.cpp
│   ├── sidebar/DataSidebar.cpp
│   ├── timetableview/{TimetableViewWidget,TimetableScene,LessonCardItem}.cpp
│   ├── dashboard/DashboardWidget.cpp
│   ├── teachers/TeacherDialog.cpp, subjects/SubjectDialog.cpp
│   ├── classes/ClassDialog.cpp, rooms/RoomDialog.cpp
│   ├── substitutions/SubstitutionWidget.cpp
│   ├── divisions/DivisionWidget.cpp
│   └── constraints/ConstraintRelaxationDialog.cpp
│
├── web/                     # Python web layer
│   ├── app.py               # Flask server — 60 API endpoints
│   ├── timetable_gen.py     # Dynamic timeline + greedy solver
│   ├── pdf_generator.py     # ReportLab aSc-style PDF
│   ├── static/js/app.js     # SPA frontend (1665 lines)
│   ├── static/css/asc.css   # Styles (290 lines)
│   └── templates/index.html # SPA shell
│
├── tests/                   # Test suites
│   ├── test_runner.cpp      # 12 C++ tests (315 lines)
│   ├── test_api.py          # 17 Flask integration tests
│   └── test_timetable_gen.py# 24 Python unit tests
│
├── utils/PathUtil.h         # Path resolution
├── data/                    # SQLite database (gitignored)
├── docs/                    # Documentation
│   └── SYSTEM_REVIEW.md     # Full system review
├── Dockerfile               # Container build
└── run.sh                   # Convenience launcher
```

---

## Dependencies

### C++
- **Compiler**: GCC (g++) with C++17 support
- **Qt5**: Widgets, Sql, PrintSupport modules
- **pkg-config** (for Qt discovery)

Install on Ubuntu/Debian:
```bash
sudo apt install g++ make pkg-config qtbase5-dev libqt5sql5-sqlite libqt5printsupport5
```

### Python
```bash
pip install flask flask-cors       # Required
pip install reportlab              # Optional — for aSc-style PDF export
pip install pdfkit                 # Optional — for HTML→PDF export (requires wkhtmltopdf)
```

---

## Build & Run

### Make (recommended)

```bash
make              # Release build (./timetableGen)
make BUILD=debug  # Debug build with sanitizers
make test         # Build and run C++ tests
make clean        # Remove build artifacts
make format       # Run clang-format on all source files
```

### CMake

```bash
cmake -B build_cmake -S .
cmake --build build_cmake -j
./build_cmake/timetableGen
```

### Run Tests

```bash
# C++ tests
make test
# or
./build_cmake/test_runner

# Python unit tests
python3 -m pytest tests/test_timetable_gen.py -v

# Flask integration tests
python3 tests/test_api.py
```

---

## Usage

### GUI Mode

```bash
./timetableGen
```

Launches the Qt5 desktop application and automatically starts the Flask web server on `http://127.0.0.1:5000`.

### Headless Solver Mode

```bash
./timetableGen --solve-from-db
```

Loads timetable data from the SQLite database, runs the C++ solver, and writes a JSON result to stdout. Used by the Flask `/api/generate` endpoint.

### Web Interface Only

```bash
python3 web/app.py
```

Starts the Flask server without the C++ GUI. The web interface provides full CRUD, generation, export, and analytics.

### Custom Timetable (Dynamic Config)

1. Click **Custom TT** in the Timetable tab
2. Click **Load Sample School** to load realistic Kenyan school data (Alliance High School)
3. Adjust day start/end, break times, per-class lesson durations
4. Click **Generate Timetable** → downloads a professional aSc-style PDF

---

## API Overview (Selected Endpoints)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/teachers`, `/api/subjects`, `/api/classes`, `/api/rooms` | GET/POST/PUT/DELETE | Entity CRUD |
| `/api/lessons` | GET/POST/PUT/DELETE | Lesson CRUD with combined classes |
| `/api/generate` | POST | Run C++ solver, return JSON result |
| `/api/generate-timetable` | POST | Run Python dynamic solver, return PDF |
| `/api/sample-school` | GET | Sample Kenyan school configuration |
| `/api/analytics` | GET | Teacher load, room utilization, gap stats |
| `/api/export/html` | GET | Full HTML timetable document |
| `/api/export/pdf` | GET | PDF timetable (via pdfkit) |
| `/api/conflicts` | GET | Current conflict list |
| `/api/substitutions` | GET/POST | Substitution lifecycle |
| `/api/divisions` | GET/POST/PUT/DELETE | Division CRUD |
| `/api/versions` | GET/POST | Save/restore/compare timetable versions |
| `/api/data/export` | GET | Full JSON backup |
| `/api/data/import` | POST | JSON restore |

See `docs/SYSTEM_REVIEW.md` or `web/app.py` for the complete endpoint listing.

---

## Solvers

| Solver | Language | Algorithm | Grid Model | Use Case |
|--------|----------|-----------|------------|----------|
| BacktrackingSolver | C++ | MRV + LCV + domain propagation | Uniform (5×8) | High-quality solutions, all classes same period duration |
| GreedySolver (C++) | C++ | Weighted slot scoring | Uniform (5×8) | Fast fallback for simple schedules |
| GreedySolver (Python) | Python | First-fit with difficulty ordering | Per-class dynamic | Schools with different lesson durations per stream |

---

## Custom Timetable JSON Schema

```json
{
  "school": { "name": "School Name" },
  "configuration": {
    "day_start": "07:30",
    "day_end": "16:30",
    "default_lesson_duration_minutes": 40
  },
  "time_blocks": [
    { "name": "Assembly", "type": "fixed_break",
      "start": "07:30", "end": "08:00", "days": ["Monday", "Friday"] },
    { "name": "Lunch", "type": "fixed_break",
      "start": "12:30", "end": "13:30", "days": ["All"] }
  ],
  "teachers": [
    { "id": "T1", "name": "Paul Inyangala", "abbreviation": "PI",
      "qualified_subjects": ["MATH"] }
  ],
  "classes": [
    { "id": "F1A", "name": "Form 1 A",
      "lesson_duration_minutes": 40,
      "subject_requirements": [
        { "subject": "MATH", "abbreviation": "MATH",
          "lessons_per_week": 5, "max_per_day": 2, "block_size": 1 }
      ]
    }
  ]
}
```

---

## License

MIT — see `LICENSE.MD`

---

*Built with Qt5, Flask, ReportLab, SQLite, and love for Kenyan schools.*
