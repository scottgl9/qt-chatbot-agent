/**
 * LLMClient.cpp - LLM backend communication client
 * 
 * Handles communication with LLM backends (Ollama/OpenAI), manages streaming
 * responses via SSE, supports tool calling, and implements retry logic.
 */

#include "LLMClient.h"
#include "Config.h"
#include "Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QTimer>

LLMClient::LLMClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_maxRetries(3)
    , m_retryDelay(1000)
    , m_currentRetryCount(0)
    , m_toolsEnabled(false)
    , m_nativeToolCallEmitted(false)
    , m_toolCallFormat("unknown")
    , m_capabilitiesDetected(false) {

    // Load settings from Config
    m_apiUrl = Config::instance().getApiUrl();
    m_model = Config::instance().getModel();

    // Defer network manager creation until event loop is running
    QTimer::singleShot(0, this, [this]() {
        m_networkManager = new QNetworkAccessManager(this);
        // Connect network manager signals
        connect(m_networkManager, &QNetworkAccessManager::finished,
                this, &LLMClient::handleNetworkReply);
        LOG_DEBUG("LLMClient: QNetworkAccessManager initialized");

        // Query model capabilities after initialization
        queryModelCapabilities();
    });

    LOG_INFO(QString("LLMClient initialized with model: %1, API: %2 (max retries: %3)")
             .arg(m_model, m_apiUrl).arg(m_maxRetries));
}

LLMClient::~LLMClient() {
    // m_networkManager has 'this' as parent, so Qt will auto-delete it
    // No manual deletion needed to avoid double-free
}

void LLMClient::setApiUrl(const QString &url) {
    m_apiUrl = url;
    LOG_DEBUG(QString("API URL set to: %1").arg(url));
}

void LLMClient::setModel(const QString &model) {
    m_model = model;
    LOG_DEBUG(QString("Model set to: %1").arg(model));
}

void LLMClient::sendPrompt(const QString &prompt, const QString &context) {
    if (prompt.isEmpty()) {
        LOG_WARNING("Attempted to send empty prompt");
        emit errorOccurred("Prompt cannot be empty");
        return;
    }

    // Queue request if capabilities not yet detected
    if (!m_capabilitiesDetected) {
        LOG_INFO("Queueing prompt until model capabilities are detected");
        PendingRequest req;
        req.prompt = prompt;
        req.tools = QJsonArray();
        req.context = context;
        req.withTools = false;
        m_pendingRequests.append(req);
        return;
    }

    LOG_INFO(QString("Sending prompt to LLM (length: %1 chars)").arg(prompt.length()));
    LOG_DEBUG(QString("Prompt: %1").arg(prompt.left(100))); // Log first 100 chars

    QString fullPrompt = prompt;
    if (!context.isEmpty()) {
        fullPrompt = QString("Context: %1\n\nPrompt: %2").arg(context, prompt);
        LOG_DEBUG(QString("Added context (length: %1 chars)").arg(context.length()));
    }

    // Reset retry counter for new request
    m_currentRetryCount = 0;
    m_toolsEnabled = false;
    m_currentTools = QJsonArray();

    QString jsonRequest = buildOllamaRequest(fullPrompt, context);
    sendRequest(jsonRequest);
}

void LLMClient::sendPromptWithTools(const QString &prompt, const QJsonArray &tools, const QString &context) {
    if (prompt.isEmpty()) {
        LOG_WARNING("Attempted to send empty prompt");
        emit errorOccurred("Prompt cannot be empty");
        return;
    }

    // Queue request if capabilities not yet detected
    if (!m_capabilitiesDetected) {
        LOG_INFO("Queueing prompt with tools until model capabilities are detected");
        PendingRequest req;
        req.prompt = prompt;
        req.tools = tools;
        req.context = context;
        req.withTools = true;
        m_pendingRequests.append(req);
        return;
    }

    LOG_INFO(QString("Sending prompt with %1 tools to LLM (length: %2 chars)")
             .arg(tools.size()).arg(prompt.length()));
    LOG_DEBUG(QString("Prompt: %1").arg(prompt.left(100)));

    QString fullPrompt = prompt;
    if (!context.isEmpty()) {
        fullPrompt = QString("Context: %1\n\nPrompt: %2").arg(context, prompt);
        LOG_DEBUG(QString("Added context (length: %1 chars)").arg(context.length()));
    }

    // Reset retry counter and enable tools
    m_currentRetryCount = 0;
    m_toolsEnabled = true;
    m_currentTools = tools;
    m_currentPrompt = fullPrompt;

    // Do NOT clear message history - we need to maintain conversation context
    // m_messageHistory = QJsonArray();

    // Choose request format based on detected model capabilities
    QString jsonRequest;
    if (m_toolCallFormat == "native") {
        LOG_INFO("Using NATIVE tool calling format (/api/chat)");
        jsonRequest = buildNativeToolRequest(fullPrompt, tools, context);

        // Save the user message to history AFTER building the request
        // so it's available for tool result processing
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = fullPrompt;
        m_messageHistory.append(userMsg);
        LOG_DEBUG("Saved user message to message history for conversation continuity");
    } else {
        LOG_INFO("Using PROMPT-BASED tool calling format (/api/generate)");
        jsonRequest = buildOllamaRequestWithTools(fullPrompt, tools, context);
    }

    sendRequest(jsonRequest);
}

