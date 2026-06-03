#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <vector>
#include "../../models/RoomType.h"

class RoomDialog : public QDialog {
    Q_OBJECT
public:
    explicit RoomDialog(const std::vector<RoomType>& roomTypes, QWidget *parent = nullptr);
    ~RoomDialog() override = default;

    QString roomName() const;
    void setRoomName(const QString &name);

    int capacity() const;
    void setCapacity(int cap);

    int roomTypeId() const;
    void setRoomTypeId(int id);

private:
    QLineEdit *nameEdit;
    QSpinBox *capacitySpin;
    QComboBox *typeCombo;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
};
