#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class SubjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubjectDialog(QWidget *parent = nullptr);
    ~SubjectDialog() override = default;

    QString subjectName() const;
    void setSubjectName(const QString &name);

private:
    QLineEdit *nameEdit;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
};
