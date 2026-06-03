#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class TeacherDialog : public QDialog {
    Q_OBJECT
public:
    explicit TeacherDialog(QWidget *parent = nullptr);
    ~TeacherDialog() override = default;

    QString teacherName() const;
    void setTeacherName(const QString &name);

private:
    QLineEdit *nameEdit;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
};
