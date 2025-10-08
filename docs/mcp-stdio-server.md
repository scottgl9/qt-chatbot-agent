# MCP Stdio Test Server

## Overview

The qt-chatbot-agent includes a built-in test MCP (Model Context Protocol) server that runs in stdio mode. This server is designed for testing and validating MCP client integrations without requiring external server setup.

## Quick Start

Launch the test MCP server:

```bash
./bin/qt-chatbot-agent --test-mcp-stdio
```

The server will:
- Listen for JSON-RPC 2.0 requests on stdin
- Send JSON-RPC 2.0 responses on stdout
- Provide 3 test tools: `hello`, `echo`, `reverse_string`
- Output debug logs to stderr

## Protocol

The server implements **JSON-RPC 2.0** over stdio, compatible with the **Model Context Protocol (MCP)** specification.

### Request Format

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "method_name",
  "params": { ... }
}
```

### Response Format

**Success:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": { ... }
}
```

**Error:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32600,
    "message": "Error description"
  }
}
```

## Supported Methods

### 1. initialize

Initialize the MCP server connection.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "initialize",
  "params": {}
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "protocolVersion": "2024-11-05",
    "serverInfo": {
      "name": "test-mcp-stdio-server",
      "version": "1.0.0"
    },
    "capabilities": {
      "tools": {"listChanged": false}
    }
  }
}
```

### 2. tools/list

List all available tools.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/list",
  "params": {}
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "tools": [
      {
        "name": "hello",
        "description": "Returns a friendly greeting message",
        "parameters": {
          "type": "object",
          "properties": {
            "name": {
              "type": "string",
              "description": "Name to greet (optional)"
            }
          },
          "required": []
        }
      },
      {
        "name": "echo",
        "description": "Echoes back the provided message",
        "parameters": {
          "type": "object",
          "properties": {
            "message": {
              "type": "string",
              "description": "Message to echo back"
            }
          },
          "required": ["message"]
        }
      },
      {
        "name": "reverse_string",
        "description": "Reverses a string",
        "parameters": {
          "type": "object",
          "properties": {
            "text": {
              "type": "string",
              "description": "Text to reverse"
            }
          },
          "required": ["text"]
        }
      }
    ]
  }
}
```

### 3. tools/call

Execute a tool.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "hello",
    "arguments": {
      "name": "Alice"
    }
  }
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Hello, Alice! This is a test MCP server running in stdio mode."
      }
    ]
  }
}
```

## Available Tools

### hello

Returns a friendly greeting message.

**Parameters:**
- `name` (string, optional): Name to greet. Defaults to "World".

**Example:**
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"hello","arguments":{"name":"Bob"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Hello, Bob! This is a test MCP server running in stdio mode."
      }
    ]
  }
}
```

### echo

Echoes back the provided message.

**Parameters:**
- `message` (string, required): Message to echo back.

**Example:**
```bash
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"echo","arguments":{"message":"Test message"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Test message"
      }
    ]
  }
}
```

### reverse_string

Reverses a string.

**Parameters:**
- `text` (string, required): Text to reverse.

**Example:**
```bash
echo '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"reverse_string","arguments":{"text":"Hello"}}}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "olleH"
      }
    ]
  }
}
```

## Error Codes

The server follows JSON-RPC 2.0 error code standards:

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON received |
| -32600 | Invalid Request | Missing required fields or invalid format |
| -32601 | Method not found | The method does not exist |
| -32602 | Invalid params | Invalid method parameters |

**Example Error:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32601,
    "message": "Method not found: unknown_method"
  }
}
```

## Testing Examples

### Interactive Testing

```bash
# Start the server
./bin/qt-chatbot-agent --test-mcp-stdio

# In another terminal, send commands:
# List tools
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | nc localhost 0

# Call a tool
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"hello","arguments":{}}}' | nc localhost 0
```

### Automated Testing

