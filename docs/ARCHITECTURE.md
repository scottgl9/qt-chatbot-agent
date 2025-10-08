# Architecture Documentation

## Overview

qt-chatbot-agent uses a modular Qt5-based architecture with clear separation of concerns. The application is organized around manager classes that handle specific domains, coordinated by a main ChatWindow class.

## Design Principles

1. **Single Responsibility** - Each class has one clear purpose
2. **Signal-Slot Communication** - Qt's signal-slot mechanism for loose coupling
3. **Manager Pattern** - Domain-specific managers handle UI and business logic
4. **Separation of Concerns** - UI, business logic, and data access are separated

## Core Architecture

### Application Entry Point

**main.cpp** (~300 lines)
- Application initialization
- Command-line argument parsing
- Built-in tool definitions (`exampleCalculatorTool`, `exampleDateTimeTool`)
- Mode selection (GUI/CLI/test/server)
- ChatWindow instantiation

### Main Window

**ChatWindow** (`ChatWindow.h` / `ChatWindow.cpp`)
- Main application window (QMainWindow)
- Coordinates all manager classes and services
- Handles menu actions
- Manages Qt signal-slot connections
- Application lifecycle management

**Responsibilities:**
- Create and connect UI components
- Initialize manager classes
- Handle user actions (menu clicks, keyboard shortcuts)
- Manage application state (streaming, thinking indicator)
- Coordinate between managers and services

## Manager Classes

### ConversationManager

**Purpose:** Conversation persistence and lifecycle management

**Files:** `ConversationManager.h` / `ConversationManager.cpp`

**Responsibilities:**
- New conversation creation
- Save conversation to JSON file
- Load conversation from JSON file
- Export conversation (TXT/Markdown)
- Track modification state
- Manage current file path

**Signals:**
- `modificationStateChanged(bool)` - Emitted when modified state changes
- `conversationChanged()` - Emitted when new conversation loaded
- `messagePosted(QString, QString)` - Emitted when restoring messages

**Key Methods:**
- `newConversation()` - Clear current conversation
- `saveConversation()` - Save to file (prompt if no current file)
- `loadConversation()` - Load from file
- `exportConversation()` - Export with format choice
- `isModified()` - Check if conversation has unsaved changes
- `setModified(bool)` - Mark conversation as modified

### MessageRenderer

**Purpose:** Chat display and message formatting

**Files:** `MessageRenderer.h` / `MessageRenderer.cpp`

**Responsibilities:**
- Append messages to chat display
- Handle streaming message updates
- Format messages with HTML/Markdown
- Track message sender (for streaming updates)
- Manage cursor position during streaming

**Signals:**
- `messageAppended(QString, QString)` - Emitted when message added

**Key Methods:**
- `appendMessage(sender, message)` - Add new message
- `updateLastMessage(content, isStreaming)` - Update during streaming
- `clear()` - Clear all messages
- `toPlainText()` - Get conversation as plain text
- `toHtml()` - Get conversation as HTML
- `lastMessageSender()` - Get sender of last message

**Features:**
- Automatic scroll to bottom
- Markdown rendering (bold, italic, code, lists)
- Code syntax highlighting (pink background for inline code)
- HTML formatting with proper spacing

### ToolUIManager

**Purpose:** Tool selection and management UI

**Files:** `ToolUIManager.h` / `ToolUIManager.cpp`

**Responsibilities:**
- Display available tools dialog
- Enable/disable tool filtering
- Show tool parameters and descriptions
- Track tool state across sessions

**Key Methods:**
- `showToolsDialog()` - Display tools management dialog
- `getEnabledTools()` - Get filtered tools for LLM (QJsonArray)
- `setToolEnabled(name, enabled)` - Enable/disable specific tool
- `getToolCount()` - Get total tool count
- `getEnabledToolCount()` - Get enabled tool count

**Features:**
- Scrollable list with checkboxes
- Tool information display (name, description, parameters)
- Local/networked tool indicators
- Real-time filtering for LLM requests

### RAGUIManager

**Purpose:** RAG document management UI

**Files:** `RAGUIManager.h` / `RAGUIManager.cpp`

**Responsibilities:**
- Document ingestion dialogs
- Directory ingestion with filtering
- Document list viewer
- Clear documents confirmation

**Signals:**
- `documentIngested(filename, chunkCount)` - Document added
- `directoryIngested(path, chunkCount)` - Directory processed
- `documentsCleared()` - All documents removed
- `ingestionFailed(error)` - Ingestion error occurred
- `statusUpdated()` - Status needs refresh

**Key Methods:**
- `ingestDocument()` - Single file ingestion dialog
- `ingestDirectory()` - Directory ingestion dialog
- `viewDocuments()` - Show document list
- `clearDocuments()` - Clear all with confirmation

**Features:**
- File filters (.txt, .md, .pdf, .docx, .doc)
- Recursive directory scanning
- Document metadata display
- Chunk count tracking

## Core Services

### LLMClient

**Purpose:** Backend communication (Ollama/OpenAI)

**Files:** `LLMClient.h` / `LLMClient.cpp`

