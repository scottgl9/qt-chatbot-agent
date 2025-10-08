# MCP Testing Guide

## Overview

This document describes the comprehensive test suite for the Model Context Protocol (MCP) implementation in qt-chatbot-agent. The tests validate both local and networked tool execution, error handling, and server communication.

## Test Structure

The MCP test suite consists of three test files:

1. **test_config.cpp** - Configuration management tests
2. **test_mcphandler.cpp** - Basic MCP handler functionality tests
3. **test_mcp_server.cpp** - Advanced tests with test server (NEW)

## Test MCP Server

### Purpose

The test MCP server simulates a real networked tool server, allowing us to validate:
- Network communication
- HTTP protocol handling
- JSON serialization/deserialization
- Asynchronous signal handling
- Error conditions
- Concurrent tool calls

### Implementation

**File**: `tests/test_mcp_server.cpp`

The test server is implemented using Qt's `QTcpServer` and provides:

```cpp
class TestMCPServer : public QObject {
    // Starts HTTP server on localhost with random port
    bool start();

    // Get server URL for testing
    QString url() const;

    // Set predefined response
    void setResponse(const QJsonObject &response);

    // Simulate error condition
    void setError(const QString &error);
};
```

### How It Works

1. **Startup**: Server listens on a random available port
2. **Request Handling**: Accepts HTTP POST requests with JSON body
3. **Response**: Returns JSON response based on:
   - Preset response via `setResponse()`
   - Error simulation via `setError()`
   - Default echo response (for testing)

4. **Cleanup**: Automatically closes connections and cleans up resources

## Test Cases

### 1. Networked Tool Registration

**Test**: `testNetworkedToolRegistration()`

**Purpose**: Verify networked tools can be registered correctly

**Validates**:
- Tool registration succeeds
- Tool appears in registered tools list
- Tool metadata is correct (name, URL, isLocal=false)

```cpp
handler.registerNetworkedTool(
    "search",
    "Search for information",
    params,
    toolUrl
);

QVERIFY(handler.getRegisteredTools().contains("search"));
```

### 2. Networked Tool Execution

**Test**: `testNetworkedToolExecution()`

**Purpose**: Validate complete networked tool call flow

**Validates**:
- Tool executes with correct parameters
- HTTP request is sent to server
- Response is received and parsed
- `toolCallCompleted` signal is emitted with correct data
- Call ID is unique and returned

**Flow**:
1. Register networked tool
2. Set server response
3. Execute tool with parameters
4. Wait for `toolCallCompleted` signal
5. Verify signal arguments match expected values

```cpp
QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);

QString callId = handler.executeToolCall("calculate", params);

QVERIFY(completedSpy.wait(5000));
QCOMPARE(completedSpy.count(), 1);
```

### 3. Networked Tool Error Handling

**Test**: `testNetworkedToolError()`

**Purpose**: Validate error handling for networked tools

**Validates**:
- Server errors are detected
- Error responses trigger `toolCallFailed` signal
- Error message is propagated correctly

**Server Response**:
```json
{
  "error": "Simulated server error"
}
```

**Expected**: `toolCallFailed` signal emitted with error message

### 4. Local Tool Execution

**Test**: `testLocalToolExecution()`

**Purpose**: Validate local tool execution for comparison

**Validates**:
- Local tools execute synchronously
- Results are computed correctly
- `toolCallCompleted` signal is emitted
- No network communication occurs

```cpp
localTool.function = [](const QJsonObject &params) -> QJsonObject {
    int a = params["a"].toInt();
    int b = params["b"].toInt();
    QJsonObject result;
    result["sum"] = a + b;
    return result;
};
```

### 5. Multiple Concurrent Calls

**Test**: `testMultipleConcurrentCalls()`

**Purpose**: Validate handling of concurrent tool calls

**Validates**:
- Multiple tools can be called simultaneously
- Call IDs are unique
- Responses are correctly matched to calls
- Both requests complete successfully

