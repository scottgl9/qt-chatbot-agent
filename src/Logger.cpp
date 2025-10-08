/**
 * Logger.cpp - Application-wide logging system
 * 
 * Provides singleton logger with multiple log levels (Debug/Info/Warning/Error),
 * file output to ~/.qtbot/qtbot.log, and Qt message handler integration.
 */

#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <iostream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_logLevel(Info)
    , m_initialized(false) {
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
}

void Logger::init(const QString &logFilePath, bool installMessageHandler) {
    QString logPath;
    bool success = false;

    {
        QMutexLocker locker(&m_mutex);

        if (m_initialized) {
            return;
        }

        logPath = logFilePath.isEmpty() ? getDefaultLogPath() : logFilePath;

        // Create log directory if it doesn't exist
        QFileInfo fileInfo(logPath);
        QDir logDir = fileInfo.dir();
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }

        // Open log file
        m_logFile.setFileName(logPath);
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_logStream.setDevice(&m_logFile);
            m_initialized = true;
            success = true;

            // Optionally install custom message handler (can cause issues if called too early)
            if (installMessageHandler) {
                qInstallMessageHandler(Logger::messageHandler);
            }
        } else {
            std::cerr << "Failed to open log file: " << logPath.toStdString() << std::endl;
        }
    }  // Mutex unlocked here

    // Write initialization message AFTER mutex is unlocked to avoid deadlock
    if (success) {
        writeLog(Info, QString("Logger initialized. Log file: %1").arg(logPath));
    }
}

void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

QString Logger::getDefaultLogPath() const {
    return QDir::homePath() + "/.qtbot/logs/qtbot.log";
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARN";
        case Error:   return "ERROR";
        default:      return "UNKNOWN";
    }
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    LogLevel level;
    switch (type) {
        case QtDebugMsg:
            level = Debug;
            break;
        case QtInfoMsg:
            level = Info;
            break;
        case QtWarningMsg:
            level = Warning;
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            level = Error;
            break;
        default:
            level = Info;
    }

    Logger::instance().writeLog(level, msg);
}

void Logger::writeLog(LogLevel level, const QString &message) {
    if (level < m_logLevel) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logLine = QString("[%1][%2] %3")
        .arg(timestamp)
        .arg(levelToString(level))
        .arg(message);

    // Write to console
    if (level >= Warning) {
        std::cerr << logLine.toStdString() << std::endl;
    } else {
        std::cout << logLine.toStdString() << std::endl;
    }

    // Write to file
    if (m_initialized && m_logFile.isOpen()) {
        m_logStream << logLine << "\n";
        m_logStream.flush();
    }
}

void Logger::log(LogLevel level, const QString &message) {
    writeLog(level, message);
}

void Logger::debug(const QString &message) {
    writeLog(Debug, message);
}

void Logger::info(const QString &message) {
    writeLog(Info, message);
}

void Logger::warning(const QString &message) {
    writeLog(Warning, message);
}

void Logger::error(const QString &message) {
    writeLog(Error, message);
}
