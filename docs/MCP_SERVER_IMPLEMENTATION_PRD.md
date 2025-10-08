# Product Requirements Document: Qt5-based MCP Server Service

**Project**: MCP Server Implementation with Qt5  
**Version**: 1.0.0  
**Status**: Draft  
**Author**: Scott Glover  
**Date**: October 10, 2025

---

## Executive Summary

This PRD describes the implementation of a production-ready Model Context Protocol (MCP) server written in C++ using Qt5, designed to run as a system service. The server will provide MCP-compliant tool endpoints that can be consumed by qt-chatbot-agent and other MCP clients.

### Goals
- Create a standalone, service-ready MCP server in C++/Qt5
- Support both HTTP and stdio communication protocols
- Provide extensible tool registration system
- Enable system-level deployment (systemd service)
- Maintain compatibility with existing MCP clients

### Non-Goals
- GUI interface for the server
- Built-in tool implementations (separate concern)
- Client-side MCP functionality (already exists in MCPHandler)

---

## Table of Contents

1. [Background](#background)
2. [Architecture](#architecture)
3. [Technical Requirements](#technical-requirements)
4. [Component Specifications](#component-specifications)
5. [API Specifications](#api-specifications)
6. [Tool Registration System](#tool-registration-system)
7. [Communication Protocols](#communication-protocols)
8. [Service Deployment](#service-deployment)
9. [Security Considerations](#security-considerations)
10. [Testing Strategy](#testing-strategy)
11. [Implementation Phases](#implementation-phases)
12. [Success Metrics](#success-metrics)

---

## Background

### Current State

qt-chatbot-agent currently includes:
- **MCPHandler**: Client-side MCP tool consumer (calls tools)
- **TestMCPStdioServer**: Simple test server for stdio protocol
- **Tool Support**: HTTP-based tools from external servers

### Problem Statement

1. **No Production Server**: TestMCPStdioServer is minimal, not service-ready
2. **External Dependencies**: Rely on external servers for HTTP tools
3. **Limited Control**: Cannot customize tool behavior at server level
4. **Deployment Complexity**: No standard service deployment model

### Proposed Solution

Implement a production-grade MCP server that:
- Runs as a systemd service
- Provides HTTP and stdio endpoints
- Supports dynamic tool registration
- Includes comprehensive logging and monitoring
- Follows MCP specification (JSON-RPC 2.0)

---

## Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────┐
│                MCP Server Service                    │
│                                                      │
│  ┌────────────────┐          ┌──────────────────┐  │
│  │   HTTP Server  │          │  Stdio Handler   │  │
│  │  (QTcpServer)  │          │ (QTextStream)    │  │
│  └────────┬───────┘          └────────┬─────────┘  │
│           │                           │             │
│           └───────────┬───────────────┘             │
│                       │                             │
│           ┌───────────▼──────────────┐              │
│           │    Request Router        │              │
│           │  (JSON-RPC Parser)       │              │
│           └───────────┬──────────────┘              │
│                       │                             │
│           ┌───────────▼──────────────┐              │
│           │     Tool Registry        │              │
│           │  (Dynamic Registration)  │              │
│           └───────────┬──────────────┘              │
│                       │                             │
│           ┌───────────▼──────────────┐              │
│           │      Tool Executor       │              │
│           │   (Plugin Interface)     │              │
│           └───────────┬──────────────┘              │
│                       │                             │
│           ┌───────────▼──────────────┐              │
│           │    Response Builder      │              │
│           └──────────────────────────┘              │
└─────────────────────────────────────────────────────┘
```

### Component Hierarchy

```
mcp-server/
├── src/
│   ├── main.cpp                    # Entry point, service initialization
│   ├── MCPServer.cpp               # Main server class
│   ├── HttpServer.cpp              # HTTP endpoint handler
│   ├── StdioHandler.cpp            # Stdio communication handler
│   ├── RequestRouter.cpp           # JSON-RPC request routing
│   ├── ToolRegistry.cpp            # Tool registration and lookup
│   ├── ToolExecutor.cpp            # Tool execution engine
│   ├── ResponseBuilder.cpp         # JSON-RPC response formatting
│   ├── Logger.cpp                  # Server logging
│   └── Config.cpp                  # Configuration management
├── include/
│   ├── MCPServer.h
│   ├── HttpServer.h
│   ├── StdioHandler.h
│   ├── RequestRouter.h
│   ├── ToolRegistry.h
│   ├── ToolExecutor.h
│   ├── ToolInterface.h             # Tool plugin interface
│   ├── ResponseBuilder.h
│   ├── Logger.h
│   └── Config.h
├── tools/                          # Built-in tool plugins
│   ├── CalculatorTool.cpp
│   ├── DateTimeTool.cpp
│   ├── SystemInfoTool.cpp
│   └── ...
├── tests/
│   ├── test_server.cpp
│   ├── test_tools.cpp
│   └── test_jsonrpc.cpp
├── systemd/
│   ├── mcp-server.service         # Systemd unit file
│   └── mcp-server.conf            # Service configuration
├── debian/                         # Debian packaging
│   ├── control
│   ├── changelog
│   ├── rules
│   └── ...
└── CMakeLists.txt
```

---

## Technical Requirements

### Functional Requirements

#### FR-1: Protocol Support
- **FR-1.1**: Support HTTP endpoints (REST-like JSON-RPC)
- **FR-1.2**: Support stdio communication (pipe-based JSON-RPC)
- **FR-1.3**: Implement JSON-RPC 2.0 specification
- **FR-1.4**: Support both single and batch requests

#### FR-2: Tool Management
- **FR-2.1**: Dynamic tool registration at runtime
- **FR-2.2**: Tool discovery endpoint (`tools/list`)
- **FR-2.3**: Tool execution endpoint (`tools/call`)
- **FR-2.4**: Tool parameter validation
- **FR-2.5**: Tool metadata (name, description, parameters)

#### FR-3: Service Operations
- **FR-3.1**: Run as systemd service
- **FR-3.2**: Graceful start/stop/restart
- **FR-3.3**: Configuration reload without restart
- **FR-3.4**: Health check endpoint
- **FR-3.5**: Status reporting

#### FR-4: Logging and Monitoring
- **FR-4.1**: Structured logging to syslog
- **FR-4.2**: Configurable log levels
- **FR-4.3**: Request/response logging
- **FR-4.4**: Error tracking and reporting
- **FR-4.5**: Performance metrics

### Non-Functional Requirements

#### NFR-1: Performance
- **NFR-1.1**: Handle 100+ concurrent requests
- **NFR-1.2**: Response time < 100ms for simple tools
- **NFR-1.3**: Memory usage < 50MB at idle
- **NFR-1.4**: CPU usage < 5% at idle

#### NFR-2: Reliability
- **NFR-2.1**: 99.9% uptime
- **NFR-2.2**: Automatic restart on crash
- **NFR-2.3**: Request timeout handling (30s default)
- **NFR-2.4**: Graceful degradation

#### NFR-3: Security
- **NFR-3.1**: Optional API key authentication
- **NFR-3.2**: IP whitelist support
- **NFR-3.3**: Request rate limiting
- **NFR-3.4**: Input sanitization
- **NFR-3.5**: TLS/SSL support (optional)

#### NFR-4: Maintainability
- **NFR-4.1**: Clear separation of concerns
- **NFR-4.2**: Plugin architecture for tools
- **NFR-4.3**: Comprehensive documentation
- **NFR-4.4**: Unit test coverage > 80%

#### NFR-5: Compatibility
- **NFR-5.1**: Qt5 5.15+
- **NFR-5.2**: C++17 standard
- **NFR-5.3**: Linux systemd systems
- **NFR-5.4**: MCP specification compliance

---

## Component Specifications

### 1. MCPServer (Main Server Class)

**Purpose**: Orchestrates all server components and manages lifecycle.

**Responsibilities**:
- Initialize HTTP and stdio handlers
- Load configuration
- Register built-in tools
- Handle signals (SIGTERM, SIGHUP)
- Coordinate graceful shutdown

**Key Methods**:
```cpp
class MCPServer : public QObject {
    Q_OBJECT
public:
    explicit MCPServer(QObject *parent = nullptr);
    ~MCPServer();

    bool start();                           // Start server
    void stop();                            // Graceful shutdown
    bool reload();                          // Reload configuration
    
    void registerTool(ToolInterface *tool); // Register tool plugin
    ToolRegistry* toolRegistry();           // Access tool registry
    
signals:
    void started();
    void stopped();
    void error(const QString &message);

private:
    HttpServer *m_httpServer;
    StdioHandler *m_stdioHandler;
    ToolRegistry *m_toolRegistry;
    RequestRouter *m_requestRouter;
    Config *m_config;
    Logger *m_logger;
};
```

### 2. HttpServer (HTTP Endpoint Handler)

**Purpose**: Handle HTTP-based JSON-RPC requests.

**Responsibilities**:
- Listen on configured port
- Parse HTTP POST requests
- Route to RequestRouter
- Send HTTP responses
- Handle CORS (if enabled)

**Key Methods**:
```cpp
class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(RequestRouter *router, QObject *parent = nullptr);
    
    bool start(quint16 port);
    void stop();
    
    void setApiKey(const QString &key);          // Optional authentication
    void setWhitelist(const QStringList &ips);   // IP filtering
    void enableCors(bool enable);                // CORS support

signals:
    void requestReceived(const QJsonObject &request, QTcpSocket *socket);
    void error(const QString &message);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QTcpServer *m_tcpServer;
    RequestRouter *m_router;
    QString m_apiKey;
    QStringList m_whitelist;
    bool m_corsEnabled;
};
```

**HTTP Endpoints**:
```
POST /jsonrpc          # Main JSON-RPC endpoint
GET  /health           # Health check
GET  /version          # Server version
POST /tools/list       # List available tools (convenience)
POST /tools/call       # Call tool (convenience)
```

### 3. StdioHandler (Stdio Communication)

**Purpose**: Handle stdio-based JSON-RPC for piped communication.

**Responsibilities**:
- Read JSON-RPC from stdin
- Write responses to stdout
- Handle multiple requests (line-delimited)
- Graceful EOF handling

**Key Methods**:
```cpp
class StdioHandler : public QObject {
    Q_OBJECT
public:
    explicit StdioHandler(RequestRouter *router, QObject *parent = nullptr);
    
    void start();
    void stop();

signals:
    void requestReceived(const QJsonObject &request);
    void finished();

private slots:
    void onReadyRead();

private:
    QTextStream m_stdin;
    QTextStream m_stdout;
    RequestRouter *m_router;
};
```

### 4. RequestRouter (JSON-RPC Parser)

**Purpose**: Parse and route JSON-RPC 2.0 requests to appropriate handlers.

**Responsibilities**:
- Validate JSON-RPC format
- Extract method and parameters
- Route to ToolExecutor or server methods
- Handle errors (parse, invalid request)
- Support batch requests

**Key Methods**:
```cpp
class RequestRouter : public QObject {
    Q_OBJECT
public:
    explicit RequestRouter(ToolRegistry *registry, QObject *parent = nullptr);
    
    QJsonObject processRequest(const QJsonObject &request);
    QJsonArray processBatchRequest(const QJsonArray &requests);

signals:
    void methodCalled(const QString &method, const QJsonObject &params);
    void parseError(const QString &message);

private:
    QJsonObject handleToolsList(const QJsonObject &params);
    QJsonObject handleToolsCall(const QJsonObject &params);
    QJsonObject createErrorResponse(int code, const QString &message, const QJsonValue &id);
    
    ToolExecutor *m_executor;
    ToolRegistry *m_registry;
};
```

**JSON-RPC Error Codes**:
```cpp
enum JsonRpcError {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ToolNotFound = -32001,      // Custom
    ToolExecutionFailed = -32002 // Custom
};
```

### 5. ToolRegistry (Tool Management)

**Purpose**: Manage tool registration, discovery, and lookup.

**Responsibilities**:
- Register tools at startup
- Dynamic tool loading from plugins
- Tool metadata management
- Thread-safe access

**Key Methods**:
```cpp
class ToolRegistry : public QObject {
    Q_OBJECT
public:
    explicit ToolRegistry(QObject *parent = nullptr);
    
    void registerTool(ToolInterface *tool);
    void unregisterTool(const QString &name);
    
    ToolInterface* getTool(const QString &name);
    QList<ToolInterface*> getAllTools() const;
    QJsonArray getToolsMetadata() const;
    
    bool hasTool(const QString &name) const;
    int toolCount() const;

signals:
    void toolRegistered(const QString &name);
    void toolUnregistered(const QString &name);

private:
    QHash<QString, ToolInterface*> m_tools;
    QReadWriteLock m_lock;  // Thread safety
};
```

### 6. ToolExecutor (Tool Execution Engine)

**Purpose**: Execute tools and handle results/errors.

**Responsibilities**:
- Validate tool parameters
- Execute tool with timeout
- Capture results and errors
- Handle exceptions
- Log execution metrics

**Key Methods**:
```cpp
class ToolExecutor : public QObject {
    Q_OBJECT
public:
    explicit ToolExecutor(ToolRegistry *registry, QObject *parent = nullptr);
    
    QJsonObject executeTool(const QString &name, const QJsonObject &params);
    void setTimeout(int seconds);  // Default: 30s

signals:
    void toolStarted(const QString &name);
    void toolCompleted(const QString &name, qint64 durationMs);
    void toolFailed(const QString &name, const QString &error);

private:
    QJsonObject validateParameters(ToolInterface *tool, const QJsonObject &params);
    QJsonObject createResult(const QString &toolName, const QJsonObject &result);
    QJsonObject createError(int code, const QString &message);
    
    ToolRegistry *m_registry;
    int m_timeoutSeconds;
};
```

### 7. ToolInterface (Plugin Interface)

**Purpose**: Define interface for tool plugins.

**Specification**:
```cpp
class ToolInterface {
public:
    virtual ~ToolInterface() = default;
    
    // Tool metadata
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QJsonObject parameters() const = 0;
    
    // Tool execution
    virtual QJsonObject execute(const QJsonObject &params) = 0;
    
    // Optional: tool lifecycle
    virtual void initialize() {}
    virtual void shutdown() {}
};

// Example: Calculator Tool
class CalculatorTool : public ToolInterface {
public:
    QString name() const override { return "calculator"; }
    QString description() const override { return "Perform arithmetic operations"; }
    
    QJsonObject parameters() const override {
        return QJsonObject{
            {"operation", "string: add, subtract, multiply, divide"},
            {"a", "number: first operand"},
            {"b", "number: second operand"}
        };
    }
    
    QJsonObject execute(const QJsonObject &params) override {
        QString op = params["operation"].toString();
        double a = params["a"].toDouble();
        double b = params["b"].toDouble();
        double result = 0.0;
        
        if (op == "add") result = a + b;
        else if (op == "subtract") result = a - b;
        else if (op == "multiply") result = a * b;
        else if (op == "divide") {
            if (b == 0) {
                return QJsonObject{{"error", "Division by zero"}};
            }
            result = a / b;
        }
        
        return QJsonObject{{"result", result}};
    }
};
```

---

## API Specifications

### JSON-RPC 2.0 Format

**Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "calculator",
    "arguments": {
      "operation": "add",
      "a": 10,
      "b": 20
    }
  },
  "id": "request-123"
}
```

**Success Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "result": 30,
    "operation": "add",
    "a": 10,
    "b": 20
  },
  "id": "request-123"
}
```

**Error Response**:
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32001,
    "message": "Tool not found: calculator",
    "data": {
      "available_tools": ["datetime", "system_info"]
    }
  },
  "id": "request-123"
}
```

### MCP Methods

#### 1. tools/list

**Purpose**: List all available tools.

**Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/list",
  "id": 1
}
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "tools": [
      {
        "name": "calculator",
        "description": "Perform arithmetic operations",
        "parameters": {
          "operation": "string: add, subtract, multiply, divide",
          "a": "number: first operand",
          "b": "number: second operand"
        }
      },
      {
        "name": "datetime",
        "description": "Get current date and time",
        "parameters": {
          "format": "string: short, long, iso, timestamp"
        }
      }
    ]
  },
  "id": 1
}
```

#### 2. tools/call

**Purpose**: Execute a tool.

**Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "calculator",
    "arguments": {
      "operation": "multiply",
      "a": 15,
      "b": 7
    }
  },
  "id": 2
}
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "result": 105,
    "operation": "multiply",
    "a": 15,
    "b": 7
  },
  "id": 2
}
```

#### 3. server/info (Custom)

**Purpose**: Get server information.

**Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "server/info",
  "id": 3
}
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "name": "MCP Server",
    "version": "1.0.0",
    "protocol_version": "2024-10-07",
    "tools_count": 5,
    "uptime_seconds": 3600,
    "requests_processed": 1234
  },
  "id": 3
}
```

---

## Communication Protocols

### HTTP Mode

**Configuration** (`/etc/mcp-server/config.json`):
```json
{
  "mode": "http",
  "http": {
    "host": "0.0.0.0",
    "port": 8080,
    "api_key": "optional-secret-key",
    "whitelist": ["127.0.0.1", "192.168.1.0/24"],
    "cors_enabled": true,
    "max_request_size": "1MB",
    "timeout_seconds": 30
  }
}
```

**Example Client Request** (curl):
```bash
curl -X POST http://localhost:8080/jsonrpc \
  -H "Content-Type: application/json" \
  -H "X-API-Key: optional-secret-key" \
  -d '{
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {
      "name": "datetime",
      "arguments": {"format": "iso"}
    },
    "id": 1
  }'
```

### Stdio Mode

**Configuration**:
```json
{
  "mode": "stdio",
  "stdio": {
    "line_buffered": true,
    "timeout_seconds": 30
  }
}
```

**Example Client Request** (pipe):
```bash
echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | mcp-server --stdio
```

**Example Client Request** (Python):
```python
import subprocess
import json

process = subprocess.Popen(
    ['mcp-server', '--stdio'],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    text=True
)

request = {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {
        "name": "calculator",
        "arguments": {"operation": "add", "a": 5, "b": 3}
    },
    "id": 1
}

process.stdin.write(json.dumps(request) + '\n')
process.stdin.flush()

response = json.loads(process.stdout.readline())
print(response['result'])
```

---

## Tool Registration System

### Built-in Tools

Compiled directly into server:

```cpp
// In main.cpp or MCPServer constructor
void MCPServer::registerBuiltInTools() {
    m_toolRegistry->registerTool(new CalculatorTool());
    m_toolRegistry->registerTool(new DateTimeTool());
    m_toolRegistry->registerTool(new SystemInfoTool());
    m_toolRegistry->registerTool(new FileHashTool());
}
```

### Plugin Tools (Future Enhancement)

Dynamically loaded from shared libraries:

```cpp
class ToolPlugin {
public:
    virtual ~ToolPlugin() = default;
    virtual QList<ToolInterface*> createTools() = 0;
};

// Plugin loading
void MCPServer::loadPlugin(const QString &path) {
    QLibrary library(path);
    if (!library.load()) {
        LOG_ERROR("Failed to load plugin: " + path);
        return;
    }
    
    typedef ToolPlugin* (*CreatePluginFunc)();
    CreatePluginFunc createPlugin = (CreatePluginFunc)library.resolve("createPlugin");
    
    if (createPlugin) {
        ToolPlugin *plugin = createPlugin();
        for (ToolInterface *tool : plugin->createTools()) {
            m_toolRegistry->registerTool(tool);
        }
    }
}
```

### Tool Configuration

Tools can have configuration in `/etc/mcp-server/tools.json`:

```json
{
  "tools": {
    "calculator": {
      "enabled": true,
      "max_value": 1000000
    },
    "datetime": {
      "enabled": true,
      "timezone": "UTC"
    },
    "system_info": {
      "enabled": true,
      "include_sensitive": false
    }
  }
}
```

---

## Service Deployment

### Systemd Unit File

`/etc/systemd/system/mcp-server.service`:

```ini
[Unit]
Description=MCP (Model Context Protocol) Server
After=network.target
Documentation=https://github.com/scottgl/mcp-server

[Service]
Type=simple
User=mcp-server
Group=mcp-server
ExecStart=/usr/bin/mcp-server --config /etc/mcp-server/config.json
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=5s
StandardOutput=journal
StandardError=journal
SyslogIdentifier=mcp-server

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/log/mcp-server

# Resource limits
LimitNOFILE=4096
MemoryLimit=256M
CPUQuota=50%

[Install]
WantedBy=multi-user.target
```

### Service Management

```bash
# Enable service
sudo systemctl enable mcp-server

# Start service
sudo systemctl start mcp-server

# Check status
sudo systemctl status mcp-server

# View logs
sudo journalctl -u mcp-server -f

# Reload configuration
sudo systemctl reload mcp-server

# Restart service
sudo systemctl restart mcp-server

# Stop service
sudo systemctl stop mcp-server
```

### Configuration Files

**Main Config** (`/etc/mcp-server/config.json`):
```json
{
  "version": "1.0",
  "mode": "http",
  "http": {
    "host": "0.0.0.0",
    "port": 8080
  },
  "logging": {
    "level": "info",
    "file": "/var/log/mcp-server/server.log",
    "max_size_mb": 100,
    "max_files": 10
  },
  "security": {
    "api_key_required": false,
    "api_key": "",
    "whitelist": []
  },
  "performance": {
    "max_concurrent_requests": 100,
    "request_timeout_seconds": 30,
    "rate_limit_per_minute": 1000
  }
}
```

---

## Security Considerations

### 1. Authentication

**API Key Authentication**:
```cpp
bool HttpServer::validateApiKey(const QHttpRequest &request) {
    if (m_apiKey.isEmpty()) {
        return true;  // No auth required
    }
    
    QString providedKey = request.header("X-API-Key");
    return providedKey == m_apiKey;
}
```

### 2. IP Whitelisting

```cpp
bool HttpServer::isIpAllowed(const QString &ip) {
    if (m_whitelist.isEmpty()) {
        return true;  // No whitelist = allow all
    }
    
    for (const QString &allowed : m_whitelist) {
        if (matchesCIDR(ip, allowed)) {
            return true;
        }
    }
    return false;
}
```

### 3. Rate Limiting

```cpp
class RateLimiter {
public:
    bool checkLimit(const QString &clientId, int maxPerMinute) {
        QMutexLocker locker(&m_mutex);
        
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 windowStart = now - 60000; // 1 minute
        
        // Remove old entries
        m_requests[clientId].erase(
            std::remove_if(m_requests[clientId].begin(), 
                          m_requests[clientId].end(),
                          [windowStart](qint64 ts) { return ts < windowStart; }),
            m_requests[clientId].end()
        );
        
        // Check limit
        if (m_requests[clientId].size() >= maxPerMinute) {
            return false;  // Rate limit exceeded
        }
        
        m_requests[clientId].append(now);
        return true;
    }

private:
    QMutex m_mutex;
    QHash<QString, QList<qint64>> m_requests;
};
```

### 4. Input Sanitization

```cpp
bool RequestRouter::validateRequest(const QJsonObject &request) {
    // Check required fields
    if (!request.contains("jsonrpc") || request["jsonrpc"].toString() != "2.0") {
        return false;
    }
    
    if (!request.contains("method") || !request["method"].isString()) {
        return false;
    }
    
    // Validate method name (alphanumeric + / only)
    QString method = request["method"].toString();
    QRegularExpression methodRegex("^[a-zA-Z0-9/_]+$");
    if (!methodRegex.match(method).hasMatch()) {
        return false;
    }
    
    // Validate params if present
    if (request.contains("params") && !request["params"].isObject()) {
        return false;
    }
    
    return true;
}
```

### 5. TLS/SSL Support (Optional)

For production deployments, use reverse proxy (nginx, Apache):

```nginx
server {
    listen 443 ssl;
    server_name mcp-server.example.com;
    
    ssl_certificate /etc/ssl/certs/mcp-server.crt;
    ssl_certificate_key /etc/ssl/private/mcp-server.key;
    
    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

---

## Testing Strategy

### Unit Tests

```cpp
// tests/test_tool_registry.cpp
class TestToolRegistry : public QObject {
    Q_OBJECT
private slots:
    void testRegisterTool() {
        ToolRegistry registry;
        CalculatorTool *tool = new CalculatorTool();
        
        registry.registerTool(tool);
        
        QVERIFY(registry.hasTool("calculator"));
        QCOMPARE(registry.toolCount(), 1);
    }
    
    void testGetTool() {
        ToolRegistry registry;
        registry.registerTool(new CalculatorTool());
        
        ToolInterface *tool = registry.getTool("calculator");
        
        QVERIFY(tool != nullptr);
        QCOMPARE(tool->name(), "calculator");
    }
    
    void testToolNotFound() {
        ToolRegistry registry;
        
        ToolInterface *tool = registry.getTool("nonexistent");
        
        QVERIFY(tool == nullptr);
    }
};
```

### Integration Tests

```cpp
// tests/test_server_integration.cpp
class TestServerIntegration : public QObject {
    Q_OBJECT
private slots:
    void testHttpToolCall() {
        MCPServer server;
        server.start();
        
        // Make HTTP request
        QNetworkRequest request(QUrl("http://localhost:8080/jsonrpc"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        QJsonObject jsonRequest {
            {"jsonrpc", "2.0"},
            {"method", "tools/call"},
            {"params", QJsonObject{
                {"name", "calculator"},
                {"arguments", QJsonObject{
                    {"operation", "add"},
                    {"a", 5},
                    {"b", 3}
                }}
            }},
            {"id", 1}
        };
        
        QNetworkReply *reply = m_nam.post(request, 
            QJsonDocument(jsonRequest).toJson());
        
        // Wait for response
        QSignalSpy spy(reply, &QNetworkReply::finished);
        QVERIFY(spy.wait(5000));
        
        // Verify response
        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        QVERIFY(response["result"].toObject()["result"].toInt() == 8);
    }
};
```

### Performance Tests

```cpp
// tests/test_performance.cpp
void testConcurrentRequests() {
    MCPServer server;
    server.start();
    
    QVector<QFuture<void>> futures;
    
    // Launch 100 concurrent requests
    for (int i = 0; i < 100; ++i) {
        futures.append(QtConcurrent::run([&]() {
            // Make request
            makeToolCallRequest("calculator", {...});
        }));
    }
    
    // Wait for all to complete
    for (auto &future : futures) {
        future.waitForFinished();
    }
    
    // Verify all succeeded
    QVERIFY(allRequestsSucceeded());
}
```

---

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1-2)
- [ ] Project setup (CMake, directory structure)
- [ ] MCPServer main class
- [ ] RequestRouter with JSON-RPC parsing
- [ ] ToolRegistry and ToolInterface
- [ ] ToolExecutor
- [ ] Configuration system
- [ ] Logging system
- [ ] Unit tests for core components

### Phase 2: HTTP Protocol (Week 3)
- [ ] HttpServer implementation
- [ ] QTcpServer integration
- [ ] HTTP request/response handling
- [ ] API key authentication
- [ ] IP whitelisting
- [ ] Health check endpoint
- [ ] HTTP integration tests

### Phase 3: Stdio Protocol (Week 4)
- [ ] StdioHandler implementation
- [ ] Line-buffered input/output
- [ ] EOF handling
- [ ] Stdio integration tests
- [ ] Compatibility with existing clients

### Phase 4: Built-in Tools (Week 5)
- [ ] Calculator tool
- [ ] DateTime tool
- [ ] System info tool
- [ ] File hash tool
- [ ] Tool unit tests
- [ ] Tool parameter validation

### Phase 5: Service Deployment (Week 6)
- [ ] Systemd unit file
- [ ] Service installation script
- [ ] Configuration file templates
- [ ] Log rotation setup
- [ ] User/group creation
- [ ] Permissions and security hardening

### Phase 6: Debian Packaging (Week 7)
- [ ] debian/ directory structure
- [ ] Package metadata (control, changelog)
- [ ] Build rules
- [ ] Pre/post install scripts
- [ ] Configuration file handling
- [ ] Package testing

### Phase 7: Documentation (Week 8)
- [ ] User guide (installation, configuration)
- [ ] API documentation
- [ ] Tool development guide
- [ ] Deployment guide
- [ ] Troubleshooting guide
- [ ] Example clients

### Phase 8: Testing and Hardening (Week 9-10)
- [ ] Comprehensive test suite
- [ ] Performance benchmarking
- [ ] Security audit
- [ ] Load testing
- [ ] Memory leak detection
- [ ] Code coverage analysis

---

## Success Metrics

### Technical Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Build Success | 100% | CMake + make clean build |
| Test Coverage | >80% | gcov/lcov report |
| Memory Usage (Idle) | <50MB | systemd MemoryCurrent |
| CPU Usage (Idle) | <5% | systemd CPUUsageNSec |
| Response Time (Simple) | <100ms | Tool execution latency |
| Concurrent Requests | 100+ | Load testing |
| Uptime | 99.9% | systemd monitoring |
| Restart Time | <5s | systemd ExecStop/ExecStart |

### Functional Metrics

- [ ] All MCP methods implemented (tools/list, tools/call)
- [ ] Both HTTP and stdio modes working
- [ ] At least 4 built-in tools
- [ ] Systemd service runs reliably
- [ ] Configuration reload without restart
- [ ] Graceful shutdown (no data loss)
- [ ] Comprehensive logging

### Quality Metrics

- [ ] Zero critical bugs
- [ ] No memory leaks (valgrind clean)
- [ ] No security vulnerabilities
- [ ] Passes lintian checks (Debian package)
- [ ] Documentation complete
- [ ] Example clients provided

---

## Deliverables

### Code Deliverables

1. **Source Code**: Complete C++/Qt5 implementation
2. **Build System**: CMakeLists.txt for all components
3. **Tests**: Unit and integration tests
4. **Tools**: At least 4 built-in tools
5. **Configuration**: Default config files

### Deployment Deliverables

1. **Systemd Unit**: Service file
2. **Installation Script**: Setup script
3. **Debian Package**: .deb package
4. **Docker Image**: Optional containerized deployment

### Documentation Deliverables

1. **README.md**: Quick start guide
2. **INSTALLATION.md**: Detailed installation
3. **CONFIGURATION.md**: Configuration reference
4. **API.md**: API documentation
5. **TOOL_DEVELOPMENT.md**: Tool creation guide
6. **TROUBLESHOOTING.md**: Common issues

---

## Future Enhancements

### Short-term (3-6 months)
- Plugin system for external tools
- WebSocket support for persistent connections
- Prometheus metrics endpoint
- Docker Compose deployment
- Kubernetes Helm chart

### Medium-term (6-12 months)
- Tool versioning and migration
- Multi-instance clustering
- Tool result caching
- GraphQL endpoint (alternative to JSON-RPC)
- Web UI for monitoring

### Long-term (12+ months)
- gRPC protocol support
- Tool marketplace
- AI-assisted tool discovery
- Distributed tool execution
- Edge deployment support

---

## Risks and Mitigations

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Qt5 API changes | High | Low | Pin Qt version, thorough testing |
| Performance bottlenecks | Medium | Medium | Load testing, profiling, optimization |
| Security vulnerabilities | High | Medium | Security audit, input validation, rate limiting |
| Systemd compatibility | Medium | Low | Test on multiple distros, document requirements |
| Tool execution timeout | Medium | High | Implement timeout, async execution |
| Memory leaks | High | Medium | Valgrind testing, RAII patterns |

---

## Appendix A: JSON-RPC 2.0 Examples

### Batch Request

**Request**:
```json
[
  {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {"name": "calculator", "arguments": {"operation": "add", "a": 5, "b": 3}},
    "id": 1
  },
  {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {"name": "datetime", "arguments": {"format": "iso"}},
    "id": 2
  }
]
```

**Response**:
```json
[
  {
    "jsonrpc": "2.0",
    "result": {"result": 8},
    "id": 1
  },
  {
    "jsonrpc": "2.0",
    "result": {"datetime": "2025-10-10T15:30:00Z"},
    "id": 2
  }
]
```

### Notification (No Response Expected)

**Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "log/event",
  "params": {"level": "info", "message": "User action recorded"}
}
```

---

## Appendix B: CMake Build System

```cmake
cmake_minimum_required(VERSION 3.10)
project(mcp-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

# Server library
add_library(mcp-server-lib
    src/MCPServer.cpp
    src/HttpServer.cpp
    src/StdioHandler.cpp
    src/RequestRouter.cpp
    src/ToolRegistry.cpp
    src/ToolExecutor.cpp
    src/ResponseBuilder.cpp
    src/Logger.cpp
    src/Config.cpp
)
target_include_directories(mcp-server-lib PUBLIC include)
target_link_libraries(mcp-server-lib Qt5::Core Qt5::Network)

# Built-in tools
add_library(mcp-tools
    tools/CalculatorTool.cpp
    tools/DateTimeTool.cpp
    tools/SystemInfoTool.cpp
)
target_include_directories(mcp-tools PUBLIC include)
target_link_libraries(mcp-tools Qt5::Core)

# Main executable
add_executable(mcp-server src/main.cpp)
target_link_libraries(mcp-server mcp-server-lib mcp-tools)

# Tests
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS mcp-server DESTINATION bin)
install(FILES systemd/mcp-server.service DESTINATION /etc/systemd/system)
install(FILES systemd/mcp-server.conf DESTINATION /etc/mcp-server)
```

---

**Document Status**: Draft  
**Next Review**: November 1, 2025  
**Approvers**: Engineering Team, Security Team

---

## References

1. [Model Context Protocol Specification](https://spec.modelcontextprotocol.io/)
2. [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
3. [Qt5 Documentation](https://doc.qt.io/qt-5/)
4. [systemd Service Documentation](https://www.freedesktop.org/software/systemd/man/systemd.service.html)
5. [Debian Policy Manual](https://www.debian.org/doc/debian-policy/)
