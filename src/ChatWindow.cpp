/**
 * ChatWindow.cpp - Main application window implementation
 * 
 * Coordinates UI components, manager classes, and core services. Handles
 * menu actions, signal-slot connections, and application lifecycle.
 */

#include "ChatWindow.h"
#include "version.h"
#include "Logger.h"
#include "Config.h"
#include "ThemeManager.h"
#include "LLMClient.h"
#include "SettingsDialog.h"
#include "LogViewerDialog.h"
#include "MCPHandler.h"
#include "RAGEngine.h"
#include "ConversationManager.h"
#include "MessageRenderer.h"
#include "ToolUIManager.h"
#include "RAGUIManager.h"
#include "HTMLHandler.h"

#include <QApplication>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QKeySequence>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCursor>
#include <QTextDocument>
#include <QClipboard>
#include <QCloseEvent>
#include <QUrl>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

// Forward declare external tool functions
extern QJsonObject exampleCalculatorTool(const QJsonObject &params);
extern QJsonObject exampleDateTimeTool(const QJsonObject &params);

ChatWindow::ChatWindow(QWidget *parent)
    : QMainWindow(parent)
    , isStreaming(false)
    , streamingMessageCreated(false)
    , lastSearchText("") {
    setWindowTitle(QString("%1 v%2").arg(APP_NAME, APP_VERSION));
    setMinimumSize(800, 600);

    // Create menu bar
    createMenuBar();

    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Chat display area with clean modern styling
    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setAcceptRichText(true);
    chatDisplay->setPlaceholderText(tr("Chat messages will appear here..."));
    chatDisplay->setStyleSheet(
        "QTextEdit {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 8px;"
        "  padding: 12px;"
        "  font-size: 10pt;"
        "  font-family: 'Segoe UI', 'Ubuntu', 'DejaVu Sans', sans-serif;"
        "}"
    );
    layout->addWidget(chatDisplay);

    // Initialize message renderer
    messageRenderer = new MessageRenderer(chatDisplay, this);
    connect(messageRenderer, &MessageRenderer::messageAppended, this, [this](const QString &sender, const QString &) {
        // Mark conversation as modified (except for system initialization messages)
        if (sender != "System" || conversationManager->isModified()) {
            conversationManager->setModified(true);
        }
    });

    // Initialize conversation manager
    conversationManager = new ConversationManager(chatDisplay, this);
    connect(conversationManager, &ConversationManager::modificationStateChanged, this, &ChatWindow::updateWindowTitle);
    connect(conversationManager, &ConversationManager::conversationChanged, this, &ChatWindow::updateWindowTitle);
    connect(conversationManager, &ConversationManager::messagePosted, messageRenderer, &MessageRenderer::appendMessage);

    // Thinking indicator
    thinkingLabel = new QLabel(this);
    thinkingLabel->setStyleSheet("color: #888; font-style: italic; padding: 5px;");
    thinkingLabel->hide();
    layout->addWidget(thinkingLabel);

    // Thinking animation timer
    thinkingTimer = new QTimer(this);
    thinkingDots = 0;
    connect(thinkingTimer, &QTimer::timeout, this, &ChatWindow::updateThinkingAnimation);

    // Input area with modern styling
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    inputField = new QLineEdit(this);
    inputField->setPlaceholderText(tr("Type your message here..."));
    inputField->setStyleSheet(
        "QLineEdit {"
        "  background-color: white;"
        "  border: 2px solid #e0e0e0;"
        "  border-radius: 20px;"
        "  padding: 10px 16px;"
        "  font-size: 11pt;"
        "  min-height: 20px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2196F3;"
        "}"
    );

    sendButton = new QPushButton(tr("Send"), this);
    sendButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 20px;"
        "  padding: 10px 24px;"
        "  font-size: 11pt;"
        "  font-weight: bold;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0D47A1;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #BDBDBD;"
        "}"
    );

    inputLayout->addWidget(inputField);
    inputLayout->addWidget(sendButton);
    layout->addLayout(inputLayout);

    // Connect signals
    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::sendMessage);
    connect(inputField, &QLineEdit::returnPressed, this, &ChatWindow::sendMessage);

    // Initialize LLM client
    llmClient = new LLMClient(this);
    connect(llmClient, &LLMClient::responseReceived, this, &ChatWindow::handleLLMResponse);
    connect(llmClient, &LLMClient::errorOccurred, this, &ChatWindow::handleLLMError);
    connect(llmClient, &LLMClient::tokenReceived, this, &ChatWindow::handleStreamingToken);
    connect(llmClient, &LLMClient::retryAttempt, this, &ChatWindow::handleRetryAttempt);
    connect(llmClient, &LLMClient::toolCallRequested, this, &ChatWindow::handleToolCallRequest);

    // Initialize MCP handler and register tools
    mcpHandler = new MCPHandler(this);
    registerLocalTools();  // Register local tools immediately (no network needed)
    connect(mcpHandler, &MCPHandler::toolCallCompleted, this, &ChatWindow::handleToolCallCompleted);
    connect(mcpHandler, &MCPHandler::toolCallFailed, this, &ChatWindow::handleToolCallFailed);
    
    // Initialize tool UI manager
    toolUIManager = new ToolUIManager(mcpHandler, this);
    
    // Defer MCP server discovery until event loop is running (network manager needs to be ready)
    QTimer::singleShot(100, this, [this]() {
        registerConfiguredServers();
        updateStatusBar();  // Update status bar with discovered tools
        
        // Show registered tools in chat
        QStringList toolNames = mcpHandler->getRegisteredTools();
        if (toolNames.size() > 2) {  // More than just the 2 built-in tools
            // Wrap tool names in backticks to prevent underscore interpretation as markdown
            QStringList wrappedNames;
            for (const QString &name : toolNames) {
                wrappedNames << QString("`%1`").arg(name);
            }
            messageRenderer->appendMessage("System", tr("Available tools: %1").arg(wrappedNames.join(", ")));
        }
    });

    // Initialize RAG engine
    ragEngine = new RAGEngine(this);
    ragEngine->setEmbeddingModel(Config::instance().getRagEmbeddingModel());
    ragEngine->setChunkSize(Config::instance().getRagChunkSize());
    ragEngine->setChunkOverlap(Config::instance().getRagChunkOverlap());
    connect(ragEngine, &RAGEngine::contextRetrieved, this, &ChatWindow::handleRAGContextRetrieved);
    connect(ragEngine, &RAGEngine::queryError, this, &ChatWindow::handleRAGError);
    LOG_INFO(QString("RAG Engine initialized (enabled: %1)").arg(Config::instance().getRagEnabled() ? "yes" : "no"));
    
    // Initialize RAG UI manager
    ragUIManager = new RAGUIManager(ragEngine, this);
    connect(ragUIManager, &RAGUIManager::documentIngested, this, [this](const QString &filename, int chunkCount) {
        messageRenderer->appendMessage("System", tr("Document ingested successfully: %1 (total chunks: %2)")
            .arg(filename).arg(chunkCount));
    });
    connect(ragUIManager, &RAGUIManager::directoryIngested, this, [this](const QString & /*path*/, int chunkCount) {
        messageRenderer->appendMessage("System", tr("Directory ingested successfully. Total chunks: %1").arg(chunkCount));
    });
    connect(ragUIManager, &RAGUIManager::ingestionFailed, this, [this](const QString &error) {
        messageRenderer->appendMessage("System", error);
    });
    connect(ragUIManager, &RAGUIManager::documentsCleared, this, [this]() {
        messageRenderer->appendMessage("System", tr("All RAG documents cleared."));
    });
    connect(ragUIManager, &RAGUIManager::statusUpdated, this, &ChatWindow::updateStatusBar);

    // Create status bar
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    updateStatusBar();

    // Add welcome message
    messageRenderer->appendMessage("System", tr("Welcome to %1!").arg(APP_NAME));
    messageRenderer->appendMessage("System", tr("This is a Qt5 chatbot application with MCP and RAG integration."));
    QString backend = Config::instance().getBackend();
    QString model = Config::instance().getModel();
    messageRenderer->appendMessage("System", tr("Backend: %1 | Model: %2").arg(backend, model));
    
    // Tool list will be shown after MCP server discovery completes
}