**Responsibilities:**
- Send prompts to LLM backends
- Handle streaming responses (SSE)
- Tool calling support
- Context window management
- Retry logic with exponential backoff

**Signals:**
- `responseReceived(QString)` - Complete response received
- `tokenReceived(QString)` - Streaming token received
- `errorOccurred(QString)` - Error occurred
- `toolCallRequested(name, params, callId)` - Tool call needed
- `retryAttempt(attempt, maxRetries)` - Retry in progress

**Key Methods:**
- `sendPrompt(prompt)` - Send without tools
- `sendPromptWithTools(prompt, tools)` - Send with tool definitions
- `sendToolResults(prompt, results)` - Send tool execution results
- `setModel(model)` - Change model
- `setApiUrl(url)` - Change backend URL

### MCPHandler

**Purpose:** MCP (Model Context Protocol) tool management

**Files:** `MCPHandler.h` / `MCPHandler.cpp`

**Responsibilities:**
- Tool registration (local and networked)
- Tool execution (local and HTTP)
- Server discovery and registration
- Tool result formatting

**Signals:**
- `toolCallCompleted(callId, name, result)` - Tool succeeded
- `toolCallFailed(callId, name, error)` - Tool failed

**Key Methods:**
- `registerTool(tool)` - Register a tool
- `executeToolCall(name, params)` - Execute tool
- `getRegisteredTools()` - List all tools
- `getToolsForLLM()` - Get tools as JSON array
- `discoverAndRegisterServerTools(name, url, type)` - Auto-discover from server
- `clearNetworkedTools()` - Remove all networked tools

**Features:**
- Local function execution
- HTTP tool calls via QNetworkAccessManager
- Async tool execution
- Call ID tracking
- Error handling and reporting

### RAGEngine

**Purpose:** Document indexing and retrieval

**Files:** `RAGEngine.h` / `RAGEngine.cpp`

**Responsibilities:**
- Document ingestion and chunking
- Embedding generation (via Ollama)
- Vector similarity search
- Document metadata management

**Signals:**
- `contextRetrieved(QStringList)` - Context chunks retrieved
- `documentIngested(filename, chunkCount)` - Document added
- `queryError(error)` - Query error occurred

**Key Methods:**
- `ingestDocument(path)` - Add document to index
- `ingestDirectory(path)` - Add directory recursively
- `retrieveContext(query, topK)` - Retrieve relevant chunks
- `clearDocuments()` - Remove all documents
- `getDocumentCount()` - Get document count
- `getChunkCount()` - Get total chunk count

**Features:**
- Configurable chunk size and overlap
- Multiple document format support
- Async embedding generation
- In-memory vector storage (FAISS optional)

### Config

**Purpose:** Application configuration management

**Files:** `Config.h` / `Config.cpp`

**Responsibilities:**
- Load/save configuration (JSON)
- Provide default values
- Manage settings categories (LLM, RAG, MCP)
- Validate configuration

**Key Methods:**
- `load()` - Load from `~/.qtbot/config.json`
- `save()` - Save to file
- `reset()` - Reset to defaults
- Getters/setters for all configuration values

**Configuration Categories:**
- **LLM Settings**: backend, model, API URL, parameters
- **RAG Settings**: enabled, embedding model, chunk size, top-K
- **MCP Servers**: server definitions (name, URL, type)
- **Context Window**: size, override flag

## Utility Classes

### Logger

**Purpose:** Application-wide logging

**Files:** `Logger.h` / `Logger.cpp`

**Features:**
- Singleton pattern
- Log levels (Debug, Info, Warning, Error)
- File output (`~/.qtbot/qt-chatbot-agent.log`)
- Console output (stderr)
- Qt message handler integration
- Macros: `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`

### ThemeManager

**Purpose:** UI theming

**Files:** `ThemeManager.h` / `ThemeManager.cpp`

**Features:**
- Light/Dark theme support
- System palette management
- Singleton pattern
- Qt stylesheet application

### MarkdownHandler

**Purpose:** Markdown parsing and rendering

**Files:** `MarkdownHandler.h` / `MarkdownHandler.cpp`

**Features:**
- Bold, italic, code formatting
- Code blocks with syntax highlighting
- Lists (ordered and unordered)
- HTML generation from markdown

### HTMLHandler

**Purpose:** HTML generation utilities

**Files:** `HTMLHandler.h` / `HTMLHandler.cpp`

**Features:**
- Tool call widgets (pending, success, error)
- Message formatting
- Consistent styling

## Communication Flow

### Normal Message Flow

```
User Input (ChatWindow)
    ↓
MessageRenderer.appendMessage("You", message)
    ↓
RAGEngine.retrieveContext() [if enabled]
    ↓ contextRetrieved signal
LLMClient.sendPromptWithTools()
    ↓ tokenReceived signals (streaming)
MessageRenderer.updateLastMessage() [multiple times]
    ↓ responseReceived signal
MessageRenderer.updateLastMessage() [final]
```

### Tool Call Flow

