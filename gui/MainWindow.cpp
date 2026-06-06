#include "MainWindow.h"
#include "../services/TimetableEngine.h"
#include "../services/ExportService.h"
#include "../services/PdfReportService.h"
#include "../services/Benchmark.h"
#include "../utils/PathUtil.h"
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QApplication>
#include "constraints/ConstraintRelaxationDialog.h"

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

    // 2. Splitter: sidebar + stacked content area
    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    sidebar = new DataSidebar(&dm, this);

    // Stacked widget: Page 0 = Dashboard, Page 1 = TimetableView
    stack = new QStackedWidget(this);
    dashboard = new DashboardWidget(&dm, this);
    timetableView = new TimetableViewWidget(&dm, this);
    substitutionWidget = new SubstitutionWidget(&dm, this);
    divisionWidget = new DivisionWidget(&dm, this);
    stack->addWidget(dashboard);
    stack->addWidget(timetableView);
    stack->addWidget(substitutionWidget);
    stack->addWidget(divisionWidget);
    stack->setCurrentIndex(0);

    splitter->addWidget(sidebar);
    splitter->addWidget(stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    QList<int> sizes; sizes.append(220); sizes.append(780);
    splitter->setSizes(sizes);

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
    connect(ribbon, &RibbonToolbar::homeClicked, this, &MainWindow::onHomeClicked);
    connect(ribbon, &RibbonToolbar::timetableClicked, this, &MainWindow::onTimetableClicked);
    connect(ribbon, &RibbonToolbar::generateClicked, this, &MainWindow::onGenerateTimetable);
    connect(ribbon, &RibbonToolbar::exportCsvClicked, this, &MainWindow::onExportCSV);
    connect(ribbon, &RibbonToolbar::exportHtmlClicked, this, &MainWindow::onExportHTML);
    connect(ribbon, &RibbonToolbar::exportPdfClicked, this, &MainWindow::onExportPDF);
    connect(ribbon, &RibbonToolbar::printPreviewClicked, this, &MainWindow::onPrintPreview);
    connect(ribbon, &RibbonToolbar::benchmarkClicked, this, &MainWindow::onBenchmark);
    connect(ribbon, &RibbonToolbar::undoClicked, this, &MainWindow::onUndoClicked);
    connect(ribbon, &RibbonToolbar::redoClicked, this, &MainWindow::onRedoClicked);
    connect(ribbon, &RibbonToolbar::lockClicked, this, &MainWindow::onLockClicked);
    connect(ribbon, &RibbonToolbar::constraintsModeToggled, this, &MainWindow::onConstraintsModeToggled);
    connect(ribbon, &RibbonToolbar::substitutionsClicked, this, &MainWindow::onSubstitutionsClicked);
    connect(ribbon, &RibbonToolbar::divisionsClicked, this, &MainWindow::onDivisionsClicked);
    connect(ribbon, &RibbonToolbar::violationsClicked, this, &MainWindow::onViolationsClicked);
    connect(ribbon, &RibbonToolbar::viewModeChanged, this, &MainWindow::onViewModeChanged);
    connect(sidebar, &DataSidebar::itemSelected, this, &MainWindow::onSidebarItemSelected);

    // Dashboard navigation
    connect(dashboard, &DashboardWidget::switchToTimetableView, this, &MainWindow::onTimetableClicked);
    connect(dashboard, &DashboardWidget::generateClicked, this, &MainWindow::onGenerateTimetable);

    // Track undo/redo state from timetable view changes
    connect(timetableView, &TimetableViewWidget::undoRedoStateChanged, this, &MainWindow::updateUndoRedoButtons);
}

