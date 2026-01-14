/**
 * SettingsDialog.cpp - Application settings dialog
 * 
 * Provides UI for configuring LLM backend, model selection, API settings,
 * RAG parameters, and context window management.
 */

#include "SettingsDialog.h"
#include "Config.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , networkManager(nullptr)
    , silentRefresh(false) {
    setWindowTitle("Settings");
    setMinimumWidth(500);

    createUI();
    loadCurrentSettings();

    // Defer network manager creation until event loop is running
    QTimer::singleShot(0, this, [this]() {
        networkManager = new QNetworkAccessManager(this);
        connect(networkManager, &QNetworkAccessManager::finished,
                this, [this](QNetworkReply *reply) {
                    // Route to appropriate handler based on request attribute
                    QVariant attr = reply->request().attribute(QNetworkRequest::User);
                    QString attrStr = attr.toString();
                    if (attrStr == "embedding_models") {
                        handleEmbeddingModelsResponse(reply);
                    } else if (attrStr == "lemonade_models") {
                        handleLemonadeModelsResponse(reply);
                    } else {
                        handleModelsResponse(reply);
                    }
                });
        LOG_DEBUG("SettingsDialog: QNetworkAccessManager initialized");

        // Auto-refresh model list if using Ollama backend
        if (backendCombo->currentText().toLower() == "ollama") {
            QTimer::singleShot(100, this, [this]() {
                LOG_INFO("Auto-refreshing Ollama models on Settings dialog open");
                fetchOllamaModels(true); // Silent mode - no success popup
            });
        }
    });
}

