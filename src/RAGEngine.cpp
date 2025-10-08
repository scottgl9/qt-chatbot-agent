/**
 * RAGEngine.cpp - Document indexing and retrieval engine
 * 
 * Handles document ingestion and chunking, embedding generation via Ollama,
 * vector similarity search, and document metadata management.
 */

#include "RAGEngine.h"
#include "Logger.h"
#include "Config.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QProcess>
#include <cmath>

// Conditionally include FAISS if available
#ifdef HAVE_FAISS
#include <faiss/IndexFlatL2.h>
#else
// Stub for when FAISS is not available
namespace faiss {
    struct IndexFlatL2 {
        IndexFlatL2(int) {}
        void add(int, const float*) {}
        void search(int, const float*, int, float*, long*) {}
    };
}
#endif

RAGEngine::RAGEngine(QObject *parent)
    : QObject(parent)
    , m_embeddingModel("nomic-embed-text")  // Default Ollama embedding model
    , m_apiUrl("http://localhost:11434/api/embeddings")
    , m_chunkSize(512)  // Characters per chunk
    , m_chunkOverlap(50)  // Overlap between chunks
    , m_embeddingDimension(768)  // Default for nomic-embed-text
    , m_index(nullptr)
    , m_networkManager(new QNetworkAccessManager(this)) {

    LOG_INFO("RAGEngine initialized");
    LOG_INFO(QString("Embedding model: %1").arg(m_embeddingModel));
    LOG_INFO(QString("Chunk size: %1 characters").arg(m_chunkSize));
}

RAGEngine::~RAGEngine() {
    LOG_INFO("RAGEngine destroyed");
}

void RAGEngine::setEmbeddingModel(const QString &modelName) {
    m_embeddingModel = modelName;
    LOG_INFO(QString("Embedding model set to: %1").arg(modelName));
}

void RAGEngine::setChunkSize(int size) {
    m_chunkSize = size;
    LOG_INFO(QString("Chunk size set to: %1").arg(size));
}

void RAGEngine::setChunkOverlap(int overlap) {
    m_chunkOverlap = overlap;
    LOG_INFO(QString("Chunk overlap set to: %1").arg(overlap));
}

void RAGEngine::setApiUrl(const QString &url) {
    m_apiUrl = url;
    LOG_INFO(QString("API URL set to: %1").arg(url));
}

bool RAGEngine::ingestDocument(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString error = QString("File does not exist: %1").arg(filePath);
        LOG_ERROR(error);
        emit ingestionError(filePath, error);
        return false;
    }

    LOG_INFO(QString("Ingesting document: %1").arg(filePath));

    QString content;
    QString suffix = fileInfo.suffix().toLower();

    if (suffix == "txt") {
        content = readTextFile(filePath);
    } else if (suffix == "md" || suffix == "markdown") {
        content = readMarkdownFile(filePath);
    } else if (suffix == "pdf") {
        content = readPDFFile(filePath);
    } else if (suffix == "docx" || suffix == "doc") {
        content = readDOCXFile(filePath);
    } else {
        QString error = QString("Unsupported file type: %1").arg(suffix);
        LOG_ERROR(error);
        emit ingestionError(filePath, error);
        return false;
    }

    if (content.isEmpty()) {
        QString error = "Failed to read file content";
        LOG_ERROR(error);
        emit ingestionError(filePath, error);
        return false;
    }

    // Chunk the document
    QStringList chunks = chunkText(content, filePath);
    LOG_INFO(QString("Created %1 chunks from %2").arg(chunks.size()).arg(fileInfo.fileName()));

    // Store document metadata
    m_documents[filePath] = chunks.size();

    // Generate embeddings for each chunk
    for (int i = 0; i < chunks.size(); ++i) {
        emit ingestionProgress(i + 1, chunks.size());
        generateEmbedding(chunks[i], m_chunks.size() + i);
    }

    emit documentIngested(filePath, chunks.size());
    return true;
}

bool RAGEngine::ingestDirectory(const QString &dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        LOG_ERROR(QString("Directory does not exist: %1").arg(dirPath));
        return false;
    }

    QStringList filters;
    filters << "*.txt" << "*.md" << "*.markdown" << "*.pdf" << "*.docx" << "*.doc";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    LOG_INFO(QString("Ingesting %1 files from directory: %2").arg(files.size()).arg(dirPath));

    int successCount = 0;
    for (const QFileInfo &fileInfo : files) {
        if (ingestDocument(fileInfo.absoluteFilePath())) {
            successCount++;
        }
    }

    LOG_INFO(QString("Successfully ingested %1/%2 files").arg(successCount).arg(files.size()));
    return successCount > 0;
}

void RAGEngine::clearDocuments() {
    LOG_INFO("Clearing all documents and embeddings");
    m_chunks.clear();
    m_embeddings.clear();
    m_documents.clear();
    m_pendingEmbeddings.clear();

    // Reset FAISS index
    if (m_index) {
        m_index.reset(new faiss::IndexFlatL2(m_embeddingDimension));
    }
}

