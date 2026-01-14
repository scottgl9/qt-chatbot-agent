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

## Phase 8: Agentic Capabilities

### Priority 1: Core Agentic Infrastructure
- [ ] **Multi-Step Tool Loop (AgentLoop class)**
  - Create `AgentLoop` class that wraps LLMClient for autonomous execution
  - Detect if LLM response contains another tool call after processing results
  - Implement iteration loop with configurable max iterations (default: 10)
  - Add stop conditions: "done" signal, no tool calls, error, or max iterations
  - Track execution state and history across iterations
  - Files: New `src/AgentLoop.cpp`, `src/AgentLoop.h`

- [ ] **Goal Decomposition / Task Planning**
  - Create `TaskPlanner` class for breaking complex tasks into sub-tasks
  - Add planning system prompt: "Break down the task into executable steps"
  - Generate execution plan as structured JSON with steps and dependencies
  - Execute each step sequentially, collecting intermediate results
  - Support re-planning based on intermediate results or failures
  - Files: New `src/TaskPlanner.cpp`, `src/TaskPlanner.h`

- [ ] **Self-Reflection / Correction**
  - Add optional evaluation step after generating responses
  - Implement confidence scoring for agent outputs
  - Create "reflection prompt" template for self-evaluation
  - Retry with feedback if evaluation score is below threshold
  - Add reflection toggle in settings
  - Files: Modify `src/LLMClient.cpp`, new prompts in resources

### Priority 2: Memory and Context Management
- [ ] **Persistent Memory / Knowledge Base**
  - Create `MemoryManager` class with short-term and long-term memory
  - Short-term: Rolling window of last N conversation turns
  - Long-term: SQLite-backed storage for facts, preferences, entities
  - Use embeddings to retrieve relevant memories for context
  - Add internal "remember" and "recall" actions
  - Memory summarization for archiving old conversations
  - Files: New `src/MemoryManager.cpp`, `src/MemoryManager.h`

- [ ] **Scratchpad / Working Memory Tool**
  - Add scratchpad as built-in tool for agent notes during execution
  - Actions: write, read, append, clear
  - Persist scratchpad across tool calls within a single task
  - Clear automatically on task completion
  - Files: Add tool in `src/main.cpp` or new `src/ScratchpadTool.cpp`

- [ ] **Improved Context Window Management**
  - Implement smarter summarization instead of dropping old messages
  - Use embeddings to select most relevant historical context
  - Add accurate token counting for context limits
  - Support different context strategies (recency, relevance, hybrid)
  - Files: Modify `src/LLMClient.cpp`

### Priority 3: Extended Tool Capabilities
- [ ] **File System Tools**
  - `read_file`: Read contents of a file (with size limits)
  - `write_file`: Write/create files (with confirmation for new files)
  - `list_directory`: List files and folders in a directory
  - `search_files`: Search for files by name pattern or content
  - Implement sandboxing with allowed directories (configurable)
  - Add confirmation prompts for destructive operations
  - Files: New `src/FileSystemTools.cpp`, `src/FileSystemTools.h`

- [ ] **Web Browsing / Search Tools**
  - `web_search`: Search the web using DuckDuckGo or SearXNG API
  - `fetch_url`: Retrieve and parse web page content
  - Convert HTML to markdown for readability
  - Implement rate limiting and response caching
  - Handle redirects, timeouts, and error pages gracefully
  - Files: New `src/WebTools.cpp`, `src/WebTools.h`

- [ ] **Code Execution Tool (Sandboxed)**
  - `execute_code`: Run Python/Bash scripts in isolated environment
  - Use subprocess with configurable timeout (default: 30s)
  - Resource limits (memory, CPU time)
  - Capture stdout, stderr, and exit code
  - Optional Docker/container isolation for enhanced security
  - Files: New `src/CodeExecutor.cpp`, `src/CodeExecutor.h`

- [ ] **Shell Command Execution Tool**
  - `run_command`: Execute whitelisted shell commands
  - Configurable command whitelist in settings
  - Require user confirmation for potentially dangerous operations
  - Capture output, errors, and exit status
  - Files: New `src/ShellTool.cpp`, `src/ShellTool.h`

### Priority 4: Advanced Agentic Patterns
- [ ] **Autonomous Task Execution Mode**
  - Add "agent mode" that runs with a goal and stop conditions
  - Background thread execution with progress updates
  - Notification system for user attention/confirmation needed
  - Pause/resume/stop controls in UI
  - Task queue for sequential autonomous tasks
  - Files: New `src/AutonomousAgent.cpp`, `src/AutonomousAgent.h`

- [ ] **Multi-Agent Coordination (Future)**
  - Create specialized agent personas (researcher, coder, critic, etc.)
  - Implement message passing between agents
  - Add orchestrator agent for coordination
  - Support parallel agent execution where applicable
  - Files: New `src/MultiAgent.cpp`, architecture refactoring

- [ ] **Tool Creation / Self-Extension (Future)**
  - Allow agent to write Python scripts as new tools
  - Dynamic tool registration at runtime
  - Persist user-created tools in config
  - Tool testing and validation before registration
  - Files: New `src/DynamicToolManager.cpp`

### Priority 5: Infrastructure Improvements
- [ ] **Structured Output / JSON Mode**
  - Add JSON mode to Ollama/OpenAI requests (`format: "json"`)
  - Validate LLM responses against expected JSON schema
  - Automatic retry on invalid JSON with error feedback
  - Files: Modify `src/LLMClient.cpp`

- [ ] **Agent Execution Logging and Observability**
  - Detailed structured logging for each agent action
  - Export execution traces as JSON for debugging
  - Visualization of agent decision tree in UI
  - Performance metrics (tokens used, latency, tool calls)
  - Files: Modify `src/Logger.cpp`, new `src/AgentTracer.cpp`

- [ ] **Streaming Tool Results**
  - Support streaming output from long-running tools
  - Progressive UI updates during tool execution
  - Cancel/abort support for running tools
  - Files: Modify `src/MCPHandler.cpp`, `src/ChatWindow.cpp`

## Agentic Features Implementation Roadmap

| Phase | Features | Estimated Time |
|-------|----------|----------------|
| Phase 8.1 | Multi-Step Tool Loop | 1-2 weeks |
| Phase 8.2 | File System + Web Tools | 2-3 weeks |
| Phase 8.3 | Memory System | 1-2 weeks |
| Phase 8.4 | Planning + Reflection | 2 weeks |
| Phase 8.5 | Autonomous Mode | 2 weeks |
| Phase 8.6 | Code Execution + Shell | 2 weeks |
| Phase 8.7 | Advanced Patterns | 4+ weeks |

## Future Enhancements
- [ ] Vector database swapping (Weaviate, Qdrant)
- [ ] Offline embeddings with ggml models
- [ ] Plugin SDK for MCP tool developers
- [ ] WebSocket-based live debugging
- [ ] Multi-language support (i18n with Qt lupdate/lrelease)
- [ ] Voice input/output support
- [ ] Image understanding (vision models)
- [ ] Clipboard integration (paste images, files)
- [ ] System tray with quick actions
- [ ] Keyboard shortcuts customization

---
**Last Updated:** 2026-01-14
