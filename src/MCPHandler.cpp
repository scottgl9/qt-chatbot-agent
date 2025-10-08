/**
 * MCPHandler.cpp - Model Context Protocol tool management
 * 
 * Manages MCP tool registration (local and networked), tool execution,
 * server discovery, and tool result formatting.
 */

#include "MCPHandler.h"
#include "SSEClient.h"
#include "Logger.h"
#include "version.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>
#include <QTimer>
#include <QEventLoop>

// ============================================================================
// MCPMessage Implementation
// ============================================================================

QJsonObject MCPMessage::toJson() const {
    QJsonObject json;
    json["role"] = role;
    json["content"] = content;

    if (!context.isEmpty()) {
        json["context"] = context;
    }

    if (!toolCallId.isEmpty()) {
        json["tool_call_id"] = toolCallId;
    }

    if (!toolName.isEmpty()) {
        json["tool_name"] = toolName;
    }

    return json;
}

MCPMessage MCPMessage::fromJson(const QJsonObject &json) {
    MCPMessage msg;
    msg.role = json["role"].toString();
    msg.content = json["content"].toString();
    msg.context = json["context"].toObject();
    msg.toolCallId = json["tool_call_id"].toString();
    msg.toolName = json["tool_name"].toString();
    return msg;
}

// ============================================================================
// MCPTool Implementation
// ============================================================================

bool MCPTool::isValid() const {
    if (name.isEmpty() || description.isEmpty()) {
        return false;
    }

    switch (toolType) {
        case MCPToolType::Local:
            // Local tools must have a function
            return function != nullptr;

        case MCPToolType::HTTP:
        case MCPToolType::SSE:
            // Network-based tools must have a valid URL
            return !networkUrl.isEmpty();

        default:
            return false;
    }
}

// ============================================================================
// MCPHandler Implementation
// ============================================================================