```bash
#!/bin/bash
# Test script for MCP stdio server

SERVER="./bin/qt-chatbot-agent --test-mcp-stdio"

# Test 1: List tools
echo "Test 1: List tools"
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | $SERVER 2>/dev/null | jq '.result.tools | length'

# Test 2: Call hello tool
echo "Test 2: Call hello tool"
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"hello","arguments":{"name":"Test"}}}' | \
  $SERVER 2>/dev/null | jq -r '.result.content[0].text'

# Test 3: Call echo tool
echo "Test 3: Call echo tool"
echo '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"echo","arguments":{"message":"Hello MCP"}}}' | \
  $SERVER 2>/dev/null | jq -r '.result.content[0].text'

# Test 4: Call reverse_string tool
echo "Test 4: Call reverse_string tool"
echo '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"reverse_string","arguments":{"text":"abcd"}}}' | \
  $SERVER 2>/dev/null | jq -r '.result.content[0].text'
```

Expected output:
```
Test 1: List tools
3
Test 2: Call hello tool
Hello, Test! This is a test MCP server running in stdio mode.
Test 3: Call echo tool
Hello MCP
Test 4: Call reverse_string tool
dcba
```

## Integration with MCP Clients

### Python Client Example

```python
import json
import subprocess

class MCPStdioClient:
    def __init__(self, server_command):
        self.process = subprocess.Popen(
            server_command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        self.request_id = 0

    def call_tool(self, tool_name, arguments):
        self.request_id += 1
        request = {
            "jsonrpc": "2.0",
            "id": self.request_id,
            "method": "tools/call",
            "params": {
                "name": tool_name,
                "arguments": arguments
            }
        }

        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()

        response_line = self.process.stdout.readline()
        return json.loads(response_line)

    def list_tools(self):
        self.request_id += 1
        request = {
            "jsonrpc": "2.0",
            "id": self.request_id,
            "method": "tools/list"
        }

        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()

        response_line = self.process.stdout.readline()
        return json.loads(response_line)

    def close(self):
        self.process.stdin.close()
        self.process.wait()

# Usage
client = MCPStdioClient(["./bin/qt-chatbot-agent", "--test-mcp-stdio"])

# List tools
tools_response = client.list_tools()
print("Available tools:", [t["name"] for t in tools_response["result"]["tools"]])

# Call hello tool
hello_response = client.call_tool("hello", {"name": "Python"})
print("Hello response:", hello_response["result"]["content"][0]["text"])

# Call reverse_string tool
reverse_response = client.call_tool("reverse_string", {"text": "MCP"})
print("Reversed:", reverse_response["result"]["content"][0]["text"])

client.close()
```

### Node.js Client Example

```javascript
const { spawn } = require('child_process');
const readline = require('readline');

class MCPStdioClient {
    constructor(serverCommand) {
        this.process = spawn(serverCommand[0], serverCommand.slice(1));
        this.rl = readline.createInterface({
            input: this.process.stdout,
            crlfDelay: Infinity
        });
        this.requestId = 0;
        this.pendingRequests = new Map();

        this.rl.on('line', (line) => {
            const response = JSON.parse(line);
            const callback = this.pendingRequests.get(response.id);
            if (callback) {
                callback(response);
                this.pendingRequests.delete(response.id);
            }
        });
    }

    async callTool(toolName, args) {
        this.requestId++;
        const request = {
            jsonrpc: "2.0",
            id: this.requestId,
            method: "tools/call",
            params: {
                name: toolName,
                arguments: args
            }
        };

        return new Promise((resolve) => {
            this.pendingRequests.set(this.requestId, resolve);
            this.process.stdin.write(JSON.stringify(request) + '\n');
        });
    }

    async listTools() {
        this.requestId++;
        const request = {
            jsonrpc: "2.0",
            id: this.requestId,
            method: "tools/list"
        };

        return new Promise((resolve) => {
            this.pendingRequests.set(this.requestId, resolve);
            this.process.stdin.write(JSON.stringify(request) + '\n');
        });
    }

    close() {
        this.process.stdin.end();
        this.process.kill();
    }
}

// Usage
(async () => {
    const client = new MCPStdioClient(['./bin/qt-chatbot-agent', '--test-mcp-stdio']);

    // List tools
    const toolsResponse = await client.listTools();
    console.log('Available tools:', toolsResponse.result.tools.map(t => t.name));

    // Call hello tool
    const helloResponse = await client.callTool('hello', { name: 'Node.js' });
    console.log('Hello response:', helloResponse.result.content[0].text);

    // Call echo tool
    const echoResponse = await client.callTool('echo', { message: 'Testing MCP' });
    console.log('Echo response:', echoResponse.result.content[0].text);

    client.close();
})();
```

