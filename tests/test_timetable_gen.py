"""Unit tests for timetable_gen.py — timeline generation and greedy solver.

Usage:
    python3 -m pytest tests/test_timetable_gen.py -v
    # or
    python3 tests/test_timetable_gen.py
"""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'web'))

from timetable_gen import (
    time_to_minutes, minutes_to_time, time_range_overlap,
    generate_periods_for_class, parse_payload, solve_timetable,
    get_sample_school,
)


# ── Helpers ──────────────────────────────────────────────────────────────────

def _assert_blocks(blocks, expected_count, lesson_count=None, break_count=None):
    """Assert block list has expected counts."""
    assert len(blocks) == expected_count, \
        f"Expected {expected_count} blocks, got {len(blocks)}"
    if lesson_count is not None:
        lc = sum(1 for b in blocks if b['type'] == 'lesson')
        assert lc == lesson_count, \
            f"Expected {lesson_count} lesson blocks, got {lc}"
    if break_count is not None:
        bc = sum(1 for b in blocks if b['type'] == 'break')
        assert bc == break_count, \
            f"Expected {break_count} break blocks, got {bc}"


def _block_times(blocks):
    """Return list of (start, end) tuples for all blocks."""
    return [(b['start_time'], b['end_time']) for b in blocks]


# ── Tests ────────────────────────────────────────────────────────────────────

def test_time_to_minutes():
    assert time_to_minutes('00:00') == 0
    assert time_to_minutes('07:30') == 450
    assert time_to_minutes('12:00') == 720
    assert time_to_minutes('16:30') == 990
    assert time_to_minutes('23:59') == 1439


def test_minutes_to_time():
    assert minutes_to_time(0) == '00:00'
    assert minutes_to_time(450) == '07:30'
    assert minutes_to_time(720) == '12:00'
    assert minutes_to_time(990) == '16:30'
    assert minutes_to_time(1439) == '23:59'


def test_minutes_roundtrip():
    for m in [0, 60, 450, 720, 990, 1439]:
        assert time_to_minutes(minutes_to_time(m)) == m


def test_time_range_overlap():
    assert time_range_overlap(100, 200, 150, 250) is True
    assert time_range_overlap(150, 250, 100, 200) is True
    assert time_range_overlap(100, 200, 200, 300) is False
    assert time_range_overlap(200, 300, 100, 200) is False
    assert time_range_overlap(100, 200, 50, 150) is True
    assert time_range_overlap(100, 200, 100, 200) is True
    assert time_range_overlap(100, 200, 0, 100) is False
    assert time_range_overlap(100, 200, 50, 100) is False


# ── generate_periods_for_class ──────────────────────────────────────────────

def test_no_breaks_40min():
    """Standard day 07:30-16:30, 40min periods, no breaks."""
    blocks = generate_periods_for_class('07:30', '16:30', 40, [])
    total_minutes = 9 * 60  # 07:30 to 16:30 = 9 hours = 540 min
    expected_lessons = total_minutes // 40  # 13.5 → 13
    _assert_blocks(blocks, expected_lessons, lesson_count=expected_lessons)


def test_no_breaks_60min():
    """60min periods for Form 4 style."""
    blocks = generate_periods_for_class('07:30', '16:30', 60, [])
    total_minutes = 9 * 60  # 540 min
    expected_lessons = total_minutes // 60  # 9
    _assert_blocks(blocks, expected_lessons, lesson_count=expected_lessons)


def test_single_break():
    """Day with 30min morning break at 10:00."""
    breaks = [{'name': 'Break', 'start_time': '10:00', 'end_time': '10:30'}]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    # 540 total - 30 break = 510 lesson-minutes → 12 lessons
    _assert_blocks(blocks, 12 + 1, lesson_count=12, break_count=1)


