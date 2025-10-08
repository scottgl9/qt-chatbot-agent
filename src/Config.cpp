/**
 * Config.cpp - Application configuration management
 * 
 * Loads/saves configuration from JSON file (~/.qtbot/config.json),
 * provides default values, and manages LLM, RAG, and MCP settings.
 */

#include "Config.h"
#include "Logger.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QFileInfo>

Config& Config::instance() {
    static Config instance;
    return instance;
}

Config::Config()
    : m_backend("ollama")
    , m_model("gpt-oss:20b")
    , m_apiUrl("http://localhost:11434/api/generate")
    , m_openaiApiKey("")
    , m_systemPrompt("You are a helpful AI assistant with access to tools. Use the available tools when appropriate to provide accurate and helpful responses.")
    , m_contextWindowSize(4096)
    , m_temperature(0.7)
    , m_topP(0.9)
    , m_topK(40)
    , m_maxTokens(2048)
    , m_overrideContextWindowSize(false)  // Default: use model defaults
    , m_overrideTemperature(false)
    , m_overrideTopP(false)
    , m_overrideTopK(false)
    , m_overrideMaxTokens(false)
    , m_ragEnabled(false)  // RAG disabled by default
    , m_ragEmbeddingModel("nomic-embed-text")
    , m_ragChunkSize(512)
    , m_ragChunkOverlap(50)
    , m_ragTopK(3) {
}

QString Config::getDefaultConfigPath() const {
    return QDir::homePath() + "/.qtbot/config.json";
}

bool Config::load(const QString &configPath) {
    QMutexLocker locker(&m_mutex);

    m_configPath = configPath.isEmpty() ? getDefaultConfigPath() : configPath;

    QFile file(m_configPath);
    if (!file.exists()) {
        LOG_INFO(QString("Config file not found at %1, creating with defaults").arg(m_configPath));

        // Create directory if it doesn't exist
        QFileInfo fileInfo(m_configPath);
        QDir configDir = fileInfo.dir();
        if (!configDir.exists()) {
            configDir.mkpath(".");
        }

        // Save default configuration
        locker.unlock();
        return save();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open config file: %1").arg(m_configPath));
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("Failed to parse config JSON: %1").arg(parseError.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Config file is not a JSON object");
        return false;
    }

    fromJson(doc.object());
    LOG_INFO(QString("Configuration loaded from %1").arg(m_configPath));
    LOG_DEBUG(QString("Backend: %1, Model: %2, API URL: %3").arg(m_backend, m_model, m_apiUrl));

    return true;
}

bool Config::save() {
    QMutexLocker locker(&m_mutex);

    if (m_configPath.isEmpty()) {
        m_configPath = getDefaultConfigPath();
    }

    // Create directory if it doesn't exist
    QFileInfo fileInfo(m_configPath);
    QDir configDir = fileInfo.dir();
    if (!configDir.exists()) {
        if (!configDir.mkpath(".")) {
            LOG_ERROR(QString("Failed to create config directory: %1").arg(configDir.path()));
            return false;
        }
    }

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("Failed to open config file for writing: %1").arg(m_configPath));
        return false;
    }

    QJsonDocument doc(toJson());
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    qint64 bytesWritten = file.write(jsonData);
    file.close();

    if (bytesWritten == -1) {
        LOG_ERROR("Failed to write config file");
        return false;
    }

    LOG_INFO(QString("Configuration saved to %1").arg(m_configPath));
    return true;
}

void Config::setBackend(const QString &backend) {
    QMutexLocker locker(&m_mutex);
    m_backend = backend;
}

void Config::setModel(const QString &model) {
    QMutexLocker locker(&m_mutex);
    m_model = model;
}

void Config::setApiUrl(const QString &apiUrl) {
    QMutexLocker locker(&m_mutex);
    m_apiUrl = apiUrl;
}

void Config::setOpenAIApiKey(const QString &apiKey) {
    QMutexLocker locker(&m_mutex);
    m_openaiApiKey = apiKey;
}

void Config::setSystemPrompt(const QString &systemPrompt) {
    QMutexLocker locker(&m_mutex);
    m_systemPrompt = systemPrompt;
}

