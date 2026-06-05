import sqlite3
import os
import random
import json
from flask import Flask, request, jsonify, render_template, Response
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

DB_PATH = os.path.join(os.path.dirname(__file__), '..', 'data', 'timetableGen.db')

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
            UNIQUE(classId, dayId, periodId)
        );
    """)
    # Migrations — add columns that may be missing in existing DBs
    for migration in [
        "ALTER TABLE timetable_slots ADD COLUMN locked INTEGER NOT NULL DEFAULT 0",
        "ALTER TABLE teachers ADD COLUMN color TEXT NOT NULL DEFAULT ''",
        "ALTER TABLE subjects ADD COLUMN color TEXT NOT NULL DEFAULT ''",
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
    for q in ["DELETE FROM teachers WHERE id=?",
              "DELETE FROM lessons WHERE teacherId=?",
              "DELETE FROM teacher_constraints WHERE teacherId=?",
              "DELETE FROM teacher_preferences WHERE teacherId=?"]:
        conn.execute(q, (tid,))
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
               s.name as subjectName,s.color as subjectColor,c.name as className
        FROM lessons l
        JOIN teachers t ON l.teacherId=t.id
        JOIN subjects s ON l.subjectId=s.id
        JOIN classes c ON l.classId=c.id
        ORDER BY c.name,s.name"""))
    conn.close()
    return jsonify(data)


@app.route('/api/lessons', methods=['POST'])
def add_lesson():
    d = request.json
    if not all([d.get('teacherId'), d.get('subjectId'), d.get('classId')]):
        return jsonify({'error': 'teacherId, subjectId, classId required'}), 400
    conn = get_db()
    c = conn.cursor()
    c.execute("""INSERT INTO lessons (teacherId,subjectId,classId,periodsPerWeek,blockSize,maxPerDay)
                 VALUES(?,?,?,?,?,?)""",
              (d['teacherId'], d['subjectId'], d['classId'],
               int(d.get('periodsPerWeek', 1)), int(d.get('blockSize', 1)),
               int(d.get('maxPerDay', 0))))
    conn.commit()
    nid = c.lastrowid
    conn.close()
    return jsonify({'id': nid}), 201


@app.route('/api/lessons/<int:lid>', methods=['PUT'])
def update_lesson(lid):
    d = request.json
    conn = get_db()
    conn.execute("""UPDATE lessons SET teacherId=?,subjectId=?,classId=?,
                    periodsPerWeek=?,blockSize=?,maxPerDay=? WHERE id=?""",
                 (d['teacherId'], d['subjectId'], d['classId'],
                  int(d.get('periodsPerWeek', 1)), int(d.get('blockSize', 1)),
                  int(d.get('maxPerDay', 0)), lid))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/lessons/<int:lid>', methods=['DELETE'])
def delete_lesson(lid):
    conn = get_db()
    conn.execute("DELETE FROM lessons WHERE id=?", (lid,))
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


