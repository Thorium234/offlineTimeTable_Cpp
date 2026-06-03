#pragma once

#include <QMainWindow>

class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();
    void loadStyleSheet();

    QTabWidget *tabWidget;
};
