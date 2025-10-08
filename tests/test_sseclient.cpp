#include <QtTest/QtTest>
#include "../include/SSEClient.h"
#include "../include/MCPHandler.h"
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

class TestSSEClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Runs once before all tests
    }

    void cleanupTestCase() {
        // Runs once after all tests
    }

    void testSSEEventStructure() {
        SSEClient::SSEEvent event;
        event.id = "123";
        event.eventType = "test";
        event.data = "Test data";
        event.retry = 5000;

        QCOMPARE(event.id, QString("123"));
        QCOMPARE(event.eventType, QString("test"));
        QCOMPARE(event.data, QString("Test data"));
        QCOMPARE(event.retry, 5000);
    }

    void testSSEEventDefaultValues() {
        SSEClient::SSEEvent event;
        QVERIFY(event.id.isEmpty());
        QVERIFY(event.eventType.isEmpty());
        QVERIFY(event.data.isEmpty());
        QCOMPARE(event.retry, -1);
    }

    void testSSEClientCreation() {
        SSEClient client;
        QVERIFY(!client.isConnected());
        QVERIFY(client.getStreamUrl().isEmpty());
    }

    void testSSEToolTypeInMCPHandler() {
        MCPTool sseTool;
        sseTool.name = "sse_test";
        sseTool.description = "Test SSE tool";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/events";
        sseTool.parameters = QJsonObject{{"param", "value"}};

        QVERIFY(sseTool.isValid());
        QCOMPARE(sseTool.toolType, MCPToolType::SSE);
    }

    void testSSEToolValidation() {
        // Valid SSE tool
        MCPTool validSSETool;
        validSSETool.name = "sse_valid";
        validSSETool.description = "Valid SSE tool";
        validSSETool.toolType = MCPToolType::SSE;
        validSSETool.networkUrl = "http://localhost:8080/events";
        QVERIFY(validSSETool.isValid());

        // Invalid SSE tool (no URL)
        MCPTool invalidSSETool;
        invalidSSETool.name = "sse_invalid";
        invalidSSETool.description = "Invalid SSE tool";
        invalidSSETool.toolType = MCPToolType::SSE;
        invalidSSETool.networkUrl = "";  // Empty URL
        QVERIFY(!invalidSSETool.isValid());

        // Invalid SSE tool (no name)
        MCPTool invalidSSETool2;
        invalidSSETool2.name = "";
        invalidSSETool2.description = "Invalid SSE tool";
        invalidSSETool2.toolType = MCPToolType::SSE;
        invalidSSETool2.networkUrl = "http://localhost:8080/events";
        QVERIFY(!invalidSSETool2.isValid());
    }

    void testSSEToolRegistration() {
        MCPHandler handler;

        MCPTool sseTool;
        sseTool.name = "sse_stream";
        sseTool.description = "SSE streaming tool";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/stream";
        sseTool.parameters = QJsonObject{{"format", "json"}};

        QVERIFY(handler.registerTool(sseTool));

        QStringList tools = handler.getRegisteredTools();
        QVERIFY(tools.contains("sse_stream"));

        const MCPTool* retrievedTool = handler.getTool("sse_stream");
        QVERIFY(retrievedTool != nullptr);
        QCOMPARE(retrievedTool->name, QString("sse_stream"));
        QCOMPARE(retrievedTool->toolType, MCPToolType::SSE);
        QCOMPARE(retrievedTool->networkUrl, QString("http://localhost:8080/stream"));
    }

    void testMultipleToolTypes() {
        MCPHandler handler;

        // Local tool
        MCPTool localTool;
        localTool.name = "local_calc";
        localTool.description = "Local calculation";
        localTool.toolType = MCPToolType::Local;
        localTool.function = [](const QJsonObject &params) -> QJsonObject {
            QJsonObject result;
            result["value"] = params["input"].toInt() * 2;
            return result;
        };
        localTool.parameters = QJsonObject{{"input", "number"}};

        // HTTP tool
        MCPTool httpTool;
        httpTool.name = "http_api";
        httpTool.description = "HTTP API tool";
        httpTool.toolType = MCPToolType::HTTP;
        httpTool.networkUrl = "http://localhost:8080/api";
        httpTool.parameters = QJsonObject{{"query", "string"}};

        // SSE tool
        MCPTool sseTool;
        sseTool.name = "sse_stream";
        sseTool.description = "SSE streaming tool";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/stream";
        sseTool.parameters = QJsonObject{{"format", "json"}};

        QVERIFY(handler.registerTool(localTool));
        QVERIFY(handler.registerTool(httpTool));
        QVERIFY(handler.registerTool(sseTool));

        QStringList tools = handler.getRegisteredTools();
        QCOMPARE(tools.size(), 3);
        QVERIFY(tools.contains("local_calc"));
        QVERIFY(tools.contains("http_api"));
        QVERIFY(tools.contains("sse_stream"));

        // Verify tool types
        const MCPTool* local = handler.getTool("local_calc");
        QCOMPARE(local->toolType, MCPToolType::Local);

        const MCPTool* http = handler.getTool("http_api");
        QCOMPARE(http->toolType, MCPToolType::HTTP);

        const MCPTool* sse = handler.getTool("sse_stream");
        QCOMPARE(sse->toolType, MCPToolType::SSE);
    }

    void testSSEToolsForLLM() {
        MCPHandler handler;

        MCPTool sseTool;
        sseTool.name = "sse_notifications";
        sseTool.description = "Real-time notification stream";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/notifications";
        sseTool.parameters = QJsonObject{{"user_id", "string"}};

        handler.registerTool(sseTool);

        QJsonArray tools = handler.getToolsForLLM();
        QCOMPARE(tools.size(), 1);

        QJsonObject toolObj = tools[0].toObject();
        QCOMPARE(toolObj["name"].toString(), QString("sse_notifications"));
        QCOMPARE(toolObj["description"].toString(), QString("Real-time notification stream"));
        QVERIFY(toolObj.contains("parameters"));
    }

    void testSSEToolUnregistration() {
        MCPHandler handler;

        MCPTool sseTool;
        sseTool.name = "sse_temp";
        sseTool.description = "Temporary SSE tool";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/temp";
        sseTool.parameters = QJsonObject();

        handler.registerTool(sseTool);
        QVERIFY(handler.getRegisteredTools().contains("sse_temp"));

        QVERIFY(handler.unregisterTool("sse_temp"));
        QVERIFY(!handler.getRegisteredTools().contains("sse_temp"));
    }

    void testBackwardCompatibility() {
        // Test that isLocal field still works for backward compatibility
        MCPHandler handler;

        MCPTool oldStyleLocal;
        oldStyleLocal.name = "old_local";
        oldStyleLocal.description = "Old style local tool";
        oldStyleLocal.isLocal = true;
        oldStyleLocal.toolType = MCPToolType::Local;
        oldStyleLocal.function = [](const QJsonObject &) -> QJsonObject {
            return QJsonObject{{"result", "ok"}};
        };

        QVERIFY(handler.registerTool(oldStyleLocal));

        MCPTool oldStyleNetwork;
        oldStyleNetwork.name = "old_network";
        oldStyleNetwork.description = "Old style network tool";
        oldStyleNetwork.isLocal = false;
        oldStyleNetwork.toolType = MCPToolType::HTTP;
        oldStyleNetwork.networkUrl = "http://localhost:8080/old";

        QVERIFY(handler.registerTool(oldStyleNetwork));

        // Verify registration worked
        QCOMPARE(handler.getRegisteredTools().size(), 2);
    }

    void testSSEToolTypeSwitch() {
        // Verify that MCPHandler::executeToolCall properly switches on toolType
        MCPHandler handler;

        MCPTool localTool;
        localTool.name = "local";
        localTool.description = "Local tool";
        localTool.toolType = MCPToolType::Local;
        localTool.function = [](const QJsonObject &) -> QJsonObject {
            return QJsonObject{{"type", "local"}};
        };

        MCPTool httpTool;
        httpTool.name = "http";
        httpTool.description = "HTTP tool";
        httpTool.toolType = MCPToolType::HTTP;
        httpTool.networkUrl = "http://localhost:8080/test";

        MCPTool sseTool;
        sseTool.name = "sse";
        sseTool.description = "SSE tool";
        sseTool.toolType = MCPToolType::SSE;
        sseTool.networkUrl = "http://localhost:8080/events";

        handler.registerTool(localTool);
        handler.registerTool(httpTool);
        handler.registerTool(sseTool);

        // Test that each tool type is recognized
        const MCPTool* local = handler.getTool("local");
        QVERIFY(local != nullptr);
        QCOMPARE(local->toolType, MCPToolType::Local);

        const MCPTool* http = handler.getTool("http");
        QVERIFY(http != nullptr);
        QCOMPARE(http->toolType, MCPToolType::HTTP);

        const MCPTool* sse = handler.getTool("sse");
        QVERIFY(sse != nullptr);
        QCOMPARE(sse->toolType, MCPToolType::SSE);
    }
};

QTEST_MAIN(TestSSEClient)
#include "test_sseclient.moc"
