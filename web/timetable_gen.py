"""Bridge between Flask JSON config and timetable solver with per-class dynamic timelines.

Each class/stream can have its own lesson duration. The timeline generator
produces per-class period grids that respect the shared break schedule.
"""

import json


def time_to_minutes(tstr):
    h, m = tstr.split(':')
    return int(h) * 60 + int(m)


def minutes_to_time(minutes):
    minutes = minutes % 1440
    return f'{minutes // 60:02d}:{minutes % 60:02d}'


def time_range_overlap(a_start, a_end, b_start, b_end):
    """Check if two time ranges [start, end) overlap."""
    return a_start < b_end and b_start < a_end


def generate_periods_for_class(day_start, day_end, lesson_duration, breaks_for_day):
    """Generate lesson and break blocks for a single class on a single day.

    Returns list of dicts with keys: slot_index, start_time, end_time,
    duration, type ('lesson'|'break'), label.
    """
    start_min = time_to_minutes(day_start)
    end_min = time_to_minutes(day_end)

    # Collect all boundaries (break starts/ends + day boundaries)
    boundaries = {start_min, end_min}
    for brk in breaks_for_day:
        bs = max(time_to_minutes(brk['start_time']), start_min)
        be = min(time_to_minutes(brk['end_time']), end_min)
        if bs < be:
            boundaries.add(bs)
            boundaries.add(be)

    boundaries = sorted(boundaries)

    blocks = []
    slot_idx = 0
    for i in range(len(boundaries) - 1):
        seg_start = boundaries[i]
        seg_end = boundaries[i + 1]
        if seg_start >= seg_end:
            continue

        # Check if this segment falls entirely inside a break
        in_break = False
        for brk in breaks_for_day:
            bs = time_to_minutes(brk['start_time'])
            be = time_to_minutes(brk['end_time'])
            if seg_start >= bs and seg_end <= be:
                blocks.append({
                    'slot_index': slot_idx,
                    'start_time': minutes_to_time(seg_start),
                    'end_time': minutes_to_time(seg_end),
                    'duration': seg_end - seg_start,
                    'type': 'break',
                    'label': brk['name'],
                })
                slot_idx += 1
                in_break = True
                break
        if in_break:
            continue

        # Slice remaining time into lesson blocks
        dur = lesson_duration
        cursor = seg_start
        while cursor + dur <= seg_end:
            blocks.append({
                'slot_index': slot_idx,
                'start_time': minutes_to_time(cursor),
                'end_time': minutes_to_time(cursor + dur),
                'duration': dur,
                'type': 'lesson',
                'label': f'P{slot_idx}',
            })
            cursor += dur
            slot_idx += 1

    return blocks


def parse_payload(payload):
    """Parse JSON payload and produce per-class timeline configurations.

    Each class can have its own lesson_duration_minutes to support
    streams with different period lengths (e.g., Form 4 uses 1-hour periods).
    """
    config = payload.get('configuration', {})
    time_blocks = payload.get('time_blocks', [])
    teachers = payload.get('teachers', [])
    classes = payload.get('classes', [])

    day_start = config.get('day_start', '07:30')
    day_end = config.get('day_end', '16:30')
    default_duration = config.get('default_lesson_duration_minutes', 40)

    # Day definitions
    all_days = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday']
    day_name_to_id = {n: i + 1 for i, n in enumerate(all_days)}
    days = [{'id': i + 1, 'name': n} for i, n in enumerate(all_days)]

    # Normalize breaks to day IDs
    breaks = []
    for tb in time_blocks:
        tb_days = tb.get('days', [])
        day_ids = set()
        for d in tb_days:
            if d == 'All':
                day_ids.update(day_name_to_id.values())
            elif d in day_name_to_id:
                day_ids.add(day_name_to_id[d])
        if not day_ids:
            continue
        breaks.append({
            'name': tb['name'],
            'start_time': tb['start'],
            'end_time': tb['end'],
            'day_ids': sorted(day_ids),
        })

    # For each day, compute the breaks that apply
    day_breaks_map = {}
    for d in days:
        did = d['id']
        day_breaks_map[did] = [b for b in breaks if did in b['day_ids']]

    # Per-class timeline generation
    class_timelines = {}
    for c in classes:
        cid = c['id']
        dur = c.get('lesson_duration_minutes', default_duration)
        blocks_by_day = {}
        for d in days:
            did = d['id']
            blocks = generate_periods_for_class(
                day_start, day_end, dur, day_breaks_map[did]
            )
            blocks_by_day[did] = blocks
        class_timelines[cid] = {
            'duration': dur,
            'blocks_by_day': blocks_by_day,
            'lesson_slots': {},  # day_id -> list of lesson-only blocks
        }
        # Pre-extract lesson slots for quick access
        for did, blocks in blocks_by_day.items():
            class_timelines[cid]['lesson_slots'][did] = [
                b for b in blocks if b['type'] == 'lesson'
            ]

    # Build subject abbreviation map
    subject_abbr = {}
    for c in classes:
        for req in c.get('subject_requirements', []):
            subj = req['subject']
            if subj not in subject_abbr:
                subject_abbr[subj] = req.get('abbreviation',
                                              subj[:3].upper())

    return {
        'days': days,
        'day_start': day_start,
        'day_end': day_end,
        'default_duration': default_duration,
        'breaks': breaks,
        'day_breaks_map': day_breaks_map,
        'teachers': teachers,
        'classes': classes,
        'class_timelines': class_timelines,
        'subject_abbr': subject_abbr,
        'config': config,
    }


