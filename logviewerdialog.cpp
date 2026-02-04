#include "logviewerdialog.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QScrollBar>

LogViewerDialog::LogViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_lineCount(1000)
    , m_currentFilter("All")
{
    setWindowTitle(tr("Log Viewer"));
    resize(900, 600);

    setupUI();

    // Setup auto-refresh timer (every 2 seconds)
    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &LogViewerDialog::onAutoRefreshTimeout);

    // Load logs initially
    refreshLogs();
}

LogViewerDialog::~LogViewerDialog()
{
    if (m_autoRefreshTimer->isActive()) {
        m_autoRefreshTimer->stop();
    }
}

void LogViewerDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Control panel
    QGroupBox* controlGroup = new QGroupBox(tr("Controls"));
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);

    // Log level filter
    controlLayout->addWidget(new QLabel(tr("Filter Level:")));
    m_filterCombo = new QComboBox();
    m_filterCombo->addItem(tr("All (Debug+)"), "All");
    m_filterCombo->addItem(tr("Info+"), "Info");
    m_filterCombo->addItem(tr("Warning+"), "Warning");
    m_filterCombo->addItem(tr("Error+"), "Error");
    m_filterCombo->addItem(tr("Critical Only"), "Critical");
    m_filterCombo->setCurrentIndex(0);  // Default: All
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerDialog::onFilterChanged);
    controlLayout->addWidget(m_filterCombo);

    controlLayout->addSpacing(20);

    // Line count
    controlLayout->addWidget(new QLabel(tr("Lines:")));
    m_lineCountSpinBox = new QSpinBox();
    m_lineCountSpinBox->setRange(100, 10000);
    m_lineCountSpinBox->setValue(1000);
    m_lineCountSpinBox->setSingleStep(100);
    m_lineCountSpinBox->setToolTip(tr("Number of log lines to display"));
    connect(m_lineCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &LogViewerDialog::onLineCountChanged);
    controlLayout->addWidget(m_lineCountSpinBox);

    controlLayout->addSpacing(20);

    // Auto-scroll checkbox
    m_autoScrollCheckBox = new QCheckBox(tr("Auto-scroll"));
    m_autoScrollCheckBox->setChecked(true);
    m_autoScrollCheckBox->setToolTip(tr("Automatically scroll to the end when loading logs"));
    controlLayout->addWidget(m_autoScrollCheckBox);

    controlLayout->addStretch();

    // Action buttons
    m_refreshButton = new QPushButton(tr("Refresh"));
    m_refreshButton->setToolTip(tr("Reload logs from file"));
    connect(m_refreshButton, &QPushButton::clicked, this, &LogViewerDialog::refreshLogs);
    controlLayout->addWidget(m_refreshButton);

    m_saveAsButton = new QPushButton(tr("Save As..."));
    m_saveAsButton->setToolTip(tr("Save current logs to a file"));
    connect(m_saveAsButton, &QPushButton::clicked, this, &LogViewerDialog::saveAs);
    controlLayout->addWidget(m_saveAsButton);

    m_clearButton = new QPushButton(tr("Clear Logs"));
    m_clearButton->setToolTip(tr("Delete all log files (cannot be undone)"));
    connect(m_clearButton, &QPushButton::clicked, this, &LogViewerDialog::clearLogs);
    controlLayout->addWidget(m_clearButton);

    mainLayout->addWidget(controlGroup);

    // Log text area
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_logTextEdit->setFont(QFont("Courier New", 9));
    mainLayout->addWidget(m_logTextEdit);

    // Close button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_closeButton = new QPushButton(tr("Close"));
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    mainLayout->addLayout(buttonLayout);
}

