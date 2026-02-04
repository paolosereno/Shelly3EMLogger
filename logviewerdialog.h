#ifndef LOGVIEWERDIALOG_H
#define LOGVIEWERDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>

/**
 * LogViewerDialog - Dialog for viewing and managing application logs
 *
 * Features:
 * - Real-time log viewing with color-coded levels
 * - Filter by log level (Debug+, Info+, Warning+, Error+, Critical Only)
 * - Adjustable line count (100-10000 lines)
 * - Auto-scroll to latest entries
 * - Refresh, Clear Logs, and Save As functionality
 */
class LogViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewerDialog(QWidget *parent = nullptr);
    ~LogViewerDialog();

private slots:
    /**
     * Refresh log display
     */
    void refreshLogs();

    /**
     * Clear all log files
     */
    void clearLogs();

    /**
     * Save log content to file
     */
    void saveAs();

    /**
     * Called when log level filter changes
     */
    void onFilterChanged();

    /**
     * Called when line count changes
     */
    void onLineCountChanged();

    /**
     * Auto-refresh timer timeout
     */
    void onAutoRefreshTimeout();

private:
    /**
     * Setup UI components
     */
    void setupUI();

    /**
     * Apply HTML color-coding to log text
     */
    QString applyColorCoding(const QString& logText);

    /**
     * Filter log lines by selected level
     */
    QString filterLogsByLevel(const QString& logText);

    /**
     * Get color for log level
     */
    QString getColorForLevel(const QString& level);

    // UI Components
    QTextEdit* m_logTextEdit;
    QComboBox* m_filterCombo;
    QSpinBox* m_lineCountSpinBox;
    QCheckBox* m_autoScrollCheckBox;
    QPushButton* m_refreshButton;
    QPushButton* m_clearButton;
    QPushButton* m_saveAsButton;
    QPushButton* m_closeButton;

    // Auto-refresh timer
    QTimer* m_autoRefreshTimer;

    // Current settings
    int m_lineCount;
    QString m_currentFilter;
};

#endif // LOGVIEWERDIALOG_H
