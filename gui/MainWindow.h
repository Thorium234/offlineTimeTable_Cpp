#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>
#include "../services/DataManager.h"
#include "../timetable/Timetable.h"
#include "ribbon/RibbonToolbar.h"
#include "sidebar/DataSidebar.h"
#include "timetableview/TimetableViewWidget.h"
#include "dashboard/DashboardWidget.h"
#include "substitutions/SubstitutionWidget.h"
#include "divisions/DivisionWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void loadStyleSheet();
    void onGenerateTimetable();
    void onExportCSV();
    void onExportHTML();
    void onExportPDF();
    void onPrintPreview();
    void onBenchmark();
    void onViewModeChanged(ViewMode mode);
    void onSidebarItemSelected(const QString &type, int id);
    void onHomeClicked();
    void onTimetableClicked();
    void onSubstitutionsClicked();
    void onDivisionsClicked();
    void onViolationsClicked();
    void onUndoClicked();
    void onRedoClicked();
    void onLockClicked();
    void onConstraintsModeToggled(bool enabled);
    void updateUndoRedoButtons();
    void updateStatusBar();

    DataManager dm;
    RibbonToolbar *ribbon;
    DataSidebar *sidebar;
    DashboardWidget *dashboard;
    TimetableViewWidget *timetableView;
    SubstitutionWidget *substitutionWidget;
    DivisionWidget *divisionWidget;
    QStackedWidget *stack;
    QSplitter *splitter;
    QLabel *statusInfoLabel;
};
