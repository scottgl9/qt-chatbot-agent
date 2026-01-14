/**
 * RAGUIManager.cpp - RAG document management UI
 * 
 * Handles document and directory ingestion dialogs, document list viewer,
 * and clear documents confirmation.
 */

#include "RAGUIManager.h"
#include "RAGEngine.h"
#include "Config.h"
#include "Logger.h"
#include <QWidget>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QFont>
#include <QDir>
#include <QFileInfo>

RAGUIManager::RAGUIManager(RAGEngine *ragEngine, QWidget *parent)
    : QObject(parent)
    , ragEngine(ragEngine)
    , parentWidget(parent) {
}

void RAGUIManager::ingestDocument() {
    QString fileName = QFileDialog::getOpenFileName(parentWidget,
        tr("Ingest Document for RAG"),
        QDir::homePath(),
        tr("Documents (*.txt *.md *.markdown *.pdf *.docx *.doc);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    LOG_INFO(QString("Ingesting document: %1").arg(fileName));
    emit statusUpdated();

    // Ingest asynchronously
    if (ragEngine && ragEngine->ingestDocument(fileName)) {
        int chunkCount = ragEngine->getChunkCount();
        emit documentIngested(QFileInfo(fileName).fileName(), chunkCount);
        emit statusUpdated();
        LOG_INFO(QString("Document ingested successfully: %1 (total chunks: %2)")
            .arg(fileName).arg(chunkCount));
    } else {
        QString error = QString("Failed to ingest document: %1").arg(fileName);
        emit ingestionFailed(error);
        LOG_ERROR(error);
    }
}

void RAGUIManager::ingestDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(parentWidget,
        tr("Ingest Directory for RAG"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    LOG_INFO(QString("Ingesting directory: %1").arg(dirPath));
    emit statusUpdated();

    // Ingest asynchronously
    if (ragEngine && ragEngine->ingestDirectory(dirPath)) {
        int chunkCount = ragEngine->getChunkCount();
        emit directoryIngested(dirPath, chunkCount);
        emit statusUpdated();
        LOG_INFO(QString("Directory ingested successfully: %1 (total chunks: %2)")
            .arg(dirPath).arg(chunkCount));
    } else {
        QString error = QString("Failed to ingest directory: %1").arg(dirPath);
        emit ingestionFailed(error);
        LOG_ERROR(error);
    }
}

void RAGUIManager::viewDocuments() {
    if (!ragEngine) {
        QMessageBox::information(parentWidget, tr("RAG Documents"), tr("RAG Engine not initialized."));
        return;
    }

    // Create dialog
    QDialog *dialog = new QDialog(parentWidget);
    dialog->setWindowTitle(tr("RAG Documents"));
    dialog->setMinimumWidth(600);
    dialog->setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    // Header
    QLabel *headerLabel = new QLabel(tr("Ingested Documents"), dialog);
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    layout->addWidget(headerLabel);

    // Statistics
    int docCount = ragEngine->getDocumentCount();
    int chunkCount = ragEngine->getChunkCount();
    int embeddingDim = ragEngine->getEmbeddingDimension();

    QString statsText = tr("Total Documents: %1 | Total Chunks: %2 | Embedding Dimension: %3")
        .arg(docCount).arg(chunkCount).arg(embeddingDim);
    QLabel *statsLabel = new QLabel(statsText, dialog);
    statsLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
    layout->addWidget(statsLabel);

    // Info text
    QTextEdit *infoText = new QTextEdit(dialog);
    infoText->setReadOnly(true);

    if (docCount == 0) {
        infoText->setPlainText(tr("No documents have been ingested yet.\n\n"
            "Use RAG → Ingest Document or RAG → Ingest Directory to add documents."));
    } else {
        QString info = tr("RAG Engine Status:\n\n")
            + tr("- Documents loaded: %1\n").arg(docCount)
            + tr("- Text chunks: %1\n").arg(chunkCount)
            + tr("- Embedding model: %1\n").arg(Config::instance().getRagEmbeddingModel())
            + tr("- Chunk size: %1 chars\n").arg(Config::instance().getRagChunkSize())
            + tr("- Chunk overlap: %1 chars\n").arg(Config::instance().getRagChunkOverlap())
            + tr("- Top K retrieval: %1\n").arg(Config::instance().getRagTopK())
            + tr("\nRAG is currently %1.").arg(Config::instance().getRagEnabled() ? tr("ENABLED") : tr("DISABLED"));

        infoText->setPlainText(info);
    }

    layout->addWidget(infoText);

    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton *closeButton = new QPushButton(tr("Close"), dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->exec();
    dialog->deleteLater();
}

void RAGUIManager::clearDocuments() {
    if (!ragEngine) {
        return;
    }

    if (ragEngine->getDocumentCount() == 0) {
        QMessageBox::information(parentWidget, tr("Clear Documents"), tr("No documents to clear."));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(parentWidget,
        tr("Clear Documents"),
        tr("Clear all %1 ingested documents (%2 chunks)?")
            .arg(ragEngine->getDocumentCount())
            .arg(ragEngine->getChunkCount()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        ragEngine->clearDocuments();
        emit documentsCleared();
        emit statusUpdated();
        LOG_INFO("All RAG documents cleared");
    }
}
