#include "RoomDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

RoomDialog::RoomDialog(const std::vector<RoomType>& roomTypes, QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Room"));
    setModal(true);

    nameEdit = new QLineEdit(this);
    capacitySpin = new QSpinBox(this);
    capacitySpin->setRange(1, 1000);
    capacitySpin->setValue(30);

    typeCombo = new QComboBox(this);
    for (const auto& rt : roomTypes) {
        typeCombo->addItem(QString::fromStdString(rt.name), rt.id);
    }

    okBtn    = new QPushButton(tr("OK"), this);
    cancelBtn= new QPushButton(tr("Cancel"), this);

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Name:"), nameEdit);
    form->addRow(tr("Capacity:"), capacitySpin);
    form->addRow(tr("Room Type:"), typeCombo);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addLayout(buttons);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString RoomDialog::roomName() const {
    return nameEdit->text().trimmed();
}

void RoomDialog::setRoomName(const QString &name) {
    nameEdit->setText(name);
}

int RoomDialog::capacity() const {
    return capacitySpin->value();
}

void RoomDialog::setCapacity(int cap) {
    capacitySpin->setValue(cap);
}

int RoomDialog::roomTypeId() const {
    return typeCombo->currentData().toInt();
}

void RoomDialog::setRoomTypeId(int id) {
    int idx = typeCombo->findData(id);
    if (idx != -1) {
        typeCombo->setCurrentIndex(idx);
    }
}
