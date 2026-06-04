#include "TimetableViewWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>

TimetableViewWidget::TimetableViewWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Timetable Viewer"));
    resize(900, 600);
    setupUi();
}

void TimetableViewWidget::setTimetable(const Timetable &t) {
    timetable = t;
    refresh();
}

void TimetableViewWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // Class Timetable tab
    QWidget *classTab = new QWidget(this);
    QVBoxLayout *classLayout = new QVBoxLayout(classTab);
    
    QHBoxLayout *classSelectLayout = new QHBoxLayout;
    classSelectLayout->addWidget(new QLabel(tr("Select Class:"), classTab));
    classCombo = new QComboBox(classTab);
    classSelectLayout->addWidget(classCombo);
    classLayout->addLayout(classSelectLayout);
    
    classTableWidget = new QTableWidget(classTab);
    classLayout->addWidget(classTableWidget);
    
    // Teacher Timetable tab
    QWidget *teacherTab = new QWidget(this);
    QVBoxLayout *teacherLayout = new QVBoxLayout(teacherTab);
    
    QHBoxLayout *teacherSelectLayout = new QHBoxLayout;
    teacherSelectLayout->addWidget(new QLabel(tr("Select Teacher:"), teacherTab));
    teacherCombo = new QComboBox(teacherTab);
    teacherSelectLayout->addWidget(teacherCombo);
    teacherLayout->addLayout(teacherSelectLayout);
    
    teacherTableWidget = new QTableWidget(teacherTab);
    teacherLayout->addWidget(teacherTableWidget);
    
    // Room Timetable tab
    QWidget *roomTab = new QWidget(this);
    QVBoxLayout *roomLayout = new QVBoxLayout(roomTab);
    
    QHBoxLayout *roomSelectLayout = new QHBoxLayout;
    roomSelectLayout->addWidget(new QLabel(tr("Select Room:"), roomTab));
    roomCombo = new QComboBox(roomTab);
    roomSelectLayout->addWidget(roomCombo);
    roomLayout->addLayout(roomSelectLayout);
    
    roomTableWidget = new QTableWidget(roomTab);
    roomLayout->addWidget(roomTableWidget);
    
    tabWidget->addTab(classTab, tr("Class Timetable"));
    tabWidget->addTab(teacherTab, tr("Teacher Timetable"));
    tabWidget->addTab(roomTab, tr("Room Timetable"));
    
    mainLayout->addWidget(tabWidget);
    
    setupClassTimetableTab();
    setupTeacherTimetableTab();
    setupRoomTimetableTab();
    
    // Connect signals
    connect(classCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &TimetableViewWidget::populateClassTimetable);
    connect(teacherCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &TimetableViewWidget::populateTeacherTimetable);
    connect(roomCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &TimetableViewWidget::populateRoomTimetable);
}

void TimetableViewWidget::setupClassTimetableTab() {
    classCombo->clear();
    for (const auto &c : dm->classes) {
        classCombo->addItem(QString::fromStdString(c.name), c.id);
    }
}

void TimetableViewWidget::setupTeacherTimetableTab() {
    teacherCombo->clear();
    for (const auto &t : dm->teachers) {
        teacherCombo->addItem(QString::fromStdString(t.name), t.id);
    }
}

void TimetableViewWidget::setupRoomTimetableTab() {
    roomCombo->clear();
    for (const auto &r : dm->rooms) {
        roomCombo->addItem(QString::fromStdString(r.name), r.id);
    }
}