void LogViewerDialog::refreshLogs()
{
    // Read logs from Logger
    QString rawLogs = Logger::instance().readLastLines(m_lineCount);

    // Apply level filter
    QString filteredLogs = filterLogsByLevel(rawLogs);

    // Apply color coding
    QString colorizedLogs = applyColorCoding(filteredLogs);

    // Display
    m_logTextEdit->setHtml(colorizedLogs);

    // Auto-scroll to bottom if enabled
    if (m_autoScrollCheckBox->isChecked()) {
        QScrollBar* scrollBar = m_logTextEdit->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

void LogViewerDialog::clearLogs()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Clear Logs"),
        tr("Are you sure you want to delete all log files?\n\n"
           "This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        int deletedCount = Logger::instance().clearLogs();
        QMessageBox::information(this, tr("Logs Cleared"),
                                tr("Successfully deleted %1 log file(s).").arg(deletedCount));
        refreshLogs();
    }
}

void LogViewerDialog::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Logs As"),
        "shelly_logger_log.txt",
        tr("Text Files (*.txt);;Log Files (*.log);;All Files (*.*)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save log file:\n") + file.errorString());
        return;
    }

    // Save raw (unfiltered, uncolorized) logs
    QString rawLogs = Logger::instance().readLastLines(m_lineCount);
    QTextStream out(&file);
    out << rawLogs;
    file.close();

    QMessageBox::information(this, tr("Success"), tr("Logs saved successfully to:\n") + fileName);
}

void LogViewerDialog::onFilterChanged()
{
    m_currentFilter = m_filterCombo->currentData().toString();
    refreshLogs();
}

void LogViewerDialog::onLineCountChanged()
{
    m_lineCount = m_lineCountSpinBox->value();
    refreshLogs();
}

void LogViewerDialog::onAutoRefreshTimeout()
{
    refreshLogs();
}

QString LogViewerDialog::filterLogsByLevel(const QString& logText)
{
    if (m_currentFilter == "All") {
        return logText;  // No filtering
    }

    QStringList lines = logText.split('\n');
    QStringList filteredLines;

    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        // Check log level in line (format: [timestamp] [LEVEL] [component] message)
        bool shouldInclude = false;

        if (m_currentFilter == "Info") {
            shouldInclude = line.contains("[INFO]") || line.contains("[WARNING]") ||
                           line.contains("[ERROR]") || line.contains("[CRITICAL]");
        } else if (m_currentFilter == "Warning") {
            shouldInclude = line.contains("[WARNING]") || line.contains("[ERROR]") ||
                           line.contains("[CRITICAL]");
        } else if (m_currentFilter == "Error") {
            shouldInclude = line.contains("[ERROR]") || line.contains("[CRITICAL]");
        } else if (m_currentFilter == "Critical") {
            shouldInclude = line.contains("[CRITICAL]");
        }

        if (shouldInclude) {
            filteredLines.append(line);
        }
    }

    return filteredLines.join('\n');
}

QString LogViewerDialog::applyColorCoding(const QString& logText)
{
    // Convert plain text to HTML with colors for different log levels
    QString html = "<pre style='font-family: Courier New, monospace; font-size: 9pt;'>";

    QStringList lines = logText.split('\n');
    for (const QString& line : lines) {
        if (line.isEmpty()) {
            continue;
        }

        QString coloredLine = line.toHtmlEscaped();

        // Color by log level
        QString color = getColorForLevel(line);
        if (!color.isEmpty()) {
            if (line.contains("[CRITICAL]")) {
                coloredLine = QString("<span style='color: %1; font-weight: bold;'>%2</span>")
                             .arg(color).arg(coloredLine);
            } else {
                coloredLine = QString("<span style='color: %1;'>%2</span>")
                             .arg(color).arg(coloredLine);
            }
        }

        html += coloredLine + "\n";
    }

    html += "</pre>";
    return html;
}

QString LogViewerDialog::getColorForLevel(const QString& line)
{
    if (line.contains("[DEBUG]")) {
        return "#888888";  // Gray
    } else if (line.contains("[INFO]")) {
        return "#2196F3";  // Blue
    } else if (line.contains("[WARNING]")) {
        return "#FF9800";  // Orange
    } else if (line.contains("[ERROR]")) {
        return "#F44336";  // Red
    } else if (line.contains("[CRITICAL]")) {
        return "#D32F2F";  // Dark Red
    }
    return QString();  // No color
}
