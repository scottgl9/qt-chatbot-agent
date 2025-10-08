# RAG (Retrieval-Augmented Generation) Guide

## Overview

The qt-chatbot-agent application includes a complete RAG (Retrieval-Augmented Generation) system that allows you to provide document context to enhance the LLM's responses. RAG enables the chatbot to answer questions based on your own documents, making it useful for:

- Technical documentation Q&A
- Internal knowledge base queries
- Research paper analysis
- Code repository documentation
- Meeting notes and project information

## Architecture

### Components

1. **RAGEngine** (`include/RAGEngine.h`, `src/RAGEngine.cpp`)
   - Document ingestion and chunking
   - Embedding generation via Ollama API
   - Vector similarity search (FAISS-based)
   - Async context retrieval

2. **Config Integration** (`include/Config.h`, `src/Config.cpp`)
   - RAG settings persistence
   - JSON serialization
   - Default configurations

3. **Settings UI** (`src/SettingsDialog.cpp`)
   - User-friendly RAG configuration
   - Enable/disable toggle
   - Parameter customization

4. **Document Management** (`src/main.cpp` - ChatWindow)
   - File/directory ingestion UI
   - Document statistics
   - Clear operations

## Configuration

### Settings Location

RAG settings are stored in: `~/.qtbot/config.json`

### Available Settings

| Setting | Default | Range | Description |
|---------|---------|-------|-------------|
| `rag_enabled` | `false` | boolean | Enable/disable RAG functionality |
| `rag_embedding_model` | `nomic-embed-text` | string | Ollama embedding model name |
| `rag_chunk_size` | `512` | 128-2048 | Text chunk size in characters |
| `rag_chunk_overlap` | `50` | 0-512 | Overlap between chunks in characters |
| `rag_top_k` | `3` | 1-10 | Number of top results to retrieve |

### Configuring via UI

1. Open **File → Settings**
2. Scroll to **RAG Settings** section
3. Check **"Enable RAG (disabled by default)"**
4. Adjust settings as needed:
   - **Embedding Model**: Ollama model for embeddings
   - **Chunk Size**: How large each text chunk should be
   - **Chunk Overlap**: Overlap to maintain context between chunks
   - **Top K Results**: How many relevant chunks to retrieve
5. Click **Save**

## Prerequisites

### 1. Ollama Server

RAG requires a running Ollama server with an embedding model:

```bash
# Install Ollama (if not already installed)
curl -fsSL https://ollama.com/install.sh | sh

# Pull the embedding model
ollama pull nomic-embed-text

# Verify Ollama is running
curl http://localhost:11434/api/tags
```

### 2. FAISS (Optional but Recommended)

For efficient vector similarity search, FAISS can be installed in several ways:

#### Option 1: Build from Source (Recommended)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake libblas-dev liblapack-dev

# Clone FAISS repository
git clone https://github.com/facebookresearch/faiss.git
cd faiss

# Build and install
cmake -B build -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release .
make -C build -j$(nproc)
sudo make -C build install

# Update library cache
sudo ldconfig
```

#### Option 2: Using Conda (If Available)

```bash
conda install -c conda-forge faiss-cpu
```

#### Option 3: Using Python pip (Alternative)

```bash
pip install faiss-cpu
# Then link against the installed library
```

#### Verify Installation

```bash
# Check if FAISS is installed
ldconfig -p | grep faiss

# Rebuild the application with FAISS support
cd /path/to/qt-chatbot-agent/build
cmake ..
make

# Look for "FAISS found" in CMake output
```

**Note**: Without FAISS, the RAG engine will still work but with a simple stub implementation. This is fine for small document sets (< 100 chunks) but performance will degrade with larger collections.

### 3. Document Processing Tools

For PDF and DOCX support, the following command-line tools are required:

#### PDF Support (pdftotext)

```bash
# Install poppler-utils package (includes pdftotext)
sudo apt-get update
sudo apt-get install -y poppler-utils

# Verify installation
which pdftotext
pdftotext -v
```

#### DOCX Support (docx2txt)

```bash
# Install docx2txt via pip
pip install --user docx2txt