void LLMClient::sendToolResults(const QString &originalPrompt, const QJsonArray &toolResults) {
    Q_UNUSED(originalPrompt);
    LOG_INFO(QString("Processing tool results (%1 results)").arg(toolResults.size()));

    // Check if any tools are complex (HTTP/networked) that need LLM processing
    bool hasComplexTools = false;
    QStringList simpleTools = {"datetime", "calculator"};

    for (const QJsonValue &result : toolResults) {
        QJsonObject obj = result.toObject();
        QString toolName = obj["tool_name"].toString();
        if (!simpleTools.contains(toolName)) {
            hasComplexTools = true;
            break;
        }
    }

    // If we have complex tools and are using native tool calling, send back to LLM
    if (hasComplexTools && m_toolCallFormat == "native") {
        LOG_INFO("Complex tool results detected, sending back to LLM for processing");

        // Build message array with tool results for native format
        QJsonArray messages;

        // Add system message if configured (must be first)
        QString systemPrompt = Config::instance().getSystemPrompt();
        if (!systemPrompt.isEmpty()) {
            QJsonObject systemMsg;
            systemMsg["role"] = "system";
            systemMsg["content"] = systemPrompt;
            messages.append(systemMsg);
            LOG_DEBUG(QString("Including system prompt in tool result processing (length: %1 chars)").arg(systemPrompt.length()));
        }

        // Prepare tool result content first
        QString toolResultContent = "Here are the tool call results. Please provide a clear, natural language summary of this information:\n\n";
        for (const QJsonValue &result : toolResults) {
            QJsonObject obj = result.toObject();
            QString toolName = obj["tool_name"].toString();
            QJsonObject resultData = obj["result"].toObject();

            toolResultContent += QString("Tool: %1\n").arg(toolName);
            toolResultContent += QString("Result: %1\n\n").arg(QString::fromUtf8(QJsonDocument(resultData).toJson(QJsonDocument::Compact)));
        }

        // Add pruned message history (context-aware)
        // Note: m_messageHistory should already contain the user message and assistant's tool call
        QJsonArray prunedHistory = pruneMessageHistoryForContext(systemPrompt, toolResultContent);
        for (const QJsonValue &msg : prunedHistory) {
            messages.append(msg);
        }

        // Add tool result as user message (Ollama may not support "tool" role)
        QJsonObject toolResultMsg;
        toolResultMsg["role"] = "user";
        toolResultMsg["content"] = toolResultContent;
        messages.append(toolResultMsg);

        // Save tool result message to history for conversation continuity
        m_messageHistory.append(toolResultMsg);
        LOG_DEBUG("Saved tool result message to message history for conversation continuity");

        // Build request to continue the conversation
        QJsonObject json;
        json["model"] = m_model;
        json["stream"] = true;
        json["messages"] = messages;

        // Add options
        QJsonObject options;
        if (Config::instance().getOverrideTemperature()) {
            options["temperature"] = Config::instance().getTemperature();
        }
        if (Config::instance().getOverrideTopP()) {
            options["top_p"] = Config::instance().getTopP();
        }
        if (Config::instance().getOverrideTopK()) {
            options["top_k"] = Config::instance().getTopK();
        }
        if (Config::instance().getOverrideContextWindowSize()) {
            options["num_ctx"] = Config::instance().getContextWindowSize();
        }
        if (!options.isEmpty()) {
            json["options"] = options;
        }
        if (Config::instance().getOverrideMaxTokens()) {
            json["num_predict"] = Config::instance().getMaxTokens();
        }

        QString jsonRequest = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
        LOG_DEBUG("Sending tool results back to LLM for natural language response");

        // Construct /api/chat endpoint URL
        QUrl baseUrl(m_apiUrl);
        QString chatEndpoint = QString("%1://%2").arg(baseUrl.scheme(), baseUrl.host());
        if (baseUrl.port() > 0) {
            chatEndpoint += QString(":%1").arg(baseUrl.port());
        }
        chatEndpoint += "/api/chat";

        // Send directly to /api/chat endpoint
        if (!m_networkManager) {
            QString error = "Network manager not initialized";
            LOG_ERROR(error);
            emit errorOccurred(error);
            return;
        }

        QUrl url(chatEndpoint);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        LOG_DEBUG(QString("Sending tool results to /api/chat: %1").arg(chatEndpoint));

        // Clear buffers for new request
        m_streamBuffer.clear();
        m_fullResponse.clear();
        m_nativeToolCallEmitted = false;

        m_currentReply = m_networkManager->post(request, jsonRequest.toUtf8());

        // Connect streaming signals
        connect(m_currentReply, &QNetworkReply::readyRead,
                this, &LLMClient::handleStreamingData, Qt::UniqueConnection);
        connect(m_currentReply, &QNetworkReply::finished,
                this, &LLMClient::handleStreamingFinished, Qt::UniqueConnection);

        // Set timeout
        QTimer::singleShot(90000, m_currentReply, [this]() {
            if (m_currentReply && m_currentReply->isRunning()) {
                LOG_ERROR("Request timed out after 90 seconds");
                m_currentReply->abort();
                emit errorOccurred("Request timed out");
            }
        });

        return;
    }

    // For simple tools or non-native format, use direct formatting
    QString naturalResponse;

    // Extract the actual data from tool results
    for (const QJsonValue &result : toolResults) {
        QJsonObject obj = result.toObject();
        QString toolName = obj["tool_name"].toString();
        QJsonObject resultData = obj["result"].toObject();

        // For datetime tool, format nicely
        if (toolName == "datetime") {
            // Handle different datetime formats
            if (resultData.contains("datetime")) {
                // ISO format
                QString datetime = resultData["datetime"].toString();
                naturalResponse = QString("The current date and time is %1.").arg(datetime);
            } else if (resultData.contains("timestamp")) {
                // Timestamp format
                qint64 timestamp = resultData["timestamp"].toVariant().toLongLong();
                naturalResponse = QString("The current timestamp is %1.").arg(timestamp);
            } else {
                // Short or long format
                QString date = resultData["date"].toString();
                QString time = resultData["time"].toString();
                QString timezone = resultData["timezone"].toString();

                if (!timezone.isEmpty()) {
                    naturalResponse = QString("It's currently %1 on %2 (%3).").arg(time, date, timezone);
                } else {
                    naturalResponse = QString("It's currently %1 on %2.").arg(time, date);
                }
            }
        }
        // For calculator tool
        else if (toolName == "calculator") {
            QJsonValue resultVal = resultData["result"];
            QString result_str = resultVal.isDouble() || resultVal.isString()
                ? resultVal.toVariant().toString()
                : QString::number(resultVal.toInt());
            naturalResponse = QString("The answer is %1.").arg(result_str);
        }
        // Generic formatting for other tools (fallback)
        else {
            QString formattedResult = QString::fromUtf8(QJsonDocument(resultData).toJson(QJsonDocument::Indented));
            naturalResponse = QString("Tool result:\n%1").arg(formattedResult);
        }
    }

    LOG_INFO(QString("Formatted natural response: %1").arg(naturalResponse.left(100)));

    // Emit the response directly
    emit responseReceived(naturalResponse);
}