void Config::setContextWindowSize(int size) {
    QMutexLocker locker(&m_mutex);
    m_contextWindowSize = size;
}

void Config::setTemperature(double temp) {
    QMutexLocker locker(&m_mutex);
    m_temperature = temp;
}

void Config::setTopP(double topP) {
    QMutexLocker locker(&m_mutex);
    m_topP = topP;
}

void Config::setTopK(int topK) {
    QMutexLocker locker(&m_mutex);
    m_topK = topK;
}

void Config::setMaxTokens(int maxTokens) {
    QMutexLocker locker(&m_mutex);
    m_maxTokens = maxTokens;
}

void Config::setOverrideContextWindowSize(bool override) {
    QMutexLocker locker(&m_mutex);
    m_overrideContextWindowSize = override;
}

void Config::setOverrideTemperature(bool override) {
    QMutexLocker locker(&m_mutex);
    m_overrideTemperature = override;
}

void Config::setOverrideTopP(bool override) {
    QMutexLocker locker(&m_mutex);
    m_overrideTopP = override;
}

void Config::setOverrideTopK(bool override) {
    QMutexLocker locker(&m_mutex);
    m_overrideTopK = override;
}

void Config::setOverrideMaxTokens(bool override) {
    QMutexLocker locker(&m_mutex);
    m_overrideMaxTokens = override;
}

void Config::setRagEnabled(bool enabled) {
    QMutexLocker locker(&m_mutex);
    m_ragEnabled = enabled;
}

void Config::setRagEmbeddingModel(const QString &model) {
    QMutexLocker locker(&m_mutex);
    m_ragEmbeddingModel = model;
}

void Config::setRagChunkSize(int size) {
    QMutexLocker locker(&m_mutex);
    m_ragChunkSize = size;
}

void Config::setRagChunkOverlap(int overlap) {
    QMutexLocker locker(&m_mutex);
    m_ragChunkOverlap = overlap;
}

void Config::setRagTopK(int topK) {
    QMutexLocker locker(&m_mutex);
    m_ragTopK = topK;
}

void Config::setMcpServers(const QJsonArray &servers) {
    QMutexLocker locker(&m_mutex);
    m_mcpServers = servers;
}

void Config::resetToDefaults() {
    QMutexLocker locker(&m_mutex);
    m_backend = "ollama";
    m_model = "gpt-oss:20b";
    m_apiUrl = "http://localhost:11434/api/generate";
    m_openaiApiKey = "";
    m_systemPrompt = "You are a helpful AI assistant with access to tools. Use the available tools when appropriate to provide accurate and helpful responses.";
    // Set reasonable values but don't override by default
    m_contextWindowSize = 4096;
    m_temperature = 0.7;
    m_topP = 0.9;
    m_topK = 40;
    m_maxTokens = 2048;
    m_overrideContextWindowSize = false;
    m_overrideTemperature = false;
    m_overrideTopP = false;
    m_overrideTopK = false;
    m_overrideMaxTokens = false;
    m_ragEnabled = false;
    m_ragEmbeddingModel = "nomic-embed-text";
    m_ragChunkSize = 512;
    m_ragChunkOverlap = 50;
    m_ragTopK = 3;
    m_mcpServers = QJsonArray();  // Empty array - no MCP servers by default
    LOG_INFO("Configuration reset to defaults (LLM parameter overrides, RAG, and MCP servers cleared)");
}

bool Config::isValid() const {
    QMutexLocker locker(&m_mutex);

    if (m_backend.isEmpty()) {
        return false;
    }

    if (m_model.isEmpty()) {
        return false;
    }

    if (m_apiUrl.isEmpty()) {
        return false;
    }

    // If backend is OpenAI, API key should be provided
    if (m_backend.toLower() == "openai" && m_openaiApiKey.isEmpty()) {
        LOG_WARNING("OpenAI backend selected but API key is empty");
    }

    return true;
}

