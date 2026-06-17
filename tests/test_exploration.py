"""
Bug-Condition Exploration Tests — Task 1 (timetable-engine-overhaul)
====================================================================
These tests MUST FAIL on unfixed code.  Failure is the SUCCESS case:
each failure documents a counterexample that proves the bug exists.

Tests are self-contained: they spin up an isolated Flask process backed
by a throw-away SQLite DB so they never touch production data.

Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.9, 1.11,
           1.12, 1.16
"""

import json
import os
import shutil
import sqlite3
import subprocess
import tempfile
import time
import urllib.request

import pytest

# ---------------------------------------------------------------------------
# Project root — two levels above this file (tests/)
# ---------------------------------------------------------------------------
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---------------------------------------------------------------------------
# Shared Flask fixture
# ---------------------------------------------------------------------------

class FlaskServer:
    """Manages a per-test-module Flask process with an isolated DB."""

    def __init__(self, port: int):
        self.port = port
        self.base = f"http://127.0.0.1:{port}"
        self._tmpdir = tempfile.mkdtemp(prefix="timetable_exploration_")
        self._db_path = os.path.join(self._tmpdir, "test.db")
        self._env = os.environ.copy()
        self._env.update({
            "FLASK_PORT": str(port),
            "FLASK_SILENT_LOGS": "1",
            "FLASK_DB_PATH": self._db_path,
        })
        self._proc = None

    def start(self):
        self._proc = subprocess.Popen(
            ["python3", "web/app.py"],
            cwd=PROJECT_ROOT,
            env=self._env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        for _ in range(30):
            try:
                urllib.request.urlopen(self.base + "/", timeout=1)
                return
            except Exception:
                time.sleep(0.3)
        raise RuntimeError(f"Flask did not start on port {self.port}")

    def stop(self):
        if self._proc:
            self._proc.terminate()
            self._proc.wait()
        shutil.rmtree(self._tmpdir, ignore_errors=True)

    # ---- helpers ----

    def post(self, path: str, data: dict) -> tuple[int, dict]:
        body = json.dumps(data).encode()
        req = urllib.request.Request(
            self.base + path,
            data=body,
            headers={"Content-Type": "application/json"},
        )
        try:
            resp = urllib.request.urlopen(req, timeout=10)
            return resp.status, json.loads(resp.read())
        except urllib.error.HTTPError as e:
            return e.code, json.loads(e.read())

    def get(self, path: str) -> tuple[int, object]:
        try:
            resp = urllib.request.urlopen(self.base + path, timeout=10)
            return resp.status, json.loads(resp.read())
        except urllib.error.HTTPError as e:
            return e.code, json.loads(e.read())

    def db(self) -> sqlite3.Connection:
        """Open a direct connection to the test SQLite DB."""
        conn = sqlite3.connect(self._db_path)
        conn.row_factory = sqlite3.Row
        return conn


# ---------------------------------------------------------------------------
# Module-level fixture — one Flask process shared across all tests in this
# module to avoid port conflicts and slow startup overhead.
# ---------------------------------------------------------------------------

_SERVER: FlaskServer | None = None


@pytest.fixture(scope="module")
def server():
    global _SERVER
    _SERVER = FlaskServer(port=15500)
    _SERVER.start()
    yield _SERVER
    _SERVER.stop()
    _SERVER = None


# ---------------------------------------------------------------------------
# Seed helpers
# ---------------------------------------------------------------------------

def _seed_minimal(server: FlaskServer) -> dict:
    """Create one teacher, one subject, one class, one room and one lesson.
    Returns a dict of the created entity IDs."""
    _, t = server.post("/api/teachers", {"name": "T1", "maxConsecutive": 0})
    _, s = server.post("/api/subjects", {"name": "Math"})
    _, c = server.post("/api/classes",  {"name": "C1", "studentCount": 30})
    _, r = server.post("/api/rooms",    {"name": "R1", "capacity": 30, "roomTypeId": 1})
    _, l = server.post("/api/lessons",  {
        "teacherId": t["id"], "subjectId": s["id"], "classId": c["id"],
        "periodsPerWeek": 1, "blockSize": 1,
    })
    return {"teacherId": t["id"], "subjectId": s["id"],
            "classId": c["id"], "roomId": r["id"], "lessonId": l["id"]}


def _seed_over_constrained(server: FlaskServer) -> None:
    """Seed 20 classes each with 40 lessons sharing a single teacher —
    guaranteed to exhaust any backtracking solver."""
    _, t = server.post("/api/teachers", {"name": "SoloTeacher"})
    _, s = server.post("/api/subjects", {"name": "All-Subject"})
    _, r = server.post("/api/rooms",    {"name": "BigRoom", "capacity": 200, "roomTypeId": 1})
    for i in range(20):
        _, c = server.post("/api/classes", {"name": f"OC-Class-{i}", "studentCount": 30})
        # 40 periods/week per class → 800 total for one teacher (far beyond grid capacity)
        server.post("/api/lessons", {
            "teacherId": t["id"], "subjectId": s["id"], "classId": c["id"],
            "periodsPerWeek": 40, "blockSize": 1,
        })


# ===========================================================================
# Test 1a — Schema Divergence
# ===========================================================================

class TestSchemaState:
    """Validates: Requirements 1.1, 1.3, 1.5
    Bug Condition: Category A — Flask init_db() does not create
    'days', 'periods', 'schema_migrations', or 'subject_requirements' tables.
    EXPECTED FAILURE on unfixed code: one or more tables absent."""

    def test_1a_required_tables_exist(self, server: FlaskServer):
        """
        **Validates: Requirements 1.1, 1.3, 1.5**

        After Flask starts and init_db() runs, the SQLite master table MUST
        contain: days, periods, schema_migrations, subject_requirements.

        On unfixed code this FAILS because init_db() never creates those four
        tables — they are absent from the DDL.
        """
        conn = server.db()
        cursor = conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
        )
        existing = {row[0] for row in cursor.fetchall()}
        conn.close()

        required = {"days", "periods", "schema_migrations", "subject_requirements"}
        missing = required - existing

        # This assertion FAILS on unfixed code: tables are absent.
        assert missing == set(), (
            f"COUNTEREXAMPLE — Tables absent from Flask schema after init_db(): {sorted(missing)}. "
            f"Tables present: {sorted(existing)}"
        )


