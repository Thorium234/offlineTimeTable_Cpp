# System Review — offlineTimeTableCpp

## 1. Project Overview

A dual-stack school timetable generator with a **Qt5 C++ GUI** and a **Flask+JS web interface**, sharing an **SQLite** database. The C++ backend provides a backtracking/MRV solver and a greedy fallback; the Python web layer adds a separate dynamic-timeline greedy solver and ReportLab PDF export.

**Purpose:** Generate conflict-free weekly timetables for secondary schools, supporting teachers, subjects, classes, rooms, constraints, fixed events, substitutions, divisions, and versioned solution history.

---

## 2. Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        C++ Binary (timetableGen)                    │
│                                                                     │
│  main.cpp ──→ QApplication ──→ MainWindow ──→ Qt5 GUI Widgets      │
│      │                                    (16 GUI classes)          │
│      ├──→ startFlaskWebServer() ──→ QProcess ──→ python3 web/app.py│
│      │                                    (port 5000-5009)          │
│      └──→ --solve-from-db ──→ DataManager ──→ TimetableEngine      │
│                                                ├── BacktrackingSolver│
│                                                └── GreedySolver     │
│                                                                     │
│  DataManager ──→ SQLiteService ──→ timetable.db                    │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                     Flask Server (web/app.py)                       │
│                                                                     │
│  60+ API endpoints: entity CRUD, generate, export, analytics, etc. │
│                                                                     │
│  /api/generate (POST) ──→ calls C++ binary --solve-from-db          │
│  /api/generate-timetable (POST) ──→ timetable_gen.py (Python solver)│
│  /api/export/pdf (GET) ──→ _build_html() ──→ pdfkit                 │
│                                                                     │
│  timetable_gen.py ──→ per-class dynamic timeline + greedy solver    │
│  pdf_generator.py  ──→ ReportLab aSc-style PDF (direct canvas)     │
│                                                                     │
│  templates/index.html ──→ SPA shell                                  │
│  static/js/app.js    ──→ 1665 lines vanilla JS                      │
│  static/css/asc.css  ──→ 290 lines                                  │
└─────────────────────────────────────────────────────────────────────┘
```

### Data Flow

```
SQLite DB ←→ C++ DataManager ←→ Solver → Timetable → DB
     ↑                                          ↓
     └── Flask API ←→ JS Frontend ←→ User       │
                          │                      │
                     /api/export/html ───────────┘
                     /api/generate-timetable ──→ PDF