def test_multiple_breaks():
    """Assembly + morning break + lunch + games."""
    breaks = [
        {'name': 'Assembly', 'start_time': '07:30', 'end_time': '08:00'},
        {'name': 'Morning Break', 'start_time': '10:00', 'end_time': '10:30'},
        {'name': 'Lunch', 'start_time': '12:30', 'end_time': '13:30'},
        {'name': 'Games', 'start_time': '16:00', 'end_time': '16:30'},
    ]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    # Total = 540 min, breaks = 30+30+60+30 = 150 min
    # Lesson = 390 min → 9 lessons (390/40 = 9.75 → 9)
    # Total blocks = 9 + 4 = 13
    _assert_blocks(blocks, 13, lesson_count=9, break_count=4)


def test_break_at_boundary():
    """Break aligned exactly with day start."""
    breaks = [{'name': 'Assembly', 'start_time': '07:30', 'end_time': '08:00'}]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    first = blocks[0]
    assert first['type'] == 'break', "First block should be the break"
    assert first['start_time'] == '07:30'
    assert first['end_time'] == '08:00'


def test_break_consumes_all_time():
    """Break spans the entire day — no lesson slots."""
    breaks = [{'name': 'All Day', 'start_time': '07:30', 'end_time': '16:30'}]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    assert len(blocks) == 1
    assert blocks[0]['type'] == 'break'


def test_overlapping_breaks():
    """Overlapping breaks should still produce correct output."""
    breaks = [
        {'name': 'Extended', 'start_time': '10:00', 'end_time': '11:00'},
        {'name': 'Nested', 'start_time': '10:15', 'end_time': '10:45'},
    ]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    # Overlapping breaks cause the break region to split at inner boundaries:
    # 07:30-10:00 (150m → 3 lessons)
    # 10:00-10:15 (break), 10:15-10:45 (break), 10:45-11:00 (break)
    # 11:00-16:30 (330m → 8 lessons)
    # Total: 11 lessons + 3 break segments = 14 blocks
    _assert_blocks(blocks, 14, lesson_count=11, break_count=3)


def test_no_lesson_time_remaining():
    """Breaks exhaust all available time."""
    breaks = [
        {'name': 'A', 'start_time': '07:30', 'end_time': '12:30'},
        {'name': 'B', 'start_time': '12:30', 'end_time': '16:30'},
    ]
    blocks = generate_periods_for_class('07:30', '16:30', 40, breaks)
    assert len(blocks) == 2
    assert all(b['type'] == 'break' for b in blocks)


def test_short_day():
    """Short 2-hour day with no breaks → exactly 3 × 40min lessons."""
    blocks = generate_periods_for_class('08:00', '10:00', 40, [])
    assert len(blocks) == 3  # 120/40 = 3
    assert all(b['type'] == 'lesson' for b in blocks)
    times = _block_times(blocks)
    assert times[0] == ('08:00', '08:40')
    assert times[1] == ('08:40', '09:20')
    assert times[2] == ('09:20', '10:00')


def test_break_separates_lesson_blocks():
    """Breaks should split lesson blocks into separate groups."""
    breaks = [{'name': 'Lunch', 'start_time': '12:00', 'end_time': '13:00'}]
    blocks = generate_periods_for_class('08:00', '14:00', 40, breaks)
    # Should have both lesson and break blocks
    lesson_indices = [i for i, b in enumerate(blocks) if b['type'] == 'lesson']
    break_indices = [i for i, b in enumerate(blocks) if b['type'] == 'break']
    assert len(lesson_indices) >= 2
    assert len(break_indices) == 1
    # Break should be sandwiched between lesson blocks
    assert lesson_indices[0] < break_indices[0] < lesson_indices[-1]


# ── parse_payload ────────────────────────────────────────────────────────────

def test_parse_sample_school():
    """Parse the full sample school payload."""
    data = get_sample_school()
    parsed = parse_payload(data)
    assert len(parsed['days']) == 5
    assert parsed['day_start'] == '07:30'
    assert parsed['day_end'] == '16:30'
    assert parsed['default_duration'] == 40
    assert len(parsed['teachers']) == 12
    assert len(parsed['classes']) == 5
    assert len(parsed['class_timelines']) == 5


