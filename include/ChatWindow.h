/**
 * ChatWindow.h - Main application window class
 * 
 * Coordinates UI components, manager classes (ConversationManager, MessageRenderer,
 * ToolUIManager, RAGUIManager), and core services (LLMClient, MCPHandler, RAGEngine).
 */

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QSet>
#include <QJsonArray>
#include <QJsonObject>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;
class QTimer;
class QStatusBar;
class QCloseEvent;
class LLMClient;
class MCPHandler;
class RAGEngine;
class ConversationManager;
class MessageRenderer;
class ToolUIManager;
class RAGUIManager;

/**
 * @brief Main chat window for the application
 * 
 * Coordinates the chatbot interface including:
 * - User input and message display
 * - LLM interaction and streaming responses
 * - Tool calling via MCP
 * - RAG context retrieval
 * - Menu bar and settings
 * - Status bar updates
 */
class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Message handling
    void sendMessage();
    void handleStreamingToken(const QString &token);
    void handleLLMResponse(const QString &response);
    void handleLLMError(const QString &error);
    void handleRetryAttempt(int attempt, int maxRetries);

    // Tool calling
    void handleToolCallRequest(const QString &toolName, const QJsonObject &parameters, const QString &callId);
    void handleToolCallCompleted(const QString &toolCallId, const QString &toolName, const QJsonObject &result);
    void handleToolCallFailed(const QString &toolCallId, const QString &toolName, const QString &error);

    // RAG
    void handleRAGContextRetrieved(const QStringList &contexts);
    void handleRAGError(const QString &error);

    // UI actions
    void updateThinkingAnimation();
    void showThinkingIndicator();
    void hideThinkingIndicator();

    // Theme
    void toggleLightTheme();
    void toggleDarkTheme();

    // Settings and tools
    void openSettings();
    void refreshMcpTools();
    void showLogViewer();
    void showToolsDialog();

    // Conversation management
    void clearConversation();
    void findInConversation();
    void copyConversation();
    void newConversation();
    void saveConversation();
    void loadConversation();
    void exportConversation();

    // RAG management
    void ingestDocument();
    void ingestDirectory();
    void viewDocuments();
    void clearDocuments();

    // Status and UI updates
    void updateWindowTitle();
    void updateStatusBar();

private:
    // Helper methods
    void registerTools();
    void registerLocalTools();
    void registerConfiguredServers();
    void createMenuBar();

    // UI widgets
    QTextEdit *chatDisplay;
    QLineEdit *inputField;
    QPushButton *sendButton;
    QLabel *thinkingLabel;
    QTimer *thinkingTimer;
    int thinkingDots;
    QStatusBar *statusBar;

    // Core components
    LLMClient *llmClient;
    MCPHandler *mcpHandler;
    RAGEngine *ragEngine;

    // Manager components
    ConversationManager *conversationManager;
    MessageRenderer *messageRenderer;
    ToolUIManager *toolUIManager;
    RAGUIManager *ragUIManager;

    // State
    bool isStreaming;
    bool streamingMessageCreated;
    QString currentStreamingResponse;
    QString currentPrompt;
    QString lastSearchText;
    QString ragContext;
};

#endif // CHATWINDOW_H
