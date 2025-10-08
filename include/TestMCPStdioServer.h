/**
 * TestMCPStdioServer.h - Test MCP server for stdio protocol
 * 
 * Implements a test MCP server using stdio communication via JSON-RPC 2.0.
 * Provides example tools: hello, echo, reverse_string for testing MCP integration.
 */

#ifndef TESTMCPSTDIOSERVER_H
#define TESTMCPSTDIOSERVER_H

/**
 * Run a test MCP server in stdio mode
 * @return Exit code (0 for success)
 */
int runTestMCPStdioServer();

#endif // TESTMCPSTDIOSERVER_H
