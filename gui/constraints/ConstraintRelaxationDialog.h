#pragma once

#include <QDialog>
#include <QTableWidget>
#include "../../models/ConstraintViolation.h"

class DataManager;

class ConstraintRelaxationDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConstraintRelaxationDialog(DataManager *dm, QWidget *parent = nullptr);

private:
    DataManager *dm;
    QTableWidget *table;
    void populate();
};