MCPHandler::MCPHandler(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_toolCallCounter(0) {

    // Defer network manager creation until event loop is running
    QTimer::singleShot(0, this, [this]() {
        m_networkManager = new QNetworkAccessManager(this);
        LOG_DEBUG("MCPHandler: QNetworkAccessManager initialized");
    });

    LOG_INFO("MCPHandler initialized");
}

MCPHandler::~MCPHandler() {
    // Disconnect SSE clients (but don't delete - they have 'this' as parent)
    for (auto *client : m_sseClients.values()) {
        client->disconnect();
    }
    m_sseClients.clear();
    m_sseToolCalls.clear();

    // m_networkManager and SSE clients have 'this' as parent, so Qt will auto-delete them
    // No manual deletion needed to avoid double-free
}

bool MCPHandler::registerTool(const MCPTool &tool) {
    if (!tool.isValid()) {
        LOG_ERROR(QString("Cannot register invalid tool: %1").arg(tool.name));
        return false;
    }

    if (m_tools.contains(tool.name)) {
        LOG_WARNING(QString("Tool '%1' already registered, replacing").arg(tool.name));
    }

    m_tools[tool.name] = tool;

    QString typeStr;
    switch (tool.toolType) {
        case MCPToolType::Local: typeStr = "local"; break;
        case MCPToolType::HTTP: typeStr = "HTTP"; break;
        case MCPToolType::SSE: typeStr = "SSE"; break;
        default: typeStr = "unknown"; break;
    }

    LOG_INFO(QString("Registered %1 tool: %2 - %3")
             .arg(typeStr)
             .arg(tool.name)
             .arg(tool.description));

    return true;
}

bool MCPHandler::registerNetworkedTool(const QString &name,
                                       const QString &description,
                                       const QJsonObject &parameters,
                                       const QString &networkUrl) {
    MCPTool tool;
    tool.name = name;
    tool.description = description;
    tool.parameters = parameters;
    tool.toolType = MCPToolType::HTTP;
    tool.isLocal = false;  // For backward compatibility
    tool.networkUrl = networkUrl;
    tool.function = nullptr;

    return registerTool(tool);
}

bool MCPHandler::unregisterTool(const QString &name) {
    if (m_tools.remove(name) > 0) {
        LOG_INFO(QString("Unregistered tool: %1").arg(name));
        return true;
    }

    LOG_WARNING(QString("Tool not found for unregistration: %1").arg(name));
    return false;
}

int MCPHandler::clearNetworkedTools() {
    QStringList networkedToolNames;

    // Identify all networked tools (HTTP and SSE)
    for (auto it = m_tools.begin(); it != m_tools.end(); ++it) {
        const MCPTool &tool = it.value();
        if (tool.toolType == MCPToolType::HTTP || tool.toolType == MCPToolType::SSE) {
            networkedToolNames.append(it.key());
        }
    }

    // Remove networked tools
    int removedCount = 0;
    for (const QString &toolName : networkedToolNames) {
        if (m_tools.remove(toolName) > 0) {
            removedCount++;
            LOG_DEBUG(QString("Cleared networked tool: %1").arg(toolName));
        }
    }

    if (removedCount > 0) {
        LOG_INFO(QString("Cleared %1 networked tools (kept %2 local tools)")
                 .arg(removedCount).arg(m_tools.size()));
    } else {
        LOG_DEBUG("No networked tools to clear");
    }

    return removedCount;
}

int MCPHandler::discoverAndRegisterServerTools(const QString &serverName,
                                               const QString &serverUrl,
                                               const QString &serverType) {
    LOG_INFO(QString("Discovering tools from MCP server: %1 (%2) at %3")
             .arg(serverName, serverType.toUpper(), serverUrl));

    // Ensure network manager is initialized
    if (!m_networkManager) {
        LOG_ERROR("Network manager not initialized yet. Please wait for initialization.");
        return -1;
    }

    QUrl url(serverUrl);
    if (!url.isValid()) {
        LOG_ERROR(QString("Invalid server URL: %1").arg(serverUrl));
        return -1;
    }

    // Step 1: Send initialize request (MCP JSON-RPC 2.0)
    QJsonObject initRequest;
    initRequest["jsonrpc"] = "2.0";
    initRequest["id"] = 1;
    initRequest["method"] = "initialize";
    initRequest["params"] = QJsonObject{
        {"protocolVersion", "2024-11-05"},
        {"capabilities", QJsonObject()},  // Required by MCP spec
        {"clientInfo", QJsonObject{
            {"name", APP_NAME},
            {"version", APP_VERSION}
        }}
    };

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json, text/event-stream");

    QByteArray initData = QJsonDocument(initRequest).toJson(QJsonDocument::Compact);
    QNetworkReply *initReply = m_networkManager->post(request, initData);

    // Wait for initialize response (with timeout)
    QEventLoop initLoop;
    QTimer::singleShot(5000, &initLoop, &QEventLoop::quit); // 5 second timeout
    QObject::connect(initReply, &QNetworkReply::finished, &initLoop, &QEventLoop::quit);
    initLoop.exec();

    if (initReply->error() != QNetworkReply::NoError) {
        LOG_ERROR(QString("Failed to initialize MCP server %1: %2")
                  .arg(serverName, initReply->errorString()));
        initReply->deleteLater();
        return -1;
    }

    QByteArray initResponse = initReply->readAll();
    initReply->deleteLater();

    // Handle SSE format response (event: message\ndata: {...})
    QString initResponseStr = QString::fromUtf8(initResponse);
    if (initResponseStr.startsWith("event:")) {
        int dataStart = initResponseStr.indexOf("data: ");
        if (dataStart != -1) {
            initResponseStr = initResponseStr.mid(dataStart + 6).trimmed();
            initResponse = initResponseStr.toUtf8();
        }
    }

    QJsonDocument initDoc = QJsonDocument::fromJson(initResponse);
    if (initDoc.isNull() || !initDoc.isObject()) {
        LOG_ERROR(QString("Invalid initialize response from MCP server: %1").arg(serverName));
        return -1;
    }

    QJsonObject initResult = initDoc.object();
    if (initResult.contains("error")) {
        QString errorMsg = initResult["error"].toObject()["message"].toString();
        LOG_ERROR(QString("MCP server %1 returned error: %2").arg(serverName, errorMsg));
        return -1;
    }

    LOG_DEBUG(QString("MCP server %1 initialized successfully").arg(serverName));

    // Step 2: Send tools/list request
    QJsonObject listRequest;
    listRequest["jsonrpc"] = "2.0";
    listRequest["id"] = 2;
    listRequest["method"] = "tools/list";
    listRequest["params"] = QJsonObject();

    QByteArray listData = QJsonDocument(listRequest).toJson(QJsonDocument::Compact);
    QNetworkReply *listReply = m_networkManager->post(request, listData);

    // Wait for tools/list response
    QEventLoop listLoop;
    QTimer::singleShot(5000, &listLoop, &QEventLoop::quit);
    QObject::connect(listReply, &QNetworkReply::finished, &listLoop, &QEventLoop::quit);
    listLoop.exec();

    if (listReply->error() != QNetworkReply::NoError) {
        LOG_ERROR(QString("Failed to list tools from MCP server %1: %2")
                  .arg(serverName, listReply->errorString()));
        listReply->deleteLater();
        return -1;
    }

    QByteArray listResponse = listReply->readAll();
    listReply->deleteLater();

    // Handle SSE format response
    QString listResponseStr = QString::fromUtf8(listResponse);
    if (listResponseStr.startsWith("event:")) {
        int dataStart = listResponseStr.indexOf("data: ");
        if (dataStart != -1) {
            listResponseStr = listResponseStr.mid(dataStart + 6).trimmed();
            listResponse = listResponseStr.toUtf8();
        }
    }

    QJsonDocument listDoc = QJsonDocument::fromJson(listResponse);
    if (listDoc.isNull() || !listDoc.isObject()) {
        LOG_ERROR(QString("Invalid tools/list response from MCP server: %1").arg(serverName));
        return -1;
    }

    QJsonObject listResult = listDoc.object();
    if (listResult.contains("error")) {
        QString errorMsg = listResult["error"].toObject()["message"].toString();
        LOG_ERROR(QString("MCP server %1 returned error for tools/list: %2")
                  .arg(serverName, errorMsg));
        return -1;
    }

    // Step 3: Extract and register tools
    QJsonObject result = listResult["result"].toObject();
    QJsonArray tools = result["tools"].toArray();

    if (tools.isEmpty()) {
        LOG_WARNING(QString("MCP server %1 has no tools").arg(serverName));
        return 0;
    }

    LOG_INFO(QString("Discovered %1 tools from MCP server: %2").arg(tools.size()).arg(serverName));

    int registeredCount = 0;
    for (const QJsonValue &toolVal : tools) {
        if (!toolVal.isObject()) {
            continue;
        }

        QJsonObject tool = toolVal.toObject();
        QString toolName = tool["name"].toString();
        QString toolDesc = tool["description"].toString();
        QJsonObject toolParams = tool["inputSchema"].toObject();

        if (toolName.isEmpty()) {
            LOG_WARNING(QString("Skipping tool with empty name from server: %1").arg(serverName));
            continue;
        }

        // Register the networked tool
        bool success = registerNetworkedTool(
            toolName,
            toolDesc.isEmpty() ? QString("Tool from %1").arg(serverName) : toolDesc,
            toolParams,
            serverUrl
        );

        if (success) {
            registeredCount++;
            LOG_DEBUG(QString("Registered tool '%1' from MCP server: %2").arg(toolName, serverName));
        } else {
            LOG_WARNING(QString("Failed to register tool '%1' from MCP server: %2")
                        .arg(toolName, serverName));
        }
    }

    LOG_INFO(QString("Successfully registered %1/%2 tools from MCP server: %3")
             .arg(registeredCount).arg(tools.size()).arg(serverName));

    return registeredCount;
}

QStringList MCPHandler::getRegisteredTools() const {
    return m_tools.keys();
}

const MCPTool* MCPHandler::getTool(const QString &name) const {
    auto it = m_tools.find(name);
    if (it != m_tools.end()) {
        return &it.value();
    }
    return nullptr;
}

QStringList MCPHandler::extractToolCalls(const MCPMessage &message) const {
    QStringList tools;

    // Extract tool names from context
    if (message.context.contains("tools")) {
        QJsonArray toolsArray = message.context["tools"].toArray();
        for (const QJsonValue &value : toolsArray) {
            QString toolName = value.toString();
            if (!toolName.isEmpty()) {
                tools.append(toolName);
            }
        }
    }

    return tools;
}

QString MCPHandler::executeToolCall(const QString &toolName,
                                    const QJsonObject &parameters) {
    if (!m_tools.contains(toolName)) {
        QString error = QString("Tool not found: %1").arg(toolName);
        LOG_ERROR(error);
        QString callId = generateToolCallId();
        emit toolCallFailed(callId, toolName, error);
        return callId;
    }

    QString toolCallId = generateToolCallId();
    const MCPTool &tool = m_tools[toolName];

    LOG_INFO(QString("Executing tool: %1 (call_id: %2)").arg(toolName, toolCallId));
    LOG_DEBUG(QString("Tool parameters: %1")
              .arg(QString::fromUtf8(QJsonDocument(parameters).toJson(QJsonDocument::Compact))));

    switch (tool.toolType) {
        case MCPToolType::Local:
            executeLocalTool(toolCallId, toolName, parameters);
            break;

        case MCPToolType::HTTP:
            executeNetworkedTool(toolCallId, toolName, parameters);
            break;

        case MCPToolType::SSE:
            executeSSETool(toolCallId, toolName, parameters);
            break;

        default:
            QString error = QString("Unknown tool type for: %1").arg(toolName);
            LOG_ERROR(error);
            emit toolCallFailed(toolCallId, toolName, error);
            break;
    }

    return toolCallId;
}

QJsonArray MCPHandler::getToolsForLLM() const {
    QJsonArray toolsArray;

    for (const MCPTool &tool : m_tools) {
        QJsonObject toolObj;
        toolObj["name"] = tool.name;
        toolObj["description"] = tool.description;
        toolObj["parameters"] = tool.parameters;
        toolsArray.append(toolObj);
    }

    return toolsArray;
}

QJsonArray MCPHandler::getToolsForLLMNative() const {
    QJsonArray toolsArray;

    for (const MCPTool &tool : m_tools) {
        // Convert to OpenAI/Ollama native format
        QJsonObject functionDef;
        functionDef["name"] = tool.name;
        functionDef["description"] = tool.description;

        // Convert simple parameter format to JSON Schema format
        QJsonObject parameters = tool.parameters;
        if (!parameters.contains("type")) {
            // If parameters don't have JSON Schema format, wrap them
            QJsonObject schema;
            schema["type"] = "object";
            QJsonObject properties;

            // Convert each parameter to a property
            for (const QString &key : parameters.keys()) {
                QJsonObject propSchema;
                QString paramDesc = parameters[key].toString();

                // Parse description like "string: description" or "number: description"
                if (paramDesc.contains(":")) {
                    QStringList parts = paramDesc.split(":", Qt::SkipEmptyParts);
                    if (parts.size() >= 2) {
                        QString type = parts[0].trimmed().toLower();
                        QString desc = parts.mid(1).join(":").trimmed();

                        if (type.contains("string")) {
                            propSchema["type"] = "string";
                        } else if (type.contains("number") || type.contains("int")) {
                            propSchema["type"] = "number";
                        } else if (type.contains("bool")) {
                            propSchema["type"] = "boolean";
                        } else {
                            propSchema["type"] = "string";
                        }

                        propSchema["description"] = desc;
                    }
                } else {
                    propSchema["type"] = "string";
                    propSchema["description"] = paramDesc;
                }

                properties[key] = propSchema;
            }

            schema["properties"] = properties;
            schema["required"] = QJsonArray(); // Could be improved to mark required params
            functionDef["parameters"] = schema;
        } else {
            // Already in JSON Schema format
            functionDef["parameters"] = parameters;
        }

        // Wrap in OpenAI format
        QJsonObject toolObj;
        toolObj["type"] = "function";
        toolObj["function"] = functionDef;

        toolsArray.append(toolObj);
    }

    return toolsArray;
}

MCPMessage MCPHandler::buildMessage(const QString &role,
                                    const QString &content,
                                    const QStringList &toolNames) const {
    MCPMessage msg;
    msg.role = role;
    msg.content = content;

    if (!toolNames.isEmpty()) {
        QJsonArray toolsArray;
        for (const QString &toolName : toolNames) {
            toolsArray.append(toolName);
        }
        msg.context["tools"] = toolsArray;
    }

    return msg;
}

void MCPHandler::executeLocalTool(const QString &toolCallId,
                                  const QString &toolName,
                                  const QJsonObject &parameters) {
    const MCPTool &tool = m_tools[toolName];

    try {
        // Execute the tool function
        QJsonObject result = tool.function(parameters);

        LOG_INFO(QString("Local tool '%1' completed successfully").arg(toolName));
        LOG_DEBUG(QString("Tool result: %1")
                  .arg(QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))));

        emit toolCallCompleted(toolCallId, toolName, result);

    } catch (const std::exception &e) {
        QString error = QString("Tool execution error: %1").arg(e.what());
        LOG_ERROR(QString("Local tool '%1' failed: %2").arg(toolName, error));
        emit toolCallFailed(toolCallId, toolName, error);

    } catch (...) {
        QString error = "Unknown error during tool execution";
        LOG_ERROR(QString("Local tool '%1' failed: %2").arg(toolName, error));
        emit toolCallFailed(toolCallId, toolName, error);
    }
}