def test_parse_per_class_durations():
    """Verify per-class durations are set correctly."""
    parsed = parse_payload(get_sample_school())
    tl = parsed['class_timelines']
    assert tl['F4']['duration'] == 60
    assert tl['F1A']['duration'] == 40
    assert tl['F1B']['duration'] == 40


def test_parse_form4_has_fewer_blocks():
    """Form 4 with 60-min periods should have fewer lesson slots per day."""
    parsed = parse_payload(get_sample_school())
    tl = parsed['class_timelines']
    f4_blocks = tl['F4']['lesson_slots'][1]  # Monday
    f1a_blocks = tl['F1A']['lesson_slots'][1]
    assert len(f4_blocks) < len(f1a_blocks), \
        f"F4 ({len(f4_blocks)}) should have fewer slots than F1A ({len(f1a_blocks)})"


def test_parse_assembly_on_monday_friday():
    """Assembly should only appear on Monday and Friday breaks."""
    parsed = parse_payload(get_sample_school())
    # Monday (day 1) should have Assembly break
    mon_blocks = parsed['class_timelines']['F1A']['blocks_by_day'][1]
    mon_breaks = [b for b in mon_blocks if b['type'] == 'break']
    mon_break_names = [b['label'] for b in mon_breaks]
    assert 'Assembly' in mon_break_names
    # Wednesday (day 3) should NOT have Assembly
    wed_blocks = parsed['class_timelines']['F1A']['blocks_by_day'][3]
    wed_break_names = [b['label'] for b in wed_blocks if b['type'] == 'break']
    assert 'Assembly' not in wed_break_names


def test_parse_with_constraints():
    """Parse payload with maxPerDay, blockSize, secondTeacherId."""
    payload = {
        'configuration': {'day_start': '08:00', 'day_end': '14:00'},
        'time_blocks': [],
        'teachers': [
            {'id': 'T1', 'name': 'Teacher 1', 'abbreviation': 'T1',
             'qualified_subjects': ['MATH']},
            {'id': 'T2', 'name': 'Teacher 2', 'abbreviation': 'T2',
             'qualified_subjects': ['MATH']},
        ],
        'classes': [{
            'id': 'C1', 'name': 'Class 1',
            'lesson_duration_minutes': 40,
            'subject_requirements': [{
                'subject': 'MATH',
                'abbreviation': 'MATH',
                'lessons_per_week': 3,
                'max_per_day': 1,
                'block_size': 2,
                'second_teacher_id': 'T2',
            }],
        }],
    }
    parsed = parse_payload(payload)
    assert len(parsed['classes']) == 1
    req = parsed['classes'][0]['subject_requirements'][0]
    assert req['max_per_day'] == 1
    assert req['block_size'] == 2
    assert req['second_teacher_id'] == 'T2'


# ── solve_timetable ─────────────────────────────────────────────────────────

def test_solve_sample_school():
    """Full sample school solves with 0 unscheduled."""
    data = get_sample_school()
    parsed = parse_payload(data)
    result = solve_timetable(parsed)
    assert result['score'] == 1000
    assert len(result['unscheduled']) == 0
    assert len(result['slots']) > 0


def test_solve_max_per_day():
    """maxPerDay=1 should prevent more than 1 lesson/day of a subject."""
    payload = {
        'configuration': {'day_start': '08:00', 'day_end': '14:00'},
        'time_blocks': [],
        'teachers': [
            {'id': 'T1', 'name': 'Teacher 1', 'abbreviation': 'T1',
             'qualified_subjects': ['MATH']},
        ],
        'classes': [{
            'id': 'C1', 'name': 'Class 1',
            'lesson_duration_minutes': 40,
            'subject_requirements': [{
                'subject': 'MATH',
                'abbreviation': 'MATH',
                'lessons_per_week': 5,
                'max_per_day': 1,
            }],
        }],
    }
    parsed = parse_payload(payload)
    result = solve_timetable(parsed)
    # Verify no day has more than 1 MATH for C1
    day_subject_counts = {}
    for s in result['slots']:
        if s['classId'] == 'C1' and s['subjectName'] == 'MATH':
            key = (s['classId'], s['dayId'], s['subjectName'])
            day_subject_counts[key] = day_subject_counts.get(key, 0) + 1
    for count in day_subject_counts.values():
        assert count <= 1, f"maxPerDay=1 violated: {count} lessons on a day"