# ===========================================================================
# Test 1b — Solver Exhaustion HTTP code
# ===========================================================================

class TestSolverExhaustionHTTPCode:
    """Validates: Requirement 1.6
    Bug Condition: Category B — when the Python solver cannot schedule all
    lessons it returns HTTP 200 with score=0 instead of HTTP 422.
    EXPECTED FAILURE on unfixed code: receives HTTP 200."""

    def test_1b_over_constrained_returns_422(self, server: FlaskServer):
        """
        **Validates: Requirements 1.6**

        POST /api/generate with an impossibly over-constrained dataset
        (20 classes × 40 lessons/week sharing a single teacher) MUST return
        HTTP 422 with body containing reason='TIMEOUT'.

        On unfixed code this FAILS: the Python solver returns HTTP 200 with
        an empty or partial slot list and score=0.
        """
        _seed_over_constrained(server)
        status, body = server.post("/api/generate", {"algorithm": "backtrack", "seed": 1})

        # This assertion FAILS on unfixed code: status is 200.
        assert status == 422, (
            f"COUNTEREXAMPLE — Expected HTTP 422 for over-constrained dataset, "
            f"got HTTP {status}. Body: {body}"
        )
        assert body.get("reason") == "TIMEOUT", (
            f"COUNTEREXAMPLE — Expected reason='TIMEOUT' in body, got: {body}"
        )