QJsonObject Config::toJson() const {
    QJsonObject obj;
    obj["backend"] = m_backend;
    obj["model"] = m_model;
    obj["api_url"] = m_apiUrl;
    obj["openai_api_key"] = m_openaiApiKey;
    obj["system_prompt"] = m_systemPrompt;
    obj["context_window_size"] = m_contextWindowSize;
    obj["temperature"] = m_temperature;
    obj["top_p"] = m_topP;
    obj["top_k"] = m_topK;
    obj["max_tokens"] = m_maxTokens;
    obj["override_context_window_size"] = m_overrideContextWindowSize;
    obj["override_temperature"] = m_overrideTemperature;
    obj["override_top_p"] = m_overrideTopP;
    obj["override_top_k"] = m_overrideTopK;
    obj["override_max_tokens"] = m_overrideMaxTokens;
    obj["rag_enabled"] = m_ragEnabled;
    obj["rag_embedding_model"] = m_ragEmbeddingModel;
    obj["rag_chunk_size"] = m_ragChunkSize;
    obj["rag_chunk_overlap"] = m_ragChunkOverlap;
    obj["rag_top_k"] = m_ragTopK;
    obj["mcp_servers"] = m_mcpServers;
    return obj;
}

void Config::fromJson(const QJsonObject &json) {
    if (json.contains("backend") && json["backend"].isString()) {
        m_backend = json["backend"].toString();
    }

    if (json.contains("model") && json["model"].isString()) {
        m_model = json["model"].toString();
    }

    if (json.contains("api_url") && json["api_url"].isString()) {
        m_apiUrl = json["api_url"].toString();
    }

    if (json.contains("openai_api_key") && json["openai_api_key"].isString()) {
        m_openaiApiKey = json["openai_api_key"].toString();
    }

    if (json.contains("system_prompt") && json["system_prompt"].isString()) {
        m_systemPrompt = json["system_prompt"].toString();
    }

    if (json.contains("context_window_size") && json["context_window_size"].isDouble()) {
        m_contextWindowSize = json["context_window_size"].toInt();
    }

    if (json.contains("temperature") && json["temperature"].isDouble()) {
        m_temperature = json["temperature"].toDouble();
    }

    if (json.contains("top_p") && json["top_p"].isDouble()) {
        m_topP = json["top_p"].toDouble();
    }

    if (json.contains("top_k") && json["top_k"].isDouble()) {
        m_topK = json["top_k"].toInt();
    }

    if (json.contains("max_tokens") && json["max_tokens"].isDouble()) {
        m_maxTokens = json["max_tokens"].toInt();
    }

    if (json.contains("override_context_window_size") && json["override_context_window_size"].isBool()) {
        m_overrideContextWindowSize = json["override_context_window_size"].toBool();
    }

    if (json.contains("override_temperature") && json["override_temperature"].isBool()) {
        m_overrideTemperature = json["override_temperature"].toBool();
    }

    if (json.contains("override_top_p") && json["override_top_p"].isBool()) {
        m_overrideTopP = json["override_top_p"].toBool();
    }

    if (json.contains("override_top_k") && json["override_top_k"].isBool()) {
        m_overrideTopK = json["override_top_k"].toBool();
    }

    if (json.contains("override_max_tokens") && json["override_max_tokens"].isBool()) {
        m_overrideMaxTokens = json["override_max_tokens"].toBool();
    }

    if (json.contains("rag_enabled") && json["rag_enabled"].isBool()) {
        m_ragEnabled = json["rag_enabled"].toBool();
    }

    if (json.contains("rag_embedding_model") && json["rag_embedding_model"].isString()) {
        m_ragEmbeddingModel = json["rag_embedding_model"].toString();
    }

    if (json.contains("rag_chunk_size") && json["rag_chunk_size"].isDouble()) {
        m_ragChunkSize = json["rag_chunk_size"].toInt();
    }

    if (json.contains("rag_chunk_overlap") && json["rag_chunk_overlap"].isDouble()) {
        m_ragChunkOverlap = json["rag_chunk_overlap"].toInt();
    }

    if (json.contains("rag_top_k") && json["rag_top_k"].isDouble()) {
        m_ragTopK = json["rag_top_k"].toInt();
    }

    if (json.contains("mcp_servers") && json["mcp_servers"].isArray()) {
        m_mcpServers = json["mcp_servers"].toArray();
    }
}