void SettingsDialog::createUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Backend Settings Group
    QGroupBox *backendGroup = new QGroupBox("Backend Settings", this);
    QFormLayout *backendLayout = new QFormLayout(backendGroup);

    backendCombo = new QComboBox(this);
    backendCombo->addItem("Ollama");
    backendCombo->addItem("Lemonade");
    backendCombo->addItem("OpenAI");
    connect(backendCombo, &QComboBox::currentTextChanged, this, &SettingsDialog::onBackendChanged);
    backendLayout->addRow("Backend:", backendCombo);

    // Model selection with refresh button
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelCombo = new QComboBox(this);
    modelCombo->setEditable(true);
    modelCombo->setPlaceholderText("Select or enter model name");
    modelLayout->addWidget(modelCombo, 1);

    refreshModelsButton = new QPushButton("Refresh", this);
    refreshModelsButton->setToolTip("Fetch available models from Ollama server");
    connect(refreshModelsButton, &QPushButton::clicked, this, &SettingsDialog::refreshModels);
    modelLayout->addWidget(refreshModelsButton);

    backendLayout->addRow("Model:", modelLayout);

    apiUrlEdit = new QLineEdit(this);
    apiUrlEdit->setPlaceholderText("http://localhost:11434/api/generate");
    backendLayout->addRow("API URL:", apiUrlEdit);

    apiKeyEdit = new QLineEdit(this);
    apiKeyEdit->setPlaceholderText("API Key (for OpenAI)");
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    backendLayout->addRow("API Key:", apiKeyEdit);

    mainLayout->addWidget(backendGroup);

    // System Prompt Group
    QGroupBox *promptGroup = new QGroupBox("System Prompt", this);
    QVBoxLayout *promptLayout = new QVBoxLayout(promptGroup);

    QLabel *promptLabel = new QLabel("Configure the system prompt for the LLM:", this);
    promptLabel->setWordWrap(true);
    promptLayout->addWidget(promptLabel);

    systemPromptEdit = new QTextEdit(this);
    systemPromptEdit->setPlaceholderText("Enter system prompt here...");
    systemPromptEdit->setMaximumHeight(100);
    systemPromptEdit->setToolTip("The system prompt sets the behavior and personality of the AI assistant");
    promptLayout->addWidget(systemPromptEdit);

    mainLayout->addWidget(promptGroup);

    // LLM Parameters Group
    QGroupBox *paramsGroup = new QGroupBox("LLM Parameters", this);
    QVBoxLayout *paramsGroupLayout = new QVBoxLayout(paramsGroup);

    QLabel *paramsNote = new QLabel("<i>Check 'Override' to customize parameters, otherwise model defaults are used.</i>");
    paramsNote->setWordWrap(true);
    paramsGroupLayout->addWidget(paramsNote);

    QFormLayout *paramsLayout = new QFormLayout();
    paramsGroupLayout->addLayout(paramsLayout);

    // Context Window
    QHBoxLayout *ctxLayout = new QHBoxLayout();
    overrideContextWindowCheckbox = new QCheckBox("Override", this);
    contextWindowSpinBox = new QSpinBox(this);
    contextWindowSpinBox->setRange(512, 32768);
    contextWindowSpinBox->setSingleStep(512);
    contextWindowSpinBox->setSuffix(" tokens");
    contextWindowSpinBox->setEnabled(false);
    connect(overrideContextWindowCheckbox, &QCheckBox::toggled, contextWindowSpinBox, &QSpinBox::setEnabled);
    ctxLayout->addWidget(overrideContextWindowCheckbox);
    ctxLayout->addWidget(contextWindowSpinBox);
    QLabel *ctxLabel = new QLabel("Context Window:");
    ctxLabel->setToolTip("Maximum context size for the conversation");
    paramsLayout->addRow(ctxLabel, ctxLayout);

    // Temperature
    QHBoxLayout *tempLayout = new QHBoxLayout();
    overrideTemperatureCheckbox = new QCheckBox("Override", this);
    temperatureSpinBox = new QDoubleSpinBox(this);
    temperatureSpinBox->setRange(0.0, 2.0);
    temperatureSpinBox->setSingleStep(0.1);
    temperatureSpinBox->setDecimals(2);
    temperatureSpinBox->setEnabled(false);
    connect(overrideTemperatureCheckbox, &QCheckBox::toggled, temperatureSpinBox, &QDoubleSpinBox::setEnabled);
    tempLayout->addWidget(overrideTemperatureCheckbox);
    tempLayout->addWidget(temperatureSpinBox);
    QLabel *tempLabel = new QLabel("Temperature:");
    tempLabel->setToolTip("Controls randomness (0.0 = focused, 1.0+ = creative)");
    paramsLayout->addRow(tempLabel, tempLayout);

    // Top-P
    QHBoxLayout *topPLayout = new QHBoxLayout();
    overrideTopPCheckbox = new QCheckBox("Override", this);
    topPSpinBox = new QDoubleSpinBox(this);
    topPSpinBox->setRange(0.0, 1.0);
    topPSpinBox->setSingleStep(0.05);
    topPSpinBox->setDecimals(2);
    topPSpinBox->setEnabled(false);
    connect(overrideTopPCheckbox, &QCheckBox::toggled, topPSpinBox, &QDoubleSpinBox::setEnabled);
    topPLayout->addWidget(overrideTopPCheckbox);
    topPLayout->addWidget(topPSpinBox);
    QLabel *topPLabel = new QLabel("Top-P:");
    topPLabel->setToolTip("Nucleus sampling threshold");
    paramsLayout->addRow(topPLabel, topPLayout);

    // Top-K
    QHBoxLayout *topKLayout = new QHBoxLayout();
    overrideTopKCheckbox = new QCheckBox("Override", this);
    topKSpinBox = new QSpinBox(this);
    topKSpinBox->setRange(1, 100);
    topKSpinBox->setSingleStep(5);
    topKSpinBox->setEnabled(false);
    connect(overrideTopKCheckbox, &QCheckBox::toggled, topKSpinBox, &QSpinBox::setEnabled);
    topKLayout->addWidget(overrideTopKCheckbox);
    topKLayout->addWidget(topKSpinBox);
    QLabel *topKLabel = new QLabel("Top-K:");
    topKLabel->setToolTip("Number of highest probability tokens to consider");
    paramsLayout->addRow(topKLabel, topKLayout);

    // Max Tokens
    QHBoxLayout *maxTokensLayout = new QHBoxLayout();
    overrideMaxTokensCheckbox = new QCheckBox("Override", this);
    maxTokensSpinBox = new QSpinBox(this);
    maxTokensSpinBox->setRange(128, 8192);
    maxTokensSpinBox->setSingleStep(128);
    maxTokensSpinBox->setSuffix(" tokens");
    maxTokensSpinBox->setEnabled(false);
    connect(overrideMaxTokensCheckbox, &QCheckBox::toggled, maxTokensSpinBox, &QSpinBox::setEnabled);
    maxTokensLayout->addWidget(overrideMaxTokensCheckbox);
    maxTokensLayout->addWidget(maxTokensSpinBox);
    QLabel *maxTokensLabel = new QLabel("Max Tokens:");
    maxTokensLabel->setToolTip("Maximum length of generated response");
    paramsLayout->addRow(maxTokensLabel, maxTokensLayout);

    mainLayout->addWidget(paramsGroup);

    // RAG Settings Group
    QGroupBox *ragGroup = new QGroupBox("RAG (Retrieval-Augmented Generation) Settings", this);
    QVBoxLayout *ragGroupLayout = new QVBoxLayout(ragGroup);

    ragEnabledCheckbox = new QCheckBox("Enable RAG (disabled by default)", this);
    ragEnabledCheckbox->setToolTip("Enable retrieval-augmented generation to inject document context into conversations");
    ragGroupLayout->addWidget(ragEnabledCheckbox);

    QFormLayout *ragLayout = new QFormLayout();
    ragGroupLayout->addLayout(ragLayout);

    // Embedding model selection with refresh button
    QHBoxLayout *embeddingModelLayout = new QHBoxLayout();
    ragEmbeddingModelCombo = new QComboBox(this);
    ragEmbeddingModelCombo->setEditable(true);
    ragEmbeddingModelCombo->setPlaceholderText("nomic-embed-text");
    ragEmbeddingModelCombo->setToolTip("Ollama model to use for generating embeddings");
    embeddingModelLayout->addWidget(ragEmbeddingModelCombo, 1);

    refreshEmbeddingModelsButton = new QPushButton("Refresh", this);
    refreshEmbeddingModelsButton->setToolTip("Fetch available embedding models from Ollama server");
    connect(refreshEmbeddingModelsButton, &QPushButton::clicked, this, &SettingsDialog::refreshEmbeddingModels);
    embeddingModelLayout->addWidget(refreshEmbeddingModelsButton);

    ragLayout->addRow("Embedding Model:", embeddingModelLayout);

    ragChunkSizeSpinBox = new QSpinBox(this);
    ragChunkSizeSpinBox->setRange(128, 2048);
    ragChunkSizeSpinBox->setSingleStep(128);
    ragChunkSizeSpinBox->setSuffix(" chars");
    ragChunkSizeSpinBox->setToolTip("Size of text chunks for document processing");
    ragLayout->addRow("Chunk Size:", ragChunkSizeSpinBox);

    ragChunkOverlapSpinBox = new QSpinBox(this);
    ragChunkOverlapSpinBox->setRange(0, 512);
    ragChunkOverlapSpinBox->setSingleStep(10);
    ragChunkOverlapSpinBox->setSuffix(" chars");
    ragChunkOverlapSpinBox->setToolTip("Overlap between consecutive chunks for better context");
    ragLayout->addRow("Chunk Overlap:", ragChunkOverlapSpinBox);

    ragTopKSpinBox = new QSpinBox(this);
    ragTopKSpinBox->setRange(1, 10);
    ragTopKSpinBox->setSingleStep(1);
    ragTopKSpinBox->setToolTip("Number of most relevant chunks to retrieve for context");
    ragLayout->addRow("Top K Results:", ragTopKSpinBox);

    mainLayout->addWidget(ragGroup);

    // MCP Servers Group
    QGroupBox *mcpGroup = new QGroupBox("MCP (Model Context Protocol) Servers", this);
    QVBoxLayout *mcpGroupLayout = new QVBoxLayout(mcpGroup);

    QLabel *mcpLabel = new QLabel("<i>Configure external MCP servers that provide additional tools via HTTP or SSE.</i>", this);
    mcpLabel->setWordWrap(true);
    mcpGroupLayout->addWidget(mcpLabel);

    // Server list and buttons
    QHBoxLayout *mcpListLayout = new QHBoxLayout();

    mcpServerList = new QListWidget(this);
    mcpServerList->setMaximumHeight(150);
    mcpServerList->setToolTip("List of configured MCP servers");
    connect(mcpServerList, &QListWidget::itemSelectionChanged, this, &SettingsDialog::onMcpServerSelectionChanged);
    mcpListLayout->addWidget(mcpServerList, 1);

    // Buttons column
    QVBoxLayout *mcpButtonsLayout = new QVBoxLayout();

    addMcpServerButton = new QPushButton("Add", this);
    addMcpServerButton->setToolTip("Add a new MCP server");
    connect(addMcpServerButton, &QPushButton::clicked, this, &SettingsDialog::addMcpServer);
    mcpButtonsLayout->addWidget(addMcpServerButton);

    editMcpServerButton = new QPushButton("Edit", this);
    editMcpServerButton->setToolTip("Edit selected MCP server");
    editMcpServerButton->setEnabled(false);
    connect(editMcpServerButton, &QPushButton::clicked, this, &SettingsDialog::editMcpServer);
    mcpButtonsLayout->addWidget(editMcpServerButton);

    deleteMcpServerButton = new QPushButton("Delete", this);
    deleteMcpServerButton->setToolTip("Delete selected MCP server");
    deleteMcpServerButton->setEnabled(false);
    connect(deleteMcpServerButton, &QPushButton::clicked, this, &SettingsDialog::deleteMcpServer);
    mcpButtonsLayout->addWidget(deleteMcpServerButton);

    toggleMcpServerButton = new QPushButton("Toggle Enabled", this);
    toggleMcpServerButton->setToolTip("Enable or disable selected MCP server");
    toggleMcpServerButton->setEnabled(false);
    connect(toggleMcpServerButton, &QPushButton::clicked, this, &SettingsDialog::toggleMcpServerEnabled);
    mcpButtonsLayout->addWidget(toggleMcpServerButton);

    mcpButtonsLayout->addStretch();

    mcpListLayout->addLayout(mcpButtonsLayout);
    mcpGroupLayout->addLayout(mcpListLayout);

    mainLayout->addWidget(mcpGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    resetButton = new QPushButton("Reset to Defaults", this);
    connect(resetButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
    buttonLayout->addWidget(resetButton);

    cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsDialog::cancelSettings);
    buttonLayout->addWidget(cancelButton);

    saveButton = new QPushButton("Save", this);
    saveButton->setDefault(true);
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    buttonLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void SettingsDialog::loadCurrentSettings() {
    // Load backend settings
    QString backend = Config::instance().getBackend();
    backendCombo->setCurrentText(backend == "ollama" ? "Ollama" : "OpenAI");

    modelCombo->setEditText(Config::instance().getModel());
    apiUrlEdit->setText(Config::instance().getApiUrl());
    apiKeyEdit->setText(Config::instance().getOpenAIApiKey());
    systemPromptEdit->setPlainText(Config::instance().getSystemPrompt());

    // Load LLM parameters
    contextWindowSpinBox->setValue(Config::instance().getContextWindowSize());
    temperatureSpinBox->setValue(Config::instance().getTemperature());
    topPSpinBox->setValue(Config::instance().getTopP());
    topKSpinBox->setValue(Config::instance().getTopK());
    maxTokensSpinBox->setValue(Config::instance().getMaxTokens());

    // Load override checkboxes
    overrideContextWindowCheckbox->setChecked(Config::instance().getOverrideContextWindowSize());
    overrideTemperatureCheckbox->setChecked(Config::instance().getOverrideTemperature());
    overrideTopPCheckbox->setChecked(Config::instance().getOverrideTopP());
    overrideTopKCheckbox->setChecked(Config::instance().getOverrideTopK());
    overrideMaxTokensCheckbox->setChecked(Config::instance().getOverrideMaxTokens());

    // Load RAG settings
    ragEnabledCheckbox->setChecked(Config::instance().getRagEnabled());
    ragEmbeddingModelCombo->setEditText(Config::instance().getRagEmbeddingModel());
    ragChunkSizeSpinBox->setValue(Config::instance().getRagChunkSize());
    ragChunkOverlapSpinBox->setValue(Config::instance().getRagChunkOverlap());
    ragTopKSpinBox->setValue(Config::instance().getRagTopK());

    // Load MCP servers
    mcpServers = Config::instance().getMcpServers();
    updateMcpServerList();

    // Update model refresh button state
    onBackendChanged(backendCombo->currentText());

    LOG_DEBUG("Loaded current settings into dialog");
}

void SettingsDialog::saveSettings() {
    // Save backend settings
    QString backend = backendCombo->currentText().toLower();
    Config::instance().setBackend(backend);
    Config::instance().setModel(modelCombo->currentText());
    Config::instance().setApiUrl(apiUrlEdit->text());
    Config::instance().setOpenAIApiKey(apiKeyEdit->text());
    Config::instance().setSystemPrompt(systemPromptEdit->toPlainText());

    // Save LLM parameters
    Config::instance().setContextWindowSize(contextWindowSpinBox->value());
    Config::instance().setTemperature(temperatureSpinBox->value());
    Config::instance().setTopP(topPSpinBox->value());
    Config::instance().setTopK(topKSpinBox->value());
    Config::instance().setMaxTokens(maxTokensSpinBox->value());

    // Save override flags
    Config::instance().setOverrideContextWindowSize(overrideContextWindowCheckbox->isChecked());
    Config::instance().setOverrideTemperature(overrideTemperatureCheckbox->isChecked());
    Config::instance().setOverrideTopP(overrideTopPCheckbox->isChecked());
    Config::instance().setOverrideTopK(overrideTopKCheckbox->isChecked());
    Config::instance().setOverrideMaxTokens(overrideMaxTokensCheckbox->isChecked());

    // Save RAG settings
    Config::instance().setRagEnabled(ragEnabledCheckbox->isChecked());
    Config::instance().setRagEmbeddingModel(ragEmbeddingModelCombo->currentText());
    Config::instance().setRagChunkSize(ragChunkSizeSpinBox->value());
    Config::instance().setRagChunkOverlap(ragChunkOverlapSpinBox->value());
    Config::instance().setRagTopK(ragTopKSpinBox->value());

    // Save MCP servers
    Config::instance().setMcpServers(mcpServers);

    // Persist to file
    if (Config::instance().save()) {
        LOG_INFO("Settings saved successfully");
        emit settingsSaved();  // Notify main window to refresh MCP tools
        accept();  // Close dialog without showing success message
    } else {
        LOG_ERROR("Failed to save settings");
        QMessageBox::warning(this, "Settings", "Failed to save settings to file.");
        // Dialog stays open on error so user can retry
    }
}

void SettingsDialog::cancelSettings() {
    LOG_DEBUG("Settings dialog cancelled");
    reject();
}

void SettingsDialog::resetToDefaults() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Reset Settings",
                                   "Reset all settings to default values?",
                                   QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        Config::instance().resetToDefaults();
        loadCurrentSettings();
        LOG_INFO("Settings reset to defaults");
    }
}