# ===========================================================================
# Test 1c — Score threshold fields absent from /api/generate response
# ===========================================================================

class TestScoreThresholdFields:
    """Validates: Requirement 1.8
    Bug Condition: Category B — /api/generate never includes a 'belowThreshold'
    field in its response regardless of score.
    EXPECTED FAILURE on unfixed code: key absent."""

    def test_1c_below_threshold_field_present(self, server: FlaskServer):
        """
        **Validates: Requirements 1.8**

        POST /api/generate with a normal (low-data) dataset MUST return a
        response body that includes the key 'belowThreshold' (boolean) and
        'threshold' (integer).

        On unfixed code this FAILS: neither key is present in the solver
        response from _solve_backtrack / _solve_greedy.
        """
        ids = _seed_minimal(server)
        status, body = server.post("/api/generate", {"algorithm": "backtrack", "seed": 42})

        # The HTTP call may succeed — we're checking the response schema.
        assert status in (200, 422), (
            f"Unexpected HTTP {status}: {body}"
        )

        # This assertion FAILS on unfixed code: 'belowThreshold' key is missing.
        assert "belowThreshold" in body, (
            f"COUNTEREXAMPLE — 'belowThreshold' key absent from /api/generate response. "
            f"Response keys present: {sorted(body.keys())}"
        )
        # Secondary check — 'threshold' must also be present.
        assert "threshold" in body, (
            f"COUNTEREXAMPLE — 'threshold' key absent from /api/generate response. "
            f"Response keys present: {sorted(body.keys())}"
        )


# ===========================================================================
# Test 1d — Missing room type abort (via Flask API)
# ===========================================================================

class TestMissingRoomTypeAbort:
    """Validates: Requirement 1.9
    Bug Condition: Category B — when a subject has no subject_requirements row
    the solver silently assigns roomId=-1; FeasibilityChecker has no
    MISSING_ROOM_TYPE check.
    EXPECTED FAILURE on unfixed code: no feasibilityErrors key and/or roomId=-1
    slots present."""

    def test_1d_missing_room_type_returns_feasibility_error(self, server: FlaskServer):
        """
        **Validates: Requirements 1.9**

        Given:
          - A subject with NO row in subject_requirements
          - The solver is invoked via POST /api/generate

        The response MUST include a non-empty 'feasibilityErrors' list with at
        least one entry whose 'category' == 'MISSING_ROOM_TYPE', AND
        timetable_slots MUST remain empty (no placement).

        On unfixed code this FAILS: there is no subject_requirements table in
        Flask at all and no MISSING_ROOM_TYPE check — roomId=-1 slots are
        written (or the table doesn't exist so the check can't run).
        """
        # We can only call Flask APIs (subject_requirements endpoint doesn't
        # exist yet on unfixed code, so we exercise the absence directly).
        ids = _seed_minimal(server)

        # Confirm subject_requirements table is absent (this is the bug).
        conn = server.db()
        cursor = conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='subject_requirements'"
        )
        sr_exists = cursor.fetchone() is not None
        conn.close()

        # Run the solver — with no subject_requirements, the solver should
        # emit MISSING_ROOM_TYPE errors and write no slots.
        status, body = server.post("/api/generate", {"algorithm": "backtrack", "seed": 42})

        # Check 1: feasibilityErrors must be present and non-empty.
        assert "feasibilityErrors" in body and len(body.get("feasibilityErrors", [])) > 0, (
            f"COUNTEREXAMPLE — 'feasibilityErrors' absent or empty in /api/generate response. "
            f"subject_requirements table exists: {sr_exists}. "
            f"Response: {body}"
        )

        # Check 2: no roomId=-1 in persisted timetable_slots.
        conn = server.db()
        bad = conn.execute(
            "SELECT COUNT(*) FROM timetable_slots WHERE roomId=-1"
        ).fetchone()[0]
        conn.close()
        assert bad == 0, (
            f"COUNTEREXAMPLE — {bad} slot(s) with roomId=-1 found in timetable_slots. "
            f"MISSING_ROOM_TYPE check not implemented."
        )


