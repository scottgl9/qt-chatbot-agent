# Quick Reference Guide

## Command Line

### Start Application

```bash
# GUI mode (default)
./bin/qt-chatbot-agent

# Test MCP stdio server
./bin/qt-chatbot-agent --test-mcp-stdio

# CLI mode
./bin/qt-chatbot-agent --cli --prompt "Your question here"

# Help
./bin/qt-chatbot-agent --help
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New conversation |
| `Ctrl+S` | Save conversation |
| `Ctrl+O` | Load conversation |
| `Ctrl+E` | Export conversation |
| `Ctrl+,` | Open settings |
| `Ctrl+Q` | Quit application |
| `Ctrl+F` | Find in conversation |
| `Ctrl+Shift+C` | Copy conversation |
| `Ctrl+L` | Clear conversation |
| `Ctrl+T` | View available tools |

## Menu Navigation

### File Menu
- New Conversation (`Ctrl+N`)
- Save Conversation (`Ctrl+S`)
- Load Conversation (`Ctrl+O`)
- Export Conversation (`Ctrl+E`)
- Settings (`Ctrl+,`)
- Quit (`Ctrl+Q`)

### Edit Menu
- Find in Conversation (`Ctrl+F`)
- Copy Conversation (`Ctrl+Shift+C`)
- Clear Conversation (`Ctrl+L`)

### View Menu
- Theme → Light
- Theme → Dark
- Available Tools (`Ctrl+T`)

## Configuration Files

### Config Location
```
~/.qtbot/config.json
```

### View Current Config
```bash
cat ~/.qtbot/config.json | jq .
```

### Default Config
```json
{
  "backend": "ollama",
  "model": "llama3",
  "api_url": "http://localhost:11434/api/generate",
  "openai_api_key": "",
  "system_prompt": "You are a helpful AI assistant with access to tools...",
  "context_window_size": 4096,
  "temperature": 0.7,
  "top_p": 0.9,
  "top_k": 40,
  "max_tokens": 2048
}
```

## MCP Tools

### Local Tools (2)

1. **calculator** - Basic arithmetic
   - Parameters: operation, a, b
   - Operations: add, subtract, multiply, divide

2. **datetime** - Current date/time
   - Parameters: format (short/long/iso/timestamp)

### Test MCP Server Tools (3)

1. **hello** - Greeting message
2. **echo** - Echo back message
3. **reverse_string** - Reverse text

## Testing

### Run Unit Tests
```bash
cd build
ctest --output-on-failure
```

### Test MCP Server
```bash
# List tools
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq .

# Call tool
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"hello","arguments":{"name":"Test"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq .
```

## Markdown Support

| Syntax | Result |
|--------|--------|
| `**bold**` | **bold** |
| `*italic*` | *italic* |
| `` `code` `` | `code` (pink highlight) |
| ` ```code``` ` | Code block (gray background) |
| `- item` | • Bullet list |
| `1. item` | 1. Numbered list |

## Tool Calling Test Queries

```
"What is 123 multiplied by 456?"
"Calculate 999 plus 888"
"What is the current time?"
"What's the date today?"
```

## Debug Logging

```bash
# View all logs
./bin/qt-chatbot-agent 2>&1 | tee app.log

# Filter specific topics
./bin/qt-chatbot-agent 2>&1 | grep -i "system prompt"
./bin/qt-chatbot-agent 2>&1 | grep -i "tool call"
./bin/qt-chatbot-agent 2>&1 | grep -i "error"
```

## Common Tasks

### Change Model
1. Settings (`Ctrl+,`)
2. Backend → Ollama
3. Click "🔄 Refresh" to list models
4. Select model
5. Save (dialog closes silently)

### View Available Tools
1. View → Available Tools (`Ctrl+T`)
2. Scroll through list
3. Inspect parameters
4. Close

### Export Chat
1. File → Export Conversation (`Ctrl+E`)
2. Choose format (.txt or .md)
3. Select location
4. Save

### Clear Chat
1. Edit → Clear Conversation (`Ctrl+L`)
2. Confirm in dialog
3. Chat clears

## Troubleshooting

### Settings Not Saving
```bash
# Check permissions
ls -l ~/.qtbot/config.json

# View errors
./bin/qt-chatbot-agent 2>&1 | grep -i "save"
```

### Tool Calling Not Working
```bash
# Verify system prompt sent
./bin/qt-chatbot-agent 2>&1 | grep "system prompt"

# Check model supports functions
# Try: llama3.1, mistral, command-r
```

### Application Crashes
```bash
# Check dependencies
ldd ./bin/qt-chatbot-agent

# Run with Qt debug
QT_DEBUG_PLUGINS=1 ./bin/qt-chatbot-agent
```

## Build from Source

```bash
# Configure
mkdir build && cd build
cmake ..

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo make install
```

## Environment Variables

```bash
# Log level (for custom Logger class)
export LOG_LEVEL=debug  # debug|info|warn|error

# Qt debugging
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES='*.debug=true'
```

## Status Bar Info

```
Backend: ollama | Model: gpt-oss:20b | Server: 192.168.1.150:11434 | Tools: 2
```

- **Backend**: ollama or openai
- **Model**: Current LLM model
- **Server**: Host:port (Ollama only)
- **Tools**: Number of registered MCP tools (2 local tools)

## File Locations

```
Application Binary:  build/bin/qt-chatbot-agent
Configuration:       ~/.qtbot/config.json
Logs:                stderr (use 2>&1 to capture)
Conversations:       User-specified location (.json format)
Exports:             User-specified location (.txt or .md)
```

## Code Organization

### Manager Classes

**ConversationManager** (`include/ConversationManager.h`):
- Handles conversation persistence
- New/save/load/export operations
- Modification state tracking

**MessageRenderer** (`include/MessageRenderer.h`):
- Manages chat display area
- Streaming message updates
- Markdown/HTML rendering

**ToolUIManager** (`include/ToolUIManager.h`):
- Tool selection dialog
- Enable/disable tool filtering
- Tool information display

**RAGUIManager** (`include/RAGUIManager.h`):
- Document ingestion dialogs
- Document list viewer
- Clear documents confirmation

### Core Components

**ChatWindow** (`include/ChatWindow.h`, `src/ChatWindow.cpp`):
- Main application window
- Coordinates all manager classes
- Handles menu actions and signals

**main.cpp** (300 lines):
- Application entry point
- Built-in tool definitions (calculator, datetime)
- CLI/server/test mode handling

## API Endpoints

### Ollama
```
Generate:       POST http://localhost:11434/api/generate
List Models:    GET  http://localhost:11434/api/tags
```

### OpenAI (if configured)
```
Chat:           POST https://api.openai.com/v1/chat/completions
```

## Context Window Management

**Automatic History Pruning**:
- Monitors token usage in conversation
- Automatically removes oldest messages when approaching limit
- Uses 80% of context window for input, 20% for response
- Preserves system prompt and recent exchanges

**Configuration**:
```json
{
  "context_window_size": 4096,
  "override_context_window_size": false
}
```

**View Pruning in Logs**:
```bash
./bin/qt-chatbot-agent 2>&1 | grep "Pruned message"
```

**Manual Clear**:
- Edit → Clear Conversation (`Ctrl+L`)
- Resets conversation history completely

## Performance Tips

1. **Increase Context Window**: Settings → Context Window (larger = more history)
2. **Lower Max Tokens**: Settings → Max Tokens (shorter responses)
3. **Adjust Temperature**: Settings → Temperature (0.0 = deterministic, faster)
4. **Use Local Models**: Ollama is faster than cloud APIs
5. **Clear History**: Edit → Clear Conversation when starting new topic

## Security Notes

- API keys stored in plaintext: `~/.qtbot/config.json`
- Set file permissions: `chmod 600 ~/.qtbot/config.json`
- Don't commit config.json to version control
- Use environment variables for sensitive data in production

## Support

- Documentation: `docs/` directory
- Testing Guide: `docs/TESTING_GUIDE.md`
- System Prompt Guide: `docs/system-prompt-configuration.md`
- MCP Stdio Server: `docs/mcp-stdio-server.md`
- Issues: https://github.com/anthropics/qt-chatbot-agent/issues

---

**Version**: 1.0
**Last Updated**: 2025-10-08
