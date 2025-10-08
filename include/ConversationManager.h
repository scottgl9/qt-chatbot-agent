/**
 * ConversationManager.h - Conversation persistence manager
 * 
 * Handles conversation lifecycle (new/save/load/export), tracks modification
 * state, and manages current file path. Emits signals for state changes.
 */

#ifndef CONVERSATIONMANAGER_H
#define CONVERSATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QWidget>

class QTextEdit;

/**
 * @brief Manages conversation persistence (save, load, export)
 * 
 * Handles all conversation file operations including:
 * - Creating new conversations
 * - Saving/loading conversations in JSON format
 * - Exporting conversations to text/markdown
 * - Tracking modification state
 */
class ConversationManager : public QObject {
    Q_OBJECT

public:
    explicit ConversationManager(QTextEdit *chatDisplay, QWidget *parent = nullptr);
    ~ConversationManager() override = default;

    // Conversation operations
    void newConversation();
    void saveConversation();
    void loadConversation();
    void exportConversation();

    // State management
    bool isModified() const { return conversationModified; }
    void setModified(bool modified);
    QString currentFile() const { return currentConversationFile; }
    void clearCurrentFile();

signals:
    void conversationChanged();
    void modificationStateChanged(bool modified);
    void messagePosted(const QString &sender, const QString &message);

private:
    QTextEdit *chatDisplay;
    QWidget *parentWidget;
    bool conversationModified;
    QString currentConversationFile;

    // Helper methods
    bool promptToSaveIfModified(const QString &operation);
};

#endif // CONVERSATIONMANAGER_H
