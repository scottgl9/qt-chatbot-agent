/**
 * MarkdownHandler.h - Markdown parsing and HTML conversion
 * 
 * Converts markdown text to HTML with support for bold, italic, code blocks,
 * syntax highlighting, and lists. Static utility class.
 */

#ifndef MARKDOWNHANDLER_H
#define MARKDOWNHANDLER_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>

/**
 * @class MarkdownHandler
 * @brief Handles conversion of Markdown text to HTML
 *
 * This class provides utilities for converting Markdown-formatted text
 * into HTML suitable for display in Qt widgets. It supports:
 * - Code blocks (```) and inline code (`)
 * - Headers (# ## ###)
 * - Bold (**text**), Italic (*text*), Strikethrough (~~text~~)
 * - Links ([text](url))
 * - Bullet and numbered lists
 * - Blockquotes (> text)
 * - Horizontal rules (---, ***, ___)
 * - Tables (| col | col |)
 */
class MarkdownHandler {
public:
    /**
     * @brief Convert Markdown text to HTML
     * @param text The Markdown-formatted text
     * @return HTML-formatted text
     */
    static QString toHtml(const QString &text);

    /**
     * @brief Convert Markdown tables to HTML tables
     * @param text The text containing potential Markdown tables
     * @return Text with Markdown tables converted to HTML tables
     */
    static QString convertTables(const QString &text);

private:
    /**
     * @brief Parse a single table row
     * @param row The table row string (e.g., "| col1 | col2 |")
     * @return List of cell contents
     */
    static QStringList parseTableRow(const QString &row);

    /**
     * @brief Parse table alignment from separator row
     * @param separator The separator row (e.g., "| :--- | :---: | ---: |")
     * @return List of alignment values ("left", "center", "right")
     */
    static QList<QString> parseTableAlignment(const QString &separator);

    /**
     * @brief Build HTML table from Markdown table lines
     * @param tableLines The list of table lines (header, separator, rows)
     * @return HTML table string
     */
    static QString buildHtmlTable(const QStringList &tableLines);
};

#endif // MARKDOWNHANDLER_H
