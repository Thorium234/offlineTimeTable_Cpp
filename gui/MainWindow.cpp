#include "MainWindow.h"
#include "dashboard/DashboardWidget.h"
#include "teachers/TeacherWidget.h"
#include "subjects/SubjectWidget.h"
#include "classes/ClassWidget.h"
#include "rooms/RoomWidget.h"
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

    tabWidget->addTab(new DashboardWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/dashboard.svg")), tr("Dashboard"));
    tabWidget->addTab(new TeacherWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/teacher.svg")), tr("Teachers"));
    tabWidget->addTab(new SubjectWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/subject.svg")), tr("Subjects"));
    tabWidget->addTab(new ClassWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/class.svg")), tr("Classes"));
    tabWidget->addTab(new RoomWidget(&dm, this), QIcon(PathUtil::resolvePath("gui/icons/room.svg")), tr("Rooms"));

    setCentralWidget(tabWidget);
    setWindowTitle(tr("Timetable Generator - Management"));
    resize(950, 650);
}

void MainWindow::loadStyleSheet() {
    QFile file(PathUtil::resolvePath("gui/style.qss"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}
