#include "RoomWidget.h"
#include "RoomDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardItem>

RoomWidget::RoomWidget(QWidget *parent) : QWidget(parent) {
    setWindowTitle(tr("Room Management"));
    setupUi();
    setupModelAndView();
    connectSignals();
}

void RoomWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add"), this);
    editBtn = new QPushButton(tr("Edit"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void RoomWidget::setupModelAndView() {
    model = new QStandardItemModel(this);
    model->setColumnCount(4);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Capacity"));
    model->setHeaderData(3, Qt::Horizontal, tr("Room Type"));

    const auto &rooms = dm.rooms;
    for (const auto &r : rooms) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(r.id));
        rowItems << new QStandardItem(QString::fromStdString(r.name));
        rowItems << new QStandardItem(QString::number(r.capacity));
        rowItems << new QStandardItem(QString::fromStdString(dm.getRoomTypeName(r.roomTypeId)));
        // We will store the actual roomTypeId in item's UserData if needed, but we can also map it or find it.
        rowItems[3]->setData(r.roomTypeId, Qt::UserRole);
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(0);
}

void RoomWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &RoomWidget::addRoom);
    connect(editBtn, &QPushButton::clicked, this, &RoomWidget::editRoom);
    connect(delBtn, &QPushButton::clicked, this, &RoomWidget::deleteRoom);
}

void RoomWidget::addRoom() {
    RoomDialog dlg(dm.roomTypes, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString name = dlg.roomName();
        int capacity = dlg.capacity();
        int roomTypeId = dlg.roomTypeId();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Room name cannot be empty."));
            return;
        }
        int newId = dm.addRoom(name.toStdString(), capacity, roomTypeId);
        if (newId > 0) {
            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(newId));
            rowItems << new QStandardItem(name);
            rowItems << new QStandardItem(QString::number(capacity));
            QStandardItem *typeItem = new QStandardItem(QString::fromStdString(dm.getRoomTypeName(roomTypeId)));
            typeItem->setData(roomTypeId, Qt::UserRole);
            rowItems << typeItem;
            model->appendRow(rowItems);
        } else {
            QMessageBox::warning(this, tr("Add failed"), tr("Could not insert the room."));
        }
    }
}

void RoomWidget::editRoom() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select room"), tr("Please select a room to edit."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    QString oldName = model->item(row, 1)->text();
    int oldCapacity = model->item(row, 2)->text().toInt();
    int oldRoomTypeId = model->item(row, 3)->data(Qt::UserRole).toInt();

    RoomDialog dlg(dm.roomTypes, this);
    dlg.setRoomName(oldName);
    dlg.setCapacity(oldCapacity);
    dlg.setRoomTypeId(oldRoomTypeId);

    if (dlg.exec() == QDialog::Accepted) {
        const QString newName = dlg.roomName();
        int newCapacity = dlg.capacity();
        int newRoomTypeId = dlg.roomTypeId();
        if (newName.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Room name cannot be empty."));
            return;
        }
        if (dm.updateRoom(id, newName.toStdString(), newCapacity, newRoomTypeId)) {
            model->item(row, 1)->setText(newName);
            model->item(row, 2)->setText(QString::number(newCapacity));
            model->item(row, 3)->setText(QString::fromStdString(dm.getRoomTypeName(newRoomTypeId)));
            model->item(row, 3)->setData(newRoomTypeId, Qt::UserRole);
        } else {
            QMessageBox::warning(this, tr("Update failed"), tr("Could not update the room."));
        }
    }
}

void RoomWidget::deleteRoom() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select room"), tr("Please select a room to delete."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm delete"),
            tr("Delete room with ID %1?").arg(id)) == QMessageBox::Yes) {
        if (dm.removeRoom(id)) {
            model->removeRow(row);
        } else {
            QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete the room."));
        }
    }
}
