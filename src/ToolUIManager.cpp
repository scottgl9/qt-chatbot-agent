/**
 * ToolUIManager.cpp - Tool selection and management UI
 * 
 * Displays available tools dialog, enables/disables tool filtering,
 * shows tool parameters and descriptions, and tracks tool state.
 */

#include "ToolUIManager.h"
#include "MCPHandler.h"
#include "Logger.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QTextEdit>
#include <QFont>
#include <QJsonObject>
#include <QJsonDocument>

ToolUIManager::ToolUIManager(MCPHandler *mcpHandler, QWidget *parent)
    : QObject(parent)
    , mcpHandler(mcpHandler)
    , parentWidget(parent)
    , disabledTools() {
}

void ToolUIManager::showToolsDialog() {
    QDialog *dialog = new QDialog(parentWidget);
    dialog->setWindowTitle("Manage Tools");
    dialog->setMinimumWidth(750);
    dialog->setMinimumHeight(550);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    // Header
    QLabel *headerLabel = new QLabel("Tool Management", dialog);
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    layout->addWidget(headerLabel);

    // Info label
    QLabel *infoLabel = new QLabel("Enable or disable individual tools. Disabled tools will not be available to the LLM.", dialog);
    infoLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // Tool count
    int toolCount = getTotalToolCount();
    int enabledCount = getEnabledToolCount();
    int disabledCount = getDisabledToolCount();
    
    QLabel *countLabel = new QLabel(QString("Tools: %1 enabled, %2 disabled, %3 total")
        .arg(enabledCount).arg(disabledCount).arg(toolCount), dialog);
    countLabel->setStyleSheet("color: #666; margin-bottom: 10px; font-weight: bold;");
    layout->addWidget(countLabel);

    // Quick actions
    QHBoxLayout *quickActionsLayout = new QHBoxLayout();
    QPushButton *enableAllBtn = new QPushButton("Enable All", dialog);
    QPushButton *disableAllBtn = new QPushButton("Disable All", dialog);
    quickActionsLayout->addWidget(enableAllBtn);
    quickActionsLayout->addWidget(disableAllBtn);
    quickActionsLayout->addStretch();
    layout->addLayout(quickActionsLayout);

    // Scrollable area for tools
    QScrollArea *scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);

    // Store checkboxes for quick actions
    QList<QCheckBox*> toolCheckboxes;

    if (toolCount == 0) {
        QLabel *noToolsLabel = new QLabel("No MCP tools are currently registered.", scrollWidget);
        noToolsLabel->setAlignment(Qt::AlignCenter);
        noToolsLabel->setStyleSheet("color: #999; padding: 40px; font-size: 12pt;");
        scrollLayout->addWidget(noToolsLabel);
    } else {
        // Get tool details from MCP handler
        QJsonArray tools = mcpHandler->getToolsForLLM();

        for (const QJsonValue &toolVal : tools) {
            QJsonObject tool = toolVal.toObject();
            QString toolName = tool["name"].toString();
            QString toolDesc = tool["description"].toString();
            QJsonObject params = tool["parameters"].toObject();

            // Create a group box for each tool with checkbox in title
            QGroupBox *toolBox = new QGroupBox(scrollWidget);
            toolBox->setCheckable(false);
            QVBoxLayout *toolLayout = new QVBoxLayout(toolBox);

            // Checkbox and tool name at the top
            QHBoxLayout *titleLayout = new QHBoxLayout();
            QCheckBox *enableCheckbox = new QCheckBox(toolName, toolBox);
            enableCheckbox->setChecked(!disabledTools.contains(toolName));
            QFont titleFont = enableCheckbox->font();
            titleFont.setBold(true);
            titleFont.setPointSize(11);
            enableCheckbox->setFont(titleFont);

            // Connect checkbox to update disabled tools set
            connect(enableCheckbox, &QCheckBox::toggled, [this, toolName, countLabel](bool checked) {
                if (checked) {
                    disabledTools.remove(toolName);
                    LOG_INFO(QString("Enabled tool: %1").arg(toolName));
                    emit toolStateChanged(toolName, true);
                } else {
                    disabledTools.insert(toolName);
                    LOG_INFO(QString("Disabled tool: %1").arg(toolName));
                    emit toolStateChanged(toolName, false);
                }
                // Update count label
                int total = getTotalToolCount();
                int enabled = getEnabledToolCount();
                int disabled = getDisabledToolCount();
                countLabel->setText(QString("Tools: %1 enabled, %2 disabled, %3 total")
                    .arg(enabled).arg(disabled).arg(total));
                emit toolCountChanged(enabled, disabled, total);
            });

            toolCheckboxes.append(enableCheckbox);
            titleLayout->addWidget(enableCheckbox);
            titleLayout->addStretch();
            toolLayout->addLayout(titleLayout);

            // Description
            QLabel *descLabel = new QLabel(QString("<b>Description:</b> %1").arg(toolDesc), toolBox);
            descLabel->setWordWrap(true);
            descLabel->setTextFormat(Qt::RichText);
            descLabel->setStyleSheet("margin-left: 20px;");
            toolLayout->addWidget(descLabel);

            // Parameters section (collapsed by default)
            QLabel *paramsHeaderLabel = new QLabel("<b>Parameters:</b>", toolBox);
            paramsHeaderLabel->setStyleSheet("margin-left: 20px;");
            toolLayout->addWidget(paramsHeaderLabel);

            // Format parameters as JSON
            QString paramsJson = QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Indented));
            QTextEdit *paramsText = new QTextEdit(toolBox);
            paramsText->setPlainText(paramsJson);
            paramsText->setReadOnly(true);
            paramsText->setMaximumHeight(120);
            paramsText->setStyleSheet("background-color: #f5f5f5; font-family: monospace; font-size: 9pt; margin-left: 20px;");
            toolLayout->addWidget(paramsText);

            // Type info
            if (tool.contains("url") && !tool["url"].toString().isEmpty()) {
                QLabel *typeLabel = new QLabel(QString("Type: <span style='color: #2196F3;'>Networked</span> | URL: <code>%1</code>")
                    .arg(tool["url"].toString()), toolBox);
                typeLabel->setTextFormat(Qt::RichText);
                typeLabel->setWordWrap(true);
                typeLabel->setStyleSheet("color: #666; font-size: 9pt; margin-top: 5px; margin-left: 20px;");
                toolLayout->addWidget(typeLabel);
            } else {
                QLabel *typeLabel = new QLabel("Type: <span style='color: #4CAF50;'>Local</span>", toolBox);
                typeLabel->setTextFormat(Qt::RichText);
                typeLabel->setStyleSheet("color: #666; font-size: 9pt; margin-top: 5px; margin-left: 20px;");
                toolLayout->addWidget(typeLabel);
            }

            scrollLayout->addWidget(toolBox);
        }
    }

    // Connect quick action buttons
    connect(enableAllBtn, &QPushButton::clicked, [toolCheckboxes]() {
        for (QCheckBox *cb : toolCheckboxes) {
            cb->setChecked(true);
        }
    });

    connect(disableAllBtn, &QPushButton::clicked, [toolCheckboxes]() {
        for (QCheckBox *cb : toolCheckboxes) {
            cb->setChecked(false);
        }
    });

    scrollLayout->addStretch();
    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    layout->addWidget(scrollArea);

    // Close button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->exec();
    dialog->deleteLater();
}