**Scenario**:
- Call tool1 with params1
- Call tool2 with params2 (before tool1 completes)
- Wait for both completions
- Verify both received correct responses

### 6. Get Tools for LLM

**Test**: `testGetToolsForLLMWithNetworked()`

**Purpose**: Validate tool list generation for LLM

**Validates**:
- Both local and networked tools appear in list
- Tool metadata is formatted correctly
- JSON structure matches LLM expectations

**Expected Format**:
```json
[
  {
    "name": "local",
    "description": "Local tool",
    "parameters": {"param": "string"}
  },
  {
    "name": "networked",
    "description": "Networked tool",
    "parameters": {"param": "string"}
  }
]
```

### 7. Invalid Tool Name

**Test**: `testInvalidToolName()`

**Purpose**: Validate error handling for invalid tool calls

**Validates**:
- Calling non-existent tool fails gracefully
- `toolCallFailed` signal is emitted
- Error message indicates tool not found

```cpp
handler.executeToolCall("nonexistent", QJsonObject());

QVERIFY(failedSpy.wait(1000));
QString errorMessage = arguments.at(2).toString();
QVERIFY(errorMessage.contains("not found"));
```

## Test Statistics

### Test Coverage

- **Total Test Files**: 3
- **Total Test Cases**: 21 (7 Config + 7 MCPHandler + 7 MCPServer)
- **Success Rate**: 100% (21/21 passing)
- **Execution Time**: ~0.5 seconds

### Code Coverage

**Classes Tested**:
- Config (src/Config.cpp)
- MCPHandler (src/MCPHandler.cpp)
- MCPMessage struct
- MCPTool struct

**Functionality Tested**:
- Local tool registration and execution
- Networked tool registration and execution
- Tool validation
- Signal emission (success and failure)
- JSON serialization/deserialization
- HTTP communication
- Error handling
- Concurrent execution

## Running Tests

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

**Output**:
```
Test project /path/to/build
    Start 1: ConfigTest
1/3 Test #1: ConfigTest ...................   Passed    0.02 sec
    Start 2: MCPHandlerTest
2/3 Test #2: MCPHandlerTest ...............   Passed    0.02 sec
    Start 3: MCPServerTest
3/3 Test #3: MCPServerTest ................   Passed    0.46 sec

100% tests passed, 0 tests failed out of 3
```

### Run Specific Test

```bash
# Run only MCP server tests
ctest -R MCPServerTest --output-on-failure

# Run with verbose output
ctest -R MCPServerTest -V
```

### Run Individual Test Executable

```bash
./bin/test_mcp_server

# Run specific test function
./bin/test_mcp_server testNetworkedToolExecution
```

## Technical Details

### Signal Timing

**Issue**: Qt signals may be emitted before QSignalSpy is ready

**Solution**:
```cpp
// Process events to ensure signal connections
QCoreApplication::processEvents();

// Check if signal already emitted
if (completedSpy.count() == 0) {
    QVERIFY(completedSpy.wait(1000));
}
```

### Network Manager Initialization

**Issue**: QNetworkAccessManager is initialized asynchronously

**Solution**:
```cpp
// Wait for deferred initialization
QTest::qWait(100);
```

This ensures the network manager is ready before executing networked tools.

### HTTP Protocol

The test server implements a minimal HTTP server:

```cpp
QString httpResponse =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: " + QString::number(responseBody.length()) + "\r\n"
    "Connection: close\r\n"
    "\r\n";

socket->write(httpResponse.toUtf8());
socket->write(responseBody);
```

### Error Detection

The MCPHandler now checks for error fields in responses:

```cpp
if (result.contains("error")) {
    QString error = result["error"].toString();
    emit toolCallFailed(toolCallId, toolName, error);
    return;
}
```

This allows servers to return structured error responses.

## Best Practices

### Writing New Tests

1. **Wait for Initialization**: Always wait for deferred initialization
   ```cpp
   QTest::qWait(100);
   ```

2. **Use Signal Spies**: For asynchronous operations
   ```cpp
   QSignalSpy spy(&handler, &MCPHandler::toolCallCompleted);
   QVERIFY(spy.wait(5000));
   ```

