#include "ConstraintWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

ConstraintWidget::ConstraintWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Teacher Unavailability"));
    resize(800, 500);
    setupUi();
    connectSignals();
}

void ConstraintWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // Teacher Constraints tab
    QWidget *constraintsTab = new QWidget(this);
    QVBoxLayout *constraintsLayout = new QVBoxLayout(constraintsTab);
    
    constraintsTableView = new QTableView(constraintsTab);
    constraintsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    constraintsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    constraintsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    constraintsLayout->addWidget(constraintsTableView);
    
    // Selection row
    QHBoxLayout *selectionLayout = new QHBoxLayout;
    selectionLayout->addWidget(new QLabel(tr("Teacher:"), constraintsTab));
    teacherCombo = new QComboBox(constraintsTab);
    selectionLayout->addWidget(teacherCombo);
    selectionLayout->addWidget(new QLabel(tr("Day:"), constraintsTab));
    dayCombo = new QComboBox(constraintsTab);
    selectionLayout->addWidget(dayCombo);
    selectionLayout->addWidget(new QLabel(tr("Period:"), constraintsTab));
    periodCombo = new QComboBox(constraintsTab);
    selectionLayout->addWidget(periodCombo);
    constraintsLayout->addLayout(selectionLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    addConstraintBtn = new QPushButton(tr("Mark Unavailable"), constraintsTab);
    delConstraintBtn = new QPushButton(tr("Remove"), constraintsTab);
    btnLayout->addStretch();
    btnLayout->addWidget(addConstraintBtn);
    btnLayout->addWidget(delConstraintBtn);
    constraintsLayout->addLayout(btnLayout);
    
    tabWidget->addTab(constraintsTab, tr("Teacher Unavailability"));
    
    mainLayout->addWidget(tabWidget);
    
    setupTeacherConstraintsTab();
}

void ConstraintWidget::setupTeacherConstraintsTab() {
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
    constraintsModel = new QStandardItemModel(this);
    constraintsModel->setColumnCount(4);
    constraintsModel->setHeaderData(0, Qt::Horizontal, tr("Teacher"));
    constraintsModel->setHeaderData(1, Qt::Horizontal, tr("Day"));
    constraintsModel->setHeaderData(2, Qt::Horizontal, tr("Period"));
    constraintsModel->setHeaderData(3, Qt::Horizontal, tr("ID"));
    
    for (const auto &c : dm->teacherConstraints) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::fromStdString(dm->getTeacherName(c.teacherId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getDayName(c.dayId)));
        
        QString periodLabel = QString::fromStdString(dm->getPeriodTime(c.periodId));
        rowItems << new QStandardItem(periodLabel);
        
        // Store IDs in hidden column
        rowItems << new QStandardItem(QString("%1:%2:%3").arg(c.teacherId).arg(c.dayId).arg(c.periodId));
        constraintsModel->appendRow(rowItems);
    }
    constraintsTableView->setModel(constraintsModel);
    constraintsTableView->hideColumn(3);
    constraintsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void ConstraintWidget::connectSignals() {
    connect(addConstraintBtn, &QPushButton::clicked, this, &ConstraintWidget::addTeacherConstraint);
    connect(delConstraintBtn, &QPushButton::clicked, this, &ConstraintWidget::deleteTeacherConstraint);
}

void ConstraintWidget::addTeacherConstraint() {
    if (teacherCombo->count() == 0 || dayCombo->count() == 0 || periodCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Please ensure you have at least one teacher, day, and period defined."));
        return;
    }
    
    int teacherId = teacherCombo->currentData().toInt();
    int dayId = dayCombo->currentData().toInt();
    int periodId = periodCombo->currentData().toInt();
    
    dm->addTeacherConstraint(teacherId, dayId, periodId);
    setupTeacherConstraintsTab();
}

void ConstraintWidget::deleteTeacherConstraint() {
    QModelIndex cur = constraintsTableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Constraint"),
            tr("Please select a constraint to remove."));
        return;
    }
    
    int row = cur.row();
    QString idStr = constraintsModel->item(row, 3)->text();
    QStringList parts = idStr.split(':');
    
    if (parts.size() == 3) {
        int teacherId = parts[0].toInt();
        int dayId = parts[1].toInt();
        int periodId = parts[2].toInt();
        
        if (QMessageBox::question(this, tr("Confirm Delete"),
                tr("Remove this unavailability constraint?")) == QMessageBox::Yes) {
            dm->teacherConstraints.erase(std::remove_if(dm->teacherConstraints.begin(), dm->teacherConstraints.end(),
                [teacherId, dayId, periodId](const TeacherConstraint &c) {
                    return c.teacherId == teacherId && c.dayId == dayId && c.periodId == periodId;
                }), dm->teacherConstraints.end());
            setupTeacherConstraintsTab();
        }
    }
}

void ConstraintWidget::refresh() {
    setupTeacherConstraintsTab();
}

void ConstraintWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
