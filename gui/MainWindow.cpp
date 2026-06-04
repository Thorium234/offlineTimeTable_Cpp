#include "MainWindow.h"
#include "dashboard/DashboardWidget.h"
#include "teachers/TeacherWidget.h"
#include "subjects/SubjectWidget.h"
#include "classes/ClassWidget.h"
#include "rooms/RoomWidget.h"
#include "lessons/LessonWidget.h"
#include "timeslot/TimeSlotWidget.h"
#include "constraints/ConstraintWidget.h"
#include "roomtypes/RoomTypeWidget.h"
#include "timetableview/TimetableViewWidget.h"
#include "export/ExportWidget.h"
#include "events/FixedEventWidget.h"
#include "preferences/PreferenceWidget.h"
#include "benchmark/BenchmarkWidget.h"
#include "../utils/PathUtil.h"
#include <QTabWidget>
#include <QIcon>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    loadStyleSheet();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    tabWidget = new QTabWidget(this);

    // Core management tabs
    tabWidget->addTab(new DashboardWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/dashboard.svg")), tr("Dashboard"));
    tabWidget->addTab(new TeacherWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/teacher.svg")), tr("Teachers"));
    tabWidget->addTab(new SubjectWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/subject.svg")), tr("Subjects"));
    tabWidget->addTab(new ClassWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/class.svg")), tr("Classes"));
    tabWidget->addTab(new RoomWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/room.svg")), tr("Rooms"));
    tabWidget->addTab(new LessonWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/lesson.svg")), tr("Lessons"));
    
    // Time and constraints
    tabWidget->addTab(new TimeSlotWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/timeslot.svg")), tr("Days & Periods"));
    tabWidget->addTab(new ConstraintWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/constraint.svg")), tr("Constraints"));
    tabWidget->addTab(new RoomTypeWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/roomtype.svg")), tr("Room Types"));
    tabWidget->addTab(new FixedEventWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/event.svg")), tr("Fixed Events"));
    tabWidget->addTab(new PreferenceWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/preference.svg")), tr("Preferences"));
    
    // Timetable operations
    tabWidget->addTab(new TimetableViewWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/timetable.svg")), tr("View Timetable"));
    tabWidget->addTab(new ExportWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/export.svg")), tr("Export"));
    tabWidget->addTab(new BenchmarkWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/benchmark.svg")), tr("Benchmark"));

    setCentralWidget(tabWidget);
    setWindowTitle(tr("Timetable Generator - Management"));
    resize(1000, 700);
}

void MainWindow::loadStyleSheet() {
    QFile file(PathUtil::resolvePath("gui/style.qss"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}
