/**
 * MessageRenderer.cpp - Chat display and message formatting
 * 
 * Manages chat display area, handles streaming message updates, formats
 * messages with HTML/Markdown, and tracks message senders.
 */

#include "MessageRenderer.h"
#include "MarkdownHandler.h"
#include "HTMLHandler.h"
#include "Logger.h"
#include <QTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QScrollBar>
#include <QTime>

MessageRenderer::MessageRenderer(QTextEdit *display, QObject *parent)
    : QObject(parent)
    , chatDisplay(display)
    , m_lastMessageSender()
    , m_lastBotMessageStartPos(-1) {
}

void MessageRenderer::appendMessage(const QString &sender, const QString &message) {
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString formattedMessage = MarkdownHandler::toHtml(message);
    QString formatted;

    if (sender == "You") {
        formatted = HTMLHandler::formatUserMessage(formattedMessage, timestamp);
    } else if (sender == "Bot") {
        formatted = HTMLHandler::formatBotMessage(formattedMessage, timestamp);
    } else {
        formatted = HTMLHandler::formatSystemMessage(formattedMessage, timestamp);
    }

    // Store position BEFORE appending (for Bot messages)
    if (sender == "Bot") {
        QTextDocument *doc = chatDisplay->document();
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::End);
        m_lastBotMessageStartPos = cursor.position();
        LOG_DEBUG(QString("Stored bot message start position: %1").arg(m_lastBotMessageStartPos));
    }

    chatDisplay->append(formatted);

    // Store the last message sender for streaming updates
    m_lastMessageSender = sender;

    // Auto-scroll to bottom
    autoScrollToBottom();

    emit messageAppended(sender, message);
}

void MessageRenderer::updateLastMessage(const QString &message, bool isStreaming) {
    // Only update if we have a last message sender and it's the Bot
    if (m_lastMessageSender.isEmpty() || m_lastMessageSender != "Bot") {
        return;
    }

    // Check if we have a valid stored position
    if (m_lastBotMessageStartPos < 0) {
        LOG_INFO("No stored bot message position, cannot update");
        appendMessage("Bot", message);
        return;
    }

    // Use QTextCursor to manipulate the document directly
    QTextDocument *doc = chatDisplay->document();
    QTextCursor cursor(doc);

    // Move to the stored position
    cursor.setPosition(m_lastBotMessageStartPos);

    // Select from stored position to end of document
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    // Build the new bot message
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    QString processedMessage;

    // IMPORTANT: Only format markdown when streaming is complete!
    // During streaming, just escape HTML and convert newlines - it's much faster.
    // When streaming completes (isStreaming=false), apply full markdown formatting.
    if (isStreaming) {
        // Fast path: streaming in progress
        processedMessage = message.toHtmlEscaped();
        processedMessage.replace("\n", "<br>");
    } else {
        // Full path: streaming complete, apply markdown formatting (includes tables)
        processedMessage = MarkdownHandler::toHtml(message);
        LOG_INFO("Applied markdown formatting to final message");
    }

    QString newBotMessage = HTMLHandler::formatBotMessage(processedMessage, timestamp);

    // Insert the HTML, replacing the selected text
    cursor.insertHtml(newBotMessage);

    // Auto-scroll to bottom
    autoScrollToBottom();
}

void MessageRenderer::clear() {
    chatDisplay->clear();
    m_lastMessageSender.clear();
    m_lastBotMessageStartPos = -1;
}

QString MessageRenderer::toPlainText() const {
    return chatDisplay->toPlainText();
}

QString MessageRenderer::toHtml() const {
    return chatDisplay->toHtml();
}

void MessageRenderer::autoScrollToBottom() {
    chatDisplay->verticalScrollBar()->setValue(
        chatDisplay->verticalScrollBar()->maximum());
}
