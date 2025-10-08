#include <QtTest/QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../include/MCPHandler.h"

/**
 * @brief Test MCP Server - Simulates a networked MCP tool server
 *
 * This class creates a simple HTTP server that responds to tool call requests.
 * It's used to test the MCPHandler's ability to interact with networked tools.
 */
class TestMCPServer : public QObject {
    Q_OBJECT

public:
    TestMCPServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_server(new QTcpServer(this))
        , m_port(0) {

        connect(m_server, &QTcpServer::newConnection,
                this, &TestMCPServer::handleNewConnection);
    }

    bool start() {
        // Listen on any available port
        if (!m_server->listen(QHostAddress::LocalHost, 0)) {
            qWarning() << "Failed to start test MCP server:" << m_server->errorString();
            return false;
        }
        m_port = m_server->serverPort();
        qInfo() << "Test MCP server started on port" << m_port;
        return true;
    }

    void stop() {
        m_server->close();
    }

    quint16 port() const { return m_port; }

    QString url() const {
        return QString("http://127.0.0.1:%1").arg(m_port);
    }

    // Set the response the server will send
    void setResponse(const QJsonObject &response) {
        m_nextResponse = response;
    }

    // Set the server to simulate an error
    void setError(const QString &error) {
        m_errorMessage = error;
    }

    void clearError() {
        m_errorMessage.clear();
    }

private slots:
    void handleNewConnection() {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket) return;

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            // Read the HTTP request
            QByteArray request = socket->readAll();

            // Extract the body (JSON after the headers)
            int bodyStart = request.indexOf("\r\n\r\n");
            QByteArray body;
            if (bodyStart != -1) {
                body = request.mid(bodyStart + 4);
            }

            QJsonObject response;

            if (!m_errorMessage.isEmpty()) {
                // Simulate error response
                response["error"] = m_errorMessage;
            } else if (!m_nextResponse.isEmpty()) {
                // Use preset response
                response = m_nextResponse;
            } else {
                // Default echo response
                QJsonDocument requestDoc = QJsonDocument::fromJson(body);
                QJsonObject requestObj = requestDoc.object();

                response["status"] = "success";
                response["received"] = requestObj;
                response["result"] = "Default test response";
            }

            // Build HTTP response
            QJsonDocument responseDoc(response);
            QByteArray responseBody = responseDoc.toJson(QJsonDocument::Compact);

            QString httpResponse =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + QString::number(responseBody.length()) + "\r\n"
                "Connection: close\r\n"
                "\r\n";

            socket->write(httpResponse.toUtf8());
            socket->write(responseBody);
            socket->flush();
            socket->waitForBytesWritten(1000);
            socket->disconnectFromHost();
        });

        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

private:
    QTcpServer *m_server;
    quint16 m_port;
    QJsonObject m_nextResponse;
    QString m_errorMessage;
};

/**
 * @brief Test suite for MCP Handler with networked tools
 */
