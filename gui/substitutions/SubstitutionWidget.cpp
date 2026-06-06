#include "SubstitutionWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QDate>
#include <QDialog>
#include <QTableWidget>

SubstitutionWidget::SubstitutionWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Substitutions"));
    resize(900, 550);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void SubstitutionWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);

    QGridLayout *inputLayout = new QGridLayout;

    inputLayout->addWidget(new QLabel(tr("Original Teacher:"), this), 0, 0);
    origTeacherCombo = new QComboBox(this);
    inputLayout->addWidget(origTeacherCombo, 0, 1);

    inputLayout->addWidget(new QLabel(tr("Substitute Teacher:"), this), 0, 2);
    subTeacherCombo = new QComboBox(this);
    inputLayout->addWidget(subTeacherCombo, 0, 3);

    inputLayout->addWidget(new QLabel(tr("Subject:"), this), 1, 0);
    subjectCombo = new QComboBox(this);
    inputLayout->addWidget(subjectCombo, 1, 1);

    inputLayout->addWidget(new QLabel(tr("Class:"), this), 1, 2);
    classCombo = new QComboBox(this);
    inputLayout->addWidget(classCombo, 1, 3);

    inputLayout->addWidget(new QLabel(tr("Day:"), this), 2, 0);
    dayCombo = new QComboBox(this);
    inputLayout->addWidget(dayCombo, 2, 1);

    inputLayout->addWidget(new QLabel(tr("Period:"), this), 2, 2);
    periodCombo = new QComboBox(this);
    inputLayout->addWidget(periodCombo, 2, 3);

    inputLayout->addWidget(new QLabel(tr("Reason:"), this), 3, 0);
    reasonEdit = new QTextEdit(this);
    reasonEdit->setMaximumHeight(50);
    reasonEdit->setPlaceholderText(tr("Reason for substitution..."));
    inputLayout->addWidget(reasonEdit, 3, 1, 1, 3);

    mainLayout->addLayout(inputLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add Substitution Request"), this);
    approveBtn = new QPushButton(tr("Approve"), this);
    completeBtn = new QPushButton(tr("Mark Completed"), this);
    cancelBtn = new QPushButton(tr("Cancel"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    suggestBtn = new QPushButton(tr("Suggest"), this);
    suggestBtn->setToolTip(tr("Suggest best substitute teachers based on current form values"));
    btnLayout->addStretch();
    btnLayout->addWidget(suggestBtn);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(approveBtn);
    btnLayout->addWidget(completeBtn);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void SubstitutionWidget::setupModelAndView() {
    origTeacherCombo->clear();
    subTeacherCombo->clear();
    for (const auto &t : dm->teachers) {
        QString name = QString::fromStdString(t.name);
        origTeacherCombo->addItem(name, t.id);
        subTeacherCombo->addItem(name, t.id);
    }

    subjectCombo->clear();
    for (const auto &s : dm->subjects) {
        subjectCombo->addItem(QString::fromStdString(s.name), s.id);
    }

    classCombo->clear();
    for (const auto &c : dm->classes) {
        classCombo->addItem(QString::fromStdString(c.name), c.id);
    }

    dayCombo->clear();
    for (const auto &d : dm->days) {
        dayCombo->addItem(QString::fromStdString(d.name), d.id);
    }

    periodCombo->clear();
    for (const auto &p : dm->periods) {
        QString label = QString::fromStdString(p.startTime) + " - " + QString::fromStdString(p.endTime);
        periodCombo->addItem(label, p.id);
    }

    model = new QStandardItemModel(this);
    model->setColumnCount(10);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Original Teacher"));
    model->setHeaderData(2, Qt::Horizontal, tr("Substitute Teacher"));
    model->setHeaderData(3, Qt::Horizontal, tr("Subject"));
    model->setHeaderData(4, Qt::Horizontal, tr("Class"));
    model->setHeaderData(5, Qt::Horizontal, tr("Day"));
    model->setHeaderData(6, Qt::Horizontal, tr("Period"));
    model->setHeaderData(7, Qt::Horizontal, tr("Status"));
    model->setHeaderData(8, Qt::Horizontal, tr("Reason"));
    model->setHeaderData(9, Qt::Horizontal, tr("Date"));

    for (const auto &s : dm->substitutions) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(s.id));
        rowItems << new QStandardItem(QString::fromStdString(dm->getTeacherName(s.originalTeacherId)));
        QString subName = s.substituteTeacherId > 0
            ? QString::fromStdString(dm->getTeacherName(s.substituteTeacherId)) : tr("—");
        rowItems << new QStandardItem(subName);
        rowItems << new QStandardItem(QString::fromStdString(dm->getSubjectName(s.subjectId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getClassName(s.classId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getDayName(s.dayId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getPeriodTime(s.periodId)));
        rowItems << new QStandardItem(QString::fromStdString(substitutionStatusToString(s.status)));
        rowItems << new QStandardItem(QString::fromStdString(s.reason));
        rowItems << new QStandardItem(QString::fromStdString(s.date));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->hideColumn(0);
}

void SubstitutionWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &SubstitutionWidget::addSubstitution);
    connect(approveBtn, &QPushButton::clicked, this, [this]() {
        QModelIndex cur = tableView->currentIndex();
        if (cur.isValid()) updateSubstitutionStatus(cur.row());
    });
    connect(completeBtn, &QPushButton::clicked, this, [this]() {
        QModelIndex cur = tableView->currentIndex();
        if (cur.isValid()) updateSubstitutionStatus(cur.row());
    });
    connect(cancelBtn, &QPushButton::clicked, this, [this]() {
        QModelIndex cur = tableView->currentIndex();
        if (cur.isValid()) updateSubstitutionStatus(cur.row());
    });
    connect(delBtn, &QPushButton::clicked, this, &SubstitutionWidget::deleteSubstitution);
    connect(suggestBtn, &QPushButton::clicked, this, &SubstitutionWidget::suggestSubstitute);
}

void SubstitutionWidget::addSubstitution() {
    QString reason = reasonEdit->toPlainText().trimmed();
    if (origTeacherCombo->count() == 0 || subjectCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Ensure teachers, subjects, classes, days, and periods are defined."));
        return;
    }

    int origTeacherId = origTeacherCombo->currentData().toInt();
    int subTeacherId = subTeacherCombo->currentData().toInt();
    int subjectId = subjectCombo->currentData().toInt();
    int classId = classCombo->currentData().toInt();
    int dayId = dayCombo->currentData().toInt();
    int periodId = periodCombo->currentData().toInt();

    if (origTeacherId == subTeacherId) {
        QMessageBox::warning(this, tr("Invalid"),
            tr("Original and substitute teacher must be different."));
        return;
    }

    std::string dateStr = QDate::currentDate().toString(Qt::ISODate).toStdString();

    dm->addSubstitution(origTeacherId, subTeacherId, subjectId, classId,
                        dayId, periodId, reason.toStdString(), dateStr);
    reasonEdit->clear();
    setupModelAndView();
}

void SubstitutionWidget::updateSubstitutionStatus(int row) {
    if (!model->item(row, 0)) return;
    int id = model->item(row, 0)->text().toInt();
    auto *senderBtn = qobject_cast<QPushButton*>(sender());
    SubstitutionStatus newStatus;
    if (senderBtn == approveBtn)
        newStatus = SubstitutionStatus::ASSIGNED;
    else if (senderBtn == completeBtn)
        newStatus = SubstitutionStatus::COMPLETED;
    else if (senderBtn == cancelBtn)
        newStatus = SubstitutionStatus::CANCELLED;
    else
        return;

    dm->updateSubstitutionStatus(id, newStatus);
    setupModelAndView();
}

void SubstitutionWidget::deleteSubstitution() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select"), tr("Select a substitution to delete."));
        return;
    }
    int row = cur.row();
    if (!model->item(row, 0)) return;
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete substitution ID %1?").arg(id)) == QMessageBox::Yes) {
        dm->removeSubstitution(id);
        setupModelAndView();
    }
}

