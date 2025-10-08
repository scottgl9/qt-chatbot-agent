# TODO List for qt-chatbot-agent

This file tracks all pending implementation tasks for the qt-chatbot-agent project.

- Investigate potentially using https://github.com/hkr04/cpp-mcp for the MCP implementation

## Status Legend
- [ ] Not started
- [x] Completed
- [~] In progress

## Current Tasks

### Phase 1: Core Infrastructure (Week 1-2) - COMPLETED
- [x] Create project structure and build system
- [x] Create basic Qt5 application skeleton
- [x] Implement CLI argument parsing
- [x] Implement logging system with file output
- [x] Create configuration file handler (~/.qtbot/config.json)
- [x] Add dark/light theme support

### Phase 2: LLM Backend Integration (Week 2-3) - COMPLETED
- [x] Implement HTTP client for REST API calls
- [x] Create LLMClient class for Ollama integration
- [x] Add OpenAI-compatible API support (response parsing)
- [x] Add error handling and timeout logic
- [x] Add LLM configuration parameters (temperature, top_p, top_k, context_window_size, max_tokens)
- [x] Add Settings dialog with model selection UI component
- [x] Implement streaming token display
- [x] Add retry logic for failed requests with exponential backoff

### Phase 3: MCP Implementation (Week 3-4) - COMPLETED
- [x] Design MCPHandler class structure
- [x] Implement MCP message structure (JSON)
- [x] Add tool registration system
- [x] Implement tool dispatch and response routing
- [x] Add support for local and networked tool calls
- [x] Create MCP diagnostic tests and example tools
- [x] Integrate MCPHandler with ChatWindow UI
- [x] Add LLM tool calling integration

### Phase 4: RAG Engine (Week 4-5)
- [ ] Integrate FAISS vector database
- [ ] Implement document ingestion (PDF, TXT, MD)
- [ ] Create embedding generation pipeline
- [ ] Implement context retrieval during chat
- [ ] Add context manager UI component
- [ ] Create RAG pipeline tests

### Phase 5: UI Enhancements (Week 5-6) - MOSTLY COMPLETED
- [x] Improve chat window with markdown rendering
- [x] Add message history and session persistence
- [ ] Implement context window management
- [x] Add status bar with model info and stats
- [x] Create logs and debugging pane
- [x] Add settings dialog
- [x] Add LLM parameter override system with checkboxes

### Phase 6: Testing & CI (Week 6-7)
- [ ] Create unit test framework setup
- [ ] Write tests for LLMClient
- [ ] Write tests for MCPHandler
- [ ] Write tests for RAGEngine
- [ ] Write tests for CLI argument parsing
- [ ] Setup GitHub Actions CI pipeline
- [ ] Add static analysis (clang-tidy, cppcheck)
- [ ] Generate code coverage reports

### Phase 7: Documentation & Packaging (Week 7-8)
- [ ] Write user documentation
- [ ] Create developer guide
- [ ] Add inline code documentation
- [ ] Create .deb package
- [ ] Test on Ubuntu 24.04
- [ ] Create release notes
- [ ] Setup GitHub releases

## Future Enhancements
- [ ] Vector database swapping (Weaviate, Qdrant)
- [ ] Offline embeddings with ggml models
- [ ] Plugin SDK for MCP tool developers
- [ ] WebSocket-based live debugging
- [ ] Multi-language support
- [ ] Voice input/output support

---
**Last Updated:** 2025-10-08