void SettingsDialog::refreshModels() {
    if (!networkManager) {
        LOG_WARNING("Network manager not initialized yet");
        QMessageBox::warning(this, "Not Ready", "Please wait a moment and try again.");
        return;
    }

    QString backend = backendCombo->currentText();
    if (backend == "Ollama") {
        fetchOllamaModels(false); // Show success message for manual refresh
    } else if (backend == "Lemonade") {
        fetchLemonadeModels(false);
    } else {
        QMessageBox::information(this, "Info",
            "Model refresh is only available for Ollama and Lemonade backends.\n"
            "For OpenAI, please enter your model name manually (e.g., gpt-4, gpt-3.5-turbo).");
    }
}

void SettingsDialog::fetchOllamaModels(bool silentMode) {
    silentRefresh = silentMode;

    refreshModelsButton->setEnabled(false);
    if (!silentMode) {
        refreshModelsButton->setText("Loading...");
    }

    // Extract base URL from API URL (http://host:port/api/generate -> http://host:port/api/tags)
    QString apiUrl = apiUrlEdit->text();
    QString baseUrl = apiUrl;

    // Remove /api/generate if present and add /api/tags
    if (baseUrl.endsWith("/api/generate")) {
        baseUrl = baseUrl.left(baseUrl.length() - 13); // Remove "/api/generate"
    } else if (baseUrl.endsWith("/generate")) {
        baseUrl = baseUrl.left(baseUrl.length() - 9); // Remove "/generate"
    }

    if (!baseUrl.endsWith("/api")) {
        if (baseUrl.endsWith("/")) {
            baseUrl += "api";
        } else {
            baseUrl += "/api";
        }
    }
    baseUrl += "/tags";

    LOG_INFO(QString("Fetching models from: %1 (silent: %2)").arg(baseUrl).arg(silentMode ? "true" : "false"));

    QUrl url(baseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager->get(request);
}

void SettingsDialog::fetchLemonadeModels(bool silentMode) {
    silentRefresh = silentMode;

    refreshModelsButton->setEnabled(false);
    if (!silentMode) {
        refreshModelsButton->setText("Loading...");
    }

    // Lemonade API endpoint for listing models: http://localhost:8000/api/v1/models
    QString apiUrl = apiUrlEdit->text();
    QString baseUrl = apiUrl;

    // Remove /api/v1/chat/completions if present and add /api/v1/models
    if (baseUrl.contains("/api/v1/chat/completions")) {
        int pos = baseUrl.indexOf("/api/v1/chat/completions");
        baseUrl = baseUrl.left(pos);
    } else if (baseUrl.contains("/chat/completions")) {
        int pos = baseUrl.indexOf("/chat/completions");
        baseUrl = baseUrl.left(pos);
    } else if (baseUrl.contains("/completions")) {
        int pos = baseUrl.indexOf("/completions");
        baseUrl = baseUrl.left(pos);
    }

    if (!baseUrl.endsWith("/api/v1")) {
        if (baseUrl.endsWith("/")) {
            baseUrl += "api/v1";
        } else {
            baseUrl += "/api/v1";
        }
    }
    baseUrl += "/models";

    LOG_INFO(QString("Fetching models from Lemonade: %1 (silent: %2)").arg(baseUrl).arg(silentMode ? "true" : "false"));

    QUrl url(baseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::User, "lemonade_models");  // Tag as Lemonade request

    networkManager->get(request);
}

void SettingsDialog::handleModelsResponse(QNetworkReply *reply) {
    reply->deleteLater();

    // Re-enable refresh button
    refreshModelsButton->setEnabled(true);
    refreshModelsButton->setText("Refresh");

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch models: %1").arg(reply->errorString());
        LOG_WARNING(errorMsg);

        // Only show error popup if this was a manual refresh
        if (!silentRefresh) {
            QMessageBox::warning(this, "Network Error",
                QString("Could not connect to Ollama server:\n%1\n\n"
                        "Make sure Ollama is running and the API URL is correct.").arg(reply->errorString()));
        } else {
            LOG_INFO("Auto-refresh failed silently - Ollama server may not be available");
        }
        return;
    }

    // Parse JSON response
    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("Failed to parse response: %1").arg(parseError.errorString());
        LOG_ERROR(errorMsg);
        QMessageBox::warning(this, "Parse Error", "Invalid response from Ollama server.");
        return;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Response is not a JSON object");
        QMessageBox::warning(this, "Parse Error", "Unexpected response format from Ollama server.");
        return;
    }

    QJsonObject jsonObj = doc.object();
    if (!jsonObj.contains("models")) {
        LOG_ERROR("Response does not contain 'models' field");
        QMessageBox::warning(this, "Parse Error", "No models found in response.");
        return;
    }

    QJsonArray modelsArray = jsonObj["models"].toArray();
    if (modelsArray.isEmpty()) {
        if (!silentRefresh) {
            QMessageBox::information(this, "No Models",
                "No models found on the Ollama server.\n"
                "You can pull models using: ollama pull <model-name>");
        }
        return;
    }

    // Store current model
    QString currentModel = modelCombo->currentText();

    // Clear and populate combo box
    modelCombo->clear();
    for (const QJsonValue &modelVal : modelsArray) {
        QJsonObject modelObj = modelVal.toObject();
        QString modelName = modelObj["name"].toString();
        if (!modelName.isEmpty()) {
            // Remove :latest tag suffix for cleaner display
            if (modelName.endsWith(":latest")) {
                modelName = modelName.left(modelName.length() - 7);
            }
            modelCombo->addItem(modelName);
        }
    }

    // Restore current model if it exists in the list
    int index = modelCombo->findText(currentModel);
    if (index >= 0) {
        modelCombo->setCurrentIndex(index);
    } else if (!currentModel.isEmpty()) {
        modelCombo->setEditText(currentModel);
    }

    LOG_INFO(QString("Loaded %1 models from Ollama").arg(modelsArray.size()));

    // Only show success popup for manual refresh
    if (!silentRefresh) {
        QMessageBox::information(this, "Success",
            QString("Found %1 model(s) on the Ollama server.").arg(modelsArray.size()));
    }
}

