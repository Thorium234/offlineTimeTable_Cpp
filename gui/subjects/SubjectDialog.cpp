#include "SubjectDialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

SubjectDialog::SubjectDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Subject"));
    setModal(true);

    nameEdit = new QLineEdit(this);
    okBtn    = new QPushButton(tr("OK"), this);
    cancelBtn= new QPushButton(tr("Cancel"), this);

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Name:"), nameEdit);

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

QString SubjectDialog::subjectName() const {
    return nameEdit->text().trimmed();
}

void SubjectDialog::setSubjectName(const QString &name) {
    nameEdit->setText(name);
}
