/**
 * CLIMode.h - Command-line interface mode
 * 
 * Implements CLI-only mode for testing and automation without GUI.
 * Supports prompts, tool calling, RAG context, and diagnostic tests.
 */

#ifndef CLIMODE_H
#define CLIMODE_H

#include <QCommandLineParser>

/**
 * @brief Run the application in CLI (Command-Line Interface) mode
 * 
 * This function handles CLI-only operation without the Qt GUI, supporting:
 * - Direct prompt processing with LLM
 * - Tool calling and execution
 * - MCP server integration
 * - Diagnostic tests (MCP, RAG, unit tests)
 * 
 * @param parser Parsed command-line arguments
 * @return Exit code (0 for success, non-zero for failure)
 */
int runCLI(const QCommandLineParser &parser);

#endif // CLIMODE_H
