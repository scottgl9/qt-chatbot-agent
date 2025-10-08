/**
 * DiagnosticTests.cpp - System diagnostic and testing utilities
 * 
 * Provides diagnostic tests for MCP tool calling, RAG engine, and system
 * configuration validation. Used for troubleshooting and verification.
 */

#include "DiagnosticTests.h"
#include "Config.h"
#include "Logger.h"
#include "RAGEngine.h"
#include "MCPHandler.h"
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <iostream>

// Forward declarations of example tools (defined in main.cpp)
extern QJsonObject exampleCalculatorTool(const QJsonObject &params);
extern QJsonObject exampleDateTimeTool(const QJsonObject &params);

int runRAGTest() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  RAG Pipeline Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Create RAG engine
    RAGEngine *ragEngine = new RAGEngine();

    // Test 1: Show configuration
    qInfo() << "[Test 1] RAG Engine Configuration...";
    qInfo() << "   Embedding Model:" << Config::instance().getRagEmbeddingModel();
    qInfo() << "   Chunk Size:" << Config::instance().getRagChunkSize();
    qInfo() << "   Chunk Overlap:" << Config::instance().getRagChunkOverlap();
    qInfo() << "   Top K:" << Config::instance().getRagTopK();
    qInfo() << "   RAG Enabled:" << (Config::instance().getRagEnabled() ? "yes" : "no");

    // Configure RAG engine
    ragEngine->setEmbeddingModel(Config::instance().getRagEmbeddingModel());
    ragEngine->setChunkSize(Config::instance().getRagChunkSize());
    ragEngine->setChunkOverlap(Config::instance().getRagChunkOverlap());

    // Test 2: Check for test document
    qInfo() << "\n[Test 2] Checking for test document...";
    QString testDoc = "/tmp/rag_test_document.txt";
    QFileInfo testDocInfo(testDoc);
    if (!testDocInfo.exists()) {
        qCritical() << "   ✗ Test document not found:" << testDoc;
        qInfo() << "   Please create a test document at /tmp/rag_test_document.txt";
        delete ragEngine;
        return 1;
    }
    qInfo() << "   ✓ Test document found:" << testDoc;
    qInfo() << "   Size:" << testDocInfo.size() << "bytes";

    // Test 3: Ingest document
    qInfo() << "\n[Test 3] Ingesting document...";

    // Track ingestion
    bool ingestionComplete = false;
    bool ingestionError = false;
    QString errorMessage;
    int expectedChunks = 0;
    int embeddingsGenerated = 0;

    QObject::connect(ragEngine, &RAGEngine::documentIngested,
                    [&](const QString &filePath, int chunkCount) {
        qInfo() << "   ✓ Document ingested:" << QFileInfo(filePath).fileName();
        qInfo() << "   Chunks created:" << chunkCount;
        expectedChunks = chunkCount;
        ingestionComplete = true;
    });

    QObject::connect(ragEngine, &RAGEngine::embeddingGenerated,
                    [&](int chunkIndex) {
        embeddingsGenerated++;
        if (embeddingsGenerated == 1 || embeddingsGenerated % 5 == 0 || embeddingsGenerated == expectedChunks) {
            qInfo() << "   Embedding generated for chunk" << (chunkIndex + 1) << "/" << expectedChunks;
        }
    });

    QObject::connect(ragEngine, &RAGEngine::ingestionError,
                    [&](const QString &filePath, const QString &error) {
        Q_UNUSED(filePath);
        qCritical() << "   ✗ Ingestion error:" << error;
        errorMessage = error;
        ingestionError = true;
    });

    // Start ingestion
    if (!ragEngine->ingestDocument(testDoc)) {
        qCritical() << "   ✗ Failed to start document ingestion";
        delete ragEngine;
        return 1;
    }

    // Wait for ingestion with event loop
    QEventLoop waitLoop;
    QTimer ingestionTimeout;
    ingestionTimeout.setSingleShot(true);
    ingestionTimeout.setInterval(30000); // 30 second timeout

    QObject::connect(&ingestionTimeout, &QTimer::timeout, &waitLoop, &QEventLoop::quit);

    // Check status periodically
    QTimer statusCheck;
    statusCheck.setInterval(100);
    QObject::connect(&statusCheck, &QTimer::timeout, [&]() {
        if ((ingestionComplete && embeddingsGenerated >= expectedChunks) || ingestionError) {
            waitLoop.quit();
        }
    });

    ingestionTimeout.start();
    statusCheck.start();
    waitLoop.exec();
    statusCheck.stop();

    if (ingestionTimeout.isActive()) {
        ingestionTimeout.stop();
    } else {
        qCritical() << "   ✗ Ingestion timeout";
        delete ragEngine;
        return 1;
    }

    if (ingestionError) {
        qCritical() << "   ✗ Ingestion failed:" << errorMessage;
        delete ragEngine;
        return 1;
    }

    qInfo() << "   ✓ All embeddings generated successfully";

    // Test 4: Show statistics
    qInfo() << "\n[Test 4] RAG Engine Statistics...";
    qInfo() << "   Documents:" << ragEngine->getDocumentCount();
    qInfo() << "   Chunks:" << ragEngine->getChunkCount();
    qInfo() << "   Embedding Dimension:" << ragEngine->getEmbeddingDimension();

    // Test 5: Test context retrieval
    qInfo() << "\n[Test 5] Testing context retrieval...";
    QString testQuery = "What embedding model does the RAG system use?";
    qInfo() << "   Query:" << testQuery;

    // Track query
    bool queryComplete = false;
    bool queryError = false;
    QStringList retrievedContexts;

    QObject::connect(ragEngine, &RAGEngine::contextRetrieved,
                    [&](const QStringList &contexts) {
        retrievedContexts = contexts;
        queryComplete = true;
    });

    QObject::connect(ragEngine, &RAGEngine::queryError,
                    [&](const QString &error) {
        qCritical() << "   ✗ Query error:" << error;
        queryError = true;
    });

    // Execute query
    ragEngine->retrieveContext(testQuery, Config::instance().getRagTopK());

    // Wait for query
    QEventLoop queryLoop;
    QTimer queryTimeout;
    queryTimeout.setSingleShot(true);
    queryTimeout.setInterval(10000); // 10 second timeout

    QObject::connect(&queryTimeout, &QTimer::timeout, &queryLoop, &QEventLoop::quit);

    QTimer queryStatusCheck;
    queryStatusCheck.setInterval(100);
    QObject::connect(&queryStatusCheck, &QTimer::timeout, [&]() {
        if (queryComplete || queryError) {
            queryLoop.quit();
        }
    });

    queryTimeout.start();
    queryStatusCheck.start();
    queryLoop.exec();
    queryStatusCheck.stop();

    if (queryTimeout.isActive()) {
        queryTimeout.stop();
    } else {
        qCritical() << "   ✗ Query timeout";
        delete ragEngine;
        return 1;
    }

    if (queryError) {
        delete ragEngine;
        return 1;
    }

    // Display results
    qInfo() << "   ✓ Retrieved" << retrievedContexts.size() << "relevant context chunks";

    if (!retrievedContexts.isEmpty()) {
        qInfo() << "\n[Test 6] Retrieved Context:";
        for (int i = 0; i < retrievedContexts.size(); ++i) {
            qInfo() << "\n   --- Chunk" << (i + 1) << "---";
            QString context = retrievedContexts[i];
            // Truncate long contexts for display
            if (context.length() > 200) {
                context = context.left(200) + "...";
            }
            qInfo().noquote() << "   " << context;
        }
    }

    qInfo() << "\n========================================";
    qInfo() << "  All RAG tests completed successfully!";
    qInfo() << "========================================\n";

    delete ragEngine;
    return 0;
}