void MCPHandler::executeNetworkedTool(const QString &toolCallId,
                                      const QString &toolName,
                                      const QJsonObject &parameters) {
    // Ensure network manager is initialized
    if (!m_networkManager) {
        QString error = "Network manager not initialized yet. Please wait for initialization.";
        LOG_ERROR(QString("Networked tool '%1' failed: %2").arg(toolName, error));
        emit toolCallFailed(toolCallId, toolName, error);
        return;
    }

    const MCPTool &tool = m_tools[toolName];

    // Build request
    QUrl url(tool.networkUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json, text/event-stream");  // Support SSE responses

    // Create MCP JSON-RPC 2.0 request body
    QJsonObject requestBody;
    requestBody["jsonrpc"] = "2.0";
    requestBody["id"] = m_toolCallCounter++;
    requestBody["method"] = "tools/call";

    QJsonObject params;
    params["name"] = toolName;
    params["arguments"] = parameters;
    requestBody["params"] = params;

    QByteArray jsonData = QJsonDocument(requestBody).toJson();

    LOG_INFO(QString("Sending networked tool request: %1 to %2")
             .arg(toolName, tool.networkUrl));
    LOG_DEBUG(QString("   Call ID: \"%1\"").arg(toolCallId));

    // Send POST request
    QNetworkReply *reply = m_networkManager->post(request, jsonData);

    // Store pending call
    m_pendingCalls[QString::number(reinterpret_cast<quintptr>(reply))] = toolCallId + ":" + toolName;

    // Connect to response handler
    connect(reply, &QNetworkReply::finished,
            this, &MCPHandler::handleNetworkToolResponse);
}

void MCPHandler::handleNetworkToolResponse() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    // Get tool call info
    QString replyKey = QString::number(reinterpret_cast<quintptr>(reply));
    QString callInfo = m_pendingCalls.value(replyKey, "");
    m_pendingCalls.remove(replyKey);

    QStringList parts = callInfo.split(":");
    if (parts.size() != 2) {
        LOG_ERROR("Invalid pending call info");
        reply->deleteLater();
        return;
    }

    QString toolCallId = parts[0];
    QString toolName = parts[1];

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("Network error: %1").arg(reply->errorString());
        LOG_ERROR(QString("Networked tool '%1' failed: %2").arg(toolName, error));
        emit toolCallFailed(toolCallId, toolName, error);
        reply->deleteLater();
        return;
    }

    // Parse response
    QByteArray responseData = reply->readAll();
    QString responseStr = QString::fromUtf8(responseData);

    // Handle SSE format if present
    if (responseStr.startsWith("event:")) {
        // Extract JSON from SSE format: "event: message\ndata: {...}"
        int dataStart = responseStr.indexOf("data: ");
        if (dataStart != -1) {
            responseStr = responseStr.mid(dataStart + 6).trimmed();
            responseData = responseStr.toUtf8();
        }
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("JSON parse error: %1").arg(parseError.errorString());
        LOG_ERROR(QString("Networked tool '%1' response parse failed: %2").arg(toolName, error));
        emit toolCallFailed(toolCallId, toolName, error);
        reply->deleteLater();
        return;
    }

    QJsonObject responseObj = doc.object();

    // Handle MCP JSON-RPC 2.0 response format
    QJsonObject result;
    if (responseObj.contains("result")) {
        // MCP format: {"jsonrpc": "2.0", "id": 1, "result": {...}}
        result = responseObj["result"].toObject();
    } else if (responseObj.contains("error")) {
        // MCP error format: {"jsonrpc": "2.0", "id": 1, "error": {...}}
        QJsonObject error = responseObj["error"].toObject();
        QString errorMsg = error.value("message").toString();
        if (errorMsg.isEmpty()) {
            errorMsg = QString::fromUtf8(QJsonDocument(error).toJson(QJsonDocument::Compact));
        }
        LOG_ERROR(QString("Networked tool '%1' returned error: %2").arg(toolName, errorMsg));
        emit toolCallFailed(toolCallId, toolName, errorMsg);
        reply->deleteLater();
        return;
    } else {
        // Legacy format or plain result
        result = responseObj;
    }

    LOG_INFO(QString("Networked tool '%1' completed successfully").arg(toolName));
    LOG_DEBUG(QString("Tool result: %1")
              .arg(QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))));

    emit toolCallCompleted(toolCallId, toolName, result);
    reply->deleteLater();
}

