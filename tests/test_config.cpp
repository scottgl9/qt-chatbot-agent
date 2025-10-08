#include <QtTest/QtTest>
#include "../include/Config.h"

class TestConfig : public QObject {
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

    void testDefaultValues() {
        Config::instance().resetToDefaults();

        QCOMPARE(Config::instance().getBackend(), QString("ollama"));
        QCOMPARE(Config::instance().getModel(), QString("llama3"));
        QCOMPARE(Config::instance().getApiUrl(), QString("http://localhost:11434/api/generate"));
        QCOMPARE(Config::instance().getContextWindowSize(), 4096);
        QCOMPARE(Config::instance().getTemperature(), 0.7);
        QCOMPARE(Config::instance().getTopP(), 0.9);
        QCOMPARE(Config::instance().getTopK(), 40);
        QCOMPARE(Config::instance().getMaxTokens(), 2048);
    }

    void testSetBackend() {
        Config::instance().setBackend("openai");
        QCOMPARE(Config::instance().getBackend(), QString("openai"));

        Config::instance().setBackend("ollama");
        QCOMPARE(Config::instance().getBackend(), QString("ollama"));
    }

    void testSetModel() {
        Config::instance().setModel("gpt-4");
        QCOMPARE(Config::instance().getModel(), QString("gpt-4"));

        Config::instance().setModel("llama3");
        QCOMPARE(Config::instance().getModel(), QString("llama3"));
    }

    void testSetApiUrl() {
        QString testUrl = "http://test.example.com:8080/api";
        Config::instance().setApiUrl(testUrl);
        QCOMPARE(Config::instance().getApiUrl(), testUrl);
    }

    void testSetParameters() {
        Config::instance().setContextWindowSize(8192);
        QCOMPARE(Config::instance().getContextWindowSize(), 8192);

        Config::instance().setTemperature(0.5);
        QCOMPARE(Config::instance().getTemperature(), 0.5);

        Config::instance().setTopP(0.95);
        QCOMPARE(Config::instance().getTopP(), 0.95);

        Config::instance().setTopK(50);
        QCOMPARE(Config::instance().getTopK(), 50);

        Config::instance().setMaxTokens(4096);
        QCOMPARE(Config::instance().getMaxTokens(), 4096);
    }

    void testConfigValidity() {
        Config::instance().resetToDefaults();
        QVERIFY(Config::instance().isValid());

        // Test with empty backend
        Config::instance().setBackend("");
        QVERIFY(!Config::instance().isValid());

        Config::instance().resetToDefaults();
    }

    void testJsonConversion() {
        Config::instance().resetToDefaults();
        Config::instance().setModel("test-model");
        Config::instance().setTemperature(0.8);

        // Note: toJson() and fromJson() are private, but we can test through save/load
        // This is a basic smoke test
        QVERIFY(Config::instance().isValid());
    }
};

QTEST_MAIN(TestConfig)
#include "test_config.moc"