void TimetableViewWidget::populateClassTimetable(const QString &className) {
    classTableWidget->clear();
    
    if (dm->days.empty() || dm->periods.empty()) {
        classTableWidget->setRowCount(0);
        classTableWidget->setColumnCount(0);
        return;
    }
    
    int classId = -1;
    for (const auto &c : dm->classes) {
        if (QString::fromStdString(c.name) == className) {
            classId = c.id;
            break;
        }
    }
    
    if (classId == -1) return;
    
    // Setup table dimensions
    classTableWidget->setRowCount(dm->days.size());
    classTableWidget->setColumnCount(dm->periods.size() + 1);
    
    // Headers
    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    classTableWidget->setHorizontalHeaderLabels(headers);
    
    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    classTableWidget->setVerticalHeaderLabels(dayLabels);
    
    // Find schedule for this class
    auto it = timetable.schedules.find(classId);
    if (it != timetable.schedules.end()) {
        const auto &grid = it->second;
        for (int d = 0; d < static_cast<int>(dm->days.size()) && d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < static_cast<int>(dm->periods.size()) && p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.subjectId > 0) {
                    QString cellText = QString::fromStdString(dm->getSubjectName(cell.subjectId)) + "\n" +
                                       QString::fromStdString(dm->getTeacherName(cell.teacherId)) + "\n" +
                                       QString::fromStdString(dm->getRoomName(cell.roomId));
                    classTableWidget->setItem(d, p + 1, new QTableWidgetItem(cellText));
                }
            }
        }
    }
    
    classTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    classTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TimetableViewWidget::populateTeacherTimetable(const QString &teacherName) {
    teacherTableWidget->clear();
    
    if (dm->days.empty() || dm->periods.empty()) {
        teacherTableWidget->setRowCount(0);
        teacherTableWidget->setColumnCount(0);
        return;
    }
    
    int teacherId = -1;
    for (const auto &t : dm->teachers) {
        if (QString::fromStdString(t.name) == teacherName) {
            teacherId = t.id;
            break;
        }
    }
    
    if (teacherId == -1) return;
    
    teacherTableWidget->setRowCount(dm->days.size());
    teacherTableWidget->setColumnCount(dm->periods.size() + 1);
    
    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    teacherTableWidget->setHorizontalHeaderLabels(headers);
    
    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    teacherTableWidget->setVerticalHeaderLabels(dayLabels);
    
    // Scan all class schedules for this teacher
    for (const auto &pair : timetable.schedules) {
        const auto &grid = pair.second;
        for (int d = 0; d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.teacherId == teacherId && cell.subjectId > 0) {
                    QString cellText = QString::fromStdString(dm->getClassName(pair.first)) + "\n" +
                                       QString::fromStdString(dm->getSubjectName(cell.subjectId)) + "\n" +
                                       QString::fromStdString(dm->getRoomName(cell.roomId));
                    teacherTableWidget->setItem(d, p + 1, new QTableWidgetItem(cellText));
                }
            }
        }
    }
    
    teacherTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    teacherTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TimetableViewWidget::populateRoomTimetable(const QString &roomName) {
    roomTableWidget->clear();
    
    if (dm->days.empty() || dm->periods.empty()) {
        roomTableWidget->setRowCount(0);
        roomTableWidget->setColumnCount(0);
        return;
    }
    
    int roomId = -1;
    for (const auto &r : dm->rooms) {
        if (QString::fromStdString(r.name) == roomName) {
            roomId = r.id;
            break;
        }
    }
    
    if (roomId == -1) return;
    
    roomTableWidget->setRowCount(dm->days.size());
    roomTableWidget->setColumnCount(dm->periods.size() + 1);
    
    QStringList headers;
    headers << "";
    for (const auto &p : dm->periods) {
        headers << QString::fromStdString(p.startTime);
    }
    roomTableWidget->setHorizontalHeaderLabels(headers);
    
    QStringList dayLabels;
    for (const auto &d : dm->days) {
        dayLabels << QString::fromStdString(d.name);
    }
    roomTableWidget->setVerticalHeaderLabels(dayLabels);
    
    // Scan all class schedules for this room
    for (const auto &pair : timetable.schedules) {
        const auto &grid = pair.second;
        for (int d = 0; d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.roomId == roomId && cell.subjectId > 0) {
                    QString cellText = QString::fromStdString(dm->getClassName(pair.first)) + "\n" +
                                       QString::fromStdString(dm->getSubjectName(cell.subjectId)) + "\n" +
                                       QString::fromStdString(dm->getTeacherName(cell.teacherId));
                    roomTableWidget->setItem(d, p + 1, new QTableWidgetItem(cellText));
                }
            }
        }
    }
    
    roomTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    roomTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TimetableViewWidget::refresh() {
    setupClassTimetableTab();
    setupTeacherTimetableTab();
    setupRoomTimetableTab();
}

void TimetableViewWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