## Debugging

### Enable Debug Logging

The server outputs debug logs to stderr. To see them:

```bash
./bin/qt-chatbot-agent --test-mcp-stdio 2>&1 | tee server.log
```

### Debug Output Example

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
[MCP Server] Received request: "tools/list" id: QJsonValue(double, 1)
[MCP Server] Handling tools/list request
[MCP Server] Sending response: {...}
```

### Common Issues

**Problem**: Server immediately exits
**Solution**: The server reads from stdin until EOF. Ensure you're piping data or providing interactive input.

**Problem**: No response received
**Solution**: Check that you're reading from stdout. Debugging logs go to stderr.

**Problem**: Invalid JSON error
**Solution**: Ensure your JSON is properly formatted. Use `jq` to validate:
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | jq .
```

## Use Cases

### 1. Development Testing

Test MCP client implementations without setting up external servers:

```bash
# Quick validation
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | jq .
```

### 2. Integration Testing

Include in automated test suites:

```python
def test_mcp_integration():
    client = MCPStdioClient(["./bin/qt-chatbot-agent", "--test-mcp-stdio"])
    response = client.list_tools()
    assert len(response["result"]["tools"]) == 3
    client.close()
```

### 3. Protocol Validation

Verify MCP protocol compliance:

```bash
# Check JSON-RPC 2.0 compliance
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | \
  ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | \
  jq -e '.jsonrpc == "2.0" and .id == 1'
```

### 4. Load Testing

Test server performance:

```bash
# Send 100 requests
for i in {1..100}; do
  echo '{"jsonrpc":"2.0","id":'$i',"method":"tools/list"}'
done | ./bin/qt-chatbot-agent --test-mcp-stdio 2>/dev/null | wc -l
```

## Implementation Details

### Architecture

- **Language**: C++ with Qt5
- **I/O**: QTextStream for stdio handling
- **JSON**: QJsonDocument for parsing/serialization
- **Protocol**: JSON-RPC 2.0 compliant
- **MCP Version**: Compatible with MCP specification 2024-11-05

### Source Files

- `src/TestMCPStdioServer.cpp` - Server implementation
- `include/TestMCPStdioServer.h` - Header file
- `src/main.cpp` - Command line integration

### Tool Registration

Tools are registered in the constructor:

```cpp
QJsonObject helloTool;
helloTool["name"] = "hello";
helloTool["description"] = "Returns a friendly greeting message";
helloTool["parameters"] = { ... };
tools.append(helloTool);
```

### Request Processing

1. Read from stdin until complete JSON detected
2. Parse JSON and validate JSON-RPC format
3. Route to appropriate method handler
4. Execute tool if `tools/call`
5. Serialize response and write to stdout

## Future Enhancements

Planned improvements:

- [ ] Support for batch requests (JSON-RPC 2.0 batch)
- [ ] Additional test tools (math, file operations)
- [ ] WebSocket mode support
- [ ] Persistent state between requests
- [ ] Tool call history logging
- [ ] Performance metrics

## Related Documentation

- [MCP Integration Guide](mcp-integration.md) - Full MCP protocol integration
- [MCP Testing Guide](mcp-testing.md) - Comprehensive testing strategies
- [Tool Call Visualization](tool-call-visualization.md) - UI tool indicators

## References

- [Model Context Protocol Specification](https://modelcontextprotocol.io/)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
- [Qt QTextStream Documentation](https://doc.qt.io/qt-5/qtextstream.html)

---

**Version**: 1.0.0
**Last Updated**: 2025-10-08
**Status**: Production Ready
