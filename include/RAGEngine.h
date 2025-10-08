/**
 * RAGEngine.h - Document indexing and retrieval engine
 * 
 * Handles document ingestion, chunking, embedding generation via Ollama,
 * vector similarity search, and document metadata management.
 */

#ifndef RAGENGINE_H
#define RAGENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

// Forward declare FAISS index type to avoid header dependency
namespace faiss {
    struct IndexFlatL2;
}

// Document chunk structure
struct DocumentChunk {
    QString text;
    QString sourceFile;
    int chunkIndex;
    QString metadata;
};

class RAGEngine : public QObject {
    Q_OBJECT

public:
    explicit RAGEngine(QObject *parent = nullptr);
    ~RAGEngine();

    // Document ingestion
    bool ingestDocument(const QString &filePath);
    bool ingestDirectory(const QString &dirPath);
    void clearDocuments();

    // Context retrieval
    QStringList retrieveContext(const QString &query, int topK = 3);

    // Statistics
    int getDocumentCount() const { return m_documents.size(); }
    int getChunkCount() const { return m_chunks.size(); }
    int getEmbeddingDimension() const { return m_embeddingDimension; }

    // Configuration
    void setEmbeddingModel(const QString &modelName);
    void setChunkSize(int size);
    void setChunkOverlap(int overlap);
    void setApiUrl(const QString &url);

signals:
    void documentIngested(const QString &filePath, int chunkCount);
    void ingestionProgress(int current, int total);
    void ingestionError(const QString &filePath, const QString &error);
    void contextRetrieved(const QStringList &contexts);
    void embeddingGenerated(int chunkIndex);
    void queryError(const QString &error);

private:
    // Document processing
    QString readTextFile(const QString &filePath);
    QString readMarkdownFile(const QString &filePath);
    QString readPDFFile(const QString &filePath);
    QString readDOCXFile(const QString &filePath);
    QString runCommandLineExtractor(const QString &command, const QStringList &args, const QString &filePath);
    QStringList chunkText(const QString &text, const QString &sourceFile);

    // Embedding generation
    void generateEmbedding(const QString &text, int chunkIndex);
    void handleEmbeddingResponse(QNetworkReply *reply, int chunkIndex);

    // Query embedding generation
    void generateQueryEmbedding(const QString &query, int topK);
    void handleQueryEmbeddingResponse(QNetworkReply *reply, int topK);

    // Vector operations
    void addEmbeddingToIndex(const QVector<float> &embedding, int chunkIndex);
    QVector<int> searchSimilar(const QVector<float> &queryEmbedding, int topK);

    // Configuration
    QString m_embeddingModel;
    QString m_apiUrl;
    int m_chunkSize;
    int m_chunkOverlap;
    int m_embeddingDimension;

    // Data storage
    QVector<DocumentChunk> m_chunks;
    QMap<QString, int> m_documents;  // filename -> chunk count
    QVector<QVector<float>> m_embeddings;

    // FAISS index for similarity search
    std::unique_ptr<faiss::IndexFlatL2> m_index;

    // Network
    QNetworkAccessManager *m_networkManager;
    QMap<int, QString> m_pendingEmbeddings;  // chunkIndex -> text
};

#endif // RAGENGINE_H
