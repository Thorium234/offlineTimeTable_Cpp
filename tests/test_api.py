"""Integration tests for the Flask API endpoints.

Usage:
    python3 tests/test_api.py

Validates entity CRUD, solver output schema, and backward-compatible
data contracts.
"""
import os, sys, json, time, subprocess, tempfile, unittest, urllib.request

_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class TestFlaskAPI(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._tmpdir = tempfile.mkdtemp()
        cls._db_path = os.path.join(cls._tmpdir, 'test_timetable.db')
        cls._env = os.environ.copy()
        cls._env.update({
            'FLASK_PORT': '15421',
            'FLASK_SILENT_LOGS': '1',
            'FLASK_DB_PATH': cls._db_path,
        })
        cls._proc = subprocess.Popen(
            ['python3', 'web/app.py'],
            cwd=_PROJECT_ROOT, env=cls._env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        cls._base = 'http://127.0.0.1:15421'
        for _ in range(25):
            try:
                urllib.request.urlopen(cls._base + '/', timeout=1)
                break
            except Exception:
                time.sleep(0.5)
        else:
            raise RuntimeError("Flask failed to start on port 15421")

    @classmethod
    def tearDownClass(cls):
        cls._proc.terminate()
        cls._proc.wait()
        import shutil
        shutil.rmtree(cls._tmpdir, ignore_errors=True)

    def _post(self, path, data):
        body = json.dumps(data).encode()
        req = urllib.request.Request(self._base + path, data=body,
                                     headers={'Content-Type': 'application/json'})
        return json.loads(urllib.request.urlopen(req, timeout=5).read())

    def _get(self, path):
        return json.loads(urllib.request.urlopen(self._base + path, timeout=5).read())

    # ── Entity CRUD ────────────────────────────────────────────────

    def test_01_add_teacher(self):
        r = self._post('/api/teachers', {'name': 'Dr. A'})
        self.assertIn('id', r)

    def test_02_add_subject(self):
        r = self._post('/api/subjects', {'name': 'Physics'})
        self.assertIn('id', r)

    def test_03_add_class(self):
        r = self._post('/api/classes', {'name': 'Form 1', 'studentCount': 30})
        self.assertIn('id', r)

    def test_04_add_room(self):
        r = self._post('/api/rooms', {'name': 'Lab-1', 'capacity': 32, 'roomTypeId': 1})
        self.assertIn('id', r)

    def test_05_add_lesson(self):
        t = self._post('/api/teachers', {'name': 'Prof. X'})
        s = self._post('/api/subjects', {'name': 'Chem'})
        c = self._post('/api/classes', {'name': 'Form 2', 'studentCount': 25})
        self._post('/api/rooms', {'name': 'Rm-1', 'capacity': 30, 'roomTypeId': 1})
        r = self._post('/api/lessons', {
            'teacherId': t['id'], 'subjectId': s['id'], 'classId': c['id'],
            'periodsPerWeek': 4, 'blockSize': 1,
        })
        self.assertIn('id', r)

    # ── Retrieval ──────────────────────────────────────────────────

    def test_06_get_teachers(self):
        self.assertIsInstance(self._get('/api/teachers'), list)

    def test_07_get_subjects(self):
        self.assertIsInstance(self._get('/api/subjects'), list)

    def test_08_get_classes(self):
        self.assertIsInstance(self._get('/api/classes'), list)

    def test_09_get_rooms(self):
        self.assertIsInstance(self._get('/api/rooms'), list)

    def test_10_get_lessons(self):
        self.assertIsInstance(self._get('/api/lessons'), list)

    def test_11_get_timetable(self):
        self.assertIsInstance(self._get('/api/timetable'), list)

    def test_12_get_analytics(self):
        r = self._get('/api/analytics')
        for key in ('teacherLoad', 'roomUtilization', 'totalScheduled'):
            self.assertIn(key, r)

    def test_13_get_meta(self):
        r = self._get('/api/meta')
        self.assertIn('days', r)
        self.assertIn('periods', r)

    # ── Data contract (backward compatibility) ─────────────────────

    def test_14_teacher_contract(self):
        r = self._get('/api/teachers')
        if r:
            for key in ('id', 'name', 'maxConsecutive', 'color'):
                self.assertIn(key, r[0])

    def test_15_lesson_contract(self):
        r = self._get('/api/lessons')
        if r:
            for key in ('teacherId', 'subjectId', 'classId', 'periodsPerWeek'):
                self.assertIn(key, r[0])

    def test_16_timetable_slot_contract(self):
        tt = self._get('/api/timetable')
        if tt:
            for key in ('classId', 'dayId', 'periodId', 'subjectId', 'teacherId', 'roomId'):
                self.assertIn(key, tt[0])

    # ── Generate (Flask Python solver) ─────────────────────────────

    def test_17_generate(self):
        r = self._post('/api/generate', {})
        self.assertIn('score', r)


if __name__ == '__main__':
    unittest.main(verbosity=2)
