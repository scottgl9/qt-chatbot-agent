# Tool Call Visualization

## Overview

The qt-chatbot-agent provides clear, real-time visual feedback when the LLM uses MCP tools. This document explains the tool call visualization system and what each indicator means.

## Visual Indicators

### 1. Tool Call Started

When the LLM decides to use a tool, you'll see:

**Format:**
```
[14:30:45] System: 🔧 Calling tool: calculator
```

**Followed by a details box:**
```
┌─────────────────────────────────────────┐
│ Parameters:                             │
│ {"operation":"add","a":5,"b":3}        │
└─────────────────────────────────────────┘
```

**Visual Style:**
- **Icon**: 🔧 (wrench icon)
- **Tool name**: Blue, bold text
- **Details box**: Light blue background with blue left border
- **Indented**: Details box is indented for visual hierarchy

**What it means:**
- The LLM has identified that it needs to use a tool
- The tool is being executed right now
- The parameters show what inputs the LLM is providing

---

### 2. Tool Completed Successfully

When a tool finishes executing successfully:

**Format:**
```
[14:30:46] System: ✓ Tool completed: calculator
```

**Followed by a results box:**
```
┌─────────────────────────────────────────┐
│ Result:                                 │
│ {                                       │
│   "result": 8,                          │
│   "operation": "add",                   │
│   "a": 5,                               │
│   "b": 3                                │
│ }                                       │
└─────────────────────────────────────────┘
```

**Visual Style:**
- **Icon**: ✓ (checkmark)
- **Tool name**: Green, bold text
- **Details box**: Light green background with green left border
- **Indented**: Results box is indented
- **JSON formatting**: Pretty-printed for readability
- **Truncation**: Results over 200 characters are truncated with "..."

**What it means:**
- The tool executed successfully
- The result is what the tool returned
- The LLM will now use this result to formulate its response

---

### 3. Tool Failed

When a tool encounters an error:

**Format:**
```
[14:30:46] System: ✗ Tool failed: calculator
```

**Followed by an error box:**
```
┌─────────────────────────────────────────┐
│ Error:                                  │
│ Division by zero                        │
└─────────────────────────────────────────┘
```

**Visual Style:**
- **Icon**: ✗ (X mark)
- **Tool name**: Red, bold text
- **Details box**: Light red background with red left border
- **Indented**: Error box is indented
- **Error message**: Red text showing what went wrong

**What it means:**
- The tool execution failed
- The error message explains what went wrong
- The LLM may try a different approach or ask for clarification

---

## Complete Example Flow

Here's what a complete tool call looks like in the chat:

```
[14:30:45] You: What is 5 + 3?

[14:30:45] System: 🔧 Calling tool: calculator
    ┌──────────────────────────────────────┐
    │ Parameters:                          │
    │ {"operation":"add","a":5,"b":3}     │
    └──────────────────────────────────────┘

[14:30:46] System: ✓ Tool completed: calculator
    ┌──────────────────────────────────────┐
    │ Result:                              │
    │ {                                    │
    │   "result": 8,                       │
    │   "operation": "add",                │
    │   "a": 5,                            │
    │   "b": 3                             │
    │ }                                    │
    └──────────────────────────────────────┘

[14:30:47] Bot: The answer is 8. I used the calculator tool to add 5 and 3, which equals 8.
```

---

## Visual Design Principles

### Color Coding

The visualization uses a clear color scheme:

| State | Color | Meaning |
|-------|-------|---------|
| Calling | Blue (#2196F3) | Information - tool is executing |
| Completed | Green (#4CAF50) | Success - tool finished successfully |
| Failed | Red (#f44336) | Error - tool encountered a problem |

### Visual Hierarchy

1. **Simple message first**: The tool name is shown prominently
2. **Details below**: Parameters/results/errors are in a separate, indented box
3. **Consistent spacing**: All elements use consistent padding and margins

### Readability

- **Monospace font**: JSON data uses monospace for better readability
- **Indented formatting**: Results are pretty-printed with indentation
- **Truncation**: Long results are truncated to prevent overwhelming the chat
- **Indented boxes**: Details are indented 20px to show hierarchy

---

## Available Tools

The chatbot currently has 5 MCP tools that can be called:

### 1. Calculator
**Name**: `calculator`
**What it does**: Basic arithmetic (add, subtract, multiply, divide)
**Example call**: `{"operation":"add","a":5,"b":3}`

### 2. Greeter
**Name**: `greeter`
**What it does**: Greets a person in different languages
**Example call**: `{"name":"Alice","language":"es"}`

### 3. DateTime
**Name**: `datetime`
**What it does**: Get current date and time in various formats
**Example call**: `{"format":"long"}`

### 4. Text Stats
**Name**: `text_stats`
**What it does**: Analyze text statistics
**Example call**: `{"text":"Hello World 123"}`

### 5. Random Number
**Name**: `random_number`
**What it does**: Generate random numbers
**Example call**: `{"min":1,"max":10,"count":3}`

---

## Implementation Details

### Code Location

All visualization code is in `src/main.cpp`:

- **Tool call started**: `handleToolCallRequest()` (lines 367-388)
- **Tool completed**: `handleToolCallCompleted()` (lines 390-426)
- **Tool failed**: `handleToolCallFailed()` (lines 428-448)

### Message Format

Each message follows this pattern:

```cpp
// Simple headline message
QString simpleMessage = QString("🔧 <b>Calling tool:</b> <span style='...'>%1</span>")
    .arg(toolName);
QString timestamp = QTime::currentTime().toString("hh:mm:ss");
QString formatted = QString("<b>[%1] System:</b> %2<br>")
    .arg(timestamp, simpleMessage);
chatDisplay->append(formatted);

// Detailed information box
QString detailsMessage = QString(
    "<div style='background-color: #e3f2fd; padding: 8px; "
    "border-left: 4px solid #2196F3; margin: 4px 0 4px 20px;'>"
    "<b>Parameters:</b><br>"
    "<span style='font-family: monospace;'>%1</span>"
    "</div>").arg(paramStr);
chatDisplay->append(detailsMessage);
```

### Styling

**CSS Properties Used:**
- `background-color`: Light tint for each state
- `border-left`: 4px solid color for visual indicator
- `padding`: 8px for comfortable spacing
- `margin`: Spacing around boxes, 20px left indent
- `font-family`: Monospace for code/JSON
- `font-size`: 9pt for detail text
- `color`: Appropriate color for each state

---

## User Experience

### Benefits

1. **Clear visibility**: You always know when tools are being used
2. **Transparency**: You can see what parameters the LLM is using
3. **Debugging**: Easy to spot issues with tool calls
4. **Learning**: Understand how the LLM decides to use tools
5. **Trust**: See exactly what data is being processed

### Design Decisions

**Why two-level display?**
- The simple message is easy to scan
- The details box provides full information when needed
- This prevents overwhelming the user while maintaining transparency

**Why color coding?**
- Instant visual feedback on the state
- Matches common UI patterns (green=good, red=error)
- Accessible and intuitive

**Why show parameters?**
- Transparency about what data is being used
- Helps users understand tool usage
- Useful for debugging and learning

**Why truncate results?**
- Prevents chat from being overwhelmed with data
- Shows enough information to be useful
- Full results are still sent to the LLM

---

## Customization

Currently, the tool visualization is not user-customizable, but future versions may include:

- [ ] Toggle to show/hide tool calls
- [ ] Compact mode (simple messages only, no details)
- [ ] Detailed mode (show full results, no truncation)
- [ ] Custom color schemes
- [ ] Adjustable truncation length
- [ ] Collapsible details boxes

---

## Troubleshooting

### Tool Call Not Showing

**Problem**: You don't see tool call messages
**Possible causes:**
1. Tools are disabled in settings
2. The LLM doesn't think a tool is needed
3. The model doesn't support tool calling

**Solutions:**
1. Check that tools are enabled
2. Try a more explicit request (e.g., "Use the calculator to add 5 and 3")
3. Verify your model supports tool/function calling

### Parameters Look Wrong

**Problem**: The parameters shown seem incorrect
**What to do:**
1. This is actually useful debugging information!
2. Check the LLM's interpretation of your request
3. Try rephrasing your question more clearly
4. Report the issue if it's a bug in tool parameter mapping

### Results Truncated

**Problem**: Results show "..." at the end
**Why**: Results over 200 characters are truncated for readability
**Solution**: This is normal - the full result is still sent to the LLM
**Note**: Future versions may allow adjusting truncation length

---

## Examples

### Example 1: Calculator

```
User: What's 15 multiplied by 7?

System: 🔧 Calling tool: calculator
    Parameters:
    {"operation":"multiply","a":15,"b":7}

System: ✓ Tool completed: calculator
    Result:
    {
      "result": 105,
      "operation": "multiply",
      "a": 15,
      "b": 7
    }

Bot: The result of 15 multiplied by 7 is 105.
```

### Example 2: DateTime

```
User: What's the current date and time?

System: 🔧 Calling tool: datetime
    Parameters:
    {"format":"long"}

System: ✓ Tool completed: datetime
    Result:
    {
      "date": "Wednesday, October 8, 2025",
      "time": "2:30:45 PM",
      "timezone": "EDT"
    }

Bot: The current date is Wednesday, October 8, 2025, and the time is 2:30:45 PM EDT.
```

### Example 3: Multiple Tools

The LLM can chain multiple tool calls:

```
User: What time is it, and what's 5 + 3?

System: 🔧 Calling tool: datetime
    Parameters:
    {"format":"short"}

System: ✓ Tool completed: datetime
    Result:
    {
      "date": "2025-10-08",
      "time": "14:30:45"
    }

System: 🔧 Calling tool: calculator
    Parameters:
    {"operation":"add","a":5,"b":3}

System: ✓ Tool completed: calculator
    Result:
    {
      "result": 8,
      "operation": "add",
      "a": 5,
      "b": 3
    }

Bot: The current time is 14:30:45, and 5 + 3 equals 8.
```

### Example 4: Error Handling

```
User: Divide 10 by 0

System: 🔧 Calling tool: calculator
    Parameters:
    {"operation":"divide","a":10,"b":0}

System: ✗ Tool failed: calculator
    Error:
    Division by zero

Bot: I encountered an error trying to divide by zero. Division by zero is undefined in mathematics. Would you like to try a different calculation?
```

---

## Best Practices

### For Users

1. **Pay attention to tool calls** - They show you what the LLM is doing
2. **Check parameters** - Make sure the LLM understood your request correctly
3. **Learn from errors** - Error messages help you understand limitations
4. **Trust the process** - The LLM knows when to use tools

### For Developers

1. **Keep messages concise** - Simple headline + detailed box
2. **Use color consistently** - Blue=info, Green=success, Red=error
3. **Show meaningful data** - Parameters and results should be informative
4. **Handle errors gracefully** - Show clear error messages
5. **Maintain visual hierarchy** - Headline first, details indented

---

## Technical Notes

### Performance

- **Message creation**: <1ms per tool call
- **Rendering**: Handled by Qt's QTextEdit (very fast)
- **Impact**: Negligible on overall performance

### Accessibility

- **Color**: Don't rely solely on color (also use icons)
- **Text**: All information is text-based
- **Screen readers**: HTML structure is semantic

### Future Improvements

Planned enhancements:

1. **Timestamps in details**: Show exact execution time
2. **Duration indicator**: Show how long tool took to execute
3. **Collapse/expand**: Ability to collapse details boxes
4. **Filter view**: Option to hide tool calls entirely
5. **Export formatting**: Better formatting in exported conversations
6. **Custom themes**: Support for different color schemes

---

## Related Documentation

- **MCP Tools**: See `status-bar-and-tools.md` for tool documentation
- **Conversation Management**: See `conversation-management.md` for export features
- **Settings**: Configure which backend/model to use

---

## Conclusion

The tool call visualization provides transparent, real-time feedback about LLM tool usage. The two-level display (simple message + detailed box) balances clarity with completeness, making it easy to understand what's happening while maintaining full transparency.

**Key Features:**
- ✅ Clear visual indicators (icons and colors)
- ✅ Two-level information display
- ✅ Full transparency (parameters and results visible)
- ✅ Error handling with helpful messages
- ✅ Consistent, accessible design

**Status**: ✅ Production ready
**Version**: 1.0.0
**Last Updated**: 2025-10-08
