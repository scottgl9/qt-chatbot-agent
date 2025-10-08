/**
 * HTMLHandler.h - HTML generation utilities
 * 
 * Static utility class for generating HTML widgets (tool calls, messages)
 * with consistent styling for the chat interface.
 */

#ifndef HTMLHANDLER_H
#define HTMLHANDLER_H

#include <QString>

/**
 * @class HTMLHandler
 * @brief Handles HTML message formatting for chat display
 *
 * This class provides utilities for formatting chat messages in HTML
 * suitable for display in Qt's QTextEdit widget. It handles:
 * - User messages (left-aligned with blue accent)
 * - Bot messages (right-aligned with light background)
 * - System messages (centered, subtle styling)
 * - Tool call indicators (centered widget with border)
 * - Tool success/error widgets
 */
class HTMLHandler {
public:
    /**
     * @brief Format a user message with timestamp
     * @param message The message text
     * @param timestamp The timestamp string (e.g., "14:30:00")
     * @return HTML-formatted user message
     */
    static QString formatUserMessage(const QString &message, const QString &timestamp);

    /**
     * @brief Format a bot/assistant message with timestamp
     * @param message The message text
     * @param timestamp The timestamp string (e.g., "14:30:00")
     * @return HTML-formatted bot message
     */
    static QString formatBotMessage(const QString &message, const QString &timestamp);

    /**
     * @brief Format a system message with timestamp
     * @param message The message text
     * @param timestamp The timestamp string (e.g., "14:30:00")
     * @return HTML-formatted system message
     */
    static QString formatSystemMessage(const QString &message, const QString &timestamp);

    /**
     * @brief Create tool call indicator widget
     * @param toolName The name of the tool being called
     * @return HTML widget for tool call indicator
     */
    static QString createToolCallWidget(const QString &toolName);

    /**
     * @brief Create tool success widget
     * @param toolName The name of the completed tool
     * @param result The result summary (optional)
     * @return HTML widget for tool success
     */
    static QString createToolSuccessWidget(const QString &toolName, const QString &result = QString());

    /**
     * @brief Create tool error widget
     * @param toolName The name of the failed tool
     * @param error The error message
     * @return HTML widget for tool error
     */
    static QString createToolErrorWidget(const QString &toolName, const QString &error);
};

#endif // HTMLHANDLER_H
