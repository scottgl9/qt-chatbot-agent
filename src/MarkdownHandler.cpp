/**
 * MarkdownHandler.cpp - Markdown parsing and rendering
 * 
 * Converts markdown text to HTML with support for bold, italic, code blocks,
 * syntax highlighting, and lists (ordered and unordered).
 */

#include "MarkdownHandler.h"
#include "Logger.h"

QString MarkdownHandler::toHtml(const QString &text) {
    QString formatted = text;

    // Process tables BEFORE converting newlines to <br>
    formatted = convertTables(formatted);

    // Convert newlines to <br> for proper line breaks
    formatted.replace("\n", "<br>");

    // Process in specific order to avoid conflicts

    // 1. Code blocks (```lang\ncode\n```) - process first to avoid conflicts
    QRegularExpression codeBlockPattern("```[^\\n]*<br>([^`]+)```");
    formatted.replace(codeBlockPattern,
        "<pre style='background-color: #f5f5f5; padding: 10px; border-radius: 4px; "
        "font-family: \"Consolas\", \"Monaco\", monospace; margin: 8px 0; overflow-x: auto;'><code>\\1</code></pre>");

    // Single-line code blocks ```code```
    QRegularExpression codeBlockSinglePattern("```([^`<]+)```");
    formatted.replace(codeBlockSinglePattern,
        "<pre style='background-color: #f5f5f5; padding: 10px; border-radius: 4px; "
        "font-family: \"Consolas\", \"Monaco\", monospace; margin: 8px 0;'><code>\\1</code></pre>");

    // 2. Inline code (`code`) - escape underscores to prevent italic interpretation
    QRegularExpression inlineCodePattern("`([^`]+)`");
    QRegularExpressionMatchIterator it = inlineCodePattern.globalMatch(formatted);
    QString result;
    int lastPos = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        result += formatted.mid(lastPos, match.capturedStart() - lastPos);
        QString codeContent = match.captured(1);
        // Escape underscores in code content to prevent italic pattern matching
        codeContent.replace("_", "&#95;");
        result += QString("<code style='background-color: #f5f5f5; padding: 2px 5px; border-radius: 3px; "
                         "font-family: \"Consolas\", \"Monaco\", monospace; color: #d63384; font-size: 0.9em;'>%1</code>")
                  .arg(codeContent);
        lastPos = match.capturedEnd();
    }
    result += formatted.mid(lastPos);
    formatted = result;

    // 3. Headers (# Header, ## Header, etc.)
    QRegularExpression h1Pattern("(^|<br>)#\\s+(.+?)(?=<br>|$)");
    formatted.replace(h1Pattern, "\\1<h1 style='font-size: 1.5em; font-weight: bold; margin: 12px 0 6px 0;'>\\2</h1><br>");

    QRegularExpression h2Pattern("(^|<br>)##\\s+(.+?)(?=<br>|$)");
    formatted.replace(h2Pattern, "\\1<h2 style='font-size: 1.3em; font-weight: bold; margin: 10px 0 5px 0;'>\\2</h2><br>");

    QRegularExpression h3Pattern("(^|<br>)###\\s+(.+?)(?=<br>|$)");
    formatted.replace(h3Pattern, "\\1<h3 style='font-size: 1.1em; font-weight: bold; margin: 8px 0 4px 0;'>\\2</h3><br>");

    // 4. Blockquotes (> text)
    QRegularExpression blockquotePattern("(^|<br>)&gt;\\s+(.+?)(?=<br>|$)");
    formatted.replace(blockquotePattern,
        "\\1<div style='border-left: 4px solid #ddd; padding-left: 12px; margin: 6px 0; color: #666; font-style: italic;'>\\2</div>");

    // 5. Links ([text](url))
    QRegularExpression linkPattern("\\[([^\\]]+)\\]\\(([^\\)]+)\\)");
    formatted.replace(linkPattern,
        "<a href='\\2' style='color: #2196F3; text-decoration: none;'>\\1</a>");

    // 6. Bold text (**text** or __text__)
    QRegularExpression boldPattern("\\*\\*([^*]+)\\*\\*|__([^_]+)__");
    formatted.replace(boldPattern, "<b>\\1\\2</b>");

    // 7. Italic text (*text* or _text_) - but not if already part of **
    QRegularExpression italicAsterisk("(?<!\\*)\\*([^*]+)\\*(?!\\*)");
    formatted.replace(italicAsterisk, "<i>\\1</i>");

    QRegularExpression italicUnderscore("(?<!_)_([^_]+)_(?!_)");
    formatted.replace(italicUnderscore, "<i>\\1</i>");

    // 8. Strikethrough (~~text~~)
    QRegularExpression strikePattern("~~([^~]+)~~");
    formatted.replace(strikePattern, "<s style='color: #999;'>\\1</s>");

    // 9. Horizontal rules (---, ***, ___)
    QRegularExpression hrPattern("(^|<br>)(-{3,}|\\*{3,}|_{3,})(?=<br>|$)");
    formatted.replace(hrPattern, "\\1<hr style='border: none; border-top: 1px solid #ddd; margin: 12px 0;'>");

    // 10. Bullet lists (- item or * item)
    QRegularExpression bulletPattern("(^|<br>)\\s*[-*]\\s+(.+?)(?=<br>|$)");
    formatted.replace(bulletPattern, "\\1<div style='margin-left: 20px;'>• \\2</div>");

    // 11. Numbered lists (1. item)
    QRegularExpression numberedPattern("(^|<br>)\\s*(\\d+)\\.\\s+(.+?)(?=<br>|$)");
    formatted.replace(numberedPattern, "\\1<div style='margin-left: 20px;'>\\2. \\3</div>");

    return formatted;
}

