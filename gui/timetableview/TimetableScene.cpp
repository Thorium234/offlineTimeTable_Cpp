#include "TimetableScene.h"
#include "LessonCardItem.h"
#include "../../timetable/Timetable.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

TimetableScene::TimetableScene(DataManager *dm, QObject *parent)
    : QGraphicsScene(parent), m_dm(dm) {}

void TimetableScene::setTimetable(const Timetable &tt, int filterEntityId, int filterMode) {
    m_tt = &tt;
    m_filterEntityId = filterEntityId;
    m_filterMode = filterMode;
    rebuildGrid();
}

void TimetableScene::setWeekFilter(int weekType) {
    m_weekFilter = weekType;
    if (m_tt) rebuildGrid();
}

void TimetableScene::setConstraintMode(bool enabled) {
    m_constraintMode = enabled;
    update(); // Trigger redraw to show/hide overlay
}

void TimetableScene::clearGrid() {
    m_cards.clear();
    clear();
    m_numDays = 0;
    m_numPeriods = 0;
}

void TimetableScene::rebuildGrid() {
    clearGrid();
    if (!m_tt || !m_dm) return;

    m_numDays = static_cast<int>(m_dm->days.size());
    m_numPeriods = static_cast<int>(m_dm->periods.size());

    if (m_numDays <= 0 || m_numPeriods <= 0) return;

    qreal totalW = HEADER_WIDTH + m_numPeriods * (CELL_WIDTH + GRID_SPACING) + GRID_SPACING;
    qreal totalH = HEADER_HEIGHT + m_numDays * (CELL_HEIGHT + GRID_SPACING) + GRID_SPACING;
    setSceneRect(0, 0, totalW, totalH);

    rebuildFromTimetable();
}

void TimetableScene::rebuildFromTimetable() {
    if (!m_tt) return;

    for (const auto &pair : m_tt->schedules) {
        int classId = pair.first;
        const auto &grid = pair.second;

        for (int d = 0; d < m_numDays && d < static_cast<int>(grid.size()); ++d) {
            for (int p = 0; p < m_numPeriods && p < static_cast<int>(grid[d].size()); ++p) {
                const auto &cell = grid[d][p];
                if (cell.isEmpty()) continue;

                // Week filter: 0=all, 1=weekA only, 2=weekB only
                if (m_weekFilter != 0 && cell.weekType != 0 && cell.weekType != m_weekFilter) continue;

                // Filtering: only show matching entries
                if (m_filterEntityId > 0) {
                    if (m_filterMode == 0 && classId != m_filterEntityId) continue;
                    if (m_filterMode == 1 && cell.teacherId != m_filterEntityId) continue;
                    if (m_filterMode == 2 && cell.roomId != m_filterEntityId) continue;
                }

                auto *card = new LessonCardItem(
                    classId, d, p,
                    cell.subjectId, cell.teacherId, cell.roomId,
                    QString::fromStdString(m_dm->getSubjectName(cell.subjectId)),
                    QString::fromStdString(m_dm->getTeacherName(cell.teacherId)),
                    QString::fromStdString(m_dm->getRoomName(cell.roomId)),
                    QString::fromStdString(m_dm->getClassName(classId)),
                    cell.locked
                );

                addCardToCell(card, d, p);
            }
        }
    }
}

QPointF TimetableScene::cellPos(int dayIndex, int periodIndex) const {
    qreal x = HEADER_WIDTH + GRID_SPACING + periodIndex * (CELL_WIDTH + GRID_SPACING);
    qreal y = HEADER_HEIGHT + GRID_SPACING + dayIndex * (CELL_HEIGHT + GRID_SPACING);
    return QPointF(x, y);
}

QPair<int, int> TimetableScene::posToCell(QPointF pos) const {
    qreal x = pos.x() - HEADER_WIDTH;
    qreal y = pos.y() - HEADER_HEIGHT;

    int period = static_cast<int>(x / (CELL_WIDTH + GRID_SPACING));
    int day = static_cast<int>(y / (CELL_HEIGHT + GRID_SPACING));

    day = qBound(0, day, m_numDays - 1);
    period = qBound(0, period, m_numPeriods - 1);

    return {day, period};
}

QRectF TimetableScene::cellRect(int dayIndex, int periodIndex) const {
    QPointF pos = cellPos(dayIndex, periodIndex);
    return QRectF(pos, QSizeF(CELL_WIDTH, CELL_HEIGHT));
}

void TimetableScene::addCardToCell(LessonCardItem *card, int dayIndex, int periodIndex) {
    CellKey key(card->classId(), {dayIndex, periodIndex});
    m_cards[key] = card;
    card->setPos(cellPos(dayIndex, periodIndex));
    addItem(card);
}

void TimetableScene::removeCardFromCell(int classId, int dayIndex, int periodIndex) {
    CellKey key(classId, {dayIndex, periodIndex});
    auto it = m_cards.find(key);
    if (it != m_cards.end()) {
        removeItem(it.value());
        delete it.value();
        m_cards.erase(it);
    }
}

LessonCardItem *TimetableScene::cardAt(int classId, int dayIndex, int periodIndex) const {
    CellKey key(classId, {dayIndex, periodIndex});
    auto it = m_cards.find(key);
    return it != m_cards.end() ? it.value() : nullptr;
}

LessonCardItem *TimetableScene::cardAtCell(int dayIndex, int periodIndex) const {
    for (auto it = m_cards.begin(); it != m_cards.end(); ++it) {
        const auto &k = it.key();
        if (k.second.first == dayIndex && k.second.second == periodIndex) {
            return it.value();
        }
    }
    return nullptr;
}