int runMCPTest() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  MCP Diagnostic Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    MCPHandler handler;

    // Test 1: Register local tools
    qInfo() << "[Test 1] Registering local tools...";

    MCPTool calculatorTool;
    calculatorTool.name = "calculator";
    calculatorTool.description = "Performs basic arithmetic operations";
    calculatorTool.isLocal = true;
    calculatorTool.function = exampleCalculatorTool;
    calculatorTool.parameters = QJsonObject{
        {"operation", QJsonValue("string (add, subtract, multiply, divide)")},
        {"a", QJsonValue("number")},
        {"b", QJsonValue("number")}
    };

    bool success1 = handler.registerTool(calculatorTool);
    qInfo() << "   Calculator tool registered:" << (success1 ? "✓" : "✗");

    // Test 2: List registered tools
    qInfo() << "\n[Test 2] Listing registered tools...";
    QStringList tools = handler.getRegisteredTools();
    qInfo() << "   Registered tools:" << tools.join(", ");

    // Test 3: Direct tool function execution (bypassing handler for now)
    qInfo() << "\n[Test 3] Testing calculator tool function directly (5 + 3)...";
    QJsonObject calcParams1;
    calcParams1["operation"] = "add";
    calcParams1["a"] = 5;
    calcParams1["b"] = 3;

    QJsonObject calcResult1 = exampleCalculatorTool(calcParams1);
    qInfo() << "   Result:" << QString::fromUtf8(QJsonDocument(calcResult1).toJson(QJsonDocument::Compact));

    // Test 4: Test error case (division by zero)
    qInfo() << "\n[Test 4] Testing calculator tool error handling (10 / 0)...";
    QJsonObject calcParams2;
    calcParams2["operation"] = "divide";
    calcParams2["a"] = 10;
    calcParams2["b"] = 0;

    QJsonObject calcResult2 = exampleCalculatorTool(calcParams2);
    qInfo() << "   Result:" << QString::fromUtf8(QJsonDocument(calcResult2).toJson(QJsonDocument::Compact));

    // Test 5: Build MCP message
    qInfo() << "\n[Test 5] Building MCP message with tool context...";
    MCPMessage msg = handler.buildMessage("user", "Calculate 5 + 3", QStringList{"calculator"});
    qInfo() << "   Message:" << QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));

    // Test 6: Get tools for LLM
    qInfo() << "\n[Test 6] Getting tools list for LLM...";
    QJsonArray toolsForLLM = handler.getToolsForLLM();
    qInfo() << "   Tools:" << QString::fromUtf8(QJsonDocument(toolsForLLM).toJson(QJsonDocument::Indented));

    qInfo() << "\n========================================";
    qInfo() << "  All MCP tests completed successfully!";
    qInfo() << "========================================\n";

    return 0;
}
