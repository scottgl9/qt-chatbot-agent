/**
 * ConversationManager.cpp - Conversation persistence and lifecycle management
 * 
 * Handles new/save/load/export operations for conversations, tracks
 * modification state, and manages current file path.
 */

#include "ConversationManager.h"
#include "Logger.h"
#include "Config.h"
#include "version.h"
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

ConversationManager::ConversationManager(QTextEdit *chatDisplay, QWidget *parent)
    : QObject(parent)
    , chatDisplay(chatDisplay)
    , parentWidget(parent)
    , conversationModified(false)
    , currentConversationFile() {
}

void ConversationManager::newConversation() {
    if (!promptToSaveIfModified("New Conversation")) {
        return; // User cancelled
    }

    chatDisplay->clear();
    conversationModified = false;
    currentConversationFile.clear();
    
    emit conversationChanged();
    emit modificationStateChanged(false);
    emit messagePosted("System", tr("New conversation started."));
    
    LOG_INFO("New conversation started");
}

void ConversationManager::saveConversation() {
    QString fileName = currentConversationFile;

    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(parentWidget,
            tr("Save Conversation"),
            QDir::homePath() + "/conversation.json",
            tr("Conversation Files (*.json);;All Files (*)"));
    }

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(parentWidget, tr("Save Failed"),
            tr("Could not open file for writing: %1").arg(fileName));
        LOG_ERROR(QString("Failed to save conversation: %1").arg(fileName));
        return;
    }

    // Create JSON structure
    QJsonObject conversation;
    conversation["version"] = "1.0";
    conversation["app"] = APP_NAME;
    conversation["app_version"] = APP_VERSION;
    conversation["saved_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    conversation["model"] = Config::instance().getModel();
    conversation["backend"] = Config::instance().getBackend();

    // Save plain text
    conversation["content"] = chatDisplay->toPlainText();

    // Save as HTML for formatting preservation
    conversation["content_html"] = chatDisplay->toHtml();

    QJsonDocument doc(conversation);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    currentConversationFile = fileName;
    conversationModified = false;
    
    emit conversationChanged();
    emit modificationStateChanged(false);
    emit messagePosted("System", tr("Conversation saved to: %1").arg(fileName));
    
    LOG_INFO(QString("Conversation saved to: %1").arg(fileName));
}

void ConversationManager::loadConversation() {
    if (!promptToSaveIfModified("Load Conversation")) {
        return; // User cancelled
    }

    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        tr("Load Conversation"),
        QDir::homePath(),
        tr("Conversation Files (*.json);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(parentWidget, tr("Load Failed"),
            tr("Could not open file for reading: %1").arg(fileName));
        LOG_ERROR(QString("Failed to load conversation: %1").arg(fileName));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(parentWidget, tr("Load Failed"),
            tr("Invalid conversation file format: %1").arg(parseError.errorString()));
        LOG_ERROR(QString("Failed to parse conversation file: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject conversation = doc.object();

    // Clear current conversation
    chatDisplay->clear();

    // Load HTML content if available (preserves formatting)
    if (conversation.contains("content_html")) {
        chatDisplay->setHtml(conversation["content_html"].toString());
    } else {
        // Fallback to plain text
        chatDisplay->setPlainText(conversation["content"].toString());
    }

    currentConversationFile = fileName;
    conversationModified = false;
    
    emit conversationChanged();
    emit modificationStateChanged(false);

    // Show metadata
    QString metadata = tr("Loaded conversation from %1\nModel: %2 | Backend: %3 | Saved: %4")
        .arg(QFileInfo(fileName).fileName())
        .arg(conversation["model"].toString())
        .arg(conversation["backend"].toString())
        .arg(conversation["saved_at"].toString());

    emit messagePosted("System", metadata);
    
    LOG_INFO(QString("Conversation loaded from: %1").arg(fileName));
}

void ConversationManager::exportConversation() {
    QString fileName = QFileDialog::getSaveFileName(parentWidget,
        tr("Export Conversation"),
        QDir::homePath() + "/conversation.txt",
        tr("Text Files (*.txt);;Markdown Files (*.md);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(parentWidget, tr("Export Failed"),
            tr("Could not open file for writing: %1").arg(fileName));
        LOG_ERROR(QString("Failed to open file for export: %1").arg(fileName));
        return;
    }

    QTextStream out(&file);

    // Write header
    out << "========================================\n";
    out << tr("%1 Conversation Export\n").arg(APP_NAME);
    out << tr("Date: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    out << tr("Model: %1\n").arg(Config::instance().getModel());
    out << tr("Backend: %1\n").arg(Config::instance().getBackend());
    out << "========================================\n\n";

    // Write conversation
    out << chatDisplay->toPlainText();

    file.close();

    emit messagePosted("System", tr("Conversation exported to: %1").arg(fileName));
    
    LOG_INFO(QString("Conversation exported to: %1").arg(fileName));
}

void ConversationManager::setModified(bool modified) {
    if (conversationModified != modified) {
        conversationModified = modified;
        emit modificationStateChanged(modified);
    }
}

void ConversationManager::clearCurrentFile() {
    currentConversationFile.clear();
    emit conversationChanged();
}

bool ConversationManager::promptToSaveIfModified(const QString &operation) {
    if (!conversationModified) {
        return true; // No unsaved changes, proceed
    }

    QMessageBox::StandardButton reply = QMessageBox::question(parentWidget,
        operation,
        tr("Current conversation has unsaved changes. Save before continuing?"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (reply == QMessageBox::Cancel) {
        return false; // User cancelled operation
    } else if (reply == QMessageBox::Yes) {
        saveConversation();
    }

    return true; // Proceed with operation
}
