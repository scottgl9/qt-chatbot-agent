/**
 * MessageRenderer.h - Chat display and message formatting manager
 * 
 * Manages chat display with streaming support, HTML/Markdown formatting,
 * and message sender tracking. Handles real-time message updates.
 */

#ifndef MESSAGERENDERER_H
#define MESSAGERENDERER_H

#include <QObject>
#include <QString>

class QTextEdit;

/**
 * @brief Manages chat display and message formatting
 * 
 * Handles all message rendering operations including:
 * - Appending messages with proper formatting
 * - Updating streaming messages in real-time
 * - Managing HTML formatting for different message types
 * - Tracking message positions for updates
 * - Auto-scrolling to latest messages
 */
class MessageRenderer : public QObject {
    Q_OBJECT

public:
    explicit MessageRenderer(QTextEdit *display, QObject *parent = nullptr);
    ~MessageRenderer() override = default;

    // Message operations
    void appendMessage(const QString &sender, const QString &message);
    void updateLastMessage(const QString &message, bool isStreaming = false);
    void clear();

    // Content access
    QString toPlainText() const;
    QString toHtml() const;

    // State management
    QString lastMessageSender() const { return m_lastMessageSender; }
    void clearLastMessageSender() { m_lastMessageSender.clear(); }
    int lastBotMessageStartPos() const { return m_lastBotMessageStartPos; }
    void setLastBotMessageStartPos(int pos) { m_lastBotMessageStartPos = pos; }

signals:
    void messageAppended(const QString &sender, const QString &message);

private:
    QTextEdit *chatDisplay;
    QString m_lastMessageSender;
    int m_lastBotMessageStartPos;

    // Helper methods
    void autoScrollToBottom();
};

#endif // MESSAGERENDERER_H