# ===========================================================================
# Test 1e — Subprocess timeout → HTTP 504
# ===========================================================================

class TestSubprocessTimeout504:
    """Validates: Requirements 1.11, 1.12
    Bug Condition: Category C — when the C++ solver subprocess times out the
    TimeoutExpired exception falls through to the generic except block which
    returns HTTP 500 instead of HTTP 504.
    EXPECTED FAILURE on unfixed code: receives HTTP 500."""

    def test_1e_solver_timeout_returns_504(self, tmp_path):
        """
        **Validates: Requirements 1.11, 1.12**

        When SOLVER_TIMEOUT=1 and the binary is a slow script that sleeps 5 s,
        POST /api/solve MUST return HTTP 504.

        On unfixed code this FAILS: the TimeoutExpired lands in
        'except Exception as e' → HTTP 500.
        """
        # Build a fake "slow solver" shell script.
        slow_bin = tmp_path / "fake_solver.sh"
        slow_bin.write_text("#!/bin/sh\nsleep 5\n")
        slow_bin.chmod(0o755)

        # Spin up a dedicated Flask instance with the fake binary + short timeout.
        srv = FlaskServer(port=15501)
        srv._env["CPP_SOLVER_PATH"] = str(slow_bin)
        srv._env["SOLVER_TIMEOUT"] = "1"
        srv.start()
        try:
            status, body = srv.post("/api/solve", {})
            # This assertion FAILS on unfixed code: status is 500 (generic handler).
            assert status == 504, (
                f"COUNTEREXAMPLE — Expected HTTP 504 for solver timeout, "
                f"got HTTP {status}. Body: {body}"
            )
            assert "SOLVER_TIMEOUT" in (body.get("error") or ""), (
                f"COUNTEREXAMPLE — Expected error='SOLVER_TIMEOUT' in body, got: {body}"
            )
        finally:
            srv.stop()


# ===========================================================================
# Test 1f — Locked slot overwrite by greedy solver
# ===========================================================================