void SettingsDialog::handleLemonadeModelsResponse(QNetworkReply *reply) {
    reply->deleteLater();

    // Re-enable refresh button
    refreshModelsButton->setEnabled(true);
    refreshModelsButton->setText("Refresh");

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch models: %1").arg(reply->errorString());
        LOG_WARNING(errorMsg);

        // Only show error popup if this was a manual refresh
        if (!silentRefresh) {
            QMessageBox::warning(this, "Network Error",
                QString("Could not connect to Lemonade server:\n%1\n\n"
                        "Make sure Lemonade is running and the API URL is correct.").arg(reply->errorString()));
        } else {
            LOG_INFO("Auto-refresh failed silently - Lemonade server may not be available");
        }
        return;
    }

    // Parse JSON response (OpenAI-compatible format)
    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("Failed to parse response: %1").arg(parseError.errorString());
        LOG_ERROR(errorMsg);
        if (!silentRefresh) {
            QMessageBox::warning(this, "Parse Error", "Invalid response from Lemonade server.");
        }
        return;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Response is not a JSON object");
        if (!silentRefresh) {
            QMessageBox::warning(this, "Parse Error", "Unexpected response format from Lemonade server.");
        }
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray modelsArray = root["data"].toArray();

    if (modelsArray.isEmpty()) {
        LOG_WARNING("No models returned from Lemonade server");
        if (!silentRefresh) {
            QMessageBox::information(this, "No Models", "No models found on Lemonade server.");
        }
        return;
    }

    // Save current selection
    QString currentModel = modelCombo->currentText();

    // Clear and populate model combo box
    modelCombo->clear();

    for (const QJsonValue &value : modelsArray) {
        QJsonObject modelObj = value.toObject();
        QString modelId = modelObj["id"].toString();
        if (!modelId.isEmpty()) {
            modelCombo->addItem(modelId);
        }
    }

    // Restore current model if it exists in the list
    int index = modelCombo->findText(currentModel);
    if (index >= 0) {
        modelCombo->setCurrentIndex(index);
    } else if (!currentModel.isEmpty()) {
        modelCombo->setEditText(currentModel);
    }

    LOG_INFO(QString("Loaded %1 models from Lemonade").arg(modelsArray.size()));

    // Only show success popup for manual refresh
    if (!silentRefresh) {
        QMessageBox::information(this, "Success",
            QString("Found %1 model(s) on the Lemonade server.").arg(modelsArray.size()));
    }
}