QString RAGEngine::readTextFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return QString();
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString content = in.readAll();
    file.close();

    LOG_DEBUG(QString("Read %1 characters from %2").arg(content.length()).arg(filePath));
    return content;
}

QString RAGEngine::readMarkdownFile(const QString &filePath) {
    // For now, treat markdown files as plain text
    // TODO: Strip markdown formatting if desired
    return readTextFile(filePath);
}

QString RAGEngine::readPDFFile(const QString &filePath) {
    LOG_INFO(QString("Extracting text from PDF: %1").arg(filePath));

    // Use pdftotext command-line tool to extract text
    QStringList args;
    args << filePath << "-";  // "-" means output to stdout

    QString content = runCommandLineExtractor("pdftotext", args, filePath);

    if (!content.isEmpty()) {
        LOG_DEBUG(QString("Extracted %1 characters from PDF").arg(content.length()));
    } else {
        LOG_WARNING(QString("No content extracted from PDF: %1").arg(filePath));
    }

    return content;
}

QString RAGEngine::readDOCXFile(const QString &filePath) {
    LOG_INFO(QString("Extracting text from DOCX: %1").arg(filePath));

    // Use docx2txt command-line tool to extract text (outputs to stdout by default)
    QStringList args;
    args << filePath;

    QString content = runCommandLineExtractor("docx2txt", args, filePath);

    if (!content.isEmpty()) {
        LOG_DEBUG(QString("Extracted %1 characters from DOCX").arg(content.length()));
    } else {
        LOG_WARNING(QString("No content extracted from DOCX: %1").arg(filePath));
    }

    return content;
}

QString RAGEngine::runCommandLineExtractor(const QString &command, const QStringList &args, const QString &filePath) {
    QProcess process;
    process.start(command, args);

    if (!process.waitForStarted(5000)) {
        LOG_ERROR(QString("Failed to start %1 for: %2").arg(command, filePath));
        return QString();
    }

    if (!process.waitForFinished(30000)) {
        LOG_ERROR(QString("%1 timeout for: %2").arg(command, filePath));
        process.kill();
        return QString();
    }

    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        LOG_ERROR(QString("%1 failed for %2: %3").arg(command, filePath, errorOutput));
        return QString();
    }

    // Read output from stdout
    QByteArray outputData = process.readAllStandardOutput();
    QString content = QString::fromUtf8(outputData);

    return content.trimmed();
}

QStringList RAGEngine::chunkText(const QString &text, const QString &sourceFile) {
    QStringList chunks;

    int textLength = text.length();
    int position = 0;
    int chunkIndex = 0;

    while (position < textLength) {
        // Determine chunk end position
        int chunkEnd = qMin(position + m_chunkSize, textLength);

        // Try to break at a sentence or word boundary
        if (chunkEnd < textLength) {
            // Look for sentence boundary (. ! ?)
            int sentenceBreak = text.lastIndexOf(QRegExp("[.!?]\\s"), chunkEnd);
            if (sentenceBreak > position && (chunkEnd - sentenceBreak) < 100) {
                chunkEnd = sentenceBreak + 1;
            } else {
                // Fallback to word boundary
                int wordBreak = text.lastIndexOf(QRegExp("\\s"), chunkEnd);
                if (wordBreak > position) {
                    chunkEnd = wordBreak;
                }
            }
        }

        // Extract chunk
        QString chunk = text.mid(position, chunkEnd - position).trimmed();

        if (!chunk.isEmpty()) {
            // Create document chunk structure
            DocumentChunk docChunk;
            docChunk.text = chunk;
            docChunk.sourceFile = sourceFile;
            docChunk.chunkIndex = chunkIndex++;
            docChunk.metadata = QString("Length: %1 chars").arg(chunk.length());

            m_chunks.append(docChunk);
            chunks.append(chunk);
        }

        // Move position forward with overlap
        position = chunkEnd - m_chunkOverlap;
        if (position < 0) position = chunkEnd;
    }

    LOG_DEBUG(QString("Chunked text into %1 chunks").arg(chunks.size()));
    return chunks;
}

