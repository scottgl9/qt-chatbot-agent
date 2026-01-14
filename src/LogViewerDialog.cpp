/**
 * LogViewerDialog.cpp - Log viewer dialog window
 * 
 * Displays application logs in real-time, supports filtering by log level,
 * export to file, and auto-scroll functionality.
 */

#include "LogViewerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include <QMutexLocker>
#include <QScrollBar>
#include <QStandardPaths>
#include <QRegularExpression>

// Static instance
LogViewerDialog* LogViewerDialog::s_instance = nullptr;

LogViewerDialog::LogViewerDialog(QWidget *parent)
    : QDialog(parent)
    , currentFilter(QtDebugMsg)  // Show all messages by default
    , autoScroll(true)
    , m_isDestroying(false) {
    setWindowTitle(tr("Log Viewer"));
    setMinimumSize(800, 600);

    createUI();
    loadExistingLogs();
}

LogViewerDialog::~LogViewerDialog() {
    // Set destruction flag FIRST to prevent new messages
    m_isDestroying = true;

    // Clear the static instance to prevent new calls to this instance
    if (s_instance == this) {
        s_instance = nullptr;
    }

    // Lock mutex to ensure no addLogMessage is in progress
    QMutexLocker locker(&mutex);

    // Now safe to destroy (mutex will be unlocked when locker goes out of scope)
}

LogViewerDialog* LogViewerDialog::instance() {
    return s_instance;
}

void LogViewerDialog::setInstance(LogViewerDialog* instance) {
    s_instance = instance;
}

void LogViewerDialog::createUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Header with controls
    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *filterLabel = new QLabel(tr("Filter:"), this);
    headerLayout->addWidget(filterLabel);

    filterCombo = new QComboBox(this);
    filterCombo->addItem(tr("All Messages"), QtDebugMsg);
    filterCombo->addItem(tr("Info and Above"), QtInfoMsg);
    filterCombo->addItem(tr("Warnings and Above"), QtWarningMsg);
    filterCombo->addItem(tr("Errors Only"), QtCriticalMsg);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerDialog::onFilterChanged);
    headerLayout->addWidget(filterCombo);

    headerLayout->addStretch();

    autoScrollCheckbox = new QCheckBox(tr("Auto-scroll"), this);
    autoScrollCheckbox->setChecked(true);
    connect(autoScrollCheckbox, &QCheckBox::toggled,
            this, &LogViewerDialog::onAutoScrollChanged);
    headerLayout->addWidget(autoScrollCheckbox);

    clearButton = new QPushButton(tr("Clear"), this);
    connect(clearButton, &QPushButton::clicked, this, &LogViewerDialog::clearLogs);
    headerLayout->addWidget(clearButton);

    saveButton = new QPushButton(tr("Save to File..."), this);
    connect(saveButton, &QPushButton::clicked, this, &LogViewerDialog::saveLogsToFile);
    headerLayout->addWidget(saveButton);

    mainLayout->addLayout(headerLayout);

    // Log display area
    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    logTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    logTextEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "  font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "  font-size: 9pt;"
        "  border: 1px solid #3e3e3e;"
        "}"
    );
    mainLayout->addWidget(logTextEdit);

    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void LogViewerDialog::addLogMessage(QtMsgType type, const QString &message) {
    QMutexLocker locker(&mutex);

    // Check if dialog is being destroyed
    if (m_isDestroying) {
        return;
    }

    if (!shouldShowMessage(type)) {
        return;
    }

    QString formattedMessage = formatLogMessage(type, message);

    // Append to log display
    logTextEdit->append(formattedMessage);

    // Auto-scroll to bottom if enabled
    if (autoScroll) {
        QScrollBar *scrollBar = logTextEdit->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

QString LogViewerDialog::formatLogMessage(QtMsgType type, const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelName = getLogLevelName(type);
    QString color = getLogLevelColor(type);

    return QString("<span style='color: #888;'>%1</span> "
                   "<span style='color: %2; font-weight: bold;'>[%3]</span> "
                   "<span style='color: #d4d4d4;'>%4</span>")
        .arg(timestamp, color, levelName, message.toHtmlEscaped());
}

QString LogViewerDialog::getLogLevelColor(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:
            return "#808080";  // Gray
        case QtInfoMsg:
            return "#4EC9B0";  // Cyan
        case QtWarningMsg:
            return "#DCDCAA";  // Yellow
        case QtCriticalMsg:
        case QtFatalMsg:
            return "#F48771";  // Red
        default:
            return "#d4d4d4";  // White
    }
}

QString LogViewerDialog::getLogLevelName(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:
            return "DEBUG";
        case QtInfoMsg:
            return "INFO ";
        case QtWarningMsg:
            return "WARN ";
        case QtCriticalMsg:
            return "ERROR";
        case QtFatalMsg:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

bool LogViewerDialog::shouldShowMessage(QtMsgType type) {
    // Show all messages if filter is set to Debug
    if (currentFilter == QtDebugMsg) {
        return true;
    }

    // Show only messages at or above the filter level
    return type >= currentFilter;
}

void LogViewerDialog::clearLogs() {
    QMutexLocker locker(&mutex);
    logTextEdit->clear();
}

void LogViewerDialog::saveLogsToFile() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Logs"),
        QDir::homePath() + "/qtbot_logs_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt",
        tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Failed"),
            tr("Could not open file for writing: %1").arg(fileName));
        return;
    }

    QTextStream out(&file);
    out << logTextEdit->toPlainText();
    file.close();

    QMessageBox::information(this, tr("Saved"),
        tr("Logs saved to: %1").arg(fileName));
}