class TestLockedSlotOverwrite:
    """Validates: Requirement 1.16
    Bug Condition: Category D — _solve_greedy has no explicit locked_set check.
    The class_busy pre-fill uses the locked slot's weekType key, but an
    every-week lesson (weekType=0) checks for key+(0,) only.  When a locked
    slot has weekType=1 (Week-A) the pre-fill inserts key+(1,) which is NOT
    found by an every-week lesson's _is_busy_week check (which only looks for
    key+(0,)).  The greedy therefore schedules a weekType=0 lesson in the
    locked Week-A slot, and _save_timetable emits an INSERT OR IGNORE that
    tries to place an overlapping row — revealing that the locked_set guard
    required by requirement 2.16 §2 is absent.
    EXPECTED FAILURE on unfixed code: an unlocked every-week slot is produced
    at the same (classId, dayId, periodId) as the locked Week-A slot."""

    def test_1f_greedy_does_not_overwrite_locked_slot(self, server: FlaskServer):
        """
        **Validates: Requirements 1.16**

        Scenario — locked Week-A slot (weekType=1) vs every-week lesson (weekType=0):

        Setup:
          1. Seed one teacher, one subject, one class, one room.
          2. Create a lesson with periodsPerWeek=40 and weekType=0 (every week).
          3. Directly INSERT a locked row with weekType=1 (Week A) at
             (classId=C, dayId=1, periodId=1).
          4. Run the greedy solver.
          5. Assert NO timetable_slots row with locked=0 and weekType=0 is placed
             at (classId=C, dayId=1, periodId=1) — the locked Week-A cell must
             be treated as occupied for every-week scheduling purposes.
          6. Assert the locked row still exists with locked=1 and weekType=1.

        Root cause: _solve_greedy pre-fills
            _mark_busy_week(class_busy, (classId, day, period), wt=1)
            → inserts (classId, day, period, 1)
        But the every-week lesson's _is_busy_week check looks for
            (classId, day, period, 0)  ← NOT present
        and therefore does NOT see the Week-A locked slot as busy, allowing
        the greedy to emit a weekType=0 slot that collides with it.

        Without an explicit locked_set keyed by (classId, dayId, periodId)
        regardless of weekType, requirement 2.16 §2 cannot be satisfied.
        """
        _, t = server.post("/api/teachers", {"name": "WeekLockTeacher"})
        _, s = server.post("/api/subjects", {"name": "WeekLockSubj"})
        _, c = server.post("/api/classes",  {"name": "WeekLockClass", "studentCount": 30})
        _, r = server.post("/api/rooms",    {"name": "WeekLockRoom", "capacity": 30, "roomTypeId": 1})

        class_id   = c["id"]
        teacher_id = t["id"]
        subject_id = s["id"]
        room_id    = r["id"]
        DAY_ID, PERIOD_ID = 1, 1

        # Lesson: every week (weekType=0), fill the grid.
        server.post("/api/lessons", {
            "teacherId": teacher_id, "subjectId": subject_id, "classId": class_id,
            "periodsPerWeek": 40, "blockSize": 1, "weekType": 0,
        })

        # Locked slot: Week A (weekType=1) at (day=1, period=1).
        conn = server.db()
        conn.execute("""
            INSERT INTO timetable_slots
                (classId, dayId, periodId, subjectId, teacherId, roomId, locked, weekType)
            VALUES (?, ?, ?, ?, ?, ?, 1, 1)
        """, (class_id, DAY_ID, PERIOD_ID, subject_id, teacher_id, room_id))
        conn.commit()
        conn.close()

        # Run greedy.
        status, body = server.post("/api/generate", {"algorithm": "greedy", "seed": 0})
        assert status == 200, f"Unexpected /api/generate error: {status} {body}"

        conn = server.db()
        # All rows at that (classId, dayId, periodId) regardless of weekType.
        rows = [dict(r) for r in conn.execute("""
            SELECT subjectId, teacherId, roomId, locked, weekType
            FROM timetable_slots
            WHERE classId=? AND dayId=? AND periodId=?
        """, (class_id, DAY_ID, PERIOD_ID)).fetchall()]
        conn.close()

        locked_rows   = [r for r in rows if r["locked"] == 1]
        unlocked_rows = [r for r in rows if r["locked"] == 0]

        # The locked Week-A slot must still be present and untouched.
        assert len(locked_rows) >= 1, (
            f"COUNTEREXAMPLE — Locked slot (classId={class_id}, day={DAY_ID}, "
            f"period={PERIOD_ID}, weekType=1) was DELETED by greedy solver."
        )
        assert locked_rows[0]["weekType"] == 1, (
            f"COUNTEREXAMPLE — Locked slot weekType mutated: {locked_rows[0]}"
        )

        # The greedy must NOT emit an every-week (weekType=0) slot at this
        # position, because that would mean it tried to schedule over a locked slot.
        # On unfixed code this fails: the greedy emits a weekType=0 slot there
        # because the class_busy key (classId,day,period,1) doesn't block wt=0.
        every_week_at_locked = [r for r in unlocked_rows if r["weekType"] == 0]
        assert len(every_week_at_locked) == 0, (
            f"COUNTEREXAMPLE — Greedy produced an every-week (weekType=0) unlocked "
            f"slot at the locked Week-A position "
            f"(classId={class_id}, day={DAY_ID}, period={PERIOD_ID}): "
            f"{every_week_at_locked}. "
            f"No explicit locked_set guard — greedy used is_busy_week which does "
            f"not treat weekType=1 locked slots as busy for weekType=0 lessons."
        )
