import sqlite3
import os
import random
import json
import logging
from datetime import datetime
from flask import Flask, request, jsonify, render_template, Response
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Silence default request logging when launched by C++ binary
if os.environ.get("FLASK_SILENT_LOGS"):
    log = logging.getLogger('werkzeug')
    log.setLevel(logging.ERROR)

DB_PATH = os.environ.get('FLASK_DB_PATH') or os.path.join(
    os.path.dirname(__file__), '..', 'data', 'timetableGen.db'
)

DAYS = [
    {'id': 1, 'name': 'Monday'},
    {'id': 2, 'name': 'Tuesday'},
    {'id': 3, 'name': 'Wednesday'},
    {'id': 4, 'name': 'Thursday'},
    {'id': 5, 'name': 'Friday'},
]
PERIODS = [
    {'id': 1, 'start': '08:00', 'end': '08:45'},
    {'id': 2, 'start': '08:45', 'end': '09:30'},
    {'id': 3, 'start': '09:30', 'end': '10:15'},
    {'id': 4, 'start': '10:15', 'end': '11:00'},
    {'id': 5, 'start': '11:00', 'end': '11:45'},
    {'id': 6, 'start': '12:30', 'end': '13:15'},
    {'id': 7, 'start': '13:15', 'end': '14:00'},
    {'id': 8, 'start': '14:00', 'end': '14:45'},
]


def get_db():
    os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    return conn


def init_db():
    conn = get_db()
    c = conn.cursor()
    c.executescript("""
        CREATE TABLE IF NOT EXISTS teachers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            maxConsecutive INTEGER DEFAULT 0,
            color TEXT DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS subjects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            color TEXT DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS classes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            studentCount INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            capacity INTEGER NOT NULL DEFAULT 30,
            roomTypeId INTEGER NOT NULL DEFAULT 1
        );
        CREATE TABLE IF NOT EXISTS room_types (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS lessons (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            teacherId INTEGER NOT NULL,
            secondTeacherId INTEGER DEFAULT -1,
            subjectId INTEGER NOT NULL,
            classId INTEGER NOT NULL,
            periodsPerWeek INTEGER NOT NULL DEFAULT 1,
            blockSize INTEGER NOT NULL DEFAULT 1,
            maxPerDay INTEGER NOT NULL DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS teacher_constraints (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            teacherId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            UNIQUE(teacherId, dayId, periodId)
        );
        CREATE TABLE IF NOT EXISTS teacher_preferences (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            teacherId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            prefType TEXT NOT NULL DEFAULT 'NEUTRAL',
            UNIQUE(teacherId, dayId, periodId)
        );
        CREATE TABLE IF NOT EXISTS timetable_slots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            classId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            subjectId INTEGER,
            teacherId INTEGER,
            roomId INTEGER,
            locked INTEGER NOT NULL DEFAULT 0,
            weekType INTEGER NOT NULL DEFAULT 0,
            UNIQUE(classId, dayId, periodId, weekType)
        );
        CREATE TABLE IF NOT EXISTS divisions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            canRunInParallel INTEGER NOT NULL DEFAULT 1
        );
        CREATE TABLE IF NOT EXISTS substitutions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            originalTeacherId INTEGER NOT NULL,
            substituteTeacherId INTEGER NOT NULL DEFAULT -1,
            subjectId INTEGER NOT NULL,
            classId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'PENDING',
            reason TEXT DEFAULT '',
            date TEXT DEFAULT ''
        );
    """)
    # Migrations — add columns that may be missing in existing DBs
    for migration in [
        "ALTER TABLE timetable_slots ADD COLUMN locked INTEGER NOT NULL DEFAULT 0",
        "ALTER TABLE teachers ADD COLUMN color TEXT NOT NULL DEFAULT ''",
        "ALTER TABLE subjects ADD COLUMN color TEXT NOT NULL DEFAULT ''",
        "ALTER TABLE classes ADD COLUMN divisionId INTEGER DEFAULT NULL",
        "ALTER TABLE classes ADD COLUMN groupId INTEGER DEFAULT NULL",
        "ALTER TABLE lessons ADD COLUMN weekType INTEGER DEFAULT 0",
        "ALTER TABLE lessons ADD COLUMN secondTeacherId INTEGER DEFAULT -1",
        "ALTER TABLE timetable_slots ADD COLUMN weekType INTEGER NOT NULL DEFAULT 0",
        "CREATE TABLE IF NOT EXISTS lesson_combined_classes (lessonId INTEGER NOT NULL, classId INTEGER NOT NULL, UNIQUE(lessonId, classId))",
    ]:
        try:
            c.execute(migration)
            conn.commit()
        except Exception:
            pass
    c.execute("SELECT COUNT(*) FROM room_types")
    if c.fetchone()[0] == 0:
        for rt in ['Classroom', 'Science Lab', 'Computer Lab', 'Art Room', 'Gym']:
            c.execute("INSERT INTO room_types (name) VALUES (?)", (rt,))
    conn.commit()
    conn.close()


def qrows(q):
    return [dict(r) for r in q.fetchall()]


# ── TEACHERS ──────────────────────────────────────────────────────────────────

@app.route('/api/teachers', methods=['GET'])
def get_teachers():
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM teachers ORDER BY name"))
    conn.close()
    return jsonify(data)


@app.route('/api/teachers', methods=['POST'])
def add_teacher():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("INSERT INTO teachers (name,maxConsecutive,color) VALUES(?,?,?)",
              (name, int(d.get('maxConsecutive', 0)), d.get('color', '')))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid, 'name': name}), 201


@app.route('/api/teachers/<int:tid>', methods=['PUT'])
def update_teacher(tid):
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("UPDATE teachers SET name=?,maxConsecutive=?,color=? WHERE id=?",
                 (name, int(d.get('maxConsecutive', 0)), d.get('color', ''), tid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/teachers/<int:tid>', methods=['DELETE'])
def delete_teacher(tid):
    conn = get_db()
    for q, params in [
            ("DELETE FROM teachers WHERE id=?", (tid,)),
            ("DELETE FROM lessons WHERE teacherId=? OR secondTeacherId=?", (tid, tid)),
            ("DELETE FROM teacher_constraints WHERE teacherId=?", (tid,)),
            ("DELETE FROM teacher_preferences WHERE teacherId=?", (tid,)),
            ("DELETE FROM substitutions WHERE originalTeacherId=? OR substituteTeacherId=?", (tid, tid)),
            ("DELETE FROM timetable_slots WHERE teacherId=?", (tid,))]:
        conn.execute(q, params)
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── SUBJECTS ──────────────────────────────────────────────────────────────────

@app.route('/api/subjects', methods=['GET'])
def get_subjects():
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM subjects ORDER BY name"))
    conn.close()
    return jsonify(data)


@app.route('/api/subjects', methods=['POST'])
def add_subject():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("INSERT INTO subjects (name,color) VALUES(?,?)", (name, d.get('color', '')))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid, 'name': name, 'color': d.get('color', '')}), 201