void ChatWindow::sendMessage() {
    QString message = inputField->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    // Display user message
    messageRenderer->appendMessage("You", message);

    // Clear input and disable while waiting
    inputField->clear();
    inputField->setEnabled(false);
    sendButton->setEnabled(false);

    // Prepare for streaming response
    isStreaming = true;
    streamingMessageCreated = false;
    currentStreamingResponse.clear();
    currentPrompt = message; // Store for potential tool result follow-up
    ragContext.clear();

    // Show thinking indicator
    showThinkingIndicator();

    // Check if RAG is enabled and has documents
    if (Config::instance().getRagEnabled() && ragEngine && ragEngine->getChunkCount() > 0) {
        LOG_INFO("RAG enabled - retrieving context");
        // Retrieve context asynchronously - will trigger handleRAGContextRetrieved
        int topK = Config::instance().getRagTopK();
        ragEngine->retrieveContext(message, topK);
    } else {
        // No RAG - send directly to LLM
        if (toolUIManager && mcpHandler) {
            QJsonArray tools = toolUIManager->getEnabledTools();
            llmClient->sendPromptWithTools(message, tools);
        } else {
            llmClient->sendPrompt(message);
        }
    }
}

void ChatWindow::handleStreamingToken(const QString &token) {
    if (!isStreaming) {
        return;
    }

    // Hide thinking indicator and create initial message on first token
    if (currentStreamingResponse.isEmpty() && !streamingMessageCreated) {
        hideThinkingIndicator();
        // Start bot response
        messageRenderer->appendMessage("Bot", "");
        streamingMessageCreated = true;
        LOG_INFO("Created initial streaming message");
    }

    // Append token to current response (even if empty, to track state)
    currentStreamingResponse.append(token);

    // Only update if the last message is from the Bot (safety check)
    if (messageRenderer->lastMessageSender() == "Bot") {
        messageRenderer->updateLastMessage(currentStreamingResponse, true);
    }
}