void LogViewerDialog::onFilterChanged(int index) {
    currentFilter = static_cast<QtMsgType>(filterCombo->itemData(index).toInt());
    // Note: This only affects future messages. To filter existing messages,
    // we would need to store all messages and re-render them.
}

void LogViewerDialog::onAutoScrollChanged(bool checked) {
    autoScroll = checked;
}

void LogViewerDialog::loadExistingLogs() {
    // Load existing logs from file
    QString logPath = QDir::homePath() + "/.qtbot/logs/qtbot.log";
    QFile logFile(logPath);

    if (!logFile.exists()) {
        logTextEdit->append(tr("<span style='color: #888;'>No log file found. Logs will appear here as they are generated.</span>"));
        return;
    }

    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logTextEdit->append(tr("<span style='color: #F48771;'>Failed to open log file: %1</span>").arg(logPath.toHtmlEscaped()));
        return;
    }

    // Read file line by line to avoid loading huge files into memory
    QTextStream in(&logFile);
    QStringList recentLines;
    const int maxLines = 1000;  // Only load last 1000 lines to prevent memory issues

    // Read all lines
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.trimmed().isEmpty()) {
            recentLines.append(line);
            // Keep only last maxLines
            if (recentLines.size() > maxLines) {
                recentLines.removeFirst();
            }
        }
    }
    logFile.close();

    // Display the logs
    if (recentLines.isEmpty()) {
        logTextEdit->append(tr("<span style='color: #888;'>Log file is empty.</span>"));
        return;
    }

    // Show truncation notice if we hit the limit
    if (recentLines.size() == maxLines) {
        logTextEdit->append(tr("<span style='color: #DCDCAA;'>Showing last %1 log entries (file may contain more)</span>").arg(maxLines));
    }

    // Parse and display logs with proper formatting
    for (const QString &line : recentLines) {
        // Parse log line format: [YYYY-MM-DD HH:MM:SS][LEVEL] message
        if (line.startsWith("[") && line.contains("][")) {
            // Extract timestamp and level to apply colors
            QRegularExpression logPattern("^\\[([^\\]]+)\\]\\[([^\\]]+)\\]\\s*(.*)$");
            QRegularExpressionMatch match = logPattern.match(line);

            if (match.hasMatch()) {
                QString timestamp = match.captured(1);
                QString level = match.captured(2).trimmed();
                QString message = match.captured(3);

                // Determine color based on level
                QString color = "#d4d4d4";  // Default
                if (level == "DEBUG") {
                    color = "#808080";  // Gray
                } else if (level == "INFO") {
                    color = "#4EC9B0";  // Cyan
                } else if (level == "WARN") {
                    color = "#DCDCAA";  // Yellow
                } else if (level == "ERROR") {
                    color = "#F48771";  // Red
                }

                QString formattedLine = QString("<span style='color: #888;'>%1</span> "
                                               "<span style='color: %2; font-weight: bold;'>[%3]</span> "
                                               "<span style='color: #d4d4d4;'>%4</span>")
                    .arg(timestamp.toHtmlEscaped(), color, level, message.toHtmlEscaped());

                logTextEdit->append(formattedLine);
            } else {
                // Fallback if format doesn't match
                logTextEdit->append(QString("<span style='color: #666;'>%1</span>").arg(line.toHtmlEscaped()));
            }
        } else if (!line.trimmed().isEmpty()) {
            // Non-standard log line
            logTextEdit->append(QString("<span style='color: #666;'>%1</span>").arg(line.toHtmlEscaped()));
        }
    }
}