3. **Process Events**: Ensure signals are delivered
   ```cpp
   QCoreApplication::processEvents();
   ```

4. **Verify All Aspects**: Check count, arguments, and values
   ```cpp
   QCOMPARE(spy.count(), 1);
   QList<QVariant> args = spy.takeFirst();
   QCOMPARE(args.at(0).toString(), expectedValue);
   ```

5. **Clean Up**: Use init/cleanup for test state
   ```cpp
   void init() {
       m_testServer->clearError();
   }
   ```

### Common Patterns

**Testing Networked Tool**:
```cpp
void testMyNetworkedTool() {
    MCPHandler handler;
    QTest::qWait(100);  // Wait for network manager

    handler.registerNetworkedTool(...);

    QSignalSpy completedSpy(&handler, &MCPHandler::toolCallCompleted);

    handler.executeToolCall("my_tool", params);

    QVERIFY(completedSpy.wait(5000));
    QCOMPARE(completedSpy.count(), 1);

    // Verify result
    QJsonObject result = completedSpy.takeFirst().at(2).toJsonObject();
    QVERIFY(result.contains("expected_field"));
}
```

**Testing Error Conditions**:
```cpp
void testMyError() {
    MCPHandler handler;
    QTest::qWait(100);

    m_testServer->setError("My error message");

    QSignalSpy failedSpy(&handler, &MCPHandler::toolCallFailed);

    handler.executeToolCall("tool", params);

    QVERIFY(failedSpy.wait(5000));
    QCOMPARE(failedSpy.count(), 1);

    QString error = failedSpy.takeFirst().at(2).toString();
    QVERIFY(error.contains("My error message"));
}
```

## Debugging Tests

### Enable Verbose Logging

Tests use Qt's logging system. Enable debug output:

```bash
QT_LOGGING_RULES="*.debug=true" ./bin/test_mcp_server
```

### Common Issues

**Issue**: Signal not received
**Solution**: Ensure event loop runs (use `QTest::qWait()` or `processEvents()`)

**Issue**: Network manager not initialized
**Solution**: Add `QTest::qWait(100)` after creating MCPHandler

**Issue**: Test hangs
**Solution**: Check timeout values, ensure server is responding

**Issue**: Intermittent failures
**Solution**: Increase wait times, check for race conditions

## Integration with CI/CD

The tests are integrated with CTest and can be run in CI/CD pipelines:

```yaml
# Example GitLab CI
test:
  script:
    - cd build
    - cmake ..
    - make
    - ctest --output-on-failure
```

**Timeout Settings**:
- ConfigTest: 30 seconds
- MCPHandlerTest: 30 seconds
- MCPServerTest: 60 seconds (allows for network delays)

## Future Enhancements

### Planned Tests

- [ ] LLMClient unit tests (network mocking)
- [ ] End-to-end integration tests
- [ ] Performance benchmarks for tool execution
- [ ] Stress tests (many concurrent calls)
- [ ] Memory leak detection
- [ ] Thread safety tests

### Test Server Enhancements

- [ ] Support for multiple response scenarios
- [ ] Simulate network delays
- [ ] Simulate connection failures
- [ ] Support for streaming responses
- [ ] Request validation and logging

## Conclusion

The MCP test suite provides comprehensive coverage of the Model Context Protocol implementation, including both local and networked tools. The test server allows realistic validation of network communication and error handling without requiring external dependencies.

**Key Achievements**:
- ✅ 100% test pass rate
- ✅ Both local and networked tools tested
- ✅ Error conditions validated
- ✅ Concurrent execution tested
- ✅ Fast execution (~0.5 seconds)
- ✅ No external dependencies

**Test Quality**:
- Clear, descriptive test names
- Comprehensive validation
- Good error messages
- Follows Qt Test best practices
- Well-documented

---

**Status**: ✅ Production ready
**Version**: 1.0.0
**Last Updated**: 2025-10-08
