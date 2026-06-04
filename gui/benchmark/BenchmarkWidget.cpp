#include "BenchmarkWidget.h"
#include "../services/Benchmark.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

BenchmarkWidget::BenchmarkWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Benchmark Suite"));
    resize(700, 500);
    setupUi();
}

void BenchmarkWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel *titleLabel = new QLabel(tr("Run Benchmark Suite"), this);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // Description
    QLabel *descLabel = new QLabel(
        tr("Run the benchmark suite to test the scheduling algorithm with various configurations.\n"
           "This will help evaluate the performance and quality of the generated timetables."), 
        this);
    descLabel->setStyleSheet("color: #888888;");
    mainLayout->addWidget(descLabel);
    
    // Run button
    runBtn = new QPushButton(tr("Run Benchmark"), this);
    runBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ff6600;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 10px 20px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #ff7711; }"
    );
    mainLayout->addWidget(runBtn);
    
    // Status label
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);
    
    // Results
    resultText = new QTextEdit(this);
    resultText->setReadOnly(true);
    resultText->setPlaceholderText(tr("Benchmark results will appear here..."));
    mainLayout->addWidget(resultText);
    
    connect(runBtn, &QPushButton::clicked, this, &BenchmarkWidget::runBenchmark);
}

void BenchmarkWidget::runBenchmark() {
    if (dm->lessons.empty()) {
        QMessageBox::warning(this, tr("Cannot Run"),
            tr("Please add at least one lesson before running the benchmark."));
        return;
    }
    
    statusLabel->setText(tr("Running benchmark... please wait."));
    statusLabel->setStyleSheet("color: #ff6600; font-weight: bold;");
    runBtn->setEnabled(false);
    resultText->clear();
    
    // Run benchmark in background - simplified for GUI
    // In a real implementation, you'd run this in a thread
    Benchmark bench;
    std::stringstream ss;
    
    ss << "=== Benchmark Suite ===\n\n";
    ss << "Running with current data:\n";
    ss << " - Teachers: " << dm->teachers.size() << "\n";
    ss << " - Subjects: " << dm->subjects.size() << "\n";
    ss << " - Classes: " << dm->classes.size() << "\n";
    ss << " - Rooms: " << dm->rooms.size() << "\n";
    ss << " - Lessons: " << dm->lessons.size() << "\n";
    ss << " - Days: " << dm->days.size() << "\n";
    ss << " - Periods: " << dm->periods.size() << "\n\n";
    
    // Run basic test
    TimetableEngine engine;
    Timetable t = engine.generate(*dm);
    
    ss << "=== Results ===\n";
    ss << "Quality Score: " << t.score << "/1000\n";
    ss << "Scheduled Lessons: " << (dm->lessons.size() - t.unscheduledLessons.size()) << "/" << dm->lessons.size() << "\n";
    ss << "Unscheduled: " << t.unscheduledLessons.size() << "\n";
    ss << "Nodes Visited: " << engine.getStatesVisited() << "\n\n";
    
    if (!t.unscheduledLessons.empty()) {
        ss << "=== Unscheduled Lessons ===\n";
        for (const auto &ul : t.unscheduledLessons) {
            ss << " - " << dm->getSubjectName(ul.subjectId) 
               << " (" << dm->getClassName(ul.classId) << ")\n";
        }
    }
    
    resultText->setPlainText(QString::fromStdString(ss.str()));
    
    statusLabel->setText(tr("Benchmark complete!"));
    statusLabel->setStyleSheet("color: #4caf50; font-weight: bold;");
    runBtn->setEnabled(true);
}
