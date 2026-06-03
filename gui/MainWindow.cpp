#include "MainWindow.h"
#include "teachers/TeacherWidget.h"
#include "subjects/SubjectWidget.h"
#include "classes/ClassWidget.h"
#include "rooms/RoomWidget.h"
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

    tabWidget->addTab(new TeacherWidget(this), QIcon("gui/icons/teacher.svg"), tr("Teachers"));
    tabWidget->addTab(new SubjectWidget(this), QIcon("gui/icons/subject.svg"), tr("Subjects"));
    tabWidget->addTab(new ClassWidget(this), QIcon("gui/icons/class.svg"), tr("Classes"));
    tabWidget->addTab(new RoomWidget(this), QIcon("gui/icons/room.svg"), tr("Rooms"));

    setCentralWidget(tabWidget);
    setWindowTitle(tr("Timetable Generator - Management"));
    resize(950, 650);
}

void MainWindow::loadStyleSheet() {
    QFile file("gui/style.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}