void ChatWindow::handleLLMResponse(const QString &response) {
    LOG_DEBUG(QString("handleLLMResponse called with response length: %1").arg(response.length()));
    LOG_DEBUG(QString("currentStreamingResponse length: %1").arg(currentStreamingResponse.length()));

    // Streaming is complete, final response received
    isStreaming = false;

    // Hide thinking indicator
    hideThinkingIndicator();

    // Determine which response to display
    QString finalResponse;
    if (!response.isEmpty()) {
        // Use the response parameter (e.g., from tool results that don't stream)
        finalResponse = response;
        LOG_DEBUG("Using response parameter for final message");
    } else if (!currentStreamingResponse.isEmpty()) {
        // Use the streamed response (from normal LLM responses)
        finalResponse = currentStreamingResponse;
        LOG_DEBUG("Using currentStreamingResponse for final message");
    }

    // Display the final response
    if (!finalResponse.isEmpty()) {
        if (messageRenderer->lastMessageSender() == "Bot") {
            // Update existing bot message
            messageRenderer->updateLastMessage(finalResponse, false);
        } else {
            // Create new bot message
            messageRenderer->appendMessage("Bot", finalResponse);
        }
    }

    // Clear for next response
    currentStreamingResponse.clear();
    streamingMessageCreated = false;

    // Re-enable input
    inputField->setEnabled(true);
    sendButton->setEnabled(true);
    inputField->setFocus();
}

void ChatWindow::handleLLMError(const QString &error) {
    // Display error
    isStreaming = false;

    // Hide thinking indicator
    hideThinkingIndicator();

    messageRenderer->appendMessage("System", tr("Error: %1").arg(error));

    // Re-enable input
    inputField->setEnabled(true);
    sendButton->setEnabled(true);
    inputField->setFocus();
}

void ChatWindow::updateThinkingAnimation() {
    thinkingDots = (thinkingDots + 1) % 4;
    QString dots = QString(".").repeated(thinkingDots);
    thinkingLabel->setText(tr("Thinking%1").arg(dots));
}

void ChatWindow::showThinkingIndicator() {
    thinkingDots = 0;
    thinkingLabel->setText(tr("Thinking"));
    thinkingLabel->show();
    thinkingTimer->start(500); // Update every 500ms
}

void ChatWindow::hideThinkingIndicator() {
    thinkingTimer->stop();
    thinkingLabel->hide();
}

void ChatWindow::handleRetryAttempt(int attempt, int maxRetries) {
    // Display retry notification
    messageRenderer->appendMessage("System", tr("Connection failed. Retrying... (attempt %1/%2)")
                  .arg(attempt).arg(maxRetries));
}

