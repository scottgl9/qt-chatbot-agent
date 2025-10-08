/**
 * TestMCPStdioServer.cpp - Test MCP server for stdio protocol
 * 
 * Implements a test MCP server using stdio communication for testing
 * MCP integration. Provides example tools (hello, echo, reverse_string).
 */

#include <QCoreApplication>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>

/**
 * Simple MCP (Model Context Protocol) test server running in stdio mode
 *
 * This server implements a minimal MCP server that communicates via stdin/stdout
 * using JSON-RPC 2.0 protocol. It provides simple test tools for validation.
 *
 * Usage: ./qt-chatbot-agent --test-mcp-stdio
 */

class TestMCPStdioServer {
public:
    TestMCPStdioServer()
        : stdinStream(::stdin, QIODevice::ReadOnly)
        , stdoutStream(::stdout, QIODevice::WriteOnly) {

        // Register test tools
        registerTools();

        qDebug() << "[MCP Server] Test MCP stdio server initialized";
        qDebug() << "[MCP Server] Registered" << tools.size() << "test tools";
    }

    void run() {
        qDebug() << "[MCP Server] Starting stdio event loop...";
        qDebug() << "[MCP Server] Waiting for JSON-RPC requests on stdin";

        QString buffer;

        while (!stdinStream.atEnd()) {
            QString line = stdinStream.readLine();

            if (line.isEmpty()) {
                continue;
            }

            buffer += line;

            // Check if we have a complete JSON object
            if (isCompleteJson(buffer)) {
                processRequest(buffer.trimmed());
                buffer.clear();
            }
        }

        qDebug() << "[MCP Server] Stdio stream ended, shutting down";
    }

private:
    void registerTools() {
        // Tool 1: Hello World
        QJsonObject helloTool;
        helloTool["name"] = "hello";
        helloTool["description"] = "Returns a friendly greeting message";

        QJsonObject helloParams;
        helloParams["type"] = "object";

        QJsonObject helloProps;
        QJsonObject nameParam;
        nameParam["type"] = "string";
        nameParam["description"] = "Name to greet (optional)";
        helloProps["name"] = nameParam;

        helloParams["properties"] = helloProps;
        helloParams["required"] = QJsonArray();

        helloTool["parameters"] = helloParams;
        tools.append(helloTool);

        // Tool 2: Echo
        QJsonObject echoTool;
        echoTool["name"] = "echo";
        echoTool["description"] = "Echoes back the provided message";

        QJsonObject echoParams;
        echoParams["type"] = "object";

        QJsonObject echoProps;
        QJsonObject messageParam;
        messageParam["type"] = "string";
        messageParam["description"] = "Message to echo back";
        echoProps["message"] = messageParam;

        echoParams["properties"] = echoProps;

        QJsonArray echoRequired;
        echoRequired.append("message");
        echoParams["required"] = echoRequired;

        echoTool["parameters"] = echoParams;
        tools.append(echoTool);

        // Tool 3: Reverse String
        QJsonObject reverseTool;
        reverseTool["name"] = "reverse_string";
        reverseTool["description"] = "Reverses a string";

        QJsonObject reverseParams;
        reverseParams["type"] = "object";

        QJsonObject reverseProps;
        QJsonObject textParam;
        textParam["type"] = "string";
        textParam["description"] = "Text to reverse";
        reverseProps["text"] = textParam;

        reverseParams["properties"] = reverseProps;

        QJsonArray reverseRequired;
        reverseRequired.append("text");
        reverseParams["required"] = reverseRequired;

        reverseTool["parameters"] = reverseParams;
        tools.append(reverseTool);
    }

    bool isCompleteJson(const QString &buffer) {
        int braceCount = 0;
        bool inString = false;
        bool escaped = false;

        for (const QChar &ch : buffer) {
            if (escaped) {
                escaped = false;
                continue;
            }

            if (ch == '\\') {
                escaped = true;
                continue;
            }

            if (ch == '"') {
                inString = !inString;
                continue;
            }

            if (!inString) {
                if (ch == '{') braceCount++;
                else if (ch == '}') braceCount--;
            }
        }

        return braceCount == 0 && buffer.trimmed().startsWith('{');
    }