QString LLMClient::buildOllamaRequest(const QString &prompt, const QString &context) {
    Q_UNUSED(context);

    QJsonObject json;
    json["model"] = m_model;
    json["prompt"] = prompt;
    json["stream"] = true; // Enable streaming for real-time token display

    // Add system prompt from Config
    QString systemPrompt = Config::instance().getSystemPrompt();
    if (!systemPrompt.isEmpty()) {
        json["system"] = systemPrompt;
        LOG_DEBUG(QString("Including system prompt (length: %1 chars)").arg(systemPrompt.length()));
    }

    // Add LLM configuration parameters from Config (only if override is enabled)
    QJsonObject options;
    if (Config::instance().getOverrideTemperature()) {
        options["temperature"] = Config::instance().getTemperature();
    }
    if (Config::instance().getOverrideTopP()) {
        options["top_p"] = Config::instance().getTopP();
    }
    if (Config::instance().getOverrideTopK()) {
        options["top_k"] = Config::instance().getTopK();
    }
    if (Config::instance().getOverrideContextWindowSize()) {
        options["num_ctx"] = Config::instance().getContextWindowSize();
    }

    if (!options.isEmpty()) {
        json["options"] = options;
    }

    // Set max tokens (num_predict in Ollama) only if override is enabled
    if (Config::instance().getOverrideMaxTokens()) {
        json["num_predict"] = Config::instance().getMaxTokens();
    }

    QJsonDocument doc(json);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    LOG_DEBUG(QString("Request options - Temp: %1, TopP: %2, TopK: %3, CtxSize: %4, MaxTokens: %5")
              .arg(Config::instance().getTemperature())
              .arg(Config::instance().getTopP())
              .arg(Config::instance().getTopK())
              .arg(Config::instance().getContextWindowSize())
              .arg(Config::instance().getMaxTokens()));

    return jsonString;
}

QString LLMClient::buildOllamaRequestWithTools(const QString &prompt, const QJsonArray &tools, const QString &context) {
    Q_UNUSED(context);

    // Build enhanced system prompt with tool instructions
    QString baseSystemPrompt = Config::instance().getSystemPrompt();
    QString toolInstructions = "\n\nAVAILABLE TOOLS:\n";
    toolInstructions += "You have access to the following tools to help answer questions:\n\n";

    for (const QJsonValue &toolVal : tools) {
        QJsonObject tool = toolVal.toObject();
        QString toolName = tool["name"].toString();
        QString toolDesc = tool["description"].toString();
        QJsonObject params = tool["parameters"].toObject();

        toolInstructions += QString("Tool: %1\n").arg(toolName);
        toolInstructions += QString("Description: %1\n").arg(toolDesc);
        toolInstructions += QString("Parameters: %1\n\n")
            .arg(QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact)));
    }

    toolInstructions += "\nTo use a tool, respond with JSON in this format:\n";
    toolInstructions += "{\"tool_call\": {\"name\": \"tool_name\", \"parameters\": {}}}\n\n";
    toolInstructions += "Or just: {\"name\": \"tool_name\", \"parameters\": {}}\n\n";
    toolInstructions += "If you don't need a tool, respond normally.\n\n";

    QString enhancedSystemPrompt = baseSystemPrompt + toolInstructions;

    QJsonObject json;
    json["model"] = m_model;
    json["prompt"] = prompt;
    json["system"] = enhancedSystemPrompt;
    json["stream"] = true;

    // Add LLM configuration parameters from Config (only if override is enabled)
    QJsonObject options;
    if (Config::instance().getOverrideTemperature()) {
        options["temperature"] = Config::instance().getTemperature();
    }
    if (Config::instance().getOverrideTopP()) {
        options["top_p"] = Config::instance().getTopP();
    }
    if (Config::instance().getOverrideTopK()) {
        options["top_k"] = Config::instance().getTopK();
    }
    if (Config::instance().getOverrideContextWindowSize()) {
        options["num_ctx"] = Config::instance().getContextWindowSize();
    }

    if (!options.isEmpty()) {
        json["options"] = options;
    }

    if (Config::instance().getOverrideMaxTokens()) {
        json["num_predict"] = Config::instance().getMaxTokens();
    }

    QJsonDocument doc(json);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    LOG_DEBUG(QString("Tool-enabled request with %1 tools (system prompt: %2 chars)")
              .arg(tools.size()).arg(enhancedSystemPrompt.length()));

    return jsonString;
}

