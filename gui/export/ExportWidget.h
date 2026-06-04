#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include "../../services/DataManager.h"
#include "../../timetable/Timetable.h"

class QShowEvent;

class ExportWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExportWidget(DataManager *dm, QWidget *parent = nullptr);
    void setTimetable(const Timetable &t);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void exportToCSV();
    void exportToHTML();

    DataManager *dm;
    Timetable timetable;
    
    QLineEdit *filenameEdit;
    QComboBox *formatCombo;
    QPushButton *exportBtn;
    QLabel *statusLabel;
};
