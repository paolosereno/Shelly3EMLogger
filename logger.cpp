#include "logger.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>

Logger::Logger()
    : m_maxFileSize(10 * 1024 * 1024)  // 10 MB default
    , m_maxBackupFiles(5)
    , m_logLevel(Info)  // Default to Info (less verbose than Debug)
    , m_consoleOutput(true)
{
    // Default log path: AppDataLocation/logs/shelly_logger.log
    QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + "/" + DEFAULT_LOG_DIR + "/" + DEFAULT_LOG_FILE;
    initialize(defaultLogPath);
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::initialize(const QString& logFilePath, qint64 maxFileSize, int maxBackupFiles)
{
    QMutexLocker locker(&m_mutex);

    // Close existing file if open
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }

    // Set parameters
    m_logFilePath = logFilePath.isEmpty() ?
                   (QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                    + "/" + DEFAULT_LOG_DIR + "/" + DEFAULT_LOG_FILE) :
                   logFilePath;
    m_maxFileSize = maxFileSize;
    m_maxBackupFiles = maxBackupFiles;

    // Create log directory if it doesn't exist
    QFileInfo fileInfo(m_logFilePath);
    QDir logDir = fileInfo.absoluteDir();
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    // Open log file in append mode
    m_logFile.setFileName(m_logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    m_stream.setDevice(&m_logFile);

    // Write initialization message
    QString initMsg = formatMessage(Info, "Logger initialized", "Logger");
    m_stream << initMsg << "\n";
    m_stream.flush();
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

void Logger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enabled;
}

void Logger::debug(const QString& message, const QString& component)
{
    log(Debug, message, component);
}

void Logger::info(const QString& message, const QString& component)
{
    log(Info, message, component);
}

void Logger::warning(const QString& message, const QString& component)
{
    log(Warning, message, component);
}

void Logger::error(const QString& message, const QString& component)
{
    log(Error, message, component);
}

void Logger::critical(const QString& message, const QString& component)
{
    log(Critical, message, component);
}

void Logger::log(LogLevel level, const QString& message, const QString& component)
{
    QMutexLocker locker(&m_mutex);

    // Filter by log level
    if (level < m_logLevel) {
        return;
    }

    // Format message
    QString formattedMessage = formatMessage(level, message, component);

    // Write to file
    writeToFile(formattedMessage);

    // Output to console if enabled
    if (m_consoleOutput) {
        switch (level) {
            case Debug:
                qDebug().noquote() << formattedMessage;
                break;
            case Info:
                qInfo().noquote() << formattedMessage;
                break;
            case Warning:
                qWarning().noquote() << formattedMessage;
                break;
            case Error:
            case Critical:
                qCritical().noquote() << formattedMessage;
                break;
        }
    }
}

void Logger::writeToFile(const QString& formattedMessage)
{
    if (!m_logFile.isOpen()) {
        return;
    }

    // Check if rotation is needed
    if (m_logFile.size() >= m_maxFileSize) {
        rotateLogFile();
    }

    // Write to file
    m_stream << formattedMessage << "\n";
    m_stream.flush();
}

void Logger::rotateLogFile()
{
    // Close current file
    m_stream.flush();
    m_logFile.close();

    // Rotate backup files (file.log.5 -> file.log.6, ..., file.log.1 -> file.log.2)
    for (int i = m_maxBackupFiles; i > 0; --i) {
        QString oldBackup = QString("%1.%2").arg(m_logFilePath).arg(i);
        QString newBackup = QString("%1.%2").arg(m_logFilePath).arg(i + 1);

        if (i == m_maxBackupFiles) {
            // Remove oldest backup
            QFile::remove(oldBackup);
        } else {
            // Rename backup
            if (QFile::exists(oldBackup)) {
                QFile::remove(newBackup);  // Remove destination if exists
                QFile::rename(oldBackup, newBackup);
            }
        }
    }

    // Rename current log file to .1
    QString firstBackup = QString("%1.1").arg(m_logFilePath);
    QFile::remove(firstBackup);  // Remove if exists
    QFile::rename(m_logFilePath, firstBackup);

    // Reopen new log file
    m_logFile.setFileName(m_logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    m_stream.setDevice(&m_logFile);

    // Write rotation message
    QString rotationMsg = formatMessage(Info, "Log file rotated", "Logger");
    m_stream << rotationMsg << "\n";
    m_stream.flush();
}

QString Logger::formatMessage(LogLevel level, const QString& message, const QString& component)
{
    // Format: [YYYY-MM-DD HH:MM:SS.zzz] [LEVEL] [Component] Message
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = logLevelToString(level).toUpper();

    if (component.isEmpty()) {
        return QString("[%1] [%2] %3")
                .arg(timestamp)
                .arg(levelStr)
                .arg(message);
    } else {
        return QString("[%1] [%2] [%3] %4")
                .arg(timestamp)
                .arg(levelStr)
                .arg(component)
                .arg(message);
    }
}

qint64 Logger::getLogFileSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_logFile.size();
}

int Logger::clearLogs()
{
    QMutexLocker locker(&m_mutex);

    int deletedCount = 0;

    // Close current file
    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }

    // Delete current log file
    if (QFile::remove(m_logFilePath)) {
        deletedCount++;
    }

    // Delete backup files
    for (int i = 1; i <= m_maxBackupFiles + 1; ++i) {
        QString backupFile = QString("%1.%2").arg(m_logFilePath).arg(i);
        if (QFile::remove(backupFile)) {
            deletedCount++;
        }
    }

    // Reopen log file
    m_logFile.setFileName(m_logFilePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        QString clearMsg = formatMessage(Info, QString("Logs cleared (%1 files deleted)").arg(deletedCount), "Logger");
        m_stream << clearMsg << "\n";
        m_stream.flush();
    }

    return deletedCount;
}

QString Logger::readLastLines(int lineCount)
{
    QMutexLocker locker(&m_mutex);

    // Flush current buffer
    if (m_logFile.isOpen()) {
        m_stream.flush();
    }

    // Read file
    QFile readFile(m_logFilePath);
    if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: Could not open log file: %1").arg(m_logFilePath);
    }

    // Read all lines
    QStringList allLines;
    QTextStream readStream(&readFile);
    while (!readStream.atEnd()) {
        allLines.append(readStream.readLine());
    }
    readFile.close();

    // Return last N lines
    int startIndex = qMax(0, allLines.size() - lineCount);
    QStringList lastLines = allLines.mid(startIndex);

    return lastLines.join("\n");
}

QString Logger::logLevelToString(LogLevel level)
{
    switch (level) {
        case Debug:    return "DEBUG";
        case Info:     return "INFO";
        case Warning:  return "WARNING";
        case Error:    return "ERROR";
        case Critical: return "CRITICAL";
        default:       return "UNKNOWN";
    }
}

Logger::LogLevel Logger::stringToLogLevel(const QString& str)
{
    QString upper = str.toUpper();
    if (upper == "DEBUG")    return Debug;
    if (upper == "INFO")     return Info;
    if (upper == "WARNING")  return Warning;
    if (upper == "ERROR")    return Error;
    if (upper == "CRITICAL") return Critical;
    return Info;  // Default
}