QString LLMClient::buildNativeToolRequest(const QString &prompt, const QJsonArray &tools, const QString &context) {
    Q_UNUSED(context);

    QJsonObject json;
    json["model"] = m_model;
    json["stream"] = true;

    // Build messages array (chat format)
    QJsonArray messages;

    // Add system message if configured
    QString systemPrompt = Config::instance().getSystemPrompt();
    if (!systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = systemPrompt;
        messages.append(systemMsg);
        LOG_DEBUG(QString("Including system prompt (length: %1 chars)").arg(systemPrompt.length()));
    }

    // Add pruned message history (context-aware)
    QJsonArray prunedHistory = pruneMessageHistoryForContext(systemPrompt, prompt);
    for (const QJsonValue &msg : prunedHistory) {
        messages.append(msg);
    }

    // Add current user message
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);

    json["messages"] = messages;

    // Convert tools to native (OpenAI) format if needed
    if (!tools.isEmpty()) {
        QJsonArray nativeTools;

        for (const QJsonValue &toolVal : tools) {
            QJsonObject tool = toolVal.toObject();

            // Check if already in OpenAI format (has "type" and "function" fields)
            if (tool.contains("type") && tool.contains("function")) {
                nativeTools.append(tool);
            } else {
                // Convert from MCP format to OpenAI format
                QJsonObject functionDef;
                functionDef["name"] = tool["name"];
                functionDef["description"] = tool["description"];

                // Convert parameters to JSON Schema format
                QJsonObject parameters = tool["parameters"].toObject();
                if (!parameters.contains("type")) {
                    // Convert simple format to JSON Schema
                    QJsonObject schema;
                    schema["type"] = "object";
                    QJsonObject properties;

                    for (const QString &key : parameters.keys()) {
                        QJsonObject propSchema;
                        QString paramDesc = parameters[key].toString();

                        // Parse "type: description" format
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
                    schema["required"] = QJsonArray();
                    functionDef["parameters"] = schema;
                } else {
                    // Already in JSON Schema format
                    functionDef["parameters"] = parameters;
                }

                // Wrap in OpenAI format
                QJsonObject toolObj;
                toolObj["type"] = "function";
                toolObj["function"] = functionDef;

                nativeTools.append(toolObj);
            }
        }

        json["tools"] = nativeTools;
        LOG_DEBUG(QString("Including %1 tools in OpenAI-compatible format").arg(nativeTools.size()));
    }

    // Add LLM configuration parameters from Config (only if override is enabled)
    QJsonObject options;
    if (Config::instance().getOverrideTemperature()) {
        options["temperature"] = Config::instance().getTemperature();
    }
    if (Config::instance().getOverrideTopP()) {
        options["top_p"] = Config::instance().getTopP();
    }
    if (Config::instance().getOverrideTopK()) {
        options["top_k"] = Config::instance().getTopK();
    }
    if (Config::instance().getOverrideContextWindowSize()) {
        options["num_ctx"] = Config::instance().getContextWindowSize();
    }

    if (!options.isEmpty()) {
        json["options"] = options;
    }

    if (Config::instance().getOverrideMaxTokens()) {
        json["num_predict"] = Config::instance().getMaxTokens();
    }

    QJsonDocument doc(json);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    LOG_DEBUG(QString("Native request - Messages: %1, Tools: %2")
              .arg(messages.size()).arg(tools.size()));

    return jsonString;
}


void LLMClient::sendRequest(const QString &jsonData) {
    // Ensure network manager is initialized
    if (!m_networkManager) {
        QString error = "Network manager not initialized yet. Please wait for initialization.";
        LOG_ERROR(error);
        emit errorOccurred(error);
        return;
    }

    // Determine the endpoint based on tool format
    QString apiUrl = m_apiUrl;
    if (m_toolsEnabled && m_toolCallFormat == "native") {
        // For native tool calling, use /api/chat endpoint
        QUrl baseUrl(m_apiUrl);
        QString base = QString("%1://%2").arg(baseUrl.scheme(), baseUrl.host());
        if (baseUrl.port() > 0) {
            base += QString(":%1").arg(baseUrl.port());
        }
        apiUrl = base + "/api/chat";
        LOG_DEBUG(QString("Using /api/chat endpoint for native tool calling: %1").arg(apiUrl));
    }

    QUrl url(apiUrl);
    if (!url.isValid()) {
        QString error = QString("Invalid API URL: %1").arg(apiUrl);
        LOG_ERROR(error);
        emit errorOccurred(error);
        return;
    }

    // Store request data for potential retry
    m_lastRequestData = jsonData;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    LOG_DEBUG(QString("Sending POST request to: %1 (retry %2/%3)")
              .arg(m_apiUrl).arg(m_currentRetryCount).arg(m_maxRetries));
    LOG_DEBUG(QString("Request body: %1").arg(jsonData));

    // Clear buffers and flags for new request
    m_streamBuffer.clear();
    m_fullResponse.clear();
    m_nativeToolCallEmitted = false;

    m_currentReply = m_networkManager->post(request, jsonData.toUtf8());

    LOG_DEBUG("Network request created, connecting streaming signals");

    // Connect streaming signals
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, &LLMClient::handleStreamingData, Qt::UniqueConnection);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LLMClient::handleStreamingFinished, Qt::UniqueConnection);

    LOG_DEBUG("Streaming signals connected, waiting for response...");

    // Set a timeout (90 seconds - increased for large tool sets)
    QTimer::singleShot(90000, m_currentReply, [this]() {
        if (m_currentReply && m_currentReply->isRunning()) {
            LOG_ERROR("Request timed out after 90 seconds");
            m_currentReply->abort();
            emit errorOccurred("Request timed out");
        }
    });
}

void LLMClient::handleNetworkReply(QNetworkReply *reply) {
    // Skip if this is a /api/show request (handled by dedicated handler)
    QString url = reply->url().toString();
    if (url.contains("/api/show")) {
        LOG_DEBUG("Skipping general handler for /api/show request");
        return;  // Don't delete - the dedicated handler will do it
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Network error: %1").arg(reply->errorString());
        LOG_ERROR(errorMsg);
        emit errorOccurred(errorMsg);
        return;
    }

    QByteArray responseData = reply->readAll();
    LOG_DEBUG(QString("Received response (size: %1 bytes)").arg(responseData.size()));

    // Skip parsing if response is empty (common with streaming)
    if (responseData.isEmpty() || responseData.trimmed().isEmpty()) {
        LOG_DEBUG("Empty response received (likely already consumed by streaming handler)");
        return;
    }

    // Parse JSON response
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("JSON parse error: %1").arg(parseError.errorString());
        LOG_ERROR(errorMsg);
        emit errorOccurred(errorMsg);
        return;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Response is not a JSON object");
        emit errorOccurred("Invalid response format");
        return;
    }

    QJsonObject jsonObj = doc.object();

    // Check for error in response
    if (jsonObj.contains("error")) {
        QString errorMsg = jsonObj["error"].toString();
        LOG_ERROR(QString("LLM API error: %1").arg(errorMsg));
        emit errorOccurred(errorMsg);
        return;
    }

    // Extract response text (Ollama format)
    QString responseText;
    if (jsonObj.contains("response")) {
        responseText = jsonObj["response"].toString();
    } else if (jsonObj.contains("choices")) {
        // OpenAI format
        QJsonArray choices = jsonObj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices[0].toObject();
            if (firstChoice.contains("message")) {
                QJsonObject message = firstChoice["message"].toObject();
                responseText = message["content"].toString();
            } else if (firstChoice.contains("text")) {
                responseText = firstChoice["text"].toString();
            }
        }
    }

    if (responseText.isEmpty()) {
        LOG_WARNING("Received empty response from LLM");
        emit errorOccurred("Empty response from LLM");
        return;
    }

    LOG_INFO(QString("Received LLM response (length: %1 chars)").arg(responseText.length()));
    LOG_DEBUG(QString("Response: %1").arg(responseText.left(100))); // Log first 100 chars

    emit responseReceived(responseText);
}

