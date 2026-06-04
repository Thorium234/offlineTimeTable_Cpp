#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QLabel>
#include "../services/DataManager.h"
#include "../timetable/Timetable.h"
#include "ribbon/RibbonToolbar.h"
#include "sidebar/DataSidebar.h"
#include "timetableview/TimetableViewWidget.h"

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
    void onBenchmark();
    void onViewModeChanged(ViewMode mode);
    void onSidebarItemSelected(const QString &type, int id);
    void updateStatusBar();

    DataManager dm;
    RibbonToolbar *ribbon;
    DataSidebar *sidebar;
    TimetableViewWidget *timetableView;
    QSplitter *splitter;
    QLabel *statusInfoLabel;
};
