#include "DashboardWidget.h"
#include "../../services/TimetableEngine.h"
#include "../../services/AnalyticsService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QShowEvent>
#include <QMessageBox>
#include <QProgressBar>
#include <QScrollArea>
#include <QFrame>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>

DashboardWidget::DashboardWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
    refreshStats();
}

QFrame *DashboardWidget::createMetricCard(const QString &title, QLabel *&valueLabel,
                                          const QString &iconColor, QFrame *&cardFrame) {
    cardFrame = new QFrame(this);
    cardFrame->setObjectName("MetricCard");
    cardFrame->setStyleSheet(QString(
        "#MetricCard {"
        "  background-color: rgba(35, 35, 35, 0.7);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 10px;"
        "}"
    ));

    QVBoxLayout *layout = new QVBoxLayout(cardFrame);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(4);

    QLabel *lblTitle = new QLabel(title, cardFrame);
    lblTitle->setStyleSheet(QString(
        "font-size: 9pt; text-transform: uppercase; letter-spacing: 0.5px;"
        " color: #888888; font-weight: bold; border: none; background: transparent;"
    ));

    valueLabel = new QLabel("0", cardFrame);
    valueLabel->setStyleSheet(QString(
        "font-size: 22pt; font-weight: 800; color: %1; border: none; background: transparent;"
    ).arg(iconColor));

    layout->addWidget(lblTitle);
    layout->addWidget(valueLabel);
    return cardFrame;
}

void DashboardWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(14);

    // ── Title ──
    QLabel *titleLabel = new QLabel(tr("⚡ Timetable Summary"), this);
    titleLabel->setStyleSheet(
        "font-size: 15pt; font-weight: bold; color: #f0a500;"
        " border: none; background: transparent; padding-bottom: 2px;"
    );
    mainLayout->addWidget(titleLabel);

    // ── 6 Metrics Cards (Flask-style) ──
    QHBoxLayout *metricsLayout = new QHBoxLayout;
    metricsLayout->setSpacing(10);

    metricsLayout->addWidget(createMetricCard(tr("SCHEDULED"), scheduledValue, "#5b8def", scheduledCard));
    metricsLayout->addWidget(createMetricCard(tr("REQUIRED"), requiredValue, "#8e44ad", requiredCard));
    metricsLayout->addWidget(createMetricCard(tr("COVERAGE"), coverageValue, "#4caf50", coverageCard));
    metricsLayout->addWidget(createMetricCard(tr("LOCKED"), lockedValue, "#f0a500", lockedCard));
    metricsLayout->addWidget(createMetricCard(tr("SCORE"), scoreValueLabel, "#ff6600", scoreCard));
    metricsLayout->addWidget(createMetricCard(tr("CONFLICTS"), conflictsValue, "#4caf50", conflictsCard));

    mainLayout->addLayout(metricsLayout);

    // ── Unscheduled Section (hidden by default) ──
    unscheduledSection = new QWidget(this);
    unscheduledSection->setObjectName("UnscheduledSection");
    unscheduledSection->setStyleSheet(
        "#UnscheduledSection {"
        "  background: rgba(244, 67, 54, 0.08);"
        "  border: 1px solid rgba(244, 67, 54, 0.25);"
        "  border-radius: 8px;"
        "  padding: 10px;"
        "}"
    );
    QVBoxLayout *unschedOuter = new QVBoxLayout(unscheduledSection);
    unschedOuter->setContentsMargins(12, 10, 12, 10);
    unschedOuter->setSpacing(6);

    QLabel *unschedTitle = new QLabel(tr("⚠ Unscheduled Blocks"), unscheduledSection);
    unschedTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #f44336; border: none; background: transparent;");
    unschedOuter->addWidget(unschedTitle);

    unscheduledLayout = new QVBoxLayout;
    unscheduledLayout->setSpacing(4);
    unschedOuter->addLayout(unscheduledLayout);

    unscheduledSection->setVisible(false);
    mainLayout->addWidget(unscheduledSection);

    // ── Action Buttons ──
    QHBoxLayout *actionLayout = new QHBoxLayout;
    actionLayout->setSpacing(8);

    viewTimetableBtn = new QPushButton(tr("📅 View Timetable"), this);
    viewTimetableBtn->setStyleSheet(
        "QPushButton { background: #2a2a2a; border: 1px solid #444; border-radius: 5px; padding: 7px 18px; color: #ddd; font-size: 9pt; }"
        "QPushButton:hover { background: #ff6600; border-color: #ff7711; color: #fff; }"
    );

    regenerateBtn = new QPushButton(tr("🔄 Re-Generate"), this);
    regenerateBtn->setStyleSheet(
        "QPushButton { background: #ff6600; border: 1px solid #ff7711; border-radius: 5px; padding: 7px 18px; color: #fff; font-weight: bold; font-size: 9pt; }"
        "QPushButton:hover { background: #ff8833; }"
    );

    viewConflictsBtn = new QPushButton(tr("⚠ View Conflicts"), this);
    viewConflictsBtn->setStyleSheet(
        "QPushButton { background: #2a2a2a; border: 1px solid #444; border-radius: 5px; padding: 7px 18px; color: #ddd; font-size: 9pt; }"
        "QPushButton:hover { background: #f44336; border-color: #e53935; color: #fff; }"
    );

    compareVersionsBtn = new QPushButton(tr("📋 Compare Versions"), this);
    compareVersionsBtn->setStyleSheet(
        "QPushButton { background: #2a2a2a; border: 1px solid #444; border-radius: 5px; padding: 7px 18px; color: #ddd; font-size: 9pt; }"
        "QPushButton:hover { background: #8e44ad; border-color: #9b59b6; color: #fff; }"
    );

    actionLayout->addWidget(viewTimetableBtn);
    actionLayout->addWidget(regenerateBtn);
    actionLayout->addWidget(viewConflictsBtn);
    actionLayout->addWidget(compareVersionsBtn);
    actionLayout->addStretch();
    mainLayout->addLayout(actionLayout);

    // ── Bottom split: Scheduler + Room Utilization ──
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(14);

    // Scheduler Panel
    QGroupBox *schedulerBox = new QGroupBox(tr("Scheduling Engine"), this);
    schedulerBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; margin-top: 14px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; color: #ff6600; }"
    );

    QVBoxLayout *schedLayout = new QVBoxLayout(schedulerBox);
    schedLayout->setContentsMargins(12, 12, 12, 12);
    schedLayout->setSpacing(8);

    QHBoxLayout *controlRow = new QHBoxLayout;
    statusLabel = new QLabel(tr("Status: Ready to schedule."), schedulerBox);
    statusLabel->setStyleSheet("font-size: 10pt; font-weight: bold; color: #e0e0e0; border: none; background: transparent;");

    executeBtn = new QPushButton(tr("▶ Execute Scheduler"), schedulerBox);
    executeBtn->setStyleSheet(
        "QPushButton { background: #ff6600; color: white; font-size: 10pt; font-weight: bold; border-radius: 5px; padding: 7px 20px; }"
        "QPushButton:hover { background: #ff7711; }"
        "QPushButton:pressed { background: #cc5200; }"
    );

    controlRow->addWidget(statusLabel);
    controlRow->addStretch();
    controlRow->addWidget(executeBtn);
    schedLayout->addLayout(controlRow);

    QLabel *reportLbl = new QLabel(tr("Diagnostic Report:"), schedulerBox);
    reportLbl->setStyleSheet("font-size: 8.5pt; color: #888; border: none; background: transparent;");
    schedLayout->addWidget(reportLbl);

    reportText = new QTextEdit(schedulerBox);
    reportText->setReadOnly(true);
    reportText->setPlaceholderText(tr("Solver diagnostic output appears here after execution."));
    reportText->setStyleSheet(
        "QTextEdit { background: rgba(20, 20, 20, 0.8); border: 1px solid rgba(255, 255, 255, 0.05); border-radius: 5px; color: #a0a0a0; font-family: monospace; font-size: 9pt; }"
    );
    schedLayout->addWidget(reportText, 1);

    bottomLayout->addWidget(schedulerBox, 3);

    // Room Utilization Panel
    QGroupBox *roomBox = new QGroupBox(tr("Room Utilization"), this);
    roomBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; margin-top: 14px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; color: #ff6600; }"
    );

    QVBoxLayout *roomBoxLayout = new QVBoxLayout(roomBox);
    roomBoxLayout->setContentsMargins(10, 12, 10, 10);

    QScrollArea *scrollArea = new QScrollArea(roomBox);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    QWidget *scrollWidget = new QWidget(scrollArea);
    scrollWidget->setStyleSheet("background: transparent;");
    roomListLayout = new QVBoxLayout(scrollWidget);
    roomListLayout->setContentsMargins(0, 0, 0, 0);
    roomListLayout->setSpacing(6);

    scrollArea->setWidget(scrollWidget);
    roomBoxLayout->addWidget(scrollArea);

    bottomLayout->addWidget(roomBox, 2);

    reportText->setMinimumHeight(100);
    mainLayout->addLayout(bottomLayout, 1);

    // Connections
    connect(executeBtn, &QPushButton::clicked, this, &DashboardWidget::onGenerateTimetable);
    connect(regenerateBtn, &QPushButton::clicked, this, &DashboardWidget::generateClicked);
    connect(viewTimetableBtn, &QPushButton::clicked, this, &DashboardWidget::switchToTimetableView);
    connect(compareVersionsBtn, &QPushButton::clicked, this, &DashboardWidget::onCompareVersions);

    connect(viewConflictsBtn, &QPushButton::clicked, this, [this]() {
        if (!dm || !dm->timetableGenerated) return;
        QString msg;
        for (const auto &ul : dm->lastTimetable.unscheduledLessons) {
            msg += QString("  • %1 | %2 | %3/%4 periods\n  Reason: %5\n\n")
                       .arg(QString::fromStdString(dm->getClassName(ul.classId)))
                       .arg(QString::fromStdString(dm->getSubjectName(ul.subjectId)))
                       .arg(ul.periodsCount)
                       .arg(ul.periodsCount)
                       .arg(QString::fromStdString(ul.reason));
        }
        if (msg.isEmpty())
            msg = tr("No conflicts detected.");
        QMessageBox::information(this, tr("Conflicts & Unscheduled"), msg);
    });
}

