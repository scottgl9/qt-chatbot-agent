/**
 * MCPHandler.h - Model Context Protocol tool manager
 * 
 * Manages MCP tool registration (local and networked), execution, server
 * discovery, and result formatting. Supports HTTP and local function tools.
 */

#ifndef MCPHANDLER_H
#define MCPHANDLER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QNetworkAccessManager>
#include <functional>
#include "SSEClient.h"

/**
 * @brief MCP (Model Context Protocol) Message structure
 *
 * Represents a standardized message format for LLM interactions with tools.
 */
struct MCPMessage {
    QString role;           // "user", "assistant", "system", or "tool"
    QString content;        // The message content
    QJsonObject context;    // Additional context (e.g., tool names, parameters)
    QString toolCallId;     // Optional: ID for tool call tracking
    QString toolName;       // Optional: Name of tool being called/responding

    // Convert to/from JSON
    QJsonObject toJson() const;
    static MCPMessage fromJson(const QJsonObject &json);
};

/**
 * @brief Tool function signature
 *
 * Tools are callbacks that take JSON parameters and return JSON results.
 */
using MCPToolFunction = std::function<QJsonObject(const QJsonObject &params)>;

/**
 * @brief Tool type enumeration
 */
enum class MCPToolType {
    Local,      // Local function callback
    HTTP,       // HTTP endpoint
    SSE         // Server-Sent Events stream
};

/**
 * @brief Tool definition structure
 */
struct MCPTool {
    QString name;                   // Tool name (e.g., "search", "summarize")
    QString description;            // Human-readable description
    QJsonObject parameters;         // JSON schema for parameters
    MCPToolFunction function;       // The actual tool implementation (for Local type)
    MCPToolType toolType;           // Type of tool
    QString networkUrl;             // URL for HTTP/SSE tools
    bool isLocal;                   // Deprecated: use toolType instead (kept for compatibility)

    MCPTool() : toolType(MCPToolType::Local), isLocal(true) {}

    // Validation
    bool isValid() const;
};

/**
 * @brief MCPHandler - Manages Model Context Protocol tools and message routing
 *
 * This class implements the MCP specification for tool calling and context management.
 * It allows registration of tools (local functions or networked services) and handles
 * dispatch of tool calls and routing of responses.
 */
class MCPHandler : public QObject {
    Q_OBJECT

public:
    explicit MCPHandler(QObject *parent = nullptr);
    ~MCPHandler();

    // Tool Registration
    /**
     * @brief Register a local tool (function callback)
     * @param tool The tool definition with function callback
     * @return true if registration successful, false otherwise
     */
    bool registerTool(const MCPTool &tool);

    /**
     * @brief Register a networked tool (HTTP endpoint)
     * @param name Tool name
     * @param description Tool description
     * @param parameters JSON schema for parameters
     * @param networkUrl HTTP endpoint URL
     * @return true if registration successful, false otherwise
     */
    bool registerNetworkedTool(const QString &name,
                               const QString &description,
                               const QJsonObject &parameters,
                               const QString &networkUrl);

    /**
     * @brief Unregister a tool by name
     * @param name Tool name
     * @return true if tool was found and removed, false otherwise
     */
    bool unregisterTool(const QString &name);

    /**
     * @brief Clear all networked tools (HTTP and SSE), keeping local tools
     * @return Number of tools cleared
     */
    int clearNetworkedTools();

    /**
     * @brief Discover and register tools from an MCP server
     * @param serverName Name of the MCP server (for logging)
     * @param serverUrl Base URL of the MCP server
     * @param serverType Type of server ("http" or "sse")
     * @return Number of tools discovered and registered, -1 on error
     */
    int discoverAndRegisterServerTools(const QString &serverName,
                                       const QString &serverUrl,
                                       const QString &serverType = "http");

    /**
     * @brief Get list of registered tool names
     * @return QStringList of tool names
     */
    QStringList getRegisteredTools() const;

    /**
     * @brief Get tool definition by name
     * @param name Tool name
     * @return Pointer to tool definition, or nullptr if not found
     */
    const MCPTool* getTool(const QString &name) const;

    // Message Processing
    /**
     * @brief Process an MCP message and extract tool calls
     * @param message The MCP message
     * @return List of tool names requested in the message context
     */
    QStringList extractToolCalls(const MCPMessage &message) const;

    /**
     * @brief Execute a tool call
     * @param toolName Name of the tool to execute
     * @param parameters JSON parameters for the tool
     * @return Tool call ID for tracking the async response
     */
    QString executeToolCall(const QString &toolName, const QJsonObject &parameters);

    /**
     * @brief Generate MCP tools list for LLM (JSON format)
     * @return JSON array of tool definitions for the LLM
     */
    QJsonArray getToolsForLLM() const;

    /**
     * @brief Generate OpenAI/Ollama native format tools list
     * @return JSON array of tool definitions in OpenAI format
     */
    QJsonArray getToolsForLLMNative() const;

    /**
     * @brief Build an MCP message with tool context
     * @param role Message role
     * @param content Message content
     * @param toolNames Optional list of tools to include in context
     * @return MCPMessage object
     */
    MCPMessage buildMessage(const QString &role,
                           const QString &content,
                           const QStringList &toolNames = QStringList()) const;

signals:
    /**
     * @brief Emitted when a tool call completes successfully
     * @param toolCallId The tool call ID
     * @param toolName Name of the tool
     * @param result JSON result from the tool
     */
    void toolCallCompleted(const QString &toolCallId,
                          const QString &toolName,
                          const QJsonObject &result);

    /**
     * @brief Emitted when a tool call fails
     * @param toolCallId The tool call ID
     * @param toolName Name of the tool
     * @param error Error message
     */
    void toolCallFailed(const QString &toolCallId,
                       const QString &toolName,
                       const QString &error);

private slots:
    void handleNetworkToolResponse();
    void handleSSEEvent(const SSEClient::SSEEvent &event);
    void handleSSEConnected(const QString &url);
    void handleSSEDisconnected();
    void handleSSEError(const QString &error);

private:
    // Execute a local tool
    void executeLocalTool(const QString &toolCallId,
                         const QString &toolName,
                         const QJsonObject &parameters);

    // Execute a networked tool (HTTP)
    void executeNetworkedTool(const QString &toolCallId,
                             const QString &toolName,
                             const QJsonObject &parameters);

    // Execute an SSE tool
    void executeSSETool(const QString &toolCallId,
                       const QString &toolName,
                       const QJsonObject &parameters);

    // Generate unique tool call ID
    QString generateToolCallId() const;

    QMap<QString, MCPTool> m_tools;              // Registered tools
    QNetworkAccessManager *m_networkManager;      // For HTTP tools
    QMap<QString, QString> m_pendingCalls;        // Track pending tool calls
    QMap<QString, SSEClient*> m_sseClients;       // Map tool name -> SSE client
    QMap<SSEClient*, QString> m_sseToolCalls;     // Map SSE client -> tool call ID
    int m_toolCallCounter;                        // For generating unique IDs
};

#endif // MCPHANDLER_H