void ChatWindow::handleToolCallRequest(const QString &toolName, const QJsonObject &parameters, const QString &callId) {
    // Remove the bot message containing the tool call JSON (streamed tokens)
    // This prevents showing both the JSON and the natural response
    if (messageRenderer->lastMessageSender() == "Bot") {
        // Get current HTML
        QString html = chatDisplay->toHtml();

        // Find the last <p> tag (the bot message with JSON)
        int lastPStart = html.lastIndexOf("<p ");
        if (lastPStart != -1) {
            // Find the end of that <p> tag
            int lastPEnd = html.indexOf("</p>", lastPStart);
            if (lastPEnd != -1) {
                // Remove the last bot message by reconstructing HTML without it
                QString beforeLast = html.left(lastPStart);
                QString afterLast = html.mid(lastPEnd + 4); // Skip "</p>"

                // Reconstruct HTML without the JSON message
                QString newHtml = beforeLast + afterLast;
                chatDisplay->setHtml(newHtml);

                LOG_DEBUG("Removed bot message with tool call JSON");
            }
        }

        // Reset last message sender since we removed the bot message
        messageRenderer->clearLastMessageSender();
    }

    // Tool call indicator widget
    QString toolWidget = HTMLHandler::createToolCallWidget(toolName);
    chatDisplay->append(toolWidget);

    // Reset last message sender to ensure assistant response starts fresh on new line
    messageRenderer->clearLastMessageSender();

    LOG_INFO(QString("Tool call requested: %1 (ID: %2)").arg(toolName, callId));

    // Execute the tool via MCP handler
    if (mcpHandler) {
        mcpHandler->executeToolCall(toolName, parameters);
    }
}

void ChatWindow::handleToolCallCompleted(const QString &toolCallId, const QString &toolName, const QJsonObject &result) {
    LOG_INFO(QString("Tool call completed: %1 (ID: %2)").arg(toolName, toolCallId));

    // Don't show success widget - we'll show the natural response directly
    // Send result to be formatted as natural language
    QJsonArray toolResults;
    QJsonObject toolResult;
    toolResult["tool_name"] = toolName;
    toolResult["call_id"] = toolCallId;
    toolResult["result"] = result;
    toolResults.append(toolResult);

    // Prepare for the new streaming response from LLM
    currentStreamingResponse.clear();
    streamingMessageCreated = false;
    isStreaming = true;
    LOG_INFO("Reset streaming state for tool result response");

    // sendToolResults will emit responseReceived with formatted natural language
    llmClient->sendToolResults(currentPrompt, toolResults);
}

void ChatWindow::handleToolCallFailed(const QString &toolCallId, const QString &toolName, const QString &error) {
    LOG_ERROR(QString("Tool call failed: %1 (ID: %2) - %3").arg(toolName, toolCallId, error));

    // Tool error widget
    QString errorWidget = HTMLHandler::createToolErrorWidget(toolName, error);
    chatDisplay->append(errorWidget);

    // Re-enable input
    inputField->setEnabled(true);
    sendButton->setEnabled(true);
    inputField->setFocus();
}

void ChatWindow::handleRAGContextRetrieved(const QStringList &contexts) {
    LOG_INFO(QString("RAG retrieved %1 context chunks").arg(contexts.size()));

    // Build context string
    ragContext.clear();
    if (!contexts.isEmpty()) {
        ragContext = "CONTEXT FROM DOCUMENTS:\n\n";
        for (int i = 0; i < contexts.size(); ++i) {
            ragContext += QString("--- Document Chunk %1 ---\n%2\n\n").arg(i + 1).arg(contexts[i]);
        }
        ragContext += "\nPlease use the above context to answer the user's question.\n\n";
    }

    // Now send the prompt with RAG context prepended
    QString enhancedPrompt = ragContext + "USER QUESTION: " + currentPrompt;

    // Send to LLM with tools if enabled
    if (toolUIManager && mcpHandler) {
        QJsonArray tools = toolUIManager->getEnabledTools();
        llmClient->sendPromptWithTools(enhancedPrompt, tools);
    } else {
        llmClient->sendPrompt(enhancedPrompt);
    }
}