void SettingsDialog::onBackendChanged(const QString &backend) {
    // Enable/disable refresh button based on backend
    QString backendLower = backend.toLower();
    bool isOllama = (backendLower == "ollama");
    bool isLemonade = (backendLower == "lemonade");
    bool canRefresh = (isOllama || isLemonade);
    
    refreshModelsButton->setEnabled(canRefresh);
    refreshModelsButton->setToolTip(canRefresh ?
        QString("Fetch available models from %1 server").arg(backend) :
        "Model refresh is only available for Ollama and Lemonade backends");
    
    // Update API URL placeholder based on backend
    if (isLemonade) {
        apiUrlEdit->setPlaceholderText("http://localhost:8000/api/v1/chat/completions");
    } else if (isOllama) {
        apiUrlEdit->setPlaceholderText("http://localhost:11434/api/generate");
    }
}

void SettingsDialog::refreshEmbeddingModels() {
    if (!networkManager) {
        LOG_WARNING("Network manager not initialized yet");
        QMessageBox::warning(this, "Not Ready", "Please wait a moment and try again.");
        return;
    }

    refreshEmbeddingModelsButton->setEnabled(false);
    refreshEmbeddingModelsButton->setText("Loading...");

    // Extract base URL from API URL
    QString apiUrl = apiUrlEdit->text();
    QString baseUrl = apiUrl;

    // Remove /api/generate if present and add /api/tags
    if (baseUrl.endsWith("/api/generate")) {
        baseUrl = baseUrl.left(baseUrl.length() - 13);
    } else if (baseUrl.endsWith("/generate")) {
        baseUrl = baseUrl.left(baseUrl.length() - 9);
    }

    if (!baseUrl.endsWith("/api")) {
        if (baseUrl.endsWith("/")) {
            baseUrl += "api";
        } else {
            baseUrl += "/api";
        }
    }
    baseUrl += "/tags";

    LOG_INFO(QString("Fetching embedding models from: %1").arg(baseUrl));

    QUrl url(baseUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::User, "embedding_models");  // Tag this request

    networkManager->get(request);
}