void SubstitutionWidget::suggestSubstitute() {
    if (origTeacherCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Suggest"),
            tr("Ensure teachers, subjects, days, and periods are defined."));
        return;
    }

    int absentId = origTeacherCombo->currentData().toInt();
    int subjectId = subjectCombo->currentData().toInt();
    int dayId = dayCombo->currentData().toInt();
    int periodId = periodCombo->currentData().toInt();

    auto suggestions = dm->suggestSubstituteTeachers(absentId, subjectId, dayId, periodId);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Cover Teacher Suggestions"));
    dlg.resize(480, 350);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);

    QLabel *hdr = new QLabel(tr("Ranked substitute teachers (higher score = better match):"), &dlg);
    hdr->setWordWrap(true);
    lay->addWidget(hdr);

    QTableWidget *tbl = new QTableWidget(&dlg);
    tbl->setColumnCount(3);
    tbl->setHorizontalHeaderLabels({tr("Teacher"), tr("Score"), tr("Reason")});
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->verticalHeader()->setVisible(false);

    tbl->setRowCount(static_cast<int>(suggestions.size()));
    for (int i = 0; i < static_cast<int>(suggestions.size()); ++i) {
        const auto &s = suggestions[i];
        tbl->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(
            dm->getTeacherName(s.teacherId))));
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(s.score));
        scoreItem->setForeground(s.score >= 80 ? QColor("#27ae60")
            : s.score >= 50 ? QColor("#f39c12") : QColor("#e74c3c"));
        tbl->setItem(i, 1, scoreItem);
        tbl->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(s.reason)));
    }

    if (suggestions.empty()) {
        tbl->setRowCount(1);
        tbl->setItem(0, 0, new QTableWidgetItem(tr("No suggestions")));
        tbl->setItem(0, 1, new QTableWidgetItem(QString()));
        tbl->setItem(0, 2, new QTableWidgetItem(tr("No suitable substitutes found.")));
    }

    lay->addWidget(tbl);

    QHBoxLayout *btnLay = new QHBoxLayout;
    btnLay->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("Close"), &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnLay->addWidget(closeBtn);
    lay->addLayout(btnLay);

    dlg.exec();
}

void SubstitutionWidget::refresh() {
    setupModelAndView();
}

void SubstitutionWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
