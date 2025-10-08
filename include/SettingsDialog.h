/**
 * SettingsDialog.h - Application settings dialog
 * 
 * Provides UI for configuring LLM backend, model selection, API URLs,
 * RAG parameters, MCP servers, and context window settings.
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    /**
     * @brief Emitted when settings are saved successfully
     * This allows the main window to refresh MCP tools if needed
     */
    void settingsSaved();

private slots:
    void saveSettings();
    void cancelSettings();
    void resetToDefaults();
    void refreshModels();
    void refreshEmbeddingModels();
    void handleModelsResponse(QNetworkReply *reply);
    void handleEmbeddingModelsResponse(QNetworkReply *reply);
    void onBackendChanged(const QString &backend);

    // MCP server management
    void addMcpServer();
    void editMcpServer();
    void deleteMcpServer();
    void toggleMcpServerEnabled();
    void onMcpServerSelectionChanged();

private:
    void createUI();
    void loadCurrentSettings();
    void fetchOllamaModels(bool silentMode = false);
    void updateMcpServerList();

    // Backend settings
    QComboBox *backendCombo;
    QLineEdit *apiUrlEdit;
    QLineEdit *apiKeyEdit;
    QComboBox *modelCombo;
    QPushButton *refreshModelsButton;
    QTextEdit *systemPromptEdit;

    // LLM parameter controls
    QCheckBox *overrideContextWindowCheckbox;
    QSpinBox *contextWindowSpinBox;
    QCheckBox *overrideTemperatureCheckbox;
    QDoubleSpinBox *temperatureSpinBox;
    QCheckBox *overrideTopPCheckbox;
    QDoubleSpinBox *topPSpinBox;
    QCheckBox *overrideTopKCheckbox;
    QSpinBox *topKSpinBox;
    QCheckBox *overrideMaxTokensCheckbox;
    QSpinBox *maxTokensSpinBox;

    // RAG settings
    QCheckBox *ragEnabledCheckbox;
    QComboBox *ragEmbeddingModelCombo;
    QPushButton *refreshEmbeddingModelsButton;
    QSpinBox *ragChunkSizeSpinBox;
    QSpinBox *ragChunkOverlapSpinBox;
    QSpinBox *ragTopKSpinBox;

    // MCP server settings
    QListWidget *mcpServerList;
    QPushButton *addMcpServerButton;
    QPushButton *editMcpServerButton;
    QPushButton *deleteMcpServerButton;
    QPushButton *toggleMcpServerButton;
    QJsonArray mcpServers;  // Local copy of server configurations

    // Buttons
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QPushButton *resetButton;

    // Network
    QNetworkAccessManager *networkManager;
    bool silentRefresh; // Don't show success popup for auto-refresh
};

#endif // SETTINGSDIALOG_H
