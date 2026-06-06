#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QComboBox>
#include "../../services/DataManager.h"
#include "../../timetable/Timetable.h"
#include "../ribbon/RibbonToolbar.h"

class TimetableScene;

class TimetableViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimetableViewWidget(DataManager *dm, QWidget *parent = nullptr);
    void setTimetable(const Timetable &t);
    void setViewMode(ViewMode mode);
    ViewMode currentViewMode() const { return currentMode; }
    void setViewSelection(const QString &type, int id);
    void setConstraintMode(bool enabled);
    bool isConstraintMode() const { return constraintMode; }
    void refresh();

signals:
    void undoRedoStateChanged();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void onCardMoved(class LessonCardItem *item, int fromDay, int fromPeriod,
                     int toDay, int toPeriod);
    void onCardLockToggled(class LessonCardItem *item);

    DataManager *dm;
    Timetable timetable;
    ViewMode currentMode = ViewMode::CLASS;
    int selectedEntityId = -1;
    bool constraintMode = false;

    QGraphicsView *view;
    TimetableScene *scene;
    QLabel *viewTitle;
    QComboBox *weekCombo;
    QWidget *emptyState;
};