void LLMClient::handleStreamingData() {
    if (!m_currentReply) {
        LOG_WARNING("handleStreamingData called but m_currentReply is null");
        return;
    }

    // Read all available data
    QByteArray newData = m_currentReply->readAll();
    LOG_DEBUG(QString("handleStreamingData: Read %1 bytes").arg(newData.size()));

    if (newData.isEmpty()) {
        LOG_DEBUG("handleStreamingData: No data to read");
        return;
    }

    m_streamBuffer.append(QString::fromUtf8(newData));
    LOG_DEBUG(QString("Stream buffer now contains %1 chars").arg(m_streamBuffer.length()));

    // Process complete lines (Ollama sends newline-delimited JSON)
    QStringList lines = m_streamBuffer.split('\n');

    // Keep the last incomplete line in the buffer
    m_streamBuffer = lines.takeLast();

    LOG_DEBUG(QString("Processing %1 complete lines").arg(lines.size()));

    // Process each complete line
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty()) {
            processStreamingChunk(line.trimmed());
        }
    }
}

void LLMClient::processStreamingChunk(const QString &line) {
    // Parse the JSON chunk
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARNING(QString("Failed to parse streaming chunk: %1 - Line: %2").arg(parseError.errorString(), line.left(100)));
        return;
    }

    if (!doc.isObject()) {
        return;
    }

    QJsonObject obj = doc.object();

    // Check for errors
    if (obj.contains("error")) {
        QString errorMsg = obj["error"].toString();
        LOG_ERROR(QString("Streaming error: %1").arg(errorMsg));
        emit errorOccurred(errorMsg);
        return;
    }

    // Log the entire chunk for debugging
    if (m_fullResponse.isEmpty()) {
        LOG_DEBUG(QString("First chunk received: %1").arg(line.left(200)));
    }

    // Handle native chat format (has "message" field)
    if (obj.contains("message")) {
        QJsonObject message = obj["message"].toObject();

        // Check for tool calls in the message
        if (message.contains("tool_calls")) {
            // Process native tool calls
            bool toolCallDetected = processNativeToolCalls(message);
            if (toolCallDetected) {
                LOG_DEBUG("Native tool call detected and handled");
                m_nativeToolCallEmitted = true;
                return;  // Don't process as regular content
            }
        }

        // Extract content from message
        if (message.contains("content")) {
            QString token = message["content"].toString();
            if (!token.isEmpty()) {
                m_fullResponse.append(token);
                emit tokenReceived(token);
                LOG_DEBUG(QString("Native message token: %1").arg(token));
            }
        }
    }
    // Extract the response token (Ollama /api/generate format)
    else if (obj.contains("response")) {
        QString token = obj["response"].toString();
        if (!token.isEmpty()) {
            m_fullResponse.append(token);
            emit tokenReceived(token);
            LOG_DEBUG(QString("Token received: %1").arg(token));
        } else {
            // Empty response token - might be the start or end marker
            LOG_DEBUG("Received empty response token in chunk");
        }
    } else {
        // No "response" or "message" field - log what we got
        LOG_DEBUG(QString("Chunk has no 'response' or 'message' field. Keys: %1").arg(obj.keys().join(", ")));
    }

    // Check if streaming is done
    if (obj.contains("done") && obj["done"].toBool()) {
        LOG_INFO(QString("Streaming complete. Total response length: %1 chars")
                 .arg(m_fullResponse.length()));

        // Log additional info from the done message
        if (obj.contains("total_duration")) {
            LOG_DEBUG(QString("Total duration: %1 ms").arg(obj["total_duration"].toDouble() / 1000000.0));
        }
        if (obj.contains("prompt_eval_count")) {
            LOG_DEBUG(QString("Prompt tokens: %1").arg(obj["prompt_eval_count"].toInt()));
        }
        if (obj.contains("eval_count")) {
            LOG_DEBUG(QString("Response tokens: %1").arg(obj["eval_count"].toInt()));
        }
    }
}

