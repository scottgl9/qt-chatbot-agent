# Model Selection Feature

## Overview
The Settings dialog now includes the ability to fetch and select available models directly from your Ollama server.

## Features

### Model Dropdown with Auto-Refresh
- **Editable Combo Box**: The model field is now a dropdown that allows both selection from a list and manual text entry
- **Auto-Refresh on Open**: When using Ollama backend, the model list is automatically fetched when Settings dialog opens
- **🔄 Manual Refresh Button**: Allows you to refresh the model list at any time
- **Backend-Aware**: The refresh button is only enabled when using the Ollama backend
- **Silent Mode**: Auto-refresh runs silently in the background without showing popup messages

### How It Works

#### Automatic Refresh
1. **Open Settings**: Click `File → Settings` or press the settings shortcut
2. **Auto-Load**: If Ollama backend is selected, available models are automatically fetched
3. **Select Model**: Choose from the dropdown of available models

#### Manual Refresh
1. **Open Settings**: Click `File → Settings`
2. **Ensure Ollama Backend**: Select "Ollama" from the Backend dropdown
3. **Click Refresh**: Click the "🔄 Refresh" button to reload the model list
4. **Select Model**: Choose from the updated dropdown

### Technical Details

#### API Endpoint
The feature queries the Ollama `/api/tags` endpoint to retrieve available models:
```
http://<your-host>:<port>/api/tags
```

The base URL is automatically derived from your configured API URL.

#### Response Format
Ollama returns a JSON response with model information:
```json
{
  "models": [
    {
      "name": "llama3:latest",
      "modified_at": "2024-01-01T00:00:00Z",
      "size": 4661224832,
      ...
    }
  ]
}
```

#### Model Name Cleanup
- The `:latest` tag suffix is automatically removed for cleaner display
- Full model names (with tags) can still be entered manually if needed

## Error Handling

The feature includes comprehensive error handling:

- **Connection Errors**: Clear message if Ollama server is unreachable (only for manual refresh)
- **Silent Failures**: Auto-refresh on dialog open fails silently without popup if server is unavailable
- **Parse Errors**: Validation of server response format
- **No Models**: Helpful guidance on how to pull models using `ollama pull`
- **Backend Mismatch**: Information dialog when trying to refresh with OpenAI backend

## Usage Examples

### Typical Workflow (with Auto-Refresh)
1. Open Settings dialog
2. Models are automatically loaded if using Ollama backend
3. Select your desired model from the dropdown (e.g., "llama3", "mistral", "codellama")
4. Adjust other settings as needed
5. Click "Save"

### Manual Refresh Workflow
1. Open Settings dialog
2. Make changes to API URL or switch backends
3. Click "🔄 Refresh" to reload available models
4. Select your desired model from the dropdown
5. Click "Save"

### Manual Entry
If you want to use a specific model tag that's not in the list:
1. Click in the model combo box
2. Type the full model name (e.g., "llama3:70b-instruct-q4_K_M")
3. Click "Save"

## Benefits

- **Convenience**: No need to remember exact model names or manually refresh
- **Discovery**: Automatically see what models are available on your Ollama server
- **Seamless Experience**: Models load in background when you open Settings
- **Validation**: Ensure the model you select actually exists
- **Flexibility**: Still allows manual entry for custom or tagged versions
- **Non-Intrusive**: Auto-refresh fails silently if server is unavailable

## Implementation Notes

- Network operations are non-blocking and show loading state
- QNetworkAccessManager is initialized after event loop to avoid Qt threading issues
- Auto-refresh triggered 100ms after network manager initialization when Ollama backend is selected
- Silent mode suppresses success/error popups for automatic refresh
- Manual refresh always shows feedback messages
- Button state is managed based on backend selection
- Current model selection is preserved when refreshing the list