QString MarkdownHandler::convertTables(const QString &text) {
    QString result = text;
    QStringList lines = text.split('\n');

    int i = 0;
    while (i < lines.size()) {
        QString line = lines[i].trimmed();

        // Check if this line looks like a table header (contains |)
        if (line.startsWith('|') && line.endsWith('|') && i + 1 < lines.size()) {
            QString separatorLine = lines[i + 1].trimmed();

            // Check if next line is a separator (contains |, -, and optionally :)
            QRegularExpression sepPattern("^\\|[\\s:|-]+\\|$");
            if (sepPattern.match(separatorLine).hasMatch()) {
                // Found a table! Parse it
                LOG_DEBUG(QString("Found markdown table at line %1").arg(i));
                QStringList tableLines;
                tableLines << line;  // Header
                tableLines << separatorLine;  // Separator

                // Collect all table rows
                int j = i + 2;
                while (j < lines.size()) {
                    QString rowLine = lines[j].trimmed();
                    if (rowLine.startsWith('|') && rowLine.endsWith('|')) {
                        tableLines << rowLine;
                        j++;
                    } else {
                        break;
                    }
                }

                // Convert table to HTML
                QString htmlTable = buildHtmlTable(tableLines);
                LOG_DEBUG(QString("Converted table to HTML (%1 rows)").arg(tableLines.size()));

                // Replace the table lines in the result
                QString tableMarkdown = lines.mid(i, j - i).join('\n');
                LOG_DEBUG(QString("Table markdown length: %1, HTML length: %2")
                    .arg(tableMarkdown.length()).arg(htmlTable.length()));
                result.replace(tableMarkdown, htmlTable);

                // Skip past the table
                i = j;
                continue;
            }
        }
        i++;
    }

    return result;
}

QStringList MarkdownHandler::parseTableRow(const QString &row) {
    QString cleaned = row;
    // Remove leading and trailing pipes
    if (cleaned.startsWith('|')) cleaned = cleaned.mid(1);
    if (cleaned.endsWith('|')) cleaned.chop(1);

    QStringList cells = cleaned.split('|');
    return cells;
}

QList<QString> MarkdownHandler::parseTableAlignment(const QString &separator) {
    QStringList parts = parseTableRow(separator);
    QList<QString> alignments;

    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        bool leftColon = trimmed.startsWith(':');
        bool rightColon = trimmed.endsWith(':');

        if (leftColon && rightColon) {
            alignments << "center";
        } else if (rightColon) {
            alignments << "right";
        } else {
            alignments << "left";
        }
    }

    return alignments;
}

QString MarkdownHandler::buildHtmlTable(const QStringList &tableLines) {
    if (tableLines.size() < 2) {
        return tableLines.join('\n');
    }

    // Parse header
    QString headerLine = tableLines[0];
    QStringList headers = parseTableRow(headerLine);

    // Parse separator to get alignment info
    QString separatorLine = tableLines[1];
    QList<QString> alignments = parseTableAlignment(separatorLine);

    // Build HTML table
    QString html = "<table style='border-collapse: collapse; margin: 12px 0; width: auto; "
                  "background-color: white; border: 1px solid #ddd; font-size: 9.5pt;'>";

    // Table header
    html += "<thead><tr style='background-color: #f5f5f5; border-bottom: 2px solid #2196F3;'>";
    for (int i = 0; i < headers.size(); i++) {
        QString align = (i < alignments.size()) ? alignments[i] : "left";
        QString headerContent = headers[i].trimmed().toHtmlEscaped();
        headerContent.replace("_", "&#95;");  // Escape underscores to prevent italic interpretation
        html += QString("<th style='padding: 10px 12px; text-align: %1; font-weight: bold; "
                       "border: 1px solid #ddd;'>%2</th>")
                .arg(align, headerContent);
    }
    html += "</tr></thead>";

    // Table body
    html += "<tbody>";
    for (int row = 2; row < tableLines.size(); row++) {
        QStringList cells = parseTableRow(tableLines[row]);

        // Alternate row colors
        QString rowStyle = (row % 2 == 0) ? "background-color: #fafafa;" : "background-color: white;";
        html += QString("<tr style='%1'>").arg(rowStyle);

        for (int i = 0; i < cells.size(); i++) {
            QString align = (i < alignments.size()) ? alignments[i] : "left";
            QString cellContent = cells[i].trimmed().toHtmlEscaped();
            cellContent.replace("_", "&#95;");  // Escape underscores to prevent italic interpretation
            html += QString("<td style='padding: 8px 12px; text-align: %1; border: 1px solid #ddd;'>%2</td>")
                    .arg(align, cellContent);
        }
        html += "</tr>";
    }
    html += "</tbody></table>";

    return html;
}