void MCPHandler::executeSSETool(const QString &toolCallId,
                                const QString &toolName,
                                const QJsonObject &parameters) {
    const MCPTool &tool = m_tools[toolName];

    // Create SSE client if it doesn't exist for this tool
    SSEClient *client = nullptr;
    if (m_sseClients.contains(toolName)) {
        client = m_sseClients[toolName];
        LOG_DEBUG(QString("Reusing existing SSE client for tool: %1").arg(toolName));
    } else {
        client = new SSEClient(this);

        // Connect SSE signals
        connect(client, &SSEClient::eventReceived,
                this, &MCPHandler::handleSSEEvent);
        connect(client, &SSEClient::connected,
                this, &MCPHandler::handleSSEConnected);
        connect(client, &SSEClient::disconnected,
                this, &MCPHandler::handleSSEDisconnected);
        connect(client, &SSEClient::errorOccurred,
                this, &MCPHandler::handleSSEError);

        m_sseClients[toolName] = client;
        LOG_INFO(QString("Created new SSE client for tool: %1").arg(toolName));
    }

    // Store the tool call ID for this SSE request
    m_sseToolCalls[client] = toolCallId + ":" + toolName;

    // Build request URL with parameters as query string
    QString url = tool.networkUrl;
    if (!parameters.isEmpty()) {
        QStringList queryParams;
        for (auto it = parameters.begin(); it != parameters.end(); ++it) {
            QString key = it.key();
            QString value = it.value().toVariant().toString();
            queryParams.append(QString("%1=%2").arg(key, value));
        }
        if (!queryParams.isEmpty()) {
            url += (url.contains('?') ? "&" : "?") + queryParams.join('&');
        }
    }

    LOG_INFO(QString("Connecting SSE stream for tool: %1 to %2").arg(toolName, url));

    // Connect to SSE stream
    client->connectToStream(url);
}

