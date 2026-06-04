#pragma once
#include <QWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include "../../services/DataManager.h"
#include "../../timetable/Timetable.h"

enum class ViewMode { CLASS, TEACHER, ROOM };

class RibbonToolbar : public QWidget {
    Q_OBJECT
public:
    explicit RibbonToolbar(DataManager *dm, QWidget *parent = nullptr);

    void setScore(int score);
    void setStatusText(const QString &text);
    void setGenerationRunning(bool running);

signals:
    void generateClicked();
    void exportCsvClicked();
    void exportHtmlClicked();
    void benchmarkClicked();
    void viewModeChanged(ViewMode mode);

private:
    void setupUi();
    QPushButton *createButton(const QString &text, const QString &tooltip);

    DataManager *dm;
    QPushButton *generateBtn;
    QPushButton *exportCsvBtn;
    QPushButton *exportHtmlBtn;
    QPushButton *benchmarkBtn;
    QButtonGroup *viewModeGroup;
    QLabel *scoreLabel;
    QLabel *statusLabel;
};