void LLMClient::handleStreamingFinished() {
    if (!m_currentReply) {
        LOG_WARNING("handleStreamingFinished called but m_currentReply is null");
        return;
    }

    LOG_DEBUG(QString("Streaming finished. m_fullResponse length: %1, m_streamBuffer length: %2")
              .arg(m_fullResponse.length()).arg(m_streamBuffer.length()));

    // Check for network errors
    if (m_currentReply->error() != QNetworkReply::NoError) {
        QNetworkReply::NetworkError error = m_currentReply->error();
        QString errorMsg = QString("Network error: %1").arg(m_currentReply->errorString());

        // Clean up the current reply
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        // Check if we should retry
        if (shouldRetry(error) && m_currentRetryCount < m_maxRetries) {
            m_currentRetryCount++;
            int delay = m_retryDelay * (1 << (m_currentRetryCount - 1)); // Exponential backoff
            LOG_WARNING(QString("%1 - Retrying in %2ms (attempt %3/%4)")
                       .arg(errorMsg).arg(delay).arg(m_currentRetryCount).arg(m_maxRetries));
            emit retryAttempt(m_currentRetryCount, m_maxRetries);

            // Schedule retry with exponential backoff
            QTimer::singleShot(delay, this, &LLMClient::retryRequest);
            return;
        }

        // Max retries reached or non-retryable error
        LOG_ERROR(QString("%1 - Max retries reached or non-retryable error").arg(errorMsg));
        emit errorOccurred(errorMsg);
        return;
    }

    // Process any remaining buffered data
    if (!m_streamBuffer.trimmed().isEmpty()) {
        processStreamingChunk(m_streamBuffer.trimmed());
    }

    // Emit the complete response
    if (!m_fullResponse.isEmpty() || m_nativeToolCallEmitted) {
        LOG_INFO(QString("Streaming finished. Full response: %1 chars").arg(m_fullResponse.length()));
        // Reset retry counter on success
        m_currentRetryCount = 0;

        // Skip prompt-based tool call processing if native tool calls were already emitted
        if (m_nativeToolCallEmitted) {
            LOG_DEBUG("Native tool call already handled, skipping prompt-based processing");
        } else {
            // Check for tool calls before emitting response (prompt-based format only)
            bool toolCallDetected = processToolCalls(m_fullResponse);

            // Only emit the raw response if no tool call was detected
            // (tool calls emit their own formatted responses)
            if (!toolCallDetected) {
                LOG_DEBUG("No tool call detected, emitting raw response");

                // Save assistant response to message history for conversation continuity
                // (only for native format, which maintains history)
                if (m_toolCallFormat == "native" && !m_fullResponse.isEmpty()) {
                    QJsonObject assistantMsg;
                    assistantMsg["role"] = "assistant";
                    assistantMsg["content"] = m_fullResponse;
                    m_messageHistory.append(assistantMsg);
                    LOG_DEBUG("Saved assistant response to message history for conversation continuity");
                }

                emit responseReceived(m_fullResponse);
            } else {
                LOG_DEBUG("Tool call detected and handled, not emitting raw response");
            }
        }
    } else {
        LOG_WARNING("Streaming finished but no response received");
        emit errorOccurred("No response received from LLM");
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

bool LLMClient::shouldRetry(QNetworkReply::NetworkError error) {
    // Retry on network/connection errors, but not on protocol/content errors
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::OperationCanceledError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::NetworkSessionFailedError:
        case QNetworkReply::BackgroundRequestNotAllowedError:
        case QNetworkReply::UnknownNetworkError:
        case QNetworkReply::UnknownProxyError:
        case QNetworkReply::UnknownContentError:
            return true;
        default:
            return false;
    }
}

bool LLMClient::processToolCalls(const QString &response) {
    if (!m_toolsEnabled) {
        LOG_DEBUG("Tool calling disabled, skipping tool call processing");
        return false;
    }

    LOG_DEBUG(QString("Processing response for tool calls (length: %1 chars)").arg(response.length()));
    LOG_DEBUG(QString("Response content: %1").arg(response.left(200))); // Log first 200 chars

    // Try to detect tool calls in the response
    // Look for JSON format: {"tool_call": {"name": "...", "parameters": {...}}}
    int toolCallStart = response.indexOf("{\"tool_call\":");
    if (toolCallStart == -1) {
        toolCallStart = response.indexOf("{ \"tool_call\":");
    }

    // If no proper tool call found, try heuristic matching for partial patterns
    if (toolCallStart == -1) {
        LOG_DEBUG("No tool call pattern found, trying heuristic detection");

        // Try to parse response as pure JSON (malformed tool call)
        QString trimmed = response.trimmed();
        if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();

                // Check if it has "name" and "parameters" fields (missing tool_call wrapper)
                if (obj.contains("name") && obj.contains("parameters")) {
                    QString toolName = obj["name"].toString();
                    QJsonObject params = obj["parameters"].toObject();

                    if (!toolName.isEmpty()) {
                        QString callId = QString("call_%1").arg(QTime::currentTime().msecsSinceStartOfDay());
                        LOG_WARNING(QString("Heuristic match: Detected tool call without wrapper for '%1'").arg(toolName));
                        emit toolCallRequested(toolName, params, callId);
                        return true; // Tool call detected and emitted
                    }
                }

                // Otherwise, try to match parameters to a known tool
                QStringList responseKeys = obj.keys();

                for (const QJsonValue &toolVal : m_currentTools) {
                    QJsonObject tool = toolVal.toObject();
                    QString toolName = tool["name"].toString();
                    QJsonObject toolParams = tool["parameters"].toObject();

                    // Check if the response parameters match this tool's parameters
                    QStringList toolKeys = toolParams.keys();

                    int matchCount = 0;
                    for (const QString &key : responseKeys) {
                        if (toolKeys.contains(key)) {
                            matchCount++;
                        }
                    }

                    // If majority of parameters match, assume this is the intended tool
                    if (matchCount > 0 && matchCount >= responseKeys.size() * 0.7) {
                        QString callId = QString("call_%1").arg(QTime::currentTime().msecsSinceStartOfDay());
                        LOG_WARNING(QString("Heuristic match: Detected malformed tool call for '%1' (matched %2/%3 params)")
                                   .arg(toolName).arg(matchCount).arg(responseKeys.size()));
                        emit toolCallRequested(toolName, obj, callId);
                        return true; // Tool call detected and emitted
                    }
                }

                LOG_DEBUG("Could not match JSON parameters to any known tool");
            }
        }

        return false; // No tool call detected
    }

    if (toolCallStart != -1) {
        LOG_DEBUG(QString("Found potential tool call at position %1").arg(toolCallStart));
        // Extract the JSON portion
        QString jsonPortion = response.mid(toolCallStart);
        int braceCount = 0;
        int endPos = -1;

        for (int i = 0; i < jsonPortion.length(); ++i) {
            if (jsonPortion[i] == '{') braceCount++;
            else if (jsonPortion[i] == '}') {
                braceCount--;
                if (braceCount == 0) {
                    endPos = i + 1;
                    break;
                }
            }
        }

        if (endPos != -1) {
            QString jsonStr = jsonPortion.left(endPos);
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("tool_call")) {
                    QJsonObject toolCall = obj["tool_call"].toObject();
                    QString toolName = toolCall["name"].toString();
                    QJsonObject parameters = toolCall["parameters"].toObject();

                    if (!toolName.isEmpty()) {
                        QString callId = QString("call_%1").arg(QTime::currentTime().msecsSinceStartOfDay());
                        LOG_INFO(QString("Tool call detected: %1 (ID: %2)").arg(toolName, callId));
                        emit toolCallRequested(toolName, parameters, callId);
                        return true; // Tool call detected and emitted
                    }
                }
            }
        }
    }

    return false; // No tool call detected
}

