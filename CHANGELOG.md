# Changelog

All notable changes to qt-chatbot-agent will be documented in this file.

# Changelog

All notable changes to qt-chatbot-agent will be documented in this file.

## [Unreleased] - 2025-01-14

### Added

#### Lemonade Backend Support (2025-01-14)
- **Lemonade AI Server Integration**
  - Added support for Lemonade server (https://lemonade-server.ai/)
  - OpenAI-compatible API endpoint: `http://localhost:8000/api/v1/chat/completions`
  - Model listing via `/api/v1/models` endpoint
  - Auto-refresh models on Settings dialog open
  - Support for GPU/NPU accelerated models (GGUF, ONNX, FLM formats)
  - Particularly optimized for AMD Ryzen AI hardware (Strix Halo)

- **Multi-Backend Model Fetching**
  - Backend dropdown now includes: Ollama, Lemonade, and OpenAI
  - Automatic API URL placeholder updates based on selected backend
  - Model refresh button enabled for both Ollama and Lemonade
  - Silent auto-refresh prevents popup spam on dialog open

- **Documentation Updates**
  - Updated README.md with Lemonade backend information
  - Added API endpoint examples for all three backends
  - Updated prerequisites to include Lemonade as an option

## [Unreleased] - 2025-10-10

### Added

#### Context Window Management (2025-10-10)
- **Token-Based History Pruning**
  - Automatic conversation history management
  - Token estimation using ~4 chars/token heuristic
  - Intelligent pruning to fit within context window limits
  - Preserves system prompt and most recent messages
  - Uses 80% of context window for input, reserves 20% for response

- **Implementation Details**
  - `estimateTokens()` - Estimates token count from text length
  - `pruneMessageHistoryForContext()` - Prunes history to fit budget
  - Works backwards from most recent messages
  - Accounts for system prompt, tool overhead, current message
  - Detailed logging of pruning actions (messages kept/dropped)

- **Benefits**
  - Prevents silent truncation by Ollama (which truncates from beginning)
  - Maintains conversation context within model limits
  - Predictable behavior with long conversations
  - Configurable via `context_window_size` setting
  - No user intervention required (automatic)

- **Manual Control**
  - `clearConversationHistory()` - Public API to reset history
  - Edit → Clear Conversation (Ctrl+L) - UI method to clear

- **Logging Examples**
  ```
  [INFO] Pruned message history: kept 8/12 messages (2847 tokens), dropped 4 oldest
  [DEBUG] Message history fits in context: 4 messages (512 tokens)
  [WARNING] Current message exceeds context budget! Message tokens: 3500, budget: 3200
  ```

- **Documentation**
  - Updated `docs/conversation-management.md` with context management section
  - Updated `README.md` with context window management features
  - Updated `docs/QUICK_REFERENCE.md` with configuration and usage

#### Tool Cleanup (2025-10-10)
- **Removed Underutilized Tools**
  - Removed `greeter` tool (multi-language greeting)
  - Removed `text_stats` tool (text analysis)
  - Removed `random_number` tool (random generation)
  - Reduced from 5 local tools to 2 (calculator, datetime)

- **Benefits**
  - Simpler tool set for users
  - Reduced token overhead in tool definitions
  - Focus on essential functionality
  - Cleaner codebase

#### Configuration Changes (2025-10-10)
- **New Config Directory**
  - Changed from `~/.config/qtragbot/` to `~/.qtbot/`
  - Simpler, shorter path
  - Consistent with application name
  - Config: `~/.qtbot/config.json`
  - Logs: `~/.qtbot/logs/qtbot.log`

- **Default Model Update**
  - Changed default from `llama3` to `gpt-oss:20b`
  - Matches actual deployment environment
  - Prevents timeout on missing model

#### Test MCP Stdio Server
- **Command Line Interface**
  - New `--test-mcp-stdio` command line argument
  - Launches built-in MCP server in stdio mode
  - Uses QCoreApplication for headless operation
  - Clean shutdown on stdin EOF

- **Server Features**
  - JSON-RPC 2.0 protocol over stdin/stdout
  - MCP-compliant server implementation
  - Three test tools: hello, echo, reverse_string
  - Proper error handling with JSON-RPC error codes
  - Debug logging to stderr (stdout reserved for JSON responses)

- **Supported Methods**
  - `initialize` - Server initialization with capabilities
  - `tools/list` - List all available tools
  - `tools/call` - Execute a specific tool

- **Use Cases**
  - Development testing without external servers
  - Integration testing in CI/CD pipelines
  - Protocol compliance validation
  - MCP client development and debugging

- **Documentation**
  - Complete protocol reference (docs/mcp-stdio-server.md)
  - Python and Node.js client examples
  - Automated testing examples
  - Debugging guide

#### System Prompt Configuration
- **Configurable System Prompt**
  - Full system prompt editor in Settings dialog
  - Multi-line text editor (QTextEdit) for composing prompts
  - System prompt now properly sent to Ollama API
  - Persists to config.json file
  - Default: "You are a helpful AI assistant with access to tools..."

- **Enhanced Tool Calling**
  - System prompt automatically enhanced when tools are enabled
  - Structured tool descriptions added to system context
  - Clear JSON format instructions for tool invocation
  - "AVAILABLE TOOLS" section with detailed specifications
  - Improved compliance with tool calling protocol

- **Integration**
  - System prompt included in all LLM requests (Ollama 'system' field)
  - Enhanced system prompt for tool-enabled requests
  - Debug logging for system prompt inclusion
  - Comprehensive documentation in docs/system-prompt-configuration.md

#### Status Bar
- **Connection Information Display**
  - Permanent status bar at bottom of window
  - Shows backend (Ollama/OpenAI)
  - Shows current model name
  - Shows server address for Ollama (host:port)
  - Shows number of registered MCP tools
  - Auto-updates when settings change

#### Tool Call Visualization Improvements
- **Enhanced Tool Call Indicators**
  - Clear, prominent headline when tool is called (e.g., "Calling tool: calculator")
  - Tool name highlighted in color (blue for calling, green for success, red for errors)
  - Two-level display: simple message + detailed information box
  - Indented details boxes for visual hierarchy
  - Separate messages for call, completion, and failure states

- **Visual Design**
  - Color-coded states: Blue (calling), Green (success), Red (error)
  - Icons for quick scanning: (calling), (success), (error)
  - Monospace font for JSON data
  - Pretty-printed results with truncation for long outputs
  - Consistent spacing and indentation

#### Additional MCP Tools
- **DateTime Tool** (`datetime`)
  - Get current date and time in multiple formats
  - Supports short, long, ISO, and timestamp formats
  - Includes timezone information
  - Use cases: timestamping, scheduling, logging

- **Text Statistics Tool** (`text_stats`)
  - Analyze text for detailed statistics
  - Character count, word count, line count
  - Uppercase/lowercase letter counts
  - Digit and space counts
  - Use cases: text analysis, validation, reporting

- **Random Number Tool** (`random_number`)
  - Generate random numbers in specified range
  - Support for multiple numbers at once
  - Customizable min/max values
  - Use cases: random selection, games, testing

#### UI Enhancements
- **Extended Markdown Formatting**
  - Bold text with `**text**`
  - Italic text with `*text*` or `_text_`
  - Inline code with `` `code` ``
  - Code blocks with ` ```code``` `
  - Bullet lists with `-` or `*`
  - Numbered lists with `1.` `2.` `3.`
  - Styled code blocks with gray background and padding
  - Inline code with pink text highlighting

- **Animated Thinking Indicator**
  - Shows "Thinking..." with animated dots while waiting for LLM
  - Updates every 500ms (0, 1, 2, 3 dots)
  - Auto-shows when sending message
  - Auto-hides when first token arrives or on error
  - Subtle italic gray styling

- **Enhanced Tool Call Visualization**
  - Color-coded styled boxes for different events
  - Blue box for tool calls with parameters
  - Green box for successful results
  - Red box for errors
  - Formatted JSON output in monospace
  - Truncated results for readability (200 char limit)

#### Conversation Management
- **Clear Conversation** (Ctrl+L)
  - Clears all messages from chat
  - Confirmation dialog to prevent accidents
  - System message confirms action

- **Copy Conversation** (Ctrl+C)
  - Copies plain text to system clipboard
  - Includes all messages with timestamps
  - Confirmation message in chat

- **Export Conversation** (Ctrl+E)
  - Saves to `.txt` or `.md` file
  - Includes metadata header (date, model, backend)
  - File dialog with format selection
  - Success/failure feedback

#### Model Selection
- **Automatic Model Refresh**
  - Auto-fetches models when Settings opens (Ollama only)
  - Silent background operation
  - No popup interruptions

- **Manual Model Refresh**
  - Refresh button in Settings dialog
  - Shows loading state
  - Success/error messages
  - Preserves current selection

#### Testing Infrastructure
- **Unit Tests**
  - Comprehensive Config class tests (7 test cases)
  - Comprehensive MCPHandler tests (7 test cases)
  - MCP Server integration tests (8 test cases) NEW
  - Qt Test framework integration
  - CTest support
  - All tests passing (100%)

- **Test MCP Server**
  - Simulates networked tool server using QTcpServer
  - HTTP protocol handling
  - JSON request/response validation
  - Error simulation capabilities
  - Tests network communication and async signals
  - Validates concurrent tool execution

#### Documentation
- `docs/markdown-formatting-guide.md` - Complete markdown syntax guide
- `docs/conversation-management.md` - Conversation features documentation
- `docs/recent-updates.md` - Recent feature summary
- `docs/model-selection-feature.md` - Model selection guide
- `docs/status-bar-and-tools.md` - Status bar and additional tools guide
- `docs/tool-call-visualization.md` - Tool call visualization documentation
- `docs/mcp-testing.md` - MCP testing guide and test server documentation
- `CHANGELOG.md` - This file

### Fixed
- **Critical Segmentation Fault on Exit (2025-10-09)**
  - Fixed X11/XCB buffer overflow crash in `xcb_shm_detach_checked()` during widget cleanup
  - Root cause: Qt 5.15/X11 platform bug in shared memory handling during QBackingStore destruction
  - Solution: Manual widget cleanup in `closeEvent()` before Qt's automatic destruction
  - Implemented explicit deletion of menu bar, central widget, and status bar
  - Called `destroy()` to forcibly release native X11 window before destructor runs
  - Use `std::exit(0)` to terminate immediately, preventing destructor from running on already-destroyed window
  - Set `QT_X11_NO_MITSHM=1` environment variable as defense-in-depth
  - Application now exits cleanly without crashes
  - Fixed log viewer race conditions with `m_isDestroying` flag
  - Fixed TOCTOU bug in customMessageHandler
  - Commits: c815fdf, 32fc279

- **Logger Initialization Deadlock (2025-10-09)**
  - Fixed application hanging on startup when message handler was installed
  - Root cause: Mutex deadlock in `Logger::init()` - `writeLog()` called while mutex already locked
  - The same thread was trying to lock the same mutex twice (line 23 lock, then line 103 lock attempt)
  - Solution: Moved mutex unlock before calling `writeLog()` in initialization
  - Used scoped block to release mutex, then called `writeLog()` outside the lock
  - Application now starts instantly without hanging
  - Logs now appear in log viewer properly
  - File: src/Logger.cpp

- **Qt Threading Issues**
  - Deferred QNetworkAccessManager initialization
  - Fixed "QSocketNotifier: Can only be used with threads started with QThread" error
  - Proper event loop initialization timing
  - Custom Logger disabled in favor of Qt's built-in logging

- **Model Selection**
  - :latest tag removal for cleaner display
  - Base URL parsing for /api/tags endpoint
  - Network error handling

- **MCP Error Handling**
  - Networked tools now check for "error" field in JSON responses
  - Error responses correctly trigger toolCallFailed signal
  - Improved error message propagation from servers

### Changed
- **Logging System**
  - Switched from custom Logger to Qt's qDebug/qInfo/qWarning/qCritical
  - LOG_* macros now use Qt logging
  - Simpler, more reliable logging

- **Tool Call Feedback**
  - Enhanced visual styling with color-coded boxes
  - Two-level display: simple headline + detailed box
  - Tool name prominently displayed in colored, bold text
  - Indented details boxes for better visual hierarchy
  - Better parameter and result display
  - Thinking indicator during tool result processing
  - Separate styling for calling, success, and error states

### Technical Improvements
- Qt 5.15.13 compatibility
- C++17 standard
- CMake 3.10+ build system
- Proper MOC handling for Qt objects
- QRegularExpression for markdown parsing
- QTimer for animations
- QClipboard integration
- QFileDialog for exports

## [1.0.0] - Initial Release

### Added
- Qt5-based GUI application
- LLM integration (Ollama, OpenAI)
- Model Context Protocol (MCP) support
- Local tool execution (calculator, greeter)
- Networked tool support
- Settings dialog
- Theme support (Light/Dark)
- Streaming token display
- Retry logic with exponential backoff
- Comprehensive logging
- Configuration management

## Version History

| Version | Date | Major Features |
|---------|------|----------------|
| 1.0.0 | 2025-10-08 | Initial release with MCP support |
| Unreleased | 2025-10-08 | Markdown, conversation mgmt, tests |

## Upcoming Features

### Planned
- [ ] RAG (Retrieval-Augmented Generation) integration
- [ ] Conversation history persistence
- [ ] Search in conversation
- [ ] Message editing/deletion
- [ ] More markdown features (headers, links, tables)
- [ ] Syntax highlighting for code blocks
- [ ] Plugin system for custom tools
- [ ] Multi-language support
- [ ] Voice input/output
- [ ] Conversation templates

### Under Consideration
- [ ] Dark theme improvements
- [ ] Custom CSS themes
- [ ] Conversation folders/organization
- [ ] Tags and labels
- [ ] Full-text search across conversations
- [ ] Export to PDF
- [ ] Import conversations
- [ ] Collaborative features
- [ ] Cloud sync (optional)

## Migration Notes

### From 1.0.0 to Unreleased
- Configuration file format unchanged
- Custom Logger usage deprecated (use Qt logging)
- New keyboard shortcuts available
- Markdown automatically applied to all messages
- Model list auto-refreshes (Ollama only)

## Known Issues

- QSocketNotifier warning at startup (harmless, doesn't affect functionality)
- Lists require newline separation (consecutive lines won't be parsed as list items)
- Code blocks don't support syntax highlighting yet
- Export uses plain text (no HTML export option)

## Support

For issues, feature requests, or questions:
- Check documentation in `docs/` directory
- Review this CHANGELOG for recent changes
- Check the README.md for usage instructions

## Statistics

- **Lines of Code**: ~2700+ (excluding tests and docs)
- **Test Files**: 3 (Config, MCPHandler, MCPServer)
- **Test Cases**: 22 total (7 Config + 7 MCPHandler + 8 MCPServer)
- **Test Success Rate**: 100% (22/22 passing)
- **Test Execution Time**: ~0.5 seconds
- **Documentation Pages**: 8
- **Supported Markdown Features**: 6
- **MCP Tools**: 2 (calculator, datetime)
- **Tool Types**: 2 (local, networked)
- **Tool Visualization States**: 3 (calling, completed, failed)
- **Keyboard Shortcuts**: 5
- **File Formats**: 2 (.txt, .md)
- **Context Window Management**: Automatic (token-based pruning)
