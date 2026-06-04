#include "PreferenceWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

PreferenceWidget::PreferenceWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Teacher Preferences"));
    resize(800, 500);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void PreferenceWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Table
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);
    
    // Selection controls
    QGridLayout *inputLayout = new QGridLayout;
    
    inputLayout->addWidget(new QLabel(tr("Teacher:"), this), 0, 0);
    teacherCombo = new QComboBox(this);
    inputLayout->addWidget(teacherCombo, 0, 1);
    
    inputLayout->addWidget(new QLabel(tr("Day:"), this), 0, 2);
    dayCombo = new QComboBox(this);
    inputLayout->addWidget(dayCombo, 0, 3);
    
    inputLayout->addWidget(new QLabel(tr("Period:"), this), 1, 0);
    periodCombo = new QComboBox(this);
    inputLayout->addWidget(periodCombo, 1, 1);
    
    inputLayout->addWidget(new QLabel(tr("Preference:"), this), 1, 2);
    typeCombo = new QComboBox(this);
    typeCombo->addItem("Preferred", static_cast<int>(PreferenceType::PREFERRED));
    typeCombo->addItem("Neutral", static_cast<int>(PreferenceType::NEUTRAL));
    typeCombo->addItem("Undesirable", static_cast<int>(PreferenceType::UNDESIRABLE));
    inputLayout->addWidget(typeCombo, 1, 3);
    
    mainLayout->addLayout(inputLayout);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add Preference"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void PreferenceWidget::setupModelAndView() {
    // Populate combos
    teacherCombo->clear();
    for (const auto &t : dm->teachers) {
        teacherCombo->addItem(QString::fromStdString(t.name), t.id);
    }
    
    dayCombo->clear();
    for (const auto &d : dm->days) {
        dayCombo->addItem(QString::fromStdString(d.name), d.id);
    }
    
    periodCombo->clear();
    for (const auto &p : dm->periods) {
        QString periodLabel = QString::fromStdString(p.startTime) + " - " + QString::fromStdString(p.endTime);
        periodCombo->addItem(periodLabel, p.id);
    }
    
    // Setup table
    model = new QStandardItemModel(this);
    model->setColumnCount(5);
    model->setHeaderData(0, Qt::Horizontal, tr("Teacher"));
    model->setHeaderData(1, Qt::Horizontal, tr("Day"));
    model->setHeaderData(2, Qt::Horizontal, tr("Period"));
    model->setHeaderData(3, Qt::Horizontal, tr("Preference"));
    model->setHeaderData(4, Qt::Horizontal, tr("ID"));
    
    for (const auto &p : dm->teacherPreferences) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::fromStdString(dm->getTeacherName(p.teacherId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getDayName(p.dayId)));
        
        QString periodLabel = QString::fromStdString(dm->getPeriodTime(p.periodId));
        rowItems << new QStandardItem(periodLabel);
        
        rowItems << new QStandardItem(QString::fromStdString(preferenceTypeToString(p.type)));
        rowItems << new QStandardItem(QString("%1:%2:%3").arg(p.teacherId).arg(p.dayId).arg(p.periodId));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(4);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void PreferenceWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &PreferenceWidget::addPreference);
    connect(delBtn, &QPushButton::clicked, this, &PreferenceWidget::deletePreference);
}

void PreferenceWidget::addPreference() {
    if (teacherCombo->count() == 0 || dayCombo->count() == 0 || periodCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Please ensure you have at least one teacher, day, and period defined."));
        return;
    }
    
    int teacherId = teacherCombo->currentData().toInt();
    int dayId = dayCombo->currentData().toInt();
    int periodId = periodCombo->currentData().toInt();
    PreferenceType type = static_cast<PreferenceType>(typeCombo->currentData().toInt());
    
    dm->addTeacherPreference(teacherId, dayId, periodId, type);
    setupModelAndView();
}

void PreferenceWidget::deletePreference() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Preference"),
            tr("Please select a preference to delete."));
        return;
    }
    
    int row = cur.row();
    QString idStr = model->item(row, 4)->text();
    QStringList parts = idStr.split(':');
    
    if (parts.size() == 3) {
        int teacherId = parts[0].toInt();
        int dayId = parts[1].toInt();
        int periodId = parts[2].toInt();
        
        if (QMessageBox::question(this, tr("Confirm Delete"),
                tr("Remove this teacher preference?")) == QMessageBox::Yes) {
            dm->teacherPreferences.erase(std::remove_if(dm->teacherPreferences.begin(), dm->teacherPreferences.end(),
                [teacherId, dayId, periodId](const TeacherPreference &p) {
                    return p.teacherId == teacherId && p.dayId == dayId && p.periodId == periodId;
                }), dm->teacherPreferences.end());
            setupModelAndView();
        }
    }
}

void PreferenceWidget::refresh() {
    setupModelAndView();
}

void PreferenceWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