void MCPHandler::handleSSEEvent(const SSEClient::SSEEvent &event) {
    SSEClient *client = qobject_cast<SSEClient*>(sender());
    if (!client) {
        LOG_ERROR("SSE event received from unknown client");
        return;
    }

    // Get tool call info
    QString callInfo = m_sseToolCalls.value(client, "");
    if (callInfo.isEmpty()) {
        LOG_WARNING("SSE event received for unknown tool call");
        return;
    }

    QStringList parts = callInfo.split(":");
    if (parts.size() != 2) {
        LOG_ERROR("Invalid SSE tool call info");
        return;
    }

    QString toolCallId = parts[0];
    QString toolName = parts[1];

    LOG_DEBUG(QString("SSE Event for %1 - Type: %2, ID: %3, Data length: %4")
              .arg(toolName, event.eventType, event.id).arg(event.data.length()));

    // Parse event data as JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(event.data.toUtf8(), &parseError);

    QJsonObject result;
    if (parseError.error == QJsonParseError::NoError) {
        result = doc.object();
    } else {
        // If not JSON, treat as plain text
        result["data"] = event.data;
        result["eventType"] = event.eventType;
        if (!event.id.isEmpty()) {
            result["id"] = event.id;
        }
    }

    // Check if this is a completion event (event type "done" or "complete")
    bool isComplete = (event.eventType == "done" || event.eventType == "complete" || event.eventType == "end");

    if (isComplete) {
        LOG_INFO(QString("SSE tool '%1' completed").arg(toolName));
        emit toolCallCompleted(toolCallId, toolName, result);

        // Clean up this tool call tracking
        m_sseToolCalls.remove(client);

        // Optionally disconnect the client (or keep it alive for reuse)
        // For now, we'll keep it alive for potential reuse
    } else {
        // For streaming events, emit as partial result
        // This could be extended to support progressive results
        LOG_DEBUG(QString("SSE streaming data for %1").arg(toolName));
        emit toolCallCompleted(toolCallId, toolName, result);
    }
}

