#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFrame>
#include "../../services/DataManager.h"
#include "../../timetable/Timetable.h"

class QVBoxLayout;

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(DataManager *dm, QWidget *parent = nullptr);
    ~DashboardWidget() override = default;

    void refreshStats();

signals:
    void generateClicked();
    void switchToTimetableView();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onGenerateTimetable();

private:
    void setupUi();
    void updateMetrics();
    void updateUnscheduledBanner();
    void updateRoomUtilization();
    void updateVersions();
    void onCompareVersions();

    DataManager *dm;

    // Metric card frames and labels
    QFrame *scheduledCard;
    QLabel *scheduledValue;
    QFrame *requiredCard;
    QLabel *requiredValue;
    QFrame *coverageCard;
    QLabel *coverageValue;
    QFrame *lockedCard;
    QLabel *lockedValue;
    QFrame *scoreCard;
    QLabel *scoreValueLabel;
    QFrame *conflictsCard;
    QLabel *conflictsValue;

    // Unscheduled section
    QWidget *unscheduledSection;
    QVBoxLayout *unscheduledLayout;

    // Action buttons
    QPushButton *viewTimetableBtn;
    QPushButton *regenerateBtn;
    QPushButton *viewConflictsBtn;

    // Scheduler widgets
    QLabel *statusLabel;
    QTextEdit *reportText;
    QPushButton *executeBtn;

    // Room Utilization widgets
    QVBoxLayout *roomListLayout;

    // Version comparison
    QWidget *versionsSection;
    QVBoxLayout *versionsLayout;
    QPushButton *compareVersionsBtn;

    QFrame *createMetricCard(const QString &title, QLabel *&valueLabel,
                             const QString &iconColor, QFrame *&cardFrame);
};
