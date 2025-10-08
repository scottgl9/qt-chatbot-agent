/**
 * LLMClient.h - LLM backend communication client
 * 
 * Handles communication with Ollama/OpenAI backends, streaming responses via SSE,
 * tool calling support, context window management, and retry logic.
 */

#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

class LLMClient : public QObject {
    Q_OBJECT

public:
    explicit LLMClient(QObject *parent = nullptr);
    ~LLMClient();

    // Send a prompt to the LLM
    void sendPrompt(const QString &prompt, const QString &context = QString());

    // Send a prompt with tool support
    void sendPromptWithTools(const QString &prompt, const QJsonArray &tools, const QString &context = QString());

    // Send a follow-up with tool results
    void sendToolResults(const QString &originalPrompt, const QJsonArray &toolResults);

    // Set custom API URL and model (overrides Config)
    void setApiUrl(const QString &url);
    void setModel(const QString &model);

    // Get current settings
    QString getApiUrl() const { return m_apiUrl; }
    QString getModel() const { return m_model; }

    // Retry configuration
    void setMaxRetries(int maxRetries) { m_maxRetries = maxRetries; }
    void setRetryDelay(int delayMs) { m_retryDelay = delayMs; }
    int getMaxRetries() const { return m_maxRetries; }
    int getRetryDelay() const { return m_retryDelay; }

    // Model capabilities detection
    void queryModelCapabilities();
    QString getToolCallFormat() const { return m_toolCallFormat; }
    QJsonObject getModelInfo() const { return m_modelInfo; }

    // Conversation history management
    void clearConversationHistory();

signals:
    // Emitted when a response is received
    void responseReceived(const QString &response);

    // Emitted on error
    void errorOccurred(const QString &errorMessage);

    // Emitted for streaming responses (if supported)
    void tokenReceived(const QString &token);

    // Emitted when a retry is attempted
    void retryAttempt(int attempt, int maxRetries);

    // Emitted when the LLM wants to call a tool
    void toolCallRequested(const QString &toolName, const QJsonObject &parameters, const QString &callId);

    // Emitted when model capabilities have been detected
    void modelCapabilitiesDetected(const QString &toolCallFormat, const QJsonObject &modelInfo);

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void handleStreamingData();
    void handleStreamingFinished();
    void retryRequest();
    void handleModelInfoReply();

private:
    QString buildOllamaRequest(const QString &prompt, const QString &context);
    QString buildOllamaRequestWithTools(const QString &prompt, const QJsonArray &tools, const QString &context);
    QString buildNativeToolRequest(const QString &prompt, const QJsonArray &tools, const QString &context);
    void sendRequest(const QString &jsonData);
    void processStreamingChunk(const QString &line);
    bool processToolCalls(const QString &response);
    bool processNativeToolCalls(const QJsonObject &message);
    bool shouldRetry(QNetworkReply::NetworkError error);

    // Context window management
    int estimateTokens(const QString &text) const;
    QJsonArray pruneMessageHistoryForContext(const QString &systemPrompt, const QString &currentUserMessage) const;

    QNetworkAccessManager *m_networkManager;
    QString m_apiUrl;
    QString m_model;
    QNetworkReply *m_currentReply;
    QString m_streamBuffer;
    QString m_fullResponse;

    // Retry logic
    int m_maxRetries;
    int m_retryDelay;
    int m_currentRetryCount;
    QString m_lastRequestData;

    // Tool calling support
    QJsonArray m_currentTools;
    bool m_toolsEnabled;
    bool m_nativeToolCallEmitted;  // Track if native tool call was already emitted

    // Model capabilities
    QString m_toolCallFormat;  // "native", "prompt", or "unknown"
    QJsonObject m_modelInfo;   // Full model info from /api/show
    bool m_capabilitiesDetected; // Whether capabilities detection is complete

    // Message history for native chat format
    QJsonArray m_messageHistory;
    QString m_currentPrompt;

    // Request queuing (for waiting on capabilities detection)
    struct PendingRequest {
        QString prompt;
        QJsonArray tools;
        QString context;
        bool withTools;
    };
    QList<PendingRequest> m_pendingRequests;
    void processPendingRequests();
};

#endif // LLMCLIENT_H