# Verify installation
which docx2txt
docx2txt --help
```

**Note**: Plain text (.txt) and Markdown (.md) files don't require any additional tools.

## Usage

### Step 1: Enable RAG

1. **File → Settings → RAG Settings**
2. Check **"Enable RAG"**
3. Click **Save**

### Step 2: Ingest Documents

#### Ingest Single Document

1. **RAG → Ingest Document** (or press `Ctrl+D`)
2. Select a document file:
   - `.txt` - Plain text files
   - `.md` or `.markdown` - Markdown files
   - `.pdf` - PDF files (via pdftotext)
   - `.docx` or `.doc` - Microsoft Word documents (via docx2txt)
3. Wait for ingestion to complete

#### Ingest Directory

1. **RAG → Ingest Directory** (or press `Ctrl+Shift+D`)
2. Select a directory containing documents
3. All supported files will be ingested automatically

### Step 3: Ask Questions

Simply type your question in the chat input as normal. If RAG is enabled and documents are loaded, the system will:

1. Generate an embedding for your query
2. Search for the top K most relevant document chunks
3. Prepend the relevant context to your question
4. Send the enhanced prompt to the LLM

### Step 4: Monitor RAG Status

- **Status Bar**: Shows `RAG: X docs (Y chunks)` when documents are loaded
- **RAG → View Documents** (or press `Ctrl+Shift+V`): Shows detailed statistics

## Document Management

### View Documents

**RAG → View Documents** (or press `Ctrl+Shift+V`)

Displays:
- Total document count
- Total chunk count
- Embedding dimension
- Current RAG configuration
- Enable/disable status

### Clear Documents

**RAG → Clear All Documents**

Removes all ingested documents and embeddings. Confirmation dialog shows counts before clearing.

## How RAG Works

### 1. Document Ingestion

```
Document → Text Extraction → Chunking → Embedding Generation → Vector Storage
```

**Chunking Algorithm:**
- Splits text into chunks of `chunk_size` characters
- Uses `chunk_overlap` to maintain context between chunks
- Attempts to break at sentence boundaries (`. ! ?`)
- Falls back to word boundaries if no sentence break found
- Each chunk stores source file and metadata

**Embedding Generation:**
- Each chunk sent to Ollama embedding API
- API endpoint: `/api/embeddings`
- Model: `nomic-embed-text` (768 dimensions by default)
- Async processing with network queue

### 2. Query Processing

```
User Query → Query Embedding → FAISS Search → Top K Chunks → Context Injection → LLM
```

**Context Injection Format:**
```
CONTEXT FROM DOCUMENTS:

--- Document Chunk 1 ---
[chunk text from document 1]

--- Document Chunk 2 ---
[chunk text from document 2]

--- Document Chunk 3 ---
[chunk text from document 3]

Please use the above context to answer the user's question.

