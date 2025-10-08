# Comprehensive Testing Guide

## Overview

This guide provides step-by-step instructions for testing all features of the qt-chatbot-agent application, with special focus on the newly implemented tool calling and MCP integration.

## Prerequisites

### Required

- ✅ Application built successfully: `./bin/qt-chatbot-agent` exists
- ✅ Ollama server running (for LLM testing)
- ✅ Qt5 installed (5.15.13 recommended)

### Optional

- Python 3 or Node.js (for MCP stdio server testing)
- `jq` command line tool (for JSON formatting)
- Network access to Ollama server

## Quick Start

### 1. Basic Application Launch

```bash
cd build
./bin/qt-chatbot-agent
```

**Expected**:
- Application window opens
- Welcome messages appear
- Status bar shows: Backend, Model, Server, Tools count
- No error messages

**Verify**:
- [ ] Window opens without errors
- [ ] Status bar shows correct information
- [ ] Chat display is ready for input

---

## Feature Testing

### 2. Settings Configuration

#### Test: Open Settings Dialog

**Steps**:
1. Click menu: File → Settings (or press `Ctrl+,`)
2. Observe the settings dialog

**Expected**:
- Settings dialog opens (500px min width)
- Shows current configuration loaded from config file
- All fields populated with existing values

**Verify**:
- [ ] Backend dropdown shows current backend
- [ ] Model field shows current model
- [ ] API URL is populated
- [ ] System Prompt section visible
- [ ] LLM Parameters section visible

#### Test: Edit System Prompt

**Steps**:
1. Open Settings (Ctrl+,)
2. Find "System Prompt" section
3. Clear existing text
4. Enter new system prompt:
   ```
   You are a helpful AI assistant. When you need to perform calculations,
   use the calculator tool. Always use available tools when appropriate.
   ```
5. Click "Save"

**Expected**:
- Dialog closes silently (no success popup)
- Settings saved to ~/.qtbot/config.json

**Verify**:
- [ ] Dialog closes without popup
- [ ] No chat message about settings update
- [ ] Config file updated (check: `cat ~/.qtbot/config.json | grep system_prompt`)

#### Test: Configure Ollama Model

**Steps**:
1. Open Settings (Ctrl+,)
2. If backend is Ollama, click "🔄 Refresh" button
3. Wait for model list to load
4. Select a model from dropdown (or enter manually)
5. Verify API URL: `http://192.168.1.150:11434/api/generate`
6. Click "Save"

**Expected**:
- Models populate from Ollama server (if available)
- Can select or type model name
- Save completes silently

**Verify**:
- [ ] Refresh button loads models
- [ ] Model dropdown populated (if server available)
- [ ] Can select model
- [ ] Settings save successfully

---

### 3. MCP Tools Viewer

#### Test: View Available Tools

**Steps**:
1. Click menu: View → Available Tools (or press `Ctrl+T`)
2. Observe the tools dialog

**Expected**:
- Dialog opens (700x500 minimum size)
- Shows "MCP Tools" header
- Displays total tool count (should be 5)
- Lists all local tools with details

**Verify**:
- [ ] Dialog opens correctly
- [ ] Shows "Total tools: 5"
- [ ] Each tool in its own group box
- [ ] Tool names visible: calculator, greeter, datetime, text_stats, random_number

#### Test: Inspect Tool Details

**Steps**:
1. Open tools viewer (Ctrl+T)
2. Find the "calculator" tool
3. Read the description
4. Examine the parameters section

**Expected**:
- Description: "Perform basic arithmetic operations"
- Parameters shown in JSON format
- Monospace font for JSON
- Read-only text area
- Type: Local (green label)

**Verify**:
- [ ] Description is clear
- [ ] Parameters include: operation, a, b
- [ ] JSON is properly formatted
- [ ] Type shows "Local" in green

#### Test: Scroll Through Tools

