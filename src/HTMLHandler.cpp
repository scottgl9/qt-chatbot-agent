/**
 * HTMLHandler.cpp - HTML generation utilities
 * 
 * Provides HTML generation for messages and UI elements including tool call
 * widgets (pending/success/error), message formatting, and consistent styling.
 */

#include "HTMLHandler.h"

QString HTMLHandler::formatUserMessage(const QString &message, const QString &timestamp) {
    // User message - left-aligned with blue accent
    // Using table for reliable left alignment in Qt's limited HTML renderer
    return QString(
        "<table width='70%%' cellpadding='0' cellspacing='0' style='margin: 4px 0;'>"
        "<tr><td style='background-color: #E3F2FD; padding: 8px 14px; border-radius: 12px;'>"
        "<b style='color: #1565C0; font-size: 9pt;'>You</b> "
        "<span style='color: #888; font-size: 8pt;'>%1</span><br>"
        "<span style='color: #000; font-size: 10pt;'>%2</span>"
        "</td></tr>"
        "</table>")
        .arg(timestamp, message);
}

QString HTMLHandler::formatBotMessage(const QString &message, const QString &timestamp) {
    // Bot message - right-aligned with light background
    // Using table for reliable right alignment in Qt's limited HTML renderer
    return QString(
        "<table width='100%%' cellpadding='0' cellspacing='0' style='margin: 4px 0;'>"
        "<tr><td width='30%%'></td>"
        "<td width='70%%' style='background-color: #F5F5F5; padding: 8px 14px; border-radius: 12px;'>"
        "<b style='color: #2196F3; font-size: 9pt;'>Assistant</b> "
        "<span style='color: #888; font-size: 8pt;'>%1</span><br>"
        "<span style='color: #000; font-size: 10pt;'>%2</span>"
        "</td></tr>"
        "</table>")
        .arg(timestamp, message);
}

QString HTMLHandler::formatSystemMessage(const QString &message, const QString &timestamp) {
    // System message - subtle and minimal
    return QString(
        "<p style='margin: 6px 0; text-align: center; color: #666; font-size: 9pt;'>"
        "<i>%1</i> <span style='font-size: 8pt; color: #999;'>%2</span>"
        "</p>")
        .arg(message, timestamp);
}

QString HTMLHandler::createToolCallWidget(const QString &toolName) {
    // Tool call indicator with bubble widget
    // Qt's QTextEdit has limited CSS support, so use a centered paragraph with table-like styling
    return QString(
        "<table width='100%%' cellpadding='0' cellspacing='0' style='margin: 8px 0;'>"
        "<tr><td align='center'>"
        "<span style='padding: 10px 16px; background-color: #FFF3E0; "
        "border: 2px solid #FF9800; border-radius: 16px; color: #E65100; "
        "font-size: 10pt; font-weight: bold;'>"
        "Calling Tool: %1"
        "</span>"
        "</td></tr>"
        "</table>")
        .arg(toolName);
}

QString HTMLHandler::createToolSuccessWidget(const QString &toolName, const QString &result) {
    // Simple success widget
    QString html = QString(
        "<p style='margin: 8px 20px; padding: 10px; background-color: #E8F5E9; "
        "border-left: 4px solid #4CAF50; border-radius: 8px;'>"
        "<b style='color: #2E7D32; font-size: 10pt;'>✓ Tool Completed: %1</b>")
        .arg(toolName);

    if (!result.isEmpty()) {
        html += QString("<br><span style='color: #388E3C; font-size: 9pt;'>%1</span>")
                .arg(result);
    }

    html += "</p>";
    return html;
}

QString HTMLHandler::createToolErrorWidget(const QString &toolName, const QString &error) {
    // Simple error widget
    return QString(
        "<p style='margin: 8px 20px; padding: 10px; background-color: #FFEBEE; "
        "border-left: 4px solid #F44336; border-radius: 8px;'>"
        "<b style='color: #D32F2F; font-size: 10pt;'>✗ Tool Failed: %1</b><br>"
        "<span style='color: #C62828; font-size: 9pt;'>%2</span>"
        "</p>")
        .arg(toolName, error);
}
