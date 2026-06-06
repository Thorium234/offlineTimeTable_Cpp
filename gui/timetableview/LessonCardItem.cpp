#include "LessonCardItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QCursor>

LessonCardItem::LessonCardItem(int classId, int dayIndex, int periodIndex,
                               int subjectId, int teacherId, int roomId,
                               const QString &subjectName, const QString &teacherName,
                               const QString &roomName, const QString &className,
                               bool locked, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_classId(classId)
    , m_dayIndex(dayIndex)
    , m_periodIndex(periodIndex)
    , m_subjectId(subjectId)
    , m_teacherId(teacherId)
    , m_roomId(roomId)
    , m_subjectName(subjectName)
    , m_teacherName(teacherName)
    , m_roomName(roomName)
    , m_className(className)
    , m_locked(locked)
    , m_dragging(false)
{
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);

    int hue = (m_subjectId * 47 + 30) % 360;
    m_bgColor = QColor::fromHsl(hue, 140, 75);
    m_fgColor = m_bgColor.lightness() > 128 ? QColor("#1a1a1a") : QColor("#ffffff");
}

QRectF LessonCardItem::boundingRect() const {
    return QRectF(0, 0, CARD_WIDTH, CARD_HEIGHT);
}

void LessonCardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF r = boundingRect();

    if (m_locked) {
        painter->setBrush(m_bgColor.darker(130));
        painter->setPen(QPen(QColor("#f0a500"), 2));
    } else {
        painter->setBrush(m_bgColor);
        painter->setPen(QPen(m_bgColor.darker(120), 1));
    }
    painter->drawRoundedRect(r.adjusted(1, 1, -1, -1), 4, 4);

    painter->setPen(m_fgColor);
    QFont f = painter->font();
    f.setPointSize(8);
    f.setBold(true);
    painter->setFont(f);
    painter->drawText(r.adjusted(4, 2, -4, 0), Qt::AlignLeft | Qt::AlignTop, m_subjectName);

    f.setPointSize(7);
    f.setBold(false);
    painter->setFont(f);
    painter->drawText(r.adjusted(4, 18, -4, 0), Qt::AlignLeft | Qt::AlignTop, m_teacherName);
    painter->drawText(r.adjusted(4, 30, -4, 2), Qt::AlignLeft | Qt::AlignTop, m_roomName);

    if (m_locked) {
        painter->setPen(QColor("#f0a500"));
        QFont lockFont = painter->font();
        lockFont.setPointSize(10);
        painter->setFont(lockFont);
        painter->drawText(r.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, QString::fromUtf8("\xF0\x9F\x94\x92"));
    }
}

void LessonCardItem::setCardColors(const QColor &bg, const QColor &fg) {
    m_bgColor = bg;
    m_fgColor = fg;
    update();
}

void LessonCardItem::setLocked(bool locked) {
    m_locked = locked;
    update();
}

void LessonCardItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !m_locked) {
        m_dragging = true;
        setCursor(Qt::ClosedHandCursor);
        setFlags(flags() | ItemIsMovable);
        event->accept();
        return;
    }
    QGraphicsObject::mousePressEvent(event);
}

void LessonCardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (m_dragging) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        setFlags(flags() & ~ItemIsMovable);
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

QVariant LessonCardItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        return newPos;
    }
    return QGraphicsObject::itemChange(change, value);
}

QColor LessonCardItem::colorForId(int id, int hueOffset) const {
    if (id <= 0) return QColor("#2a2a2a");
    int hue = (id * 61 + hueOffset) % 360;
    return QColor::fromHsl(hue, 160, 80);
}