void ChatWindow::handleRAGError(const QString &error) {
    LOG_WARNING(QString("RAG error: %1 - proceeding without RAG context").arg(error));

    // Proceed with original prompt without RAG context
    if (toolUIManager && mcpHandler) {
        QJsonArray tools = toolUIManager->getEnabledTools();
        llmClient->sendPromptWithTools(currentPrompt, tools);
    } else {
        llmClient->sendPrompt(currentPrompt);
    }
}

void ChatWindow::toggleLightTheme() {
    ThemeManager::instance().setTheme(ThemeManager::Light);
    QApplication *app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        ThemeManager::instance().applyTheme(app);
    }
}

void ChatWindow::toggleDarkTheme() {
    ThemeManager::instance().setTheme(ThemeManager::Dark);
    QApplication *app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        ThemeManager::instance().applyTheme(app);
    }
}

void ChatWindow::openSettings() {
    SettingsDialog dialog(this);

    // Connect signal to refresh MCP tools when settings are saved
    connect(&dialog, &SettingsDialog::settingsSaved, this, &ChatWindow::refreshMcpTools);

    if (dialog.exec() == QDialog::Accepted) {
        // Settings were saved, update LLM client if needed
        llmClient->setModel(Config::instance().getModel());
        llmClient->setApiUrl(Config::instance().getApiUrl());
        LOG_INFO("Settings updated from dialog");

        // Update status bar
        updateStatusBar();
    }
}

void ChatWindow::refreshMcpTools() {
    if (!mcpHandler) {
        return;
    }

    LOG_INFO("Refreshing MCP tools after settings change");

    // Clear all networked tools (keep local tools)
    int removedCount = mcpHandler->clearNetworkedTools();
    LOG_DEBUG(QString("Cleared %1 networked tools").arg(removedCount));

    // Re-register MCP server tools from updated configuration
    registerConfiguredServers();

    // Update status bar to reflect new tool count
    updateStatusBar();

    LOG_INFO(QString("MCP tools refreshed: now have %1 total tools")
             .arg(mcpHandler->getRegisteredTools().size()));
}

void ChatWindow::showLogViewer() {
    // Create log viewer dialog WITHOUT parent (to avoid double-free with WA_DeleteOnClose)
    LogViewerDialog *logViewer = new LogViewerDialog(nullptr);
    logViewer->setAttribute(Qt::WA_DeleteOnClose);  // Auto-delete when closed

    // Set as the active instance for message forwarding
    LogViewerDialog::setInstance(logViewer);

    // Show the dialog
    logViewer->show();

    LOG_INFO("Log viewer opened");
}

void ChatWindow::showToolsDialog() {
    if (toolUIManager) {
        toolUIManager->showToolsDialog();
    }
}

void ChatWindow::clearConversation() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear Conversation"),
                                   tr("Are you sure you want to clear the conversation?"),
                                   QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        messageRenderer->clear();
        conversationManager->setModified(false);
        conversationManager->clearCurrentFile();
        LOG_INFO("Conversation cleared");
        messageRenderer->appendMessage("System", tr("Conversation cleared."));
    }
}

void ChatWindow::findInConversation() {
    bool ok;
    QString searchText = QInputDialog::getText(this,
        tr("Find in Conversation"),
        tr("Enter search text:"),
        QLineEdit::Normal,
        lastSearchText,
        &ok);

    if (!ok || searchText.isEmpty()) {
        return;
    }

    lastSearchText = searchText;

    // Get cursor and start from current position
    QTextCursor cursor = chatDisplay->textCursor();
    QTextDocument::FindFlags flags;

    // Search forward from current position
    cursor = chatDisplay->document()->find(searchText, cursor, flags);

    if (!cursor.isNull()) {
        chatDisplay->setTextCursor(cursor);
        chatDisplay->ensureCursorVisible();
        LOG_INFO(QString("Found text: %1").arg(searchText));
    } else {
        // Try from beginning
        cursor = chatDisplay->document()->find(searchText, 0, flags);
        if (!cursor.isNull()) {
            chatDisplay->setTextCursor(cursor);
            chatDisplay->ensureCursorVisible();
            LOG_INFO(QString("Found text from beginning: %1").arg(searchText));
        } else {
            QMessageBox::information(this, tr("Find"),
                tr("Text not found: %1").arg(searchText));
            LOG_INFO(QString("Text not found: %1").arg(searchText));
        }
    }
}

