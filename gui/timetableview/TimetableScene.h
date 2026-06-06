#pragma once
#include <QGraphicsScene>
#include <QMap>
#include <QPair>
#include <QString>
#include "../../services/DataManager.h"

class LessonCardItem;
class Timetable;

class TimetableScene : public QGraphicsScene {
    Q_OBJECT
public:
    static constexpr qreal HEADER_WIDTH = 70;
    static constexpr qreal HEADER_HEIGHT = 32;
    static constexpr qreal CELL_WIDTH = 140;
    static constexpr qreal CELL_HEIGHT = 60;
    static constexpr qreal GRID_SPACING = 2;

    explicit TimetableScene(DataManager *dm, QObject *parent = nullptr);

    void setTimetable(const Timetable &tt, int filterEntityId = -1, int filterMode = 0);
    void setWeekFilter(int weekType);
    int weekFilter() const { return m_weekFilter; }
    void setConstraintMode(bool enabled);
    bool isConstraintMode() const { return m_constraintMode; }
    void clearGrid();
    void rebuildGrid();

    QPointF cellPos(int dayIndex, int periodIndex) const;
    QPair<int, int> posToCell(QPointF pos) const;
    QRectF cellRect(int dayIndex, int periodIndex) const;

    int numDays() const { return m_numDays; }
    int numPeriods() const { return m_numPeriods; }
    int filterEntityId() const { return m_filterEntityId; }
    int filterMode() const { return m_filterMode; }

    void addCardToCell(LessonCardItem *card, int dayIndex, int periodIndex);
    void removeCardFromCell(int classId, int dayIndex, int periodIndex);
    LessonCardItem *cardAt(int classId, int dayIndex, int periodIndex) const;
    LessonCardItem *cardAtCell(int dayIndex, int periodIndex) const;

signals:
    void cardMoved(LessonCardItem *item, int fromDay, int fromPeriod,
                   int toDay, int toPeriod);
    void cardLockToggled(LessonCardItem *item);
    void constraintToggled(int teacherId, int dayId, int periodId, bool blocked);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QColor colorForSubject(int subjectId) const;
    void rebuildFromTimetable();

    DataManager *m_dm;
    const Timetable *m_tt = nullptr;
    int m_numDays = 0;
    int m_numPeriods = 0;
    int m_filterEntityId = -1;
    int m_filterMode = 0; // 0=class, 1=teacher, 2=room
    bool m_constraintMode = false;
    int m_weekFilter = 0; // 0=all, 1=weekA, 2=weekB

    using CellKey = QPair<int, QPair<int, int>>; // classId, day, period
    QMap<CellKey, LessonCardItem *> m_cards;
};
