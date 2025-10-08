/**
 * CLIMode.cpp - Command-line interface mode
 * 
 * Implements CLI-only mode for testing and automation, supports prompts,
 * tool calling, RAG context, and diagnostic tests without GUI.
 */

#include "CLIMode.h"
#include "Config.h"
#include "LLMClient.h"
#include "MCPHandler.h"
#include "Logger.h"
#include "DiagnosticTests.h"
#include "version.h"
#include <QCoreApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// Forward declare example tool functions from main.cpp
extern QJsonObject exampleCalculatorTool(const QJsonObject &params);
extern QJsonObject exampleDateTimeTool(const QJsonObject &params);

int runCLI(const QCommandLineParser &parser) {
    QString prompt = parser.value("prompt");
    QString context = parser.value("context");
    QString model = parser.value("model");
    QString logLevel = parser.value("log-level");

    LOG_INFO("Running in CLI mode");
    LOG_DEBUG(QString("Prompt: %1").arg(prompt));

    if (parser.isSet("mcp-test")) {
        return runMCPTest();
    }

    if (parser.isSet("rag-test")) {
        return runRAGTest();
    }

    if (parser.isSet("unit-tests")) {
        qInfo() << "Running unit tests...";
        qInfo() << "Unit tests: Not yet implemented";
        return 0;
    }

    if (!prompt.isEmpty()) {
        qInfo() << "\n=== CLI Mode - Tool Calling Test ===";
        qInfo() << "Prompt:" << prompt;

        // Create LLM client
        LLMClient *llmClient = new LLMClient();

        // Create MCP handler for tools
        MCPHandler *mcpHandler = new MCPHandler();

        // Register local tools
        MCPTool calcTool;
        calcTool.name = "calculator";
        calcTool.description = "Performs basic arithmetic operations (add, subtract, multiply, divide)";
        calcTool.isLocal = true;
        calcTool.function = exampleCalculatorTool;
        calcTool.parameters = QJsonObject{
            {"operation", QJsonValue("string: add, subtract, multiply, or divide")},
            {"a", QJsonValue("number: first operand")},
            {"b", QJsonValue("number: second operand")}
        };
        mcpHandler->registerTool(calcTool);

        MCPTool datetimeTool;
        datetimeTool.name = "datetime";
        datetimeTool.description = "Get current date and time in various formats";
        datetimeTool.isLocal = true;
        datetimeTool.function = exampleDateTimeTool;
        datetimeTool.parameters = QJsonObject{
            {"format", QJsonValue("string: 'short', 'long', 'iso', or 'timestamp' (default: long)")}
        };
        mcpHandler->registerTool(datetimeTool);

        qInfo() << "Registered" << mcpHandler->getRegisteredTools().size() << "local tools";

        // Discover and register MCP server tools (deferred until network manager is ready)
        QTimer::singleShot(100, [mcpHandler]() {
            QJsonArray mcpServers = Config::instance().getMcpServers();
            if (!mcpServers.isEmpty()) {
                LOG_INFO(QString("Found %1 configured MCP servers in CLI mode").arg(mcpServers.size()));
                for (const QJsonValue &serverVal : mcpServers) {
                    if (serverVal.isObject()) {
                        QJsonObject server = serverVal.toObject();
                        QString serverName = server["name"].toString();
                        QString serverUrl = server["url"].toString();
                        QString serverType = server["type"].toString().toLower();
                        bool enabled = server["enabled"].toBool(true);

                        if (enabled && !serverName.isEmpty() && !serverUrl.isEmpty()) {
                            int toolCount = mcpHandler->discoverAndRegisterServerTools(serverName, serverUrl, serverType);
                            if (toolCount < 0) {
                                LOG_WARNING(QString("CLI: Failed to discover tools from MCP server: %1").arg(serverName));
                            }
                        }
                    }
                }
                qInfo() << "Total registered tools:" << mcpHandler->getRegisteredTools().size();
            }
        });

        // Track response state
        bool responseReceived = false;
        QString finalResponse;
        QString streamingResponse;  // Accumulate tokens for display

        // Connect signals
        QObject::connect(llmClient, &LLMClient::tokenReceived, [&streamingResponse](const QString &token) {
            streamingResponse += token;  // Accumulate tokens instead of printing each one
        });

        QObject::connect(llmClient, &LLMClient::responseReceived, [&](const QString &response) {
            qInfo() << "\n=== Final Response ===";
            qInfo().noquote() << response;
            finalResponse = response;
            responseReceived = true;

            // Use timer to quit after ensuring all signals are processed
            QTimer::singleShot(100, []() {
                QCoreApplication::quit();
            });
        });

        QObject::connect(llmClient, &LLMClient::errorOccurred, [&](const QString &error) {
            qCritical() << "Error:" << error;
            responseReceived = true;

            QTimer::singleShot(100, []() {
                QCoreApplication::quit();
            });
        });

        QObject::connect(llmClient, &LLMClient::toolCallRequested,
                        [&](const QString &toolName, const QJsonObject &params, const QString &callId) {
            Q_UNUSED(callId);  // LLM provides a call ID but MCPHandler generates its own
            qInfo() << "\n🔧 Tool Call:" << toolName;
            qInfo() << "   Parameters:" << QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact));

            // Execute tool - returns the tool call ID generated by MCPHandler
            QString actualCallId = mcpHandler->executeToolCall(toolName, params);
            qInfo() << "   Call ID:" << actualCallId;
        });

        QObject::connect(mcpHandler, &MCPHandler::toolCallCompleted,
                        [&](const QString &callId, const QString &toolName, const QJsonObject &result) {
            qInfo() << "✓ Tool Completed:" << toolName;
            qInfo() << "   Result:" << QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));

            // Send result back to LLM
            QJsonArray toolResults;
            QJsonObject toolResult;
            toolResult["tool_name"] = toolName;
            toolResult["call_id"] = callId;
            toolResult["result"] = result;
            toolResults.append(toolResult);

            llmClient->sendToolResults(prompt, toolResults);
        });

        // Send prompt with tools after MCP discovery completes (wait for network manager + discovery)
        QTimer::singleShot(200, [&]() {
            qInfo() << "\nSending prompt to LLM with tools...";
            llmClient->sendPromptWithTools(prompt, mcpHandler->getToolsForLLM());
        });

        // Run event loop with 90 second timeout (increased for large tool sets)
        QTimer::singleShot(90000, []() {
            qWarning() << "Timeout: No response received within 90 seconds";
            QCoreApplication::quit();
        });

        QCoreApplication::exec();

        delete llmClient;
        delete mcpHandler;

        return responseReceived ? 0 : 1;
    } else {
        qInfo() << "No prompt provided. Use --prompt to specify a prompt.";
        qInfo() << "Example: qt-chatbot-agent --cli --prompt \"What time is it?\"";
    }

    return 0;
}
