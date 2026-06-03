#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include "../../services/DataManager.h"

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(DataManager *dm, QWidget *parent = nullptr);
    ~DashboardWidget() override = default;

    void refreshStats();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onGenerateTimetable();

private:
    void setupUi();

    DataManager *dm;

    // Metrics widgets
    QLabel *teacherCountLabel;
    QLabel *subjectCountLabel;
    QLabel *classCountLabel;
    QLabel *roomCountLabel;

    // Scheduler widgets
    QLabel *statusLabel;
    QTextEdit *reportText;
    QPushButton *generateBtn;
};
