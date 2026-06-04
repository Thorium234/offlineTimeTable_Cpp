#include "TimetableViewWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolTip>

TimetableViewWidget::TimetableViewWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
}

void TimetableViewWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    viewTitle = new QLabel(tr("Class Timetable"), this);
    viewTitle->setObjectName("GridTitle");
    viewTitle->setFixedHeight(32);
    mainLayout->addWidget(viewTitle);

    gridWidget = new QTableWidget(this);
    gridWidget->setObjectName("TimetableGrid");
    gridWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    gridWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    gridWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gridWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    gridWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    gridWidget->setShowGrid(true);
    mainLayout->addWidget(gridWidget, 1);

    // Empty state
    emptyState = new QWidget(this);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyState);
    QLabel *emptyLabel = new QLabel(tr("No timetable generated yet.\nAdd resources and lessons, then click Generate."), emptyState);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #555; font-size: 14pt; border: none; background: transparent;");
    emptyLayout->addWidget(emptyLabel);
    emptyState->setVisible(true);
    mainLayout->addWidget(emptyState);

    connect(gridWidget, &QTableWidget::cellClicked, this, &TimetableViewWidget::onCellClicked);
}

void TimetableViewWidget::setTimetable(const Timetable &t) {
    timetable = t;
    dm->timetableGenerated = true;
    dm->lastTimetable = t;
    emptyState->setVisible(false);
    gridWidget->setVisible(true);
    populateGrid();
}

void TimetableViewWidget::setViewMode(ViewMode mode) {
    currentMode = mode;
    selectedEntityId = -1;
    if (dm->timetableGenerated) {
        populateGrid();
    }
}

void TimetableViewWidget::setViewSelection(const QString &type, int id) {
    selectedEntityId = id;
    if (dm->timetableGenerated) {
        populateGrid();
    }
}

void TimetableViewWidget::refresh() {
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
}

void TimetableViewWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
}

void TimetableViewWidget::populateGrid() {
    gridWidget->clear();

    if (dm->days.empty() || dm->periods.empty()) {
        gridWidget->setRowCount(0);
        gridWidget->setColumnCount(0);
        return;
    }

    switch (currentMode) {
        case ViewMode::CLASS: populateClassGrid(); break;
        case ViewMode::TEACHER: populateTeacherGrid(); break;
        case ViewMode::ROOM: populateRoomGrid(); break;
    }
}

void TimetableViewWidget::populateClassGrid() {
    int numDays = static_cast<int>(dm->days.size());
    int numPeriods = static_cast<int>(dm->periods.size());

    gridWidget->setRowCount(numDays);
    gridWidget->setColumnCount(numPeriods + 1);

    // Horizontal headers
    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    gridWidget->setHorizontalHeaderLabels(headers);

    // Vertical headers
    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    gridWidget->setVerticalHeaderLabels(dayLabels);

    // Determine which classes to show
    auto iterBegin = timetable.schedules.begin();
    auto iterEnd = timetable.schedules.end();

    if (selectedEntityId > 0) {
        // Show only selected class
        auto it = timetable.schedules.find(selectedEntityId);
        if (it != timetable.schedules.end()) {
            iterBegin = it;
            iterEnd = std::next(it);
            viewTitle->setText(tr("Class Timetable: %1")
                .arg(QString::fromStdString(dm->getClassName(selectedEntityId))));
        }
    } else {
        viewTitle->setText(tr("Class Timetable (all classes)"));
    }

    // For each class, add its cells to the grid (we'll overlay by appending text)
    // This is a simplified view - shows first class's schedule or overlays
    // For multi-class view, we show the first class
    bool populated = false;
    for (auto it = iterBegin; it != iterEnd; ++it) {
        int classId = it->first;
        const auto &grid = it->second;
        for (int d = 0; d < numDays && d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < numPeriods && p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (!cell.isEmpty()) {
                    QString teacherName = QString::fromStdString(dm->getTeacherName(cell.teacherId));
                    QString subjectName = QString::fromStdString(dm->getSubjectName(cell.subjectId));
                    QString roomName = QString::fromStdString(dm->getRoomName(cell.roomId));
                    QString cellText = subjectName + "\n" + teacherName + "\n" + roomName;

                    QTableWidgetItem *item = new QTableWidgetItem(cellText);
                    item->setToolTip(tr("Subject: %1\nTeacher: %2\nRoom: %3\nClass: %4")
                        .arg(subjectName).arg(teacherName).arg(roomName)
                        .arg(QString::fromStdString(dm->getClassName(classId))));
                    item->setData(Qt::UserRole, cell.subjectId);
                    item->setData(Qt::UserRole + 1, cell.teacherId);
                    item->setBackground(colorForSubject(cell.subjectId));

                    // Set text color based on background brightness
                    QColor bg = colorForSubject(cell.subjectId);
                    item->setForeground(bg.lightness() > 128 ? QColor("#1a1a1a") : QColor("#ffffff"));

                    gridWidget->setItem(d, p + 1, item);
                    populated = true;
                }
            }
        }
        break; // Show only first class in "all" view
    }

    if (!populated && selectedEntityId > 0) {
        viewTitle->setText(tr("No schedule found for selected class."));
    }
}

