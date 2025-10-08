# Debian Package Build Guide

**Project**: qt-chatbot-agent
**Package Name**: hptc-qt-chatbot-agent
**Version**: 1.1.0-1
**Date**: 2025-10-09

---

## Overview

The qt-chatbot-agent now includes full Debian packaging support with the package name **hptc-qt-chatbot-agent**, allowing easy installation and distribution on Debian-based systems (Debian, Ubuntu, Linux Mint, etc.).

---

## Quick Start

### Building the Package

```bash
# From the project root directory:
dpkg-buildpackage -b -uc -us

# The .deb package will be created in the parent directory:
# ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb
```

### Installing the Package

```bash
# Install with dpkg:
sudo dpkg -i ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb

# If there are dependency issues, fix them with:
sudo apt-get install -f
```

### Removing the Package

```bash
sudo apt-get remove hptc-qt-chatbot-agent

# Or to remove including configuration:
sudo apt-get purge hptc-qt-chatbot-agent
```

---

## Package Information

### Metadata

- **Package Name**: hptc-qt-chatbot-agent
- **Binary Name**: qt-chatbot-agent (executable remains unchanged)
- **Version**: 1.1.0-1
- **Architecture**: amd64 (64-bit x86)
- **Section**: utils
- **Priority**: optional
- **Maintainer**: Scott Glover <scottgl@gmail.com>
- **Homepage**: https://github.com/scottgl/qt-chatbot-agent

### Size

- **Package Size**: ~149 KB
- **Installed Size**: ~444 KB

### Dependencies

**Build Dependencies**:
- debhelper (>= 9)
- cmake (>= 3.10)
- qtbase5-dev (>= 5.15)
- libqt5network5 (>= 5.15)
- qt5-qmake

**Runtime Dependencies** (automatically determined):
- libc6 (>= 2.34)
- libgcc-s1 (>= 3.0)
- libqt5core5t64 (>= 5.15.1)
- libqt5gui5t64 (>= 5.15) or libqt5gui5-gles (>= 5.15)
- libqt5network5t64 (>= 5.15)
- libqt5widgets5t64 (>= 5.15.1)
- libstdc++6 (>= 13.1)

---

## File Structure

### Debian Directory

```
debian/
├── changelog                      # Version history
├── compat                         # Debian compatibility level (9)
├── control                        # Package metadata and dependencies
├── copyright                      # License information (MIT)
├── install                        # File installation mappings
├── rules                          # Build rules (executable)
└── hptc-qt-chatbot-agent.desktop   # Desktop entry file
```

### Installed Files

After installation, files are placed in:

```
/usr/bin/qt-chatbot-agent                   # Main executable
/etc/qt-chatbot-agent/                      # Configuration directory
/usr/share/doc/hptc-qt-chatbot-agent/       # Documentation files
└── *.md.gz                                # Compressed documentation
```

### Documentation Included

All documentation is automatically compressed and installed:
- QUICK_REFERENCE.md
- README.md
- TESTING_GUIDE.md
- conversation-management.md
- markdown-formatting-guide.md
- mcp-stdio-server.md
- mcp-testing.md
- model-selection-feature.md
- modern-chat-interface.md
- recent-updates.md
- status-bar-and-tools.md
- system-prompt-configuration.md
- tool-call-visualization.md

---

## Detailed Build Instructions

### Prerequisites

Install build dependencies:

```bash
sudo apt-get update
sudo apt-get install -y \
    debhelper \
    cmake \
    qtbase5-dev \
    libqt5network5 \
    qt5-qmake \
    build-essential
```

### Build Process

1. **Clean previous builds** (optional):
   ```bash
   debian/rules clean
   ```

2. **Build the package**:
   ```bash
   dpkg-buildpackage -b -uc -us
   ```

   Flags explained:
   - `-b`: Build binary package only (no source)
   - `-uc`: Don't sign .changes file
   - `-us`: Don't sign source package

3. **Locate the package**:
   ```bash
   ls -lh ../hptc-qt-chatbot-agent*.deb
   ```

### Build Artifacts

After building, you'll find in the parent directory:

