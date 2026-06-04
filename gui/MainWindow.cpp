#include "MainWindow.h"
#include "../services/TimetableEngine.h"
#include "../services/ExportService.h"
#include "../services/Benchmark.h"
#include "../utils/PathUtil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    loadStyleSheet();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    // Central widget
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Ribbon toolbar at the top
    ribbon = new RibbonToolbar(&dm, this);
    mainLayout->addWidget(ribbon);

    // 2. Splitter: sidebar + timetable grid
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    sidebar = new DataSidebar(&dm, this);
    timetableView = new TimetableViewWidget(&dm, this);

    splitter->addWidget(sidebar);
    splitter->addWidget(timetableView);
    splitter->setStretchFactor(0, 0);  // sidebar doesn't stretch
    splitter->setStretchFactor(1, 1);  // grid stretches
    splitter->setSizes({220, 780});

    mainLayout->addWidget(splitter, 1);

    // 3. Status bar
    statusInfoLabel = new QLabel(tr("Ready. Add resources and lessons, then click Generate."), this);
    statusInfoLabel->setStyleSheet("color: #888; padding: 2px 8px;");
    statusBar()->addWidget(statusInfoLabel, 1);
    statusBar()->setStyleSheet(
        "QStatusBar { background: #141414; border-top: 1px solid #333; }"
        "QStatusBar::item { border: none; }"
    );

    setWindowTitle(tr("Timetable Generator"));
    resize(1200, 800);

    // Signal connections
    connect(ribbon, &RibbonToolbar::generateClicked, this, &MainWindow::onGenerateTimetable);
    connect(ribbon, &RibbonToolbar::exportCsvClicked, this, &MainWindow::onExportCSV);
    connect(ribbon, &RibbonToolbar::exportHtmlClicked, this, &MainWindow::onExportHTML);
    connect(ribbon, &RibbonToolbar::benchmarkClicked, this, &MainWindow::onBenchmark);
    connect(ribbon, &RibbonToolbar::viewModeChanged, this, &MainWindow::onViewModeChanged);
    connect(sidebar, &DataSidebar::itemSelected, this, &MainWindow::onSidebarItemSelected);
}

void MainWindow::loadStyleSheet() {
    QFile file(PathUtil::resolvePath("gui/style.qss"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}

void MainWindow::onGenerateTimetable() {
    if (dm.lessons.empty()) {
        QMessageBox::warning(this, tr("Cannot Generate"),
            tr("No lessons have been assigned yet. Add lessons first."));
        return;
    }

    ribbon->setGenerationRunning(true);
    ribbon->setStatusText(tr("Generating..."));
    statusInfoLabel->setText(tr("Running scheduling engine..."));
    QApplication::processEvents();

    dm.placementRejectLog.clear();

    TimetableEngine engine;
    Timetable timetable = engine.generate(dm);

    dm.lastTimetable = timetable;
    dm.timetableGenerated = true;

    timetableView->setTimetable(timetable);
    ribbon->setScore(timetable.score);
    ribbon->setGenerationRunning(false);

    int unscheduled = static_cast<int>(timetable.unscheduledLessons.size());
    if (unscheduled == 0) {
        ribbon->setStatusText(tr("Ready"));
        statusInfoLabel->setText(tr("Timetable generated successfully! Score: %1/1000, Nodes: %2")
            .arg(timetable.score).arg(engine.getLastRunStats().nodesVisited));
    } else {
        ribbon->setStatusText(tr("Warnings"));
        statusInfoLabel->setText(tr("Generated with %1 unscheduled lessons. Score: %2/1000")
            .arg(unscheduled).arg(timetable.score));
    }
}

void MainWindow::onExportCSV() {
    if (!dm.timetableGenerated) {
        QMessageBox::information(this, tr("No Timetable"), tr("Generate a timetable first."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Export CSV"), "timetable.csv", tr("CSV Files (*.csv)"));
    if (path.isEmpty()) return;
    if (ExportService::exportToCSV(path.toStdString(), dm.lastTimetable, dm)) {
        statusInfoLabel->setText(tr("Exported to CSV: %1").arg(path));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not export CSV."));
    }
}

void MainWindow::onExportHTML() {
    if (!dm.timetableGenerated) {
        QMessageBox::information(this, tr("No Timetable"), tr("Generate a timetable first."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Export HTML"), "timetable.html", tr("HTML Files (*.html)"));
    if (path.isEmpty()) return;
    if (ExportService::exportToHTML(path.toStdString(), dm.lastTimetable, dm)) {
        statusInfoLabel->setText(tr("Exported to HTML: %1").arg(path));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not export HTML."));
    }
}

void MainWindow::onBenchmark() {
    if (dm.lessons.empty()) {
        QMessageBox::warning(this, tr("Cannot Benchmark"),
            tr("Add at least one lesson before running the benchmark."));
        return;
    }
    ribbon->setGenerationRunning(true);
    ribbon->setStatusText(tr("Benchmarking..."));
    QApplication::processEvents();

    Benchmark::runSuite();

    ribbon->setGenerationRunning(false);
    ribbon->setStatusText(tr("Ready"));
    statusInfoLabel->setText(tr("Benchmark complete. Check console output for results."));
}

void MainWindow::onViewModeChanged(ViewMode mode) {
    timetableView->setViewMode(mode);
}

void MainWindow::onSidebarItemSelected(const QString &type, int id) {
    if (type == "teacher") {
        timetableView->setViewMode(ViewMode::TEACHER);
        timetableView->setViewSelection("teacher", id);
        statusInfoLabel->setText(tr("Viewing teacher timetable: %1")
            .arg(QString::fromStdString(dm.getTeacherName(id))));
    } else if (type == "class") {
        timetableView->setViewMode(ViewMode::CLASS);
        timetableView->setViewSelection("class", id);
        statusInfoLabel->setText(tr("Viewing class timetable: %1")
            .arg(QString::fromStdString(dm.getClassName(id))));
    } else if (type == "room") {
        timetableView->setViewMode(ViewMode::ROOM);
        timetableView->setViewSelection("room", id);
        statusInfoLabel->setText(tr("Viewing room timetable: %1")
            .arg(QString::fromStdString(dm.getRoomName(id))));
    }
}
