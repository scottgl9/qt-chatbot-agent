# Agent Instructions for qt-chatbot-agent

## Build and Test Policy

**CRITICAL:** Always build and test the application after adding new features or making changes.

## Build Instructions

### Prerequisites
- Qt5 development libraries (qtbase5-dev)
- CMake 3.10 or higher
- C++17 compatible compiler (GCC 9+ or Clang 10+)
- Additional dependencies: libfaiss-dev, nlohmann-json3-dev (for future features)

### Building the Application

```bash
# Create build directory (if it doesn't exist)
mkdir -p build

# Build from build directory
cd build && cmake .. && make -j$(nproc)
```

**Note:** Always run the build command from the project root directory.

### Testing the Application

```bash
# From the build directory
ctest --output-on-failure

# Or run the unit tests directly
./tests/unit_tests
```

### Running the Application

```bash
# GUI mode (default)
./bin/qt-chatbot-agent

# CLI mode
./bin/qt-chatbot-agent --cli --prompt "Test prompt"
```

## Testing Tool Calls and MCP Integration

**IMPORTANT:** Always use CLI mode to test tool calls and MCP integration. This eliminates GUI complexity and provides clear debugging output.

### Why Use CLI Mode for Tool Testing

1. **Clear Output**: See token streaming, tool execution, and results in real-time
2. **No GUI Complexity**: Eliminates HTML rendering, Qt event loop issues, and UI state
3. **Fast Iteration**: Quickly test tool calling without GUI overhead
4. **Easy Debugging**: All logs and responses printed to console
5. **Reproducible**: Simple command-line invocation for consistent testing

### Testing Tool Calls via CLI

```bash
# From build directory
cd build

# Test datetime tool
./bin/qt-chatbot-agent --cli --prompt "What time is it?"

# Test calculator tool
./bin/qt-chatbot-agent --cli --prompt "Calculate 5 + 3"

# Test with different models (if configured)
./bin/qt-chatbot-agent --cli --prompt "What time is it?" --model "different-model"
```

### Expected CLI Output for Tool Calls

When tool calling works correctly, you should see:

```
=== CLI Mode - Tool Calling Test ===
Prompt: What time is it?
Registered 2 tools

Sending prompt to LLM with tools...

🔧 Tool Call: datetime
   Parameters: {"format":"long"}
   Call ID: 6276dd8d

✓ Tool Completed: datetime
   Result: {"date":"Wednesday, October 8, 2025","time":"10:42:16 PM","timezone":"CDT"}

=== Final Response ===
It's currently 10:42:16 PM on Wednesday, October 8, 2025 (CDT).
```

### Debugging Tool Call Issues

If tool calling doesn't work in CLI mode:

1. **Check tool registration**: Verify tools are registered correctly
2. **Check LLM prompt**: Ensure system prompt includes tool definitions
3. **Check JSON parsing**: Look for "Heuristic match" or parsing errors in logs
4. **Check tool execution**: Verify tool actually runs and returns results
5. **Check response formatting**: Ensure natural language response is generated

**Only after CLI mode works**, test in GUI mode to verify UI integration.

### Common CLI Testing Patterns

```bash
# Quick tool test loop
for prompt in "What time is it?" "Calculate 10 * 5" "Generate a random number"; do
    echo "Testing: $prompt"
    ./bin/qt-chatbot-agent --cli --prompt "$prompt"
    echo "---"
done

# Test with debug logging
LOG_LEVEL=DEBUG ./bin/qt-chatbot-agent --cli --prompt "What time is it?"

# Test with timeout
timeout 30 ./bin/qt-chatbot-agent --cli --prompt "What time is it?"
```

## Development Workflow

1. **Before making changes:**
   - Pull latest changes from main branch
   - Ensure existing tests pass

2. **While developing:**
   - Write code following Qt5 and C++ best practices
   - Add unit tests for new functionality
   - Update documentation if needed

3. **After making changes:**
   - **ALWAYS** build the project: `cd build && make`
   - **ALWAYS** run tests: `ctest --output-on-failure`
   - Fix any build errors or test failures before proceeding
   - Update PROGRESS.md with completed items
   - Update TODO.md if new tasks are discovered

4. **Committing and pushing changes:**
   - **CRITICAL:** Always commit and push after completing a new feature/update
   - **CRITICAL:** Always commit and push after a successful build
   - **CRITICAL:** Always commit and push when testing works correctly
   - Verify all tests pass before committing
   - Run static analysis if available: `cppcheck src/`
   - Update changelog if significant changes
   - Stage your changes: `git add .`
   - Create a descriptive commit message
   - Commit changes: `git commit -m "description of changes"`
   - Push to remote: `git push origin main`

### Git Commit Workflow Example

```bash
# After successful build and tests
cd /path/to/qt-chatbot-agent

# Stage all changes
git add .

# Commit with descriptive message
git commit -m "Add logging system with file output

- Implemented Logger class with singleton pattern
- Added multi-level logging (Debug, Info, Warning, Error)
- Integrated Qt message handler
- Updated build system to include Logger files
- Refactored main() to use QCoreApplication for CLI mode
- Updated TODO.md and PROGRESS.md"

# Push to remote repository
git push origin main
```

5. **Commit Message Guidelines:**
   - Use clear, descriptive titles (50 chars or less)
   - Include bullet points explaining changes
   - Reference issue numbers if applicable

## Continuous Integration

Every commit should:
- Build successfully
- Pass all unit tests
- Pass integration tests (when available)
- Maintain or improve code coverage

## Quality Standards

- **Code Style:** Follow Qt coding conventions
- **Testing:** Aim for >80% code coverage
- **Documentation:** Update docs for new features
- **Logging:** Use appropriate log levels (Debug, Info, Warning, Error)

## Common Commands Reference

```bash
# Clean build
rm -rf build && mkdir build && cd build && cmake .. && make

# Verbose build
make VERBOSE=1

# Run specific test
ctest -R test_name -V

# Check for memory leaks
valgrind ./bin/qt-chatbot-agent
```