QJsonArray ToolUIManager::getEnabledTools() const {
    if (!mcpHandler) {
        return QJsonArray();
    }

    QJsonArray allTools = mcpHandler->getToolsForLLM();

    // If no tools are disabled, return all tools
    if (disabledTools.isEmpty()) {
        return allTools;
    }

    // Filter out disabled tools
    QJsonArray enabledTools;
    for (const QJsonValue &toolVal : allTools) {
        QJsonObject tool = toolVal.toObject();
        QString toolName = tool["name"].toString();
        if (!disabledTools.contains(toolName)) {
            enabledTools.append(toolVal);
        }
    }

    LOG_DEBUG(QString("Filtered tools: %1 enabled out of %2 total")
        .arg(enabledTools.size()).arg(allTools.size()));

    return enabledTools;
}

bool ToolUIManager::isToolEnabled(const QString &toolName) const {
    return !disabledTools.contains(toolName);
}

void ToolUIManager::setToolEnabled(const QString &toolName, bool enabled) {
    if (enabled) {
        if (disabledTools.remove(toolName)) {
            emit toolStateChanged(toolName, true);
        }
    } else {
        if (!disabledTools.contains(toolName)) {
            disabledTools.insert(toolName);
            emit toolStateChanged(toolName, false);
        }
    }
}

int ToolUIManager::getEnabledToolCount() const {
    if (!mcpHandler) {
        return 0;
    }
    return getTotalToolCount() - disabledTools.size();
}

int ToolUIManager::getDisabledToolCount() const {
    return disabledTools.size();
}

int ToolUIManager::getTotalToolCount() const {
    if (!mcpHandler) {
        return 0;
    }
    return mcpHandler->getRegisteredTools().size();
}

void ToolUIManager::enableAllTools() {
    disabledTools.clear();
    LOG_INFO("All tools enabled");
}

void ToolUIManager::disableAllTools() {
    if (!mcpHandler) {
        return;
    }
    QStringList allTools = mcpHandler->getRegisteredTools();
    for (const QString &toolName : allTools) {
        disabledTools.insert(toolName);
    }
    LOG_INFO("All tools disabled");
}