def _solve_backtrack(lessons, rooms, constraints, prefs, teachers, locked_slots, seed=42):
    """
    Constraint-based greedy solver with soft-constraint scoring.
    Iterates lessons ordered by difficulty (most periods first, fewest options first),
    scores each candidate slot, picks the best available, and backtracks if stuck.
    Respects locked slots (pre-placed, immovable).
    """
    rng = random.Random(seed)
    blocked = {(c['teacherId'], c['dayId'], c['periodId']) for c in constraints}
    pref_map = _build_pref_map(prefs)
    teacher_max_consec = {t['id']: t.get('maxConsecutive', 0) for t in teachers}

    teacher_busy = {}
    class_busy = {}
    room_busy = {}
    soft_violations = 0

    # Pre-fill locked slots
    for s in locked_slots:
        teacher_busy[(s['teacherId'], s['dayId'], s['periodId'])] = True
        class_busy[(s['classId'], s['dayId'], s['periodId'])] = True
        if s['roomId']:
            room_busy[(s['roomId'], s['dayId'], s['periodId'])] = True

    all_rooms = rooms[:]
    slots = []
    unscheduled = []

    # Sort lessons: most periods/week first; ties broken by fewer available slots
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
        max_consec = teacher_max_consec.get(tid, 0)
        placed = 0
        day_count = {}

        # Build candidate list and score each
        candidates = []
        for d in DAYS:
            for p in PERIODS:
                did, pid = d['id'], p['id']
                if (tid, did, pid) in blocked:
                    continue
                if teacher_busy.get((tid, did, pid)):
                    continue
                if class_busy.get((cid, did, pid)):
                    continue
                sc = _slot_score(tid, did, pid, pref_map, teacher_busy, max_consec, {})
                candidates.append((sc, rng.random(), did, pid))

        # Sort by score desc, shuffle ties via rng
        candidates.sort(key=lambda x: (-x[0], x[1]))

        for _, _, day, period in candidates:
            if placed >= ppw:
                break
            if teacher_busy.get((tid, day, period)):
                continue
            if class_busy.get((cid, day, period)):
                continue
            if mpd and day_count.get(day, 0) >= mpd:
                continue

            # Check consecutive constraint
            if max_consec > 0:
                consecutive = sum(1 for pid in range(max(1, period - max_consec), period)
                                  if teacher_busy.get((tid, day, pid)))
                if consecutive >= max_consec:
                    soft_violations += 1
                    continue

            # Pick a room
            room_id = None
            for rm in all_rooms:
                if rm.get('id') and not room_busy.get((rm['id'], day, period)):
                    room_id = rm['id']
                    break

            # Track soft constraint violations (undesirable slots)
            pref = pref_map.get((tid, day, period), 'NEUTRAL')
            if pref == 'UNDESIRABLE':
                soft_violations += 1

            teacher_busy[(tid, day, period)] = True
            class_busy[(cid, day, period)] = True
            if room_id:
                room_busy[(room_id, day, period)] = True
            day_count[day] = day_count.get(day, 0) + 1
            slots.append({'classId': cid, 'dayId': day, 'periodId': period,
                          'subjectId': sid, 'teacherId': tid, 'roomId': room_id,
                          'locked': 0})
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
    """Fast greedy solver — random shuffle, no backtracking."""
    rng = random.Random(seed)
    blocked = {(c['teacherId'], c['dayId'], c['periodId']) for c in constraints}

    teacher_busy, class_busy, room_busy = {}, {}, {}
    for s in locked_slots:
        teacher_busy[(s['teacherId'], s['dayId'], s['periodId'])] = True
        class_busy[(s['classId'], s['dayId'], s['periodId'])] = True
        if s['roomId']:
            room_busy[(s['roomId'], s['dayId'], s['periodId'])] = True

    all_rooms = rooms[:]
    slots, unscheduled = [], []

    for lesson in sorted(lessons, key=lambda x: -x['periodsPerWeek']):
        tid = lesson['teacherId']
        sid = lesson['subjectId']
        cid = lesson['classId']
        ppw = lesson['periodsPerWeek']
        mpd = lesson.get('maxPerDay', 0)
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
            if teacher_busy.get((tid, day, period)):
                continue
            if class_busy.get((cid, day, period)):
                continue
            room_id = None
            for rm in all_rooms:
                if rm.get('id') and not room_busy.get((rm['id'], day, period)):
                    room_id = rm['id']
                    break
            teacher_busy[(tid, day, period)] = True
            class_busy[(cid, day, period)] = True
            if room_id:
                room_busy[(room_id, day, period)] = True
            day_count[day] = day_count.get(day, 0) + 1
            slots.append({'classId': cid, 'dayId': day, 'periodId': period,
                          'subjectId': sid, 'teacherId': tid, 'roomId': room_id, 'locked': 0})
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
    # Remove only unlocked slots
    conn.execute("DELETE FROM timetable_slots WHERE locked=0")
    for s in data.get('slots', []):
        conn.execute("""INSERT OR IGNORE INTO timetable_slots
                        (classId,dayId,periodId,subjectId,teacherId,roomId,locked) VALUES(?,?,?,?,?,?,0)""",
                     (s['classId'], s['dayId'], s['periodId'],
                      s.get('subjectId'), s.get('teacherId'), s.get('roomId')))
    conn.commit()
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
    src = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=?",
                       (cid, fd, fp)).fetchone()
    if not src:
        conn.close()
        return jsonify({'error': 'Source slot not found'}), 404
    if dict(src).get('locked'):
        conn.close()
        return jsonify({'error': 'This slot is locked and cannot be moved'}), 400
    target = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=?",
                          (cid, td2, tp)).fetchone()
    if target and dict(target).get('locked'):
        conn.close()
        return jsonify({'error': 'Target slot is locked'}), 400
    if target:
        conn.execute("UPDATE timetable_slots SET dayId=?,periodId=? WHERE classId=? AND dayId=? AND periodId=?",
                     (fd, fp, cid, td2, tp))
    conn.execute("UPDATE timetable_slots SET dayId=?,periodId=? WHERE classId=? AND dayId=? AND periodId=?",
                 (td2, tp, cid, fd, fp))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/remove', methods=['POST'])
def remove_slot():
    d = request.json
    cid, day, period = d['classId'], d['dayId'], d['periodId']
    conn = get_db()
    slot = conn.execute("SELECT * FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=?",
                        (cid, day, period)).fetchone()
    if slot and dict(slot).get('locked'):
        conn.close()
        return jsonify({'error': 'Slot is locked. Unlock it first.'}), 400
    conn.execute("DELETE FROM timetable_slots WHERE classId=? AND dayId=? AND periodId=?",
                 (cid, day, period))
    conn.commit()
    conn.close()
    return jsonify({'ok': True})


