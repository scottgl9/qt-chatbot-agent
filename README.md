# qt-chatbot-agent

A Qt5-based chatbot application with advanced features including:
- **Model Context Protocol (MCP)** support for tool calling
- **RAG (Retrieval-Augmented Generation)** for document-based Q&A
- **Streaming responses** from LLM backends
- **Multiple backend support**: Ollama, Lemonade, OpenAI
- **Rich UI** with markdown rendering and syntax highlighting

## Features

### Core Features
- Interactive chat interface with streaming responses
- MCP tool calling support (calculator, datetime)
- RAG engine for document-based context injection
- Light/Dark theme support
- Conversation save/load/export
- In-conversation search
- Markdown rendering with code syntax highlighting
- Automatic context window management (prevents token overflow)

### RAG (Retrieval-Augmented Generation)
- Document ingestion (.txt, .md, .pdf, .docx, .doc)
- Async embedding generation via Ollama
- Vector similarity search (FAISS-based)
- Configurable chunk size and overlap
- Top-K context retrieval
- Document management UI

### MCP (Model Context Protocol)
- Local and networked tool support
- Async tool execution
- Tool result formatting
- Server-Sent Events (SSE) support
- Built-in example tools

### Context Window Management
- Automatic token-based history pruning
- Prevents silent truncation by LLM
- Preserves system prompt and recent messages
- Configurable context window size
- Detailed logging of pruning actions

## Quick Start

### Prerequisites
- Qt5 (5.15+)
- CMake (3.10+)
- C++17 compiler
- LLM server (Ollama, Lemonade, or OpenAI)

### Build from Source

```bash
# Clone repository
git clone <repository-url>
cd qt-chatbot-agent

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./bin/qt-chatbot-agent
```

### Build Debian Package

```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install -y debhelper cmake qtbase5-dev libqt5network5 qt5-qmake build-essential

# Build package (from project root)
dpkg-buildpackage -b -uc -us

# Install the package
sudo dpkg -i ../qt-chatbot-agent_1.1.0-1_amd64.deb

# Fix dependencies if needed
sudo apt-get install -f
```

**Package Details:**
- Package name: `qt-chatbot-agent`
- Binary name: `qt-chatbot-agent`
- Installed to: `/usr/bin/qt-chatbot-agent`
- See [Debian Package Guide](docs/DEBIAN_PACKAGE_GUIDE.md) for full details

### First Run

1. **Configure Backend**:
   - File → Settings
   - Select backend (Ollama/Lemonade/OpenAI)
   - Configure API URL and model

2. **Enable RAG (Optional)**:
   - File → Settings → RAG Settings
   - Check "Enable RAG"
   - Configure embedding model and parameters

3. **Ingest Documents (Optional)**:
   - RAG → Ingest Document/Directory
   - Select files to add to knowledge base

4. **Start Chatting**:
   - Type message and press Enter
   - Use tools automatically when needed
   - Questions use RAG context if enabled

## Documentation

### User Guides
- **[Quick Reference](docs/QUICK_REFERENCE.md)** - Quick start and common tasks
- **[RAG Guide](docs/RAG_GUIDE.md)** - Complete RAG system documentation
- **[MCP Tool Calling Guide](docs/MCP_TOOL_CALLING_GUIDE.md)** - MCP integration and tool usage
- **[Testing Guide](docs/TESTING_GUIDE.md)** - Testing and quality assurance

### Feature Documentation
- **[Lemonade Backend](docs/lemonade-backend.md)** - Lemonade AI server setup and configuration
- **[Conversation Management](docs/conversation-management.md)** - Save, load, and export conversations
- **[Markdown Formatting](docs/markdown-formatting-guide.md)** - Markdown rendering features
- **[Model Selection](docs/model-selection-feature.md)** - Backend and model configuration
- **[Status Bar & Tools](docs/status-bar-and-tools.md)** - UI elements and tool integration
- **[System Prompt Configuration](docs/system-prompt-configuration.md)** - Custom system prompts
- **[Tool Call Visualization](docs/tool-call-visualization.md)** - Tool execution UI

