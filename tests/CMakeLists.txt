# Unit tests for qt-chatbot-agent

find_package(Qt5 COMPONENTS Test REQUIRED)

# Enable automatic MOC for test files
set(CMAKE_AUTOMOC ON)

# Test executable for Config
add_executable(test_config test_config.cpp
    ${CMAKE_SOURCE_DIR}/src/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
)

target_link_libraries(test_config
    Qt5::Core
    Qt5::Test
)

target_include_directories(test_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Add test to CTest
add_test(NAME ConfigTest COMMAND test_config)

# Test executable for MCPHandler
add_executable(test_mcphandler test_mcphandler.cpp
    ${CMAKE_SOURCE_DIR}/src/MCPHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/SSEClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/include/MCPHandler.h
    ${CMAKE_SOURCE_DIR}/include/SSEClient.h
)

target_link_libraries(test_mcphandler
    Qt5::Core
    Qt5::Network
    Qt5::Test
)

target_include_directories(test_mcphandler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure MOC runs on MCPHandler.h
set_target_properties(test_mcphandler PROPERTIES AUTOMOC ON)

# Add test to CTest
add_test(NAME MCPHandlerTest COMMAND test_mcphandler)

# Test executable for MCP Server integration
add_executable(test_mcp_server test_mcp_server.cpp
    ${CMAKE_SOURCE_DIR}/src/MCPHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/SSEClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/include/MCPHandler.h
    ${CMAKE_SOURCE_DIR}/include/SSEClient.h
)

target_link_libraries(test_mcp_server
    Qt5::Core
    Qt5::Network
    Qt5::Test
)

target_include_directories(test_mcp_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure MOC runs on test_mcp_server
set_target_properties(test_mcp_server PROPERTIES AUTOMOC ON)

# Add test to CTest
add_test(NAME MCPServerTest COMMAND test_mcp_server)

# Set test properties
set_tests_properties(ConfigTest PROPERTIES
    TIMEOUT 30
)

set_tests_properties(MCPHandlerTest PROPERTIES
    TIMEOUT 30
)

set_tests_properties(MCPServerTest PROPERTIES
    TIMEOUT 60
)

# Test executable for SSEClient
add_executable(test_sseclient test_sseclient.cpp
    ${CMAKE_SOURCE_DIR}/src/SSEClient.cpp
    ${CMAKE_SOURCE_DIR}/src/MCPHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/include/SSEClient.h
    ${CMAKE_SOURCE_DIR}/include/MCPHandler.h
)

target_link_libraries(test_sseclient
    Qt5::Core
    Qt5::Network
    Qt5::Test
)

target_include_directories(test_sseclient PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure MOC runs on SSEClient.h and MCPHandler.h
set_target_properties(test_sseclient PROPERTIES AUTOMOC ON)

# Add test to CTest
add_test(NAME SSEClientTest COMMAND test_sseclient)

set_tests_properties(SSEClientTest PROPERTIES
    TIMEOUT 30
)

# Test executable for RAGEngine
add_executable(test_ragengine test_ragengine.cpp
    ${CMAKE_SOURCE_DIR}/src/RAGEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/include/RAGEngine.h
)

target_link_libraries(test_ragengine
    Qt5::Core
    Qt5::Network
    Qt5::Test
)

# Link FAISS if available
if(faiss_FOUND)
    target_link_libraries(test_ragengine faiss)
endif()

target_include_directories(test_ragengine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Ensure MOC runs on RAGEngine.h
set_target_properties(test_ragengine PROPERTIES AUTOMOC ON)

# Add test to CTest
add_test(NAME RAGEngineTest COMMAND test_ragengine)

set_tests_properties(RAGEngineTest PROPERTIES
    TIMEOUT 30
)
