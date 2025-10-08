# 🧠 PRD — Qt5 Chatbot Application with MCP and RAG Integration

**Product Name:** `QtRAGBot`  
**Platform:** Ubuntu 24.04+  
**Primary Language:** C++ (Qt5 Framework)  
**LLM Backend:** Ollama (default), OpenAI-compatible API interface  
**Version:** 1.0  
**Owner:** Engineering (AI Systems Team)  
**Last Updated:** October 2025

---

## 1. Overview

`QtRAGBot` is a desktop chatbot application built with Qt5 in C++ for Linux systems. It integrates **Model Context Protocol (MCP)** and **Retrieval-Augmented Generation (RAG)** capabilities, allowing both direct LLM interaction and contextual augmentation using external data sources.

The application supports both **Graphical (UI)** and **Command-Line (CLI)** interfaces. Developers can bypass the UI for testing and debugging via CLI options. Continuous integration ensures that every code change triggers an automated build and test process to validate the codebase.

---

## 2. Objectives

- Provide a **Qt5 GUI** chatbot client that interacts with an LLM via OpenAI-compatible endpoints.
- Integrate **Ollama** as the default local LLM backend.
- Implement **MCP (Model Context Protocol)** to allow structured tool calling and contextual session handling.
- Implement **RAG (Retrieval-Augmented Generation)** for context-based responses using a vector database (e.g., SQLite + Faiss).
- Provide **CLI testing options** for developers to interact with the LLM and RAG pipeline without launching the GUI.
- Maintain **automated builds**, **unit tests**, and **logging** to ensure quality and reliability.

---

## 3. Key Features

### 3.1 Chatbot UI
- **Main Chat Window:**  
  - Text input and response display (Markdown-enabled).  
  - Context window management.  
  - Message history and session persistence.  
  - Theme: Light/Dark mode toggle.  

- **Model Selector:**  
  - Default: `Ollama` (local backend).  
  - Option to specify OpenAI-compatible API endpoint via settings.

- **Context Manager:**  
  - Displays loaded documents for RAG.  
  - Allows users to add/remove files or data sources.  

- **Logs and Status Bar:**  
  - Displays active model, tokens used, and request status.  
  - Real-time logging pane for debugging.

---

### 3.2 Command-Line Interface (CLI)

Enable developers to test RAG and MCP functionality without the GUI.

**Usage Example:**
```bash
./qt-chatbot-agent --cli --prompt "Summarize this text" --context /data/docs/test.txt
```

**CLI Options:**
| Option | Description |
|--------|--------------|
| `--cli` | Run in CLI-only mode |
| `--prompt <text>` | Text prompt for the model |
| `--context <path>` | Load file or folder into RAG |
| `--model <name>` | Override default model |
| `--log-level <debug|info|warn|error>` | Set verbosity |
| `--mcp-test` | Run Model Context Protocol diagnostic |
| `--rag-test` | Test retrieval pipeline |
| `--unit-tests` | Execute unit test suite |

---

### 3.3 RAG Engine
- Uses FAISS or similar vector store.  
- Document ingestion (PDF, TXT, MD).  
- Embedding generation via Ollama embedding API or compatible OpenAI endpoint.  
- Context retrieval during chat sessions.

---

### 3.4 Model Context Protocol (MCP)
- Implements standardized message structure:
  ```json
  {
    "role": "user",
    "content": "Explain kernel CI pipelines",
    "context": {"tools": ["search", "summarize"]}
  }
  ```
- Handles tool registration and response routing.
- Supports local and networked tool calls (e.g., via HTTP or socket).

---

### 3.5 Backend Integration
- **Default:** Ollama backend via REST API (`http://localhost:11434`).  
- **Alternate:** Any OpenAI-compatible API (`https://api.openai.com/v1/chat/completions`).  
- **Config File:** `~/.qtbot/config.json`
  ```json
  {
    "backend": "ollama",
    "model": "llama3",
    "api_url": "http://localhost:11434/api/generate",
    "openai_api_key": ""
  }
  ```

---

## 4. Development and Testing Infrastructure

### 4.1 Automated Build and Test
- **Trigger:** On every git commit or pull request.
- **Pipeline:**
  1. Build project using `cmake` and `make`.
  2. Run static analysis (`clang-tidy`, `cppcheck`).
  3. Execute unit tests.
  4. Verify RAG and MCP integration tests.
  5. Generate coverage report.
  6. Deploy artifacts to `build/bin/`.

Example GitHub Actions YAML:
```yaml
on: [push, pull_request]
jobs:
  build-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install Dependencies
        run: sudo apt install qtbase5-dev libfaiss-dev nlohmann-json3-dev
      - name: Build
        run: mkdir build && cd build && cmake .. && make -j$(nproc)
      - name: Run Tests
        run: ctest --output-on-failure
```

---

### 4.2 Logging
- **Framework:** Qt logging + spdlog (optional).  
- **Log Levels:** Debug, Info, Warning, Error.  
- **File Output:** `~/.qtbot/logs/qtbot.log`.  
- **Format:**
  ```
  [2025-10-08 12:30:05][INFO] Ollama backend initialized successfully.
  ```

---

### 4.3 Unit Tests
- **Framework:** GoogleTest or QtTest.
- **Test Coverage:**
  - RAG pipeline validation (embedding + retrieval).  
  - MCP tool dispatch and response parsing.  
  - API communication (mock HTTP client).  
  - CLI argument parsing.  
  - Error handling and logging.

---

## 5. Architecture

### 5.1 Component Diagram
```
┌────────────────────┐
│     Qt5 GUI        │
│  (ChatWindow.cpp)  │
└───────┬────────────┘
        │
        ▼
┌────────────────────┐
│    ChatController  │
│ Handles user input │
└───────┬────────────┘
        │
        ▼
┌────────────────────┐
│   LLMClient        │──► REST call to Ollama / OpenAI
│   (http client)    │
└───────┬────────────┘
        │
        ▼
┌────────────────────┐
│   RAGEngine        │──► FAISS / SQLite
│   Embeddings, etc. │
└───────┬────────────┘
        │
        ▼
┌────────────────────┐
│   MCPHandler       │──► Tools / Plugins
└────────────────────┘
```

---

## 6. Deliverables and Milestones

| Phase | Deliverable | Description | ETA |
|-------|--------------|--------------|-----|
| Phase 1 | Core Chat UI | Qt5 GUI + LLM integration | Week 2 |
| Phase 2 | MCP + RAG Modules | Add context integration | Week 4 |
| Phase 3 | CLI Interface | Developer CLI support | Week 5 |
| Phase 4 | Unit Tests & Logging | Full coverage and CI setup | Week 6 |
| Phase 5 | Release Candidate | Packaged `.deb` build | Week 7 |

---

## 7. Future Enhancements
- Vector database swapping (e.g., Weaviate, Qdrant).  
- Streaming token display.  
- Offline embeddings generation using `ggml` models.  
- Plugin SDK for MCP tool developers.  
- WebSocket-based live debugging for developers.
