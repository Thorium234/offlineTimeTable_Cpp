#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include "../services/DataManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void loadStyleSheet();

    QTabWidget *tabWidget;
    DataManager dm;
};