void RAGEngine::generateEmbedding(const QString &text, int chunkIndex) {
    // Build request body
    QJsonObject requestBody;
    requestBody["model"] = m_embeddingModel;
    requestBody["prompt"] = text;

    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson();

    // Create network request (use temporary QUrl to avoid vexing parse)
    QUrl url(m_apiUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Store pending embedding
    m_pendingEmbeddings[chunkIndex] = text;

    // Send request
    QNetworkReply *reply = m_networkManager->post(request, data);

    // Connect reply to handler
    connect(reply, &QNetworkReply::finished, this, [this, reply, chunkIndex]() {
        handleEmbeddingResponse(reply, chunkIndex);
    });

    LOG_DEBUG(QString("Generating embedding for chunk %1").arg(chunkIndex));
}

void RAGEngine::handleEmbeddingResponse(QNetworkReply *reply, int chunkIndex) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR(QString("Embedding generation failed for chunk %1: %2")
                  .arg(chunkIndex).arg(reply->errorString()));
        m_pendingEmbeddings.remove(chunkIndex);
        return;
    }

    // Parse response
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (!obj.contains("embedding")) {
        LOG_ERROR(QString("Invalid embedding response for chunk %1").arg(chunkIndex));
        m_pendingEmbeddings.remove(chunkIndex);
        return;
    }

    // Extract embedding vector
    QJsonArray embeddingArray = obj["embedding"].toArray();
    QVector<float> embedding;
    embedding.reserve(embeddingArray.size());

    for (const QJsonValue &val : embeddingArray) {
        embedding.append(val.toDouble());
    }

    // Initialize FAISS index if needed
    if (!m_index) {
        m_embeddingDimension = embedding.size();
        m_index.reset(new faiss::IndexFlatL2(m_embeddingDimension));
        LOG_INFO(QString("Initialized FAISS index with dimension %1").arg(m_embeddingDimension));
    }

    // Store embedding and add to index
    addEmbeddingToIndex(embedding, chunkIndex);
    m_pendingEmbeddings.remove(chunkIndex);

    emit embeddingGenerated(chunkIndex);
    LOG_DEBUG(QString("Generated embedding for chunk %1 (dim: %2)")
              .arg(chunkIndex).arg(embedding.size()));
}

void RAGEngine::addEmbeddingToIndex(const QVector<float> &embedding, int chunkIndex) {
    // Ensure we have enough space in embeddings vector
    if (chunkIndex >= m_embeddings.size()) {
        m_embeddings.resize(chunkIndex + 1);
    }

    // Store embedding
    m_embeddings[chunkIndex] = embedding;

    // Add to FAISS index
    if (m_index) {
        m_index->add(1, embedding.constData());
    }
}

QStringList RAGEngine::retrieveContext(const QString &query, int topK) {
    if (m_chunks.isEmpty()) {
        LOG_WARNING("No documents ingested yet");
        emit queryError("No documents ingested yet");
        return QStringList();
    }

    if (m_embeddings.isEmpty()) {
        LOG_WARNING("No embeddings available yet - documents may still be processing");
        emit queryError("Embeddings not ready yet");
        return QStringList();
    }

    LOG_INFO(QString("Retrieving top %1 contexts for query").arg(topK));

    // Generate embedding for query asynchronously
    generateQueryEmbedding(query, topK);

    // Return empty list immediately - results will come via contextRetrieved signal
    return QStringList();
}

QVector<int> RAGEngine::searchSimilar(const QVector<float> &queryEmbedding, int topK) {
    QVector<int> results;

    if (!m_index || m_embeddings.isEmpty()) {
        return results;
    }

    // Prepare result buffers
    QVector<float> distances(topK);
    QVector<long> indices(topK);

    // Perform FAISS search
    m_index->search(1, queryEmbedding.constData(), topK,
                    distances.data(), indices.data());

    // Convert indices to QVector<int>
    for (int i = 0; i < topK; ++i) {
        if (indices[i] >= 0 && indices[i] < m_chunks.size()) {
            results.append(static_cast<int>(indices[i]));
        }
    }

    return results;
}

void RAGEngine::generateQueryEmbedding(const QString &query, int topK) {
    // Build request body
    QJsonObject requestBody;
    requestBody["model"] = m_embeddingModel;
    requestBody["prompt"] = query;

    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson();

    // Create network request
    QUrl url(m_apiUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Send request
    QNetworkReply *reply = m_networkManager->post(request, data);

    // Connect reply to handler
    connect(reply, &QNetworkReply::finished, this, [this, reply, topK]() {
        handleQueryEmbeddingResponse(reply, topK);
    });

    LOG_DEBUG(QString("Generating query embedding with topK=%1").arg(topK));
}

void RAGEngine::handleQueryEmbeddingResponse(QNetworkReply *reply, int topK) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Query embedding generation failed: %1").arg(reply->errorString());
        LOG_ERROR(errorMsg);
        emit queryError(errorMsg);
        return;
    }

    // Parse response
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (!obj.contains("embedding")) {
        QString errorMsg = "Invalid query embedding response";
        LOG_ERROR(errorMsg);
        emit queryError(errorMsg);
        return;
    }

    // Extract embedding vector
    QJsonArray embeddingArray = obj["embedding"].toArray();
    QVector<float> queryEmbedding;
    queryEmbedding.reserve(embeddingArray.size());

    for (const QJsonValue &val : embeddingArray) {
        queryEmbedding.append(val.toDouble());
    }

    LOG_DEBUG(QString("Query embedding generated (dim: %1)").arg(queryEmbedding.size()));

    // Perform similarity search
    QVector<int> indices = searchSimilar(queryEmbedding, topK);

    // Build result context list
    QStringList contexts;
    for (int idx : indices) {
        if (idx >= 0 && idx < m_chunks.size()) {
            contexts.append(m_chunks[idx].text);
            LOG_DEBUG(QString("Retrieved chunk %1 from %2")
                      .arg(m_chunks[idx].chunkIndex)
                      .arg(m_chunks[idx].sourceFile));
        }
    }

    LOG_INFO(QString("Retrieved %1 relevant contexts").arg(contexts.size()));
    emit contextRetrieved(contexts);
}
