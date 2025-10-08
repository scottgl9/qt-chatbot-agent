/**
 * main.cpp - Application entry point and built-in tool definitions
 * 
 * Handles application initialization, command-line parsing, mode selection
 * (GUI/CLI/test/server), and defines built-in MCP tools (calculator, datetime).
 */

#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QWidget>
#include <QTime>
#include <QTimer>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <QScrollBar>
#include <QScrollArea>
#include <QSet>
#include <QGroupBox>
#include <QFont>
#include <QEventLoop>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>
#include <QTextCursor>
#include <QTextDocument>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QStatusBar>
#include <QUrl>
#include <iostream>
#include <cstdlib>
#include "version.h"
#include "Logger.h"
#include "Config.h"
#include "ThemeManager.h"
#include "LLMClient.h"
#include "SettingsDialog.h"
#include "LogViewerDialog.h"
#include "MCPHandler.h"
#include "TestMCPStdioServer.h"
#include "RAGEngine.h"
#include "MarkdownHandler.h"
#include "HTMLHandler.h"
#include "DiagnosticTests.h"
#include "CLIMode.h"
#include "ConversationManager.h"
#include "MessageRenderer.h"
#include "ToolUIManager.h"
#include "RAGUIManager.h"
#include "ChatWindow.h"

// Example MCP Tools (must be defined before ChatWindow)
QJsonObject exampleCalculatorTool(const QJsonObject &params) {
    // Simple calculator tool
    QString operation = params["operation"].toString();
    double a = params["a"].toDouble();
    double b = params["b"].toDouble();
    double result = 0.0;

    if (operation == "add") {
        result = a + b;
    } else if (operation == "subtract") {
        result = a - b;
    } else if (operation == "multiply") {
        result = a * b;
    } else if (operation == "divide") {
        if (b != 0) {
            result = a / b;
        } else {
            QJsonObject error;
            error["error"] = "Division by zero";
            return error;
        }
    }

    QJsonObject response;
    response["result"] = result;
    response["operation"] = operation;
    response["a"] = a;
    response["b"] = b;
    return response;
}

QJsonObject exampleDateTimeTool(const QJsonObject &params) {
    // Time and date tool
    QString format = params["format"].toString("long");
    QJsonObject response;
    QDateTime now = QDateTime::currentDateTime();

    if (format == "short") {
        response["date"] = now.toString("yyyy-MM-dd");
        response["time"] = now.toString("HH:mm:ss");
    } else if (format == "iso") {
        response["datetime"] = now.toString(Qt::ISODate);
    } else if (format == "timestamp") {
        response["timestamp"] = now.toMSecsSinceEpoch();
    } else {
        response["date"] = now.toString("dddd, MMMM d, yyyy");
        response["time"] = now.toString("h:mm:ss AP");
        response["timezone"] = now.timeZoneAbbreviation();
    }

    return response;
}

// Custom message handler for log viewer
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // Forward to log viewer if it exists (store instance to avoid TOCTOU race condition)
    LogViewerDialog* viewer = LogViewerDialog::instance();
    if (viewer) {
        viewer->addLogMessage(type, msg);
    }

    // Also call the default Logger message handler
    Logger::messageHandler(type, context, msg);
}

