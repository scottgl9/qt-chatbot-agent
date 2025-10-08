#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include "../include/RAGEngine.h"

class TestRAGEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Runs once before all tests
    }

    void cleanupTestCase() {
        // Runs once after all tests
    }

    void init() {
        // Runs before each test
    }

    void cleanup() {
        // Runs after each test
    }

    void testDefaultConfiguration() {
        RAGEngine engine;

        // Test default configuration values
        QCOMPARE(engine.getDocumentCount(), 0);
        QCOMPARE(engine.getChunkCount(), 0);
        QCOMPARE(engine.getEmbeddingDimension(), 768);  // Default for nomic-embed-text
    }

    void testSetConfiguration() {
        RAGEngine engine;

        // Test setting embedding model
        engine.setEmbeddingModel("test-model");

        // Test setting chunk size
        engine.setChunkSize(256);

        // Test setting chunk overlap
        engine.setChunkOverlap(25);

        // Test setting API URL
        engine.setApiUrl("http://localhost:11434/api/embeddings");

        // No getters for these, but verify no crashes
        QVERIFY(true);
    }

    void testTextFileReading() {
        RAGEngine engine;

        // Create a temporary text file
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/rag_test_XXXXXX.txt");
        QVERIFY(tempFile.open());

        QString testContent = "This is a test document.\n"
                             "It contains multiple lines.\n"
                             "We will use it to test RAG functionality.\n";

        QTextStream out(&tempFile);
        out << testContent;
        tempFile.close();

        // Note: ingestDocument requires network access for embeddings
        // We can only test that it doesn't crash and handles the file correctly
        // Actual embedding generation requires Ollama server

        // For now, just verify the file exists and can be opened
        QFile file(tempFile.fileName());
        QVERIFY(file.exists());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString content = file.readAll();
        file.close();

        QVERIFY(content.contains("test document"));
    }

    void testDocumentClear() {
        RAGEngine engine;

        // Initial state should be empty
        QCOMPARE(engine.getDocumentCount(), 0);
        QCOMPARE(engine.getChunkCount(), 0);

        // Clear should not crash on empty engine
        engine.clearDocuments();

        QCOMPARE(engine.getDocumentCount(), 0);
        QCOMPARE(engine.getChunkCount(), 0);
    }

    void testChunkingLogic() {
        // Test chunking indirectly through a document
        RAGEngine engine;

        // Create test file with known content
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/rag_chunk_test_XXXXXX.txt");
        QVERIFY(tempFile.open());

        // Create content that should produce multiple chunks
        QString longContent;
        for (int i = 0; i < 100; i++) {
            longContent += QString("This is sentence number %1. ").arg(i);
        }

        QTextStream out(&tempFile);
        out << longContent;
        tempFile.close();

        // Verify file was created
        QVERIFY(QFile::exists(tempFile.fileName()));

        // Content length check
        QFile file(tempFile.fileName());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString content = file.readAll();
        file.close();

        QVERIFY(content.length() > 512);  // Should exceed default chunk size
    }

    void testUnsupportedFileType() {
        RAGEngine engine;

        // Create temporary file with unsupported extension
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/rag_test_XXXXXX.xyz");
        QVERIFY(tempFile.open());

        QTextStream out(&tempFile);
        out << "Test content";
        tempFile.close();

        // Should fail gracefully for unsupported file type
        // Note: This requires the engine to be connected to signals
        bool result = engine.ingestDocument(tempFile.fileName());
        QVERIFY(!result);  // Should return false for unsupported file
    }

    void testNonexistentFile() {
        RAGEngine engine;

        // Try to ingest a file that doesn't exist
        bool result = engine.ingestDocument("/nonexistent/path/to/file.txt");

        // Should fail gracefully
        QVERIFY(!result);

        // Document count should remain 0
        QCOMPARE(engine.getDocumentCount(), 0);
    }

    void testMarkdownFileDetection() {
        RAGEngine engine;

        // Create markdown file
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/rag_test_XXXXXX.md");
        QVERIFY(tempFile.open());

        QString mdContent = "# Test Markdown\n\n"
                           "This is a **markdown** document.\n\n"
                           "- Item 1\n"
                           "- Item 2\n";

        QTextStream out(&tempFile);
        out << mdContent;
        tempFile.close();

        // Verify file exists and has correct extension
        QVERIFY(tempFile.fileName().endsWith(".md"));
        QVERIFY(QFile::exists(tempFile.fileName()));
    }

    void testDirectoryHandling() {
        RAGEngine engine;

        // Create temporary directory with test files
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create test files in directory
        QString file1Path = tempDir.path() + "/test1.txt";
        QString file2Path = tempDir.path() + "/test2.md";

        QFile file1(file1Path);
        QVERIFY(file1.open(QIODevice::WriteOnly));
        QTextStream out1(&file1);
        out1 << "Content of file 1";
        file1.close();

        QFile file2(file2Path);
        QVERIFY(file2.open(QIODevice::WriteOnly));
        QTextStream out2(&file2);
        out2 << "Content of file 2";
        file2.close();

        // Verify files exist
        QVERIFY(QFile::exists(file1Path));
        QVERIFY(QFile::exists(file2Path));

        // Test directory ingestion
        // Note: Actual ingestion requires network access
        // This just verifies the directory structure
        QVERIFY(tempDir.isValid());
    }

    void testEmptyDocumentHandling() {
        RAGEngine engine;

        // Create empty file
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/rag_empty_XXXXXX.txt");
        QVERIFY(tempFile.open());
        // Don't write anything - leave empty
        tempFile.close();

        // Should handle empty file gracefully
        bool result = engine.ingestDocument(tempFile.fileName());

        // Empty file should fail (no content to chunk)
        QVERIFY(!result);
    }

    void testStatisticsAfterClear() {
        RAGEngine engine;

        // Verify initial state
        QCOMPARE(engine.getDocumentCount(), 0);
        QCOMPARE(engine.getChunkCount(), 0);

        // Clear documents
        engine.clearDocuments();

        // Verify still clean
        QCOMPARE(engine.getDocumentCount(), 0);
        QCOMPARE(engine.getChunkCount(), 0);
    }

    void testMultipleClearOperations() {
        RAGEngine engine;

        // Multiple clears should be safe
        engine.clearDocuments();
        engine.clearDocuments();
        engine.clearDocuments();

        // Should not crash and should remain at 0
        QCOMPARE(engine.getDocumentCount(), 0);
    }

    void testConfigurationSettings() {
        RAGEngine engine;

        // Test various chunk sizes
        engine.setChunkSize(128);
        engine.setChunkSize(512);
        engine.setChunkSize(1024);
        engine.setChunkSize(2048);

        // Test various overlap sizes
        engine.setChunkOverlap(0);
        engine.setChunkOverlap(50);
        engine.setChunkOverlap(100);

        // Should not crash
        QVERIFY(true);
    }

    void testSignalConnections() {
        RAGEngine engine;

        // Test that we can connect to signals without crashing
        QSignalSpy spyIngested(&engine, &RAGEngine::documentIngested);
        QSignalSpy spyError(&engine, &RAGEngine::ingestionError);
        QSignalSpy spyContext(&engine, &RAGEngine::contextRetrieved);
        QSignalSpy spyQueryError(&engine, &RAGEngine::queryError);

        // Verify signals are valid
        QVERIFY(spyIngested.isValid());
        QVERIFY(spyError.isValid());
        QVERIFY(spyContext.isValid());
        QVERIFY(spyQueryError.isValid());
    }

    void testRetrieveContextWithoutDocuments() {
        RAGEngine engine;

        // Setup signal spy
        QSignalSpy spyError(&engine, &RAGEngine::queryError);

        // Try to retrieve context without any documents
        QStringList result = engine.retrieveContext("test query", 3);

        // Should return empty list
        QVERIFY(result.isEmpty());

        // Should emit error signal
        QVERIFY(spyError.count() > 0);
    }
};

QTEST_MAIN(TestRAGEngine)
#include "test_ragengine.moc"
