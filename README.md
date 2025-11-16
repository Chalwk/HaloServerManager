# Halo Server Manager

A Java Swing application for managing **Halo PC** and **Halo CE** dedicated SAPP servers with built-in script management
and configuration tools.

![Halo Server Manager](https://img.shields.io/badge/Version-1.0.0-blue.svg)
![Java](https://img.shields.io/badge/Java-11%2B-orange.svg)
![License](https://img.shields.io/badge/License-All%20rights%20reserved-lightgrey.svg)

## Overview

Halo Server Manager is a comprehensive desktop application that simplifies the setup, configuration, and management of
**Halo PC** and **Halo CE** SAPP dedicated servers. It provides an intuitive interface for downloading server files,
editing
configurations, browsing and installing Lua scripts, and launching your dedicated game servers.

## Features

### Server Management

- **One-Click Installation**: Download and extract **Halo PC** or **Halo CE** SAPP dedicated servers automatically
- **File Browser**: Built-in file explorer for server directories with syntax-highlighted text editor
- **Configuration Editing**: Edit all server configuration files (.txt, .bat, .lua) directly within the app
- **Server Launching**: One-click server startup with proper directory context

### Script Management

- **Integrated Script Browser**: Browse 150+ Lua scripts from the official HSP (Halo Script Projects) repository
- **Categories Organized**:
    - **Attractive**: Gameplay enhancements and fun modifications
    - **Custom Games**: Complete game modes and custom gameplay
    - **Utility**: Server administration and management tools
- **One-Click Installation**: Download scripts directly to the appropriate server's Lua folder
- **GitHub Integration**: View scripts on GitHub with direct links

### Professional Interface

- **Tabbed Interface**: Separate tabs for Halo PC, Halo CE, and Script Browser
- **Syntax Highlighting**: Lua script editor with color-coded syntax
- **Persistent Settings**: Remembers installation directories and server configurations
- **Progress Tracking**: Real-time download progress with status updates

## Requirements

- **Java 11** or higher
- Windows 10/11 (primary target)
- Internet connection for downloading servers and scripts

## Installation

### Option 1: Download Pre-built Executable

1. Download `HaloServerManager.exe` from the releases page
2. Run the executable - no installation required
3. The application will create a configuration file in the same directory

### Option 2: Build from Source

```bash
# Clone the repository
git clone https://github.com/Chalwk/HaloServerManager.git
cd HaloServerManager

# Build with Maven
mvn clean package

# The executable will be created at:
# target/HaloServerManager.exe
```

## Usage

### Setting Up a Server

1. **Launch** the Halo Server Manager
2. **Select** either Halo PC or Halo CE tab
3. **Click** "Download & Install" to download the server files
4. **Choose** an installation directory when prompted
5. **Wait** for download and extraction to complete

### Managing Server Files

1. **Browse** server files using the built-in file tree
2. **Double-click** any editable file (.txt, .bat, .lua) to open the editor
3. **Edit** configuration files with syntax highlighting for Lua scripts
4. **Save** changes directly within the application

### Installing Scripts

1. **Navigate** to the "Script Browser" tab
2. **Select** target server (Halo PC or Halo CE)
3. **Choose** a category (Attractive, Custom Games, Utility)
4. **Click** on any script to view its description
5. **Press** "Install Script" to download to the server's Lua folder
6. **Follow** the instructions to add `lua_load "script_name"` to your `init.txt` file

### Launching Your Server

1. **Ensure** your server is installed and configured
2. **Click** the "Launch Server" button
3. **Monitor** the server console window that opens

## Project Structure

```
HaloServerManager/
├── src/main/java/com/chalwk/
│   ├── HaloServerManager.java          # Main application entry point
│   ├── model/                          # Data models
│   │   ├── ScriptCategory.java
│   │   └── ScriptMetadata.java
│   │   ├── ServerConfig.java
│   │   ├── ServerType.java
│   │   ├── UpdateConfig.java
│   ├── service/                        # Business logic services
│   │   ├── DownloadService.java
│   │   ├── FileService.java
│   │   └── ScriptService.java
│   │   ├── ServerService.java
│   │   ├── UpdateService.java
│   ├── ui/                             # User interface components
│   │   ├── FileEditorDialog.java
│   │   ├── MainFrame.java
│   │   ├── UpdateDialog.java
│   │   └── components/
│   │       ├── ScriptBrowserPanel.java
│   │       ├── ServerPanel.java
│   └── util/
│       └── PreferencesManager.java     # Configuration persistence
├── pom.xml                            # Maven build configuration
└── README.md
```

## Building from Source

### Prerequisites

- Java Development Kit (JDK) 11 or higher
- Apache Maven 3.6 or higher

### Build Commands

```bash
# Compile and run tests
mvn clean compile

# Create executable JAR
mvn package

# The build will produce:
# - target/HaloServerManager.jar
# - target/HaloServerManager.exe
```

### Development

```bash
# Import into your favorite IDE as a Maven project
# The main class is: com.chalwk.HaloServerManager
```

## Supported Server Files

The application automatically manages the complete server directory structure:

```
HCE_Server/ or HPC_Server/
├── haloceded.exe (HCE) or haloded.exe (HPC)
├── run.bat
├── sapp.dll
├── motd.txt
├── cg/
│   ├── init.txt
│   ├── sapp/
│   │   ├── commands.txt
│   │   ├── events.txt
│   │   ├── init.txt
│   │   ├── mapcycle.txt
│   │   ├── mapvotes.txt
│   │   └── lua/           # Lua scripts directory
│   └── savegames/
├── maps/                  # All Halo maps
└── sapp/
    ├── admins.txt
    ├── areas.txt
    ├── ipbans.txt
    ├── locations.txt
    └── users.txt
```

## Script Integration

The application integrates with the official [HALO-SCRIPT-PROJECTS](https://github.com/Chalwk/HALO-SCRIPT-PROJECTS)
GitHub repository, featuring:

- **150+ Scripts**: Comprehensive library of server modifications
- **Real-time Updates**: Always access the latest script versions
- **Metadata Integration**: Rich descriptions and categorization
- **Automatic Updates**: Script metadata fetched directly from GitHub

## Troubleshooting

### Common Issues

**Server won't launch:**

- Ensure the installation directory is correct
- Check that all server files were downloaded completely
- Verify run.bat exists in the server directory

**Scripts not working:**

- Confirm scripts are installed to the correct server's `cg/sapp/lua` folder
- Add `lua_load "script_name"` to `cg/sapp/init.txt`
- Remove the `.lua` extension from the script name in the load command

**Download failures:**

- Check your internet connection
- Verify GitHub is accessible from your network
- Ensure you have write permissions to the installation directory

## [License (MIT)](https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE)

© 2025 Halo Server Manager - Jericho Crosby (Chalwk). All rights reserved.

## Credits

**Developed by:** Jericho Crosby (Chalwk)  
**Halo Script Projects:** [GitHub Repository](https://github.com/Chalwk/HALO-SCRIPT-PROJECTS)  
**Support:** Create an issue on the GitHub repository

## Links

- [HALO SCRIPT PROJECTS GitHub](https://github.com/Chalwk/HALO-SCRIPT-PROJECTS)
- [SAPP Official Documentation](https://github.com/Chalwk/HALO-SCRIPT-PROJECTS/blob/master/docs/sapp-2.4.pdf)

---

*Halo Server Manager is not affiliated with Microsoft, Bungie, or 343 Industries. Halo is a registered trademark of
Microsoft Corporation.*