- `hptc-qt-chatbot-agent_1.1.0-1_amd64.deb` - Main package
- `hptc-qt-chatbot-agent-dbgsym_1.1.0-1_amd64.ddeb` - Debug symbols
- `hptc-qt-chatbot-agent_1.1.0-1_amd64.buildinfo` - Build information
- `hptc-qt-chatbot-agent_1.1.0-1_amd64.changes` - Changes file

These are automatically ignored by git (see `.gitignore`).

---

## Package Contents

### Inspecting the Package

```bash
# Show package information:
dpkg-deb -I ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb

# List package contents:
dpkg-deb -c ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb

# Extract package contents (for inspection):
dpkg-deb -x ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb /tmp/inspect
```

### Package Description

The package includes this description:

> Modern AI chatbot with MCP integration
>
> hptc-qt-chatbot-agent is a Qt5-based AI chatbot application with:
>
>  - Support for Ollama and OpenAI backends
>  - Model Context Protocol (MCP) tool calling
>  - Modern chat interface with message bubbles
>  - 5 built-in MCP tools (calculator, greeter, datetime, etc.)
>  - Streaming token display
>  - Markdown formatting support
>  - Conversation save/load functionality
>  - Light and dark theme support
>  - CLI and GUI modes
>
> The application provides a clean, modern interface similar to
> popular chat applications with special widgets for tool execution.

---

## Installation

### Installing from .deb File

```bash
# Install the package:
sudo dpkg -i ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb

# If dependency errors occur:
sudo apt-get install -f
```

### Verifying Installation

```bash
# Check if installed:
dpkg -l | grep hptc-qt-chatbot-agent

# Verify binary location:
which qt-chatbot-agent

# Run the application:
qt-chatbot-agent

# Check version:
qt-chatbot-agent --version
```

### Post-Installation

After installation:

1. **Configuration**: User config stored in `~/.qtbot/config.json`
2. **Desktop Entry**: Application appears in application menu
3. **Documentation**: Available in `/usr/share/doc/hptc-qt-chatbot-agent/`

---

## Uninstallation

### Remove Package

```bash
# Remove package but keep configuration:
sudo apt-get remove hptc-qt-chatbot-agent

# Remove package and configuration:
sudo apt-get purge hptc-qt-chatbot-agent
```

### Clean User Data

```bash
# Remove user configuration (optional):
rm -rf ~/.qtbot/
```

---

## Debian Packaging Files Explained

### debian/control

Defines package metadata, dependencies, and description.

**Key Fields**:
- `Source`: Source package name
- `Build-Depends`: Packages needed to build
- `Package`: Binary package name
- `Depends`: Runtime dependencies
- `Description`: Package description

### debian/changelog

Version history in Debian format.

**Format**:
```
package (version) distribution; urgency=level

  * Change description
  * Another change

 -- Maintainer <email>  Date
```

### debian/rules

Executable Makefile that controls the build process.

**Override Targets**:
- `override_dh_auto_configure`: Custom CMake configuration
- `override_dh_auto_test`: Run tests during build
- `override_dh_install`: Custom installation steps

### debian/install

Maps built files to installation locations.

**Format**:
```
source/path destination/path
```

### debian/compat

Specifies debhelper compatibility level (currently 9).

### debian/copyright

License information in machine-readable format.

### debian/hptc-qt-chatbot-agent.desktop

Desktop entry for application menu integration.

**Fields**:
- `Name`: Application name (HP Qt Chatbot)
- `Exec`: Executable path (/usr/bin/qt-chatbot-agent)
- `Icon`: Icon name
- `Categories`: Menu categories

---

## Advanced Usage

### Building with Signatures

To create signed packages (requires GPG key):

```bash
# Build and sign:
dpkg-buildpackage -b

# Will prompt for GPG passphrase
```

### Building Source Package

To create source package for upload to repositories:

```bash
# Build source package:
dpkg-buildpackage -S

# Creates .dsc, .tar.xz, and .changes files
```

### Cross-Compilation

To build for different architectures:

```bash
# Install cross-compilation tools:
sudo apt-get install crossbuild-essential-arm64

# Build for arm64:
dpkg-buildpackage -b -aarm64
```

---

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build Debian Package

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y debhelper cmake qtbase5-dev

      - name: Build package
        run: dpkg-buildpackage -b -uc -us

      - name: Upload artifact
        uses: actions/upload-artifact@v2
        with:
          name: debian-package
          path: ../hptc-qt-chatbot-agent*.deb