void SettingsDialog::handleEmbeddingModelsResponse(QNetworkReply *reply) {
    reply->deleteLater();

    // Re-enable refresh button
    refreshEmbeddingModelsButton->setEnabled(true);
    refreshEmbeddingModelsButton->setText("Refresh");

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("Failed to fetch embedding models: %1").arg(reply->errorString());
        LOG_WARNING(errorMsg);
        QMessageBox::warning(this, "Network Error",
            QString("Could not connect to Ollama server:\n%1\n\n"
                    "Make sure Ollama is running and the API URL is correct.").arg(reply->errorString()));
        return;
    }

    // Parse JSON response
    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = QString("Failed to parse response: %1").arg(parseError.errorString());
        LOG_ERROR(errorMsg);
        QMessageBox::warning(this, "Parse Error", "Invalid response from Ollama server.");
        return;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Response is not a JSON object");
        QMessageBox::warning(this, "Parse Error", "Unexpected response format from Ollama server.");
        return;
    }

    QJsonObject jsonObj = doc.object();
    if (!jsonObj.contains("models")) {
        LOG_ERROR("Response does not contain 'models' field");
        QMessageBox::warning(this, "Parse Error", "No models found in response.");
        return;
    }

    QJsonArray modelsArray = jsonObj["models"].toArray();
    if (modelsArray.isEmpty()) {
        QMessageBox::information(this, "No Models",
            "No models found on the Ollama server.\n"
            "You can pull models using: ollama pull <model-name>");
        return;
    }

    // Store current embedding model
    QString currentModel = ragEmbeddingModelCombo->currentText();

    // Clear and populate combo box with all models
    ragEmbeddingModelCombo->clear();

    // Add common embedding models first
    QStringList embeddingModels;
    for (const QJsonValue &modelVal : modelsArray) {
        QJsonObject modelObj = modelVal.toObject();
        QString modelName = modelObj["name"].toString();
        if (!modelName.isEmpty()) {
            // Remove :latest tag suffix for cleaner display
            if (modelName.endsWith(":latest")) {
                modelName = modelName.left(modelName.length() - 7);
            }

            // Prioritize embedding models
            if (modelName.contains("embed", Qt::CaseInsensitive) ||
                modelName.contains("nomic", Qt::CaseInsensitive) ||
                modelName.contains("bge", Qt::CaseInsensitive) ||
                modelName.contains("minilm", Qt::CaseInsensitive)) {
                embeddingModels.prepend(modelName);
            } else {
                embeddingModels.append(modelName);
            }
        }
    }

    // Add all models to combo box
    for (const QString &modelName : embeddingModels) {
        ragEmbeddingModelCombo->addItem(modelName);
    }

    // Restore current model if it exists in the list
    int index = ragEmbeddingModelCombo->findText(currentModel);
    if (index >= 0) {
        ragEmbeddingModelCombo->setCurrentIndex(index);
    } else if (!currentModel.isEmpty()) {
        ragEmbeddingModelCombo->setEditText(currentModel);
    }

    LOG_INFO(QString("Loaded %1 models for embedding selection").arg(modelsArray.size()));
    QMessageBox::information(this, "Success",
        QString("Found %1 model(s) on the Ollama server.\n"
                "Embedding-related models are shown first.").arg(modelsArray.size()));
}

