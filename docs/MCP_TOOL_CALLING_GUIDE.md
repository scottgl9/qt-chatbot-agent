# MCP Tool Calling Integration Guide

## Overview

Phase 3 of the qt-chatbot-agent project is now complete with full MCP (Model Context Protocol) integration. The application can now interact with LLMs that support tool calling, allowing the chatbot to perform actions like calculations and greetings through registered tools.

## Implementation Summary

### Components

1. **MCPHandler** (`include/MCPHandler.h`, `src/MCPHandler.cpp`)
   - Manages tool registration and execution
   - Supports local function callbacks and networked HTTP endpoints
   - Emits signals when tools complete or fail

2. **LLMClient Extensions** (`include/LLMClient.h`, `src/LLMClient.cpp`)
   - `sendPromptWithTools()`: Sends prompts with tool descriptions
   - `sendToolResults()`: Sends tool execution results back to LLM
   - `processToolCalls()`: Detects tool call requests in LLM responses
   - Signal: `toolCallRequested(toolName, parameters, callId)`

3. **ChatWindow Integration** (`src/main.cpp`)
   - MCPHandler instance with registered tools
   - Signal/slot connections for tool workflow
   - Displays tool execution status and results

### Available Tools

1. **Calculator**
   - Operations: add, subtract, multiply, divide
   - Parameters: operation (string), a (number), b (number)
   - Example: `{"operation": "add", "a": 5, "b": 3}`

2. **DateTime**
   - Get current date and time in various formats
   - Parameters: format (string: 'short', 'long', 'iso', or 'timestamp')
   - Example: `{"format": "long"}`

## Tool Calling Workflow

```
User Input
    ↓
LLM receives prompt + tool descriptions
    ↓
LLM decides to use a tool
    ↓
LLM outputs: {"tool_call": {"name": "calculator", "parameters": {...}}}
    ↓
ChatWindow detects tool call
    ↓
MCPHandler executes tool
    ↓
Tool result displayed in chat
    ↓
Tool result sent back to LLM
    ↓
LLM generates natural language response
    ↓
Response streamed to user
```

## Testing the Feature

### Prerequisites

1. **Ollama Running**: Ensure Ollama is running with a compatible model
   ```bash
   ollama serve
   ```

2. **Model Loaded**: Use a model that can follow instructions (e.g., llama3, mistral)
   ```bash
   ollama pull llama3
   ```

3. **Configuration**: Check `~/.qtbot/config.json`
   ```json
   {
     "backend": "Ollama",
     "model": "llama3",
     "api_url": "http://localhost:11434/api/generate"
   }
   ```

### Test Cases

#### Test 1: Basic Calculator
**User Prompt**: "Calculate 15 + 27"

**Expected Behavior**:
1. LLM recognizes need for calculator tool
2. System message: "Executing tool: calculator"
3. System message: "Tool 'calculator' result: {result: 42, ...}"
4. Bot response: Natural language answer with result

#### Test 2: DateTime Tool
**User Prompt**: "What's the current date and time?"

**Expected Behavior**:
1. LLM recognizes need for datetime tool
2. System message: "Executing tool: datetime"
3. System message: "Tool 'datetime' result: {date: '...', time: '...', ...}"
4. Bot response: Natural language answer with date/time

#### Test 3: Division by Zero
**User Prompt**: "Divide 10 by 0"

**Expected Behavior**:
1. Calculator tool executes
2. Error result: {"error": "Division by zero"}
3. LLM responds acknowledging the error

### Running the Application

```bash
# Build the project
cmake --build build

# Run in GUI mode (requires X server or Wayland)
./build/bin/qt-chatbot-agent

# Run in CLI mode (for testing)
./build/bin/qt-chatbot-agent --cli --prompt "Calculate 5 + 3"
```

## Prompt Engineering

The LLMClient includes tool information in the prompt:

```
You have access to the following tools:
- calculator: Performs basic arithmetic operations (add, subtract, multiply, divide)
  Parameters: {"operation": "...", "a": number, "b": number}
- datetime: Get current date and time in various formats
  Parameters: {"format": "..."}

To use a tool, respond with a JSON object in this format:
{"tool_call": {"name": "tool_name", "parameters": {...}}}
Otherwise, respond normally.
```

## Architecture Notes

### Signal Flow

1. **User → LLM**:
   - `ChatWindow::sendMessage()`
   - `LLMClient::sendPromptWithTools(tools)`

2. **LLM → Tool Execution**:
   - `LLMClient::processToolCalls()` detects tool call
   - Signal: `toolCallRequested(name, params, id)`
   - `ChatWindow::handleToolCallRequest()`
   - `MCPHandler::executeToolCall()`

3. **Tool → LLM**:
   - Signal: `MCPHandler::toolCallCompleted(id, name, result)`
   - `ChatWindow::handleToolCallCompleted()`
   - `LLMClient::sendToolResults(prompt, results)`

4. **LLM → User**:
   - Streaming tokens via `tokenReceived` signal
   - Final response via `responseReceived` signal

### Thread Safety

- All Qt signals/slots are thread-safe
- MCPHandler uses QMutex for tool registry
- Network operations are async via QNetworkAccessManager

## Known Limitations

1. **LLM Dependent**: Tool calling relies on the LLM's ability to follow instructions and generate the correct JSON format
2. **No Native Function Calling**: Uses prompt engineering rather than OpenAI-style native function calling API
3. **Single Tool Per Turn**: Currently processes one tool call per LLM response
4. **No Tool Chaining**: Tools cannot call other tools

## Future Enhancements

1. **Multiple Tool Calls**: Support multiple tools in a single response
2. **Tool Chaining**: Allow tools to invoke other tools
3. **OpenAI Function Calling**: Add support for native function calling APIs
4. **Tool Discovery**: Dynamic tool loading from plugins
5. **Networked Tools**: Fully implement HTTP endpoint tool support
6. **Tool Validation**: Parameter validation against JSON schemas

## Code Locations

- **MCP Handler**: `include/MCPHandler.h`, `src/MCPHandler.cpp`
- **LLM Client**: `include/LLMClient.h`, `src/LLMClient.cpp` (lines 56, 71-114, 148-190, 440-490)
- **ChatWindow**: `src/main.cpp` (lines 87-154, 233-275, 307-338)
- **Example Tools**: `src/main.cpp` (lines 33-85)

## Debugging

Enable debug logging:
```bash
./build/bin/qt-chatbot-agent --log-level debug
```

Check logs:
```bash
tail -f ~/.qtbot/logs/qtbot.log
```

Look for:
- "Tool call detected: [name] (ID: [id])"
- "Tool call completed: [name] (ID: [id])"
- "Tool call failed: [name] (ID: [id])"

## Next Steps

Phase 4 will implement the RAG (Retrieval-Augmented Generation) engine:
- FAISS vector database integration
- Document ingestion (PDF, TXT, MD)
- Embedding generation
- Context retrieval during chat

---

**Last Updated**: 2025-10-08
**Phase**: 3 - MCP Implementation ✓ Complete