USER QUESTION: [user's actual question]
```

### 3. Response Generation

The LLM receives the enhanced prompt and generates a response using both:
- Retrieved document context
- Its pre-trained knowledge
- Available tool calls (if enabled)

## Performance Optimization

### Chunk Size Selection

| Document Type | Recommended Chunk Size | Reasoning |
|---------------|------------------------|-----------|
| Technical docs | 512-768 | Balance detail and context |
| Code files | 256-512 | Keep functions/methods intact |
| Research papers | 768-1024 | Preserve paragraph structure |
| Meeting notes | 512-768 | Keep topics together |
| FAQs | 256-512 | Individual Q&A pairs |

### Top K Selection

| Use Case | Recommended Top K | Reasoning |
|----------|-------------------|-----------|
| Precise answers | 1-3 | Minimize noise |
| Broad research | 3-5 | More context |
| Exploratory | 5-10 | Maximum coverage |

### Embedding Model Selection

| Model | Dimensions | Speed | Quality | Use Case |
|-------|------------|-------|---------|----------|
| `nomic-embed-text` | 768 | Fast | High | General purpose (default) |
| `all-minilm` | 384 | Very Fast | Good | Quick prototyping |
| `bge-large` | 1024 | Slow | Very High | Production systems |

## Troubleshooting

### RAG Not Working

**Problem**: Questions don't use document context

**Solutions:**
1. Verify RAG is enabled: **File → Settings → RAG Settings**
2. Check documents are loaded: **RAG → View Documents**
3. Verify Ollama server is running: `curl http://localhost:11434/api/tags`
4. Check embedding model is pulled: `ollama list | grep nomic-embed-text`
5. Review logs: **View → Log Viewer**

### Embedding Generation Fails

**Problem**: Error messages during document ingestion

**Solutions:**
1. Check Ollama server status
2. Verify network connectivity to `localhost:11434`
3. Ensure embedding model is available
4. Check log viewer for detailed error messages

### Slow Ingestion

**Problem**: Document ingestion takes too long

**Solutions:**
1. Install FAISS for better performance: `sudo apt-get install libfaiss-dev`
2. Reduce chunk size to generate fewer embeddings
3. Ingest fewer documents
4. Use a faster embedding model

### Poor Answer Quality

**Problem**: Answers don't seem to use documents correctly

**Solutions:**
1. Increase Top K to retrieve more context
2. Adjust chunk size for better document structure
3. Increase chunk overlap to maintain context
4. Verify documents are relevant to questions
5. Check that questions are clearly written

### Out of Memory

**Problem**: Application crashes with large document sets

**Solutions:**
1. Install FAISS for efficient vector operations
2. Reduce number of ingested documents
3. Clear documents and re-ingest selectively
4. Reduce chunk size to generate fewer embeddings

## API Reference

### RAGEngine Class

```cpp
class RAGEngine : public QObject {
public:
    // Document ingestion
    bool ingestDocument(const QString &filePath);
    bool ingestDirectory(const QString &dirPath);
    void clearDocuments();

    // Context retrieval
    QStringList retrieveContext(const QString &query, int topK = 3);

    // Statistics
    int getDocumentCount() const;
    int getChunkCount() const;
    int getEmbeddingDimension() const;

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
};
```

### Config Methods

```cpp
// RAG Configuration Getters
bool getRagEnabled() const;
QString getRagEmbeddingModel() const;
int getRagChunkSize() const;
int getRagChunkOverlap() const;
int getRagTopK() const;

// RAG Configuration Setters
void setRagEnabled(bool enabled);
void setRagEmbeddingModel(const QString &model);
void setRagChunkSize(int size);
void setRagChunkOverlap(int overlap);
void setRagTopK(int topK);
```

## Testing

### Unit Tests

Run RAG engine tests:

```bash
cd build
ctest -R RAGEngineTest -V
```

**Test Coverage:**
- Configuration validation
- File reading and parsing
- Document chunking logic
- Error handling
- Signal/slot connections
- Statistics tracking
- Memory safety

**Test Results:**
- 17 unit tests
- All tests passing
- 10ms execution time

### Manual Testing

1. **Test Document Ingestion:**
   ```bash
   # Create test document
   echo "This is a test document about Qt5 development." > /tmp/test.txt

   # Launch application
   ./bin/qt-chatbot-agent

   # RAG → Ingest Document → Select /tmp/test.txt
   # Verify in chat: "Document ingested successfully. Total chunks: 1"
   ```

2. **Test Context Retrieval:**
   ```bash
   # Enable RAG in settings
   # Ask: "What is this document about?"
   # Expected: Response mentions Qt5 development
   ```

3. **Test Document Clearing:**
   ```bash
   # RAG → View Documents → Check count
   # RAG → Clear All Documents → Confirm
   # RAG → View Documents → Verify count is 0
   ```

## Best Practices

### 1. Document Organization

- **Group related documents**: Keep related files in same directory
- **Use clear filenames**: Helps identify relevant sources
- **Regular updates**: Re-ingest when documents change
- **Quality over quantity**: Focus on relevant, well-written documents

### 2. Query Formulation

- **Be specific**: "What is the Qt5 signal/slot mechanism?" vs "Tell me about Qt5"
- **Reference context**: "Based on the documentation, how do I..."
- **Ask follow-ups**: Build on previous context
- **Use keywords**: Include terms likely in documents

### 3. Configuration Tuning

- **Start with defaults**: Test before adjusting
- **Iterate on chunk size**: Match to document structure
- **Adjust Top K**: Balance precision vs. coverage
- **Monitor performance**: Use View Documents to check stats

### 4. Maintenance

- **Clear regularly**: Remove outdated documents
- **Monitor status bar**: Check document counts
- **Review logs**: Use Log Viewer to debug issues
- **Test queries**: Verify answers use current documents

## Examples

### Example 1: Technical Documentation

```bash
# Ingest Qt5 documentation
RAG → Ingest Directory → /usr/share/doc/qt5

# Enable RAG
File → Settings → RAG Settings → Enable RAG → Save

# Ask questions
"How do I create a signal in Qt5?"
"What is the difference between QWidget and QMainWindow?"
"Show me how to use QNetworkAccessManager"
```

### Example 2: Code Repository

```bash
# Ingest project README and documentation
RAG → Ingest Directory → /path/to/project/docs

# Ask about implementation
"How does the RAG engine handle embeddings?"
"What is the chunking algorithm?"
"Where is the configuration stored?"
```

### Example 3: Research Papers

```bash
# Ingest PDF research papers
RAG → Ingest Document → paper1.pdf
RAG → Ingest Document → paper2.pdf
RAG → Ingest Document → paper3.pdf

# Settings: Increase chunk size for papers
File → Settings → RAG Settings
  Chunk Size: 1024
  Top K: 5

# Ask comparative questions
"What are the main findings across these papers?"
"Compare the methodologies used"
"What are the limitations mentioned?"
```

## Limitations

1. **File Format Support**:
   - Plain text and Markdown fully supported
   - PDF support via pdftotext (text extraction only, no images or complex layouts)
   - DOCX/DOC support via docx2txt (text extraction only, no images or formatting)
   - No support for: images, tables, structured formats (JSON, XML, CSV)

2. **Language Support**:
   - Embedding models vary in multilingual support
   - Best performance with English documents
   - Test other languages with your chosen model

3. **Document Size**:
   - Large documents split into chunks
   - Very large files (>10MB) may be slow
   - Consider pre-processing huge documents

4. **Embedding Latency**:
   - Network call required for each chunk
   - Ingestion can be slow for many files
   - Async processing helps but adds complexity

5. **Context Window**:
   - Retrieved chunks use LLM context window
   - Balance Top K with available context
   - Very long contexts may be truncated

## Future Enhancements

Potential improvements for RAG system:

1. **Advanced File Support**:
   - XLSX and other spreadsheet formats
   - Image text extraction (OCR)
   - Structured data (JSON, XML, CSV)
   - Table extraction from PDF/DOCX

2. **Enhanced Search**:
   - Hybrid search (keyword + semantic)
   - Metadata filtering
   - Date range queries
   - Source ranking

3. **Caching**:
   - Persistent embedding storage
   - Incremental updates
   - Faster restart times

4. **Analytics**:
   - Query success metrics
   - Document usage statistics
   - Context relevance scoring

5. **Multi-modal**:
   - Image embeddings
   - Audio transcription
   - Video content extraction

## References

- **Ollama Documentation**: https://ollama.com/docs
- **FAISS Library**: https://github.com/facebookresearch/faiss
- **nomic-embed-text**: https://ollama.com/library/nomic-embed-text
- **RAG Research**: https://arxiv.org/abs/2005.11401

## Support

For issues, questions, or feature requests:
- Check logs: **View → Log Viewer**
- Review settings: **File → Settings → RAG Settings**
- Test connection: `curl http://localhost:11434/api/tags`
- File issues: GitHub repository issue tracker

---

**Document Version**: 1.0
**Last Updated**: 2025-10-09
**Application Version**: qt-chatbot-agent v1.0.0
