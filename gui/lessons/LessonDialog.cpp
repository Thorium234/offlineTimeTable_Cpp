#include "LessonDialog.h"
#include "../../services/DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QGroupBox>

LessonDialog::LessonDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Add/Edit Lesson"));
    resize(450, 550);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Teacher
    mainLayout->addWidget(new QLabel(tr("Teacher:"), this));
    teacherCombo = new QComboBox(this);
    mainLayout->addWidget(teacherCombo);
    
    // Second Teacher
    mainLayout->addWidget(new QLabel(tr("Second Teacher (optional):"), this));
    secondTeacherCombo = new QComboBox(this);
    secondTeacherCombo->addItem(tr("None"), -1);
    mainLayout->addWidget(secondTeacherCombo);
    
    // Subject
    mainLayout->addWidget(new QLabel(tr("Subject:"), this));
    subjectCombo = new QComboBox(this);
    mainLayout->addWidget(subjectCombo);
    
    // Primary Class
    mainLayout->addWidget(new QLabel(tr("Class:"), this));
    classCombo = new QComboBox(this);
    mainLayout->addWidget(classCombo);
    
    // Combined Classes
    QGroupBox *combinedGroup = new QGroupBox(tr("Combined Classes (select additional classes)"), this);
    QVBoxLayout *combinedLayout = new QVBoxLayout(combinedGroup);
    combinedClassList = new QListWidget(this);
    combinedClassList->setSelectionMode(QAbstractItemView::MultiSelection);
    combinedLayout->addWidget(combinedClassList);
    mainLayout->addWidget(combinedGroup);
    
    // Periods per week
    mainLayout->addWidget(new QLabel(tr("Periods per Week:"), this));
    periodsSpin = new QSpinBox(this);
    periodsSpin->setRange(1, 20);
    periodsSpin->setValue(1);
    mainLayout->addWidget(periodsSpin);
    
    // Block size
    mainLayout->addWidget(new QLabel(tr("Block Size (1=single, 2=double, etc.):"), this));
    blockSizeSpin = new QSpinBox(this);
    blockSizeSpin->setRange(1, 10);
    blockSizeSpin->setValue(1);
    mainLayout->addWidget(blockSizeSpin);
    
    // Max per day
    mainLayout->addWidget(new QLabel(tr("Max Periods per Day (0=no limit):"), this));
    maxPerDaySpin = new QSpinBox(this);
    maxPerDaySpin->setRange(0, 10);
    maxPerDaySpin->setValue(0);
    mainLayout->addWidget(maxPerDaySpin);

    // Week type
    mainLayout->addWidget(new QLabel(tr("Week Type:"), this));
    weekTypeCombo = new QComboBox(this);
    weekTypeCombo->addItem(tr("Every Week"), 0);
    weekTypeCombo->addItem(tr("Week A Only"), 1);
    weekTypeCombo->addItem(tr("Week B Only"), 2);
    mainLayout->addWidget(weekTypeCombo);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton(tr("OK"), this);
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void LessonDialog::loadFromDataManager(DataManager *dm) {
    teacherCombo->clear();
    for (const auto &t : dm->teachers) {
        teacherCombo->addItem(QString::fromStdString(t.name), t.id);
    }
    
    secondTeacherCombo->clear();
    secondTeacherCombo->addItem(tr("None"), -1);
    for (const auto &t : dm->teachers) {
        secondTeacherCombo->addItem(QString::fromStdString(t.name), t.id);
    }
    
    subjectCombo->clear();
    for (const auto &s : dm->subjects) {
        subjectCombo->addItem(QString::fromStdString(s.name), s.id);
    }
    
    classCombo->clear();
    for (const auto &c : dm->classes) {
        classCombo->addItem(QString::fromStdString(c.name), c.id);
    }
    
    combinedClassList->clear();
    for (const auto &c : dm->classes) {
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(c.name));
        item->setData(Qt::UserRole, c.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        combinedClassList->addItem(item);
    }
}

int LessonDialog::teacherId() const {
    return teacherCombo->currentData().toInt();
}

int LessonDialog::secondTeacherId() const {
    return secondTeacherCombo->currentData().toInt();
}

int LessonDialog::subjectId() const {
    return subjectCombo->currentData().toInt();
}

int LessonDialog::classId() const {
    return classCombo->currentData().toInt();
}

std::vector<int> LessonDialog::combinedClassIds() const {
    std::vector<int> ids;
    for (int i = 0; i < combinedClassList->count(); ++i) {
        QListWidgetItem *item = combinedClassList->item(i);
        if (item->checkState() == Qt::Checked) {
            ids.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    return ids;
}

int LessonDialog::periodsPerWeek() const {
    return periodsSpin->value();
}

int LessonDialog::blockSize() const {
    return blockSizeSpin->value();
}

int LessonDialog::maxPerDay() const {
    return maxPerDaySpin->value();
}

int LessonDialog::weekType() const {
    return weekTypeCombo->currentData().toInt();
}

void LessonDialog::setTeacherId(int id) {
    for (int i = 0; i < teacherCombo->count(); ++i) {
        if (teacherCombo->itemData(i).toInt() == id) {
            teacherCombo->setCurrentIndex(i);
            break;
        }
    }
}

void LessonDialog::setSecondTeacherId(int id) {
    for (int i = 0; i < secondTeacherCombo->count(); ++i) {
        if (secondTeacherCombo->itemData(i).toInt() == id) {
            secondTeacherCombo->setCurrentIndex(i);
            break;
        }
    }
}

void LessonDialog::setSubjectId(int id) {
    for (int i = 0; i < subjectCombo->count(); ++i) {
        if (subjectCombo->itemData(i).toInt() == id) {
            subjectCombo->setCurrentIndex(i);
            break;
        }
    }
}

void LessonDialog::setClassId(int id) {
    for (int i = 0; i < classCombo->count(); ++i) {
        if (classCombo->itemData(i).toInt() == id) {
            classCombo->setCurrentIndex(i);
            break;
        }
    }
}

void LessonDialog::setCombinedClassIds(const std::vector<int>& ids) {
    for (int i = 0; i < combinedClassList->count(); ++i) {
        QListWidgetItem *item = combinedClassList->item(i);
        int cid = item->data(Qt::UserRole).toInt();
        bool checked = false;
        for (int id : ids) {
            if (id == cid) { checked = true; break; }
        }
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}

void LessonDialog::setPeriodsPerWeek(int periods) {
    periodsSpin->setValue(periods);
}

void LessonDialog::setBlockSize(int blocks) {
    blockSizeSpin->setValue(blocks);
}

void LessonDialog::setMaxPerDay(int max) {
    maxPerDaySpin->setValue(max);
}

void LessonDialog::setWeekType(int wt) {
    for (int i = 0; i < weekTypeCombo->count(); ++i) {
        if (weekTypeCombo->itemData(i).toInt() == wt) {
            weekTypeCombo->setCurrentIndex(i);
            break;
        }
    }
}
