#pragma once

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include "../../services/DataManager.h"

class BenchmarkWidget : public QWidget {
    Q_OBJECT
public:
    explicit BenchmarkWidget(DataManager *dm, QWidget *parent = nullptr);

private:
    void setupUi();
    void runBenchmark();

    DataManager *dm;
    QPushButton *runBtn;
    QTextEdit *resultText;
    QLabel *statusLabel;
};
