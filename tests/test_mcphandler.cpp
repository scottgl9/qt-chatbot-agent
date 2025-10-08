#include <QtTest/QtTest>
#include "../include/MCPHandler.h"
#include <QJsonObject>
#include <QJsonArray>

// Test tool function
QJsonObject testAddTool(const QJsonObject &params) {
    int a = params["a"].toInt();
    int b = params["b"].toInt();
    QJsonObject result;
    result["sum"] = a + b;
    return result;
}

QJsonObject testMultiplyTool(const QJsonObject &params) {
    int a = params["a"].toInt();
    int b = params["b"].toInt();
    QJsonObject result;
    result["product"] = a * b;
    return result;
}

class TestMCPHandler : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Runs once before all tests
    }

    void cleanupTestCase() {
        // Runs once after all tests
    }

    void testToolRegistration() {
        MCPHandler handler;

        MCPTool addTool;
        addTool.name = "add";
        addTool.description = "Adds two numbers";
        addTool.isLocal = true;
        addTool.function = testAddTool;
        addTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        QVERIFY(handler.registerTool(addTool));

        QStringList tools = handler.getRegisteredTools();
        QVERIFY(tools.contains("add"));
        QCOMPARE(tools.size(), 1);
    }

    void testMultipleToolRegistration() {
        MCPHandler handler;

        MCPTool addTool;
        addTool.name = "add";
        addTool.description = "Adds two numbers";
        addTool.isLocal = true;
        addTool.function = testAddTool;
        addTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        MCPTool multiplyTool;
        multiplyTool.name = "multiply";
        multiplyTool.description = "Multiplies two numbers";
        multiplyTool.isLocal = true;
        multiplyTool.function = testMultiplyTool;
        multiplyTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        QVERIFY(handler.registerTool(addTool));
        QVERIFY(handler.registerTool(multiplyTool));

        QStringList tools = handler.getRegisteredTools();
        QCOMPARE(tools.size(), 2);
        QVERIFY(tools.contains("add"));
        QVERIFY(tools.contains("multiply"));
    }

    void testToolUnregistration() {
        MCPHandler handler;

        MCPTool addTool;
        addTool.name = "add";
        addTool.description = "Adds two numbers";
        addTool.isLocal = true;
        addTool.function = testAddTool;
        addTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        handler.registerTool(addTool);
        QVERIFY(handler.getRegisteredTools().contains("add"));

        QVERIFY(handler.unregisterTool("add"));
        QVERIFY(!handler.getRegisteredTools().contains("add"));

        // Unregistering non-existent tool should return false
        QVERIFY(!handler.unregisterTool("nonexistent"));
    }

    void testGetTool() {
        MCPHandler handler;

        MCPTool addTool;
        addTool.name = "add";
        addTool.description = "Adds two numbers";
        addTool.isLocal = true;
        addTool.function = testAddTool;
        addTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        handler.registerTool(addTool);

        const MCPTool* tool = handler.getTool("add");
        QVERIFY(tool != nullptr);
        QCOMPARE(tool->name, QString("add"));
        QCOMPARE(tool->description, QString("Adds two numbers"));
        QVERIFY(tool->isLocal);

        const MCPTool* nonexistent = handler.getTool("nonexistent");
        QVERIFY(nonexistent == nullptr);
    }

    void testToolValidity() {
        MCPTool validTool;
        validTool.name = "test";
        validTool.description = "Test tool";
        validTool.isLocal = true;
        validTool.function = testAddTool;
        QVERIFY(validTool.isValid());

        MCPTool invalidTool1; // Missing name
        invalidTool1.description = "Test";
        invalidTool1.isLocal = true;
        invalidTool1.function = testAddTool;
        QVERIFY(!invalidTool1.isValid());

        MCPTool invalidTool2; // Missing description
        invalidTool2.name = "test";
        invalidTool2.isLocal = true;
        invalidTool2.function = testAddTool;
        QVERIFY(!invalidTool2.isValid());

        MCPTool invalidTool3; // Local but no function
        invalidTool3.name = "test";
        invalidTool3.description = "Test";
        invalidTool3.isLocal = true;
        invalidTool3.function = nullptr;
        QVERIFY(!invalidTool3.isValid());
    }

    void testGetToolsForLLM() {
        MCPHandler handler;

        MCPTool addTool;
        addTool.name = "add";
        addTool.description = "Adds two numbers";
        addTool.isLocal = true;
        addTool.function = testAddTool;
        addTool.parameters = QJsonObject{{"a", "number"}, {"b", "number"}};

        handler.registerTool(addTool);

        QJsonArray tools = handler.getToolsForLLM();
        QCOMPARE(tools.size(), 1);

        QJsonObject toolObj = tools[0].toObject();
        QCOMPARE(toolObj["name"].toString(), QString("add"));
        QCOMPARE(toolObj["description"].toString(), QString("Adds two numbers"));
        QVERIFY(toolObj.contains("parameters"));
    }

    void testBuildMessage() {
        MCPHandler handler;

        QStringList toolNames = {"tool1", "tool2"};
        MCPMessage msg = handler.buildMessage("user", "Test content", toolNames);

        QCOMPARE(msg.role, QString("user"));
        QCOMPARE(msg.content, QString("Test content"));
        QVERIFY(msg.context.contains("tools"));

        QJsonArray tools = msg.context["tools"].toArray();
        QCOMPARE(tools.size(), 2);
        QCOMPARE(tools[0].toString(), QString("tool1"));
        QCOMPARE(tools[1].toString(), QString("tool2"));
    }
};

QTEST_MAIN(TestMCPHandler)
#include "test_mcphandler.moc"
