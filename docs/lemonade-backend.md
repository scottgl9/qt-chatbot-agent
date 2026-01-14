# Lemonade Backend Guide

This guide explains how to use qt-chatbot-agent with the Lemonade AI server.

## What is Lemonade?

**Lemonade** (https://lemonade-server.ai/) is an open-source LLM server that provides:

- **OpenAI-compatible API** - Works seamlessly with OpenAI client libraries
- **Multi-engine support** - llama.cpp, OnnxRuntime GenAI, FastFlowLM
- **Hardware acceleration** - Optimized for AMD GPUs and NPUs (especially Ryzen AI)
- **Multiple model formats** - GGUF, ONNX, FLM
- **Built-in model manager** - Easy model download and management
- **Cross-platform** - Windows and Linux support

Lemonade is particularly useful for running local LLMs on AMD hardware with GPU/NPU acceleration.

## Installation

### Quick Install

**Windows:**
```bash
# Download and run installer
https://github.com/lemonade-sdk/lemonade/releases/latest/download/lemonade.msi
```

**Linux (Ubuntu/Arch):**
```bash
# See installation options at:
https://lemonade-server.ai/install_options.html
```

### Starting Lemonade Server

After installation, start the Lemonade server:

```bash
# Start server (default port 8000)
lemonade-server serve

# Or run a specific model directly
lemonade-server run Gemma-3-4b-it-GGUF
```

The server will start at: `http://localhost:8000`

## Configuring qt-chatbot-agent

### 1. Open Settings

- File → Settings (or Ctrl+,)

### 2. Configure Backend

- **Backend**: Select "Lemonade"
- **API URL**: `http://localhost:8000/api/v1/chat/completions`
- **Model**: Click "Refresh" to fetch available models from your Lemonade server
  - Or manually enter model name (e.g., `Gemma-3-4b-it-GGUF`, `Llama-3.2-1B-Instruct-Hybrid`)

### 3. Optional: Adjust LLM Parameters

- Temperature, Top-P, Top-K, Context Window
- Enable "Override" checkboxes to customize (otherwise model defaults are used)

### 4. Save Settings

- Click "OK" to save
- Your settings are stored in `~/.qtbot/config.json`

## Managing Models

### Using Lemonade CLI

```bash
# List available models
lemonade-server list

# Download a model
lemonade-server pull Gemma-3-4b-it-GGUF

# Run and chat with a model
lemonade-server run Gemma-3-4b-it-GGUF
```

### Using the Model Manager

Lemonade includes a built-in GUI for managing models:
- Download new models
- Try models with built-in chat
- Switch between models
- Import custom GGUF/ONNX models from Hugging Face

## Supported Hardware

Lemonade works on various hardware configurations:

### CPU
- All platforms (default fallback)

### GPU
- **Vulkan**: All platforms
- **ROCm**: AMD GPUs (RDNA3, RDNA4, Strix Halo)
  - Radeon RX 7900 XTX/XT/GRE
  - Radeon RX 7800/7700 XT
  - Ryzen AI MAX+ Pro 395

### NPU
- AMD Ryzen AI 300 series (Windows)
- Optimized for edge deployment

## API Compatibility

Lemonade implements the OpenAI-compatible API, so qt-chatbot-agent can:

- ✅ Send chat completion requests
- ✅ Stream responses token-by-token
- ✅ Use tool calling (function calling)
- ✅ Configure model parameters (temperature, top_p, etc.)
- ✅ List available models via `/api/v1/models`

## Example Configuration

Here's an example `~/.qtbot/config.json` for Lemonade:

```json
{
    "backend": "lemonade",
    "model": "Gemma-3-4b-it-GGUF",
    "api_url": "http://localhost:8000/api/v1/chat/completions",
    "system_prompt": "You are a helpful AI assistant.",
    "override_temperature": true,
    "temperature": 0.7,
    "override_context_window_size": true,
    "context_window_size": 4096
}
```

## Troubleshooting

### Server Not Running

**Error**: "Could not connect to Lemonade server"

**Solution**:
```bash
# Check if server is running
curl http://localhost:8000/api/v1/models

# Start server if not running
lemonade-server serve
```

### No Models Available

**Error**: "No models found on Lemonade server"

**Solution**:
```bash
# Download a model first
lemonade-server pull Gemma-3-4b-it-GGUF

# Or use the Model Manager GUI
```

### Port Conflicts

If port 8000 is in use:

```bash
# Start server on different port
lemonade-server serve --port 8080

# Update API URL in qt-chatbot-agent settings to:
# http://localhost:8080/api/v1/chat/completions
```

## Performance Tips

### For AMD GPUs

```bash
# Use ROCm backend for best performance
lemonade-server run --llamacpp rocm Gemma-3-4b-it-GGUF
```

### For AMD NPUs

```bash
# Use OnnxRuntime GenAI (OGA) for NPU
lemonade-server run --engine oga <model-name>
```

### For CPU-only

```bash
# Use Vulkan backend (works everywhere)
lemonade-server run --llamacpp vulkan Gemma-3-4b-it-GGUF
```

## Resources

- **Lemonade Website**: https://lemonade-server.ai/
- **GitHub Repository**: https://github.com/lemonade-sdk/lemonade
- **Documentation**: https://lemonade-server.ai/docs/
- **Discord Community**: https://discord.gg/5xXzkMu8Zk
- **Model Library**: https://lemonade-server.ai/docs/server/server_models/

## Comparison with Ollama

| Feature | Lemonade | Ollama |
|---------|----------|--------|
| **API** | OpenAI-compatible | Ollama-specific |
| **GPU Support** | AMD (ROCm), Vulkan, Metal | NVIDIA (CUDA), AMD (ROCm), Metal |
| **NPU Support** | ✅ AMD Ryzen AI | ❌ No |
| **Model Formats** | GGUF, ONNX, FLM | GGUF only |
| **Hardware Focus** | AMD-optimized | NVIDIA-optimized |
| **Default Port** | 8000 | 11434 |

Both backends work great with qt-chatbot-agent - choose based on your hardware and preferences!

## See Also

- [Model Selection Feature](model-selection-feature.md)
- [Quick Reference](QUICK_REFERENCE.md)
- [System Prompt Configuration](system-prompt-configuration.md)
- [MCP Tool Calling Guide](MCP_TOOL_CALLING_GUIDE.md)