**Steps**:
1. Open tools viewer (Ctrl+T)
2. Scroll down to see all 5 tools
3. Verify each tool is properly displayed

**Expected**:
- All 5 tools visible when scrolling
- Each in separate group box
- Parameters JSON for each tool
- Close button at bottom

**Verify**:
- [ ] calculator - basic arithmetic
- [ ] greeter - friendly greeting
- [ ] datetime - current date/time
- [ ] text_stats - text analysis
- [ ] random_number - random generation

---

### 4. System Prompt Integration

#### Test: Verify System Prompt is Sent

**Steps**:
1. Ensure system prompt is set in Settings
2. Start the application
3. Send a test message: "Hello"
4. Check logs for system prompt inclusion

**Commands**:
```bash
# Run with debug logging
./bin/qt-chatbot-agent 2>&1 | grep -i "system prompt"
```

**Expected Log Output**:
```
Including system prompt (length: XXX chars)
Tool-enabled request with 5 tools (system prompt: XXX chars)
```

**Verify**:
- [ ] Log shows "Including system prompt"
- [ ] Character count is reasonable (100-500)
- [ ] Tool-enabled request mentions system prompt

---

### 5. Tool Calling with LLM

#### Test: Calculator Tool

**Prerequisites**:
- Ollama server running at http://192.168.1.150:11434
- Model: gpt-oss:20b (or any model with function calling support)
- System prompt configured with tool awareness

**Steps**:
1. Start application
2. Send message: "What is 123 multiplied by 456?"
3. Watch for tool call indicators
4. Observe response

**Expected (if tool calling works)**:
```
[HH:MM:SS] You: What is 123 multiplied by 456?
[HH:MM:SS] System: 🔧 Calling tool: calculator
    Parameters:
    {"operation":"multiply","a":123,"b":456}

[HH:MM:SS] System: ✓ Tool completed: calculator
    Result:
    {"result": 56088, ...}

[HH:MM:SS] Bot: The result of 123 multiplied by 456 is 56,088.
```

