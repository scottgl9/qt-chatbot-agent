/**
 * DiagnosticTests.h - System diagnostic and testing utilities
 * 
 * Provides diagnostic tests for MCP tool calling, RAG engine, and system
 * configuration. Command-line accessible for troubleshooting and verification.
 */

#ifndef DIAGNOSTICTESTS_H
#define DIAGNOSTICTESTS_H

/**
 * @brief Run RAG (Retrieval-Augmented Generation) diagnostic test
 * 
 * Tests the RAG pipeline including:
 * - Configuration loading
 * - Document ingestion
 * - Embedding generation
 * - Context retrieval
 * 
 * @return 0 on success, 1 on failure
 */
int runRAGTest();

/**
 * @brief Run MCP (Model Context Protocol) diagnostic test
 * 
 * Tests the MCP handler including:
 * - Tool registration
 * - Tool listing
 * - Direct tool execution
 * - Error handling
 * - Message building
 * 
 * @return 0 on success, 1 on failure
 */
int runMCPTest();

#endif // DIAGNOSTICTESTS_H