void DashboardWidget::refreshStats() {
    updateMetrics();
    updateUnscheduledBanner();
    updateRoomUtilization();
}

void DashboardWidget::updateMetrics() {
    if (!dm) return;

    // Scheduled: count non-empty cells across all schedules
    int scheduled = 0;
    for (const auto &classEntry : dm->lastTimetable.schedules) {
        for (const auto &dayVec : classEntry.second) {
            for (const auto &cell : dayVec) {
                if (!cell.isEmpty()) ++scheduled;
            }
        }
    }

    // Required: sum of all lesson periodsPerWeek
    int required = 0;
    for (const auto &l : dm->lessons) {
        required += l.periodsPerWeek;
    }

    // Coverage
    int coverage = (required > 0) ? (scheduled * 100 / required) : 0;

    // Locked (reserved for future — C++ solver does not set locked flag yet)
    int locked = 0;

    // Score
    int scoreVal = dm->timetableGenerated ? dm->lastTimetable.score : 0;

    // Conflicts = unscheduled
    int conflicts = dm->timetableGenerated
        ? static_cast<int>(dm->lastTimetable.unscheduledLessons.size()) : 0;

    scheduledValue->setText(QString::number(scheduled));
    requiredValue->setText(QString::number(required));

    // Coverage color: green >=95%, orange >=80%, red <80%
    QString coverColor = (coverage >= 95) ? "#4caf50" : (coverage >= 80) ? "#ff9800" : "#f44336";
    coverageValue->setText(QString::number(coverage) + "%");
    coverageValue->setStyleSheet(QString("font-size: 22pt; font-weight: 800; color: %1; border: none; background: transparent;").arg(coverColor));

    lockedValue->setText(QString::number(locked));
    scoreValueLabel->setText(scoreVal > 0 ? QString::number(scoreVal) : QString("—"));
    conflictsValue->setText(QString::number(conflicts));

    // Conflicts color: green if 0, red if >0
    QString conflictColor = (conflicts == 0) ? "#4caf50" : "#f44336";
    conflictsValue->setStyleSheet(QString("font-size: 22pt; font-weight: 800; color: %1; border: none; background: transparent;").arg(conflictColor));

    // Score color: orange always
    scoreValueLabel->setStyleSheet("font-size: 22pt; font-weight: 800; color: #ff6600; border: none; background: transparent;");
}

