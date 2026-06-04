#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include "../../services/DataManager.h"
#include "../../timetable/Timetable.h"
#include "../ribbon/RibbonToolbar.h"

class TimetableViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimetableViewWidget(DataManager *dm, QWidget *parent = nullptr);
    void setTimetable(const Timetable &t);
    void setViewMode(ViewMode mode);
    void setViewSelection(const QString &type, int id);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void populateGrid();
    void populateClassGrid();
    void populateTeacherGrid();
    void populateRoomGrid();
    void onCellClicked(int row, int col);
    QColor colorForTeacher(int teacherId) const;
    QColor colorForSubject(int subjectId) const;

    DataManager *dm;
    Timetable timetable;
    ViewMode currentMode = ViewMode::CLASS;
    int selectedEntityId = -1;

    QLabel *viewTitle;
    QTableWidget *gridWidget;
    QWidget *emptyState;
};
