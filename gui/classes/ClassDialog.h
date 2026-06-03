#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>

class ClassDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClassDialog(QWidget *parent = nullptr);
    ~ClassDialog() override = default;

    QString className() const;
    void setClassName(const QString &name);

    int studentCount() const;
    void setStudentCount(int count);

private:
    QLineEdit *nameEdit;
    QSpinBox *studentCountSpin;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
};