void TimetableScene::drawBackground(QPainter *painter, const QRectF &) {
    if (m_numDays <= 0 || m_numPeriods <= 0) return;

    painter->setRenderHint(QPainter::Antialiasing, false);

    // Background
    painter->fillRect(sceneRect(), QColor("#1a1a2e"));

    // Draw headers - Day names on left
    QFont hFont = painter->font();
    hFont.setPointSize(9);
    hFont.setBold(true);
    painter->setFont(hFont);
    painter->setPen(QColor("#e0e0e0"));

    for (int d = 0; d < m_numDays; ++d) {
        QRectF dayRect(0, HEADER_HEIGHT + GRID_SPACING + d * (CELL_HEIGHT + GRID_SPACING),
                       HEADER_WIDTH, CELL_HEIGHT);
        painter->drawText(dayRect, Qt::AlignCenter,
                          QString::fromStdString(m_dm->days[d].name));
    }

    // Draw headers - Period times on top
    QFont pFont = painter->font();
    pFont.setPointSize(8);
    pFont.setBold(false);
    painter->setFont(pFont);

    for (int p = 0; p < m_numPeriods; ++p) {
        QRectF periodRect(HEADER_WIDTH + GRID_SPACING + p * (CELL_WIDTH + GRID_SPACING),
                          0, CELL_WIDTH, HEADER_HEIGHT);
        painter->drawText(periodRect, Qt::AlignCenter,
                          QString::fromStdString(m_dm->periods[p].startTime));
    }

    // Draw grid cells
    painter->setPen(QPen(QColor("#2a2a4a"), 1));
    for (int d = 0; d < m_numDays; ++d) {
        for (int p = 0; p < m_numPeriods; ++p) {
            QRectF r = cellRect(d, p);
            bool blocked = false;
            // Check if this cell is blocked for the current teacher in constraint mode
            if (m_constraintMode && m_filterMode == 1 && m_filterEntityId > 0) {
                int dayId = m_dm->days[d].id;
                int periodId = m_dm->periods[p].id;
                blocked = m_dm->isTeacherUnavailable(m_filterEntityId, dayId, periodId);
            }
            if (blocked) {
                painter->fillRect(r, QColor("#5c1a1a"));
                painter->setPen(QPen(QColor("#e74c3c"), 2));
                painter->drawRect(r);
                // Draw X
                painter->setPen(QPen(QColor("#e74c3c"), 2));
                painter->drawLine(r.topLeft(), r.bottomRight());
                painter->drawLine(r.topRight(), r.bottomLeft());
            } else {
                painter->fillRect(r, QColor("#16213e"));
                painter->setPen(QPen(QColor("#2a2a4a"), 1));
                painter->drawRect(r);
            }
        }
    }
}

void TimetableScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    if (auto *card = dynamic_cast<LessonCardItem *>(item)) {
        if (event->button() == Qt::RightButton) {
            emit cardLockToggled(card);
            event->accept();
            return;
        }
    }

    // Constraint mode: click on empty cell to toggle teacher block
    if (m_constraintMode && m_filterMode == 1 && m_filterEntityId > 0) {
        QPair<int, int> cell = posToCell(event->scenePos());
        int dayIndex = cell.first;
        int periodIndex = cell.second;
        if (dayIndex >= 0 && dayIndex < m_numDays && periodIndex >= 0 && periodIndex < m_numPeriods) {
            if (!cardAtCell(dayIndex, periodIndex)) {
                int teacherId = m_filterEntityId;
                int dayId = m_dm->days[dayIndex].id;
                int periodId = m_dm->periods[periodIndex].id;
                bool currentlyBlocked = m_dm->isTeacherUnavailable(teacherId, dayId, periodId);
                emit constraintToggled(teacherId, dayId, periodId, !currentlyBlocked);
                event->accept();
                return;
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void TimetableScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QList<QGraphicsItem *> items = this->items(event->scenePos());
    LessonCardItem *droppedCard = nullptr;
    LessonCardItem *cardAtPos = nullptr;

    for (auto *item : items) {
        if (auto *card = dynamic_cast<LessonCardItem *>(item)) {
            if (card->isUnderMouse()) {
                droppedCard = card;
            } else {
                cardAtPos = card;
            }
        }
    }

    if (droppedCard) {
        QPair<int, int> cell = posToCell(droppedCard->pos());
        int fromDay = droppedCard->dayIndex();
        int fromPeriod = droppedCard->periodIndex();
        int toDay = cell.first;
        int toPeriod = cell.second;

        if (fromDay != toDay || fromPeriod != toPeriod) {
            qreal snapX = HEADER_WIDTH + GRID_SPACING + toPeriod * (CELL_WIDTH + GRID_SPACING);
            qreal snapY = HEADER_HEIGHT + GRID_SPACING + toDay * (CELL_HEIGHT + GRID_SPACING);

            droppedCard->setPos(snapX, snapY);
            droppedCard->setDayIndex(toDay);
            droppedCard->setPeriodIndex(toPeriod);
            emit cardMoved(droppedCard, fromDay, fromPeriod, toDay, toPeriod);
        } else {
            // Snap back to exact position
            QPointF snapPos = cellPos(fromDay, fromPeriod);
            droppedCard->setPos(snapPos);
        }
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

QColor TimetableScene::colorForSubject(int subjectId) const {
    if (subjectId <= 0) return QColor("#2a2a2a");
    int hue = (subjectId * 47 + 30) % 360;
    return QColor::fromHsl(hue, 140, 75);
}
