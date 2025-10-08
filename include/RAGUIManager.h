/**
 * RAGUIManager.h - RAG document management UI manager
 * 
 * Handles document/directory ingestion dialogs, document list viewing,
 * and clear operations. Emits signals for ingestion events.
 */

#ifndef RAGUIMANAGER_H
#define RAGUIMANAGER_H

#include <QObject>
#include <QString>

class QWidget;
class RAGEngine;

/**
 * @brief Manages RAG-related UI operations
 * 
 * Handles all RAG user interface operations including:
 * - Document ingestion (single file or directory)
 * - Viewing ingested documents and statistics
 * - Clearing document store
 * - RAG status display
 */
class RAGUIManager : public QObject {
    Q_OBJECT

public:
    explicit RAGUIManager(RAGEngine *ragEngine, QWidget *parent = nullptr);
    ~RAGUIManager() override = default;

    // RAG operations
    void ingestDocument();
    void ingestDirectory();
    void viewDocuments();
    void clearDocuments();

signals:
    void documentIngested(const QString &filename, int chunkCount);
    void directoryIngested(const QString &path, int chunkCount);
    void documentsCleared();
    void ingestionFailed(const QString &error);
    void statusUpdated();

private:
    RAGEngine *ragEngine;
    QWidget *parentWidget;
};

#endif // RAGUIMANAGER_H