def _teacher_busy_check(busy_map, tid, did, start_min, end_min):
    """Check if a teacher is free during [start_min, end_min) on day did."""
    for existing_did, es, ee in busy_map.get(tid, []):
        if existing_did == did and time_range_overlap(start_min, end_min, es, ee):
            return True
    return False


def solve_timetable(parsed):
    """Greedy solver for per-class dynamic timelines.

    Supports per-class durations, clock-time conflict detection,
    maxPerDay, blockSize, and secondTeacherId constraints.
    """
    teachers = parsed['teachers']
    classes = parsed['classes']
    class_timelines = parsed['class_timelines']
    days = parsed['days']

    teacher_map = {t['id']: t for t in teachers}
    class_map = {c['id']: c for c in classes}

    teacher_subjects = {t['id']: set(t.get('qualified_subjects', []))
                        for t in teachers}

    # Build lesson units with constraints
    lesson_units = []
    for c in classes:
        cid = c['id']
        for req in c.get('subject_requirements', []):
            subj = req['subject']
            count = req.get('lessons_per_week', 1)
            max_per_day = req.get('max_per_day', 99)
            block_size = req.get('block_size', 1)
            second_teacher_id = req.get('second_teacher_id')

            candidates = [tid for tid, subs in teacher_subjects.items()
                          if subj in subs]
            teacher_id = candidates[0] if candidates else 'UNASSIGNED'

            for _ in range(count):
                lesson_units.append({
                    'class_id': cid,
                    'subject': subj,
                    'teacher_id': teacher_id,
                    'max_per_day': max_per_day,
                    'block_size': block_size,
                    'second_teacher_id': second_teacher_id,
                })

    # Sort: subjects with fewer qualified teachers go first
    lesson_units.sort(key=lambda u: -len(
        [tid for tid, subs in teacher_subjects.items()
         if u['subject'] in subs]) if teacher_subjects else 0)

    # Teacher clock-time busy map: teacher_id -> [(day_id, start_min, end_min)]
    teacher_busy = {}
    # Per-class per-day per-subject lesson count
    class_day_subject_count = {}
    assignments = []
    scheduled_indices = set()

    for idx, unit in enumerate(lesson_units):
        if unit['teacher_id'] == 'UNASSIGNED':
            continue

        cid = unit['class_id']
        tl = class_timelines.get(cid)
        if not tl:
            continue

        tid = unit['teacher_id']
        second_tid = unit.get('second_teacher_id')
        bs = unit['block_size']
        mpd = unit['max_per_day']
        subj = unit['subject']

        placed = False
        for d in days:
            did = d['id']

            # maxPerDay check
            mpd_key = (cid, did, subj)
            if class_day_subject_count.get(mpd_key, 0) >= mpd:
                continue

            lesson_slots = tl['lesson_slots'].get(did, [])

            for si in range(len(lesson_slots)):
                if lesson_slots[si].get('occupied'):
                    continue
                if si + bs > len(lesson_slots):
                    continue

                # Check all slots in block are free
                block_free = True
                for bi in range(bs):
                    if lesson_slots[si + bi].get('occupied'):
                        block_free = False
                        break
                if not block_free:
                    continue

                # Compute block time range
                first_slot = lesson_slots[si]
                last_slot = lesson_slots[si + bs - 1]
                block_start = time_to_minutes(first_slot['start_time'])
                block_end = time_to_minutes(last_slot['end_time'])

                # Primary teacher conflict check
                if _teacher_busy_check(teacher_busy, tid, did,
                                       block_start, block_end):
                    continue

                # Second teacher conflict check
                if second_tid and _teacher_busy_check(
                        teacher_busy, second_tid, did,
                        block_start, block_end):
                    continue

                # Place the block
                for bi in range(bs):
                    s = lesson_slots[si + bi]
                    s['occupied'] = True
                    s['subject'] = subj
                    s['teacher_id'] = tid
                    if second_tid:
                        s['second_teacher_id'] = second_tid

                teacher_busy.setdefault(tid, []).append(
                    (did, block_start, block_end))
                if second_tid:
                    teacher_busy.setdefault(second_tid, []).append(
                        (did, block_start, block_end))

                class_day_subject_count[mpd_key] = \
                    class_day_subject_count.get(mpd_key, 0) + 1

                assignments.append({
                    'class_id': cid,
                    'subject': subj,
                    'teacher_id': tid,
                    'second_teacher_id': second_tid,
                    'day_id': did,
                    'slot_index': first_slot['slot_index'],
                    'block_size': bs,
                    'start': first_slot['start_time'],
                    'end': last_slot['end_time'],
                })
                scheduled_indices.add(idx)
                placed = True
                break
            if placed:
                break

    # Build result with abbreviations and colors
    subject_colors = [
        '#E74C3C', '#3498DB', '#2ECC71', '#F39C12', '#9B59B6',
        '#1ABC9C', '#E67E22', '#2980B9', '#27AE60', '#D35400',
        '#8E44AD', '#16A085', '#C0392B', '#2C3E50', '#F1C40F',
    ]
    subject_color_map = {}
    next_color = 0
    for c in classes:
        for req in c.get('subject_requirements', []):
            subj = req['subject']
            if subj not in subject_color_map:
                subject_color_map[subj] = subject_colors[
                    next_color % len(subject_colors)]
                next_color += 1

    subject_abbr = parsed.get('subject_abbr', {})

    result_slots = []
    for a in assignments:
        subj = a['subject']
        tid = a['teacher_id']
        t_info = teacher_map.get(tid, {})
        second_t_info = teacher_map.get(a['second_teacher_id']) if a.get(
            'second_teacher_id') else None
        result_slots.append({
            'classId': a['class_id'],
            'dayId': a['day_id'],
            'periodId': a['slot_index'],
            'subjectId': hash(subj) % 10000,
            'teacherId': tid,
            'secondTeacherId': a.get('second_teacher_id') or '',
            'roomId': 1,
            'subjectName': subj,
            'subjectAbbr': subject_abbr.get(subj, subj[:3].upper()),
            'subjectColor': subject_color_map.get(subj, '#4A90D9'),
            'teacherName': t_info.get('name', tid),
            'teacherAbbr': t_info.get('abbreviation', tid[:2].upper()),
            'secondTeacherName': second_t_info.get('name', '') if second_t_info else '',
            'secondTeacherAbbr': second_t_info.get('abbreviation', '') if second_t_info else '',
            'className': class_map.get(a['class_id'], {}).get(
                'name', str(a['class_id'])),
            'start': a['start'],
            'end': a['end'],
            'blockSize': a['block_size'],
            'weekType': 0,
            'locked': False,
        })

    # Unscheduled (track by unit index, not subject pair)
    unscheduled = []
    for i, unit in enumerate(lesson_units):
        if i not in scheduled_indices:
            unscheduled.append({
                'className': class_map.get(unit['class_id'], {}).get(
                    'name', str(unit['class_id'])),
                'subjectName': unit['subject'],
                'teacherName': teacher_map.get(unit['teacher_id'], {}).get(
                    'name', unit['teacher_id']),
                'periodsCount': unit['block_size'],
                'reason': 'No available slot or unqualified teacher',
            })

    return {
        'slots': result_slots,
        'unscheduled': unscheduled,
        'score': max(0, 1000 - 50 * len(unscheduled)),
    }