@app.route('/api/subjects/<int:sid>', methods=['PUT'])
def update_subject(sid):
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("UPDATE subjects SET name=?,color=? WHERE id=?",
                 (name, d.get('color', ''), sid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/subjects/<int:sid>', methods=['DELETE'])
def delete_subject(sid):
    conn = get_db()
    conn.execute("DELETE FROM subjects WHERE id=?", (sid,))
    conn.execute("DELETE FROM lessons WHERE subjectId=?", (sid,))
    conn.execute("DELETE FROM timetable_slots WHERE subjectId=?", (sid,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── CLASSES ───────────────────────────────────────────────────────────────────

@app.route('/api/classes', methods=['GET'])
def get_classes():
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM classes ORDER BY name"))
    conn.close()
    return jsonify(data)


@app.route('/api/classes', methods=['POST'])
def add_class():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("INSERT INTO classes (name,studentCount) VALUES(?,?)",
              (name, int(d.get('studentCount', 0))))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid, 'name': name}), 201


@app.route('/api/classes/<int:cid>', methods=['PUT'])
def update_class(cid):
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("UPDATE classes SET name=?,studentCount=? WHERE id=?",
                 (name, int(d.get('studentCount', 0)), cid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/classes/<int:cid>', methods=['DELETE'])
def delete_class(cid):
    conn = get_db()
    conn.execute("DELETE FROM classes WHERE id=?", (cid,))
    conn.execute("DELETE FROM lessons WHERE classId=?", (cid,))
    conn.execute("DELETE FROM lesson_combined_classes WHERE classId=?", (cid,))
    conn.execute("DELETE FROM substitutions WHERE classId=?", (cid,))
    conn.execute("DELETE FROM timetable_slots WHERE classId=?", (cid,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── ROOMS ─────────────────────────────────────────────────────────────────────

@app.route('/api/rooms', methods=['GET'])
def get_rooms():
    conn = get_db()
    data = qrows(conn.execute("""
        SELECT r.*,COALESCE(rt.name,'') as roomTypeName
        FROM rooms r LEFT JOIN room_types rt ON r.roomTypeId=rt.id ORDER BY r.name"""))
    conn.close()
    return jsonify(data)


@app.route('/api/rooms', methods=['POST'])
def add_room():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("INSERT INTO rooms (name,capacity,roomTypeId) VALUES(?,?,?)",
              (name, int(d.get('capacity', 30)), int(d.get('roomTypeId', 1))))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid}), 201


@app.route('/api/rooms/<int:rid>', methods=['PUT'])
def update_room(rid):
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("UPDATE rooms SET name=?,capacity=?,roomTypeId=? WHERE id=?",
                 (name, int(d.get('capacity', 30)), int(d.get('roomTypeId', 1)), rid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/rooms/<int:rid>', methods=['DELETE'])
def delete_room(rid):
    conn = get_db()
    conn.execute("DELETE FROM rooms WHERE id=?", (rid,))
    conn.execute("DELETE FROM timetable_slots WHERE roomId=?", (rid,))
    conn.execute("DELETE FROM substitutions WHERE substituteTeacherId=?", (rid,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── ROOM TYPES ────────────────────────────────────────────────────────────────

@app.route('/api/room_types', methods=['GET'])
def get_room_types():
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM room_types ORDER BY name"))
    conn.close()
    return jsonify(data)


@app.route('/api/room_types', methods=['POST'])
def add_room_type():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("INSERT INTO room_types (name) VALUES(?)", (name,))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid, 'name': name}), 201


@app.route('/api/room_types/<int:rid>', methods=['DELETE'])
def delete_room_type(rid):
    conn = get_db()
    conn.execute("DELETE FROM room_types WHERE id=?", (rid,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── LESSONS ───────────────────────────────────────────────────────────────────

@app.route('/api/lessons', methods=['GET'])
def get_lessons():
    conn = get_db()
    data = qrows(conn.execute("""
        SELECT l.*,t.name as teacherName,t.color as teacherColor,
               t2.name as secondTeacherName,
               s.name as subjectName,s.color as subjectColor,c.name as className
        FROM lessons l
        JOIN teachers t ON l.teacherId=t.id
        LEFT JOIN teachers t2 ON l.secondTeacherId=t2.id
        JOIN subjects s ON l.subjectId=s.id
        JOIN classes c ON l.classId=c.id
        ORDER BY c.name,s.name"""))
    for row in data:
        row['combinedClassIds'] = [r['classId'] for r in qrows(conn.execute(
            "SELECT classId FROM lesson_combined_classes WHERE lessonId=? ORDER BY classId", (row['id'],)))]
    conn.close()
    return jsonify(data)


@app.route('/api/lessons', methods=['POST'])
def add_lesson():
    d = request.json
    if not all([d.get('teacherId'), d.get('subjectId'), d.get('classId')]):
        return jsonify({'error': 'teacherId, subjectId, classId required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("""INSERT INTO lessons (teacherId,secondTeacherId,subjectId,classId,periodsPerWeek,blockSize,maxPerDay,weekType)
                 VALUES(?,?,?,?,?,?,?,?)""",
              (d['teacherId'], d.get('secondTeacherId', -1), d['subjectId'], d['classId'],
               int(d.get('periodsPerWeek', 1)), int(d.get('blockSize', 1)),
               int(d.get('maxPerDay', 0)), int(d.get('weekType', 0))))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid}), 201


@app.route('/api/lessons/<int:lid>', methods=['PUT'])
def update_lesson(lid):
    d = request.json
    conn = get_db()
    conn.execute("""UPDATE lessons SET teacherId=?,secondTeacherId=?,subjectId=?,classId=?,
                    periodsPerWeek=?,blockSize=?,maxPerDay=?,weekType=? WHERE id=?""",
                 (d['teacherId'], d.get('secondTeacherId', -1), d['subjectId'], d['classId'],
                  int(d.get('periodsPerWeek', 1)), int(d.get('blockSize', 1)),
                  int(d.get('maxPerDay', 0)), int(d.get('weekType', 0)), lid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/lessons/<int:lid>', methods=['DELETE'])
def delete_lesson(lid):
    conn = get_db()
    conn.execute("DELETE FROM lesson_combined_classes WHERE lessonId=?", (lid,))
    conn.execute("DELETE FROM lessons WHERE id=?", (lid,))
    conn.execute("DELETE FROM timetable_slots WHERE subjectId=? AND classId IN "
                 "(SELECT classId FROM lessons WHERE id=?)",
                 (lid, lid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── COMBINED CLASSES ────────────────────────────────────────────────────────────

@app.route('/api/lessons/<int:lid>/combined_classes', methods=['GET'])
def get_combined_classes(lid):
    conn = get_db()
    data = [row['classId'] for row in qrows(conn.execute(
        "SELECT classId FROM lesson_combined_classes WHERE lessonId=? ORDER BY classId", (lid,)))]
    conn.close()
    return jsonify(data)

@app.route('/api/lessons/<int:lid>/combined_classes', methods=['POST'])
def set_combined_classes(lid):
    d = request.json or {}
    class_ids = d.get('classIds', [])
    conn = get_db()
    conn.execute("DELETE FROM lesson_combined_classes WHERE lessonId=?", (lid,))
    for cid in class_ids:
        conn.execute("INSERT OR IGNORE INTO lesson_combined_classes (lessonId, classId) VALUES (?,?)", (lid, cid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── CONSTRAINTS / PREFERENCES ─────────────────────────────────────────────────

@app.route('/api/constraints/<int:tid>', methods=['GET'])
def get_constraints(tid):
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM teacher_constraints WHERE teacherId=?", (tid,)))
    conn.close()
    return jsonify(data)


@app.route('/api/constraints', methods=['POST'])
def set_constraint():
    d = request.json
    conn = get_db()
    conn.execute("INSERT OR IGNORE INTO teacher_constraints (teacherId,dayId,periodId) VALUES(?,?,?)",
                 (d['teacherId'], d['dayId'], d['periodId']))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/constraints/remove', methods=['POST'])
def remove_constraint():
    d = request.json
    conn = get_db()
    conn.execute("DELETE FROM teacher_constraints WHERE teacherId=? AND dayId=? AND periodId=?",
                 (d['teacherId'], d['dayId'], d['periodId']))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/preferences/<int:tid>', methods=['GET'])
def get_preferences(tid):
    conn = get_db()
    data = qrows(conn.execute("SELECT * FROM teacher_preferences WHERE teacherId=?", (tid,)))
    conn.close()
    return jsonify(data)


@app.route('/api/preferences', methods=['POST'])
def set_preference():
    d = request.json
    conn = get_db()
    conn.execute("""INSERT INTO teacher_preferences (teacherId,dayId,periodId,prefType) VALUES(?,?,?,?)
                    ON CONFLICT(teacherId,dayId,periodId) DO UPDATE SET prefType=excluded.prefType""",
                 (d['teacherId'], d['dayId'], d['periodId'], d.get('prefType', 'NEUTRAL')))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── GENERATE ──────────────────────────────────────────────────────────────────

@app.route('/api/generate', methods=['POST'])
def generate_timetable():
    d = request.json or {}
    seed = int(d.get('seed', 42))
    algorithm = d.get('algorithm', 'backtrack')  # 'backtrack' or 'greedy'
    conn = get_db()
    lessons_data = qrows(conn.execute("SELECT * FROM lessons"))
    rooms_data = qrows(conn.execute("SELECT * FROM rooms"))
    constraints_data = qrows(conn.execute("SELECT * FROM teacher_constraints"))
    prefs_data = qrows(conn.execute("SELECT * FROM teacher_preferences"))
    teacher_data = qrows(conn.execute("SELECT * FROM teachers"))
    # Load locked slots (they are preserved)
    locked_slots = qrows(conn.execute("SELECT * FROM timetable_slots WHERE locked=1"))
    # Load combined classes for each lesson
    combined_map = {}
    for row in qrows(conn.execute("SELECT lessonId, classId FROM lesson_combined_classes ORDER BY lessonId, classId")):
        lid = row['lessonId']
        if lid not in combined_map:
            combined_map[lid] = []
        combined_map[lid].append(row['classId'])
    for lesson in lessons_data:
        lesson['combinedClassIds'] = combined_map.get(lesson['id'], [])
    conn.close()
    if not lessons_data:
        return jsonify({'error': 'No lessons defined. Add lessons first.'}), 400

    if algorithm == 'backtrack':
        result = _solve_backtrack(lessons_data, rooms_data, constraints_data, prefs_data,
                                  teacher_data, locked_slots, seed)
    else:
        result = _solve_greedy(lessons_data, rooms_data, constraints_data, prefs_data,
                               teacher_data, locked_slots, seed)

    _save_timetable(result, locked_slots)
    return jsonify(result)


def _build_pref_map(prefs_data):
    pm = {}
    for p in prefs_data:
        pm[(p['teacherId'], p['dayId'], p['periodId'])] = p['prefType']
    return pm


def _slot_score(tid, day, period, pref_map, teacher_busy, max_consecutive, day_count):
    """Score a candidate slot (higher=better). Used to order options."""
    score = 0
    pref = pref_map.get((tid, day, period), 'NEUTRAL')
    if pref == 'PREFERRED':
        score += 10
    elif pref == 'UNDESIRABLE':
        score -= 5
    # Penalise same-day stacking
    score -= day_count.get(day, 0) * 2
    # Penalise consecutive overload
    if max_consecutive > 0:
        consecutive = 0
        for pid in range(max(1, period - max_consecutive), period):
            if teacher_busy.get((tid, day, pid)):
                consecutive += 1
        if consecutive >= max_consecutive:
            score -= 20
    return score


def _is_busy_week(busy_map, key, week_type):
    """Check if resource is busy considering week type. week_type: 0=every, 1=A, 2=B."""
    if (key + (0,)) in busy_map:
        return True  # every-week blocks everything
    if week_type == 1 and (key + (1,)) in busy_map:
        return True
    if week_type == 2 and (key + (2,)) in busy_map:
        return True
    return False


def _mark_busy_week(busy_map, key, week_type):
    busy_map[key + (week_type,)] = True


def _unmark_busy_week(busy_map, key, week_type):
    busy_map.pop(key + (week_type,), None)


def _solve_backtrack(lessons, rooms, constraints, prefs, teachers, locked_slots, seed=42):
    """
    Week-aware constraint-based greedy solver with soft-constraint scoring.
    Respects locked slots (pre-placed, immovable) and weekType.
    """
    rng = random.Random(seed)
    blocked = {(c['teacherId'], c['dayId'], c['periodId']) for c in constraints}
    pref_map = _build_pref_map(prefs)
    teacher_max_consec = {t['id']: int(t.get('maxConsecutive', 0) or 0) for t in teachers}

    teacher_busy = {}
    class_busy = {}
    room_busy = {}
    soft_violations = 0

    # Pre-fill locked slots
    for s in locked_slots:
        wt = int(s.get('weekType', 0))
        _mark_busy_week(teacher_busy, (s['teacherId'], s['dayId'], s['periodId']), wt)
        _mark_busy_week(class_busy, (s['classId'], s['dayId'], s['periodId']), wt)
        if s['roomId']:
            _mark_busy_week(room_busy, (s['roomId'], s['dayId'], s['periodId']), wt)

    all_rooms = rooms[:]
    slots = []
    unscheduled = []

    def difficulty(lesson):
        tid = lesson['teacherId']
        avail = sum(1 for d in DAYS for p in PERIODS
                    if (tid, d['id'], p['id']) not in blocked)
        return (-lesson['periodsPerWeek'], avail)

    sorted_lessons = sorted(lessons, key=difficulty)

    for lesson in sorted_lessons:
        tid = lesson['teacherId']
        sid = lesson['subjectId']
        cid = lesson['classId']
        ppw = lesson['periodsPerWeek']
        mpd = lesson.get('maxPerDay', 0)
        wt = int(lesson.get('weekType', 0))
        second_tid_raw = lesson.get('secondTeacherId')
        second_tid = -1 if second_tid_raw is None else int(second_tid_raw)
        combined_ids = lesson.get('combinedClassIds', [])

        all_class_ids = [cid] + combined_ids
        all_teacher_ids = [tid]
        if second_tid >= 0:
            all_teacher_ids.append(second_tid)

        max_consec = teacher_max_consec.get(tid, 0)
        placed = 0
        day_count = {}

        candidates = []
        for d in DAYS:
            for p in PERIODS:
                did, pid = d['id'], p['id']
                if (tid, did, pid) in blocked:
                    continue
                # Check all teachers
                any_teacher_busy = any(_is_busy_week(teacher_busy, (atid, did, pid), wt) for atid in all_teacher_ids)
                if any_teacher_busy:
                    continue
                # Check all classes
                any_class_busy = any(_is_busy_week(class_busy, (acid, did, pid), wt) for acid in all_class_ids)
                if any_class_busy:
                    continue
                sc = _slot_score(tid, did, pid, pref_map, teacher_busy, max_consec, {})
                candidates.append((sc, rng.random(), did, pid))

        candidates.sort(key=lambda x: (-x[0], x[1]))

        for _, _, day, period in candidates:
            if placed >= ppw:
                break
            any_teacher_busy = any(_is_busy_week(teacher_busy, (atid, day, period), wt) for atid in all_teacher_ids)
            if any_teacher_busy:
                continue
            any_class_busy = any(_is_busy_week(class_busy, (acid, day, period), wt) for acid in all_class_ids)
            if any_class_busy:
                continue
            if mpd and day_count.get(day, 0) >= mpd:
                continue

            if max_consec > 0:
                consecutive = sum(1 for pid in range(max(1, period - max_consec), period)
                                  if any(_is_busy_week(teacher_busy, (atid, day, pid), wt) for atid in all_teacher_ids))
                if consecutive >= max_consec:
                    soft_violations += 1
                    continue

            room_id = None
            for rm in all_rooms:
                if rm.get('id') and not _is_busy_week(room_busy, (rm['id'], day, period), wt):
                    room_id = rm['id']
                    break

            pref = pref_map.get((tid, day, period), 'NEUTRAL')
            if pref == 'UNDESIRABLE':
                soft_violations += 1

            for atid in all_teacher_ids:
                _mark_busy_week(teacher_busy, (atid, day, period), wt)
            for acid in all_class_ids:
                _mark_busy_week(class_busy, (acid, day, period), wt)
            if room_id:
                _mark_busy_week(room_busy, (room_id, day, period), wt)
            day_count[day] = day_count.get(day, 0) + 1
            slots.append({'classId': cid, 'dayId': day, 'periodId': period,
                          'subjectId': sid, 'teacherId': tid, 'roomId': room_id,
                          'locked': 0, 'weekType': wt})
            placed += 1

        if placed < ppw:
            unscheduled.append({
                'subjectId': sid, 'classId': cid, 'teacherId': tid,
                'periodsCount': ppw - placed,
                'placed': placed, 'required': ppw,
                'reason': 'Only %d of %d periods could be placed (constraint conflict)' % (placed, ppw)
            })

    hard_score = max(0, 1000 - len(unscheduled) * 100)
    soft_score = max(0, hard_score - soft_violations * 2)
    return {
        'slots': slots,
        'unscheduled': unscheduled,
        'score': soft_score,
        'hardScore': hard_score,
        'softViolations': soft_violations,
        'algorithm': 'backtrack'
    }


def _solve_greedy(lessons, rooms, constraints, prefs, teachers, locked_slots, seed=42):
    """Fast greedy solver — random shuffle, no backtracking. Week-aware."""
    rng = random.Random(seed)
    blocked = {(c['teacherId'], c['dayId'], c['periodId']) for c in constraints}

    teacher_busy, class_busy, room_busy = {}, {}, {}
    for s in locked_slots:
        wt = int(s.get('weekType', 0))
        _mark_busy_week(teacher_busy, (s['teacherId'], s['dayId'], s['periodId']), wt)
        _mark_busy_week(class_busy, (s['classId'], s['dayId'], s['periodId']), wt)
        if s['roomId']:
            _mark_busy_week(room_busy, (s['roomId'], s['dayId'], s['periodId']), wt)

    all_rooms = rooms[:]
    slots, unscheduled = [], []

    for lesson in sorted(lessons, key=lambda x: -x['periodsPerWeek']):
        tid = lesson['teacherId']
        sid = lesson['subjectId']
        cid = lesson['classId']
        ppw = lesson['periodsPerWeek']
        mpd = lesson.get('maxPerDay', 0)
        wt = int(lesson.get('weekType', 0))
        second_tid_raw = lesson.get('secondTeacherId')
        second_tid = -1 if second_tid_raw is None else int(second_tid_raw)
        combined_ids = lesson.get('combinedClassIds', [])

        all_class_ids = [cid] + combined_ids
        all_teacher_ids = [tid]
        if second_tid >= 0:
            all_teacher_ids.append(second_tid)

        placed = 0
        day_count = {}
        options = [(d['id'], p['id']) for d in DAYS for p in PERIODS
                   if (tid, d['id'], p['id']) not in blocked]
        rng.shuffle(options)
        for day, period in options:
            if placed >= ppw:
                break
            if mpd and day_count.get(day, 0) >= mpd:
                continue
            any_teacher_busy = any(_is_busy_week(teacher_busy, (atid, day, period), wt) for atid in all_teacher_ids)
            if any_teacher_busy:
                continue
            any_class_busy = any(_is_busy_week(class_busy, (acid, day, period), wt) for acid in all_class_ids)
            if any_class_busy:
                continue
            room_id = None
            for rm in all_rooms:
                if rm.get('id') and not _is_busy_week(room_busy, (rm['id'], day, period), wt):
                    room_id = rm['id']
                    break
            for atid in all_teacher_ids:
                _mark_busy_week(teacher_busy, (atid, day, period), wt)
            for acid in all_class_ids:
                _mark_busy_week(class_busy, (acid, day, period), wt)
            if room_id:
                _mark_busy_week(room_busy, (room_id, day, period), wt)
            day_count[day] = day_count.get(day, 0) + 1
            slots.append({'classId': cid, 'dayId': day, 'periodId': period,
                          'subjectId': sid, 'teacherId': tid, 'roomId': room_id,
                          'locked': 0, 'weekType': wt})
            placed += 1
        if placed < ppw:
            unscheduled.append({'subjectId': sid, 'classId': cid, 'teacherId': tid,
                                 'periodsCount': ppw - placed, 'placed': placed, 'required': ppw,
                                 'reason': 'Placed %d/%d periods' % (placed, ppw)})

    score = max(0, 1000 - len(unscheduled) * 100)
    return {'slots': slots, 'unscheduled': unscheduled, 'score': score,
            'hardScore': score, 'softViolations': 0, 'algorithm': 'greedy'}


def _save_timetable(data, locked_slots):
    conn = get_db()
    conn.execute("BEGIN")
    try:
        # Remove only unlocked slots
        conn.execute("DELETE FROM timetable_slots WHERE locked=0")
        for s in data.get('slots', []):
            conn.execute("""INSERT OR IGNORE INTO timetable_slots
                            (classId,dayId,periodId,subjectId,teacherId,roomId,locked,weekType) VALUES(?,?,?,?,?,?,0,?)""",
                         (s['classId'], s['dayId'], s['periodId'],
                          s.get('subjectId'), s.get('teacherId'), s.get('roomId'),
                          s.get('weekType', 0)))
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


# ── TIMETABLE ─────────────────────────────────────────────────────────────────

@app.route('/api/timetable', methods=['GET'])
def get_timetable():
    conn = get_db()
    data = qrows(conn.execute("""
        SELECT ts.*,s.name as subjectName,s.color as subjectColor,
               t.name as teacherName,r.name as roomName,c.name as className
        FROM timetable_slots ts
        LEFT JOIN subjects s ON ts.subjectId=s.id
        LEFT JOIN teachers t ON ts.teacherId=t.id
        LEFT JOIN rooms r ON ts.roomId=r.id
        LEFT JOIN classes c ON ts.classId=c.id"""))
    conn.close()
    return jsonify(data)


@app.route('/api/timetable/move', methods=['POST'])
def move_slot():
    d = request.json
    cid = d['classId']
    fd, fp, td2, tp = d['fromDay'], d['fromPeriod'], d['toDay'], d['toPeriod']
    conn = get_db()
    src = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                       (cid, fd, fp, int(d.get('fromWeekType', 0)))).fetchone()
    if not src:
        conn.close()
        return jsonify({'error': 'Source slot not found'}), 404
    if dict(src).get('locked'):
        conn.close()
        return jsonify({'error': 'This slot is locked and cannot be moved'}), 400
    target = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                          (cid, td2, tp, int(d.get('fromWeekType', 0)))).fetchone()
    if target and dict(target).get('locked'):
        conn.close()
        return jsonify({'error': 'Target slot is locked'}), 400
    if target:
        conn.execute("UPDATE timetable_slots SET dayId=?,periodId=? WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                     (fd, fp, cid, td2, tp, int(d.get('fromWeekType', 0))))
    conn.execute("UPDATE timetable_slots SET dayId=?,periodId=? WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                 (td2, tp, cid, fd, fp, int(d.get('fromWeekType', 0))))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/remove', methods=['POST'])
def remove_slot():
    d = request.json
    cid, day, period = d['classId'], d['dayId'], d['periodId']
    wt = int(d.get('weekType', 0))
    conn = get_db()
    slot = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                        (cid, day, period, wt)).fetchone()
    if slot and dict(slot).get('locked'):
        conn.close()
        return jsonify({'error': 'Slot is locked. Unlock it first.'}), 400
    conn.execute("DELETE FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                 (cid, day, period, wt))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/lock', methods=['POST'])
def lock_slot():
    d = request.json
    cid, day, period = d['classId'], d['dayId'], d['periodId']
    wt = int(d.get('weekType', 0))
    locked = 1 if d.get('locked', True) else 0
    conn = get_db()
    conn.execute("UPDATE timetable_slots SET locked=? WHERE classId=? AND dayId=? AND periodId=? AND weekType=?",
                 (locked, cid, day, period, wt))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/clear', methods=['POST'])
def clear_timetable():
    conn = get_db()
    conn.execute("DELETE FROM timetable_slots WHERE locked=0")
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/clear_all', methods=['POST'])
def clear_all_timetable():
    conn = get_db()
    conn.execute("DELETE FROM timetable_slots")
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


# ── ANALYTICS ─────────────────────────────────────────────────────────────────

@app.route('/api/analytics', methods=['GET'])
def get_analytics():
    conn = get_db()
    teacher_load = qrows(conn.execute("""
        SELECT t.id,t.name,t.color,COUNT(ts.id) as scheduledPeriods,
               COALESCE((SELECT SUM(l.periodsPerWeek) FROM lessons l WHERE l.teacherId=t.id),0) as totalRequired
        FROM teachers t LEFT JOIN timetable_slots ts ON ts.teacherId=t.id
        GROUP BY t.id ORDER BY t.name"""))
    room_util = qrows(conn.execute("""
        SELECT r.id,r.name,COUNT(ts.id) as used,40 as total
        FROM rooms r LEFT JOIN timetable_slots ts ON ts.roomId=r.id
        GROUP BY r.id ORDER BY r.name"""))
    total = conn.execute("SELECT COUNT(*) as c FROM timetable_slots").fetchone()['c']
    locked = conn.execute("SELECT COUNT(*) as c FROM timetable_slots WHERE locked=1").fetchone()['c']
    # Day distribution
    day_dist = qrows(conn.execute("""
        SELECT dayId, COUNT(*) as cnt FROM timetable_slots GROUP BY dayId ORDER BY dayId"""))
    # Period distribution
    period_dist = qrows(conn.execute("""
        SELECT periodId, COUNT(*) as cnt FROM timetable_slots GROUP BY periodId ORDER BY periodId"""))
    # Per class summary
    class_summary = qrows(conn.execute("""
        SELECT c.name, COUNT(ts.id) as scheduled,
               COALESCE((SELECT SUM(l.periodsPerWeek) FROM lessons l WHERE l.classId=c.id),0) as required
        FROM classes c LEFT JOIN timetable_slots ts ON ts.classId=c.id
        GROUP BY c.id ORDER BY c.name"""))

    # Subject distribution per class
    subject_dist = qrows(conn.execute("""
        SELECT c.name as className, sub.name as subjectName, COUNT(ts.id) as slots
        FROM classes c
        JOIN timetable_slots ts ON ts.classId=c.id
        JOIN subjects sub ON ts.subjectId=sub.id
        GROUP BY c.id, ts.subjectId
        ORDER BY c.name, slots DESC"""))

    # Gap statistics per class
    slots = qrows(conn.execute("""
        SELECT classId, dayId, periodId, subjectId
        FROM timetable_slots ORDER BY classId, dayId, periodId"""))
    gap_by_class = {}
    for s in slots:
        cid = s['classId']
        if cid not in gap_by_class:
            gap_by_class[cid] = {}
        if s['dayId'] not in gap_by_class[cid]:
            gap_by_class[cid][s['dayId']] = []
        gap_by_class[cid][s['dayId']].append(s['periodId'])

    gap_stats_list = []
    class_names = {r['id']: r['name'] for r in qrows(conn.execute("SELECT id, name FROM classes"))}
    for cid, days in gap_by_class.items():
        total_gaps = 0
        max_gap = 0
        gap_periods = 0
        class_name = class_names.get(cid, f"Class {cid}")
        for day_id, periods in days.items():
            periods.sort()
            for i in range(1, len(periods)):
                gap = periods[i] - periods[i-1] - 1
                if gap > 0:
                    total_gaps += gap
                    max_gap = max(max_gap, gap)
                    gap_periods += gap
        gap_stats_list.append({
            'className': class_name,
            'totalGaps': total_gaps,
            'maxGap': max_gap,
            'gapPeriods': gap_periods
        })

    # Week type distribution
    week_dist = qrows(conn.execute("""
        SELECT weekType, COUNT(*) as cnt FROM timetable_slots GROUP BY weekType ORDER BY weekType"""))

    conn.close()
    return jsonify({
        'teacherLoad': teacher_load,
        'roomUtilization': room_util,
        'totalScheduled': total,
        'lockedSlots': locked,
        'dayDistribution': day_dist,
        'periodDistribution': period_dist,
        'classSummary': class_summary,
        'subjectDistribution': subject_dist,
        'gapStats': gap_stats_list,
        'weekTypeDistribution': week_dist,
    })


# ── META ──────────────────────────────────────────────────────────────────────

@app.route('/api/meta', methods=['GET'])
def get_meta():
    return jsonify({'days': DAYS, 'periods': PERIODS})


# ── DATA IMPORT / EXPORT ──────────────────────────────────────────────────────

@app.route('/api/data/export', methods=['GET'])
def export_data():
    conn = get_db()
    data = {
        'version': 1,
        'teachers': qrows(conn.execute("SELECT * FROM teachers")),
        'subjects': qrows(conn.execute("SELECT * FROM subjects")),
        'classes': qrows(conn.execute("SELECT * FROM classes")),
        'rooms': qrows(conn.execute("SELECT * FROM rooms")),
        'room_types': qrows(conn.execute("SELECT * FROM room_types")),
        'lessons': qrows(conn.execute("SELECT * FROM lessons")),
        'teacher_constraints': qrows(conn.execute("SELECT * FROM teacher_constraints")),
        'teacher_preferences': qrows(conn.execute("SELECT * FROM teacher_preferences")),
        'divisions': qrows(conn.execute("SELECT * FROM divisions")),
        'substitutions': qrows(conn.execute("SELECT * FROM substitutions")),
    }
    conn.close()
    return Response(
        json.dumps(data, indent=2),
        mimetype='application/json',
        headers={'Content-Disposition': 'attachment;filename=thorium234_data.json'}
    )


@app.route('/api/data/import', methods=['POST'])
def import_data():
    try:
        data = request.json
        if not data or data.get('version') != 1:
            return jsonify({'error': 'Invalid import file (version mismatch)'}), 400
        conn = get_db()
        conn.execute("BEGIN")
        try:
            # Clear existing
            for tbl in ['teacher_preferences', 'teacher_constraints', 'lessons',
                        'timetable_slots', 'substitutions', 'divisions',
                        'rooms', 'classes', 'subjects', 'teachers', 'room_types']:
                conn.execute('DELETE FROM ' + tbl)
            # Insert teachers
            for t in data.get('teachers', []):
                conn.execute("INSERT INTO teachers (id,name,maxConsecutive,color) VALUES(?,?,?,?)",
                             (t['id'], t['name'], int(t.get('maxConsecutive', 0) or 0), t.get('color', '')))
            for s in data.get('subjects', []):
                conn.execute("INSERT INTO subjects (id,name,color) VALUES(?,?,?)",
                             (s['id'], s['name'], s.get('color', '')))
            for c in data.get('classes', []):
                conn.execute("INSERT INTO classes (id,name,studentCount) VALUES(?,?,?)",
                             (c['id'], c['name'], c.get('studentCount', 0)))
            for rt in data.get('room_types', []):
                conn.execute("INSERT INTO room_types (id,name) VALUES(?,?)", (rt['id'], rt['name']))
            for r in data.get('rooms', []):
                conn.execute("INSERT INTO rooms (id,name,capacity,roomTypeId) VALUES(?,?,?,?)",
                             (r['id'], r['name'], r.get('capacity', 30), r.get('roomTypeId', 1)))
            for l in data.get('lessons', []):
                conn.execute("""INSERT INTO lessons (id,teacherId,subjectId,classId,periodsPerWeek,blockSize,maxPerDay)
                                VALUES(?,?,?,?,?,?,?)""",
                             (l['id'], l['teacherId'], l['subjectId'], l['classId'],
                              l.get('periodsPerWeek', 1), l.get('blockSize', 1), l.get('maxPerDay', 0)))
            for tc in data.get('teacher_constraints', []):
                conn.execute("INSERT OR IGNORE INTO teacher_constraints (teacherId,dayId,periodId) VALUES(?,?,?)",
                             (tc['teacherId'], tc['dayId'], tc['periodId']))
            for tp in data.get('teacher_preferences', []):
                conn.execute("""INSERT OR IGNORE INTO teacher_preferences (teacherId,dayId,periodId,prefType)
                                VALUES(?,?,?,?)""",
                             (tp['teacherId'], tp['dayId'], tp['periodId'], tp.get('prefType', 'NEUTRAL')))
            for d in data.get('divisions', []):
                conn.execute("INSERT OR IGNORE INTO divisions (id,name,canRunInParallel) VALUES(?,?,?)",
                             (d['id'], d['name'], int(d.get('canRunInParallel', 1))))
            for s in data.get('substitutions', []):
                conn.execute("""INSERT OR IGNORE INTO substitutions
                                (id,originalTeacherId,substituteTeacherId,subjectId,classId,dayId,periodId,status,reason,date)
                                VALUES(?,?,?,?,?,?,?,?,?,?)""",
                             (s['id'], s['originalTeacherId'], s.get('substituteTeacherId', -1),
                              s['subjectId'], s['classId'], s['dayId'], s['periodId'],
                              s.get('status', 'PENDING'), s.get('reason', ''), s.get('date', '')))
            conn.commit()
        except Exception:
            conn.rollback()
            raise
        finally:
            conn.close()
        return jsonify({'ok': True})
    except Exception as e:
        return jsonify({'error': str(e)}), 500


# ── HTML EXPORT ───────────────────────────────────────────────────────────────

@app.route('/api/export/html')
def export_html():
    view = request.args.get('view', 'class')
    week = request.args.get('week', '0')
    conn = get_db()
    slot_data = qrows(conn.execute("""
        SELECT ts.*,s.name as subjectName,s.color as subjectColor,
               t.name as teacherName,r.name as roomName,c.name as className
        FROM timetable_slots ts
        LEFT JOIN subjects s ON ts.subjectId=s.id
        LEFT JOIN teachers t ON ts.teacherId=t.id
        LEFT JOIN rooms r ON ts.roomId=r.id
        LEFT JOIN classes c ON ts.classId=c.id"""))
    unscheduled = _build_unscheduled_list(conn)
    conn.close()
    html = _build_html(slot_data, unscheduled, view, week)
    return Response(html, mimetype='text/html',
                    headers={'Content-Disposition': 'attachment;filename=timetable.html'})


@app.route('/api/export/pdf')
def export_pdf():
    view = request.args.get('view', 'class')
    week = request.args.get('week', '0')
    conn = get_db()
    slot_data = qrows(conn.execute("""
        SELECT ts.*,s.name as subjectName,s.color as subjectColor,
               t.name as teacherName,r.name as roomName,c.name as className
        FROM timetable_slots ts
        LEFT JOIN subjects s ON ts.subjectId=s.id
        LEFT JOIN teachers t ON ts.teacherId=t.id
        LEFT JOIN rooms r ON ts.roomId=r.id
        LEFT JOIN classes c ON ts.classId=c.id"""))
    unscheduled = _build_unscheduled_list(conn)
    conn.close()
    html = _build_html(slot_data, unscheduled, view, week)
    try:
        import pdfkit
        pdf = pdfkit.from_string(html, options={
            'page-size': 'A3', 'orientation': 'Landscape',
            'margin-top': '8mm', 'margin-bottom': '8mm',
            'margin-left': '8mm', 'margin-right': '8mm',
            'enable-local-file-access': '',
            'no-outline': '',
            'encoding': 'UTF-8',
        })
        return Response(pdf, mimetype='application/pdf',
                        headers={'Content-Disposition': 'attachment;filename=timetable.pdf'})
    except (ImportError, OSError, IOError) as e:
        return Response(html, mimetype='text/html',
                        headers={'Content-Disposition': 'attachment;filename=timetable.html'})


def _build_html(slots, unscheduled, view, week='0'):
    now = datetime.now().strftime('%Y-%m-%d %H:%M')
    view_label = {'class': 'Class', 'teacher': 'Teacher', 'room': 'Room'}.get(view, 'Class')
    week_label = {0: 'All Weeks', 1: 'Week A', 2: 'Week B'}.get(int(week), 'All Weeks')

    key = 'className' if view == 'class' else ('teacherName' if view == 'teacher' else 'roomName')
    by = {}
    for s in slots:
        e = s.get(key) or 'Unknown'
        by.setdefault(e, [])
        by[e].append(s)

    # Sort each entity's slots into a (dayId, periodId) lookup
    entity_grids = {}
    for entity, slot_list in sorted(by.items()):
        grid = {}
        for s in slot_list:
            grid[(s['dayId'], s['periodId'])] = s
        entity_grids[entity] = grid

    h = []
    h.append('''<!DOCTYPE html>
<html><head><meta charset="UTF-8">
<title>Timetable - ''' + view_label + ''' View</title>
<style>
  @page { margin: 12mm 10mm; }
  * { box-sizing: border-box; }
  body {
    font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
    font-size: 9pt; color: #1e293b; margin: 0; padding: 0;
    -webkit-print-color-adjust: exact; print-color-adjust: exact;
  }
  .report-header {
    text-align: center; padding: 24px 0 16px; margin-bottom: 20px;
    border-bottom: 3px solid #f0a500;
  }
  .report-header h1 {
    font-size: 22pt; font-weight: 800; color: #1e3a5f; margin: 0 0 4px;
  }
  .report-header .meta {
    font-size: 9pt; color: #64748b; margin: 4px 0;
  }
  .report-header .meta span { margin: 0 12px; }
  .entity-section {
    page-break-before: always; padding-top: 8px;
  }
  .entity-section:first-of-type { page-break-before: avoid; }
  .entity-title {
    font-size: 14pt; font-weight: 700; color: #1e3a5f;
    margin: 0 0 10px; padding-bottom: 6px;
    border-bottom: 2px solid #e2e8f0;
  }
  table {
    width: 100%; border-collapse: collapse; table-layout: fixed;
  }
  thead th {
    background: #1e3a5f; color: #fff; padding: 7px 4px;
    font-size: 8pt; font-weight: 600; text-align: center;
    border: 1px solid #1e3a5f; letter-spacing: 0.3px;
  }
  thead th.day-header { width: 10%; }
  thead th.period-col { width: 8%; }
  tbody td {
    border: 1px solid #cbd5e1; padding: 5px 3px;
    vertical-align: middle; text-align: center;
    height: 48px;
  }
  tbody tr:nth-child(even) td { background: #f8fafc; }
  td.period-label {
    background: #f1f5f9; font-weight: 700; font-size: 8pt;
    color: #475569; width: 8%;
  }
  td.period-label small { display: block; font-weight: 400; color: #94a3b8; font-size: 6.5pt; }
  .slot-card {
    display: inline-block; border-radius: 4px; padding: 4px 6px;
    color: #fff; width: 100%; line-height: 1.4;
  }
  .slot-subj {
    font-weight: 700; font-size: 8.5pt; display: block;
    white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
  }
  .slot-detail {
    font-size: 6.5pt; display: block; opacity: 0.9;
    white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
  }
  .slot-detail i { font-style: normal; }
  .week-badge {
    display: inline-block; font-size: 6pt; font-weight: 700;
    background: rgba(255,255,255,0.3); border-radius: 2px;
    padding: 0 3px; margin-left: 2px; vertical-align: middle;
  }
  .locked-mark {
    display: inline-block; font-size: 6pt; margin-left: 2px;
  }
  .empty-cell { color: #cbd5e1; font-size: 9pt; }

  /* Unscheduled section */
  .unsched-section { page-break-before: always; }
  .unsched-title {
    font-size: 14pt; font-weight: 700; color: #dc2626;
    margin: 0 0 10px; padding-bottom: 6px;
    border-bottom: 2px solid #fecaca;
  }
  .unsched-table th {
    background: #dc2626; color: #fff; padding: 7px 8px;
    font-size: 8pt; font-weight: 600; text-align: left;
    border: 1px solid #dc2626;
  }
  .unsched-table td {
    border: 1px solid #fecaca; padding: 6px 8px; font-size: 8pt;
  }
  .unsched-table tr:nth-child(even) td { background: #fef2f2; }

  @media print {
    .report-header { margin-bottom: 12px; }
  }
</style>
</head><body>
''')

    h.append('<div class="report-header">')
    h.append('<h1>School Timetable</h1>')
    h.append('<div class="meta">')
    h.append('<span>View: <strong>' + view_label + '</strong></span>')
    h.append('<span>Week: <strong>' + week_label + '</strong></span>')
    h.append('<span>Generated: <strong>' + now + '</strong></span>')
    h.append('</div></div>')

    for entity, grid in entity_grids.items():
        h.append('<div class="entity-section">')
        h.append('<div class="entity-title">' + view_label + ': ' + entity + '</div>')
        h.append('<table><thead><tr>')
        h.append('<th class="period-col">Period</th>')
        for d in DAYS:
            h.append('<th class="day-header">' + d['name'] + '</th>')
        h.append('</tr></thead><tbody>')

        for p in PERIODS:
            pi = p['id']
            h.append('<tr>')
            h.append('<td class="period-label">P' + str(pi) + '<small>' + p['start'] + '</small></td>')
            for d in DAYS:
                di = d['id']
                s = grid.get((di, pi))
                if s:
                    col = s.get('subjectColor') or '#4A90D9'
                    wt = s.get('weekType', 0)
                    locked = s.get('locked', False)
                    wt_badge = ''
                    if wt == 1:
                        wt_badge = '<span class="week-badge">A</span>'
                    elif wt == 2:
                        wt_badge = '<span class="week-badge">B</span>'
                    lock_mark = '&#128274; ' if locked else ''
                    h.append('<td><div class="slot-card" style="background:' + col + '">')
                    h.append('<span class="slot-subj">' + lock_mark + (s['subjectName'] or '') + wt_badge + '</span>')
                    if view == 'class':
                        h.append('<span class="slot-detail"><i>' + (s['teacherName'] or '') + '</i> &middot; ' + (s.get('roomName', '') or '') + '</span>')
                    elif view == 'teacher':
                        h.append('<span class="slot-detail"><i>' + (s['className'] or '') + '</i> &middot; ' + (s.get('roomName', '') or '') + '</span>')
                    else:
                        h.append('<span class="slot-detail"><i>' + (s['className'] or '') + '</i> &middot; ' + (s['teacherName'] or '') + '</span>')
                    h.append('</div></td>')
                else:
                    h.append('<td><span class="empty-cell">&mdash;</span></td>')
            h.append('</tr>')

        h.append('</tbody></table></div>')

    if unscheduled:
        h.append('<div class="unsched-section">')
        h.append('<div class="unsched-title">Unscheduled Lessons (' + str(len(unscheduled)) + ')</div>')
        h.append('<table class="unsched-table"><thead><tr>')
        h.append('<th style="width:25%">Class</th>')
        h.append('<th style="width:25%">Subject</th>')
        h.append('<th style="width:20%">Teacher</th>')
        h.append('<th style="width:12%">Unscheduled Periods</th>')
        h.append('<th style="width:18%">Reason</th>')
        h.append('</tr></thead><tbody>')
        for u in unscheduled:
            h.append('<tr>')
            h.append('<td><strong>' + (u.get('className') or '') + '</strong></td>')
            h.append('<td>' + (u.get('subjectName') or '') + '</td>')
            h.append('<td>' + (u.get('teacherName') or '') + '</td>')
            h.append('<td>' + str(u.get('periodsCount', 0)) + '</td>')
            h.append('<td>' + (u.get('reason') or 'Insufficient available slots') + '</td>')
            h.append('</tr>')
        h.append('</tbody></table></div>')

    h.append('</body></html>')
    return ''.join(h)


# ── CONFLICT DETECTION ────────────────────────────────────────────────────────

@app.route('/api/conflicts', methods=['GET'])
def get_conflicts():
    conn = get_db()
    slots = qrows(conn.execute("""
        SELECT ts.*,s.name as subjectName,t.name as teacherName,c.name as className
        FROM timetable_slots ts
        LEFT JOIN subjects s ON ts.subjectId=s.id
        LEFT JOIN teachers t ON ts.teacherId=t.id
        LEFT JOIN classes c ON ts.classId=c.id"""))
    conn.close()
    conflicts = []
    # Teacher double-booking
    teacher_slots = {}
    for s in slots:
        key = (s['teacherId'], s['dayId'], s['periodId'])
        if key in teacher_slots:
            conflicts.append({
                'type': 'teacher_double',
                'description': 'Teacher %s double-booked on Day %d Period %d' % (
                    s['teacherName'], s['dayId'], s['periodId']),
                'dayId': s['dayId'], 'periodId': s['periodId'],
            })
        else:
            teacher_slots[key] = s
    # Room double-booking
    room_slots = {}
    for s in slots:
        if not s['roomId']:
            continue
        key = (s['roomId'], s['dayId'], s['periodId'])
        if key in room_slots:
            conflicts.append({
                'type': 'room_double',
                'description': 'Room double-booked on Day %d Period %d' % (s['dayId'], s['periodId']),
                'dayId': s['dayId'], 'periodId': s['periodId'],
            })
        else:
            room_slots[key] = s
    return jsonify(conflicts)


# ── SUBSTITUTIONS ─────────────────────────────────────────────────────────────

@app.route('/api/substitutions', methods=['GET'])
def get_substitutions():
    conn = get_db()
    conn.execute("""
        CREATE TABLE IF NOT EXISTS substitutions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            originalTeacherId INTEGER NOT NULL,
            substituteTeacherId INTEGER NOT NULL DEFAULT -1,
            subjectId INTEGER NOT NULL,
            classId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'PENDING',
            reason TEXT DEFAULT '',
            date TEXT DEFAULT ''
        )""")
    data = qrows(conn.execute("""
        SELECT s.*,t1.name as origTeacherName,t2.name as subTeacherName,
               sub.name as subjectName,c.name as className
        FROM substitutions s
        LEFT JOIN teachers t1 ON s.originalTeacherId=t1.id
        LEFT JOIN teachers t2 ON s.substituteTeacherId=t2.id
        LEFT JOIN subjects sub ON s.subjectId=sub.id
        LEFT JOIN classes c ON s.classId=c.id
        ORDER BY s.id DESC"""))
    conn.close()
    return jsonify(data)

@app.route('/api/substitutions', methods=['POST'])
def add_substitution():
    d = request.json
    conn = get_db()
    conn.execute("""
        CREATE TABLE IF NOT EXISTS substitutions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            originalTeacherId INTEGER NOT NULL,
            substituteTeacherId INTEGER NOT NULL DEFAULT -1,
            subjectId INTEGER NOT NULL,
            classId INTEGER NOT NULL,
            dayId INTEGER NOT NULL,
            periodId INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'PENDING',
            reason TEXT DEFAULT '',
            date TEXT DEFAULT ''
        )""")
    c = conn.cursor()
    c.execute("""INSERT INTO substitutions
                 (originalTeacherId,substituteTeacherId,subjectId,classId,dayId,periodId,status,reason,date)
                 VALUES(?,?,?,?,?,?,'PENDING',?,?)""",
              (d['originalTeacherId'], d.get('substituteTeacherId', -1),
               d['subjectId'], d['classId'], d['dayId'], d['periodId'],
               d.get('reason', ''), d.get('date', '')))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid}), 201

@app.route('/api/substitutions/<int:sid>/status', methods=['PUT'])
def update_substitution_status(sid):
    d = request.json
    status = d.get('status', 'PENDING')
    conn = get_db()
    conn.execute("UPDATE substitutions SET status=? WHERE id=?", (status, sid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/substitutions/<int:sid>', methods=['DELETE'])
def delete_substitution(sid):
    conn = get_db()
    conn.execute("DELETE FROM substitutions WHERE id=?", (sid,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/substitutions/available', methods=['GET'])
def get_available_substitutes():
    """Return teachers who are free for a given (day,period)."""
    day = request.args.get('day', type=int)
    period = request.args.get('period', type=int)
    if not day or not period:
        return jsonify({'error': 'day and period required'}), 400
    conn = get_db()
    busy = set()
    for row in conn.execute("SELECT teacherId FROM timetable_slots WHERE dayId=? AND periodId=?", (day, period)):
        busy.add(row['teacherId'])
    for row in conn.execute("SELECT teacherId FROM teacher_constraints WHERE dayId=? AND periodId=?", (day, period)):
        busy.add(row['teacherId'])
    teachers = qrows(conn.execute("SELECT * FROM teachers ORDER BY name"))
    conn.close()
    available = [t for t in teachers if t['id'] not in busy]
    return jsonify(available)

@app.route('/api/substitutions/suggest', methods=['GET'])
def suggest_substitute_teachers():
    """Rank potential substitute teachers by subject match + availability + workload."""
    absent = request.args.get('absentTeacherId', type=int)
    subject_id = request.args.get('subjectId', type=int)
    day = request.args.get('dayId', type=int)
    period = request.args.get('periodId', type=int)
    if not absent or not subject_id or not day or not period:
        return jsonify({'error': 'absentTeacherId, subjectId, dayId, periodId required'}), 400

    conn = get_db()

    busy_teachers = set()
    for row in conn.execute("SELECT teacherId FROM timetable_slots WHERE dayId=? AND periodId=?", (day, period)):
        busy_teachers.add(row['teacherId'])
    for row in conn.execute("SELECT teacherId FROM teacher_constraints WHERE dayId=? AND periodId=?", (day, period)):
        busy_teachers.add(row['teacherId'])

    teachers = qrows(conn.execute("SELECT * FROM teachers ORDER BY name"))
    lessons = qrows(conn.execute("SELECT * FROM lessons"))

    suggestions = []
    for t in teachers:
        if t['id'] == absent:
            continue

        score = 0
        reasons = []

        teaches_subject = any(
            l['subjectId'] == subject_id and (l['teacherId'] == t['id'] or l.get('secondTeacherId') == t['id'])
            for l in lessons
        )
        if teaches_subject:
            score += 50
            reasons.append('Teaches this subject')
        else:
            reasons.append('Does not teach this subject')

        is_free = t['id'] not in busy_teachers
        if is_free:
            score += 30
            reasons.append('Free at this period')
        else:
            reasons.append('Busy at this period')

        lesson_count = sum(1 for l in lessons if l['teacherId'] == t['id'] or l.get('secondTeacherId') == t['id'])
        workload_bonus = max(0, 20 - lesson_count)
        score += workload_bonus

        suggestions.append({
            'teacherId': t['id'],
            'teacherName': t['name'],
            'score': score,
            'reason': '; '.join(reasons)
        })

    suggestions.sort(key=lambda s: s['score'], reverse=True)
    conn.close()
    return jsonify(suggestions)

# ── DIVISIONS ──────────────────────────────────────────────────────────────────

@app.route('/api/divisions', methods=['GET'])
def get_divisions():
    conn = get_db()
    conn.execute("""
        CREATE TABLE IF NOT EXISTS divisions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            canRunInParallel INTEGER NOT NULL DEFAULT 1
        )""")
    # Also ensure classes have divisionId column
    try:
        conn.execute("ALTER TABLE classes ADD COLUMN divisionId INTEGER DEFAULT NULL")
        conn.execute("ALTER TABLE classes ADD COLUMN groupId INTEGER DEFAULT NULL")
        conn.commit()
    except Exception:
        pass
    data = qrows(conn.execute("SELECT * FROM divisions ORDER BY name"))
    # Attach class members
    for d in data:
        d['members'] = qrows(conn.execute(
            "SELECT id,name FROM classes WHERE divisionId=? ORDER BY name", (d['id'],)))
    conn.close()
    return jsonify(data)

@app.route('/api/divisions', methods=['POST'])
def add_division():
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("""
        CREATE TABLE IF NOT EXISTS divisions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            canRunInParallel INTEGER NOT NULL DEFAULT 1
        )""")
    c = conn.cursor()
    c.execute("INSERT INTO divisions (name,canRunInParallel) VALUES(?,?)",
              (name, int(d.get('canRunInParallel', 1))))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid, 'name': name}), 201

@app.route('/api/divisions/<int:did>', methods=['PUT'])
def update_division(did):
    d = request.json
    name = (d.get('name') or '').strip()
    if not name:
        return jsonify({'error': 'Name required'}), 400
    conn = get_db()
    conn.execute("UPDATE divisions SET name=?,canRunInParallel=? WHERE id=?",
                 (name, int(d.get('canRunInParallel', 1)), did))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/divisions/<int:did>', methods=['DELETE'])
def delete_division(did):
    conn = get_db()
    conn.execute("DELETE FROM divisions WHERE id=?", (did,))
    conn.execute("UPDATE classes SET divisionId=NULL WHERE divisionId=?", (did,))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/divisions/assign', methods=['POST'])
def assign_class_to_division():
    d = request.json
    conn = get_db()
    try:
        conn.execute("ALTER TABLE classes ADD COLUMN divisionId INTEGER DEFAULT NULL")
        conn.commit()
    except Exception:
        pass
    conn.execute("UPDATE classes SET divisionId=? WHERE id=?",
                 (d['divisionId'], d['classId']))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

@app.route('/api/divisions/unassign', methods=['POST'])
def unassign_class_from_division():
    d = request.json
    conn = get_db()
    conn.execute("UPDATE classes SET divisionId=NULL WHERE id=?", (d['classId'],))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})

# ── VERSION COMPARISON ─────────────────────────────────────────────────────────

@app.route('/api/versions/compare', methods=['POST'])
def compare_versions():
    d = request.json
    v1_id = d.get('v1')
    v2_id = d.get('v2')
    if v1_id is None or v2_id is None or v1_id >= len(_VERSIONS) or v2_id >= len(_VERSIONS):
        return jsonify({'error': 'Invalid version IDs'}), 400
    v1 = _VERSIONS[v1_id]
    v2 = _VERSIONS[v2_id]

    # Index slots by (classId, dayId, periodId)
    idx1 = {(s['classId'], s['dayId'], s['periodId']): s for s in v1['slots']}
    idx2 = {(s['classId'], s['dayId'], s['periodId']): s for s in v2['slots']}

    added = []
    removed = []
    changed = []
    all_keys = set(idx1.keys()) | set(idx2.keys())
    for key in all_keys:
        s1 = idx1.get(key)
        s2 = idx2.get(key)
        if s1 and not s2:
            removed.append(s1)
        elif s2 and not s1:
            added.append(s2)
        elif s1 and s2 and (s1['teacherId'] != s2['teacherId'] or s1['subjectId'] != s2['subjectId']):
            changed.append({'from': s1, 'to': s2})

    return jsonify({
        'v1': {'label': v1['label'], 'score': v1['score']},
        'v2': {'label': v2['label'], 'score': v2['score']},
        'added': len(added),
        'removed': len(removed),
        'changed': len(changed),
        'addedSlots': added[:20],
        'removedSlots': removed[:20],
        'changedSlots': changed[:20],
    })

# ── ENTRY POINT ───────────────────────────────────────────────────────────────

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def index(path):
    return render_template('index.html')


# ── TIMETABLE VERSIONS ─────────────────────────────────────────────────────────

_VERSIONS = []  # list of {'label': str, 'timestamp': str, 'score': int, 'slots': list, 'unscheduled': list}

@app.route('/api/versions', methods=['GET'])
def get_versions():
    summaries = []
    for i, v in enumerate(_VERSIONS):
        summaries.append({
            'id': i,
            'label': v['label'],
            'timestamp': v['timestamp'],
            'score': v['score'],
            'slotCount': len(v['slots']),
            'unscheduledCount': len(v['unscheduled']),
        })
    return jsonify(summaries)

@app.route('/api/versions', methods=['POST'])
def save_version():
    d = request.json or {}
    label = d.get('label', '').strip() or 'Version %d' % (len(_VERSIONS) + 1)
    conn = get_db()
    slots = qrows(conn.execute("SELECT * FROM timetable_slots"))
    # Get current unscheduled from last generation
    unscheduled = _build_unscheduled_list(conn) if slots else []
    conn.close()
    _VERSIONS.append({
        'label': label,
        'timestamp': datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
        'score': 0,
        'slots': slots,
        'unscheduled': unscheduled,
    })
    return jsonify({'id': len(_VERSIONS) - 1, 'label': label}), 201

@app.route('/api/versions/<int:vid>', methods=['GET'])
def get_version(vid):
    if vid < 0 or vid >= len(_VERSIONS):
        return jsonify({'error': 'Version not found'}), 404
    return jsonify(_VERSIONS[vid])

@app.route('/api/versions/<int:vid>/restore', methods=['POST'])
def restore_version(vid):
    if vid < 0 or vid >= len(_VERSIONS):
        return jsonify({'error': 'Version not found'}), 404
    ver = _VERSIONS[vid]
    conn = get_db()
    conn.execute("DELETE FROM timetable_slots")
    for s in ver['slots']:
        conn.execute("""INSERT INTO timetable_slots
                        (classId,dayId,periodId,subjectId,teacherId,roomId,locked,weekType)
                        VALUES(?,?,?,?,?,?,?,?)""",
                     (s['classId'], s['dayId'], s['periodId'],
                      s.get('subjectId'), s.get('teacherId'), s.get('roomId'),
                      s.get('locked', 0), s.get('weekType', 0)))
    conn.commit()
    conn.close()
    return jsonify({'message': 'Restored version: ' + ver['label']})


def _build_unscheduled_list(conn):
    """Helper: build unscheduled list from the DB state."""
    lessons = qrows(conn.execute("""
        SELECT l.*, c.name as className, s.name as subjectName,
               t.name as teacherName
        FROM lessons l
        LEFT JOIN classes c ON l.classId=c.id
        LEFT JOIN subjects s ON l.subjectId=s.id
        LEFT JOIN teachers t ON l.teacherId=t.id
    """))
    # Count scheduled periods per lesson
    scheduled_counts = {}
    for row in qrows(conn.execute(
        "SELECT classId, subjectId, COUNT(*) as cnt FROM timetable_slots GROUP BY classId, subjectId"
    )):
        key = (row['classId'], row['subjectId'])
        scheduled_counts[key] = row['cnt']
    unscheduled = []
    for l in lessons:
        key = (l['classId'], l['subjectId'])
        scheduled = scheduled_counts.get(key, 0)
        if scheduled < l['periodsPerWeek']:
            unscheduled.append({
                'subjectName': l.get('subjectName', ''),
                'className': l.get('className', ''),
                'teacherName': l.get('teacherName', ''),
                'periodsCount': l['periodsPerWeek'] - scheduled,
            })
    return unscheduled


# ── C++ SOLVER BRIDGE ──────────────────────────────────────────────────────────

import subprocess as _subprocess
import json as _json

_binary_path = os.environ.get('CPP_SOLVER_PATH') or os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    'build_cmake', 'timetableGen'
)

@app.route('/api/solve', methods=['POST'])
def solve_with_cpp():
    if not os.path.exists(_binary_path):
        return jsonify({'error': 'C++ solver binary not found at ' + _binary_path}), 500
    try:
        result = _subprocess.run(
            [_binary_path, '--solve-from-db'],
            capture_output=True, text=True, timeout=120,
            cwd=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        )
        if result.returncode != 0:
            stderr = result.stderr.strip()
            return jsonify({'error': stderr or 'Solver exited with code %d' % result.returncode}), 500
        output = _json.loads(result.stdout.strip())
        return jsonify(output)
    except Exception as e:
        return jsonify({'error': str(e)}), 500


# ── SAMPLE DATA ──────────────────────────────────────────────────────────────

@app.route('/api/sample-school', methods=['GET'])
def sample_school():
    """Return realistic Kenyan secondary school data as a config template."""
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import timetable_gen as tg
    return jsonify(tg.get_sample_school())


# ── DYNAMIC TIMETABLE GENERATION ─────────────────────────────────────────────

@app.route('/api/generate-timetable', methods=['POST'])
def generate_timetable_from_config():
    """Generate a timetable from a fully user-defined configuration.

    Each class can have its own lesson_duration_minutes to support
    streams with different period lengths (e.g., Form 4: 60min, others: 40min).
    """
    import sys
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import timetable_gen as tg
    import pdf_generator as pg
    import tempfile

    data = request.json
    if not data:
        return jsonify({'error': 'Empty request body'}), 400

    try:
        parsed = tg.parse_payload(data)
    except Exception as e:
        return jsonify({'error': 'Invalid configuration: ' + str(e)}), 400

    try:
        result = tg.solve_timetable(parsed)
    except Exception as e:
        return jsonify({'error': 'Solver error: ' + str(e)}), 500

    slots = result.get('slots', [])
    unscheduled = result.get('unscheduled', [])

    days = parsed['days']
    school_name = data.get('school', {}).get(
        'name', 'School Timetable')

    output_format = request.args.get('format', 'pdf')
    if output_format == 'pdf' and pg.HAS_REPORTLAB:
        tmp = tempfile.NamedTemporaryFile(suffix='.pdf', delete=False)
        tmp.close()
        try:
            success = pg.generate_timetable_pdf(
                slot_data=slots,
                days=days,
                periods=[],
                unscheduled=unscheduled,
                output_path=tmp.name,
                title='Class Timetables',
                school_name=school_name,
                class_timelines=parsed.get('class_timelines', {}),
            )
            if success:
                with open(tmp.name, 'rb') as f:
                    pdf_data = f.read()
                os.unlink(tmp.name)
                return Response(
                    pdf_data,
                    mimetype='application/pdf',
                    headers={
                        'Content-Disposition':
                        'attachment;filename=timetable_generated.pdf'
                    },
                )
        except Exception:
            import traceback
            traceback.print_exc()
        finally:
            if os.path.exists(tmp.name):
                os.unlink(tmp.name)

    return jsonify({
        'score': result.get('score', 0),
        'slots': len(slots),
        'unscheduled': len(unscheduled),
        'slots_data': slots[:50],
        'unscheduled_data': unscheduled[:20],
    })


# ── ENTRY POINT ───────────────────────────────────────────────────────────────

if __name__ == '__main__':
    init_db()
    port = int(os.environ.get('FLASK_PORT', '5000'))
    app.run(host='127.0.0.1', port=port, debug=False)
