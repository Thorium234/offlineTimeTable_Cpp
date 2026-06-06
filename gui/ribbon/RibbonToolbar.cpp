#include "RibbonToolbar.h"
#include <QHBoxLayout>
#include <QFrame>

RibbonToolbar::RibbonToolbar(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
}

void RibbonToolbar::setupUi() {
    setFixedHeight(52);
    setObjectName("RibbonToolbar");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto addSeparator = [&]() {
        QFrame *sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("background: #333; max-width: 1px;");
        layout->addWidget(sep);
    };

    // Tab navigation: Home | Timetable
    homeBtn = createButton(tr("🏠 Home"), tr("Go to dashboard home"));
    homeBtn->setObjectName("TabBtn");
    layout->addWidget(homeBtn);

    timetableTabBtn = createButton(tr("📅 Timetable"), tr("Go to timetable view"));
    timetableTabBtn->setObjectName("TabBtn");
    layout->addWidget(timetableTabBtn);

    addSeparator();

    // Generate button (prominent orange)
    generateBtn = new QPushButton(tr("  ▶  Generate"), this);
    generateBtn->setObjectName("GenerateBtn");
    generateBtn->setToolTip(tr("Generate the school timetable"));
    generateBtn->setFixedHeight(38);
    layout->addWidget(generateBtn);

    addSeparator();

    // Export group
    exportCsvBtn = createButton(tr("CSV"), tr("Export timetable to CSV"));
    layout->addWidget(exportCsvBtn);
    exportHtmlBtn = createButton(tr("HTML"), tr("Export timetable to HTML"));
    layout->addWidget(exportHtmlBtn);
    exportPdfBtn = createButton(tr("PDF"), tr("Export timetable to PDF"));
    layout->addWidget(exportPdfBtn);
    printBtn = createButton(tr("🖨 Print"), tr("Print timetable preview"));
    layout->addWidget(printBtn);

    addSeparator();

    // View mode toggle
    QLabel *viewLabel = new QLabel(tr("View:"), this);
    viewLabel->setStyleSheet("color: #a0a0a0; font-weight: bold; font-size: 9pt; border: none; background: transparent;");
    layout->addWidget(viewLabel);

    viewModeGroup = new QButtonGroup(this);
    QPushButton *classView = new QPushButton(tr("Class"), this);
    QPushButton *teacherView = new QPushButton(tr("Teacher"), this);
    QPushButton *roomView = new QPushButton(tr("Room"), this);
    classView->setCheckable(true);
    teacherView->setCheckable(true);
    roomView->setCheckable(true);
    classView->setChecked(true);
    for (auto *btn : {classView, teacherView, roomView}) {
        btn->setFixedHeight(30);
        btn->setStyleSheet(
            "QPushButton { background: #2a2a2a; border: 1px solid #444; "
            "border-radius: 3px; padding: 4px 12px; color: #ccc; font-size: 9pt; }"
            "QPushButton:checked { background: #ff6600; border-color: #ff7711; color: #fff; font-weight: bold; }"
            "QPushButton:hover:!checked { background: #333; }"
        );
    }
    viewModeGroup->addButton(classView, 0);
    viewModeGroup->addButton(teacherView, 1);
    viewModeGroup->addButton(roomView, 2);
    layout->addWidget(classView);
    layout->addWidget(teacherView);
    layout->addWidget(roomView);

    addSeparator();

    // Undo / Redo
    undoBtn = createButton(tr("↩ Undo"), tr("Undo last manual change"));
    undoBtn->setEnabled(false);
    layout->addWidget(undoBtn);

    redoBtn = createButton(tr("↪ Redo"), tr("Redo last undone change"));
    redoBtn->setEnabled(false);
    layout->addWidget(redoBtn);

    addSeparator();

    // Lock toggle
    lockBtn = createButton(tr("🔒 Lock"), tr("Toggle lock on selected slot"));
    layout->addWidget(lockBtn);

    // Constraint/block mode toggle
    constraintsBtn = new QPushButton(tr("🚫 Block"), this);
    constraintsBtn->setToolTip(tr("Toggle constraint blocking mode (click cells to block teacher)"));
    constraintsBtn->setFixedHeight(30);
    constraintsBtn->setCheckable(true);
    constraintsBtn->setStyleSheet(
        "QPushButton { background: #2a2a2a; border: 1px solid #444; "
        "border-radius: 3px; padding: 4px 14px; color: #ddd; font-size: 9pt; }"
        "QPushButton:checked { background: #c0392b; border-color: #e74c3c; color: #fff; }"
        "QPushButton:hover:!checked { background: #333; }"
    );
    layout->addWidget(constraintsBtn);

    addSeparator();

    // Benchmark
    benchmarkBtn = createButton(tr("Benchmark"), tr("Run performance benchmark"));
    layout->addWidget(benchmarkBtn);

    addSeparator();

    // Substitutions & Divisions & Violations
    substitutionsBtn = createButton(tr("Substitutions"), tr("Manage teacher substitutions"));
    layout->addWidget(substitutionsBtn);
    divisionsBtn = createButton(tr("Divisions"), tr("Manage class divisions/groups"));
    layout->addWidget(divisionsBtn);
    violationsBtn = createButton(tr("Violations"), tr("Show constraint violations and suggestions"));
    layout->addWidget(violationsBtn);

    layout->addStretch();

    // Score + Status on the right
    scoreLabel = new QLabel(tr("Score: ---"), this);
    scoreLabel->setStyleSheet("color: #ff6600; font-weight: bold; font-size: 10pt; border: none; background: transparent; padding: 0 12px;");
    layout->addWidget(scoreLabel);

    statusLabel = new QLabel(tr("Ready"), this);
    statusLabel->setStyleSheet("color: #888; font-size: 9pt; border: none; background: transparent; padding: 0 8px;");
    layout->addWidget(statusLabel);

    // Connections
    connect(homeBtn, &QPushButton::clicked, this, &RibbonToolbar::homeClicked);
    connect(timetableTabBtn, &QPushButton::clicked, this, &RibbonToolbar::timetableClicked);
    connect(generateBtn, &QPushButton::clicked, this, &RibbonToolbar::generateClicked);
    connect(exportCsvBtn, &QPushButton::clicked, this, &RibbonToolbar::exportCsvClicked);
    connect(exportHtmlBtn, &QPushButton::clicked, this, &RibbonToolbar::exportHtmlClicked);
    connect(exportPdfBtn, &QPushButton::clicked, this, &RibbonToolbar::exportPdfClicked);
    connect(printBtn, &QPushButton::clicked, this, &RibbonToolbar::printPreviewClicked);
    connect(undoBtn, &QPushButton::clicked, this, &RibbonToolbar::undoClicked);
    connect(redoBtn, &QPushButton::clicked, this, &RibbonToolbar::redoClicked);
    connect(lockBtn, &QPushButton::clicked, this, &RibbonToolbar::lockClicked);
    connect(constraintsBtn, &QPushButton::toggled, this, &RibbonToolbar::constraintsModeToggled);
    connect(benchmarkBtn, &QPushButton::clicked, this, &RibbonToolbar::benchmarkClicked);
    connect(substitutionsBtn, &QPushButton::clicked, this, &RibbonToolbar::substitutionsClicked);
    connect(divisionsBtn, &QPushButton::clicked, this, &RibbonToolbar::divisionsClicked);
    connect(violationsBtn, &QPushButton::clicked, this, &RibbonToolbar::violationsClicked);
    connect(viewModeGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
        emit viewModeChanged(static_cast<ViewMode>(id));
    });
}

QPushButton *RibbonToolbar::createButton(const QString &text, const QString &tooltip) {
    QPushButton *btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setFixedHeight(30);
    btn->setStyleSheet(
        "QPushButton { background: #2a2a2a; border: 1px solid #444; "
        "border-radius: 3px; padding: 4px 14px; color: #ddd; font-size: 9pt; }"
        "QPushButton:hover { background: #ff6600; border-color: #ff7711; color: #fff; }"
    );
    return btn;
}

void RibbonToolbar::setScore(int score) {
    scoreLabel->setText(tr("Score: %1/1000").arg(score));
}

void RibbonToolbar::setStatusText(const QString &text) {
    statusLabel->setText(text);
}

void RibbonToolbar::setGenerationRunning(bool running) {
    generateBtn->setEnabled(!running);
    generateBtn->setText(running ? tr("  ⏳ Generating...") : tr("  ▶  Generate"));
}

void RibbonToolbar::setUndoEnabled(bool enabled) {
    undoBtn->setEnabled(enabled);
}

void RibbonToolbar::setRedoEnabled(bool enabled) {
    redoBtn->setEnabled(enabled);
}
