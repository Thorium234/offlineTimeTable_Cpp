#include "ConstraintRelaxationDialog.h"
#include "../../services/DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>

ConstraintRelaxationDialog::ConstraintRelaxationDialog(DataManager *dataManager, QWidget *parent)
    : QDialog(parent), dm(dataManager) {
    setWindowTitle(tr("Constraint Violations & Suggestions"));
    resize(750, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *header = new QLabel(tr("The following constraints prevented full timetable generation. "
                                   "Review the suggestions below and adjust your settings."), this);
    header->setWordWrap(true);
    header->setStyleSheet("font-size:13px; color:var(--text-muted); margin-bottom:8px");
    mainLayout->addWidget(header);

    table = new QTableWidget(this);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({tr("Type"), tr("Explanation"), tr("Suggestion")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(table);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    populate();
}

void ConstraintRelaxationDialog::populate() {
    auto violations = dm->getConstraintViolations();
    table->setRowCount(static_cast<int>(violations.size()));
    for (int i = 0; i < static_cast<int>(violations.size()); ++i) {
        const auto &v = violations[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(v.type)));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(v.explanation)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(v.recommendation)));
    }
    if (violations.empty()) {
        table->setRowCount(1);
        table->setItem(0, 0, new QTableWidgetItem(tr("No violations")));
        table->setItem(0, 1, new QTableWidgetItem(tr("The timetable was generated successfully with all lessons placed.")));
        table->setItem(0, 2, new QTableWidgetItem(tr("No changes needed.")));
    }
}