**Alternative (if tool calling doesn't work)**:
- LLM responds directly: "123 × 456 = 56088"
- No tool call indicators appear
- This indicates the model doesn't support function calling well

**Verify**:
- [ ] Message sent successfully
- [ ] Tool call indicator appears (if supported)
- [ ] Tool executes and returns result (if supported)
- [ ] Bot provides final answer

**Debug Steps if Tool Calling Fails**:

1. Check logs for tool detection:
   ```bash
   ./bin/qt-chatbot-agent 2>&1 | grep -E "(tool call|Processing response)"
   ```

2. Verify system prompt includes tool instructions:
   ```bash
   cat ~/.qtbot/config.json | jq -r '.system_prompt' | head -20
   ```

3. Try a more explicit request:
   ```
   "Use the calculator tool to compute 999 plus 888"
   ```

4. Try a different model with better tool support:
   - llama3.1:latest
   - mistral:latest
   - command-r:latest

#### Test: Other Tools

**DateTime Tool**:
```
Query: "What is the current date and time?"
Expected: Tool call to datetime, returns formatted time
```

**Text Stats Tool**:
```
Query: "Analyze this text for me: Hello World 123"
Expected: Tool call to text_stats, returns character/word counts
```

**Random Number Tool**:
```
Query: "Generate 3 random numbers between 1 and 100"
Expected: Tool call to random_number with min=1, max=100, count=3
```

**Greeter Tool**:
```
Query: "Greet me with my name Alice"
Expected: Tool call to greeter with name="Alice"
```

---

### 6. Test MCP Stdio Server

#### Test: Launch Server

**Steps**:
```bash
./bin/qt-chatbot-agent --test-mcp-stdio
```

**Expected Output**:
```
========================================
Test MCP Stdio Server
========================================
Protocol: JSON-RPC 2.0 over stdio
Tools: hello, echo, reverse_string
========================================
[MCP Server] Test MCP stdio server initialized
[MCP Server] Registered 3 test tools
[MCP Server] Starting stdio event loop...
[MCP Server] Waiting for JSON-RPC requests on stdin
```

**Verify**:
- [ ] Server starts without errors
- [ ] Shows registered tools count (3)
- [ ] Waits for input on stdin

#### Test: List Tools via Stdio

**Commands**:
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq .
```

**Expected Output**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "tools": [
      {
        "name": "hello",
        "description": "Returns a friendly greeting message",
        "parameters": { ... }
      },
      {
        "name": "echo",
        "description": "Echoes back the provided message",
        "parameters": { ... }
      },
      {
        "name": "reverse_string",
        "description": "Reverses a string",
        "parameters": { ... }
      }
    ]
  }
}
```

**Verify**:
- [ ] Valid JSON-RPC 2.0 response
- [ ] Contains 3 tools
- [ ] Each tool has name, description, parameters

#### Test: Call Hello Tool

**Commands**:
```bash
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"hello","arguments":{"name":"Tester"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq -r '.result.content[0].text'
```

**Expected Output**:
```
Hello, Tester! This is a test MCP server running in stdio mode.
```

**Verify**:
- [ ] Response includes custom name
- [ ] JSON structure is correct
- [ ] Message is friendly and informative

#### Test: Call Echo Tool

**Commands**:
```bash
echo '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"echo","arguments":{"message":"Testing MCP"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq -r '.result.content[0].text'
```

**Expected Output**:
```
Testing MCP
```

**Verify**:
- [ ] Exact message echoed back
- [ ] No modifications to content

#### Test: Call Reverse String Tool

**Commands**:
```bash
echo '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"reverse_string","arguments":{"text":"Hello"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq -r '.result.content[0].text'
```

**Expected Output**:
```
olleH
```

**Verify**:
- [ ] String is correctly reversed
- [ ] Character-by-character reversal

#### Test: Error Handling

**Invalid Method**:
```bash
echo '{"jsonrpc":"2.0","id":5,"method":"unknown"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq .
```

**Expected**:
```json
{
  "jsonrpc": "2.0",
  "id": 5,
  "error": {
    "code": -32601,
    "message": "Method not found: unknown"
  }
}
```

**Verify**:
- [ ] Returns JSON-RPC error
- [ ] Error code -32601
- [ ] Descriptive error message

---

### 7. Conversation Features

#### Test: Clear Conversation

**Steps**:
1. Send a few messages to populate chat
2. Press `Ctrl+L` or Edit → Clear Conversation
3. Confirm in dialog

**Expected**:
- Confirmation dialog appears
- Clicking "Yes" clears all messages
- System message confirms: "Conversation cleared."

**Verify**:
- [ ] Confirmation dialog shown
- [ ] Chat clears on confirm
- [ ] System message appears

#### Test: Copy Conversation

**Steps**:
1. Send a few messages
2. Press `Ctrl+Shift+C` or Edit → Copy Conversation
3. Paste into text editor

**Expected**:
- Plain text copied to clipboard
- Includes timestamps
- All messages present

**Verify**:
- [ ] Clipboard contains conversation
- [ ] Timestamps included
- [ ] Formatting preserved

#### Test: Export Conversation

**Steps**:
1. Send a few messages
2. Press `Ctrl+E` or File → Export Conversation
3. Choose location and format (.txt or .md)
4. Save file

**Expected**:
- File dialog opens
- Can choose format
- File saves successfully
- Includes metadata header

**Verify**:
- [ ] File dialog works
- [ ] File created successfully
- [ ] Contains metadata (date, model, backend)
- [ ] All messages exported

---

### 8. UI/UX Features

#### Test: Markdown Formatting

**Test Messages**:
```
**Bold text**
*Italic text*
`inline code`
```code block```
- List item 1
- List item 2
1. Numbered item
```

**Expected**:
- **Bold** renders as bold
- *Italic* renders as italic
- `code` has pink highlighting
- Code blocks have gray background
- Lists formatted properly

**Verify**:
- [ ] Bold formatting works
- [ ] Italic formatting works
- [ ] Inline code highlighted
- [ ] Code blocks styled
- [ ] Lists formatted

#### Test: Thinking Indicator

**Steps**:
1. Send a message
2. Observe "Thinking..." label below chat
3. Wait for response

**Expected**:
- "Thinking..." appears immediately
- Dots animate (0, 1, 2, 3 dots cycling)
- Disappears when first token arrives

**Verify**:
- [ ] Thinking indicator shows
- [ ] Animation cycles correctly
- [ ] Hides on response start

#### Test: Status Bar

**Observe**:
- Bottom of window shows status bar
- Contains: Backend | Model | Server | Tools

**Expected**:
```
Backend: Ollama | Model: gpt-oss:20b | Server: 192.168.1.150:11434 | Tools: 5
```

**Verify**:
- [ ] Status bar visible
- [ ] Shows correct backend
- [ ] Shows correct model
- [ ] Shows server address
- [ ] Shows tool count (5)

---

## Automated Testing

### Unit Tests

**Run All Tests**:
```bash
cd build
ctest --output-on-failure
```

**Expected**:
```
Test #1: ConfigTest ............... Passed
Test #2: MCPHandlerTest ........... Passed
Test #3: MCPServerTest ............ Passed

100% tests passed, 0 tests failed out of 3
```

**Verify**:
- [ ] All 3 test suites pass
- [ ] Total 22 test cases pass
- [ ] No failures or errors

### Test Script

Create `test_all.sh`:
```bash
#!/bin/bash

echo "=== Running Unit Tests ==="
cd build
ctest --output-on-failure
if [ $? -ne 0 ]; then
    echo "❌ Unit tests failed"
    exit 1
fi
echo "✅ Unit tests passed"

echo ""
echo "=== Testing MCP Stdio Server ==="

# Test 1: List tools
echo "Test 1: List tools"
TOOLS_COUNT=$(echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | \
  jq -r '.result.tools | length')

if [ "$TOOLS_COUNT" = "3" ]; then
    echo "✅ Tools list: 3 tools found"
else
    echo "❌ Tools list: Expected 3, got $TOOLS_COUNT"
    exit 1
fi

# Test 2: Call hello tool
echo "Test 2: Call hello tool"
HELLO_RESPONSE=$(echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"hello","arguments":{"name":"Test"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | \
  jq -r '.result.content[0].text')

if echo "$HELLO_RESPONSE" | grep -q "Hello, Test"; then
    echo "✅ Hello tool: Correct response"
else
    echo "❌ Hello tool: Unexpected response: $HELLO_RESPONSE"
    exit 1
fi

# Test 3: Call reverse_string tool
echo "Test 3: Call reverse_string tool"
REVERSED=$(echo '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"reverse_string","arguments":{"text":"abcd"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | \
  jq -r '.result.content[0].text')

if [ "$REVERSED" = "dcba" ]; then
    echo "✅ Reverse tool: Correct reversal"
else
    echo "❌ Reverse tool: Expected 'dcba', got '$REVERSED'"
    exit 1
fi

echo ""
echo "=== All Tests Passed ==="
```

**Run**:
```bash
chmod +x test_all.sh
./test_all.sh
```

---

## Troubleshooting

### Issue: Application Won't Start

**Symptoms**: Crashes immediately or shows Qt errors

**Solutions**:
1. Check Qt5 is installed: `qmake --version`
2. Verify binary exists: `ls -lh build/bin/qt-chatbot-agent`
3. Check libraries: `ldd build/bin/qt-chatbot-agent`
4. Run with debug: `QT_DEBUG_PLUGINS=1 ./bin/qt-chatbot-agent`

### Issue: No Tool Call Indicators Appear

**Symptoms**: LLM responds directly without calling tools

**Possible Causes**:
1. Model doesn't support function calling
2. System prompt not sent to API
3. Tool instructions unclear

**Solutions**:
1. Check logs: `./bin/qt-chatbot-agent 2>&1 | grep "system prompt"`
2. Verify system prompt in config:
   ```bash
   cat ~/.qtbot/config.json | jq -r '.system_prompt'
   ```
3. Try explicit request: "Use the calculator tool to compute X"
4. Try different model: llama3.1, mistral, command-r

### Issue: Settings Don't Save

**Symptoms**: Changes revert after restart

**Solutions**:
1. Check config file permissions:
   ```bash
   ls -l ~/.qtbot/config.json
   ```
2. Check directory permissions:
   ```bash
   ls -ld ~/.qtbot/
   ```
3. View logs for save errors:
   ```bash
   ./bin/qt-chatbot-agent 2>&1 | grep -i "failed to save"
   ```

### Issue: MCP Stdio Server Not Responding

**Symptoms**: No output when sending JSON requests

**Solutions**:
1. Verify JSON is valid: `echo '...' | jq .`
2. Check stderr for errors (use `2>&1`)
3. Ensure complete JSON object (balanced braces)
4. Try with timeout: `timeout 2 ./bin/qt-chatbot-agent --test-mcp-stdio`

---

## Performance Testing

### Response Time

**Test**:
```bash
time echo "What is 2+2?" | ./bin/qt-chatbot-agent --cli --prompt "2+2"
```

**Expected**: < 5 seconds for simple queries

### Memory Usage

**Test**:
```bash
/usr/bin/time -v ./bin/qt-chatbot-agent 2>&1 | grep "Maximum resident"
```

**Expected**: < 100MB for typical usage

### Concurrent Requests

**Test**: Run multiple MCP stdio requests in parallel

```bash
for i in {1..10}; do
  (echo '{"jsonrpc":"2.0","id":'$i',"method":"tools/list"}' | \
    ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null) &
done
wait
```

**Expected**: All requests complete successfully

---

## Test Results Template

```
# Test Results - [DATE]

## Environment
- OS: [Linux/macOS/Windows]
- Qt Version: [5.15.13]
- Ollama Server: [Running/Not Available]
- Model: [gpt-oss:20b]

## Test Results

### Settings Configuration
- [ ] Open settings dialog: PASS/FAIL
- [ ] Edit system prompt: PASS/FAIL
- [ ] Configure model: PASS/FAIL
- [ ] Save settings: PASS/FAIL

### MCP Tools Viewer
- [ ] View available tools: PASS/FAIL
- [ ] Inspect tool details: PASS/FAIL
- [ ] Scroll through tools: PASS/FAIL

### Tool Calling
- [ ] Calculator tool: PASS/FAIL/NOT_SUPPORTED
- [ ] DateTime tool: PASS/FAIL/NOT_SUPPORTED
- [ ] Other tools: PASS/FAIL/NOT_SUPPORTED

### MCP Stdio Server
- [ ] Launch server: PASS/FAIL
- [ ] List tools: PASS/FAIL
- [ ] Call hello: PASS/FAIL
- [ ] Call echo: PASS/FAIL
- [ ] Call reverse_string: PASS/FAIL
- [ ] Error handling: PASS/FAIL

### UI/UX
- [ ] Markdown formatting: PASS/FAIL
- [ ] Thinking indicator: PASS/FAIL
- [ ] Status bar: PASS/FAIL

### Unit Tests
- [ ] All tests pass: PASS/FAIL
- [ ] Test count: [22/22]

## Notes
[Any observations, issues, or recommendations]
```

---

## Next Steps

After completing all tests:

1. **If all tests pass**: Application is ready for production use
2. **If tool calling doesn't work**: Try different LLM models with better function calling support
3. **If specific tests fail**: Review troubleshooting section and logs
4. **Report issues**: Include test results template with bug reports

---

**Version**: 1.0
**Last Updated**: 2025-10-08
**Status**: Production Ready
