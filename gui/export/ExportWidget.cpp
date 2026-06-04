#include "ExportWidget.h"
#include "../../services/ExportService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>

ExportWidget::ExportWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Export Timetable"));
    resize(500, 300);
    setupUi();
}

void ExportWidget::setTimetable(const Timetable &t) {
    timetable = t;
}

void ExportWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // Title
    QLabel *titleLabel = new QLabel(tr("Export Timetable to File"), this);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // Format selection
    QHBoxLayout *formatLayout = new QHBoxLayout;
    formatLayout->addWidget(new QLabel(tr("Format:"), this));
    formatCombo = new QComboBox(this);
    formatCombo->addItem("CSV", "csv");
    formatCombo->addItem("HTML", "html");
    formatLayout->addWidget(formatCombo);
    formatLayout->addStretch();
    mainLayout->addLayout(formatLayout);
    
    // Filename
    QHBoxLayout *filenameLayout = new QHBoxLayout;
    filenameLayout->addWidget(new QLabel(tr("Filename:"), this));
    filenameEdit = new QLineEdit(this);
    filenameEdit->setPlaceholderText("timetable");
    filenameLayout->addWidget(filenameEdit);
    mainLayout->addLayout(filenameLayout);
    
    // Export button
    exportBtn = new QPushButton(tr("Export"), this);
    exportBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ff6600;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 10px 20px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #ff7711; }"
    );
    mainLayout->addWidget(exportBtn);
    
    // Status label
    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("color: #888888;");
    mainLayout->addWidget(statusLabel);
    
    mainLayout->addStretch();
    
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        if (formatCombo->currentData().toString() == "csv") {
            exportToCSV();
        } else {
            exportToHTML();
        }
    });
}

void ExportWidget::exportToCSV() {
    QString filename = filenameEdit->text().trimmed();
    if (filename.isEmpty()) {
        filename = "timetable";
    }
    if (!filename.endsWith(".csv", Qt::CaseInsensitive)) {
        filename += ".csv";
    }
    
    QString filepath = QFileDialog::getSaveFileName(this, tr("Save CSV"), filename, tr("CSV Files (*.csv)"));
    if (filepath.isEmpty()) return;
    
    bool success = ExportService::exportToCSV(filepath.toStdString(), timetable, *dm);
    
    if (success) {
        statusLabel->setText(tr("Exported successfully to: %1").arg(filepath));
        statusLabel->setStyleSheet("color: #4caf50;");
        QMessageBox::information(this, tr("Export Success"), tr("Timetable exported to CSV successfully!"));
    } else {
        statusLabel->setText(tr("Export failed"));
        statusLabel->setStyleSheet("color: #f44336;");
        QMessageBox::warning(this, tr("Export Failed"), tr("Failed to export timetable."));
    }
}

void ExportWidget::exportToHTML() {
    QString filename = filenameEdit->text().trimmed();
    if (filename.isEmpty()) {
        filename = "timetable";
    }
    if (!filename.endsWith(".html", Qt::CaseInsensitive)) {
        filename += ".html";
    }
    
    QString filepath = QFileDialog::getSaveFileName(this, tr("Save HTML"), filename, tr("HTML Files (*.html)"));
    if (filepath.isEmpty()) return;
    
    bool success = ExportService::exportToHTML(filepath.toStdString(), timetable, *dm);
    
    if (success) {
        statusLabel->setText(tr("Exported successfully to: %1").arg(filepath));
        statusLabel->setStyleSheet("color: #4caf50;");
        QMessageBox::information(this, tr("Export Success"), tr("Timetable exported to HTML successfully!"));
    } else {
        statusLabel->setText(tr("Export failed"));
        statusLabel->setStyleSheet("color: #f44336;");
        QMessageBox::warning(this, tr("Export Failed"), tr("Failed to export timetable."));
    }
}

void ExportWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}

void ExportWidget::refresh() {
    if (dm->timetableGenerated) {
        setTimetable(dm->lastTimetable);
    }
    // Reset status
    statusLabel->setText("");
    filenameEdit->clear();
}