```
LLMClient receives tool call from LLM
    ↓ toolCallRequested signal
ChatWindow.handleToolCallRequest()
    ↓
MessageRenderer (show tool call widget)
    ↓
MCPHandler.executeToolCall()
    ↓ toolCallCompleted signal
ChatWindow.handleToolCallCompleted()
    ↓
LLMClient.sendToolResults()
    ↓ tokenReceived signals
MessageRenderer.updateLastMessage()
```

### Conversation Save/Load Flow

```
User: File → Save Conversation
    ↓
ConversationManager.saveConversation()
    ↓ messagePosted signal (if loading)
MessageRenderer.appendMessage()
    ↓ modificationStateChanged signal
ChatWindow.updateWindowTitle()
```

## Data Flow

### Configuration
```
Config.json → Config class → UI widgets
UI changes → Config class → Config.json
```

### Conversation
```
ChatWindow → MessageRenderer → QTextEdit (display)
User action → ConversationManager → JSON file
JSON file → ConversationManager → MessageRenderer
```

### RAG
```
Document → RAGEngine → Chunks + Embeddings
Query → RAGEngine → Top-K contexts → LLMClient
```

### Tools
```
Config → MCPHandler (registration)
LLM → ChatWindow → MCPHandler (execution)
MCPHandler → LLMClient → LLM (results)
```

## Build System

### CMake Structure

```cmake
# Main executable
add_executable(qt-chatbot-agent
    src/main.cpp
    src/ChatWindow.cpp
    src/ConversationManager.cpp
    src/MessageRenderer.cpp
    src/ToolUIManager.cpp
    src/RAGUIManager.cpp
    # ... other sources
)

# Qt5 modules
Qt5::Core
Qt5::Widgets
Qt5::Network
Qt5::Gui

# Optional: FAISS
if(faiss_FOUND)
    add_definitions(-DHAVE_FAISS)
endif()
```

### Auto-generated Files

- **MOC files**: `*_autogen/mocs_compilation.cpp` (for Q_OBJECT classes)
- **UI files**: Generated from .ui files (if any)
- **Resource files**: Generated from .qrc files (if any)

## Testing

### Unit Tests

Located in `tests/`:
- `test_config.cpp` - Config class tests
- `test_mcphandler.cpp` - MCPHandler tests
- `test_mcp_server.cpp` - Networked MCP server tests
- `test_ragengine.cpp` - RAGEngine tests
- `test_sseclient.cpp` - SSEClient tests

### Test Framework

- Qt Test framework (QTest)
- CMake CTest integration
- Signal spy for async testing

## Deployment

### Runtime Dependencies

- Qt5 libraries (Core, Widgets, Network, Gui)
- System libraries (libstdc++, glibc)
- Optional: FAISS library

### Configuration Files

- `~/.qtbot/config.json` - User configuration
- `~/.qtbot/qt-chatbot-agent.log` - Application log

### Environment Variables

- `QT_X11_NO_MITSHM=1` - Disable X11 MIT-SHM (set by main.cpp)
- `LOG_LEVEL` - Custom log level (if implemented)

## Future Architecture Considerations

### Potential Improvements

1. **Plugin System**: Dynamic tool loading without recompilation
2. **Database Backend**: Replace JSON with SQLite for conversations
3. **Async I/O**: Use QFuture/QtConcurrent for background tasks
4. **Network Abstraction**: Interface for LLM backends (easier to add new ones)
5. **View Models**: Separate data models from views (Model-View pattern)
6. **Dependency Injection**: Constructor injection for better testability

### Scalability

Current architecture supports:
- Multiple MCP servers (configured in config.json)
- Multiple tools (local and networked)
- Multiple conversations (save/load)
- Multiple document sources (RAG)

Limitations:
- Single conversation at a time
- In-memory RAG index (no persistence)
- Synchronous UI updates (could block on large operations)

## Code Metrics

| Component | Lines | Responsibility |
|-----------|-------|----------------|
| main.cpp | ~300 | Entry point, tool definitions |
| ChatWindow | ~1,000 | Main window coordination |
| ConversationManager | ~150 | Persistence |
| MessageRenderer | ~180 | Display |
| ToolUIManager | ~194 | Tool UI |
| RAGUIManager | ~96 | RAG UI |
| LLMClient | ~600 | Backend communication |
| MCPHandler | ~500 | Tool management |
| RAGEngine | ~400 | Document indexing |
| Config | ~300 | Settings |

**Total Application Code**: ~3,720 lines (down from ~5,300 before refactoring)

## Contribution Guidelines

When adding features:

1. **Choose the right component**: 
   - UI logic → Manager classes
   - Business logic → Core services
   - Utilities → Utility classes

2. **Follow Qt patterns**:
   - Use signals/slots for communication
   - Emit signals for state changes
   - Connect in parent classes

3. **Maintain separation**:
   - Don't mix UI and business logic
   - Keep managers focused on their domain
   - Use dependency injection where possible

4. **Test your changes**:
   - Add unit tests for new functionality
   - Test signal/slot connections
   - Verify UI updates correctly

---

**Last Updated**: October 10, 2025
**Version**: 1.0.0