### Development Documentation
- **[AGENTS.md](AGENTS.md)** - Build and test instructions for AI agents
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Detailed architecture documentation
- **[PRD](docs/PRD.md)** - Product Requirements Document
- **[Debian Packaging](docs/DEBIAN_PACKAGE_GUIDE.md)** - Creating .deb packages
- **[MCP Testing](docs/mcp-testing.md)** - MCP protocol testing
- **[MCP Stdio Server](docs/mcp-stdio-server.md)** - MCP server implementation

### Project Status
- **[CHANGELOG.md](CHANGELOG.md)** - Version history and changes
- **[TODO.md](TODO.md)** - Implementation roadmap
- **[PROGRESS.md](PROGRESS.md)** - Development progress tracking

## Configuration

Configuration is stored at: `~/.qtbot/config.json`

### Key Settings
- **Backend**: ollama, lemonade, openai
- **Model**: Model name (e.g., llama3, Gemma-3-4b-it-GGUF, gpt-4)
- **API URL**: Backend endpoint
  - Ollama: `http://localhost:11434/api/generate`
  - Lemonade: `http://localhost:8000/api/v1/chat/completions`
  - OpenAI: `https://api.openai.com/v1/chat/completions`
- **RAG**: Enable/disable, embedding model, chunk settings
- **LLM Parameters**: Temperature, top-p, top-k, context window

## Testing

```bash
# Run all tests
cd build
ctest

# Run specific test suite
ctest -R RAGEngineTest -V
ctest -R ConfigTest -V
ctest -R MCPHandlerTest -V
```

## Architecture

### Modular Design

The codebase uses a modular architecture with clear separation of concerns:

```
qt-chatbot-agent/
├── include/                    # Header files
│   ├── ChatWindow.h            # Main window class
│   ├── ConversationManager.h   # Conversation persistence
│   ├── MessageRenderer.h       # Chat display & formatting
│   ├── ToolUIManager.h         # Tool management UI
│   ├── RAGUIManager.h          # RAG document UI
│   ├── Config.h                # Configuration management
│   ├── RAGEngine.h             # RAG implementation
│   ├── LLMClient.h             # LLM backend client
│   ├── MCPHandler.h            # MCP tool handling
│   └── ...
├── src/                        # Source files
│   ├── main.cpp                # Application entry point (300 lines)
│   ├── ChatWindow.cpp          # Main window implementation
│   ├── ConversationManager.cpp # Save/load/export logic
│   ├── MessageRenderer.cpp     # Message display & streaming
│   ├── ToolUIManager.cpp       # Tool dialogs & filtering
│   ├── RAGUIManager.cpp        # Document ingestion UI
│   ├── RAGEngine.cpp           # RAG implementation
│   ├── Config.cpp              # Configuration
│   └── ...
├── tests/                      # Unit tests
│   ├── test_ragengine.cpp
│   ├── test_config.cpp
│   ├── test_mcphandler.cpp
│   └── ...
└── docs/                       # Documentation (*.md)
```

### Key Components

**Main Window (ChatWindow)**:
- Coordinates UI components and manager classes
- Handles Qt signal/slot connections
- Manages application lifecycle

**Manager Classes**:
- `ConversationManager` - Conversation persistence (new/save/load/export)
- `MessageRenderer` - Chat display with streaming and formatting
- `ToolUIManager` - Tool selection and management UI
- `RAGUIManager` - Document ingestion and management

**Core Services**:
- `LLMClient` - Backend communication (Ollama/OpenAI)
- `MCPHandler` - Tool registration and execution
- `RAGEngine` - Document indexing and retrieval
- `Config` - Settings management

**Utilities**:
- `Logger` - Application logging
- `ThemeManager` - UI theming
- `MarkdownHandler` - Markdown parsing
- `HTMLHandler` - HTML generation

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

Copyright 2025 Scott Glover <scottgl@gmail.com>

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues and questions:
- Check the [RAG Guide](docs/RAG_GUIDE.md) for RAG-specific issues
- Review logs via View → Log Viewer
- File issues on GitHub

---

**Version**: 1.0.0
**Qt Version**: 5.15.13
**C++ Standard**: C++17