void ChatWindow::copyConversation() {
    QString plainText = messageRenderer->toPlainText();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(plainText);
    LOG_INFO("Conversation copied to clipboard");
    messageRenderer->appendMessage("System", tr("Conversation copied to clipboard!"));
}

void ChatWindow::newConversation() {
    conversationManager->newConversation();
}

void ChatWindow::saveConversation() {
    conversationManager->saveConversation();
}

void ChatWindow::loadConversation() {
    conversationManager->loadConversation();
}

void ChatWindow::exportConversation() {
    conversationManager->exportConversation();
}

void ChatWindow::ingestDocument() {
    if (ragUIManager) {
        ragUIManager->ingestDocument();
    }
}

void ChatWindow::ingestDirectory() {
    if (ragUIManager) {
        ragUIManager->ingestDirectory();
    }
}

void ChatWindow::viewDocuments() {
    if (ragUIManager) {
        ragUIManager->viewDocuments();
    }
}

void ChatWindow::clearDocuments() {
    if (ragUIManager) {
        ragUIManager->clearDocuments();
    }
}

void ChatWindow::updateWindowTitle() {
    QString title = QString("%1 v%2").arg(APP_NAME, APP_VERSION);
    if (!conversationManager->currentFile().isEmpty()) {
        title += QString(" - %1").arg(QFileInfo(conversationManager->currentFile()).fileName());
    }
    if (conversationManager->isModified()) {
        title += " *";
    }
    setWindowTitle(title);
}

void ChatWindow::updateStatusBar() {
    if (!statusBar) {
        return;
    }

    QString backend = Config::instance().getBackend();
    QString model = Config::instance().getModel();
    QString apiUrl = Config::instance().getApiUrl();

    QString statusText;
    if (backend == "ollama") {
        // Extract host and port from URL
        QUrl url(apiUrl);
        QString host = url.host();
        int port = url.port();
        QString server = host;
        if (port > 0) {
            server += QString(":%1").arg(port);
        }
        statusText = QString("Backend: Ollama | Model: %1 | Server: %2").arg(model, server);
    } else if (backend == "openai") {
        statusText = QString("Backend: OpenAI | Model: %1").arg(model);
    } else {
        statusText = QString("Backend: %1 | Model: %2").arg(backend, model);
    }

    // Add tool count
    if (mcpHandler) {
        int toolCount = mcpHandler->getRegisteredTools().size();
        statusText += QString(" | Tools: %1").arg(toolCount);
    }

    // Add RAG document count
    if (ragEngine) {
        int docCount = ragEngine->getDocumentCount();
        int chunkCount = ragEngine->getChunkCount();
        if (docCount > 0) {
            statusText += QString(" | RAG: %1 docs (%2 chunks)").arg(docCount).arg(chunkCount);
        }
    }

    statusBar->showMessage(statusText);
    LOG_DEBUG(QString("Status bar updated: %1").arg(statusText));
}

