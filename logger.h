#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QDir>

/**
 * Logger class - Singleton pattern for application-wide logging
 * Centralized logging with file rotation, levels, and thread safety
 */
class Logger
{
public:
    /**
     * Log levels (from lowest to highest priority)
     */
    enum LogLevel {
        Debug = 0,      // Detailed debugging information
        Info = 1,       // General informational messages
        Warning = 2,    // Warning messages (potential issues)
        Error = 3,      // Error messages (failures)
        Critical = 4    // Critical errors (severe failures)
    };

    /**
     * Get singleton instance
     */
    static Logger& instance();

    /**
     * Initialize logger with log file path and settings
     * @param logFilePath Path to log file (default: logs/shelly_logger.log)
     * @param maxFileSize Maximum log file size in bytes before rotation (default: 10 MB)
     * @param maxBackupFiles Number of backup files to keep (default: 5)
     */
    void initialize(const QString& logFilePath = QString(),
                   qint64 maxFileSize = 10 * 1024 * 1024,
                   int maxBackupFiles = 5);

    /**
     * Set minimum log level (messages below this level are ignored)
     * @param level Minimum log level
     */
    void setLogLevel(LogLevel level);

    /**
     * Get current log level
     */
    LogLevel getLogLevel() const { return m_logLevel; }

    /**
     * Enable or disable console output (qDebug)
     * @param enabled If true, logs are also printed to console
     */
    void setConsoleOutput(bool enabled);

    /**
     * Check if console output is enabled
     */
    bool isConsoleOutputEnabled() const { return m_consoleOutput; }

    /**
     * Log a debug message
     */
    void debug(const QString& message, const QString& component = QString());

    /**
     * Log an info message
     */
    void info(const QString& message, const QString& component = QString());

    /**
     * Log a warning message
     */
    void warning(const QString& message, const QString& component = QString());

    /**
     * Log an error message
     */
    void error(const QString& message, const QString& component = QString());

    /**
     * Log a critical error message
     */
    void critical(const QString& message, const QString& component = QString());

    /**
     * Get log file path
     */
    QString getLogFilePath() const { return m_logFilePath; }

    /**
     * Get current log file size in bytes
     */
    qint64 getLogFileSize() const;

    /**
     * Clear all log files (current + backups)
     * @return Number of files deleted
     */
    int clearLogs();

    /**
     * Read last N lines from log file
     * @param lineCount Number of lines to read (default: 1000)
     * @return Log content as string
     */
    QString readLastLines(int lineCount = 1000);

    /**
     * Convert log level to string
     */
    static QString logLevelToString(LogLevel level);

    /**
     * Convert string to log level
     */
    static LogLevel stringToLogLevel(const QString& str);

private:
    // Singleton pattern
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * Log a message with specified level
     */
    void log(LogLevel level, const QString& message, const QString& component);

    /**
     * Write message to log file
     */
    void writeToFile(const QString& formattedMessage);

    /**
     * Rotate log file if size exceeds limit
     */
    void rotateLogFile();

    /**
     * Format log message with timestamp, level, component
     */
    QString formatMessage(LogLevel level, const QString& message, const QString& component);

    // Log file settings
    QString m_logFilePath;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;

    // Current log level filter
    LogLevel m_logLevel;

    // Console output flag
    bool m_consoleOutput;

    // File handle
    QFile m_logFile;
    QTextStream m_stream;

    // Thread safety
    mutable QMutex m_mutex;

    // Default log directory
    static constexpr const char* DEFAULT_LOG_DIR = "logs";
    static constexpr const char* DEFAULT_LOG_FILE = "shelly_logger.log";
};

// Convenience macros for logging
#define LOG_DEBUG(msg) Logger::instance().debug(msg, Q_FUNC_INFO)
#define LOG_INFO(msg) Logger::instance().info(msg, Q_FUNC_INFO)
#define LOG_WARNING(msg) Logger::instance().warning(msg, Q_FUNC_INFO)
#define LOG_ERROR(msg) Logger::instance().error(msg, Q_FUNC_INFO)
#define LOG_CRITICAL(msg) Logger::instance().critical(msg, Q_FUNC_INFO)

#endif // LOGGER_H
