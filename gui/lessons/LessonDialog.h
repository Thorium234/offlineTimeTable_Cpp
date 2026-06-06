#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>

class LessonDialog : public QDialog {
    Q_OBJECT
public:
    explicit LessonDialog(QWidget *parent = nullptr);
    
    int teacherId() const;
    int secondTeacherId() const;
    int subjectId() const;
    int classId() const;
    std::vector<int> combinedClassIds() const;
    int periodsPerWeek() const;
    int blockSize() const;
    int maxPerDay() const;
    int weekType() const;

    void setTeacherId(int id);
    void setSecondTeacherId(int id);
    void setSubjectId(int id);
    void setClassId(int id);
    void setCombinedClassIds(const std::vector<int>& ids);
    void setPeriodsPerWeek(int periods);
    void setBlockSize(int blocks);
    void setMaxPerDay(int max);
    void setWeekType(int wt);
    
    void loadFromDataManager(class DataManager *dm);

private:
    QComboBox *teacherCombo;
    QComboBox *secondTeacherCombo;
    QComboBox *subjectCombo;
    QComboBox *classCombo;
    QListWidget *combinedClassList;
    QSpinBox *periodsSpin;
    QSpinBox *blockSizeSpin;
    QSpinBox *maxPerDaySpin;
    QComboBox *weekTypeCombo;
};