bool LLMClient::processNativeToolCalls(const QJsonObject &message) {
    if (!m_toolsEnabled) {
        LOG_DEBUG("Tool calling disabled for native format");
        return false;
    }

    if (!message.contains("tool_calls")) {
        return false;
    }

    QJsonArray toolCalls = message["tool_calls"].toArray();
    if (toolCalls.isEmpty()) {
        LOG_DEBUG("Native format: tool_calls array is empty");
        return false;
    }

    LOG_INFO(QString("Processing %1 native tool calls").arg(toolCalls.size()));

    // Save the assistant's message with tool calls to message history
    QJsonObject assistantMsg;
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = message.value("content").toString("");  // May be empty when making tool calls
    assistantMsg["tool_calls"] = toolCalls;
    m_messageHistory.append(assistantMsg);
    LOG_DEBUG("Saved assistant message with tool calls to message history");

    for (const QJsonValue &toolCallVal : toolCalls) {
        QJsonObject toolCall = toolCallVal.toObject();

        // Native format: {"id": "...", "type": "function", "function": {"name": "...", "arguments": "..."}}
        if (toolCall.contains("function")) {
            QJsonObject function = toolCall["function"].toObject();
            QString toolName = function["name"].toString();

            // Arguments might be a string (JSON) or an object
            QJsonObject arguments;
            if (function.contains("arguments")) {
                QJsonValue argsVal = function["arguments"];
                if (argsVal.isString()) {
                    // Parse string as JSON
                    QString argsStr = argsVal.toString();
                    QJsonParseError parseError;
                    QJsonDocument argsDoc = QJsonDocument::fromJson(argsStr.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && argsDoc.isObject()) {
                        arguments = argsDoc.object();
                    } else {
                        LOG_WARNING(QString("Failed to parse native tool call arguments: %1").arg(parseError.errorString()));
                    }
                } else if (argsVal.isObject()) {
                    arguments = argsVal.toObject();
                }
            }

            if (!toolName.isEmpty()) {
                // Use the tool call ID from the response if available, otherwise generate one
                QString callId = toolCall.value("id").toString();
                if (callId.isEmpty()) {
                    callId = QString("call_%1").arg(QTime::currentTime().msecsSinceStartOfDay());
                }

                LOG_INFO(QString("Native tool call: %1 (ID: %2)").arg(toolName, callId));
                LOG_DEBUG(QString("Tool arguments: %1")
                         .arg(QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))));

                emit toolCallRequested(toolName, arguments, callId);
            } else {
                LOG_WARNING("Native tool call has empty function name");
            }
        } else {
            LOG_WARNING(QString("Native tool call missing 'function' field. Keys: %1")
                       .arg(toolCall.keys().join(", ")));
        }
    }

    return true;  // Tool calls were processed
}

void LLMClient::retryRequest() {
    if (m_lastRequestData.isEmpty()) {
        LOG_ERROR("Cannot retry: no previous request data");
        emit errorOccurred("Retry failed: no request data");
        return;
    }

    LOG_INFO(QString("Retrying request (attempt %1/%2)")
             .arg(m_currentRetryCount).arg(m_maxRetries));
    sendRequest(m_lastRequestData);
}

void LLMClient::queryModelCapabilities() {
    if (!m_networkManager) {
        LOG_WARNING("Cannot query model capabilities: network manager not initialized");
        return;
    }

    // Parse the base URL from the API URL (remove /api/generate or /api/chat)
    QUrl apiUrl(m_apiUrl);
    if (!apiUrl.isValid()) {
        LOG_ERROR(QString("Invalid API URL for model capabilities query: %1").arg(m_apiUrl));
        return;
    }

    // Construct the /api/show endpoint
    QString baseUrl = QString("%1://%2").arg(apiUrl.scheme(), apiUrl.host());
    if (apiUrl.port() > 0) {
        baseUrl += QString(":%1").arg(apiUrl.port());
    }
    QString showUrl = baseUrl + "/api/show";

    LOG_INFO(QString("Querying model capabilities from: %1 for model: %2").arg(showUrl, m_model));

    // Build request body
    QJsonObject requestBody;
    requestBody["name"] = m_model;

    QJsonDocument doc(requestBody);
    QString jsonData = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // Create request
    QUrl requestUrl(showUrl);
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Send POST request
    QNetworkReply *reply = m_networkManager->post(request, jsonData.toUtf8());

    // Connect to dedicated handler (not the general handler)
    connect(reply, &QNetworkReply::finished, this, &LLMClient::handleModelInfoReply);

    LOG_DEBUG(QString("Model capabilities request sent: %1").arg(jsonData));
}

