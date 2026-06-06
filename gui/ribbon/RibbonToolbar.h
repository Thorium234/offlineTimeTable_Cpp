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
    void setUndoEnabled(bool enabled);
    void setRedoEnabled(bool enabled);

signals:
    void generateClicked();
    void exportCsvClicked();
    void exportHtmlClicked();
    void exportPdfClicked();
    void printPreviewClicked();
    void benchmarkClicked();
    void viewModeChanged(ViewMode mode);
    void homeClicked();
    void timetableClicked();
    void undoClicked();
    void redoClicked();
    void lockClicked();
    void constraintsModeToggled(bool enabled);
    void substitutionsClicked();
    void divisionsClicked();
    void violationsClicked();

private:
    void setupUi();
    QPushButton *createButton(const QString &text, const QString &tooltip);

    DataManager *dm;
    QPushButton *homeBtn;
    QPushButton *timetableTabBtn;
    QPushButton *generateBtn;
    QPushButton *exportCsvBtn;
    QPushButton *exportHtmlBtn;
    QPushButton *exportPdfBtn;
    QPushButton *printBtn;
    QPushButton *benchmarkBtn;
    QPushButton *undoBtn;
    QPushButton *redoBtn;
    QPushButton *lockBtn;
    QPushButton *constraintsBtn;
    QPushButton *substitutionsBtn;
    QPushButton *divisionsBtn;
    QPushButton *violationsBtn;
    QButtonGroup *viewModeGroup;
    QLabel *scoreLabel;
    QLabel *statusLabel;
};