@app.route('/api/timetable/lock', methods=['POST'])
def lock_slot():
    d = request.json
    cid, day, period = d['classId'], d['dayId'], d['periodId']
    locked = 1 if d.get('locked', True) else 0
    conn = get_db()
    conn.execute("UPDATE timetable_slots SET locked=? WHERE classId=? AND dayId=? AND periodId=?",
                 (locked, cid, day, period))
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
    conn.close()
    return jsonify({
        'teacherLoad': teacher_load,
        'roomUtilization': room_util,
        'totalScheduled': total,
        'lockedSlots': locked,
        'dayDistribution': day_dist,
        'periodDistribution': period_dist,
        'classSummary': class_summary,
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
        # Clear existing
        for tbl in ['teacher_preferences', 'teacher_constraints', 'lessons',
                    'timetable_slots', 'rooms', 'classes', 'subjects', 'teachers', 'room_types']:
            conn.execute('DELETE FROM ' + tbl)
        # Insert teachers
        for t in data.get('teachers', []):
            conn.execute("INSERT INTO teachers (id,name,maxConsecutive,color) VALUES(?,?,?,?)",
                         (t['id'], t['name'], t.get('maxConsecutive', 0), t.get('color', '')))
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
        conn.commit()
        conn.close()
        return jsonify({'ok': True})
    except Exception as e:
        return jsonify({'error': str(e)}), 500


# ── HTML EXPORT ───────────────────────────────────────────────────────────────

@app.route('/api/export/html')
def export_html():
    view = request.args.get('view', 'class')
    conn = get_db()
    slot_data = qrows(conn.execute("""
        SELECT ts.*,s.name as subjectName,s.color as subjectColor,
               t.name as teacherName,r.name as roomName,c.name as className
        FROM timetable_slots ts
        LEFT JOIN subjects s ON ts.subjectId=s.id
        LEFT JOIN teachers t ON ts.teacherId=t.id
        LEFT JOIN rooms r ON ts.roomId=r.id
        LEFT JOIN classes c ON ts.classId=c.id"""))
    conn.close()
    html = _build_html(slot_data, view)
    return Response(html, mimetype='text/html',
                    headers={'Content-Disposition': 'attachment;filename=thorium234_timetable.html'})


def _build_html(slots, view):
    key = 'className' if view == 'class' else ('teacherName' if view == 'teacher' else 'roomName')
    by = {}
    for s in slots:
        e = s.get(key) or 'Unknown'
        by.setdefault(e, {})
        by[e][(s['dayId'], s['periodId'])] = s
    day_names = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday']
    h = [
        '<!DOCTYPE html><html><head><meta charset="UTF-8">',
        '<title>Thorium234 Timetable</title><style>',
        'body{font-family:Segoe UI,sans-serif;padding:20px;background:#f5f7fa}',
        'h1{color:#1e3a5f;border-bottom:3px solid #f0a500;padding-bottom:8px}',
        'h2{color:#1e3a5f;margin-top:24px}',
        'table{border-collapse:collapse;width:100%;margin:8px 0}',
        'th{background:#1e3a5f;color:#fff;padding:8px 12px;font-size:12px}',
        'td{border:1px solid #dde;padding:6px;min-width:90px;text-align:center;font-size:11px}',
        '.c{border-radius:4px;padding:5px;color:#fff;font-weight:600;line-height:1.5}',
        '.locked{outline:2px solid #f0a500}',
        '</style></head><body>',
        '<h1>&#128197; Thorium234 — School Timetable</h1>',
    ]
    for entity, grid in sorted(by.items()):
        h.append('<h2>%s</h2><table><tr><th>Period</th>' % entity)
        h += ['<th>%s</th>' % d for d in day_names]
        h.append('</tr>')
        for pi in range(1, 9):
            h.append('<tr><td style="background:#f0f4f8;font-weight:600">P%d</td>' % pi)
            for di in range(1, 6):
                s = grid.get((di, pi))
                if s:
                    col = s.get('subjectColor') or '#4A90D9'
                    lk = ' locked' if s.get('locked') else ''
                    h.append('<td><div class="c%s" style="background:%s">%s<br>'
                              '<small>%s</small><br><small>%s</small></div></td>'
                              % (lk, col, s['subjectName'], s['teacherName'], s.get('roomName', '')))
                else:
                    h.append('<td style="color:#ccc">&#8212;</td>')
            h.append('</tr>')
        h.append('</table>')
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


# ── ENTRY POINT ───────────────────────────────────────────────────────────────

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def index(path):
    return render_template('index.html')


if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000, debug=False)
