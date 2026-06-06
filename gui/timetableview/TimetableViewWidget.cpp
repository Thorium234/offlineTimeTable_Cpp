#include "TimetableViewWidget.h"
#include "TimetableScene.h"
#include "LessonCardItem.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

TimetableViewWidget::TimetableViewWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
}

void TimetableViewWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    viewTitle = new QLabel(tr("Class Timetable"), this);
    viewTitle->setObjectName("GridTitle");
    viewTitle->setFixedHeight(32);
    titleRow->addWidget(viewTitle, 1);

    weekCombo = new QComboBox(this);
    weekCombo->addItem(tr("Combined"), 0);
    weekCombo->addItem(tr("Week A"), 1);
    weekCombo->addItem(tr("Week B"), 2);
    weekCombo->setFixedHeight(28);
    weekCombo->setStyleSheet(
        "QComboBox { background: #2a2a2a; border: 1px solid #444; border-radius: 4px; padding: 2px 8px; color: #ddd; }"
        "QComboBox:hover { border-color: #8e44ad; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #2a2a2a; color: #ddd; selection-background-color: #8e44ad; }"
    );
    titleRow->addWidget(weekCombo);
    mainLayout->addLayout(titleRow);

    scene = new TimetableScene(dm, this);
    view = new QGraphicsView(scene, this);
    view->setObjectName("TimetableGraphicsView");
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::NoDrag);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setBackgroundBrush(QColor("#1a1a2e"));
    mainLayout->addWidget(view, 1);

    connect(weekCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int wt = weekCombo->itemData(idx).toInt();
        scene->setWeekFilter(wt);
    });

    connect(scene, &TimetableScene::cardMoved, this, &TimetableViewWidget::onCardMoved);
    connect(scene, &TimetableScene::cardLockToggled, this, &TimetableViewWidget::onCardLockToggled);
    connect(scene, &TimetableScene::constraintToggled, this, [this](int teacherId, int dayId, int periodId, bool blocked) {
        if (blocked) {
            dm->addTeacherConstraint(teacherId, dayId, periodId);
        } else {
            // Remove constraint — find and remove from vector
            for (auto it = dm->teacherConstraints.begin(); it != dm->teacherConstraints.end(); ++it) {
                if (it->teacherId == teacherId && it->dayId == dayId && it->periodId == periodId) {
                    dm->teacherConstraints.erase(it);
                    break;
                }
            }
            // Also remove from DB
            dm->sql.removeTeacherConstraint(teacherId, dayId, periodId);
        }
        // Redraw scene to reflect changes
        if (dm->timetableGenerated)
            setTimetable(dm->lastTimetable);
    });

    // Empty state
    emptyState = new QWidget(this);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyState);
    QLabel *emptyLabel = new QLabel(tr("No timetable generated yet.\nAdd resources and lessons, then click Generate."), emptyState);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #555; font-size: 14pt; border: none; background: transparent;");
    emptyLayout->addWidget(emptyLabel);
    emptyState->setVisible(true);
    mainLayout->addWidget(emptyState);
}

void TimetableViewWidget::setTimetable(const Timetable &t) {
    timetable = t;
    dm->timetableGenerated = true;
    dm->lastTimetable = t;
    emptyState->setVisible(false);
    view->setVisible(true);

    int filterId = -1;
    int filterMode = 0;
    if (selectedEntityId > 0) {
        switch (currentMode) {
            case ViewMode::CLASS:   filterMode = 0; filterId = selectedEntityId; break;
            case ViewMode::TEACHER: filterMode = 1; filterId = selectedEntityId; break;
            case ViewMode::ROOM:    filterMode = 2; filterId = selectedEntityId; break;
        }
    }

    scene->setTimetable(t, filterId, filterMode);

    if (filterId > 0) {
        QString modeStr = (currentMode == ViewMode::CLASS) ? "Class" :
                          (currentMode == ViewMode::TEACHER) ? "Teacher" : "Room";
        viewTitle->setText(tr("%1 Timetable: %2")
            .arg(modeStr)
            .arg(QString::fromStdString(
                currentMode == ViewMode::CLASS ? dm->getClassName(filterId) :
                currentMode == ViewMode::TEACHER ? dm->getTeacherName(filterId) :
                dm->getRoomName(filterId))));
    } else {
        viewTitle->setText(tr("Timetable (all)"));
    }

    emit undoRedoStateChanged();
}

void TimetableViewWidget::setConstraintMode(bool enabled) {
    constraintMode = enabled;
    scene->setConstraintMode(enabled);
}

void TimetableViewWidget::setViewMode(ViewMode mode) {
    currentMode = mode;
    selectedEntityId = -1;
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
}

void TimetableViewWidget::setViewSelection(const QString &type, int id) {
    selectedEntityId = id;
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
}

void TimetableViewWidget::refresh() {
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
    emit undoRedoStateChanged();
}

void TimetableViewWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
}

void TimetableViewWidget::onCardMoved(LessonCardItem *item, int fromDay, int fromPeriod,
                                       int toDay, int toPeriod) {
    if (!dm->manualMoveSlot(item->classId(), fromDay, fromPeriod,
                            item->classId(), toDay, toPeriod)) {
        // Move failed — snap card back
        QPointF snapBack = scene->cellPos(fromDay, fromPeriod);
        item->setPos(snapBack);
        item->setDayIndex(fromDay);
        item->setPeriodIndex(fromPeriod);
    }
    emit undoRedoStateChanged();
}

void TimetableViewWidget::onCardLockToggled(LessonCardItem *item) {
    dm->toggleSlotLock(item->classId(), item->dayIndex(), item->periodIndex());
    bool locked = dm->isSlotLocked(item->classId(), item->dayIndex(), item->periodIndex());
    item->setLocked(locked);
    emit undoRedoStateChanged();
}
