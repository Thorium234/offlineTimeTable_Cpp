#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>

class LessonDialog : public QDialog {
    Q_OBJECT
public:
    explicit LessonDialog(QWidget *parent = nullptr);
    
    int teacherId() const;
    int subjectId() const;
    int classId() const;
    int periodsPerWeek() const;
    int blockSize() const;
    int maxPerDay() const;
    
    void setTeacherId(int id);
    void setSubjectId(int id);
    void setClassId(int id);
    void setPeriodsPerWeek(int periods);
    void setBlockSize(int blocks);
    void setMaxPerDay(int max);
    
    void loadFromDataManager(class DataManager *dm);

private:
    QComboBox *teacherCombo;
    QComboBox *subjectCombo;
    QComboBox *classCombo;
    QSpinBox *periodsSpin;
    QSpinBox *blockSizeSpin;
    QSpinBox *maxPerDaySpin;
};