def test_solve_block_size():
    """blockSize=2 should occupy 2 consecutive slots."""
    payload = {
        'configuration': {'day_start': '08:00', 'day_end': '12:00'},
        'time_blocks': [],
        'teachers': [
            {'id': 'T1', 'name': 'Teacher 1', 'abbreviation': 'T1',
             'qualified_subjects': ['PHY']},
        ],
        'classes': [{
            'id': 'C1', 'name': 'Class 1',
            'lesson_duration_minutes': 40,
            'subject_requirements': [{
                'subject': 'PHY',
                'abbreviation': 'PHY',
                'lessons_per_week': 3,
                'block_size': 2,
            }],
        }],
    }
    parsed = parse_payload(payload)
    result = solve_timetable(parsed)
    # Each assignment with blockSize=2 should have blockSize=2 in output
    for s in result['slots']:
        assert s.get('blockSize', 1) == 2, \
            f"Expected blockSize=2, got {s.get('blockSize')}"


def test_solve_second_teacher():
    """A lesson with a second teacher blocks both teachers."""
    payload = {
        'configuration': {'day_start': '08:00', 'day_end': '14:00'},
        'time_blocks': [],
        'teachers': [
            {'id': 'T1', 'name': 'Teacher 1', 'abbreviation': 'T1',
             'qualified_subjects': ['MATH']},
            {'id': 'T2', 'name': 'Teacher 2', 'abbreviation': 'T2',
             'qualified_subjects': ['MATH', 'ENG']},
        ],
        'classes': [{
            'id': 'C1', 'name': 'Class 1',
            'lesson_duration_minutes': 40,
            'subject_requirements': [
                {'subject': 'MATH', 'abbreviation': 'MATH',
                 'lessons_per_week': 3, 'second_teacher_id': 'T2'},
                {'subject': 'ENG', 'abbreviation': 'ENG',
                 'lessons_per_week': 3},
            ],
        }],
    }
    parsed = parse_payload(payload)
    result = solve_timetable(parsed)
    # Both teachers should be present in MATH slots
    for s in result['slots']:
        if s['subjectName'] == 'MATH':
            assert s.get('secondTeacherId') == 'T2', \
                f"Expected secondTeacherId=T2, got {s.get('secondTeacherId')}"


def test_solve_unscheduled_with_limited_slots():
    """When slots are limited, some lessons should remain unscheduled."""
    payload = {
        'configuration': {'day_start': '08:00', 'day_end': '09:00'},
        'time_blocks': [],
        'teachers': [
            {'id': 'T1', 'name': 'Teacher 1', 'abbreviation': 'T1',
             'qualified_subjects': ['MATH', 'ENG', 'KIS']},
        ],
        'classes': [{
            'id': 'C1', 'name': 'Class 1',
            'lesson_duration_minutes': 40,
            'subject_requirements': [
                {'subject': 'MATH', 'abbreviation': 'MATH',
                 'lessons_per_week': 3},
                {'subject': 'ENG', 'abbreviation': 'ENG',
                 'lessons_per_week': 3},
                {'subject': 'KIS', 'abbreviation': 'KIS',
                 'lessons_per_week': 3},
            ],
        }],
    }
    parsed = parse_payload(payload)
    result = solve_timetable(parsed)
    # Only 1 period/day × 5 days = 5 slots available
    # 9 lessons requested → at least 4 should be unscheduled
    assert len(result['unscheduled']) >= 4, \
        f"Expected >=4 unscheduled with only 5 slots for 9 lessons"


# ── Main ─────────────────────────────────────────────────────────────────────

if __name__ == '__main__':
    import pytest
    sys.exit(pytest.main([__file__, '-v']))
