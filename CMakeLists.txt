cmake_minimum_required(VERSION 3.10)
project(qt-chatbot-agent VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable automatic MOC, UIC, and RCC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5 packages
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Gui
)

# Find FAISS (optional)
find_package(faiss QUIET)
if(faiss_FOUND)
    message(STATUS "FAISS found: ${faiss_VERSION}")
    add_definitions(-DHAVE_FAISS)
else()
    message(STATUS "FAISS not found - RAG engine will have limited functionality")
    message(STATUS "To install FAISS: sudo apt-get install libfaiss-dev")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Logger.cpp
    src/Config.cpp
    src/ThemeManager.cpp
    src/LLMClient.cpp
    src/SettingsDialog.cpp
    src/LogViewerDialog.cpp
    src/MCPHandler.cpp
    src/RAGEngine.cpp
    src/SSEClient.cpp
    src/TestMCPStdioServer.cpp
    src/MarkdownHandler.cpp
    src/HTMLHandler.cpp
    src/DiagnosticTests.cpp
    src/CLIMode.cpp
    src/ConversationManager.cpp
    src/MessageRenderer.cpp
    src/ToolUIManager.cpp
    src/RAGUIManager.cpp
    src/ChatWindow.cpp
)

# Header files
set(HEADERS
    include/version.h
    include/Logger.h
    include/Config.h
    include/ThemeManager.h
    include/LLMClient.h
    include/SettingsDialog.h
    include/LogViewerDialog.h
    include/MCPHandler.h
    include/RAGEngine.h
    include/SSEClient.h
    include/TestMCPStdioServer.h
    include/MarkdownHandler.h
    include/HTMLHandler.h
    include/DiagnosticTests.h
    include/CLIMode.h
    include/ConversationManager.h
    include/MessageRenderer.h
    include/ToolUIManager.h
    include/RAGUIManager.h
    include/ChatWindow.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Gui
)

# Link FAISS if available
if(faiss_FOUND)
    target_link_libraries(${PROJECT_NAME} faiss)
endif()

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

# Print configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt5 version: ${Qt5_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")