class TestMCPNetworked : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup test server
        m_testServer = new TestMCPServer(this);
        QVERIFY(m_testServer->start());
        qInfo() << "Test server URL:" << m_testServer->url();

        // Process events to allow deferred initialization
        QTest::qWait(100);
    }

    void cleanupTestCase() {
        // Cleanup
        if (m_testServer) {
            m_testServer->stop();
            delete m_testServer;
            m_testServer = nullptr;
        }
    }

    void init() {
        // Before each test
        m_testServer->clearError();
    }

    void cleanup() {
        // After each test
    }

    /**
     * Test: Register a networked tool
     */
    void testNetworkedToolRegistration() {
        MCPHandler handler;

        QString toolUrl = m_testServer->url() + "/tool/search";
        QJsonObject params{{"query", "string"}};

        bool registered = handler.registerNetworkedTool(
            "search",
            "Search for information",
            params,
            toolUrl
        );

        QVERIFY(registered);
        QVERIFY(handler.getRegisteredTools().contains("search"));

        const MCPTool *tool = handler.getTool("search");
        QVERIFY(tool != nullptr);
        QCOMPARE(tool->name, QString("search"));
        QVERIFY(!tool->isLocal);
        QCOMPARE(tool->networkUrl, toolUrl);
    }

    /**
     * Test: Execute networked tool call
     */
    void testNetworkedToolExecution() {
        MCPHandler handler;

        // Wait for network manager initialization
        QTest::qWait(100);

        // Register networked tool
        QString toolUrl = m_testServer->url() + "/tool/calculate";
        handler.registerNetworkedTool(
            "calculate",
            "Perform calculation",
            QJsonObject{{"operation", "string"}, {"a", "number"}, {"b", "number"}},
            toolUrl
        );

        // Set server response
        QJsonObject expectedResult;
        expectedResult["result"] = 42;
        expectedResult["operation"] = "add";
        m_testServer->setResponse(expectedResult);

        // Setup signal spy
        QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);
        QSignalSpy failedSpy(&handler, &MCPHandler::toolCallFailed);

        // Execute tool
        QJsonObject params;
        params["operation"] = "add";
        params["a"] = 40;
        params["b"] = 2;

        QString callId = handler.executeToolCall("calculate", params);
        QVERIFY(!callId.isEmpty());

        // Wait for signal (networked call is async)
        QVERIFY(completedSpy.wait(5000));

        // Verify completed signal was emitted
        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(failedSpy.count(), 0);

        // Check signal arguments
        QList<QVariant> arguments = completedSpy.takeFirst();
        QString returnedCallId = arguments.at(0).toString();
        QString returnedToolName = arguments.at(1).toString();
        QJsonObject result = arguments.at(2).toJsonObject();

        QCOMPARE(returnedCallId, callId);
        QCOMPARE(returnedToolName, QString("calculate"));
        QVERIFY(result.contains("result"));
        QCOMPARE(result["result"].toInt(), 42);
    }

    /**
     * Test: Networked tool error handling
     */
    void testNetworkedToolError() {
        MCPHandler handler;

        // Wait for network manager initialization
        QTest::qWait(100);

        // Register networked tool
        QString toolUrl = m_testServer->url() + "/tool/failing";
        handler.registerNetworkedTool(
            "failing_tool",
            "A tool that fails",
            QJsonObject{{"param", "string"}},
            toolUrl
        );

        // Set server to return error
        m_testServer->setError("Simulated server error");

        // Setup signal spy
        QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);
        QSignalSpy failedSpy(&handler, &MCPHandler::toolCallFailed);

        // Execute tool
        QJsonObject params;
        params["param"] = "test";

        QString callId = handler.executeToolCall("failing_tool", params);
        QVERIFY(!callId.isEmpty());

        // Wait for failure signal
        QVERIFY(failedSpy.wait(5000));

        // Verify failed signal was emitted
        QCOMPARE(failedSpy.count(), 1);
        QCOMPARE(completedSpy.count(), 0);

        // Check signal arguments
        QList<QVariant> arguments = failedSpy.takeFirst();
        QString returnedCallId = arguments.at(0).toString();
        QString returnedToolName = arguments.at(1).toString();
        QString errorMessage = arguments.at(2).toString();

        QCOMPARE(returnedCallId, callId);
        QCOMPARE(returnedToolName, QString("failing_tool"));
        QVERIFY(!errorMessage.isEmpty());
    }

    /**
     * Test: Local tool execution (for comparison)
     */
    void testLocalToolExecution() {
        MCPHandler handler;

        // Wait for event loop processing
        QTest::qWait(10);

        // Register local tool
        MCPTool localTool;
        localTool.name = "local_add";
        localTool.description = "Add two numbers locally";
        localTool.isLocal = true;
        localTool.function = [](const QJsonObject &params) -> QJsonObject {
            int a = params["a"].toInt();
            int b = params["b"].toInt();
            QJsonObject result;
            result["sum"] = a + b;
            return result;
        };
        localTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        QVERIFY(handler.registerTool(localTool));

        // Setup signal spy
        QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);
        QSignalSpy failedSpy(&handler, &MCPHandler::toolCallFailed);

        // Important: Process events to ensure signal connections are established
        QCoreApplication::processEvents();

        // Execute local tool
        QJsonObject params;
        params["a"] = 10;
        params["b"] = 20;

        QString callId = handler.executeToolCall("local_add", params);
        QVERIFY(!callId.isEmpty());

        // Process events for signal emission (local tools emit synchronously)
        QCoreApplication::processEvents();

        // Check if signal was already emitted
        if (completedSpy.count() == 0) {
            // Wait if not yet emitted
            QVERIFY(completedSpy.wait(1000));
        }

        // Verify completed signal
        QCOMPARE(completedSpy.count(), 1);
        QCOMPARE(failedSpy.count(), 0);

        QList<QVariant> arguments = completedSpy.takeFirst();
        QJsonObject result = arguments.at(2).toJsonObject();

        QVERIFY(result.contains("sum"));
        QCOMPARE(result["sum"].toInt(), 30);
    }

    /**
     * Test: Multiple concurrent tool calls
     */
    void testMultipleConcurrentCalls() {
        MCPHandler handler;

        // Wait for network manager initialization
        QTest::qWait(100);

        // Register multiple networked tools
        QString toolUrl1 = m_testServer->url() + "/tool/tool1";
        QString toolUrl2 = m_testServer->url() + "/tool/tool2";

        handler.registerNetworkedTool("tool1", "Tool 1", QJsonObject{{"x", "number"}}, toolUrl1);
        handler.registerNetworkedTool("tool2", "Tool 2", QJsonObject{{"y", "number"}}, toolUrl2);

        // Setup signal spy
        QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);

        // Set server response
        QJsonObject response1;
        response1["result"] = "response1";
        m_testServer->setResponse(response1);

        // Execute multiple tools
        QJsonObject params1{{"x", 1}};
        QJsonObject params2{{"y", 2}};

        QString callId1 = handler.executeToolCall("tool1", params1);

        // Change response for second call
        QJsonObject response2;
        response2["result"] = "response2";
        m_testServer->setResponse(response2);

        QString callId2 = handler.executeToolCall("tool2", params2);

        QVERIFY(!callId1.isEmpty());
        QVERIFY(!callId2.isEmpty());
        QVERIFY(callId1 != callId2); // Call IDs should be unique

        // Wait for both to complete (may arrive in any order)
        QVERIFY(completedSpy.wait(5000));
        if (completedSpy.count() < 2) {
            QVERIFY(completedSpy.wait(5000));
        }

        // Should have 2 completions
        QCOMPARE(completedSpy.count(), 2);
    }

    /**
     * Test: Get tools for LLM includes networked tools
     */
    void testGetToolsForLLMWithNetworked() {
        MCPHandler handler;

        // Register both local and networked tools
        MCPTool localTool;
        localTool.name = "local";
        localTool.description = "Local tool";
        localTool.isLocal = true;
        localTool.function = [](const QJsonObject &) { return QJsonObject(); };
        localTool.parameters = QJsonObject{{"param", "string"}};

        handler.registerTool(localTool);

        QString toolUrl = m_testServer->url() + "/tool/networked";
        handler.registerNetworkedTool(
            "networked",
            "Networked tool",
            QJsonObject{{"param", "string"}},
            toolUrl
        );

        // Get tools for LLM
        QJsonArray tools = handler.getToolsForLLM();
        QCOMPARE(tools.size(), 2);

        // Verify both tools are in the array
        QStringList toolNames;
        for (const QJsonValue &val : tools) {
            QJsonObject toolObj = val.toObject();
            toolNames.append(toolObj["name"].toString());
        }

        QVERIFY(toolNames.contains("local"));
        QVERIFY(toolNames.contains("networked"));
    }

    /**
     * Test: Tool call with invalid tool name
     */
    void testInvalidToolName() {
        MCPHandler handler;

        // Wait for event loop processing
        QTest::qWait(10);

        QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);
        QSignalSpy failedSpy(&handler, &MCPHandler::toolCallFailed);

        // Process events to ensure signal connections
        QCoreApplication::processEvents();

        // Try to execute non-existent tool
        QString callId = handler.executeToolCall("nonexistent", QJsonObject());

        // Should still return a call ID
        QVERIFY(!callId.isEmpty());

        // Process events for signal emission
        QCoreApplication::processEvents();

        // Check if signal was already emitted
        if (failedSpy.count() == 0) {
            // Wait if not yet emitted
            QVERIFY(failedSpy.wait(1000));
        }
        QCOMPARE(failedSpy.count(), 1);
        QCOMPARE(completedSpy.count(), 0);

        QList<QVariant> arguments = failedSpy.takeFirst();
        QString errorMessage = arguments.at(2).toString();
        QVERIFY(errorMessage.contains("not found") || errorMessage.contains("not registered"));
    }

private:
    TestMCPServer *m_testServer = nullptr;
};

QTEST_MAIN(TestMCPNetworked)
#include "test_mcp_server.moc"