void LLMClient::handleModelInfoReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        LOG_ERROR("handleModelInfoReply: Invalid sender");
        return;
    }

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARNING(QString("Failed to query model capabilities: %1").arg(reply->errorString()));
        m_toolCallFormat = "unknown";
        return;
    }

    QByteArray responseData = reply->readAll();
    LOG_DEBUG(QString("Model info response received (size: %1 bytes)").arg(responseData.size()));

    // Parse JSON response
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("Failed to parse model info: %1").arg(parseError.errorString()));
        m_toolCallFormat = "unknown";
        return;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Model info response is not a JSON object");
        m_toolCallFormat = "unknown";
        return;
    }

    m_modelInfo = doc.object();
    LOG_INFO("Model info received successfully");

    // Log some key information
    if (m_modelInfo.contains("modelfile")) {
        QString modelfile = m_modelInfo["modelfile"].toString();
        LOG_DEBUG(QString("Modelfile: %1").arg(modelfile.left(200))); // First 200 chars
    }

    if (m_modelInfo.contains("parameters")) {
        QString params = m_modelInfo["parameters"].toString();
        LOG_DEBUG(QString("Parameters: %1").arg(params));
    }

    if (m_modelInfo.contains("template")) {
        QString template_str = m_modelInfo["template"].toString();
        LOG_DEBUG(QString("Template: %1").arg(template_str.left(200))); // First 200 chars
    }

    // Detect tool calling format
    // Check for native tool calling support indicators
    QString modelfile = m_modelInfo.value("modelfile").toString().toLower();
    QString template_str = m_modelInfo.value("template").toString().toLower();
    QString details = QString::fromUtf8(QJsonDocument(m_modelInfo.value("details").toObject()).toJson());

    // Look for indicators of native tool calling support
    bool hasToolSupport = false;

    // Check for "tools" or "function" keywords in modelfile or template
    if (modelfile.contains("tool") || modelfile.contains("function_call") ||
        template_str.contains("tool") || template_str.contains("function")) {
        hasToolSupport = true;
    }

    // Check model details for tool-related capabilities
    if (details.contains("tool") || details.contains("function")) {
        hasToolSupport = true;
    }

    if (hasToolSupport) {
        m_toolCallFormat = "native";
        LOG_INFO("Model supports NATIVE tool calling format");
    } else {
        m_toolCallFormat = "prompt";
        LOG_INFO("Model uses PROMPT-BASED tool calling format (system prompt injection)");
    }

    // Mark capabilities as detected
    m_capabilitiesDetected = true;

    // Emit signal with detected capabilities
    emit modelCapabilitiesDetected(m_toolCallFormat, m_modelInfo);

    // Log full model info for debugging
    LOG_DEBUG(QString("Full model info: %1").arg(QString::fromUtf8(doc.toJson(QJsonDocument::Indented))));

    // Process any pending requests that were queued while waiting for capabilities
    processPendingRequests();
}

void LLMClient::processPendingRequests() {
    if (m_pendingRequests.isEmpty()) {
        LOG_DEBUG("No pending requests to process");
        return;
    }

    LOG_INFO(QString("Processing %1 pending requests after capability detection")
            .arg(m_pendingRequests.size()));

    // Process all queued requests
    QList<PendingRequest> requests = m_pendingRequests;
    m_pendingRequests.clear();

    for (const PendingRequest &req : requests) {
        if (req.withTools) {
            LOG_DEBUG(QString("Processing pending request with tools: %1")
                     .arg(req.prompt.left(50)));
            sendPromptWithTools(req.prompt, req.tools, req.context);
        } else {
            LOG_DEBUG(QString("Processing pending request: %1")
                     .arg(req.prompt.left(50)));
            sendPrompt(req.prompt, req.context);
        }
    }
}

// Conversation history management
void LLMClient::clearConversationHistory() {
    m_messageHistory = QJsonArray();
    LOG_INFO("Conversation history cleared");
}

// Context window management implementation
int LLMClient::estimateTokens(const QString &text) const {
    if (text.isEmpty()) {
        return 0;
    }

    // Rough estimation for English and code:
    // - Average: ~4 characters per token
    // - For JSON/structured: ~3.5 characters per token
    // - For natural language: ~4.5 characters per token
    // We use 4 as a reasonable middle ground

    // Also count spaces and newlines as slight overhead
    int charCount = text.length();
    int spaceCount = text.count(' ') + text.count('\n');

    // More conservative estimate: treat each space/newline as potential token boundary
    return (charCount / 4) + (spaceCount / 10);
}

QJsonArray LLMClient::pruneMessageHistoryForContext(const QString &systemPrompt, const QString &currentUserMessage) const {
    QJsonArray prunedMessages;

    // Get context window size from config
    int contextWindowSize = Config::instance().getContextWindowSize();

    // Reserve 20% for model response, use 80% for input
    int maxInputTokens = static_cast<int>(contextWindowSize * 0.8);

    // Calculate token budget
    int systemPromptTokens = estimateTokens(systemPrompt);
    int currentMessageTokens = estimateTokens(currentUserMessage);
    int toolsOverheadTokens = 200;  // Estimated overhead for tool definitions

    int remainingTokens = maxInputTokens - systemPromptTokens - currentMessageTokens - toolsOverheadTokens;

    LOG_DEBUG(QString("Context window management: max=%1, input budget=%2, system=%3, current msg=%4, tools overhead=%5, remaining=%6")
              .arg(contextWindowSize).arg(maxInputTokens).arg(systemPromptTokens)
              .arg(currentMessageTokens).arg(toolsOverheadTokens).arg(remainingTokens));

    if (remainingTokens <= 0) {
        LOG_WARNING(QString("Current message exceeds context budget! Message tokens: %1, budget: %2")
                   .arg(currentMessageTokens).arg(maxInputTokens - systemPromptTokens - toolsOverheadTokens));
        // Return empty history, only current message will fit
        return prunedMessages;
    }

    // Strategy: Work backwards from most recent messages
    // Keep as many recent messages as fit in the remaining token budget
    int usedTokens = 0;
    int messagesIncluded = 0;
    int totalMessages = m_messageHistory.size();

    for (int i = totalMessages - 1; i >= 0; i--) {
        QJsonObject msg = m_messageHistory[i].toObject();
        QString content = msg["content"].toString();

        // Estimate tokens for this message
        int msgTokens = estimateTokens(content);

        // Add overhead for role and JSON structure (~20 tokens)
        msgTokens += 20;

        // Check if adding this message would exceed budget
        if (usedTokens + msgTokens > remainingTokens) {
            LOG_DEBUG(QString("Stopping history pruning: would exceed budget (%1 + %2 > %3)")
                     .arg(usedTokens).arg(msgTokens).arg(remainingTokens));
            break;
        }

        // Add message to beginning of pruned array (since we're working backwards)
        prunedMessages.prepend(msg);
        usedTokens += msgTokens;
        messagesIncluded++;
    }

    int messagesDropped = totalMessages - messagesIncluded;

    if (messagesDropped > 0) {
        LOG_INFO(QString("Pruned message history: kept %1/%2 messages (%3 tokens), dropped %4 oldest messages")
                .arg(messagesIncluded).arg(totalMessages).arg(usedTokens).arg(messagesDropped));
    } else {
        LOG_DEBUG(QString("Message history fits in context: %1 messages (%2 tokens)")
                 .arg(messagesIncluded).arg(usedTokens));
    }

    return prunedMessages;
}
