# Conversation Management

The qt-chatbot-agent provides comprehensive tools for managing your chat conversations, including clearing, copying, and exporting functionality.

## Features

### 1. Clear Conversation

**Description**: Clears all messages from the current chat session.

**How to Use**:
- Menu: `Edit → Clear Conversation`
- Keyboard: `Ctrl+L`

**Behavior**:
- Shows confirmation dialog to prevent accidental clearing
- Clears all messages from chat display
- Adds a system message confirming the action
- Logged to application log

**Confirmation Dialog**:
```
┌─────────────────────────────────────────┐
│ Clear Conversation                      │
├─────────────────────────────────────────┤
│ Are you sure you want to clear the     │
│ conversation?                           │
│                                         │
│           [ Yes ]    [ No ]             │
└─────────────────────────────────────────┘
```

**Use Cases**:
- Starting a new conversation topic
- Clearing context for a fresh start
- Removing sensitive information from display
- Resetting the chat before sharing screen

---

### 2. Copy Conversation

**Description**: Copies the entire conversation to the system clipboard as plain text.

**How to Use**:
- Menu: `Edit → Copy Conversation`
- Keyboard: `Ctrl+C`

**Behavior**:
- Copies all visible messages to clipboard
- Uses plain text format (no HTML)
- Preserves timestamps and sender names
- Shows confirmation message in chat
- Logged to application log

**Output Format**:
```
[14:23:45] You: Hello!
[14:23:47] Bot: Hi! How can I help you today?
[14:24:01] You: What's 5 + 3?
[14:24:03] System: 🔧 Tool Call: calculator
[14:24:03] System: ✓ Tool Result: calculator
[14:24:04] Bot: The answer is 8.
```

**Use Cases**:
- Sharing conversations via email or messaging
- Creating documentation from chat sessions
- Backing up important conversations
- Copying for note-taking applications
- Pasting into reports or documentation

**Note**: After copying, you can paste (Ctrl+V) into any application.

---

### 3. Export Conversation

**Description**: Saves the conversation to a file with metadata and formatting.

**How to Use**:
- Menu: `File → Export Conversation...`
- Keyboard: `Ctrl+E`

**Behavior**:
- Opens file dialog for location and format selection
- Supports `.txt` and `.md` file formats
- Includes metadata header with session information
- Uses plain text format for maximum compatibility
- Shows success/failure message
- Logged to application log

**File Dialog**:
```
┌─────────────────────────────────────────────────────┐
│ Export Conversation                                 │
├─────────────────────────────────────────────────────┤
│ Location: /home/user/                               │
│ File name: conversation.txt                         │
│                                                     │
│ File type: ○ Text Files (*.txt)                     │
│            ○ Markdown Files (*.md)                  │
│            ○ All Files (*)                          │
│                                                     │
│               [ Cancel ]  [ Save ]                  │
└─────────────────────────────────────────────────────┘
```

**File Format**:

The exported file includes a header with metadata:

```
========================================
qt-chatbot-agent Conversation Export
Date: 2025-10-08 14:30:45
Model: gpt-oss:20b
Backend: ollama
========================================

[14:23:45] You: Hello!
[14:23:47] Bot: Hi! How can I help you today?
[14:24:01] You: What's 5 + 3?
...
```

**Use Cases**:
- Creating permanent records of conversations
- Archiving important chat sessions
- Sharing formatted conversations
- Creating training data or examples
- Documentation and reporting
- Legal or compliance requirements

**File Formats**:

| Format | Extension | Best For |
|--------|-----------|----------|
| Text | `.txt` | General purpose, maximum compatibility |
| Markdown | `.md` | GitHub, documentation, formatted display |
| All Files | `*` | Custom extensions |

---

## Keyboard Shortcuts Summary

| Action | Shortcut | Menu Location |
|--------|----------|---------------|
| Clear Conversation | `Ctrl+L` | Edit → Clear Conversation |
| Copy Conversation | `Ctrl+C` | Edit → Copy Conversation |
| Export Conversation | `Ctrl+E` | File → Export Conversation... |
| Settings | `Ctrl+,` | File → Settings... |
| Quit | `Ctrl+Q` | File → Quit |

## Best Practices

### When to Clear

✅ **Do clear when**:
- Starting a completely new topic
- The context is no longer relevant
- You want to reduce token usage
- Testing with fresh state

❌ **Don't clear when**:
- You might need to reference earlier messages
- Building on previous context
- Before exporting (export first!)

### When to Copy

✅ **Copy for**:
- Quick sharing via messaging apps
- Pasting into emails
- Temporary storage
- Quick reference

### When to Export

✅ **Export for**:
- Long-term archival
- Professional documentation
- Including metadata
- Formal records
- Large conversations