// MCP Server management methods

void SettingsDialog::updateMcpServerList() {
    mcpServerList->clear();

    for (const QJsonValue &serverVal : mcpServers) {
        if (serverVal.isObject()) {
            QJsonObject server = serverVal.toObject();
            QString name = server["name"].toString();
            QString url = server["url"].toString();
            QString type = server["type"].toString();
            bool enabled = server["enabled"].toBool();

            QString displayText = QString("%1 (%2) - %3")
                .arg(name, type.toUpper(), url);

            if (!enabled) {
                displayText += " [DISABLED]";
            }

            mcpServerList->addItem(displayText);
        }
    }

    LOG_DEBUG(QString("Updated MCP server list with %1 servers").arg(mcpServers.size()));
}

void SettingsDialog::addMcpServer() {
    // Simple dialog for now - will create proper dialog later
    bool ok;
    QString name = QInputDialog::getText(this, "Add MCP Server",
                                         "Server Name:", QLineEdit::Normal,
                                         "", &ok);
    if (!ok || name.isEmpty()) {
        return;
    }

    QString url = QInputDialog::getText(this, "Add MCP Server",
                                       "Server URL:", QLineEdit::Normal,
                                        "http://localhost:8080", &ok);
    if (!ok || url.isEmpty()) {
        return;
    }

    QStringList types;
    types << "http" << "sse";
    QString type = QInputDialog::getItem(this, "Add MCP Server",
                                         "Connection Type:", types,
                                         0, false, &ok);
    if (!ok) {
        return;
    }

    // Create new server object
    QJsonObject newServer;
    newServer["name"] = name;
    newServer["url"] = url;
    newServer["type"] = type;
    newServer["enabled"] = true;

    // Add to array
    mcpServers.append(newServer);

    // Update display
    updateMcpServerList();

    LOG_INFO(QString("Added MCP server: %1 (%2)").arg(name, url));
}