# ── SAMPLE DATA ──────────────────────────────────────────────────────────────

def get_sample_school():
    """Return realistic Kenyan secondary school data as per the JSON schema."""
    return {
        'school': {
            'name': 'Alliance High School',
            'motto': 'Strong to Serve',
            'location': 'Kikuyu, Kenya',
        },
        'configuration': {
            'day_start': '07:30',
            'day_end': '16:30',
            'default_lesson_duration_minutes': 40,
        },
        'time_blocks': [
            {
                'name': 'Assembly',
                'type': 'fixed_break',
                'start': '07:30',
                'end': '08:00',
                'days': ['Monday', 'Friday'],
            },
            {
                'name': 'Morning Break',
                'type': 'fixed_break',
                'start': '10:00',
                'end': '10:30',
                'days': ['All'],
            },
            {
                'name': 'Lunch Break',
                'type': 'fixed_break',
                'start': '12:30',
                'end': '13:30',
                'days': ['All'],
            },
            {
                'name': 'Games',
                'type': 'fixed_break',
                'start': '16:00',
                'end': '16:30',
                'days': ['All'],
            },
        ],
        'teachers': [
            {'id': 'T1', 'name': 'Paul Inyangala', 'abbreviation': 'PI',
             'qualified_subjects': ['MATH']},
            {'id': 'T2', 'name': 'Jane Akinyi', 'abbreviation': 'JA',
             'qualified_subjects': ['ENG']},
            {'id': 'T3', 'name': 'Peter Kamau', 'abbreviation': 'PK',
             'qualified_subjects': ['KIS']},
            {'id': 'T4', 'name': 'Grace Wanjiku', 'abbreviation': 'GW',
             'qualified_subjects': ['PHY']},
            {'id': 'T5', 'name': 'David Omondi', 'abbreviation': 'DO',
             'qualified_subjects': ['CHE']},
            {'id': 'T6', 'name': 'Sarah Chebet', 'abbreviation': 'SC',
             'qualified_subjects': ['BIO']},
            {'id': 'T7', 'name': 'James Mwangi', 'abbreviation': 'JM',
             'qualified_subjects': ['HIS']},
            {'id': 'T8', 'name': 'Faith Nyambura', 'abbreviation': 'FN',
             'qualified_subjects': ['GEO']},
            {'id': 'T9', 'name': 'Samuel Kiprop', 'abbreviation': 'SK',
             'qualified_subjects': ['CRE']},
            {'id': 'T10', 'name': 'Diana Wambui', 'abbreviation': 'DW',
             'qualified_subjects': ['BUS']},
            {'id': 'T11', 'name': 'Patrick Wafula', 'abbreviation': 'PW',
             'qualified_subjects': ['AGR']},
            {'id': 'T12', 'name': 'Esther Muthoni', 'abbreviation': 'EM',
             'qualified_subjects': ['COM']},
        ],
        'classes': [
            {
                'id': 'F1A', 'name': 'Form 1 A',
                'lesson_duration_minutes': 40,
                'subject_requirements': [
                    {'subject': 'MATH', 'abbreviation': 'MATH',
                     'lessons_per_week': 5},
                    {'subject': 'ENG', 'abbreviation': 'ENG',
                     'lessons_per_week': 5},
                    {'subject': 'KIS', 'abbreviation': 'KIS',
                     'lessons_per_week': 4},
                    {'subject': 'PHY', 'abbreviation': 'PHY',
                     'lessons_per_week': 3},
                    {'subject': 'CHE', 'abbreviation': 'CHE',
                     'lessons_per_week': 3},
                    {'subject': 'BIO', 'abbreviation': 'BIO',
                     'lessons_per_week': 3},
                    {'subject': 'HIS', 'abbreviation': 'HIS',
                     'lessons_per_week': 3},
                    {'subject': 'GEO', 'abbreviation': 'GEO',
                     'lessons_per_week': 3},
                    {'subject': 'CRE', 'abbreviation': 'CRE',
                     'lessons_per_week': 2},
                    {'subject': 'BUS', 'abbreviation': 'BUS',
                     'lessons_per_week': 2},
                    {'subject': 'AGR', 'abbreviation': 'AGR',
                     'lessons_per_week': 2},
                    {'subject': 'COM', 'abbreviation': 'COM',
                     'lessons_per_week': 1},
                ],
            },
            {
                'id': 'F1B', 'name': 'Form 1 B',
                'lesson_duration_minutes': 40,
                'subject_requirements': [
                    {'subject': 'MATH', 'abbreviation': 'MATH',
                     'lessons_per_week': 5},
                    {'subject': 'ENG', 'abbreviation': 'ENG',
                     'lessons_per_week': 5},
                    {'subject': 'KIS', 'abbreviation': 'KIS',
                     'lessons_per_week': 4},
                    {'subject': 'PHY', 'abbreviation': 'PHY',
                     'lessons_per_week': 3},
                    {'subject': 'CHE', 'abbreviation': 'CHE',
                     'lessons_per_week': 3},
                    {'subject': 'BIO', 'abbreviation': 'BIO',
                     'lessons_per_week': 3},
                    {'subject': 'HIS', 'abbreviation': 'HIS',
                     'lessons_per_week': 3},
                    {'subject': 'GEO', 'abbreviation': 'GEO',
                     'lessons_per_week': 3},
                    {'subject': 'CRE', 'abbreviation': 'CRE',
                     'lessons_per_week': 2},
                    {'subject': 'BUS', 'abbreviation': 'BUS',
                     'lessons_per_week': 2},
                    {'subject': 'AGR', 'abbreviation': 'AGR',
                     'lessons_per_week': 2},
                    {'subject': 'COM', 'abbreviation': 'COM',
                     'lessons_per_week': 1},
                ],
            },
            {
                'id': 'F2', 'name': 'Form 2',
                'lesson_duration_minutes': 40,
                'subject_requirements': [
                    {'subject': 'MATH', 'abbreviation': 'MATH',
                     'lessons_per_week': 5},
                    {'subject': 'ENG', 'abbreviation': 'ENG',
                     'lessons_per_week': 4},
                    {'subject': 'KIS', 'abbreviation': 'KIS',
                     'lessons_per_week': 4},
                    {'subject': 'PHY', 'abbreviation': 'PHY',
                     'lessons_per_week': 3},
                    {'subject': 'CHE', 'abbreviation': 'CHE',
                     'lessons_per_week': 3},
                    {'subject': 'BIO', 'abbreviation': 'BIO',
                     'lessons_per_week': 3},
                    {'subject': 'HIS', 'abbreviation': 'HIS',
                     'lessons_per_week': 2},
                    {'subject': 'GEO', 'abbreviation': 'GEO',
                     'lessons_per_week': 2},
                    {'subject': 'CRE', 'abbreviation': 'CRE',
                     'lessons_per_week': 2},
                    {'subject': 'BUS', 'abbreviation': 'BUS',
                     'lessons_per_week': 2},
                    {'subject': 'AGR', 'abbreviation': 'AGR',
                     'lessons_per_week': 2},
                ],
            },
            {
                'id': 'F3', 'name': 'Form 3',
                'lesson_duration_minutes': 40,
                'subject_requirements': [
                    {'subject': 'MATH', 'abbreviation': 'MATH',
                     'lessons_per_week': 5},
                    {'subject': 'ENG', 'abbreviation': 'ENG',
                     'lessons_per_week': 4},
                    {'subject': 'KIS', 'abbreviation': 'KIS',
                     'lessons_per_week': 4},
                    {'subject': 'PHY', 'abbreviation': 'PHY',
                     'lessons_per_week': 3},
                    {'subject': 'CHE', 'abbreviation': 'CHE',
                     'lessons_per_week': 3},
                    {'subject': 'BIO', 'abbreviation': 'BIO',
                     'lessons_per_week': 3},
                    {'subject': 'HIS', 'abbreviation': 'HIS',
                     'lessons_per_week': 2},
                    {'subject': 'GEO', 'abbreviation': 'GEO',
                     'lessons_per_week': 2},
                    {'subject': 'CRE', 'abbreviation': 'CRE',
                     'lessons_per_week': 2},
                ],
            },
            {
                'id': 'F4', 'name': 'Form 4',
                'lesson_duration_minutes': 60,
                'subject_requirements': [
                    {'subject': 'MATH', 'abbreviation': 'MATH',
                     'lessons_per_week': 5},
                    {'subject': 'ENG', 'abbreviation': 'ENG',
                     'lessons_per_week': 4},
                    {'subject': 'KIS', 'abbreviation': 'KIS',
                     'lessons_per_week': 4},
                    {'subject': 'PHY', 'abbreviation': 'PHY',
                     'lessons_per_week': 3},
                    {'subject': 'CHE', 'abbreviation': 'CHE',
                     'lessons_per_week': 3},
                    {'subject': 'BIO', 'abbreviation': 'BIO',
                     'lessons_per_week': 3},
                    {'subject': 'HIS', 'abbreviation': 'HIS',
                     'lessons_per_week': 3},
                    {'subject': 'GEO', 'abbreviation': 'GEO',
                     'lessons_per_week': 3},
                ],
            },
        ],
    }
