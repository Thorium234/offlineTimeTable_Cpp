#include "DashboardWidget.h"
#include "../../services/TimetableEngine.h"
#include "../../services/AnalyticsService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QShowEvent>
#include <QMessageBox>
#include <QProgressBar>
#include <QScrollArea>

DashboardWidget::DashboardWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
    refreshStats();
}

void DashboardWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title / Welcome Banner
    QVBoxLayout *titleLayout = new QVBoxLayout;
    QLabel *titleLabel = new QLabel(tr("School Scheduling Dashboard"), this);
    titleLabel->setStyleSheet("font-size: 20pt; font-weight: bold; color: #ff6600;");
    QLabel *subtitleLabel = new QLabel(tr("Welcome to Timetable Generator. Monitor registered entities and execute the constraint scheduler."), this);
    subtitleLabel->setStyleSheet("font-size: 10pt; color: #a0a0a0;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(titleLayout);

    // Metrics grid
    QHBoxLayout *metricsLayout = new QHBoxLayout;
    metricsLayout->setSpacing(15);

    auto createMetricCard = [this](const QString &title, QLabel *&valLabel, const QString &iconColor) -> QFrame* {
        QFrame *card = new QFrame(this);
        card->setStyleSheet(QString(
            "QFrame {"
            "  background-color: rgba(35, 35, 35, 0.7);"
            "  border: 1px solid rgba(255, 255, 255, 0.08);"
            "  border-radius: 10px;"
            "}"
        ));
        
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(15, 15, 15, 15);
        layout->setSpacing(5);

        QLabel *lblTitle = new QLabel(title, card);
        lblTitle->setStyleSheet("font-size: 9pt; color: #888888; font-weight: bold; border: none; background: transparent;");
        
        valLabel = new QLabel("0", card);
        valLabel->setStyleSheet("font-size: 24pt; color: #ffffff; font-weight: bold; border: none; background: transparent;");

        layout->addWidget(lblTitle);
        layout->addWidget(valLabel);
        return card;
    };

    metricsLayout->addWidget(createMetricCard(tr("TEACHERS"), teacherCountLabel, "#ff6600"));
    metricsLayout->addWidget(createMetricCard(tr("SUBJECTS"), subjectCountLabel, "#ff6600"));
    metricsLayout->addWidget(createMetricCard(tr("CLASSES"), classCountLabel, "#ff6600"));
    metricsLayout->addWidget(createMetricCard(tr("ROOMS"), roomCountLabel, "#ff6600"));
    mainLayout->addLayout(metricsLayout);

    // Bottom layout: Scheduler Box (Left) and Room Utilization Box (Right)
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->setSpacing(20);

    // Scheduler Panel
    QGroupBox *schedulerBox = new QGroupBox(tr("Scheduling Engine Control"), this);
    schedulerBox->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 8px;"
        "  margin-top: 15px;"
        "  padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "  color: #ff6600;"
        "}"
    );

    QVBoxLayout *schedLayout = new QVBoxLayout(schedulerBox);
    schedLayout->setContentsMargins(15, 15, 15, 15);
    schedLayout->setSpacing(12);

    QHBoxLayout *controlRow = new QHBoxLayout;
    statusLabel = new QLabel(tr("Status: Ready to schedule."), schedulerBox);
    statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #e0e0e0; border: none; background: transparent;");

    generateBtn = new QPushButton(tr("Execute Scheduler"), schedulerBox);
    generateBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ff6600;"
        "  color: white;"
        "  font-size: 11pt;"
        "  font-weight: bold;"
        "  border-radius: 5px;"
        "  padding: 8px 24px;"
        "}"
        "QPushButton:hover { background-color: #ff7711; }"
        "QPushButton:pressed { background-color: #cc5200; }"
    );

    controlRow->addWidget(statusLabel);
    controlRow->addStretch();
    controlRow->addWidget(generateBtn);
    schedLayout->addLayout(controlRow);

    QLabel *reportLbl = new QLabel(tr("Execution Diagnostic Report:"), schedulerBox);
    reportLbl->setStyleSheet("font-size: 9pt; color: #888888; border: none; background: transparent;");
    schedLayout->addWidget(reportLbl);

    reportText = new QTextEdit(schedulerBox);
    reportText->setReadOnly(true);
    reportText->setPlaceholderText(tr("Scheduler diagnostic outputs and constraint violation details will appear here after execution."));
    reportText->setStyleSheet(
        "QTextEdit {"
        "  background-color: rgba(20, 20, 20, 0.8);"
        "  border: 1px solid rgba(255, 255, 255, 0.05);"
        "  border-radius: 5px;"
        "  color: #a0a0a0;"
        "  font-family: monospace;"
        "  font-size: 9.5pt;"
        "}"
    );
    schedLayout->addWidget(reportText);

    bottomLayout->addWidget(schedulerBox, 3); // 3/5 width ratio

    // Room Utilization Panel
    roomUtilizationBox = new QGroupBox(tr("Room Utilization"), this);
    roomUtilizationBox->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 8px;"
        "  margin-top: 15px;"
        "  padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "  color: #ff6600;"
        "}"
    );

    QVBoxLayout *roomBoxLayout = new QVBoxLayout(roomUtilizationBox);
    roomBoxLayout->setContentsMargins(10, 15, 10, 10);

    QScrollArea *scrollArea = new QScrollArea(roomUtilizationBox);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    QWidget *scrollWidget = new QWidget(scrollArea);
    scrollWidget->setStyleSheet("background: transparent;");
    roomListLayout = new QVBoxLayout(scrollWidget);
    roomListLayout->setContentsMargins(0, 0, 0, 0);
    roomListLayout->setSpacing(8);

    scrollArea->setWidget(scrollWidget);
    roomBoxLayout->addWidget(scrollArea);

    bottomLayout->addWidget(roomUtilizationBox, 2); // 2/5 width ratio

    mainLayout->addLayout(bottomLayout);
    mainLayout->setStretchFactor(bottomLayout, 1);

    connect(generateBtn, &QPushButton::clicked, this, &DashboardWidget::onGenerateTimetable);
}