void SettingsDialog::editMcpServer() {
    int currentRow = mcpServerList->currentRow();
    if (currentRow < 0 || currentRow >= mcpServers.size()) {
        return;
    }

    QJsonObject server = mcpServers[currentRow].toObject();

    bool ok;
    QString name = QInputDialog::getText(this, "Edit MCP Server",
                                         "Server Name:", QLineEdit::Normal,
                                         server["name"].toString(), &ok);
    if (!ok) {
        return;
    }

    QString url = QInputDialog::getText(this, "Edit MCP Server",
                                       "Server URL:", QLineEdit::Normal,
                                        server["url"].toString(), &ok);
    if (!ok) {
        return;
    }

    QStringList types;
    types << "http" << "sse";
    int currentType = types.indexOf(server["type"].toString());
    QString type = QInputDialog::getItem(this, "Edit MCP Server",
                                         "Connection Type:", types,
                                         currentType, false, &ok);
    if (!ok) {
        return;
    }

    // Update server object
    server["name"] = name;
    server["url"] = url;
    server["type"] = type;

    // Update in array
    mcpServers[currentRow] = server;

    // Update display
    updateMcpServerList();
    mcpServerList->setCurrentRow(currentRow);

    LOG_INFO(QString("Updated MCP server: %1").arg(name));
}

void SettingsDialog::deleteMcpServer() {
    int currentRow = mcpServerList->currentRow();
    if (currentRow < 0 || currentRow >= mcpServers.size()) {
        return;
    }

    QJsonObject server = mcpServers[currentRow].toObject();
    QString name = server["name"].toString();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete MCP Server",
                                   QString("Delete server '%1'?").arg(name),
                                   QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        mcpServers.removeAt(currentRow);
        updateMcpServerList();
        LOG_INFO(QString("Deleted MCP server: %1").arg(name));
    }
}

void SettingsDialog::toggleMcpServerEnabled() {
    int currentRow = mcpServerList->currentRow();
    if (currentRow < 0 || currentRow >= mcpServers.size()) {
        return;
    }

    QJsonObject server = mcpServers[currentRow].toObject();
    bool enabled = server["enabled"].toBool();
    server["enabled"] = !enabled;

    mcpServers[currentRow] = server;

    updateMcpServerList();
    mcpServerList->setCurrentRow(currentRow);

    LOG_INFO(QString("Toggled MCP server '%1': %2").arg(server["name"].toString(), !enabled ? "enabled" : "disabled"));
}

void SettingsDialog::onMcpServerSelectionChanged() {
    bool hasSelection = mcpServerList->currentRow() >= 0;
    editMcpServerButton->setEnabled(hasSelection);
    deleteMcpServerButton->setEnabled(hasSelection);
    toggleMcpServerButton->setEnabled(hasSelection);
}
