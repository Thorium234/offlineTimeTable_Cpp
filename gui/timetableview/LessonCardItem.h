#pragma once
#include <QGraphicsObject>
#include <QColor>

class LessonCardItem : public QGraphicsObject {
    Q_OBJECT
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    LessonCardItem(int classId, int dayIndex, int periodIndex,
                   int subjectId, int teacherId, int roomId,
                   const QString &subjectName, const QString &teacherName,
                   const QString &roomName, const QString &className,
                   bool locked, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    void setCardColors(const QColor &bg, const QColor &fg);

    int classId() const { return m_classId; }
    int dayIndex() const { return m_dayIndex; }
    int periodIndex() const { return m_periodIndex; }
    int subjectId() const { return m_subjectId; }
    int teacherId() const { return m_teacherId; }
    int roomId() const { return m_roomId; }
    bool isLocked() const { return m_locked; }

    void setDayIndex(int d) { m_dayIndex = d; }
    void setPeriodIndex(int p) { m_periodIndex = p; }
    void setLocked(bool locked);

    static constexpr qreal CARD_WIDTH = 130;
    static constexpr qreal CARD_HEIGHT = 56;

signals:
    void cardMoved(LessonCardItem *item, int fromDay, int fromPeriod,
                   int toDay, int toPeriod);
    void lockToggled(LessonCardItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QColor colorForId(int id, int hueOffset) const;

    int m_classId;
    int m_dayIndex;
    int m_periodIndex;
    int m_subjectId;
    int m_teacherId;
    int m_roomId;
    QString m_subjectName;
    QString m_teacherName;
    QString m_roomName;
    QString m_className;
    bool m_locked;
    bool m_dragging;

    QColor m_bgColor;
    QColor m_fgColor;
};