void ChatWindow::closeEvent(QCloseEvent *event) {
    fprintf(stderr, "[DEBUG] closeEvent: Start\n");
    fflush(stderr);

    // Close log viewer BEFORE ChatWindow starts destroying to prevent X11 conflicts
    LogViewerDialog* viewer = LogViewerDialog::instance();
    if (viewer) {
        fprintf(stderr, "[DEBUG] closeEvent: Closing log viewer\n");
        fflush(stderr);
        LogViewerDialog::setInstance(nullptr);
        viewer->setAttribute(Qt::WA_DeleteOnClose, false);
        viewer->hide();
        delete viewer;
        QApplication::processEvents();
    }

    // Explicitly delete menu bar to avoid X11 shared memory crash
    fprintf(stderr, "[DEBUG] closeEvent: Deleting menu bar\n");
    fflush(stderr);

    QMenuBar *mb = menuBar();
    if (mb) {
        setMenuBar(nullptr);  // Detach from window first
        delete mb;  // Delete explicitly
        fprintf(stderr, "[DEBUG] closeEvent: Menu bar deleted\n");
        fflush(stderr);
    }

    // Delete central widget and all children to cleanup backing stores
    fprintf(stderr, "[DEBUG] closeEvent: Deleting central widget\n");
    fflush(stderr);
    QWidget *central = centralWidget();
    if (central) {
        setCentralWidget(nullptr);  // Detach first
        delete central;  // Delete explicitly
    }

    // Delete status bar (using member variable)
    if (statusBar) {
        setStatusBar(nullptr);
        delete statusBar;
        statusBar = nullptr;
    }

    // Process events to ensure all X11 resources are released
    fprintf(stderr, "[DEBUG] closeEvent: Processing events for X11 cleanup\n");
    fflush(stderr);
    QApplication::processEvents();
    QApplication::processEvents();

    // Forcibly destroy the native X11 window to prevent Qt's automatic
    // cleanup from triggering the xcb_shm_detach_checked() buffer overflow
    fprintf(stderr, "[DEBUG] closeEvent: Destroying native window\n");
    fflush(stderr);
    destroy();  // Destroy native window handle
    QApplication::processEvents();

    fprintf(stderr, "[DEBUG] closeEvent: Calling parent closeEvent\n");
    fflush(stderr);

    // Accept the close event
    QMainWindow::closeEvent(event);

    fprintf(stderr, "[DEBUG] closeEvent: Complete\n");
    fflush(stderr);

    // Exit immediately to prevent the stack-allocated ChatWindow destructor
    // from running after we've manually destroyed everything
    fprintf(stderr, "[DEBUG] closeEvent: Exiting application\n");
    fflush(stderr);
    std::exit(0);  // Terminate immediately, skip destructors
}

void ChatWindow::registerTools() {
    if (!mcpHandler) {
        return;
    }

    // Register built-in local tools
    registerLocalTools();

    // Register tools from configured MCP servers (deferred to event loop)
    QTimer::singleShot(100, this, [this]() {
        registerConfiguredServers();
        LOG_INFO(QString("Registered %1 MCP tools total").arg(mcpHandler->getRegisteredTools().size()));
    });
}

void ChatWindow::registerLocalTools() {
    if (!mcpHandler) {
        return;
    }

    LOG_DEBUG("Registering built-in local tools");

    // Register calculator tool
    MCPTool calculatorTool;
    calculatorTool.name = "calculator";
    calculatorTool.description = "Performs basic arithmetic operations (add, subtract, multiply, divide)";
    calculatorTool.isLocal = true;
    calculatorTool.function = exampleCalculatorTool;
    calculatorTool.parameters = QJsonObject{
        {"operation", QJsonValue("string: add, subtract, multiply, or divide")},
        {"a", QJsonValue("number: first operand")},
        {"b", QJsonValue("number: second operand")}
    };
    mcpHandler->registerTool(calculatorTool);

    // Register datetime tool
    MCPTool datetimeTool;
    datetimeTool.name = "datetime";
    datetimeTool.description = "Get current date and time in various formats";
    datetimeTool.isLocal = true;
    datetimeTool.function = exampleDateTimeTool;
    datetimeTool.parameters = QJsonObject{
        {"format", QJsonValue("string: 'short', 'long', 'iso', or 'timestamp' (default: long)")}
    };
    mcpHandler->registerTool(datetimeTool);

    LOG_DEBUG(QString("Registered %1 built-in local tools").arg(2));
}

void ChatWindow::registerConfiguredServers() {
    if (!mcpHandler) {
        return;
    }

    QJsonArray mcpServers = Config::instance().getMcpServers();
    if (mcpServers.isEmpty()) {
        LOG_DEBUG("No MCP servers configured");
        return;
    }

    LOG_INFO(QString("Found %1 configured MCP servers").arg(mcpServers.size()));

    for (const QJsonValue &serverVal : mcpServers) {
        if (!serverVal.isObject()) {
            continue;
        }

        QJsonObject server = serverVal.toObject();
        QString name = server["name"].toString();
        QString url = server["url"].toString();
        QString type = server["type"].toString().toLower();
        bool enabled = server["enabled"].toBool(true);

        if (!enabled) {
            LOG_DEBUG(QString("Skipping disabled MCP server: %1").arg(name));
            continue;
        }

        if (name.isEmpty() || url.isEmpty()) {
            LOG_WARNING(QString("Invalid MCP server configuration (missing name or URL)"));
            continue;
        }

        // Use MCPHandler's discovery method - no more duplication!
        int toolCount = mcpHandler->discoverAndRegisterServerTools(name, url, type);
        if (toolCount < 0) {
            LOG_WARNING(QString("Failed to discover tools from MCP server: %1").arg(name));
        }
    }
}

