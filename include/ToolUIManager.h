/**
 * ToolUIManager.h - Tool selection and management UI manager
 * 
 * Provides dialogs for viewing and filtering MCP tools, tracks enabled/disabled
 * state, and generates filtered tool arrays for LLM requests.
 */

#ifndef TOOLUIMANAGER_H
#define TOOLUIMANAGER_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QJsonArray>

class QWidget;
class MCPHandler;

/**
 * @brief Manages tool-related UI operations
 * 
 * Handles all tool management UI including:
 * - Tool management dialog (enable/disable individual tools)
 * - Tool filtering based on enabled/disabled state
 * - Tool enable/disable tracking
 * - Tool count and status information
 */
class ToolUIManager : public QObject {
    Q_OBJECT

public:
    explicit ToolUIManager(MCPHandler *mcpHandler, QWidget *parent = nullptr);
    ~ToolUIManager() override = default;

    // Tool management dialog
    void showToolsDialog();

    // Tool filtering
    QJsonArray getEnabledTools() const;
    
    // Tool state
    bool isToolEnabled(const QString &toolName) const;
    void setToolEnabled(const QString &toolName, bool enabled);
    int getEnabledToolCount() const;
    int getDisabledToolCount() const;
    int getTotalToolCount() const;

    // Disable all tools (for global toggle)
    void enableAllTools();
    void disableAllTools();

signals:
    void toolStateChanged(const QString &toolName, bool enabled);
    void toolCountChanged(int enabled, int disabled, int total);

private:
    MCPHandler *mcpHandler;
    QWidget *parentWidget;
    QSet<QString> disabledTools;  // Track individually disabled tools
};

#endif // TOOLUIMANAGER_H