## Error Handling

### Clear Conversation
- **Error**: None (operation is always safe)
- **Recovery**: Re-type or paste if cleared accidentally

### Copy Conversation
- **Error**: Clipboard access denied (rare)
- **Recovery**: Try exporting instead

### Export Conversation
- **Error**: Cannot write to file
  - Permission denied
  - Disk full
  - Invalid path

- **Recovery**:
  - Choose different location
  - Check permissions
  - Free up disk space
  - Use copy instead

**Error Dialog Example**:
```
┌─────────────────────────────────────┐
│ Export Failed                       │
├─────────────────────────────────────┤
│ Could not open file for writing:   │
│ /protected/path/file.txt            │
│                                     │
│ Please check permissions and try    │
│ a different location.               │
│                                     │
│              [ OK ]                 │
└─────────────────────────────────────┘
```

## Implementation Details

### Clear Conversation
```cpp
void clearConversation() {
    // Show confirmation
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Clear Conversation", ...);

    if (reply == QMessageBox::Yes) {
        chatDisplay->clear();  // Qt text edit clear
        LOG_INFO("Conversation cleared");
        appendMessage("System", "Conversation cleared.");
    }
}
```

### Copy Conversation
```cpp
void copyConversation() {
    QString plainText = chatDisplay->toPlainText();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(plainText);
    LOG_INFO("Conversation copied to clipboard");
}
```

### Export Conversation
```cpp
void exportConversation() {
    QString fileName = QFileDialog::getSaveFileName(...);
    QFile file(fileName);
    QTextStream out(&file);

    // Write metadata header
    out << "========================================\n";
    out << "qt-chatbot-agent Conversation Export\n";
    ...

    // Write conversation
    out << chatDisplay->toPlainText();
}
```

## Privacy and Security

- **Local Storage**: All operations are local-only
- **No Cloud**: No data sent to external services
- **User Control**: User chooses export location
- **Plain Text**: No encryption (user can encrypt files separately)
- **Clipboard**: Standard system clipboard (consider clearing after use)

## Tips

1. **Export before clearing**: Save important conversations first
2. **Use meaningful filenames**: Include date and topic
3. **Regular exports**: Periodically save interesting conversations
4. **Clipboard hygiene**: Clear clipboard after pasting sensitive info
5. **Backup exports**: Keep exported files in version control or backup

## Context Window Management

**Description**: Automatically manages conversation history to prevent exceeding the model's context window limits.

**How It Works**:
- Monitors token usage in conversation history
- Automatically prunes oldest messages when approaching context limit
- Preserves system prompt and most recent exchanges
- Uses 80% of context window for input, reserves 20% for response

**Configuration**:
```json
{
  "context_window_size": 4096,
  "override_context_window_size": false
}
```

**Behavior**:
- **Automatic pruning**: Removes oldest messages to fit context budget
- **Smart estimation**: Uses ~4 characters per token heuristic
- **Detailed logging**: Shows what was kept/dropped
- **Transparent**: Logs token usage for debugging

**Example Logs**:
```
[INFO] Pruned message history: kept 8/12 messages (2847 tokens), dropped 4 oldest messages
[DEBUG] Message history fits in context: 4 messages (512 tokens)
[WARNING] Current message exceeds context budget! Message tokens: 3500, budget: 3200
```

**Why This Matters**:
- Prevents silent truncation by Ollama (which truncates from beginning)
- Maintains conversation context within model limits
- Preserves important context like system prompt
- Predictable behavior with long conversations

**Manual Reset**:
While automatic pruning happens transparently, you can manually clear history:
- Use `Edit → Clear Conversation` (Ctrl+L) to reset completely
- Call `llmClient->clearConversationHistory()` programmatically

**Best Practices**:
- ✅ Increase `context_window_size` for longer conversations
- ✅ Clear conversation when starting new topic
- ✅ Monitor logs to see pruning behavior
- ❌ Don't set context window larger than model supports
- ❌ Don't disable pruning (it's automatic and beneficial)

**Token Budget Breakdown**:
```
Context Window: 4096 tokens (configurable)
├─ System Prompt: ~50 tokens (always included)
├─ Tool Overhead: ~200 tokens (for tool definitions)
├─ Current Message: varies
├─ Response Reserve (20%): ~819 tokens
└─ Available for History: remaining tokens
    └─ Keeps most recent messages that fit
```

---

## Related Features

- **Settings**: Configure model and backend (affects metadata in exports)
- **Theme**: Visual appearance (doesn't affect exports)
- **Tool Calls**: Included in all operations (clear, copy, export)
- **Context Management**: Automatic history pruning (see above)
