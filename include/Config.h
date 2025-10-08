/**
 * Config.h - Application configuration manager
 * 
 * Singleton for managing application settings (LLM, RAG, MCP), loads/saves
 * JSON configuration from ~/.qtbot/config.json, provides default values.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <cmath>

class Config {
public:
    static Config& instance();

    // Initialize and load configuration
    bool load(const QString &configPath = QString());
    bool save();

    // Getters
    QString getBackend() const { return m_backend; }
    QString getModel() const { return m_model; }
    QString getApiUrl() const { return m_apiUrl; }
    QString getOpenAIApiKey() const { return m_openaiApiKey; }
    QString getSystemPrompt() const { return m_systemPrompt; }
    QString getConfigPath() const { return m_configPath; }

    // LLM Configuration Getters
    int getContextWindowSize() const { return m_contextWindowSize; }
    double getTemperature() const { return m_temperature; }
    double getTopP() const { return m_topP; }
    int getTopK() const { return m_topK; }
    int getMaxTokens() const { return m_maxTokens; }

    // Check if parameter override is enabled
    bool getOverrideContextWindowSize() const { return m_overrideContextWindowSize; }
    bool getOverrideTemperature() const { return m_overrideTemperature; }
    bool getOverrideTopP() const { return m_overrideTopP; }
    bool getOverrideTopK() const { return m_overrideTopK; }
    bool getOverrideMaxTokens() const { return m_overrideMaxTokens; }

    // RAG Configuration Getters
    bool getRagEnabled() const { return m_ragEnabled; }
    QString getRagEmbeddingModel() const { return m_ragEmbeddingModel; }
    int getRagChunkSize() const { return m_ragChunkSize; }
    int getRagChunkOverlap() const { return m_ragChunkOverlap; }
    int getRagTopK() const { return m_ragTopK; }

    // MCP Server Configuration Getters
    QJsonArray getMcpServers() const { return m_mcpServers; }

    // Setters
    void setBackend(const QString &backend);
    void setModel(const QString &model);
    void setApiUrl(const QString &apiUrl);
    void setOpenAIApiKey(const QString &apiKey);
    void setSystemPrompt(const QString &systemPrompt);

    // LLM Configuration Setters
    void setContextWindowSize(int size);
    void setTemperature(double temp);
    void setTopP(double topP);
    void setTopK(int topK);
    void setMaxTokens(int maxTokens);

    // Override flag setters
    void setOverrideContextWindowSize(bool override);
    void setOverrideTemperature(bool override);
    void setOverrideTopP(bool override);
    void setOverrideTopK(bool override);
    void setOverrideMaxTokens(bool override);

    // RAG Configuration Setters
    void setRagEnabled(bool enabled);
    void setRagEmbeddingModel(const QString &model);
    void setRagChunkSize(int size);
    void setRagChunkOverlap(int overlap);
    void setRagTopK(int topK);

    // MCP Server Configuration Setters
    void setMcpServers(const QJsonArray &servers);

    // Reset to defaults
    void resetToDefaults();

    // Validate configuration
    bool isValid() const;

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QString getDefaultConfigPath() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

    QString m_configPath;
    QString m_backend;
    QString m_model;
    QString m_apiUrl;
    QString m_openaiApiKey;
    QString m_systemPrompt;

    // LLM Configuration Parameters
    int m_contextWindowSize;
    double m_temperature;
    double m_topP;
    int m_topK;
    int m_maxTokens;

    // Override flags - if false, don't include parameter in request (use model default)
    bool m_overrideContextWindowSize;
    bool m_overrideTemperature;
    bool m_overrideTopP;
    bool m_overrideTopK;
    bool m_overrideMaxTokens;

    // RAG Configuration
    bool m_ragEnabled;
    QString m_ragEmbeddingModel;
    int m_ragChunkSize;
    int m_ragChunkOverlap;
    int m_ragTopK;

    // MCP Server Configuration
    QJsonArray m_mcpServers;

    mutable QMutex m_mutex;
};

#endif // CONFIG_H
