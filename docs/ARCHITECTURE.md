# Madagascar (2005) Multiplayer Architecture & Technical Specification

## 1. Übersicht & Zielsetzung
Entwicklung eines nativen 2-Spieler-Multiplayer-Systems (Client-DLL `MadMultiplayer.dll` für `Game.exe` und Standalone-Relay-Server) für die deutsche Retail-Version von **Madagascar (2005)** (PC / RenderWare 3.7 / DirectX 8.1 / Win32 x86).

Grafische Anpassungen (wie Ultrawide/Widescreen) sind explizit aus dem Scope ausgegliedert und laufen in autarken Modulen. Der Fokus liegt zu 100 % auf:
- x86 Memory Manipulation & Pointer Chains (Safe Dereferencing, SEH / IsBadReadPtr)
- Direct3D 8 VTable Hooking (`IDirect3DDevice8::EndScene`) & Dear ImGui Integration
- Win32 WndProc Subclassing & Input Dispatching
- Asynchrones Non-blocking Winsock2 UDP Netzwerk-Subsystem
- Kompaktes binäres Übertragungsprotokoll (`PlayerStatePacket`)
- RenderWare 3.7 Entity Management (RwFrame, RpClump, RpAtomic Transformationen)
- Client-seitige Interpolation (Dead Reckoning & Lerp)

---

## 2. Reversierte Speichermodelle & Offsets (Deutsche Game.exe)

| Parameter / Funktion | Offset / Adresse | Typ / Größe | Beschreibung |
|---|---|---|---|
| **Player Base Pointer** | `0x12BF49D4` | `uintptr_t*` (Heap) | Zeiger auf die aktive Spieler-Instanz (Alex, Marty, Melman, Gloria) |
| **X-Koordinate** | Base + `+0x1F4` | `float` (4 Bytes) | World-Space Horizontal (X) |
| **Y-Koordinate** | Base + `+0x1F8` | `float` (4 Bytes) | World-Space Vertikal / Höhe (Z-Height im RW-Koordinatensystem) |
| **Z-Koordinate** | Base + `+0x1FC` | `float` (4 Bytes) | World-Space Tiefe / Horizontal (Z) |
| **Physics Overwrite Patch** | `0x00428E9C` | `fstp dword ptr [ebp+1F8h]` | Engine-Schreibzugriff auf Y-Position. Bei Override via `0x90` (NOP) zu patchen. |
| **Kamera Pitch** | `0x006181FC` | `float` | Neigungswinkel der Kamera |
| **Kamera Yaw** | `0x00618220` | `float` | Horizontale Drehung / Ausrichtung |

---

## 3. Modulare Schichten-Architektur

```
+-------------------------------------------------------------------------+
|                                GAME.EXE                                 |
|                                                                         |
|  +-------------------+     +------------------+     +----------------+  |
|  | D3D8 Device Hook  |     | Memory Subsystem |     | RenderWare 3.7 |  |
|  | (EndScene/ImGui)  |     | (Reader/Writer)  |     | Entity Cloner  |  |
|  +---------+---------+     +--------+---------+     +-------+--------+  |
|            |                        |                       |           |
|            +-------------------+    |    +------------------+           |
|                                |    |    |                              |
|                       +--------v----v----v-------+                      |
|                       |   MadMultiplayer.dll     |                      |
|                       |  Core Client Controller  |                      |
|                       +------------+-------------+                      |
+------------------------------------|------------------------------------+
                                     |
                       Winsock2 UDP  | Binary Packets
                        (Non-blocking| Port 27015)
                                     v
                       +---------------------------+
                       | Standalone Relay Server   |
                       | Session / State Broadcast |
                       +---------------------------+
```

### Schicht 1: Memory Subsystem (`LocalPlayer`)
- Thread-sichere Pointer-Validierung (Null-Checks + SEH Guard).
- Extraktion von Position (`Vector3`), Rotation (`Yaw`), Charakter-ID und Animation/State.
- NOP-Sicherheits-Patching bei direktem State-Override.

### Schicht 2: Direct3D8 VTable Hook & ImGui Overlay
- Einhängen in `IDirect3DDevice8::EndScene` über VTable Swap / Detour.
- Renderung von Dear ImGui mit DirectX 8 Backend.
- Subclassing von `WndProc` via `SetWindowLongPtrA(hWnd, GWLP_WNDPROC, ...)` mit Hotkey-Umschaltung (`F2`).
- Blockieren von `WM_MOUSE*` und `WM_KEY*` Nachrichten an die Spielfigur, solange das Overlay geöffnet ist.

### Schicht 3: Netzwerk-Infrastruktur & Relay-Server
- Binäres Protokoll ohne Padding (`#pragma pack(push, 1)`):
  ```cpp
  #pragma pack(push, 1)
  struct PlayerStatePacket {
      uint8_t  packetType;      // 0x01: PositionUpdate, 0x02: Heartbeat, 0x03: ConnectReq, 0x04: Disconnect
      uint32_t playerId;        // Session-ID
      uint32_t sequenceNumber;  // Monoton steigender Tick-Zähler
      uint8_t  characterId;     // 0 = Alex, 1 = Marty, 2 = Melman, 3 = Gloria
      float    position[3];     // X, Y, Z
      float    rotation;        // Yaw
  };
  #pragma pack(pop)
  ```
- Non-blocking UDP-Sockets, Timeout-Handling, Paket-Reordering-Erkennung über `sequenceNumber`.

### Schicht 4: Remote Entity Spawning & Interpolation
- Klonen von `RpClump` / `RwFrame` Instanzen zur Repräsentation des Mitspielers.
- Lineare Interpolation (Lerp) zwischen empfangenen Ticks zur Kompensation von Jitter und Paketverlusten.
- Dead Reckoning für flüssige 60 FPS Darstellung ohne Ruckler.

---

## 4. Projekt-Struktur-Planung

```
f:/Madagascar Multiplayer/
├── docs/                      # Dokumentation, Changelogs, Memory-Dumps
│   ├── ARCHITECTURE.md        # Master Architektur-Dokument
│   └── CHANGELOG.md           # Fortlaufendes Änderungsprotokoll
├── client/                    # C++20 MadMultiplayer.dll
│   ├── include/               # Header-Dateien (Memory, Hook, Net, RW)
│   ├── src/                   # Quellcodedateien
│   └── vendor/                # Dear ImGui, D3D8 SDK Wrappers
├── server/                    # Standalone Relay Server (Python / C#)
└── tools/                     # Hilfsskripte (Memory Validator, Packet Tester)
```