```

---

## Repository Setup

### Creating an APT Repository

To distribute packages via APT repository:

1. **Generate Packages file**:
   ```bash
   dpkg-scanpackages . /dev/null | gzip -9c > Packages.gz
   ```

2. **Generate Release file**:
   ```bash
   apt-ftparchive release . > Release
   ```

3. **Sign Release** (optional):
   ```bash
   gpg --clearsign -o InRelease Release
   ```

4. **Add to sources.list**:
   ```
   deb [trusted=yes] http://your-server/repo ./
   ```

---

## Troubleshooting

### Build Failures

**Issue**: Missing build dependencies
```bash
# Install all build dependencies:
sudo apt-get build-dep .
```

**Issue**: CMake configuration fails
```bash
# Check CMake version:
cmake --version

# Upgrade if needed:
sudo apt-get install cmake
```

**Issue**: Qt version mismatch
```bash
# Check Qt version:
qmake --version

# Install specific version:
sudo apt-get install qtbase5-dev=5.15.*
```

### Installation Issues

**Issue**: Dependency conflicts
```bash
# Show why dependencies failed:
sudo apt-get install -f

# Force installation (not recommended):
sudo dpkg -i --force-depends package.deb
```

**Issue**: Already installed
```bash
# Remove old version first:
sudo dpkg -r hptc-qt-chatbot-agent

# Then install new version:
sudo dpkg -i new-package.deb
```

---

## Version Updates

### Updating Package Version

1. **Edit debian/changelog**:
   ```bash
   dch -v 1.2.0-1 "New feature: XYZ"
   ```

   Or manually add entry:
   ```
   hptc-qt-chatbot-agent (1.2.0-1) unstable; urgency=medium

     * New feature: XYZ
     * Bug fix: ABC

    -- Scott Glover <scottgl@gmail.com>  Date
   ```

2. **Update src/version.h**:
   ```cpp
   #define APP_VERSION "1.2.0"
   ```

3. **Rebuild package**:
   ```bash
   dpkg-buildpackage -b -uc -us
   ```

---

## Best Practices

### Package Naming

- Use lowercase package names
- Separate words with hyphens
- Keep names descriptive but concise

### Versioning

- Format: `upstream-version-debian-revision`
- Example: `1.1.0-1`
- Increment debian revision for packaging changes
- Increment upstream version for code changes

### Dependencies

- List only necessary runtime dependencies
- Let dh_shlibdeps auto-detect library dependencies
- Specify minimum versions when needed

### Documentation

- Include all user-facing documentation
- Compress large docs with gzip
- Place in `/usr/share/doc/package-name/`

### Configuration

- Store system config in `/etc/package-name/`
- Store user config in `~/.config/package-name/`
- Don't include secrets in package

---

## Testing

### Linting

Check package for common issues:

```bash
# Install lintian:
sudo apt-get install lintian

# Check package:
lintian ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb

# Check with all tags:
lintian -I ../hptc-qt-chatbot-agent_1.1.0-1_amd64.deb
```

### Installation Test

```bash
# Test in clean environment (Docker):
docker run -it ubuntu:22.04 bash

# Inside container:
apt-get update
apt-get install -y /path/to/package.deb
qt-chatbot-agent --version
```

---

## References

### Official Documentation

- [Debian New Maintainers' Guide](https://www.debian.org/doc/manuals/maint-guide/)
- [Debian Policy Manual](https://www.debian.org/doc/debian-policy/)
- [debhelper Documentation](https://manpages.debian.org/debhelper)

### Tools

- `dpkg-buildpackage`: Build Debian packages
- `dh_make`: Create debian/ directory template
- `lintian`: Check packages for policy violations
- `debuild`: Build and check packages
- `dch`: Edit debian/changelog

---

## License

This packaging is released under the MIT License, same as the qt-chatbot-agent application.

---

## Support

For issues with packaging:
- Check this guide first
- Review Debian packaging documentation
- File an issue on GitHub

For application issues:
- See `TESTING_GUIDE.md`
- See `QUICK_REFERENCE.md`

---

**Last Updated**: 2025-10-08
**Package Version**: 1.1.0-1
**Debian Compatibility**: 9+
