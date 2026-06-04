#include "LessonDialog.h"
#include "../services/DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

LessonDialog::LessonDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Add/Edit Lesson"));
    resize(400, 350);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Teacher
    mainLayout->addWidget(new QLabel(tr("Teacher:"), this));
    teacherCombo = new QComboBox(this);
    mainLayout->addWidget(teacherCombo);
    
    // Subject
    mainLayout->addWidget(new QLabel(tr("Subject:"), this));
    subjectCombo = new QComboBox(this);
    mainLayout->addWidget(subjectCombo);
    
    // Class
    mainLayout->addWidget(new QLabel(tr("Class:"), this));
    classCombo = new QComboBox(this);
    mainLayout->addWidget(classCombo);
    
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
    
    subjectCombo->clear();
    for (const auto &s : dm->subjects) {
        subjectCombo->addItem(QString::fromStdString(s.name), s.id);
    }
    
    classCombo->clear();
    for (const auto &c : dm->classes) {
        classCombo->addItem(QString::fromStdString(c.name), c.id);
    }
}

int LessonDialog::teacherId() const {
    return teacherCombo->currentData().toInt();
}

int LessonDialog::subjectId() const {
    return subjectCombo->currentData().toInt();
}

int LessonDialog::classId() const {
    return classCombo->currentData().toInt();
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

void LessonDialog::setTeacherId(int id) {
    for (int i = 0; i < teacherCombo->count(); ++i) {
        if (teacherCombo->itemData(i).toInt() == id) {
            teacherCombo->setCurrentIndex(i);
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

void LessonDialog::setPeriodsPerWeek(int periods) {
    periodsSpin->setValue(periods);
}

void LessonDialog::setBlockSize(int blocks) {
    blockSizeSpin->setValue(blocks);
}

void LessonDialog::setMaxPerDay(int max) {
    maxPerDaySpin->setValue(max);
}