void DashboardWidget::updateUnscheduledBanner() {
    if (!dm || !dm->timetableGenerated || dm->lastTimetable.unscheduledLessons.empty()) {
        unscheduledSection->setVisible(false);
        return;
    }

    unscheduledSection->setVisible(true);

    // Clear previous items
    QLayoutItem *child;
    while ((child = unscheduledLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    int count = 0;
    for (const auto &ul : dm->lastTimetable.unscheduledLessons) {
        if (count >= 5) {
            QLabel *moreLbl = new QLabel(
                tr("…and %1 more").arg(dm->lastTimetable.unscheduledLessons.size() - 5),
                unscheduledSection);
            moreLbl->setStyleSheet("font-size: 9pt; color: #888; border: none; background: transparent;");
            unscheduledLayout->addWidget(moreLbl);
            break;
        }

        QString line = QString("  %1  |  %2  |  %3/%4  |  %5")
                           .arg(QString::fromStdString(dm->getClassName(ul.classId)), -12)
                           .arg(QString::fromStdString(dm->getSubjectName(ul.subjectId)), -12)
                           .arg(0)
                           .arg(ul.periodsCount)
                           .arg(QString::fromStdString(ul.reason));

        QLabel *itemLabel = new QLabel(line, unscheduledSection);
        itemLabel->setStyleSheet("font-size: 9pt; color: #e0e0e0; font-family: monospace; border: none; background: transparent;");
        unscheduledLayout->addWidget(itemLabel);
        ++count;
    }
}

void DashboardWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refreshStats();
}

void DashboardWidget::onGenerateTimetable() {
    if (!dm) return;

    if (dm->lessons.empty()) {
        QMessageBox::warning(this, tr("Scheduling Denied"),
                             tr("Cannot schedule: No lessons have been assigned yet."));
        return;
    }

    statusLabel->setText(tr("Status: Running scheduling engine…"));
    statusLabel->repaint();
    executeBtn->setEnabled(false);
    regenerateBtn->setEnabled(false);

    dm->placementRejectLog.clear();

    TimetableEngine engine;
    Timetable timetable = engine.generate(*dm);

    dm->lastTimetable = timetable;
    dm->timetableGenerated = true;

    executeBtn->setEnabled(true);
    regenerateBtn->setEnabled(true);

    QString report;
    report += QString("==== Scheduling Complete ====\n");
    report += QString("Quality Score: %1 / 1000\n").arg(timetable.score);
    report += QString("Nodes Visited: %1\n\n").arg(engine.getLastRunStats().nodesVisited);

    if (timetable.unscheduledLessons.empty()) {
        statusLabel->setText(tr("Status: Scheduling Succeeded (Score: %1/1000)").arg(timetable.score));
        statusLabel->setStyleSheet("font-size: 10pt; font-weight: bold; color: #4caf50; border: none; background: transparent;");
        report += QString("Success: All lessons scheduled successfully!\n");
    } else {
        statusLabel->setText(tr("Status: Scheduling Completed with warnings."));
        statusLabel->setStyleSheet("font-size: 10pt; font-weight: bold; color: #ff9800; border: none; background: transparent;");
        report += QString("WARNING: %1 lessons could not be scheduled.\n\n")
                      .arg(timetable.unscheduledLessons.size());
        report += QString("Unscheduled Lessons Details:\n");
        for (const auto &ul : timetable.unscheduledLessons) {
            report += QString(" - Class: %1, Subject: %2, Teacher: %3, Periods: %4\n   Reason: %5\n")
                          .arg(QString::fromStdString(dm->getClassName(ul.classId)))
                          .arg(QString::fromStdString(dm->getSubjectName(ul.subjectId)))
                          .arg(QString::fromStdString(dm->getTeacherName(ul.teacherId)))
                          .arg(ul.periodsCount)
                          .arg(QString::fromStdString(ul.reason));
        }
    }

    const auto &rejectLog = dm->getPlacementRejectLog();
    if (!rejectLog.empty()) {
        report += QString("\nConstraint Violations / Rejected Placements:\n");
        for (const auto &msg : rejectLog) {
            report += QString(" - %1\n").arg(QString::fromStdString(msg));
        }
    }

    // Append analytics summary
    AnalyticsService analytics;
    AnalyticsReport analyticsReport = analytics.generateReport(dm->lastTimetable, *dm);

    report += QString("\n==== Gap Analysis ====\n");
    int totalGaps = 0, maxGapOverall = 0;
    for (const auto &g : analyticsReport.classGaps) {
        if (g.totalGaps > 0) {
            report += QString(" %1: %2 gaps (max gap: %3)\n")
                          .arg(QString::fromStdString(g.className), -15)
                          .arg(g.totalGaps)
                          .arg(g.maxGap);
            totalGaps += g.totalGaps;
            maxGapOverall = std::max(maxGapOverall, g.maxGap);
        }
    }
    report += QString("Total gaps across all classes: %1 (largest: %2)\n").arg(totalGaps).arg(maxGapOverall);

    if (!analyticsReport.weekTypeDistribution.empty()) {
        report += QString("\n==== Week Type Distribution ====\n");
        for (const auto &w : analyticsReport.weekTypeDistribution) {
            QString label = (w.weekType == 0) ? "Every Week" : (w.weekType == 1) ? "Week A" : "Week B";
            report += QString(" %1: %2 slots\n").arg(label, -12).arg(w.count);
        }
    }

    if (!analyticsReport.subjectDistribution.empty()) {
        report += QString("\n==== Subject Distribution (per class) ====\n");
        std::map<std::string, std::vector<std::pair<std::string, int>>> grouped;
        for (const auto &s : analyticsReport.subjectDistribution) {
            grouped[s.className].push_back({s.subjectName, s.slotCount});
        }
        for (const auto &[cls, subs] : grouped) {
            report += QString(" %1:\n").arg(QString::fromStdString(cls));
            for (const auto &[sub, cnt] : subs) {
                report += QString("   %1: %2 slots\n")
                              .arg(QString::fromStdString(sub), -15)
                              .arg(cnt);
            }
        }
    }

    reportText->setPlainText(report);
    updateMetrics();
    updateUnscheduledBanner();
    updateRoomUtilization();

    emit generateClicked();
}

void DashboardWidget::updateRoomUtilization() {
    QLayoutItem *child;
    while ((child = roomListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    if (!dm) return;

    AnalyticsService analytics;
    AnalyticsReport report = analytics.generateReport(dm->lastTimetable, *dm);

    for (const auto &roomInfo : report.roomUtilizations) {
        QWidget *roomRow = new QWidget(this);
        roomRow->setStyleSheet("background: transparent;");
        QHBoxLayout *rowLayout = new QHBoxLayout(roomRow);
        rowLayout->setContentsMargins(0, 3, 0, 3);
        rowLayout->setSpacing(6);

        QLabel *nameLabel = new QLabel(QString::fromStdString(roomInfo.name), roomRow);
        nameLabel->setFixedWidth(90);
        nameLabel->setStyleSheet("font-weight: bold; color: #ffffff; border: none; background: transparent;");

        QProgressBar *bar = new QProgressBar(roomRow);
        bar->setRange(0, 100);
        bar->setValue(static_cast<int>(roomInfo.utilization));
        bar->setFixedHeight(14);
        bar->setStyleSheet(
            "QProgressBar { border: 1px solid rgba(255,255,255,0.1); border-radius: 3px; background: rgba(20,20,20,0.5); text-align: center; color: white; font-weight: bold; font-size: 8pt; }"
            "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff6600, stop:1 #ff8833); border-radius: 2px; }"
        );

        QLabel *detailLabel = new QLabel(QString("%1/%2").arg(roomInfo.usedSlots).arg(roomInfo.totalSlots), roomRow);
        detailLabel->setFixedWidth(60);
        detailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        detailLabel->setStyleSheet("color: #a0a0a0; font-size: 8pt; border: none; background: transparent;");

        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(bar, 1);
        rowLayout->addWidget(detailLabel);

        roomListLayout->addWidget(roomRow);
    }
    roomListLayout->addStretch();
}

namespace {

int countNonEmpty(const std::vector<std::vector<TimetableCell>> &grid) {
    int count = 0;
    for (const auto &day : grid)
        for (const auto &cell : day)
            if (!cell.isEmpty()) ++count;
    return count;
}

}  // namespace

void DashboardWidget::updateVersions() {
    if (compareVersionsBtn) {
        compareVersionsBtn->setVisible(dm->savedTimetables.size() >= 2);
    }
}

void DashboardWidget::onCompareVersions() {
    if (dm->savedTimetables.size() < 2) {
        QMessageBox::information(this, tr("Version Compare"),
            tr("Save at least 2 timetable versions first (use Save button in timetable view)."));
        return;
    }

    QStringList items;
    for (size_t i = 0; i < dm->savedTimetables.size(); ++i) {
        items << QString::fromStdString(dm->savedTimetables[i].label);
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Compare Versions"));
    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);

    dlgLayout->addWidget(new QLabel(tr("Select two versions to compare:")));

    QComboBox *v1Combo = new QComboBox(&dlg);
    QComboBox *v2Combo = new QComboBox(&dlg);
    v1Combo->addItems(items);
    v2Combo->addItems(items);
    if (items.size() >= 2) v2Combo->setCurrentIndex(1);

    dlgLayout->addWidget(new QLabel(tr("Version 1:")));
    dlgLayout->addWidget(v1Combo);
    dlgLayout->addWidget(new QLabel(tr("Version 2:")));
    dlgLayout->addWidget(v2Combo);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    dlgLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    int idx1 = v1Combo->currentIndex();
    int idx2 = v2Combo->currentIndex();
    if (idx1 == idx2) {
        QMessageBox::information(this, tr("Compare"), tr("Select two different versions."));
        return;
    }

    const auto &v1 = dm->savedTimetables[idx1].timetable;
    const auto &v2 = dm->savedTimetables[idx2].timetable;

    int onlyV1 = 0, onlyV2 = 0, changed = 0;
    for (const auto &pair : v1.schedules) {
        int cid = pair.first;
        const auto &grid1 = pair.second;
        auto it2 = v2.schedules.find(cid);
        if (it2 == v2.schedules.end()) {
            onlyV1 += countNonEmpty(grid1);
            continue;
        }
        const auto &grid2 = it2->second;
        for (size_t d = 0; d < grid1.size() && d < grid2.size(); ++d) {
            for (size_t p = 0; p < grid1[d].size() && p < grid2[d].size(); ++p) {
                if (grid1[d][p].isEmpty() && !grid2[d][p].isEmpty()) ++onlyV2;
                else if (!grid1[d][p].isEmpty() && grid2[d][p].isEmpty()) ++onlyV1;
                else if (!grid1[d][p].isEmpty() && !grid2[d][p].isEmpty() &&
                         (grid1[d][p].subjectId != grid2[d][p].subjectId ||
                          grid1[d][p].teacherId != grid2[d][p].teacherId))
                    ++changed;
            }
        }
    }
    for (const auto &pair : v2.schedules) {
        if (v1.schedules.find(pair.first) == v1.schedules.end()) {
            onlyV2 += countNonEmpty(pair.second);
        }
    }

    QString diff;
    diff += tr("Comparing: %1 vs %2\n\n")
                .arg(QString::fromStdString(dm->savedTimetables[idx1].label))
                .arg(QString::fromStdString(dm->savedTimetables[idx2].label));
    diff += tr("Score: %1 / %2\n\n").arg(v1.score).arg(v2.score);
    diff += tr("Differences:\n");
    diff += tr("  - Slots only in Version 1: %1\n").arg(onlyV1);
    diff += tr("  - Slots only in Version 2: %1\n").arg(onlyV2);
    diff += tr("  - Changed assignments: %1\n").arg(changed);

    QMessageBox::information(this, tr("Version Comparison"), diff);
}
