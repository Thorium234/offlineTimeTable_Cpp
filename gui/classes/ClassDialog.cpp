#include "ClassDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

ClassDialog::ClassDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Class"));
    setModal(true);

    nameEdit = new QLineEdit(this);
    studentCountSpin = new QSpinBox(this);
    studentCountSpin->setRange(1, 1000);
    studentCountSpin->setValue(30); // Default

    okBtn    = new QPushButton(tr("OK"), this);
    cancelBtn= new QPushButton(tr("Cancel"), this);

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Name:"), nameEdit);
    form->addRow(tr("Student Count:"), studentCountSpin);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addLayout(buttons);

    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString ClassDialog::className() const {
    return nameEdit->text().trimmed();
}

void ClassDialog::setClassName(const QString &name) {
    nameEdit->setText(name);
}

int ClassDialog::studentCount() const {
    return studentCountSpin->value();
}

void ClassDialog::setStudentCount(int count) {
    studentCountSpin->setValue(count);
}
