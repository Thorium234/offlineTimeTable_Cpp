'use strict';

const COLORS = ["#4A90D9","#E67E22","#27AE60","#8E44AD","#C0392B","#16A085","#F39C12","#2980B9","#D35400","#1ABC9C","#2ECC71","#9B59B6","#E74C3C","#3498DB","#F1C40F","#E91E63","#00BCD4","#FF5722","#607D8B","#795548"];

// ── STATE ──────────────────────────────────────────────────────────────────
const S = {
  teachers: [], subjects: [], classes: [], rooms: [], roomTypes: [],
  lessons: [], slots: [], unscheduled: [],
  days: [], periods: [],
  currentTab: 'home',
  ttView: 'class',
  ttEntity: null,
  score: null,
  conflicts: [],
  solverSeed: 42,
  solverAlgo: 'backtrack',
};

// ── API ──────────────────────────────────────────────────────────────────
const api = {
  async get(url) {
    const r = await fetch(url);
    if (!r.ok) { const t = await r.text(); throw new Error(t); }
    return r.json();
  },
  async post(url, body) {
    const r = await fetch(url, {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if (!r.ok) { const t = await r.text(); let m; try{m=JSON.parse(t).error}catch(e){m=t}; throw new Error(m||t); }
    return r.json();
  },
  async put(url, body) {
    const r = await fetch(url, {method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if (!r.ok) { const t = await r.text(); throw new Error(t); }
    return r.json();
  },
  async del(url) {
    const r = await fetch(url, {method:'DELETE'});
    if (!r.ok) { const t = await r.text(); throw new Error(t); }
    return r.json();
  },
};

// ── TOAST ────────────────────────────────────────────────────────────────
function toast(msg, type='') {
  const el = document.createElement('div');
  el.className = 'toast ' + type;
  const icon = type==='error'?'circle-xmark':type==='success'?'circle-check':type==='warn'?'triangle-exclamation':'bell';
  el.innerHTML = '<i class="fa-solid fa-'+icon+'"></i>' + msg;
  document.getElementById('toasts').prepend(el);
  setTimeout(() => el.classList.add('fade-out'), 3200);
  setTimeout(() => el.remove(), 3500);
}

// ── SPINNER ──────────────────────────────────────────────────────────────
function showSpinner(msg='Working…') {
  let ov = document.getElementById('spin-ov');
  if (!ov) {
    ov = document.createElement('div');
    ov.id = 'spin-ov';
    ov.className = 'spin-ov';
    document.body.appendChild(ov);
  }
  ov.innerHTML = '<div class="spinner"></div><div class="spin-lbl">' + esc(msg) + '</div>';
}
function hideSpinner() {
  const ov = document.getElementById('spin-ov');
  if (ov) ov.remove();
}

// ── MODAL ────────────────────────────────────────────────────────────────
function openModal(title, icon, bodyHtml, footerHtml, wide=false) {
  closeModal();
  const ov = document.createElement('div');
  ov.className = 'overlay';
  ov.id = 'modal-overlay';
  ov.innerHTML =
    '<div class="modal' + (wide?' wide':'') + '">' +
      '<div class="modal-hd"><i class="fa-solid fa-' + icon + '"></i><h3>' + title + '</h3>' +
      '<button class="x" onclick="closeModal()">&times;</button></div>' +
      '<div class="modal-bd">' + bodyHtml + '</div>' +
      '<div class="modal-ft">' + footerHtml + '</div>' +
    '</div>';
  document.body.appendChild(ov);
  ov.addEventListener('click', e => { if (e.target === ov) closeModal(); });
  setTimeout(() => { const f = ov.querySelector('input,select'); if(f) f.focus(); }, 60);
}
function closeModal() {
  const ov = document.getElementById('modal-overlay');
  if (ov) ov.remove();
}

// ── COLOR PICKER ─────────────────────────────────────────────────────────
function colorPickerHtml(sel, name='color') {
  return '<div class="color-row">' +
    COLORS.map(c => '<span class="cswatch' + (c===sel?' sel':'') + '" style="background:' + c + '" data-c="' + c + '" onclick="selectColor(this,\'' + name + '\')"></span>').join('') +
    '<input type="hidden" id="inp-' + name + '" value="' + (sel||COLORS[0]) + '"></div>';
}
function selectColor(el, name) {
  el.closest('.color-row').querySelectorAll('.cswatch').forEach(s => s.classList.remove('sel'));
  el.classList.add('sel');
  document.getElementById('inp-' + name).value = el.dataset.c;
}
function autoColor(list) { return COLORS[list.length % COLORS.length]; }

// ── LOAD ALL DATA ─────────────────────────────────────────────────────────
async function loadAll() {
  try {
    const [teachers, subjects, classes, rooms, roomTypes, lessons, slots, meta] = await Promise.all([
      api.get('/api/teachers'), api.get('/api/subjects'), api.get('/api/classes'),
      api.get('/api/rooms'), api.get('/api/room_types'), api.get('/api/lessons'),
      api.get('/api/timetable'), api.get('/api/meta'),
    ]);
    S.teachers=teachers; S.subjects=subjects; S.classes=classes;
    S.rooms=rooms; S.roomTypes=roomTypes; S.lessons=lessons;
    S.slots=slots; S.days=meta.days; S.periods=meta.periods;
    renderSidebar();
    renderCurrentView();
    checkConflicts();
  } catch(e) { toast('Failed to load data: ' + e.message, 'error'); }
}

// ── CONFLICT CHECK ───────────────────────────────────────────────────────
async function checkConflicts() {
  try {
    S.conflicts = await api.get('/api/conflicts');
    const w = document.getElementById('conflict-w');
    const cnt = document.getElementById('conflict-count');
    if (S.conflicts.length > 0) {
      w.style.display = 'flex';
      cnt.textContent = S.conflicts.length;
    } else {
      w.style.display = 'none';
    }
  } catch(e) {}
}

function showConflicts() {
  if (S.conflicts.length === 0) {
    openModal('No Conflicts', 'circle-check',
      '<p style="color:var(--success);font-size:13px"><i class="fa-solid fa-circle-check"></i> No scheduling conflicts detected.</p>',
      '<button class="btn btn-ghost" onclick="closeModal()">Close</button>');
    return;
  }
  const rows = S.conflicts.map(c =>
    '<div class="conflict-item"><i class="fa-solid fa-triangle-exclamation"></i>' +
    '<div class="desc">' + esc(c.description) +
    '<div class="loc">Day ' + c.dayId + ' · Period ' + c.periodId + '</div></div></div>'
  ).join('');
  openModal('Conflicts (' + S.conflicts.length + ')', 'triangle-exclamation',
    '<div style="margin-bottom:10px;font-size:12px;color:var(--text-muted)">These must be resolved. Re-generate to fix automatically.</div>' + rows,
    '<button class="btn btn-ghost" onclick="closeModal()">Close</button>' +
    '<button class="btn btn-primary" onclick="closeModal();App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i> Re-Generate</button>');
}

// ── SIDEBAR ──────────────────────────────────────────────────────────────
function renderSidebar() {
  const search = (document.getElementById('sidebar-search').value || '').toLowerCase();
  renderEntityList('teachers', S.teachers, search);
  renderEntityList('subjects', S.subjects, search);
  renderEntityList('classes', S.classes, search);
  renderEntityList('rooms', S.rooms, search);
  renderLessonsList(search);
}

function renderEntityList(type, items, search='') {
  document.getElementById('cnt-' + type).textContent = items.length;
  const filtered = search ? items.filter(i => i.name.toLowerCase().includes(search)) : items;
  const el = document.getElementById('list-' + type);
  el.innerHTML = filtered.map(item => {
    const color = item.color || '#888';
    const sub = type==='rooms' ? (item.roomTypeName||'') : type==='classes' ? (item.studentCount ? item.studentCount+' students' : '') : '';
    return '<div class="ei" data-id="' + item.id + '" data-type="' + type + '" onclick="App.selectEntity(\'' + type + '\',' + item.id + ')">' +
      '<span class="dot" style="background:' + color + '"></span>' +
      '<span class="nm">' + esc(item.name) + '</span>' +
      (sub ? '<span class="sub">' + esc(sub) + '</span>' : '') +
      '<span class="acts">' +
        '<button onclick="event.stopPropagation();App.editEntity(\'' + type + '\',' + item.id + ')"><i class="fa-solid fa-pen"></i></button>' +
        '<button class="del" onclick="event.stopPropagation();App.deleteEntity(\'' + type + '\',' + item.id + ')"><i class="fa-solid fa-trash"></i></button>' +
      '</span></div>';
  }).join('');
}

function renderLessonsList(search='') {
  document.getElementById('cnt-lessons').textContent = S.lessons.length;
  const filtered = search
    ? S.lessons.filter(l => (l.subjectName+l.teacherName+l.className).toLowerCase().includes(search))
    : S.lessons;
  const el = document.getElementById('list-lessons');
  el.innerHTML = filtered.map(l =>
    '<div class="ei" data-id="' + l.id + '" data-type="lessons">' +
    '<span class="dot" style="background:' + (l.subjectColor||'#888') + '"></span>' +
    '<span class="nm">' + esc(l.className) + ' · ' + esc(l.subjectName) + '</span>' +
    '<span class="sub">' + l.periodsPerWeek + '/wk</span>' +
    '<span class="acts">' +
      '<button onclick="event.stopPropagation();App.openLessonDialog(' + l.id + ')"><i class="fa-solid fa-pen"></i></button>' +
      '<button class="del" onclick="event.stopPropagation();App.deleteLesson(' + l.id + ')"><i class="fa-solid fa-trash"></i></button>' +
    '</span></div>'
  ).join('');
}

document.getElementById('sidebar-search').addEventListener('input', () => renderSidebar());

// ── RIBBON CONTENT ────────────────────────────────────────────────────────
const RIBBON_TABS = {
  home:
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn primary" onclick="App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i><span>Generate</span></button>' +
      '<button class="rbtn" onclick="App.openSolverOptions()"><i class="fa-solid fa-sliders"></i><span>Options</span></button>' +
      '<button class="rbtn" onclick="App.clearTimetable()"><i class="fa-solid fa-eraser"></i><span>Clear</span></button>' +
    '</div><div class="rgroup-lbl">Scheduling</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.switchTab(\'data\')"><i class="fa-solid fa-database"></i><span>Data</span></button>' +
      '<button class="rbtn" onclick="App.switchTab(\'timetable\')"><i class="fa-solid fa-calendar-week"></i><span>Timetable</span></button>' +
      '<button class="rbtn" onclick="App.switchTab(\'analytics\')"><i class="fa-solid fa-chart-bar"></i><span>Analytics</span></button>' +
    '</div><div class="rgroup-lbl">Navigate</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.exportHTML(\'class\')"><i class="fa-solid fa-file-export"></i><span>Export</span></button>' +
      '<button class="rbtn" onclick="App.exportData()"><i class="fa-solid fa-download"></i><span>Backup</span></button>' +
      '<button class="rbtn" onclick="App.importData()"><i class="fa-solid fa-upload"></i><span>Restore</span></button>' +
    '</div><div class="rgroup-lbl">File</div></div>',

  data:
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.openTeacherDialog()"><i class="fa-solid fa-chalkboard-user"></i><span>Teacher</span></button>' +
      '<button class="rbtn" onclick="App.openSubjectDialog()"><i class="fa-solid fa-book"></i><span>Subject</span></button>' +
      '<button class="rbtn" onclick="App.openClassDialog()"><i class="fa-solid fa-users"></i><span>Class</span></button>' +
      '<button class="rbtn" onclick="App.openRoomDialog()"><i class="fa-solid fa-door-open"></i><span>Room</span></button>' +
    '</div><div class="rgroup-lbl">Add Entity</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn primary" onclick="App.openLessonDialog()"><i class="fa-solid fa-plus-circle"></i><span>Add Lesson</span></button>' +
    '</div><div class="rgroup-lbl">Lessons</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.exportData()"><i class="fa-solid fa-download"></i><span>Export</span></button>' +
      '<button class="rbtn" onclick="App.importData()"><i class="fa-solid fa-upload"></i><span>Import</span></button>' +
    '</div><div class="rgroup-lbl">Data</div></div>',

  timetable:
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn active" id="rbtn-class" onclick="App.setTTView(\'class\')"><i class="fa-solid fa-users"></i><span>By Class</span></button>' +
      '<button class="rbtn" id="rbtn-teacher" onclick="App.setTTView(\'teacher\')"><i class="fa-solid fa-chalkboard-user"></i><span>By Teacher</span></button>' +
      '<button class="rbtn" id="rbtn-room" onclick="App.setTTView(\'room\')"><i class="fa-solid fa-door-open"></i><span>By Room</span></button>' +
    '</div><div class="rgroup-lbl">View</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn primary" onclick="App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i><span>Generate</span></button>' +
      '<button class="rbtn" onclick="App.openSolverOptions()"><i class="fa-solid fa-sliders"></i><span>Options</span></button>' +
    '</div><div class="rgroup-lbl">Solver</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.clearTimetable()"><i class="fa-solid fa-eraser"></i><span>Clear</span></button>' +
      '<button class="rbtn" onclick="App.clearAllTimetable()"><i class="fa-solid fa-trash-can"></i><span>Clear All</span></button>' +
    '</div><div class="rgroup-lbl">Reset</div></div>' +
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.exportHTML(S.ttView)"><i class="fa-solid fa-file-export"></i><span>HTML</span></button>' +
      '<button class="rbtn" onclick="window.print()"><i class="fa-solid fa-print"></i><span>Print</span></button>' +
    '</div><div class="rgroup-lbl">Export</div></div>',

  analytics:
    '<div class="rgroup"><div class="rbtns">' +
      '<button class="rbtn" onclick="App.loadAnalytics()"><i class="fa-solid fa-rotate"></i><span>Refresh</span></button>' +
    '</div><div class="rgroup-lbl">Analytics</div></div>',
};

// ── TAB SWITCHING ─────────────────────────────────────────────────────────
function switchTab(tab) {
  S.currentTab = tab;
  document.querySelectorAll('.rtab').forEach(b => b.classList.toggle('active', b.dataset.tab===tab));
  document.querySelectorAll('.view').forEach(v => v.style.display='none');
  const view = document.getElementById('view-' + tab);
  if (view) view.style.display = 'block';
  document.getElementById('ribbon-content').innerHTML = RIBBON_TABS[tab] || RIBBON_TABS.home;
  renderCurrentView();
}

function renderCurrentView() {
  if (S.currentTab==='home') renderHomeView();
  else if (S.currentTab==='data') renderDataView();
  else if (S.currentTab==='timetable') renderTimetableView();
  else if (S.currentTab==='analytics') App.loadAnalytics();
}

// ── HOME VIEW ─────────────────────────────────────────────────────────────
function renderHomeView() {
  const el = document.getElementById('view-home');
  const hasData = S.teachers.length>0 && S.lessons.length>0;
  const hasSlots = S.slots.length>0;
  if (!hasData) {
    el.innerHTML =
      '<div class="empty-st">' +
      '<i class="fa-solid fa-calendar-days"></i>' +
      '<h3>Welcome to Thorium234</h3>' +
      '<p>Start by adding <strong>Teachers</strong>, <strong>Subjects</strong>, <strong>Classes</strong>, and <strong>Rooms</strong> in the sidebar, then create <strong>Lessons</strong> and click <strong>Generate</strong>.</p>' +
      '<div style="margin-top:20px;display:flex;gap:10px;flex-wrap:wrap;justify-content:center">' +
        '<button class="btn btn-primary" onclick="App.openTeacherDialog()"><i class="fa-solid fa-plus"></i> Add Teacher</button>' +
        '<button class="btn btn-ghost" onclick="App.switchTab(\'data\')"><i class="fa-solid fa-database"></i> Data Entry</button>' +
      '</div></div>';
    return;
  }
  if (!hasSlots) {
    const totalPW = S.lessons.reduce((a,l)=>a+l.periodsPerWeek, 0);
    el.innerHTML =
      '<div class="empty-st">' +
      '<i class="fa-solid fa-wand-magic-sparkles"></i>' +
      '<h3>Ready to Generate</h3>' +
      '<p>You have <strong>' + S.lessons.length + ' lesson(s)</strong> (' + totalPW + ' periods/week) across <strong>' + S.classes.length + ' class(es)</strong>.</p>' +
      '<div style="margin-top:20px;display:flex;gap:10px;flex-wrap:wrap;justify-content:center">' +
        '<button class="btn btn-primary" onclick="App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i> Generate Timetable</button>' +
        '<button class="btn btn-ghost" onclick="App.openSolverOptions()"><i class="fa-solid fa-sliders"></i> Solver Options</button>' +
      '</div></div>';
    return;
  }
  const scheduled = S.slots.length;
  const required = S.lessons.reduce((a,l)=>a+l.periodsPerWeek, 0);
  const pct = required>0 ? Math.round(scheduled/required*100) : 0;
  const locked = S.slots.filter(s=>s.locked).length;
  const coverColor = pct>=95 ? 'var(--success)' : pct>=80 ? 'var(--warning)' : 'var(--danger)';
  el.innerHTML =
    '<div style="margin-bottom:16px">' +
    '<h2 style="color:var(--navy);font-size:16px;margin-bottom:14px"><i class="fa-solid fa-calendar-check" style="margin-right:8px;color:var(--accent)"></i>Timetable Summary</h2>' +
    '<div style="display:flex;gap:12px;flex-wrap:wrap">' +
      summaryCard('Scheduled', scheduled, 'fa-calendar-check', 'var(--navy)') +
      summaryCard('Required', required, 'fa-list-check', '#8e44ad') +
      summaryCard('Coverage', pct+'%', 'fa-chart-pie', coverColor) +
      summaryCard('Locked', locked, 'fa-lock', 'var(--accent)') +
      summaryCard('Score', S.score!==null?S.score:'—', 'fa-star', 'var(--accent)') +
      summaryCard('Conflicts', S.conflicts.length, 'fa-triangle-exclamation', S.conflicts.length>0?'var(--danger)':'var(--success)') +
    '</div></div>' +
    (S.unscheduled && S.unscheduled.length>0 ? renderUnscheduledBanner() : '') +
    '<div style="display:flex;gap:8px;flex-wrap:wrap">' +
      '<button class="btn btn-primary" onclick="App.switchTab(\'timetable\')"><i class="fa-solid fa-calendar"></i> View Timetable</button>' +
      '<button class="btn btn-accent" onclick="App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i> Re-Generate</button>' +
      '<button class="btn btn-ghost" onclick="App.openSolverOptions()"><i class="fa-solid fa-sliders"></i> Solver Options</button>' +
      (S.conflicts.length>0 ? '<button class="btn btn-danger" onclick="App.showConflicts()"><i class="fa-solid fa-triangle-exclamation"></i> View Conflicts</button>' : '') +
    '</div>';
}

function renderUnscheduledBanner() {
  const items = S.unscheduled.slice(0,5).map(u =>
    '<tr><td>' + esc(u.className||'?') + '</td><td>' + esc(u.subjectName||'') + '</td>' +
    '<td>' + u.placed + '/' + u.required + '</td><td style="color:var(--text-muted)">' + esc(u.reason||'') + '</td></tr>'
  ).join('');
  return '<div class="unsched-list" style="margin-bottom:16px">' +
    '<h4><i class="fa-solid fa-triangle-exclamation"></i> ' + S.unscheduled.length + ' unscheduled block(s)</h4>' +
    '<table><tr><th>Class</th><th>Subject</th><th>Placed</th><th>Reason</th></tr>' + items + '</table>' +
    (S.unscheduled.length>5 ? '<p style="font-size:11px;color:var(--text-muted);margin-top:6px">…and ' + (S.unscheduled.length-5) + ' more</p>' : '') +
  '</div>';
}

function summaryCard(label, val, icon, color) {
  return '<div style="background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:14px 18px;min-width:120px;box-shadow:var(--shadow)">' +
    '<div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:6px"><i class="fa-solid ' + icon + '" style="margin-right:5px;color:' + color + '"></i>' + label + '</div>' +
    '<div style="font-size:26px;font-weight:800;color:' + color + '">' + val + '</div>' +
  '</div>';
}

// ── DATA VIEW ──────────────────────────────────────────────────────────────
function renderDataView() {
  const el = document.getElementById('view-data');
  if (S.lessons.length===0) {
    el.innerHTML =
      '<div style="margin-bottom:14px;display:flex;align-items:center;gap:10px">' +
        '<h2 style="color:var(--navy);font-size:15px">Lessons</h2>' +
        '<button class="btn btn-primary btn-sm" onclick="App.openLessonDialog()"><i class="fa-solid fa-plus"></i> Add Lesson</button>' +
      '</div>' +
      '<div class="empty-st" style="padding:40px">' +
        '<i class="fa-solid fa-book-open"></i><h3>No lessons yet</h3>' +
        '<p>A lesson links a Teacher + Subject + Class and defines how many periods per week they meet.</p>' +
        (S.teachers.length===0 || S.subjects.length===0 || S.classes.length===0
          ? '<p style="margin-top:8px;color:var(--danger)"><i class="fa-solid fa-triangle-exclamation"></i> Add teachers, subjects &amp; classes first.</p>' : '') +
        '<button class="btn btn-primary" style="margin-top:16px" onclick="App.openLessonDialog()"><i class="fa-solid fa-plus"></i> Add First Lesson</button>' +
      '</div>';
    return;
  }
  const totalPW = S.lessons.reduce((a,l)=>a+l.periodsPerWeek, 0);
  el.innerHTML =
    '<div style="margin-bottom:14px;display:flex;align-items:center;gap:10px;flex-wrap:wrap">' +
      '<h2 style="color:var(--navy);font-size:15px">Lessons <span style="font-size:12px;color:var(--text-muted);font-weight:400">(' + S.lessons.length + ' lessons · ' + totalPW + ' periods/week)</span></h2>' +
      '<button class="btn btn-primary" onclick="App.openLessonDialog()"><i class="fa-solid fa-plus"></i> Add Lesson</button>' +
    '</div>' +
    '<table class="data-table"><thead><tr>' +
      '<th>Class</th><th>Subject</th><th>Teacher</th><th>Periods/Week</th><th>Block</th><th>Max/Day</th><th></th>' +
    '</tr></thead><tbody>' +
    S.lessons.map(l =>
      '<tr><td><strong>' + esc(l.className) + '</strong></td>' +
      '<td><span class="chip" style="background:' + (l.subjectColor||'#4A90D9') + '">' + esc(l.subjectName) + '</span></td>' +
      '<td>' + esc(l.teacherName) + '</td>' +
      '<td><span class="badge">' + l.periodsPerWeek + '</span></td>' +
      '<td>' + l.blockSize + '</td>' +
      '<td>' + (l.maxPerDay||'—') + '</td>' +
      '<td style="white-space:nowrap">' +
        '<button class="btn btn-ghost btn-sm" onclick="App.openLessonDialog(' + l.id + ')"><i class="fa-solid fa-pen"></i></button> ' +
        '<button class="btn btn-danger btn-sm" onclick="App.deleteLesson(' + l.id + ')"><i class="fa-solid fa-trash"></i></button>' +
      '</td></tr>'
    ).join('') + '</tbody></table>';
}

// ── TIMETABLE VIEW ────────────────────────────────────────────────────────
function renderTimetableView() {
  const el = document.getElementById('view-timetable');
  if (S.slots.length===0) {
    el.innerHTML =
      '<div class="empty-st"><i class="fa-solid fa-calendar-xmark"></i><h3>No timetable yet</h3>' +
      '<p>Click <strong>Generate</strong> in the ribbon to create a timetable.</p>' +
      '<button class="btn btn-primary" style="margin-top:16px" onclick="App.generate()"><i class="fa-solid fa-wand-magic-sparkles"></i> Generate</button></div>';
    return;
  }

  const view = S.ttView;
  let entities = view==='class' ? S.classes : view==='teacher' ? S.teachers : S.rooms;
  if (entities.length===0) {
    el.innerHTML = '<div class="empty-st"><i class="fa-solid fa-circle-exclamation"></i><h3>No ' + view + 's</h3><p>Add some first.</p></div>';
    return;
  }
  if (!S.ttEntity || !entities.find(e=>e.id===S.ttEntity)) S.ttEntity=entities[0].id;
  const entity = entities.find(e=>e.id===S.ttEntity);

  let filteredSlots = view==='class' ? S.slots.filter(s=>s.classId===S.ttEntity)
    : view==='teacher' ? S.slots.filter(s=>s.teacherId===S.ttEntity)
    : S.slots.filter(s=>s.roomId===S.ttEntity);

  const slotMap = {};
  filteredSlots.forEach(s => { slotMap[s.dayId+'_'+s.periodId] = s; });

  // Conflict keys for highlighting
  const conflictKeys = new Set(S.conflicts.map(c => c.dayId+'_'+c.periodId));

  let banner = '';
  if (S.unscheduled && S.unscheduled.length>0) {
    banner = '<div class="warn-banner"><i class="fa-solid fa-triangle-exclamation"></i><strong>' +
      S.unscheduled.length + ' unscheduled</strong> — some periods could not be placed. Try Re-Generate.</div>';
  }

  const navBtns = entities.map(e =>
    '<button class="tt-nav' + (e.id===S.ttEntity?' active':'') + '" onclick="App.selectTTEntity(' + e.id + ')">' + esc(e.name) + '</button>'
  ).join('');

  const rows = S.periods.map(p => {
    const cells = S.days.map(d => {
      const slot = slotMap[d.id+'_'+p.id];
      const isConflict = conflictKeys.has(d.id+'_'+p.id);
      const cellId = 'cell-'+d.id+'-'+p.id;
      if (slot) {
        const color = slot.subjectColor || '#4A90D9';
        const isLocked = slot.locked;
        const lockIcon = isLocked
          ? '<button title="Unlock" onclick="App.toggleLock(' + slot.classId + ',' + slot.dayId + ',' + slot.periodId + ',false)"><i class="fa-solid fa-lock"></i></button>'
          : '<button title="Lock (pin)" onclick="App.toggleLock(' + slot.classId + ',' + slot.dayId + ',' + slot.periodId + ',true)"><i class="fa-solid fa-lock-open"></i></button>';
        return '<td id="' + cellId + '" data-day="' + d.id + '" data-period="' + p.id + '"' +
          (isConflict?' class="conflict-cell"':'') +
          ' ondragover="App.onDragOver(event,this)" ondrop="App.onDrop(event,this)" ondragleave="App.onDragLeave(this)">' +
          '<div class="lcard' + (isLocked?' locked':'') + '" style="background:' + color + '"' +
            (isLocked ? '' : ' draggable="true"') +
            ' data-class="' + slot.classId + '" data-day="' + slot.dayId + '" data-period="' + slot.periodId + '"' +
            (isLocked ? '' : ' ondragstart="App.onDragStart(event,this)"') + '>' +
            '<div>' +
              '<div class="lc-subj">' + esc(slot.subjectName||'') + '</div>' +
              '<div class="lc-teacher"><i class="fa-solid fa-user" style="font-size:9px;margin-right:3px"></i>' + esc(slot.teacherName||'') + '</div>' +
              '<div class="lc-room"><i class="fa-solid fa-door-open" style="font-size:9px;margin-right:3px"></i>' + esc(slot.roomName||'—') + '</div>' +
            '</div>' +
            '<div class="lc-acts">' + lockIcon +
              '<button title="Remove" onclick="App.removeSlot(' + slot.classId + ',' + slot.dayId + ',' + slot.periodId + ')"><i class="fa-solid fa-xmark"></i></button>' +
            '</div>' +
          '</div></td>';
      }
      return '<td id="' + cellId + '" data-day="' + d.id + '" data-period="' + p.id + '"' +
        (isConflict?' class="conflict-cell"':'') +
        ' ondragover="App.onDragOver(event,this)" ondrop="App.onDrop(event,this)" ondragleave="App.onDragLeave(this)"></td>';
    }).join('');
    return '<tr><td class="pcell"><span class="p-lbl">P' + p.id + '</span><span class="p-time">' + p.start + '–' + p.end + '</span></td>' + cells + '</tr>';
  }).join('');

  el.innerHTML = banner +
    '<div class="tt-hd"><h2><i class="fa-solid fa-calendar-week" style="margin-right:8px"></i>' + esc(entity.name) + '</h2>' +
    '<div class="tt-sel">' + navBtns + '</div></div>' +
    '<div class="tt-wrap"><table><thead><tr>' +
      '<th class="pcol">Period</th>' + S.days.map(d=>'<th>'+d.name+'</th>').join('') +
    '</tr></thead><tbody>' + rows + '</tbody></table></div>';
}

// ── GENERATE ──────────────────────────────────────────────────────────────
async function generate() {
  showSpinner('Generating timetable…');
  try {
    const result = await api.post('/api/generate', {
      seed: S.solverSeed,
      algorithm: S.solverAlgo,
    });
    S.unscheduled = result.unscheduled || [];
    S.score = result.score;
    document.getElementById('score-num').textContent = result.score;
    S.slots = await api.get('/api/timetable');
    await checkConflicts();
    renderCurrentView();
    renderHomeView();
    if (result.unscheduled && result.unscheduled.length>0) {
      toast(result.unscheduled.length + ' period(s) unscheduled — try adjusting constraints', 'warn');
    } else {
      const algo = result.algorithm==='backtrack' ? 'Backtracking' : 'Greedy';
      toast(algo + ' · Score: ' + result.score + (result.softViolations ? ' · Soft violations: '+result.softViolations : ''), 'success');
    }
    if (S.currentTab!=='timetable') switchTab('timetable');
  } catch(e) {
    toast(e.message||'Generation failed', 'error');
  } finally {
    hideSpinner();
  }
}

async function clearTimetable() {
  if (!confirm('Clear unlocked slots? Locked (pinned) slots will be kept.')) return;
  try {
    await api.post('/api/timetable/clear', {});
    S.slots = await api.get('/api/timetable');
    S.unscheduled = [];
    document.getElementById('score-num').textContent = '—';
    await checkConflicts();
    renderCurrentView();
    toast('Unlocked slots cleared', 'warn');
  } catch(e) { toast(e.message, 'error'); }
}

async function clearAllTimetable() {
  if (!confirm('Clear ALL slots including locked ones? This cannot be undone.')) return;
  try {
    await api.post('/api/timetable/clear_all', {});
    S.slots = []; S.unscheduled = []; S.score = null;
    document.getElementById('score-num').textContent = '—';
    await checkConflicts();
    renderCurrentView();
    toast('All slots cleared', 'warn');
  } catch(e) { toast(e.message, 'error'); }
}

// ── SOLVER OPTIONS ────────────────────────────────────────────────────────
function openSolverOptions() {
  const body =
    '<div class="solver-opt">' +
    '<div class="fg"><label>Algorithm</label><select id="opt-algo">' +
      '<option value="backtrack"' + (S.solverAlgo==='backtrack'?' selected':'') + '>Backtracking (Constraint-Based) — Best quality</option>' +
      '<option value="greedy"' + (S.solverAlgo==='greedy'?' selected':'') + '>Greedy (Fast) — Random shuffle</option>' +
    '</select></div>' +
    '<div class="fg"><label>Random Seed <span style="color:var(--text-muted);text-transform:none;font-weight:400">(change for different results)</span></label>' +
      '<div style="display:flex;gap:8px;align-items:center">' +
        '<input type="number" id="opt-seed" value="' + S.solverSeed + '" min="0" max="99999" style="flex:1">' +
        '<button class="btn btn-ghost btn-sm" onclick="document.getElementById(\'opt-seed\').value=Math.floor(Math.random()*99999)"><i class="fa-solid fa-dice"></i></button>' +
      '</div>' +
    '</div>' +
    '<div style="background:#f0f4f8;border-radius:var(--radius);padding:12px;font-size:12px;color:var(--text-muted)">' +
      '<strong style="color:var(--navy)">Backtracking solver</strong> scores each candidate slot using soft constraints (teacher preferences, max consecutive periods, day distribution) and picks the best option. Respects locked slots.<br><br>' +
      '<strong style="color:var(--navy)">Greedy solver</strong> is faster for large datasets but ignores soft constraints.' +
    '</div></div>';
  openModal('Solver Options', 'sliders', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveSolverOptions()"><i class="fa-solid fa-check"></i> Save &amp; Generate</button>');
}

async function saveSolverOptions() {
  S.solverAlgo = document.getElementById('opt-algo').value;
  S.solverSeed = parseInt(document.getElementById('opt-seed').value) || 42;
  closeModal();
  await generate();
}

// ── SLOT LOCK / REMOVE ───────────────────────────────────────────────────
async function toggleLock(classId, dayId, periodId, lock) {
  try {
    await api.post('/api/timetable/lock', {classId, dayId, periodId, locked: lock});
    S.slots = await api.get('/api/timetable');
    renderTimetableView();
    toast(lock ? 'Slot pinned — won\'t move on re-generate' : 'Slot unpinned', 'success');
  } catch(e) { toast(e.message, 'error'); }
}

async function removeSlot(classId, dayId, periodId) {
  try {
    await api.post('/api/timetable/remove', {classId, dayId, periodId});
    S.slots = await api.get('/api/timetable');
    await checkConflicts();
    renderTimetableView();
    toast('Slot removed');
  } catch(e) { toast(e.message, 'error'); }
}

// ── DRAG & DROP ──────────────────────────────────────────────────────────
let dragData = null;
function onDragStart(e, el) {
  dragData = {
    classId: parseInt(el.dataset.class),
    dayId: parseInt(el.dataset.day),
    periodId: parseInt(el.dataset.period),
  };
  e.dataTransfer.effectAllowed = 'move';
  el.style.opacity = '0.5';
}
function onDragOver(e, td) {
  e.preventDefault();
  td.classList.add('drag-over');
}
function onDragLeave(td) { td.classList.remove('drag-over'); }
async function onDrop(e, td) {
  e.preventDefault();
  td.classList.remove('drag-over');
  if (!dragData) return;
  const toDay = parseInt(td.dataset.day);
  const toPeriod = parseInt(td.dataset.period);
  if (dragData.dayId===toDay && dragData.periodId===toPeriod) { dragData=null; return; }
  try {
    await api.post('/api/timetable/move', {
      classId: dragData.classId,
      fromDay: dragData.dayId, fromPeriod: dragData.periodId,
      toDay, toPeriod,
    });
    S.slots = await api.get('/api/timetable');
    await checkConflicts();
    renderTimetableView();
    toast('Lesson moved', 'success');
  } catch(e2) { toast(e2.message||'Move failed', 'error'); }
  finally { dragData = null; }
}

// ── TEACHER DIALOG ────────────────────────────────────────────────────────
function openTeacherDialog(id) {
  const t = id ? S.teachers.find(x=>x.id===id) : null;
  const body =
    '<div class="fg"><label>Full Name</label><input type="text" id="t-name" value="' + esc(t?t.name:'') + '" placeholder="e.g. Mr. Smith"></div>' +
    '<div class="fg"><label>Max Consecutive Periods <span style="text-transform:none;font-weight:400;color:var(--text-muted)">(0 = unlimited)</span></label><input type="number" id="t-maxc" value="' + (t?t.maxConsecutive:0) + '" min="0" max="8"></div>' +
    '<div class="fg"><label>Color</label>' + colorPickerHtml(t?t.color:autoColor(S.teachers)) + '</div>' +
    (t ? '<button class="btn btn-ghost" onclick="App.openConstraintsDialog('+t.id+')" style="margin-top:4px"><i class="fa-solid fa-calendar-xmark"></i> Set Availability / Preferences</button>' : '');
  openModal(t?'Edit Teacher':'Add Teacher', 'chalkboard-user', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveTeacher(' + (t?t.id:'null') + ')"><i class="fa-solid fa-check"></i> Save</button>');
}

async function saveTeacher(id) {
  const name = document.getElementById('t-name').value.trim();
  const maxc = parseInt(document.getElementById('t-maxc').value)||0;
  const color = document.getElementById('inp-color').value;
  if (!name) { toast('Name required', 'error'); return; }
  try {
    if (id) { await api.put('/api/teachers/'+id, {name,maxConsecutive:maxc,color}); toast('Teacher updated','success'); }
    else { await api.post('/api/teachers', {name,maxConsecutive:maxc,color}); toast('Teacher added','success'); }
    closeModal(); await loadAll();
  } catch(e) { toast(e.message,'error'); }
}

// ── SUBJECT DIALOG ────────────────────────────────────────────────────────
function openSubjectDialog(id) {
  const s = id ? S.subjects.find(x=>x.id===id) : null;
  const body =
    '<div class="fg"><label>Subject Name</label><input type="text" id="s-name" value="' + esc(s?s.name:'') + '" placeholder="e.g. Mathematics"></div>' +
    '<div class="fg"><label>Color</label>' + colorPickerHtml(s?s.color:autoColor(S.subjects)) + '</div>';
  openModal(s?'Edit Subject':'Add Subject', 'book', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveSubject(' + (s?s.id:'null') + ')"><i class="fa-solid fa-check"></i> Save</button>');
}

async function saveSubject(id) {
  const name = document.getElementById('s-name').value.trim();
  const color = document.getElementById('inp-color').value;
  if (!name) { toast('Name required','error'); return; }
  try {
    if (id) { await api.put('/api/subjects/'+id, {name,color}); toast('Subject updated','success'); }
    else { await api.post('/api/subjects', {name,color}); toast('Subject added','success'); }
    closeModal(); await loadAll();
  } catch(e) { toast(e.message,'error'); }
}

// ── CLASS DIALOG ──────────────────────────────────────────────────────────
function openClassDialog(id) {
  const c = id ? S.classes.find(x=>x.id===id) : null;
  const body =
    '<div class="fg"><label>Class Name</label><input type="text" id="c-name" value="' + esc(c?c.name:'') + '" placeholder="e.g. Grade 10A"></div>' +
    '<div class="fg"><label>Number of Students</label><input type="number" id="c-sc" value="' + (c?c.studentCount:30) + '" min="1" max="500"></div>';
  openModal(c?'Edit Class':'Add Class', 'users', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveClass(' + (c?c.id:'null') + ')"><i class="fa-solid fa-check"></i> Save</button>');
}

async function saveClass(id) {
  const name = document.getElementById('c-name').value.trim();
  const sc = parseInt(document.getElementById('c-sc').value)||0;
  if (!name) { toast('Name required','error'); return; }
  try {
    if (id) { await api.put('/api/classes/'+id, {name,studentCount:sc}); toast('Class updated','success'); }
    else { await api.post('/api/classes', {name,studentCount:sc}); toast('Class added','success'); }
    closeModal(); await loadAll();
  } catch(e) { toast(e.message,'error'); }
}

// ── ROOM DIALOG ───────────────────────────────────────────────────────────
function openRoomDialog(id) {
  const r = id ? S.rooms.find(x=>x.id===id) : null;
  const body =
    '<div class="fg"><label>Room Name</label><input type="text" id="r-name" value="' + esc(r?r.name:'') + '" placeholder="e.g. Room 101"></div>' +
    '<div class="form-row">' +
      '<div class="fg"><label>Capacity</label><input type="number" id="r-cap" value="' + (r?r.capacity:30) + '" min="1"></div>' +
      '<div class="fg"><label>Room Type</label><select id="r-type">' +
        S.roomTypes.map(rt=>'<option value="'+rt.id+'"'+(r&&r.roomTypeId===rt.id?' selected':'')+'>'+esc(rt.name)+'</option>').join('') +
      '</select></div>' +
    '</div>';
  openModal(r?'Edit Room':'Add Room', 'door-open', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveRoom(' + (r?r.id:'null') + ')"><i class="fa-solid fa-check"></i> Save</button>');
}

async function saveRoom(id) {
  const name = document.getElementById('r-name').value.trim();
  const cap = parseInt(document.getElementById('r-cap').value)||30;
  const rt = parseInt(document.getElementById('r-type').value)||1;
  if (!name) { toast('Name required','error'); return; }
  try {
    if (id) { await api.put('/api/rooms/'+id, {name,capacity:cap,roomTypeId:rt}); toast('Room updated','success'); }
    else { await api.post('/api/rooms', {name,capacity:cap,roomTypeId:rt}); toast('Room added','success'); }
    closeModal(); await loadAll();
  } catch(e) { toast(e.message,'error'); }
}

// ── LESSON DIALOG ─────────────────────────────────────────────────────────
function openLessonDialog(id) {
  const l = id ? S.lessons.find(x=>x.id===id) : null;
  if (S.teachers.length===0||S.subjects.length===0||S.classes.length===0) {
    toast('Add teachers, subjects and classes first','warn'); return;
  }
  const body =
    '<div class="fg"><label>Class</label><select id="l-class">' +
      S.classes.map(c=>'<option value="'+c.id+'"'+(l&&l.classId===c.id?' selected':'')+'>'+esc(c.name)+'</option>').join('') +
    '</select></div>' +
    '<div class="fg"><label>Subject</label><select id="l-subject">' +
      S.subjects.map(s=>'<option value="'+s.id+'"'+(l&&l.subjectId===s.id?' selected':'')+'>'+esc(s.name)+'</option>').join('') +
    '</select></div>' +
    '<div class="fg"><label>Teacher</label><select id="l-teacher">' +
      S.teachers.map(t=>'<option value="'+t.id+'"'+(l&&l.teacherId===t.id?' selected':'')+'>'+esc(t.name)+'</option>').join('') +
    '</select></div>' +
    '<div class="form-row">' +
      '<div class="fg"><label>Periods / Week</label><input type="number" id="l-ppw" value="' + (l?l.periodsPerWeek:2) + '" min="1" max="40"></div>' +
      '<div class="fg"><label>Block Size</label><input type="number" id="l-block" value="' + (l?l.blockSize:1) + '" min="1" max="4"></div>' +
    '</div>' +
    '<div class="fg"><label>Max Per Day <span style="text-transform:none;font-weight:400;color:var(--text-muted)">(0 = unlimited)</span></label>' +
      '<input type="number" id="l-mpd" value="' + (l?l.maxPerDay:0) + '" min="0" max="8"></div>';
  openModal(l?'Edit Lesson':'Add Lesson', 'book-open', body,
    '<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>' +
    '<button class="btn btn-primary" onclick="App.saveLesson(' + (l?l.id:'null') + ')"><i class="fa-solid fa-check"></i> Save</button>');
}

async function saveLesson(id) {
  const classId = parseInt(document.getElementById('l-class').value);
  const subjectId = parseInt(document.getElementById('l-subject').value);
  const teacherId = parseInt(document.getElementById('l-teacher').value);
  const ppw = parseInt(document.getElementById('l-ppw').value)||1;
  const block = parseInt(document.getElementById('l-block').value)||1;
  const mpd = parseInt(document.getElementById('l-mpd').value)||0;
  try {
    if (id) {
      await api.put('/api/lessons/'+id, {classId,subjectId,teacherId,periodsPerWeek:ppw,blockSize:block,maxPerDay:mpd});
      toast('Lesson updated','success');
    } else {
      await api.post('/api/lessons', {classId,subjectId,teacherId,periodsPerWeek:ppw,blockSize:block,maxPerDay:mpd});
      toast('Lesson added','success');
    }
    closeModal(); await loadAll();
    if (S.currentTab==='data') renderDataView();
  } catch(e) { toast(e.message,'error'); }
}

async function deleteLesson(id) {
  if (!confirm('Delete this lesson?')) return;
  try {
    await api.del('/api/lessons/'+id);
    await loadAll(); toast('Lesson deleted');
    renderDataView();
  } catch(e) { toast(e.message,'error'); }
}

// ── CONSTRAINTS / PREFERENCES DIALOG ─────────────────────────────────────
async function openConstraintsDialog(teacherId) {
  const teacher = S.teachers.find(t=>t.id===teacherId);
  showSpinner('Loading availability…');
  let constraints, prefs;
  try {
    [constraints, prefs] = await Promise.all([
      api.get('/api/constraints/'+teacherId),
      api.get('/api/preferences/'+teacherId),
    ]);
  } finally { hideSpinner(); }

  const blockedSet = new Set(constraints.map(c=>c.dayId+'_'+c.periodId));
  const prefMap = {};
  prefs.forEach(p => { prefMap[p.dayId+'_'+p.periodId] = p.prefType; });

  const legend =
    '<div style="display:flex;gap:14px;margin-bottom:12px;font-size:11px;align-items:center;flex-wrap:wrap">' +
      '<span><span style="display:inline-block;width:14px;height:14px;background:var(--surface);border:1px solid var(--border);border-radius:3px;vertical-align:middle;margin-right:4px"></span>Available</span>' +
      '<span><span style="display:inline-block;width:14px;height:14px;background:#fde8e8;border:1px solid #e74c3c;border-radius:3px;vertical-align:middle;margin-right:4px"></span>Unavailable</span>' +
      '<span><span style="display:inline-block;width:14px;height:14px;background:#e6f8ee;border:1px solid #27ae60;border-radius:3px;vertical-align:middle;margin-right:4px"></span>Preferred (+score)</span>' +
      '<span><span style="display:inline-block;width:14px;height:14px;background:#fff8e4;border:1px solid #f39c12;border-radius:3px;vertical-align:middle;margin-right:4px"></span>Undesirable (−score)</span>' +
      '<span style="color:var(--text-muted);font-style:italic">Click to cycle states</span>' +
    '</div>';

  const table =
    '<div class="cg-wrap"><table><thead><tr><th style="min-width:60px">Period</th>' +
    S.days.map(d=>'<th>'+d.name.slice(0,3)+'</th>').join('') +
    '</tr></thead><tbody>' +
    S.periods.map(p => {
      const cells = S.days.map(d => {
        const key = d.id+'_'+p.id;
        const blocked = blockedSet.has(key);
        const pref = prefMap[key]||'NEUTRAL';
        const cls = blocked?'blocked':pref==='PREFERRED'?'preferred':pref==='UNDESIRABLE'?'undesirable':'';
        const icon = blocked?'✕':pref==='PREFERRED'?'★':pref==='UNDESIRABLE'?'~':'';
        return '<td><div class="cg-cell '+cls+'" data-tid="'+teacherId+'" data-day="'+d.id+'" data-period="'+p.id+'" data-blocked="'+blocked+'" data-pref="'+pref+'" onclick="App.toggleConstraint(this)">'+icon+'</div></td>';
      }).join('');
      return '<tr><td style="padding:4px 10px;font-size:11px;font-weight:600;background:#f8fafc;text-align:center;white-space:nowrap">P'+p.id+'<br><small style="color:var(--text-muted);font-weight:400">'+p.start+'</small></td>'+cells+'</tr>';
    }).join('') + '</tbody></table></div>';

  closeModal();
  openModal('Availability — '+esc(teacher.name), 'calendar-xmark', legend+table,
    '<button class="btn btn-ghost" onclick="closeModal()">Done</button>', true);
}

async function toggleConstraint(el) {
  const tid = parseInt(el.dataset.tid);
  const dayId = parseInt(el.dataset.day);
  const periodId = parseInt(el.dataset.period);
  const blocked = el.dataset.blocked==='true';
  const pref = el.dataset.pref;
  try {
    if (!blocked && pref==='NEUTRAL') {
      await api.post('/api/constraints', {teacherId:tid,dayId,periodId});
      el.dataset.blocked='true'; el.className='cg-cell blocked'; el.textContent='✕';
    } else if (blocked) {
      await api.post('/api/constraints/remove', {teacherId:tid,dayId,periodId});
      await api.post('/api/preferences', {teacherId:tid,dayId,periodId,prefType:'PREFERRED'});
      el.dataset.blocked='false'; el.dataset.pref='PREFERRED'; el.className='cg-cell preferred'; el.textContent='★';
    } else if (pref==='PREFERRED') {
      await api.post('/api/preferences', {teacherId:tid,dayId,periodId,prefType:'UNDESIRABLE'});
      el.dataset.pref='UNDESIRABLE'; el.className='cg-cell undesirable'; el.textContent='~';
    } else {
      await api.post('/api/preferences', {teacherId:tid,dayId,periodId,prefType:'NEUTRAL'});
      el.dataset.pref='NEUTRAL'; el.className='cg-cell'; el.textContent='';
    }
  } catch(e) { toast(e.message,'error'); }
}

// ── ANALYTICS ─────────────────────────────────────────────────────────────
async function loadAnalytics() {
  const el = document.getElementById('view-analytics');
  showSpinner('Loading analytics…');
  try {
    const data = await api.get('/api/analytics');
    hideSpinner();
    const totalReq = S.lessons.reduce((a,l)=>a+l.periodsPerWeek, 0);
    const dayNames = ['Mon','Tue','Wed','Thu','Fri'];
    const maxDay = Math.max(...(data.dayDistribution.map(d=>d.cnt)||[1]),1);
    const maxPeriod = Math.max(...(data.periodDistribution.map(p=>p.cnt)||[1]),1);

    const teacherBars = data.teacherLoad.map(t => {
      const pct = t.totalRequired>0 ? Math.round(t.scheduledPeriods/t.totalRequired*100) : 0;
      const cls = pct>=95?'ok':pct>=70?'':'warn danger';
      return '<div class="bar-item">' +
        '<span class="bar-lbl" title="'+esc(t.name)+'">' +
          (t.color?'<span style="display:inline-block;width:8px;height:8px;border-radius:50%;background:'+t.color+';margin-right:5px"></span>':'') +
          esc(t.name) + '</span>' +
        '<div class="bar-track"><div class="bar-fill '+cls+'" style="width:'+Math.min(100,pct)+'%"></div></div>' +
        '<span class="bar-pct">'+t.scheduledPeriods+'/'+t.totalRequired+'</span></div>';
    }).join('') || '<p style="color:var(--text-muted);font-size:12px">No teachers yet</p>';

    const roomBars = data.roomUtilization.map(r => {
      const pct = Math.round(r.used/r.total*100);
      return '<div class="bar-item">' +
        '<span class="bar-lbl" title="'+esc(r.name)+'">'+esc(r.name)+'</span>' +
        '<div class="bar-track"><div class="bar-fill ok" style="width:'+Math.min(100,pct)+'%"></div></div>' +
        '<span class="bar-pct">'+pct+'%</span></div>';
    }).join('') || '<p style="color:var(--text-muted);font-size:12px">No rooms yet</p>';

    const classBars = data.classSummary.map(c => {
      const pct = c.required>0 ? Math.round(c.scheduled/c.required*100) : 0;
      const cls = pct>=95?'ok':pct>=70?'':'warn';
      return '<div class="bar-item">' +
        '<span class="bar-lbl">'+esc(c.name)+'</span>' +
        '<div class="bar-track"><div class="bar-fill '+cls+'" style="width:'+Math.min(100,pct)+'%"></div></div>' +
        '<span class="bar-pct">'+c.scheduled+'/'+c.required+'</span></div>';
    }).join('') || '<p style="color:var(--text-muted);font-size:12px">No classes yet</p>';

    const dayBars = dayNames.map((name, i) => {
      const d = data.dayDistribution.find(x=>x.dayId===i+1);
      const cnt = d?d.cnt:0;
      const pct = Math.round(cnt/maxDay*100);
      return '<div class="bar-item">' +
        '<span class="bar-lbl">'+name+'</span>' +
        '<div class="bar-track"><div class="bar-fill ok" style="width:'+pct+'%"></div></div>' +
        '<span class="bar-pct">'+cnt+'</span></div>';
    }).join('');

    const periodBars = S.periods.map(p => {
      const d = data.periodDistribution.find(x=>x.periodId===p.id);
      const cnt = d?d.cnt:0;
      const pct = Math.round(cnt/maxPeriod*100);
      return '<div class="bar-item">' +
        '<span class="bar-lbl">P'+p.id+' '+p.start+'</span>' +
        '<div class="bar-track"><div class="bar-fill" style="width:'+pct+'%;background:var(--navy-light)"></div></div>' +
        '<span class="bar-pct">'+cnt+'</span></div>';
    }).join('');

    el.innerHTML =
      '<h2 style="color:var(--navy);font-size:15px;margin-bottom:14px"><i class="fa-solid fa-chart-bar" style="margin-right:8px;color:var(--accent)"></i>Analytics Dashboard</h2>' +
      '<div class="analytics-grid">' +
        '<div class="stat-card">' +
          '<h4><i class="fa-solid fa-calendar-check"></i> Overview</h4>' +
          '<div class="val">' + data.totalScheduled + '</div>' +
          '<div class="sub">of ' + totalReq + ' required periods</div>' +
          (data.lockedSlots>0 ? '<div class="sub" style="margin-top:4px"><i class="fa-solid fa-lock" style="color:var(--accent)"></i> ' + data.lockedSlots + ' locked</div>' : '') +
          (S.conflicts.length>0 ? '<div class="sub" style="color:var(--danger);margin-top:4px"><i class="fa-solid fa-triangle-exclamation"></i> ' + S.conflicts.length + ' conflicts</div>' : '<div class="sub" style="color:var(--success);margin-top:4px"><i class="fa-solid fa-circle-check"></i> No conflicts</div>') +
        '</div>' +
        '<div class="stat-card"><h4><i class="fa-solid fa-chalkboard-user"></i> Teacher Workload</h4>' + teacherBars + '</div>' +
        '<div class="stat-card"><h4><i class="fa-solid fa-users"></i> Class Coverage</h4>' + classBars + '</div>' +
        '<div class="stat-card"><h4><i class="fa-solid fa-door-open"></i> Room Utilization</h4>' + roomBars + '</div>' +
        '<div class="stat-card"><h4><i class="fa-solid fa-calendar-days"></i> Day Distribution</h4>' + dayBars + '</div>' +
        '<div class="stat-card"><h4><i class="fa-solid fa-clock"></i> Period Load</h4>' + periodBars + '</div>' +
      '</div>';
  } catch(e) {
    hideSpinner();
    toast('Analytics error: '+e.message,'error');
  }
}

// ── ENTITY CRUD ───────────────────────────────────────────────────────────
function selectEntity(type, id) {
  document.querySelectorAll('.ei').forEach(el=>el.classList.remove('sel'));
  const el = document.querySelector('.ei[data-type="'+type+'"][data-id="'+id+'"]');
  if (el) el.classList.add('sel');
  if (type==='classes') { S.ttView='class'; S.ttEntity=id; switchTab('timetable'); }
  else if (type==='teachers') { S.ttView='teacher'; S.ttEntity=id; switchTab('timetable'); }
  else if (type==='rooms') { S.ttView='room'; S.ttEntity=id; switchTab('timetable'); }
}

function editEntity(type, id) {
  if (type==='teachers') openTeacherDialog(id);
  else if (type==='subjects') openSubjectDialog(id);
  else if (type==='classes') openClassDialog(id);
  else if (type==='rooms') openRoomDialog(id);
}

async function deleteEntity(type, id) {
  const labels = {teachers:'teacher',subjects:'subject',classes:'class',rooms:'room'};
  if (!confirm('Delete this '+(labels[type]||type)+'? Related lessons will also be removed.')) return;
  try {
    await api.del('/api/'+type+'/'+id);
    await loadAll(); toast((labels[type]||type)+' deleted');
    renderCurrentView();
  } catch(e) { toast(e.message,'error'); }
}

// ── TIMETABLE HELPERS ──────────────────────────────────────────────────────
function setTTView(view) {
  S.ttView = view; S.ttEntity = null;
  ['class','teacher','room'].forEach(v => {
    const btn = document.getElementById('rbtn-'+v);
    if (btn) btn.classList.toggle('active', v===view);
  });
  renderTimetableView();
}
function selectTTEntity(id) { S.ttEntity=id; renderTimetableView(); }
function openLessonsView() { switchTab('data'); setTimeout(()=>{ if(S.lessons.length===0) App.openLessonDialog(); },100); }
function exportHTML(view) { window.open('/api/export/html?view='+view,'_blank'); }

function exportData() {
  window.open('/api/data/export','_blank');
}

function importData() {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.json,application/json';
  input.onchange = async e => {
    const file = e.target.files[0];
    if (!file) return;
    try {
      const text = await file.text();
      const data = JSON.parse(text);
      if (!confirm('This will replace all your current data with the imported data. Continue?')) return;
      showSpinner('Importing…');
      await api.post('/api/data/import', data);
      await loadAll();
      toast('Data imported successfully','success');
    } catch(err) {
      toast('Import failed: '+(err.message||err),'error');
    } finally { hideSpinner(); }
  };
  input.click();
}

// ── UTILS ──────────────────────────────────────────────────────────────────
function esc(s) {
  if (!s) return '';
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function togglePanel(type) {
  document.getElementById('sp-'+type).classList.toggle('collapsed');
}

// ── APP OBJECT ─────────────────────────────────────────────────────────────
window.App = {
  switchTab, generate, clearTimetable, clearAllTimetable,
  openSolverOptions, saveSolverOptions,
  openTeacherDialog, saveTeacher,
  openSubjectDialog, saveSubject,
  openClassDialog, saveClass,
  openRoomDialog, saveRoom,
  openLessonDialog, saveLesson, deleteLesson,
  openLessonsView, openConstraintsDialog, toggleConstraint,
  loadAnalytics, exportHTML, exportData, importData,
  selectEntity, editEntity, deleteEntity,
  setTTView, selectTTEntity,
  togglePanel, toggleLock, removeSlot,
  onDragStart, onDragOver, onDragLeave, onDrop,
  showConflicts,
};

// ── INIT ───────────────────────────────────────────────────────────────────
document.querySelectorAll('.rtab').forEach(btn => {
  btn.addEventListener('click', () => switchTab(btn.dataset.tab));
});
document.getElementById('ribbon-content').innerHTML = RIBBON_TABS.home;
loadAll();
