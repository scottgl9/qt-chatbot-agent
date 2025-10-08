/**
 * Logger.h - Application-wide logging system
 * 
 * Singleton logger with multiple log levels, file/console output, and
 * Qt message handler integration. Provides LOG_* macros for convenience.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QtGlobal>
#include <QDebug>

class Logger {
public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger& instance();

    void init(const QString &logFilePath = QString(), bool installMessageHandler = false);
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const { return m_logLevel; }

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void log(LogLevel level, const QString &message);
    void debug(const QString &message);
    void info(const QString &message);
    void warning(const QString &message);
    void error(const QString &message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLog(LogLevel level, const QString &message);
    QString levelToString(LogLevel level) const;
    QString getDefaultLogPath() const;

    QFile m_logFile;
    QTextStream m_logStream;
    LogLevel m_logLevel;
    QMutex m_mutex;
    bool m_initialized;
};

// Convenience macros - Using Qt's built-in logging
#define LOG_DEBUG(msg) qDebug().noquote() << msg
#define LOG_INFO(msg) qInfo().noquote() << msg
#define LOG_WARNING(msg) qWarning().noquote() << msg
#define LOG_ERROR(msg) qCritical().noquote() << msg

#endif // LOGGER_H