void MCPHandler::handleSSEConnected(const QString &url) {
    LOG_INFO(QString("SSE connected to: %1").arg(url));
}

void MCPHandler::handleSSEDisconnected() {
    SSEClient *client = qobject_cast<SSEClient*>(sender());
    if (!client) {
        return;
    }

    LOG_INFO(QString("SSE disconnected from: %1").arg(client->getStreamUrl()));

    // If there's a pending tool call, mark it as failed
    if (m_sseToolCalls.contains(client)) {
        QString callInfo = m_sseToolCalls[client];
        QStringList parts = callInfo.split(":");
        if (parts.size() == 2) {
            QString toolCallId = parts[0];
            QString toolName = parts[1];
            emit toolCallFailed(toolCallId, toolName, "SSE connection closed unexpectedly");
        }
        m_sseToolCalls.remove(client);
    }
}

void MCPHandler::handleSSEError(const QString &error) {
    SSEClient *client = qobject_cast<SSEClient*>(sender());
    if (!client) {
        LOG_ERROR(QString("SSE error from unknown client: %1").arg(error));
        return;
    }

    LOG_ERROR(QString("SSE error for %1: %2").arg(client->getStreamUrl(), error));

    // If there's a pending tool call, mark it as failed
    if (m_sseToolCalls.contains(client)) {
        QString callInfo = m_sseToolCalls[client];
        QStringList parts = callInfo.split(":");
        if (parts.size() == 2) {
            QString toolCallId = parts[0];
            QString toolName = parts[1];
            emit toolCallFailed(toolCallId, toolName, error);
        }
        m_sseToolCalls.remove(client);
    }
}

QString MCPHandler::generateToolCallId() const {
    // Generate a unique ID using UUID
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}