void MainWindow::loadStyleSheet() {
    QFile file(PathUtil::resolvePath("gui/style.qss"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}

void MainWindow::onHomeClicked() {
    stack->setCurrentIndex(0);
    dashboard->refreshStats();
    statusInfoLabel->setText(tr("Dashboard"));
}

void MainWindow::onTimetableClicked() {
    stack->setCurrentIndex(1);
    if (dm.timetableGenerated)
        timetableView->setTimetable(dm.lastTimetable);
    statusInfoLabel->setText(tr("Timetable view"));
}

void MainWindow::onSubstitutionsClicked() {
    stack->setCurrentIndex(2);
    substitutionWidget->refresh();
    statusInfoLabel->setText(tr("Substitutions manager"));
}

void MainWindow::onDivisionsClicked() {
    stack->setCurrentIndex(3);
    divisionWidget->refresh();
    statusInfoLabel->setText(tr("Divisions / Groups manager"));
}

void MainWindow::onViolationsClicked() {
    ConstraintRelaxationDialog dlg(&dm, this);
    dlg.exec();
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

    // Clear undo history on fresh generation
    dm.clearUndoHistory();
    updateUndoRedoButtons();

    // Refresh the dashboard if visible
    dashboard->refreshStats();
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

void MainWindow::onExportPDF() {
    if (!dm.timetableGenerated) {
        QMessageBox::information(this, tr("No Timetable"), tr("Generate a timetable first."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Export PDF"), "timetable.pdf",
                                                 tr("PDF Files (*.pdf)"));
    if (path.isEmpty()) return;
    PdfReportService pdfSvc;
    if (pdfSvc.exportToPdf(dm.lastTimetable, dm, path.toStdString())) {
        statusInfoLabel->setText(tr("Exported to PDF: %1").arg(path));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not export PDF."));
    }
}

void MainWindow::onPrintPreview() {
    if (!dm.timetableGenerated) {
        QMessageBox::information(this, tr("No Timetable"), tr("Generate a timetable first."));
        return;
    }

    // Build HTML from timetable and use QPrintPreviewDialog
    PdfReportService pdfSvc;
    QString tmpPath = QDir::tempPath() + "/timetable_preview.pdf";
    pdfSvc.exportToPdf(dm.lastTimetable, dm, tmpPath.toStdString());

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::NativeFormat);
    printer.setPageSize(QPageSize(QPageSize::A3));
    printer.setPageOrientation(QPageLayout::Landscape);

    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(tr("Print Timetable"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this, &printer](QPrinter *p) {
        // Re-render using the same HTML approach
        int numDays = static_cast<int>(dm.days.size());
        int numPeriods = static_cast<int>(dm.periods.size());

        QString html;
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Timetable</title><style>";
        html += "body{font-family:Segoe UI,sans-serif;padding:20px;color:#1e293b}";
        html += "h1{text-align:center;color:#1e3a5f;border-bottom:3px solid #f0a500;padding-bottom:8px;font-size:20pt}";
        html += ".score{text-align:center;font-size:11pt;color:#475569;margin:10px 0 20px}";
        html += ".card{border:1px solid #e2e8f0;border-radius:10px;padding:16px;margin-bottom:24px}";
        html += ".card-title{font-size:14pt;font-weight:700;color:#1e3a5f;margin-bottom:10px}";
        html += "table{width:100%;border-collapse:collapse;font-size:8pt}";
        html += "th{background:#1e3a5f;color:#fff;padding:6px 4px;border:1px solid #cbd5e1}";
        html += "td{border:1px solid #cbd5e1;padding:4px;text-align:center}";
        html += ".subj{font-weight:700;color:#0f172a;display:block}";
        html += ".det{font-size:7pt;color:#475569}";
        html += ".empty{color:#94a3b8}";
        html += "</style></head><body>";
        html += "<h1>School Timetable</h1>";
        html += "<div class='score'>Score: " + QString::number(dm.lastTimetable.score) + " / 1000</div>";

        for (const auto& pair : dm.lastTimetable.schedules) {
            int cid = pair.first;
            const auto& grid = pair.second;
            html += "<div class='card'><div class='card-title'>" + QString::fromStdString(dm.getClassName(cid)) + "</div><table><tr><th>Day</th>";
            for (int pp = 0; pp < numPeriods; ++pp) html += "<th>P" + QString::number(pp+1) + "</th>";
            html += "</tr>";
            for (int d = 0; d < numDays; ++d) {
                html += "<tr><td style='font-weight:600'>" + QString::fromStdString(dm.getDayName(dm.days[d].id)) + "</td>";
                for (int pp = 0; pp < numPeriods; ++pp) {
                    html += "<td>";
                    bool f = false;
                    if (d < (int)grid.size() && pp < (int)grid[d].size() && !grid[d][pp].isEmpty()) {
                        const auto& c = grid[d][pp];
                        html += "<span class='subj'>" + QString::fromStdString(dm.getSubjectName(c.subjectId)) + "</span>";
                        html += "<span class='det'>" + QString::fromStdString(dm.getTeacherName(c.teacherId)) + " / " + QString::fromStdString(dm.getRoomName(c.roomId)) + "</span>";
                        f = true;
                    }
                    if (!f) html += "<span class='empty'>—</span>";
                    html += "</td>";
                }
                html += "</tr>";
            }
            html += "</table></div>";
        }

        if (!dm.lastTimetable.unscheduledLessons.empty()) {
            html += "<div style='background:#fef2f2;border:1px solid #fca5a5;border-radius:10px;padding:16px'>";
            html += "<h2 style='color:#dc2626;font-size:12pt;margin:0 0 8px'>Unscheduled: " + QString::number(dm.lastTimetable.unscheduledLessons.size()) + "</h2><table>";
            html += "<tr><th>Class</th><th>Subject</th><th>Teacher</th><th>Periods</th><th>Reason</th></tr>";
            for (const auto& ul : dm.lastTimetable.unscheduledLessons) {
                html += "<tr><td>" + QString::fromStdString(dm.getClassName(ul.classId)) + "</td>"
                        "<td>" + QString::fromStdString(dm.getSubjectName(ul.subjectId)) + "</td>"
                        "<td>" + QString::fromStdString(dm.getTeacherName(ul.teacherId)) + "</td>"
                        "<td>" + QString::number(ul.periodsCount) + "</td>"
                        "<td style='color:#dc2626'>" + QString::fromStdString(ul.reason) + "</td></tr>";
            }
            html += "</table></div>";
        }

        html += "</body></html>";

        QTextDocument doc;
        doc.setHtml(html);
        doc.setPageSize(QSizeF(p->pageRect(QPrinter::DevicePixel).size()));
        doc.print(p);
    });
    preview.exec();
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

void MainWindow::onUndoClicked() {
    if (dm.performUndo()) {
        timetableView->setTimetable(dm.lastTimetable);
        statusInfoLabel->setText(tr("Undo: %1").arg(QString::fromStdString(dm.undoStack.lastActionLabel)));
    }
    updateUndoRedoButtons();
}

void MainWindow::onRedoClicked() {
    if (dm.performRedo()) {
        timetableView->setTimetable(dm.lastTimetable);
        statusInfoLabel->setText(tr("Redo: %1").arg(QString::fromStdString(dm.undoStack.lastActionLabel)));
    }
    updateUndoRedoButtons();
}

void MainWindow::onLockClicked() {
    statusInfoLabel->setText(tr("Click on a slot in the timetable to toggle its lock state."));
}

void MainWindow::onConstraintsModeToggled(bool enabled) {
    timetableView->setConstraintMode(enabled);
    if (enabled) {
        statusInfoLabel->setText(tr("Constraint blocking mode active — click cells to block/unblock teacher"));
        // Switch to teacher view if not already
        if (timetableView->currentViewMode() != ViewMode::TEACHER) {
            timetableView->setViewMode(ViewMode::TEACHER);
        }
    } else {
        statusInfoLabel->setText(tr("Constraint blocking mode disabled"));
    }
}

void MainWindow::updateUndoRedoButtons() {
    ribbon->setUndoEnabled(dm.undoStack.canUndo());
    ribbon->setRedoEnabled(dm.undoStack.canRedo());
}

void MainWindow::onViewModeChanged(ViewMode mode) {
    // Switch to timetable tab when view mode changes
    stack->setCurrentIndex(1);
    timetableView->setViewMode(mode);
}

void MainWindow::onSidebarItemSelected(const QString &type, int id) {
    // Switch to timetable tab when sidebar item selected
    stack->setCurrentIndex(1);
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