void TimetableViewWidget::populateTeacherGrid() {
    int numDays = static_cast<int>(dm->days.size());
    int numPeriods = static_cast<int>(dm->periods.size());

    gridWidget->setRowCount(numDays);
    gridWidget->setColumnCount(numPeriods + 1);

    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    gridWidget->setHorizontalHeaderLabels(headers);

    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    gridWidget->setVerticalHeaderLabels(dayLabels);

    if (selectedEntityId > 0) {
        viewTitle->setText(tr("Teacher Timetable: %1")
            .arg(QString::fromStdString(dm->getTeacherName(selectedEntityId))));
    } else {
        viewTitle->setText(tr("Teacher Timetable (select a teacher from the sidebar)"));
        return;
    }

    // Scan all class schedules for this teacher
    for (const auto &pair : timetable.schedules) {
        int classId = pair.first;
        const auto &grid = pair.second;
        for (int d = 0; d < numDays && d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < numPeriods && p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.teacherId == selectedEntityId && !cell.isEmpty()) {
                    QString subjectName = QString::fromStdString(dm->getSubjectName(cell.subjectId));
                    QString className = QString::fromStdString(dm->getClassName(classId));
                    QString roomName = QString::fromStdString(dm->getRoomName(cell.roomId));
                    QString cellText = subjectName + "\n" + className + "\n" + roomName;

                    QTableWidgetItem *item = new QTableWidgetItem(cellText);
                    item->setToolTip(tr("Subject: %1\nClass: %2\nRoom: %3")
                        .arg(subjectName).arg(className).arg(roomName));
                    item->setData(Qt::UserRole, cell.subjectId);
                    item->setData(Qt::UserRole + 1, cell.teacherId);
                    item->setBackground(colorForSubject(cell.subjectId));

                    QColor bg = colorForSubject(cell.subjectId);
                    item->setForeground(bg.lightness() > 128 ? QColor("#1a1a1a") : QColor("#ffffff"));

                    gridWidget->setItem(d, p + 1, item);
                }
            }
        }
    }
}

void TimetableViewWidget::populateRoomGrid() {
    int numDays = static_cast<int>(dm->days.size());
    int numPeriods = static_cast<int>(dm->periods.size());

    gridWidget->setRowCount(numDays);
    gridWidget->setColumnCount(numPeriods + 1);

    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    gridWidget->setHorizontalHeaderLabels(headers);

    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    gridWidget->setVerticalHeaderLabels(dayLabels);

    if (selectedEntityId > 0) {
        viewTitle->setText(tr("Room Timetable: %1")
            .arg(QString::fromStdString(dm->getRoomName(selectedEntityId))));
    } else {
        viewTitle->setText(tr("Room Timetable (select a room from the sidebar)"));
        return;
    }

    // Scan all class schedules for this room
    for (const auto &pair : timetable.schedules) {
        int classId = pair.first;
        const auto &grid = pair.second;
        for (int d = 0; d < numDays && d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < numPeriods && p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.roomId == selectedEntityId && !cell.isEmpty()) {
                    QString subjectName = QString::fromStdString(dm->getSubjectName(cell.subjectId));
                    QString teacherName = QString::fromStdString(dm->getTeacherName(cell.teacherId));
                    QString className = QString::fromStdString(dm->getClassName(classId));
                    QString cellText = subjectName + "\n" + teacherName + "\n" + className;

                    QTableWidgetItem *item = new QTableWidgetItem(cellText);
                    item->setToolTip(tr("Subject: %1\nTeacher: %2\nClass: %3")
                        .arg(subjectName).arg(teacherName).arg(className));
                    item->setData(Qt::UserRole, cell.subjectId);
                    item->setData(Qt::UserRole + 1, cell.teacherId);
                    item->setBackground(colorForSubject(cell.subjectId));

                    QColor bg = colorForSubject(cell.subjectId);
                    item->setForeground(bg.lightness() > 128 ? QColor("#1a1a1a") : QColor("#ffffff"));

                    gridWidget->setItem(d, p + 1, item);
                }
            }
        }
    }
}

void TimetableViewWidget::onCellClicked(int row, int col) {
    QTableWidgetItem *item = gridWidget->item(row, col);
    if (!item) return;
    QString tip = item->toolTip();
    if (!tip.isEmpty()) {
        QToolTip::showText(QCursor::pos(), tip, this);
    }
}

QColor TimetableViewWidget::colorForTeacher(int teacherId) const {
    if (teacherId <= 0) return QColor("#2a2a2a");
    int hue = (teacherId * 61) % 360;
    return QColor::fromHsl(hue, 160, 80);
}

QColor TimetableViewWidget::colorForSubject(int subjectId) const {
    if (subjectId <= 0) return QColor("#2a2a2a");
    int hue = (subjectId * 47 + 30) % 360;
    return QColor::fromHsl(hue, 140, 75);
}