void DashboardWidget::refreshStats() {
    if (dm) {
        teacherCountLabel->setText(QString::number(dm->teachers.size()));
        subjectCountLabel->setText(QString::number(dm->subjects.size()));
        classCountLabel->setText(QString::number(dm->classes.size()));
        roomCountLabel->setText(QString::number(dm->rooms.size()));
        updateRoomUtilization(Timetable()); // Populate rooms with 0% utilization on start
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
                             tr("Cannot schedule: No lessons have been assigned yet in the system."));
        return;
    }

    statusLabel->setText(tr("Status: Running scheduling engine..."));
    statusLabel->repaint();
    generateBtn->setEnabled(false);

    // Clear placement reject logs first
    dm->placementRejectLog.clear();

    TimetableEngine engine;
    Timetable timetable = engine.generate(*dm);

    // Store in DataManager so other tabs can access it
    dm->lastTimetable = timetable;
    dm->timetableGenerated = true;

    generateBtn->setEnabled(true);

    QString report;
    report += QString("==== Scheduling Complete ====\n");
    report += QString("Quality Score: %1 / 1000\n").arg(timetable.score);
    report += QString("Nodes Visited: %1\n\n").arg(engine.getLastRunStats().nodesVisited);

    if (timetable.unscheduledLessons.empty()) {
        statusLabel->setText(tr("Status: Scheduling Succeeded (Score: %1/1000)").arg(timetable.score));
        statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #4caf50;");
        report += QString("Success: All lessons scheduled successfully!\n");
    } else {
        statusLabel->setText(tr("Status: Scheduling Completed with warnings."));
        statusLabel->setStyleSheet("font-size: 11pt; font-weight: bold; color: #ff9800;");
        report += QString("WARNING: %1 lessons could not be scheduled.\n\n").arg(timetable.unscheduledLessons.size());
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

    reportText->setPlainText(report);
    updateRoomUtilization(timetable);
}

void DashboardWidget::updateRoomUtilization(const Timetable &timetable) {
    // Clear previous items in room layout
    QLayoutItem *child;
    while ((child = roomListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    AnalyticsService analytics;
    AnalyticsReport report = analytics.generateReport(timetable, *dm);

    for (const auto &roomInfo : report.roomUtilizations) {
        QWidget *roomRow = new QWidget(this);
        QHBoxLayout *rowLayout = new QHBoxLayout(roomRow);
        rowLayout->setContentsMargins(0, 5, 0, 5);

        QLabel *nameLabel = new QLabel(QString::fromStdString(roomInfo.name), roomRow);
        nameLabel->setFixedWidth(100);
        nameLabel->setStyleSheet("font-weight: bold; color: #ffffff; border: none; background: transparent;");

        QProgressBar *bar = new QProgressBar(roomRow);
        bar->setRange(0, 100);
        bar->setValue(static_cast<int>(roomInfo.utilization));
        // Customize styling: ASC orange gradient
        bar->setStyleSheet(
            "QProgressBar {"
            "  border: 1px solid rgba(255, 255, 255, 0.1);"
            "  border-radius: 4px;"
            "  background-color: rgba(20, 20, 20, 0.5);"
            "  text-align: center;"
            "  color: white;"
            "  font-weight: bold;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff6600, stop:1 #ff8833);"
            "  border-radius: 3px;"
            "}"
        );

        QLabel *detailLabel = new QLabel(QString("%1/%2 slots").arg(roomInfo.usedSlots).arg(roomInfo.totalSlots), roomRow);
        detailLabel->setFixedWidth(80);
        detailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        detailLabel->setStyleSheet("color: #a0a0a0; font-size: 9pt; border: none; background: transparent;");

        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(bar);
        rowLayout->addWidget(detailLabel);

        roomListLayout->addWidget(roomRow);
    }
    // Add stretch at the end to keep items compressed at top
    roomListLayout->addStretch();
}
