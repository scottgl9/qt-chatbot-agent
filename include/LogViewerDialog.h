/**
 * LogViewerDialog.h - Real-time log viewer dialog
 * 
 * Displays application logs with filtering by level, auto-scroll,
 * export functionality, and real-time updates via Qt message handler.
 */

#ifndef LOGVIEWERDIALOG_H
#define LOGVIEWERDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QMutex>
#include <QtGlobal>

class LogViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit LogViewerDialog(QWidget *parent = nullptr);
    ~LogViewerDialog();

    // Add a log message (called from message handler)
    void addLogMessage(QtMsgType type, const QString &message);

    // Static instance access for message handler
    static LogViewerDialog* instance();
    static void setInstance(LogViewerDialog* instance);

private slots:
    void clearLogs();
    void saveLogsToFile();
    void onFilterChanged(int index);
    void onAutoScrollChanged(bool checked);

private:
    void createUI();
    void loadExistingLogs();
    QString formatLogMessage(QtMsgType type, const QString &message);
    QString getLogLevelColor(QtMsgType type);
    QString getLogLevelName(QtMsgType type);
    bool shouldShowMessage(QtMsgType type);

    QTextEdit *logTextEdit;
    QPushButton *clearButton;
    QPushButton *saveButton;
    QComboBox *filterCombo;
    QCheckBox *autoScrollCheckbox;

    QtMsgType currentFilter;
    bool autoScroll;
    bool m_isDestroying;  // Flag to prevent addLogMessage during destruction
    QMutex mutex;

    static LogViewerDialog* s_instance;
};

#endif // LOGVIEWERDIALOG_H