    void processRequest(const QString &jsonStr) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            sendError(-32700, "Parse error: " + parseError.errorString(), QJsonValue::Null);
            return;
        }

        if (!doc.isObject()) {
            sendError(-32600, "Invalid Request: not a JSON object", QJsonValue::Null);
            return;
        }

        QJsonObject request = doc.object();

        // Extract JSON-RPC fields
        QString jsonrpc = request["jsonrpc"].toString();
        QJsonValue id = request["id"];
        QString method = request["method"].toString();
        QJsonObject params = request["params"].toObject();

        qDebug() << "[MCP Server] Received request:" << method << "id:" << id;

        // Validate JSON-RPC version
        if (jsonrpc != "2.0") {
            sendError(-32600, "Invalid Request: jsonrpc must be '2.0'", id);
            return;
        }

        // Handle methods
        if (method == "tools/list") {
            handleListTools(id);
        } else if (method == "tools/call") {
            handleToolCall(id, params);
        } else if (method == "initialize") {
            handleInitialize(id);
        } else {
            sendError(-32601, "Method not found: " + method, id);
        }
    }

    void handleInitialize(const QJsonValue &id) {
        qDebug() << "[MCP Server] Handling initialize request";

        QJsonObject result;
        result["protocolVersion"] = "2024-11-05";
        result["serverInfo"] = QJsonObject{
            {"name", "test-mcp-stdio-server"},
            {"version", "1.0.0"}
        };

        QJsonObject capabilities;
        capabilities["tools"] = QJsonObject{{"listChanged", false}};
        result["capabilities"] = capabilities;

        sendResponse(result, id);
    }

    void handleListTools(const QJsonValue &id) {
        qDebug() << "[MCP Server] Handling tools/list request";

        QJsonObject result;
        result["tools"] = tools;

        sendResponse(result, id);
    }

    void handleToolCall(const QJsonValue &id, const QJsonObject &params) {
        QString toolName = params["name"].toString();
        QJsonObject arguments = params["arguments"].toObject();

        qDebug() << "[MCP Server] Handling tool call:" << toolName;
        qDebug() << "[MCP Server] Arguments:" << QJsonDocument(arguments).toJson(QJsonDocument::Compact);

        QJsonObject result;

        if (toolName == "hello") {
            QString name = arguments["name"].toString();
            if (name.isEmpty()) {
                name = "World";
            }
            result["content"] = QJsonArray{
                QJsonObject{
                    {"type", "text"},
                    {"text", QString("Hello, %1! This is a test MCP server running in stdio mode.").arg(name)}
                }
            };
        } else if (toolName == "echo") {
            QString message = arguments["message"].toString();
            result["content"] = QJsonArray{
                QJsonObject{
                    {"type", "text"},
                    {"text", message}
                }
            };
        } else if (toolName == "reverse_string") {
            QString text = arguments["text"].toString();
            QString reversed;
            for (int i = text.length() - 1; i >= 0; --i) {
                reversed.append(text[i]);
            }
            result["content"] = QJsonArray{
                QJsonObject{
                    {"type", "text"},
                    {"text", reversed}
                }
            };
        } else {
            sendError(-32602, "Unknown tool: " + toolName, id);
            return;
        }

        sendResponse(result, id);
    }

    void sendResponse(const QJsonObject &result, const QJsonValue &id) {
        QJsonObject response;
        response["jsonrpc"] = "2.0";
        response["result"] = result;
        response["id"] = id;

        QJsonDocument doc(response);
        QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        qDebug() << "[MCP Server] Sending response:" << jsonStr.left(200);

        stdoutStream << jsonStr << "\n";
        stdoutStream.flush();
    }

    void sendError(int code, const QString &message, const QJsonValue &id) {
        QJsonObject error;
        error["code"] = code;
        error["message"] = message;

        QJsonObject response;
        response["jsonrpc"] = "2.0";
        response["error"] = error;
        response["id"] = id;

        QJsonDocument doc(response);
        QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        qDebug() << "[MCP Server] Sending error:" << jsonStr;

        stdoutStream << jsonStr << "\n";
        stdoutStream.flush();
    }

    QTextStream stdinStream;
    QTextStream stdoutStream;
    QJsonArray tools;
};

int runTestMCPStdioServer() {
    qDebug() << "========================================";
    qDebug() << "Test MCP Stdio Server";
    qDebug() << "========================================";
    qDebug() << "Protocol: JSON-RPC 2.0 over stdio";
    qDebug() << "Tools: hello, echo, reverse_string";
    qDebug() << "========================================";

    TestMCPStdioServer server;
    server.run();

    return 0;
}