int main(int argc, char *argv[]) {
    // Disable X11 MIT-SHM extension to work around Qt/X11 buffer overflow bug
    // in xcb_shm_detach_checked during widget backing store cleanup
    qputenv("QT_X11_NO_MITSHM", "1");

    // Check if CLI/server/test mode is requested (before creating QApplication)
    bool cliMode = false;
    bool serverMode = false;
    bool testMode = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString(argv[i]);
        if (arg == "--cli") {
            cliMode = true;
            break;
        }
        if (arg == "--test-mcp-stdio") {
            serverMode = true;
            break;
        }
        if (arg == "--mcp-test" || arg == "--rag-test" || arg == "--unit-tests") {
            testMode = true;
            break;
        }
    }

    // Use QCoreApplication for CLI/server/test mode, QApplication for GUI mode
    QCoreApplication *app;
    if (cliMode || serverMode || testMode) {
        app = new QCoreApplication(argc, argv);
    } else {
        app = new QApplication(argc, argv);
    }

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    // Initialize Logger FIRST (creates log file and directory if needed)
    // Install Logger's message handler so all qDebug/qInfo/qWarning/qCritical go to log file
    Logger::instance().init("", true);

    // Now we can start logging
    LOG_INFO(QString("Starting %1 v%2").arg(APP_NAME, APP_VERSION));

    // Load configuration
    if (!Config::instance().load()) {
        LOG_WARNING("Failed to load configuration, using defaults");
    }

    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(APP_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();

    // CLI options
    QCommandLineOption cliOption("cli", "Run in CLI-only mode");
    parser.addOption(cliOption);

    QCommandLineOption promptOption("prompt", "Text prompt for the model", "text");
    parser.addOption(promptOption);

    QCommandLineOption contextOption("context", "Load file or folder into RAG", "path");
    parser.addOption(contextOption);

    QCommandLineOption modelOption("model", "Override default model", "name");
    parser.addOption(modelOption);

    QCommandLineOption logLevelOption("log-level",
        "Set verbosity (debug|info|warn|error)", "level", "info");
    parser.addOption(logLevelOption);

    QCommandLineOption mcpTestOption("mcp-test",
        "Run Model Context Protocol diagnostic");
    parser.addOption(mcpTestOption);

    QCommandLineOption ragTestOption("rag-test", "Test retrieval pipeline");
    parser.addOption(ragTestOption);

    QCommandLineOption unitTestsOption("unit-tests", "Execute unit test suite");
    parser.addOption(unitTestsOption);

    QCommandLineOption testMCPStdioOption("test-mcp-stdio",
        "Run test MCP server in stdio mode (for testing MCP integration)");
    parser.addOption(testMCPStdioOption);

    // Process arguments
    parser.process(*app);

    // Set log level
    QString logLevel = parser.value(logLevelOption).toLower();
    if (logLevel == "debug") {
        Logger::instance().setLogLevel(Logger::Debug);
    } else if (logLevel == "warn") {
        Logger::instance().setLogLevel(Logger::Warning);
    } else if (logLevel == "error") {
        Logger::instance().setLogLevel(Logger::Error);
    } else {
        Logger::instance().setLogLevel(Logger::Info);
    }

    // Override config with command line options if provided
    if (parser.isSet(modelOption)) {
        QString modelName = parser.value(modelOption);
        Config::instance().setModel(modelName);
        LOG_INFO(QString("Model overridden from command line: %1").arg(modelName));
    }

    // Log current configuration
    LOG_DEBUG(QString("Backend: %1").arg(Config::instance().getBackend()));
    LOG_DEBUG(QString("Model: %1").arg(Config::instance().getModel()));
    LOG_DEBUG(QString("API URL: %1").arg(Config::instance().getApiUrl()));

    // Check if test MCP stdio server mode
    if (parser.isSet(testMCPStdioOption)) {
        LOG_INFO("Starting test MCP stdio server");
        int result = runTestMCPStdioServer();
        delete app;
        return result;
    }

    // Check if CLI mode or test modes
    if (parser.isSet(cliOption) || parser.isSet(mcpTestOption) || parser.isSet(ragTestOption) || parser.isSet(unitTestsOption)) {
        LOG_INFO("Entering CLI mode");
        int result = runCLI(parser);
        delete app;
        return result;
    }

    // GUI mode (QApplication must be used)
    LOG_INFO("Starting GUI mode");
    QApplication *guiApp = qobject_cast<QApplication*>(app);
    if (!guiApp) {
        LOG_ERROR("GUI mode requires QApplication");
        delete app;
        return 1;
    }

    // Apply default theme
    ThemeManager::instance().setTheme(ThemeManager::Light);
    ThemeManager::instance().applyTheme(guiApp);

    // Install custom message handler for log viewer
    qInstallMessageHandler(customMessageHandler);

    ChatWindow window;
    window.show();

    int result = app->exec();

    // Uninstall custom message handler before destruction to prevent
    // logging during Qt widget cleanup (which can cause X11/XCB crashes)
    qInstallMessageHandler(nullptr);

    // Ensure log viewer is destroyed SYNCHRONOUSLY before app cleanup
    // to prevent X11 resource conflicts during destruction
    LogViewerDialog* viewer = LogViewerDialog::instance();
    if (viewer) {
        LogViewerDialog::setInstance(nullptr);  // Clear instance first
        viewer->setAttribute(Qt::WA_DeleteOnClose, false);  // Prevent auto-delete
        viewer->hide();  // Hide the window
        delete viewer;   // Delete immediately and synchronously

        // Process events to ensure X11 cleanup completes
        if (guiApp) {
            guiApp->processEvents();
        }
    }

    delete app;
    return result;
}