```

---

## 3. C++ Backend

### Models (`models/`, 17 headers)

| File | Struct/Class | Key Fields |
|------|-------------|------------|
| `Teacher.h` | `Teacher` | `id, name, maxConsecutive` |
| `Subject.h` | `Subject` | `id, name` |
| `SchoolClass.h` | `SchoolClass` | `id, name, studentCount, earliestPeriod, latestPeriod, divisionId, groupId` |
| `Room.h` | `Room` | `id, name, capacity, roomTypeId` |
| `Lesson.h` | `Lesson` | `id, teacherId, secondTeacherId, subjectId, classId, combinedClassIds, periodsPerWeek, blockSize, maxPerDay, weekType` |
| `LessonUnit.h` | `LessonUnit` | `lessonIndex, teacherId, secondTeacherId, subjectId, classId, combinedClassIds, reqRoomTypeId, studentCount, blockSize, maxPerDay, weekType` |
| `FixedEvent.h` | `FixedEvent`, `RecurrenceType` | `id, dayId, periodId, name, recurrence (NONE/DAILY/WEEKLY)` |
| `TeacherConstraint.h` | `TeacherConstraint` | Teacher availability per slot |
| `TeacherPreference.h` | `TeacherPreference` | `PreferenceType (NEUTRAL, PREFERRED, UNDESIRABLE, BLOCKED)` |
| `TimeBlock.h` | `TimeBlock, BreakDefinition, DayLayout, ScheduleConfig` | Dynamic timeline structures (new) |
| `Day.h`, `Period.h` | `Day`, `Period` | Grid dimensions |
| `RoomType.h` | `RoomType` | Room categorization |
| `SubjectRequirement.h` | `SubjectRequirement` | Room type per subject |
| `Division.h` | `Division` | Class grouping for parallel scheduling |
| `Substitution.h` | `Substitution` | `origTeacherId, subTeacherId, subjectId, classId, dayId, periodId, status, reason, date` |
| `ConstraintViolation.h` | `ConstraintViolation` | Violation reporting |

### Services (`services/`, 19 `.cpp/.h` pairs)

| Service | Role |
|---------|------|
| **DataManager** | God object: CRUD for all entities, loads/saves from SQLite, undo/redo, conflict checking, substitution suggestion |
| **SQLiteService** | All raw SQL operations (schema creation, inserts, updates, deletes, constraints/preferences) |
| **TimetableEngine** | Orchestrator: dispatches to solver strategy, wraps solver options |
| **BacktrackingSolver** | Advanced solver: MRV variable ordering, LCV value ordering, domain propagation, 1.5M state limit |
| **GreedySolver** | Fallback: simpler heuristic-based placement |
| **ResourceTracker** | Tracks teacher/room/subject usage during solver run |
| **DomainTracker** | Domain pruning for backtracking solver |
| **FeasibilityChecker** | Pre-solve checks: teacher overload, subject room type, etc. |
| **ConflictChecker** | Post-placement conflict detection (teacher clash, room clash, etc.) |
| **TimetableEvaluator** | Score computation for timetable quality |
| **AnalyticsService** | Teacher load, room utilization, gap statistics |
| **ExportService** | CSV/HTML export of timetable |
| **PdfReportService** | Qt5-based PDF generation |
| **ConstraintExplanationService** | Human-readable rejection logging |
| **ConstraintExplain** | Constraint violation messages |
| **Benchmark** | Solver performance benchmarking |
| **UndoRedoStack** | Manual move undo/redo |
| **TimelineGenerator** | C++ version of dynamic timeline generation (new) |

### Timetable (`timetable/`)

| File | Purpose |
|------|---------|
| `Timetable.h` | `TimetableCell { subjectId, teacherId, roomId, locked, weekType }`, `Timetable { schedules (map<classId → 2D cell grid>), unscheduledLessons, score }` |
| `Timetable.cpp` | Slot manipulation, init, move, swap, lock, print |

### Entry Point (`main.cpp`, 224 lines)

Two modes:
1. **GUI mode** (default): Starts Flask as child process, shows splash screen, opens `MainWindow`
2. **`--solve-from-db`**: Headless solver — loads DB, runs `TimetableEngine::generate()`, writes JSON result to stdout

---

## 4. Python Web Layer

### Dependencies

- `flask>=3.1.3`, `flask-cors>=6.0.2` (production)
- `reportlab` (optional, for PDF generation)
- `pdfkit` (optional, for HTML→PDF export)

### `web/app.py` — Flask Server (~1950 lines)

**60 API endpoints** organized as:

| Group | Endpoints |
|-------|-----------|
| **Entity CRUD** | `/api/teachers`, `/api/subjects`, `/api/classes`, `/api/rooms` — GET, POST, PUT, DELETE each |
| **Room Types** | GET, POST, DELETE |
| **Lessons** | GET, POST, PUT, DELETE, combined classes sub-endpoints |
| **Constraints** | GET by teacher, POST set, POST remove |
| **Preferences** | GET by teacher, POST set |
| **Generate & Solve** | `/api/generate` — runs C++ `--solve-from-db` via subprocess; `/api/solve` — alternative solver endpoint |
| **Timetable** | GET, move, remove, lock, clear, clear_all |
| **Analytics** | `/api/analytics` — teacher load, room utilization, gap stats |
| **Meta** | `/api/meta` — days, periods |
| **Data Import/Export** | `/api/data/export` (JSON dump), `/api/data/import` (JSON restore) |
| **HTML/PDF Export** | `/api/export/html`, `/api/export/pdf` — generate per-view HTML or PDF |
| **Conflicts** | `/api/conflicts` |
| **Substitutions** | CRUD + status + suggest + available teachers |
| **Divisions** | CRUD + assign/unassign classes |
| **Versions** | CRUD + compare + restore |
| **Custom Timetable** | `/api/sample-school` — sample data; `/api/generate-timetable` — dynamic config → PDF |

**Key internal functions:**

| Function | Purpose |
|----------|---------|
| `_build_html(view, week)` | Builds full HTML timetable document from DB slots (shared by HTML/PDF export) |
| `_solve_backtrack(data)` | Python implementation of backtracking solver (46-stage heuristic solver) |
| `_slot_score(tid, day, period, ...)` | Scoring heuristic for Python solver |
| `generate_timetable()` | C++ subprocess solver wrapper |

### `web/timetable_gen.py` (~615 lines)

| Function | Purpose |
|----------|---------|
| `time_to_minutes()` / `minutes_to_time()` | Time string ↔ int conversion |
| `time_range_overlap()` | Check if two time ranges overlap (clock-time) |
| `generate_periods_for_class()` | Slice a day into lesson + break blocks given durations and break definitions |
| `parse_payload()` | Convert JSON config into per-class timeline configurations |
| `solve_timetable()` | Greedy solver supporting maxPerDay, blockSize, secondTeacherId, clock-time conflict detection |
| `get_sample_school()` | Alliance High School sample: 12 teachers, 5 streams, 12 subjects, 4 break types |

### `web/pdf_generator.py` (~380 lines)

| Function | Purpose |
|----------|---------|
| `generate_timetable_pdf()` | ReportLab A3 landscape canvas: colored subject abbreviation cards with teacher initials, break markers, unscheduled list |
| `_rgb()` | Hex→ReportLab RGB tuple |
| `_time_label()` | `07:30` → `7:30` |

---

## 5. Frontend

### `web/templates/index.html` (159 lines)

Single-page app shell with:
- **Ribbon**: branding, 4 tabs (Home/Data/Timetable/Analytics), score display, conflict warning
- **Sidebar**: 7 collapsible panels (Teachers, Subjects, Classes, Rooms, Lessons, Substitutions, Divisions)
- **View area**: 4 view divs swapped by tab
- **Toast notification container**

### `web/static/js/app.js` (1665 lines)

| Lines | Section | Functions |
|-------|---------|-----------|
| 1-20 | State (`S`) | All entity arrays, solver settings, view state |
| 22-44 | API wrappers | `api.get/post/put/del` using fetch |
| 46-93 | Toast, Spinner, Modal | Reusable UI primitives |
| 95-106 | Color picker | Swatch-based selection |
| 108-125 | `loadAll()` | Parallel API fetches for all entities |
| 127-158 | Conflicts | Display conflict modal |
| 160-208 | Sidebar | Render entity lists with search |
| 210-275 | Ribbon config | `RIBBON_TABS` HTML strings for each tab |
| 275-511 | Views | Home, Data, Timetable renderers |
| 513-565 | Generate/Clear | Wrapper for solver, clearing |
| 567-615 | Solver options, Lock/Remove | |
| 617-654 | Drag & drop | Timetable cell drag/drop with conflict checking |
| 656-904 | CRUD dialogs | Teacher, Subject, Class, Room, Lesson, Constraints dialogs |
| 906-1045 | Analytics | Charts, gap stats |
| 1047-1112 | Export/Import | HTML, PDF, JSON backup/restore |
| 1114-1445 | Versions, Substitutions, Divisions, Compare | |
| 1447-1461 | Utils | `esc()`, `togglePanel()`, `setWeekFilter()` |
| 1463-1632 | Custom Timetable | Modal, sample load, generate (new) |
| 1635-1658 | App object | Public API exposure |
| 1660-1665 | Init | Tab listeners, ribbon load, loadAll |

### `web/static/css/asc.css` (290 lines)

| Section | Lines | Content |
|---------|-------|---------|
| Root | 1-19 | Design tokens (navy, accent, spacing, shadows) |
| Ribbon | 23-53 | Brand, tabs, groups, buttons, score/conflict widgets |
| Layout | 55-91 | Sidebar, main area |
| Timetable | 93-127 | Grid, lesson cards, locked state |
| Modal | 134-145 | Overlay, modal box, header/body/footer |
| Form | 147-156 | Inputs, selects, color swatches |
| Buttons | 158-170 | Primary, accent, danger, ghost variants |
| Misc | 172-264 | Constraint grid, analytics, data table, toast, spinner, print, solver options |
| Custom TT | 268-290 | `.ct-block-row`, `.ct-class-row`, dynamic form styles |

---

## 6. Solvers — Detailed Comparison

| Aspect | C++ BacktrackingSolver | C++ GreedySolver | Python GreedySolver (timetable_gen) |
|--------|----------------------|-------------------|--------------------------------------|
| **Algorithm** | MRV + LCV + domain propagation | Weighted slot scoring | First-fit with difficulty ordering |
| **Grid model** | Uniform DAYS×PERIODS (5×8) | Uniform DAYS×PERIODS | Per-class dynamic grids |
| **Period durations** | All classes same | All classes same | Per-class configurable |
| **Breaks** | Fixed events block specific slots | Fixed events block specific slots | Dynamic break-aware timeline slicing |
| **Clock-time** | Slot-index based | Slot-index based | Wall-clock overlap detection |
| **maxPerDay** | Yes (Lesson field) | Yes | Yes |
| **blockSize** | Yes | Yes | Yes |
| **secondTeacher** | Yes (Lesson field) | Yes | Yes |
| **Locked slots** | Yes | Yes | No (locked not exposed in payload) |
| **Combined classes** | Yes | Yes | No |
| **Week types** | Yes (A/B/Every) | Yes | No (all every-week) |
| **Room allocation** | Yes | Yes | No (all roomId=1) |
| **Teacher max consecutive** | Yes (Teacher.maxConsecutive) | Yes | No |
| **Class period constraints** | Yes (earliestPeriod/latestPeriod) | Yes | No |

---

## 7. Build System

### Makefile (85 lines)

- `make` — release build with `-O2 -DNDEBUG`
- `make BUILD=debug` — debug with `-O0 -g3 -fsanitize=address,undefined -Werror`
- `make test` — builds and runs `test_runner`
- `make format` — runs `clang-format` on all source files
- Uses `pkg-config` for Qt5 linkage

### CMakeLists.txt (111 lines)

- Identical source list to Makefile (includes `TimelineGenerator.cpp`)
- Qt5 AutoMoc/AutoUic/AutoRcc
- Separate targets: `timetableGen` (GUI) and `test_runner`
- AddSan/UbSan in Debug builds
- Custom `run` and `check` targets

### `pyproject.toml`

Minimal — only declares `flask` and `flask-cors` as dependencies. ReportLab is detected at runtime via try/except.

### `Dockerfile`, `run.sh`

Docker deployment support and convenience launcher.

---

## 8. Tests

### C++ Tests (`tests/test_runner.cpp`, 315 lines)

12 test functions covering:

| Test | What it validates |
|------|-------------------|
| `runRegressionTest()` | Daily fixed event blocks P3 slot in greedy solver |
| `runFixedEventTest()` | NONE/WEEKLY/DAILY recurrence semantics |
| `runTeacherPreferenceTest()` | BLOCKED/PREFERRED/UNDESIRABLE slots respected |
| `runTeacherOverloadTest()` | `FeasibilityChecker` detects teacher overload with secondTeacher |
| `runLessonOrderTest()` | subjectRequirements with room type via `ResourceTracker` |
| `runCombinedClassesTest()` | Combined class lessons occupy both class grids |
| `runRoomConstraintTest()` | Room capacity and type constraints |
| `runBacktrackSolverTest()` | Full backtrack solver run with score > 0 |
| `runGreedySolverTest()` | Greedy solver produces valid timetable |
| `runTimetableEvaluatorTest()` | Evaluator computes stats correctly |
| `runDomainTrackerTest()` | Domain pruning correctness |
| `runAnalyticsTest()` | Analytics service: teacher load, room utilization, gaps |

### Python Tests (`tests/test_api.py`, 144 lines)

17 integration tests for Flask API endpoints using a real spawned server on port 15421:

- Entity CRUD (teachers, subjects, classes, rooms, lessons)
- Retrieval and data contract validation
- Generate endpoint

### Python Unit Tests (`tests/test_timetable_gen.py`, 24 tests)

| Category | Tests |
|----------|-------|
| Time helpers | `time_to_minutes`, `minutes_to_time`, `roundtrip`, `time_range_overlap` |
| Timeline generation | no breaks (40/60min), single/multiple/overlapping breaks, break at boundary, all-day break, short day, break separation |
| Payload parsing | sample school, per-class durations, Form 4 fewer blocks, Assembly day filter, constraint passthrough |
| Solver | full sample, maxPerDay, blockSize, secondTeacher, limited slots unscheduled |

---

## 9. Gap Analysis

### 9.1 Structural Issues

| Issue | Location | Impact | Recommendation |
|-------|----------|--------|---------------|
| **DataManager is a god object** | `DataManager.h/.cpp` | ~50 public methods, mixes data access, CRUD, solver config, conflict checking, undo/redo. Hard to test, maintain, or extend. | Split into focused repositories (TeacherRepo, LessonRepo, TimetableRepo) and services (SolverOrchestrator, ConflictService). |
| **UIFramework pollutes domain** | `services/TimetableEvaluator.cpp` | #includes Qt headers in a supposedly domain-pure service | Move Qt-dependent code to GUI layer; keep evaluation logic pure C++ |
| **Inconsistent DB schema** | `SQLiteService.cpp` | Days/periods are in-memory only, never persisted — but fixed events reference dayId/periodId | Document this design decision explicitly; consider persisting day/period config |
| **Uniform grid assumption (5×8)** | `Timetable::setSlot()` | All classes share the same `numDays×numPeriods` grid | Already partially addressed via `TimelineGenerator` and Python dynamic solver, but the C++ solver still assumes uniform grid |
| **Week types not in solver output** | `timetable_gen.py` solver | Generated slots always have `weekType: 0` | Add week rotation to Python solver |

### 9.2 Testing Gaps

| Gap | Details |
|-----|---------|
| **No Python solver tests for week rotation** | `weekType` field is set to 0 but never tested for A/B week scheduling |
| **No C++ TimelineGenerator test** | C++ `services/TimelineGenerator.cpp` exists but has no test coverage |
| **No pdf_generator.py unit tests** | ReportLab PDF generation has zero tests |
| **No GUI automated tests** | Qt GUI (16+ widgets) has no UI test coverage |
| **No integration test for custom timetable pipeline** | The full chain: JS modal → Flask → timetable_gen → pdf_generator is untested |
| **test_runner lacks test isolation** | All C++ tests share a single DataManager instance with no cleanup between runs |

### 9.3 Issues from Bug Fix History

| ID | Issue | Status |
|----|-------|--------|
| B1 | `updateLesson` matched by content fields instead of ID | Fixed |
| B2 | `GreedySolver` room check missing `wt` param | Fixed |
| B3 | `removeTeacher/Subject/Class/Room` no cascade delete | Fixed |
| B5 | `removeFixedEvent` returned true on not-found | Fixed |
| B9 | `ConflictChecker` didn't validate full block | Fixed |
| B10 | `FeasibilityChecker` ignored `secondTeacherId` | Fixed |
| B14 | Flask cascade deletes missing tables | Fixed |
| B19 | Flask bound to `0.0.0.0` | Fixed |
| B20 | `killPort` killed non-python processes | Fixed |
| B4 | `_build_unscheduled_list` duplicate check | Fixed |

### 9.4 Recommendations (Priority Order)

1. **Persist day/period configuration** in SQLite so the dynamic timeline system works across restarts
2. **Add `blockSize` awareness to the HTML/PDF export** — currently exports show one row per period cell, but blockSize=2 lessons should span 2 rows
3. **Add week rotation to Python solver** — generate A/B week assignments
4. **Add locked slots support** to the Python solver (respect `locked` in slot data)
5. **Split DataManager** into focused repositories before adding more functionality
6. **Add integration test** for the custom timetable pipeline (JS → Flask → PDF)
7. **Add unit tests** for `pdf_generator.py` and C++ `TimelineGenerator`
8. **Document all 60 API endpoints** in OpenAPI format

---

## 10. Key File Summary

```
offlineTimeTableCpp/
├── main.cpp                          # 224 lines — Entry point, Flask launcher, headless solver
├── Makefile                          # 85 lines — Build system
├── CMakeLists.txt                    # 111 lines — CMake build
├── AGENTS.md                         # Agent development guide
├── pyproject.toml                    # Python deps (flask, flask-cors)
│
├── models/                           # 17 domain headers
│   ├── Teacher.h, Subject.h, SchoolClass.h, Room.h, Lesson.h
│   ├── LessonUnit.h, FixedEvent.h, TimeBlock.h
│   ├── Day.h, Period.h, RoomType.h, SubjectRequirement.h
│   ├── Division.h, Substitution.h
│   ├── TeacherConstraint.h, TeacherPreference.h
│   └── ConstraintViolation.h
│
├── services/                         # 19 .cpp/.h service pairs
│   ├── DataManager.h/.cpp            # God object — CRUD + solver config + undo/redo
│   ├── SQLiteService.h/.cpp          # All SQL operations
│   ├── TimetableEngine.h/.cpp        # Solver orchestrator
│   ├── BacktrackingSolver.h/.cpp     # MRV + LCV + domain propagation
│   ├── GreedySolver.h/.cpp           # Heuristic fallback
│   ├── ResourceTracker.h/.cpp        # Placement resource tracking
│   ├── DomainTracker.h/.cpp          # Domain pruning
│   ├── FeasibilityChecker.h/.cpp     # Pre-solve validation
│   ├── ConflictChecker.h/.cpp        # Post-placement conflict detection
│   ├── TimetableEvaluator.h/.cpp     # Score computation
│   ├── AnalyticsService.h/.cpp       # Statistics
│   ├── ExportService.h/.cpp          # CSV/HTML export
│   ├── PdfReportService.h/.cpp       # Qt5 PDF
│   ├── ConstraintExplanationService.h/.cpp
│   ├── ConstraintExplain.h/.cpp
│   ├── Benchmark.h/.cpp
│   ├── UndoRedoStack.h/.cpp
│   └── TimelineGenerator.h/.cpp      # C++ dynamic timeline (new)
│
├── timetable/                        # Core grid model
│   └── Timetable.h/.cpp              # TimetableCell, 2D grid storage
│
├── gui/                              # 16 Qt5 widget files
│   ├── MainWindow.cpp
│   ├── ribbon/RibbonToolbar.cpp
│   ├── sidebar/DataSidebar.cpp
│   ├── timetableview/{TimetableViewWidget, TimetableScene, LessonCardItem}.cpp
│   ├── dashboard/DashboardWidget.cpp
│   ├── teachers/TeacherDialog.cpp
│   ├── subjects/SubjectDialog.cpp
│   ├── classes/ClassDialog.cpp
│   ├── rooms/RoomDialog.cpp
│   ├── substitutions/SubstitutionWidget.cpp
│   ├── divisions/DivisionWidget.cpp
│   └── constraints/ConstraintRelaxationDialog.cpp
│
├── web/                              # Python web layer
│   ├── app.py                        # ~1950 lines — Flask server, 60 endpoints
│   ├── timetable_gen.py              # ~615 lines — Dynamic timeline + greedy solver
│   ├── pdf_generator.py              # ~380 lines — ReportLab aSc-style PDF
│   ├── static/js/app.js              # 1665 lines — SPA frontend
│   ├── static/css/asc.css            # 290 lines — Styles
│   └── templates/index.html          # 159 lines — SPA shell
│
├── tests/                            # Tests
│   ├── test_runner.cpp               # 315 lines — 12 C++ tests
│   ├── test_api.py                   # 144 lines — 17 Flask integration tests
│   └── test_timetable_gen.py         # 24 tests — Python unit tests
│
├── utils/PathUtil.h                  # Path resolution utility
├── data/                             # SQLite DB (gitignored)
└── docs/                             # Documentation
```

---

*Generated: 2026-06-06*
