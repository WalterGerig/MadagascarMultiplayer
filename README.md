# Madagascar Multiplayer (PC 2005) 🎮🌐

[![Platform](https://img.shields.io/badge/Platform-PC%20%7C%20Windows%20x86-blue.svg)](https://github.com)
[![Engine](https://img.shields.io/badge/Engine-RenderWare%203.7%20%7C%20DirectX%208.1-brightgreen.svg)](https://github.com)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20MSVC%20Win32-orange.svg)](https://github.com)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

A native 2-player multiplayer and modernization mod for **Madagascar (2005) PC** (_Game.exe_, RenderWare 3.7 / DirectX 8.1).

This project injects a custom C++ runtime (`MadMultiplayer.dll`) into the game process to enable synchronized multiplayer gameplay, in-game Dear ImGui menus, 16:9 borderless widescreen, camera culling fixes, and memory inspection.

---

## ✨ Features

- **🎮 In-Game ImGui Overlay (`F3`):**
  - **Multiplayer Lobby:** Server IP/Port connection panel, player status, ping display.
  - **Live Player Memory Inspector:** Real-time monitoring of Position (X, Y, Z), Rotation, Velocity, Health, and Coins (`0x00400000` base).
  - **Debug & Engine Logs:** Thread-safe real-time console buffer with log export and ANSI-safe rendering.
- **🖥️ 16:9 Borderless Widescreen (`Key 9`):**
  - Instant toggle to native borderless desktop resolution (e.g. 1920x1080 / 2560x1440) without stretching.
  - Matrix projection scaling preserving true aspect ratio.
- **👁️ RenderWare Frustum Culling Fix:**
  - Dynamically recalculates `RwCamera` view windows (`viewWindow.x`, `recipViewWindow.x`) and triggers `_rwCameraSetFrustum (+0x10)` in real-time, preventing objects at screen edges from popping out.
- **🖱️ UI Mouse Scaling & Capture (`F2`):**
  - Seamlessly unlocks the cursor for UI interactions (`F2`) or locks it back to the game.
  - Translates monitor-space mouse coordinates to 800x600 space so pause menu buttons are clickable right where they appear.
- **🎬 2D Cutscene Letterbox Filtering:**
  - VTable interceptor on `DrawPrimitive` / `DrawPrimitiveUP` detecting and suppressing obsolete 800x600 black border quads during in-engine cutscenes.
- **⚙️ Config System (`multiplayer_config.ini`):**
  - Custom hotkeys, network defaults, and startup preferences saved in `patches/multiplayer_config.ini`.
- **🔌 Proxy Loader Included (`d3d8.dll`):**
  - Fully self-contained D3D8 proxy loader automatically loads `patches/MadMultiplayer.dll` on game startup—no external injector required.

---

## 📁 Repository Structure

```
MadagascarMultiplayer/
├── CMakeLists.txt                 # Root CMake build file (builds both client & loader)
├── build.bat                      # One-click Windows build script (CMake Win32 Release)
├── deploy.bat                     # Automatic deployment to Game.exe directory / VirtualStore
├── client/                        # Core Multiplayer Mod DLL (MadMultiplayer.dll)
│   ├── CMakeLists.txt
│   ├── include/                   # MemoryManager, D3D8Hook, Logger, Config, NetworkProtocol
│   ├── src/                       # Hook implementations, RenderWare patches, UI
│   └── vendor/
│       ├── d3d8/                  # d3d8_minimal.h (No DirectX SDK required!)
│       └── imgui/                 # Dear ImGui + Win32/DX8 backends
├── loader/                        # Proxy DLL Loader (d3d8.dll)
│   ├── CMakeLists.txt
│   └── src/                       # Proxy export wrappers and patch loader
├── server/                        # Standalone Python UDP Relay Server (60 Hz heartbeat)
│   └── relay_server.py
├── tools/                         # Debugging and validation scripts
│   ├── packet_tester.py           # UDP packet simulation
│   └── phase1_memory_validator.py # Real-time player memory monitor
├── config/                        # Default configuration template
│   └── multiplayer_config.ini
└── docs/                          # Architecture blueprints & reversing documentation
    ├── ARCHITECTURE.md
    ├── CHANGELOG.md
    ├── 01_memory_and_dll_setup.txt
    └── 02_directx8_imgui_hook.txt
```

---

## 🛠️ Prerequisites & Automatic Setup

### ⚡ Option A: Automatisches Setup (Empfohlen)

Um CMake und alle Python-Entwicklungswerkzeuge automatisch einzurichten, führe einfach aus:

```bat
.\install_requirements.bat
```

_Oder installiere die Python-Abhängigkeiten direkt via pip:_

```bash
pip install -r requirements.txt
```

### 📋 Option B: Manuelle Voraussetzungen

Falls du die Tools manuell installieren möchtest:

1. **Windows 10 or 11 (64-bit)**
2. **Visual Studio 2019 oder 2022** (Community oder Build Tools)
   - Workload: _Desktop development with C++_
   - Component: _MSVC C++ x64/x86 build tools_
3. **CMake 3.20 oder neuer** (kann über `pip install -r requirements.txt` oder `winget install Kitware.CMake` installiert werden)
4. **Python 3.10+** (für Server, Tester und Memory-Validator)

> [!NOTE]
> Das DirectX 8 / 9 SDK wird **nicht** benötigt. Alle Schnittstellen sind autark in `client/vendor/d3d8/d3d8_minimal.h` enthalten.

---

## 🔨 Building

### Option 1: One-Click Build Script

Double-click `build.bat` in the root folder, or run in PowerShell/CMD:

```bat
.\build.bat
```

### Option 2: Command Line (CMake)

```powershell
# 1. Configure the build directory for 32-bit x86
cmake -B build -A Win32

# 2. Compile Release binaries
cmake --build build --config Release
```

The resulting binaries will be placed in:

- `build/client/Release/MadMultiplayer.dll` (Multiplayer Mod DLL)
- `build/loader/Release/d3d8.dll` (Proxy Loader)

---

## 🚀 Installation & Playing

1. **Automatic Deployment:**
   Run `deploy.bat`. It will locate your game installation and copy the binaries to both the game folder and Windows VirtualStore automatically.

2. **Manual Installation:**
   - Copy `build/loader/Release/d3d8.dll` into your Madagascar game folder (next to `Game.exe`).
   - Create a `patches` subfolder inside the game folder (e.g. `Madagascar\Game\patches\`).
   - Copy `build/client/Release/MadMultiplayer.dll` and `config/multiplayer_config.ini` into `patches\`.

> [!IMPORTANT]
> **Windows UAC VirtualStore Notice:**  
> If Madagascar is installed in `C:\Program Files (x86)\...` and run without elevation, Windows redirects file reads/writes to:  
> `%LOCALAPPDATA%\VirtualStore\Program Files (x86)\Activision\Madagascar\Game\`  
> `deploy.bat` handles this automatically.

---

## ⌨️ Controls & Keybinds

|  Key   | Function              | Description                                                         |
| :----: | :-------------------- | :------------------------------------------------------------------ |
| **F3** | **Toggle Overlay**    | Shows or hides the ImGui multiplayer menu                           |
| **F2** | **Toggle Mouse Mode** | Frees mouse cursor for UI interaction or locks it for gameplay      |
| **9**  | **16:9 Widescreen**   | Toggles between original 4:3 800x600 and borderless 16:9 widescreen |

_Keybinds can be customized in `patches/multiplayer_config.ini` using Win32 virtual-key codes._

---

## 🌐 Running the Relay Server

To host a multiplayer game session:

```powershell
python server/relay_server.py --port 27015
```

Players can then open the in-game overlay (`F3`) and connect to the host's IP and port.

---

## 📜 Memory Offsets (`Game.exe` Base `0x00400000`)

| Entity / Structure    | Address / Offset            | Description                               |
| --------------------- | --------------------------- | ----------------------------------------- |
| `Player 1 Pointer`    | `0x0062AC18` / `0x00609D80` | Current active character object           |
| `Position X, Y, Z`    | `+0x44`, `+0x48`, `+0x4C`   | IEEE-754 32-bit floats                    |
| `Yaw Rotation`        | `+0x58`                     | Character facing angle                    |
| `Health`              | `+0x184`                    | 32-bit integer                            |
| `RwCamera`            | `RwGlobals + 0x00`          | Active RenderWare camera                  |
| `viewWindow.x/y`      | `Camera + 0x68 / 0x6C`      | Frustum view window coordinates           |
| `_rwCameraSetFrustum` | `Camera + 0x10`             | RenderWare frustum recalculation function |

Detailed reverse-engineering notes are documented in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

---

## 📄 License

This project is licensed under the MIT License. Reverse engineering is conducted for educational, preservation, and interoperability purposes. All rights to Madagascar belong to Activision / DreamWorks Animation.