void ChatWindow::createMenuBar() {
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    QAction *newAction = new QAction(tr("&New Conversation"), this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &ChatWindow::newConversation);
    fileMenu->addAction(newAction);

    QAction *saveAction = new QAction(tr("&Save Conversation..."), this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &ChatWindow::saveConversation);
    fileMenu->addAction(saveAction);

    QAction *loadAction = new QAction(tr("&Load Conversation..."), this);
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &ChatWindow::loadConversation);
    fileMenu->addAction(loadAction);

    fileMenu->addSeparator();

    QAction *exportAction = new QAction(tr("&Export Conversation..."), this);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &ChatWindow::exportConversation);
    fileMenu->addAction(exportAction);

    fileMenu->addSeparator();

    QAction *settingsAction = new QAction(tr("&Settings..."), this);
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &ChatWindow::openSettings);
    fileMenu->addAction(settingsAction);

    fileMenu->addSeparator();

    QAction *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAction);

    // Edit menu
    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

    QAction *findAction = new QAction(tr("&Find in Conversation..."), this);
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &ChatWindow::findInConversation);
    editMenu->addAction(findAction);

    editMenu->addSeparator();

    QAction *copyAction = new QAction(tr("&Copy Conversation"), this);
    copyAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(copyAction, &QAction::triggered, this, &ChatWindow::copyConversation);
    editMenu->addAction(copyAction);

    editMenu->addSeparator();

    QAction *clearAction = new QAction(tr("C&lear Conversation"), this);
    clearAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(clearAction, &QAction::triggered, this, &ChatWindow::clearConversation);
    editMenu->addAction(clearAction);

    // View menu
    QMenu *viewMenu = menuBar->addMenu(tr("&View"));

    // Theme submenu
    QMenu *themeMenu = viewMenu->addMenu(tr("&Theme"));

    QAction *lightThemeAction = new QAction(tr("&Light"), this);
    connect(lightThemeAction, &QAction::triggered, this, &ChatWindow::toggleLightTheme);
    themeMenu->addAction(lightThemeAction);

    QAction *darkThemeAction = new QAction(tr("&Dark"), this);
    connect(darkThemeAction, &QAction::triggered, this, &ChatWindow::toggleDarkTheme);
    themeMenu->addAction(darkThemeAction);

    viewMenu->addSeparator();

    QAction *manageToolsAction = new QAction(tr("&Manage Tools..."), this);
    manageToolsAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(manageToolsAction, &QAction::triggered, this, &ChatWindow::showToolsDialog);
    viewMenu->addAction(manageToolsAction);

    QAction *viewLogsAction = new QAction(tr("&Log Viewer..."), this);
    viewLogsAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(viewLogsAction, &QAction::triggered, this, &ChatWindow::showLogViewer);
    viewMenu->addAction(viewLogsAction);

    // RAG menu
    QMenu *ragMenu = menuBar->addMenu(tr("&RAG"));

    QAction *ingestDocAction = new QAction(tr("Ingest &Document..."), this);
    ingestDocAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(ingestDocAction, &QAction::triggered, this, &ChatWindow::ingestDocument);
    ragMenu->addAction(ingestDocAction);

    QAction *ingestDirAction = new QAction(tr("Ingest D&irectory..."), this);
    ingestDirAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(ingestDirAction, &QAction::triggered, this, &ChatWindow::ingestDirectory);
    ragMenu->addAction(ingestDirAction);

    ragMenu->addSeparator();

    QAction *viewDocsAction = new QAction(tr("&View Documents..."), this);
    viewDocsAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(viewDocsAction, &QAction::triggered, this, &ChatWindow::viewDocuments);
    ragMenu->addAction(viewDocsAction);

    ragMenu->addSeparator();

    QAction *clearDocsAction = new QAction(tr("&Clear All Documents"), this);
    connect(clearDocsAction, &QAction::triggered, this, &ChatWindow::clearDocuments);
    ragMenu->addAction(clearDocsAction);
}
