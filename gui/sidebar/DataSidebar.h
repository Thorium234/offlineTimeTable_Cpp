#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include "../../services/DataManager.h"

class DataSidebar : public QWidget {
    Q_OBJECT
public:
    explicit DataSidebar(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

signals:
    void itemSelected(const QString &entityType, int id);
    void addEntityClicked(const QString &entityType);

private:
    void setupUi();
    void populateTree();
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onContextMenu(const QPoint &pos);
    void deleteEntity(const QString &type, int id);
    void openAddDialog(const QString &type);
    void openEditDialog(const QString &type, int id);

    DataManager *dm;
    QTreeWidget *tree;
    QPushButton *addTeacherBtn;
    QPushButton *addSubjectBtn;
    QPushButton *addClassBtn;
    QPushButton *addRoomBtn;

    QTreeWidgetItem *teacherRoot;
    QTreeWidgetItem *subjectRoot;
    QTreeWidgetItem *classRoot;
    QTreeWidgetItem *roomRoot;
};
