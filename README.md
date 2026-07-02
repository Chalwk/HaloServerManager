# HaloServerManager

A Windows desktop application built with Qt Widgets that downloads, installs, and manages dedicated servers for Halo CE and Halo PC (SAPP). It simplifies server deployment with a graphical interface for installation, launching, stopping, and port configuration.

[![Version][version-badge]][version-link]
[![License: GPL v3][license-badge]][license-link]
![C++17][cpp-badge]
![Qt 6][qt-badge]
![Windows][windows-badge]
![CMake][cmake-badge]

---

### Features

- **One-Click Installation** - Download and extract pre-packaged SAPP server templates (CE and PC) directly from GitHub releases.
- **Server Management** - Launch, stop, and restart dedicated servers with a single click; view real-time running status in the server list.
- **Live Console Output** - See the server's console output **inside the application**.
- **Command Sending** - Type commands into the console input line and send them directly to the running server (e.g., `map`, `pl`, `k`).
- **Configuration Editor** - Edit `init.txt`, `mapcycle.txt`, etc., directly from the UI with syntax highlighting and save support.
- **Auto-Restart** - Enable automatic restart if the server crashes (configurable delay).
- **Port Configuration** - Easily change the server port via the Settings tab; settings are persisted.
- **Multiple Servers** - Keep track of several installed server instances; select which one to control.
- **System Tray** - Minimize to tray with a context menu for quick actions; double-click to restore the window.
- **Data Persistence** - User configuration (installed servers, port, auto-start/restart flags) is saved locally in `%APPDATA%\HaloServerManager\config.json`.

---

## Requirements

- **Operating System:** Windows 10 version **1809 (build 17763)** or later (Windows 11 fully supported).  
  > The application uses the Windows Pseudo Console (ConPTY) API to capture server output. This API was introduced in Windows 10 1809.
- **Visual Studio Build Tools 2022 or 2026** with the *Desktop Development with C++* workload.
- **CMake 3.24 or newer**.
- **Qt 6.x** (MSVC 2022 64-bit build).
- **NSIS** (optional, for creating the installer).

---

## Getting Started

You have two options to get the application:

1. **Download the installer (recommended)**
   Grab the latest `HaloServerManagerSetup.exe` from the [Releases page][releases].  
   The installer will:
   - Install the application to `C:\Program Files\HaloServerManager`.
   - Create a Desktop shortcut and a Start Menu folder with both application and uninstall shortcuts.
   - Register the application in Windows **Add/Remove Programs** for easy uninstallation.

2. **Build from source** - follow the instructions below.

---

## Build & Package

### Automated (recommended)
Run `build.bat` from the project root. It will clean, configure, build, package dependencies, and generate the NSIS installer.

### Manual steps
```cmd
rmdir /s /q build
cmake -S . -B build -DQt6_DIR="C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6"
cmake --build build --config Release
package.bat build
cd installer
makensis HaloServerManager.nsi
```

> Adjust the Qt path in `build.bat` and `package.bat` if needed.  
> `HaloServerManagerSetup.exe` will appear in the `/installer/` directory.

---

## License

This project is licensed under the **GNU General Public License Version 3, 29 June 2007**.  
Copyright © 2026 Jericho Crosby (Chalwk). See the [LICENSE](LICENSE) file for details.

---

[releases]: https://github.com/Chalwk/HaloServerManager/releases
[version-badge]: https://img.shields.io/github/v/release/Chalwk/HaloServerManager?label=Version&display_name=tag
[version-link]: https://github.com/Chalwk/HaloServerManager/releases/latest
[license-badge]: https://img.shields.io/github/license/Chalwk/HaloServerManager
[license-link]: https://github.com/Chalwk/HaloServerManager/blob/main/LICENSE
[cpp-badge]: https://img.shields.io/badge/C%2B%2B-17-blue.svg
[qt-badge]: https://img.shields.io/badge/Qt-6-green.svg
[windows-badge]: https://img.shields.io/badge/Platform-Windows-0078D6
[cmake-badge]: https://img.shields.io/badge/CMake-3.24%2B-064F8C