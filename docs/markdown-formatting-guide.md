# Markdown Formatting Guide

The qt-chatbot-agent supports rich markdown formatting in all chat messages, allowing for better readability and professional-looking conversations.

## Supported Markdown Syntax

### 1. Bold Text

Use double asterisks to create bold text:

```
**This text will be bold**
```

Output: **This text will be bold**

### 2. Italic Text

Use single asterisks or underscores for italic text:

```
*This text will be italic*
_This text will also be italic_
```

Output: *This text will be italic*

### 3. Inline Code

Use single backticks for inline code:

```
Use the `print()` function to display output.
```

Output: Use the `print()` function to display output.

**Styling**: Light gray background with pink text

### 4. Code Blocks

Use triple backticks for multi-line code blocks:

````
```
def hello_world():
    print("Hello, World!")
    return True
```
````

**Styling**: Gray background, monospace font, padding and rounded corners

### 5. Bullet Lists

Start lines with `-` or `*` followed by a space:

```
- First item
- Second item
- Third item
```

Output:
- First item
- Second item
- Third item

**Display**: Uses bullet point (•) character

### 6. Numbered Lists

Use numbers followed by a period and space:

```
1. First step
2. Second step
3. Third step
```

Output:
1. First step
2. Second step
3. Third step

## Combining Formatting

You can combine multiple formatting styles:

```
**Bold and *italic* text**
Use `code` in **bold sentences**
```

## Examples

### Technical Documentation

```
User: How do I use the calculator tool?

Bot: The **calculator** tool supports these operations:
- Addition: `add(a, b)`
- Subtraction: `subtract(a, b)`
- Multiplication: `multiply(a, b)`
- Division: `divide(a, b)`

Example usage:
```python
result = calculator.add(5, 3)
print(result)  # Output: 8
```
```

### Code Examples

```
Bot: Here's a **simple Python function**:

```python
def fibonacci(n):
    """Calculate the nth Fibonacci number"""
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)
```

Key points:
- Use *recursion* for elegant solution
- **Base case** is critical (n <= 1)
- Consider using `@cache` decorator for performance
```

### Lists and Structure

```
Bot: **Steps to configure the application:**

1. Open *Settings* from the File menu
2. Select your preferred **backend**:
   - `ollama` for local models
   - `openai` for cloud-based models
3. Choose a **model** from the dropdown
4. Click *Save* to apply changes
```

## Automatic Formatting

All markdown formatting is applied automatically to:
- User messages
- Bot responses (including streaming)
- System messages
- Tool call results

No special action is required - just type naturally using markdown syntax.

## Processing Order

Markdown patterns are processed in this order to avoid conflicts:

1. Code blocks (```)
2. Inline code (`)
3. Bold text (**)
4. Italic text (*, _)
5. Bullet lists (-, *)
6. Numbered lists (1., 2., 3.)

This ensures that code is protected from other formatting, and bold/italic patterns don't interfere with list markers.

## Styling Details

### Code Blocks
- Background: `#f5f5f5` (light gray)
- Padding: `8px`
- Border radius: `4px`
- Font: Monospace
- Margin: `4px 0`

### Inline Code
- Background: `#f5f5f5` (light gray)
- Padding: `2px 4px`
- Border radius: `3px`
- Color: `#c7254e` (pink/red)
- Font: Monospace

## Tips

1. **Use code formatting for technical terms**: `API`, `JSON`, `HTTP`
2. **Bold important concepts**: **configuration**, **settings**, **model**
3. **Italic for emphasis**: *recommended*, *optional*, *experimental*
4. **Lists for step-by-step instructions**
5. **Code blocks for multi-line examples**

## Limitations

- Nested lists are not currently supported
- Headers (# ## ###) are not implemented
- Links [text](url) are not supported yet
- Tables are not supported
- Blockquotes (>) are not supported

These may be added in future versions based on user needs.

## Future Enhancements

Planned markdown features:
- [ ] Header support (# H1, ## H2, ### H3)
- [ ] Link support [text](url)
- [ ] Blockquote support (>)
- [ ] Horizontal rules (---)
- [ ] Strikethrough (~~text~~)
- [ ] Task lists (- [ ] task)
- [ ] Syntax highlighting for code blocks
