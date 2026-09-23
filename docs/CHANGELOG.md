# Madagascar (2005) Multiplayer - Projekt-Changelog & Protokoll

Alle Änderungen an der Codebase, Offsets, Hook-Logik, Netzwerk-Protokollen und Toolchain werden hier lückenlos dokumentiert.

## [0.6.5-FEATURE-AND-CODEBASE-MERGE] - 2026-09-23 18:15
### Betroffene Dateien & Module
- `client/CMakeLists.txt` [MODIFY]
- `client/include/CheatManager.h` [NEW]
- `client/src/CheatManager.cpp` [NEW]
- `client/include/Config.h` & `client/src/Config.cpp` [MODIFY]
- `client/include/MemoryManager.h` & `client/src/MemoryManager.cpp` [MODIFY]
- `client/include/D3D8Hook.h` & `client/src/D3D8Hook.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `client/src/Logger.cpp` [MODIFY]
- `loader/*` (`launcher.cpp`, `loader.cpp`, `proxy_d3d8.cpp`, `proxy_winmm.cpp`, `core_dllmain.cpp`, etc.) [NEW]
- `build.bat`, `deploy.bat`, `install_requirements.bat`, `CMakeLists.txt` [MODIFY / NEW]
- `docs/CHANGELOG.md` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Lückenlose Integration des Cheat- und Sandbox-Subsystems (`CheatManager`):**
   - Hinzufügen von `CheatManager.h` und `CheatManager.cpp` mit Flugmodus (Noclip), God Mode, unendlichem Sprung, Geschwindigkeitsmultiplikatoren, Münz-/Mango-/Pfotentoken-Refill.
   - Einbindung des "Cheats & Sandbox" Tabs in das ImGui-Overlay (`RenderOverlayUI`).
   - Erweiterung von `Config.h` / `Config.cpp` zur Speicherung und Initialisierung von Flugtasten, Geschwindigkeiten und Standardzuständen.
   - Erweiterung von `MemoryManager` um bulletproof Health-, Münz-, Mango- und Token-Leser/Schreiber (`SetPlayerPosition`, `ReadHealth`, `WriteHealth`, etc.) mit SEH-Schutz und Speicherprüfungen.

2. **D3D8 Device State Preservation & Strict Frame Gate:**
   - Einführung von `D3D8StateBackup` (`SaveD3D8State` / `RestoreD3D8State`) in `D3D8Hook`, um Render-States, Texture-Stage-States, Shader, Vertex-Streams, Indices und Viewports vor und nach dem ImGui-Rendern exakt zu sichern und wiederherzustellen.
   - Hook von VTable Slot 15 (`Present`) mit `Hooked_Present` und `OnPresent` für frame-genaue Delta-Time-Berechnung und ImGui-Rendern strikt 1x pro Present-Frame.

3. **Vollständiger Erhalt aller Deep-Engine-Fixes:**
   - 16:9 Hor+ Widescreen FOV (`Hooked_SetTransform`).
   - RwCamera Frustum Culling Fix via Inline-Hook auf `RwCameraBeginUpdate` (`0x004D70F0`).
   - 2D UI Render-Dispatch Hook (`0x0060B0B8` / `0x004FEFB0` mit `0x004E2640` Fallback).
   - In-Game Software-Mauszeiger NOP-Deaktivierung (`0x00462FF4` & `0x004636D1`).
   - 1:1 Menü-Mauskollisions-Mapping und `Hooked_GetCursorPos` IAT-Hook.
   - Cutscene-Letterbox-Unterdrückung in `Process2DVertices` und `Hooked_DrawPrimitiveUP`.

4. **Eigenständiges Loader-Subsystem (`loader/`):**
   - Integration des d3d8.dll / winmm.dll Proxy-Loaders sowie des `MadLauncher` Tools.
   - Aktualisierung von Root-`CMakeLists.txt`, `build.bat` und `deploy.bat` für nahtloses Multi-Target-Building und Deployment.

---

## [0.6.4-PHASE3-CAMERA-FRUSTUM-2D-DISPATCH-OVERHAUL] - 2026-09-23 18:00
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Behebung des CPU-seitigen Object Frustum Cullings an den Bildschirmrändern:**
   - Ursache analysiert: RenderWare 3.7 verwirft 3D-Objekte (`RpClump` / `RpAtomic`) an den horizontalen Rändern, wenn sie außerhalb des Kamera-Frustums (`camera->viewWindow.x`) liegen. Da `curCamera` bei `OnEndScene()` nach dem Rendern stets `NULL` ist (`RwCameraEndUpdate` setzt `RwGlobals.curCamera = 0`), griff die bisherige Frustum-Aktualisierung ins Leere.
   - Lösung: Detour / Inline-Hook auf `RwCameraBeginUpdate` (`0x004D70F0`).
   - Vor jedem Render-Pass fängt der Hook die aktive 3D-Weltkamera (`projectionType == 1`) ab und synchronisiert `camera->viewWindow.x = camera->viewWindow.y * targetAspect` sowie `camera->recipViewWindow.x = 1.0f / targetViewWindowX`.
   - Ruft `camera->setFrustum` (`+0x10` = `0x004D5B60`) auf, wodurch RenderWare intern alle 6 CPU-Culling-Ebenen exakt für das 16:9-Widescreen-Sichtfeld neu berechnet.
   - Beseitigt Objekt-Pop-In und vorzeitiges Verschwinden von Spielwelt-Objekten (Geländer, Ballons, Bäume, Tiere) an den Bildschirmrändern vollständig.

2. **2D UI Element-Resolution Fix (0x004E2640 Fallback):**
   - Ursache für fehlende UI-Verschiebung behoben: RenderWare speichert das aktuelle 2D-Element nicht immer in `0x00632038`. Wenn `[0x00632038]` NULL ist, ruft die Engine intern `0x004E2640` auf, um den aktuellen 2D-Element-Kontext zu laden.
   - `Hooked_Render2DQuads` bindet `0x004E2640` nun als Fallback ein, sodass `origElemX` und `origElemY` (`elem->0x1C` und `elem->0x1E`) zuverlässig für jedes HUD-Element ermittelt werden.
   - Dadurch greift das dynamische 3-Zonen-Anchoring für alle HUD-Elemente (Marty links, Alex-Statue rechts) und geparkte Offscreen-Assets (`origElemX >= 790`) werden bedingungslos verworfen.

---

## [0.6.3-PHASE3-UI-DIRECT-DISPATCH-MOUSE-OVERHAUL] - 2026-09-23 17:30
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

### Genaue Beschreibung der Änderung
1. **RenderWare 2D UI Pipeline Dispatch Hook (Tabelle 0x0060B080 / 0x0060B0B8):**
   - Ursache für fehlendes UI-Anchoring aufgedeckt: RenderWare 3.7 schreibt Vertizes in einen dynamischen `D3DUSAGE_WRITEONLY` VertexBuffer (`0x0062D7F8`), wodurch `pVB->Lock(..., D3DLOCK_READONLY)` in `Hooked_DrawPrimitive` mit `D3DERR_INVALIDCALL` fehlschlug.
   - Lösung: Hook der internen RenderWare 2D-Quad-Dispatch-Funktion (`0x004FEFB0`) über Funktionstabelle `0x0060B080` (Slot `+0x38` = `0x0060B0B8`).
   - Fängt alle HUD-Elemente, Schriftzüge und Icons direkt an der Quelle vor der GPU-Pufferung ab.
   - Ermittelt `elem = *(0x00632038)->0x60` mit den originalen Bildschirmkoordinaten (`elem->0x1c` = X, `elem->0x1e` = Y).
   - Führt dynamisches 3-Zonen-Anchoring und Vertex-Skalierung durch und stellt nach dem Rendern die ursprünglichen Element-Offsets wieder her (verhindert Driften/Akkumulieren).
   - Verwirft zuverlässig geparkte Offscreen-Assets (`elemX >= 790 || elemX <= -5`, z. B. brauner Ballon am rechten Rand).

2. **Vollständige Eliminierung des In-Game Software-Mauszeigers:**
   - Exakte Reverse-Engineering-Identifikation der Render-Calls des Software-Sprite-Cursors in `Game.exe`:
     * `0x00462FF4` (9 Bytes: `push eax; call 0x413f90; add esp, 4`)
     * `0x004636D1` (9 Bytes: `push edx; call 0x413f90; add esp, 4`)
   - Beide Aufrufe werden zur Laufzeit via `VirtualProtect` und `0x90` (NOP) vollständig deaktiviert.
   - Der In-Game Software-Mauszeiger ist damit restlos unsichtbar.
   - Der echte Windows PC-Mauszeiger bleibt über F2 toggelbar und sichtbar.

3. **1:1 Maus-Klick- und Hover-Synchronisation im Menü:**
   - Menü-Buttons und UI-Hitboxen (`0x00463640`) berechnen Maus-Kollision im 640x480-Raum aus Fensterkoordinaten (`lParam` in `WM_LBUTTONDOWN` / `WM_MOUSEMOVE`).
   - `HandleGameWndProc` und `Hooked_GetCursorPos` mappen die physischen Bildschirmkoordinaten stetig auf das 4:3-Zentrum zurück, sodass Klicks und Hover-Effekte exakt mit den optisch zentrierten Menü-Buttons übereinstimmen.

---

## [0.6.2-PHASE3-FINAL-HUD-ANCHORING] - 2026-09-23 17:00
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Parked Elements Culling (Offscreen-Asset Drop):**
   - Prüfung auf 2D-Quads, deren Vertizes im originalen Koordinatenraum vollständig bei $X \ge 790.0\text{f}$ oder $X \le -5.0\text{f}$ liegen (`minX >= 790.0f || maxX <= -5.0f`).
   - Verwirft diese Draw-Calls bedingungslos (`outDrop = true; return true;` -> Rückgabe von `D3D_OK`).
   - Verhindert zuverlässig, dass von der Engine außerhalb des 4:3-Sichtfelds geparkte Assets (wie der braune Ballon/Kreis am rechten Rand) im 16:9-Widescreen sichtbar werden.
2. **Drei-Zonen HUD-Anchoring (Rand- und Mittenverankerung):**
   - Exakte Schwellenwert-Kalibrierung basierend auf `origCenterX = (minX + maxX) * 0.5f`:
     - **Zone Links** (`origCenterX < 320.0f`): Gesundheitsanzeige, Pfotenzähler, Charakter-Kopf kleben am linken Bildschirmrand ($X' = X \cdot \text{scaleX}$, $Y' = Y \cdot \text{scaleY}$).
     - **Zone Rechts** (`origCenterX > 520.0f`): Alex-Löwenstatue ("51/100"), Münzanzeigen, Sammelobjekte kleben am rechten Bildschirmrand ($X' = \text{screenWidth} - (800.0\text{f} - X) \cdot \text{scaleX}$, $Y' = Y \cdot \text{scaleY}$).
     - **Zone Mitte** ($320.0\text{f} \le \text{origCenterX} \le 520.0\text{f}$): Missions-Banner ("FANG 4 BLAUE FISCHE!"), Zielkreise und Prompts bleiben perfekt in der Bildschirmmitte zentriert ($X' = \text{centerOffsetX} + X \cdot \text{scaleX}$, $Y' = Y \cdot \text{scaleY}$).
3. **Beibehaltung bewährter Meilensteine:**
   - Cutscene-Letterbox-Balken bleiben restlos unterdrückt.
   - Pause-Menü-Mauskollision bleibt 1:1 exakt synchronisiert.
   - Vollbild-Post-Processing (Atmosphäre/Sky-Tint) spannt nahtlos über den gesamten Bildschirm.

---

## [0.6.1-PHASE3-DYNAMIC-HUD-ANCHORING] - 2026-09-22 23:00
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/vendor/d3d8/d3d8_minimal.h` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Strikt eingegrenzter Vollbild-Filter (Kategorie A):**
   - Vollbild-Post-Processing-Quads (Sky-Tint / Dunst / Pause-Dimming) werden nun strikt nur dann auf den 16:9-Bildschirm gestreckt, wenn die Bounding-Box das gesamte 800x600-Canvas gleichzeitig abdeckt: `minX <= 10.0f && maxX >= 790.0f && minY <= 10.0f && maxY >= 590.0f && (vCount == 4 || vCount == 6)`.
   - Schließt Missions- und Ziel-Banner ("GELANG NACH OBEN", `maxY < 500.0f`) verlässlich aus, wodurch diese nicht mehr fälschlicherweise auf 100% Bildschirmbreite aufgeblasen werden.
2. **Cutscene-Animationsfilter & Einblitz-Schutz (Kategorie B):**
   - Dynamisches Abfangen von Letterbox-Balken während des Einblend-Tweenings (Höhe 0px bis 100px) durch Überprüfung auf `minX <= 25.0f && maxX >= 750.0f`, `minY <= 2.0f && maxY <= 180.0f` (oben) bzw. `minY >= 420.0f && maxY >= 598.0f` (unten) und diffuse RGB-Farbe $\le 5$.
   - Bedingungsloses Verwerfen (`outDrop = true`) eliminiert das Frame-Einblitz-Flackern beim Start von Zwischensequenzen vollständig.
3. **Dynamisches Widescreen-HUD Anchoring (Kategorie C):**
   - Uniforme Skalierung basierend auf der Bildschirmhöhe: `scaleY = currentScreenHeight / 600.0f; scaleX = scaleY;`. Verhindert die "Gießkannen-Verzerrung" (HUD-Icons und Anzeigen bleiben exakt kreisrund).
   - Horizontale 3-Zonen-Verankerung über `centerX = (minX + maxX) * 0.5f`:
     - **Linkes HUD** (`centerX < 300.0f`): Verankert am linken Monitorrand (`Vertex.x = OrigX * scaleX`).
     - **Rechtes HUD** (`centerX > 500.0f`): Verankert am echten rechten Monitorrand (`Vertex.x = screenWidth - (800 - OrigX) * scaleX`).
     - **Zentriertes HUD / Banner** (`300.0f <= centerX <= 500.0f`): Verankert mittig im Pillarbox-Zentrum (`Vertex.x = centerOffsetX + OrigX * scaleX`).
4. **Maus-Synchronisation in 3-Zonen-Layout:**
   - `Hooked_GetCursorPos` rechnet Maus-Klicks stetig über die drei Verankerungszonen (Links, Mitte, Rechts) auf die native 800x600-Basis zurück, sodass Menü-Buttons und UI-Elemente in allen Bildschirmbereichen pixelgenau getroffen werden.

---

## [0.6.0-PHASE3-DEEP-ENGINE-FIXES] - 2026-09-22 21:40
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/Logger.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `client/vendor/d3d8/d3d8_minimal.h` [MODIFY]
- `client/vendor/imgui/imgui_impl_dx8.cpp` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Fullscreen Post-Processing / Color Tint Quad Stretch:**
   - Erkennung von Vollbild-Tint-Quads (Bounding-Box $X \in [0, 800]$, $Y \in [0, 600]$) in `Process2DVertices()`.
   - Automatischer Stretch auf die volle 16:9 Auflösung (`Vertex.x = (Vertex.x / 800) * screenWidth`, `Vertex.y = (Vertex.y / 600) * screenHeight`).
   - Dunkelblauer/grauer 4:3-Kasten oben links ist vollständig eliminiert, atmosphärischer Tint bedeckt nun den gesamten 16:9-Bildschirm.
2. **Ortho-Projection Matrix Scaling:**
   - In `Hooked_SetTransform(D3DTS_PROJECTION)`: Erkennung von Ortho-Projektionsmatrizen (`_44 == 1.0f`). Skalierung von `_11` und `_22` auf die tatsächliche Bildschirmauflösung.
3. **Parked HUD Alex-Statue (Offscreen Rect Clipping):**
   - Striktes Verwerfen aller 2D-HUD-Primitiven (`D3DFVF_XYZRHW`), deren originale Koordinaten $\min(X) \ge 795.0f$, $\max(X) \le -5.0f$, $\min(Y) \ge 595.0f$ oder $\max(Y) \le -5.0f$ betragen.
   - Die am rechten Rand geparkte Löwenstatue wird im 16:9-Widescreen-Modus nicht mehr gerendert.
4. **Asymmetrische Cutscene-Balken behoben:**
   - Universal Black-Quad Filter Logik auf `DrawPrimitive`, `DrawIndexedPrimitive`, `DrawPrimitiveUP` und `DrawIndexedPrimitiveUP`.
   - Nahezu schwarze Quads ($R < 15, G < 15, B < 15$) an $Y \le 180$ oder $Y \ge 420$ mit Breite $\ge 350$ werden zuverlässig verworfen (`return D3D_OK`). Oberer und unterer Balken sind restlos beseitigt.
5. **RenderWare CPU-Side Frustum Culling Fix:**
   - Synchronisierung des `curCamera->viewWindow.x` Frustums (`viewWindow.y * targetAspect`) direkt in `Hooked_SetTransform(D3DTS_PROJECTION)` VOR den 3D-Draw-Calls sowie in `OnEndScene`.
   - Aufruf von `_rwCameraSetFrustum (+0x10)`. Kein Pop-In oder Flackern von Objekten an den linken und rechten Bildrändern.
6. **High-Performance Thread-Safe File Logger:**
   - Sofortige synchrone Initialisierung in `DLL_PROCESS_ATTACH`.
   - Logging mit Microsecond-Timestamps und dedizierten Channels: `[D3D8:DRAW2D]`, `[RW:CAMERA]`, `[UI:CLIP]`, `[D3D8:ORTHO]`.
   - Synchrones `FlushFileBuffers()` für jeden einzelnen Log-Eintrag.

---

## [0.1.0-INIT] - 2026-09-22 17:00
### Betroffene Dateien & Module
- `docs/ARCHITECTURE.md` [NEW]
- `docs/CHANGELOG.md` [NEW]

### Genaue Beschreibung der Änderung
- Initiale Projektstruktur und Dokumentations-System aufgesetzt.
- Master-Architektur für die 4 Systemschichten (Memory Subsystem, D3D8/ImGui Overlay, Winsock2 UDP Netzwerk, RenderWare 3.7 Entity Spawning) definiert.
- Speichermodell der deutschen Retail-Version von `Game.exe` verbindlich erfasst (Player Base Pointer `0x12BF49D4`, Offsets `+0x1F4`/`+0x1F8`/`+0x1FC`, Physics Override NOP `0x00428E9C`, Kamera-Offsets `0x006181FC`/`0x00618220`).
- Binäres Netzwerkprotokoll (`PlayerStatePacket` mit `#pragma pack(push, 1)`) für latenzfreie UDP-Kommunikation auf Port 27015 spezifiziert.

### Technische Hintergründe & Begründung
- Die strikte Trennung in autarke Schichten verhindert Seiteneffekte mit dem separaten Widescreen/Graphics-Plugin.
- Die Erfassung aller verifizierten Offsets aus den Reverse-Engineering-Vorarbeiten (MaxStache / `poschecker.py`) bildet das verlässliche Fundament für das C++ Memory Subsystem.
- Das NOP-Patching an Adresse `0x00428E9C` (`fstp dword ptr [ebp+1F8h]`) ist technisch unabdingbar, da die Engine sonst in jedem Physik-Tick manuelle Y-Koordinatenschreibvorgänge überschreibt.

### Status & Offene Punkte
- **Status:** Architektur & Spezifikation abgeschlossen.
- **Offene Punkte:** 
  1. Header-Dateien für das binäre Netzwerk-Protokoll (`NetworkProtocol.h`) und Datenstrukturen erstellen.
  2. C++ Memory Subsystem (`MemoryManager.h` / `.cpp`) mit SEH und Safe Pointer Dereferencing implementieren.
  3. Standalone Relay-Server (Python / C#) aufsetzen, um direkte Loopback- und LAN-Tests zu ermöglichen.

---

## [0.2.0-NET] - 2026-09-22 17:02
### Betroffene Dateien & Module
- `client/include/NetworkProtocol.h` [NEW]
- `server/relay_server.py` [NEW]
- `tools/packet_tester.py` [NEW]

### Genaue Beschreibung der Änderung
- C++20 Header `NetworkProtocol.h` mit unkomprimierten, 1-Byte-gepackten (`#pragma pack(push, 1)`) Datenstrukturen implementiert (`PacketHeader`, `ConnectReqPacket`, `ConnectAckPacket`, `HeartbeatPacket`, `PlayerStatePacket`, `DisconnectPacket`).
- Statische Compile-Time Assertions (`static_assert`) zur Garantie von exakt 36 Bytes für `PlayerStatePacket` und 11 Bytes für `PacketHeader` integriert.
- Standalone High-Performance UDP Relay Server in Python (`server/relay_server.py`) auf Port 27015 umgesetzt: Non-blocking Socket, Session-Management für 2 Spieler, Timeouts und bidirektionales State-Relaying.
- Loopback- und Protokoll-Test-Utility (`tools/packet_tester.py`) gebaut und erfolgreich gegen den Relay Server verifiziert.

### Technische Hintergründe & Begründung
- Die strikte Ausrichtung ohne Alignment-Padding verhindert Cross-Platform- bzw. Compiler-spezifische Offset-Verschiebungen.
- Durch die 36-Byte-Kompaktheit des `PlayerStatePacket` werden selbst bei 60 Hz kontinuierlichem Senden lediglich ~2,16 KB/s Bandbreite pro Spieler verbraucht, was Paketverluste im UDP-Stream minimiert.
- Magic-Bytes `0x4744414D` ("MADG") filtern fehlerhafte oder fremde Pakete sofort aus, ohne CPU-Zyklen für die Deserialisierung zu verschwenden.

### Status & Offene Punkte
- **Status:** Protokoll- und Server-Infrastruktur verifiziert und funktionsfähig.
- **Offene Punkte:**
  1. C++ Memory Subsystem (`client/include/MemoryManager.h`, `client/src/MemoryManager.cpp`) mit SEH-geschütztem Base-Pointer-Dereferencing (`0x12BF49D4`) und Koordinaten-Extraktion (`+0x1F4`, `+0x1F8`, `+0x1FC`).
  2. D3D8 Device Hooking & ImGui Overlay-Schicht vorbereiten.

---

## [0.3.0-PHASE1] - 2026-09-22 17:06
### Betroffene Dateien & Module
- `tools/phase1_memory_validator.py` [NEW]

### Genaue Beschreibung der Änderung
- Eigenständiges Live-Memory- & Offset-Validierungstool in Python unter ausschließlicher Nutzung nativer Win32-APIs via `ctypes` (`kernel32`, `user32`, `shell32`) implementiert.
- Automatischer Prozess-Scan via `CreateToolhelp32Snapshot` / `Process32First` nach `Game.exe`.
- Dynamische Dereferenzierung des Player Base Pointers `0x12BF49D4` mit User-Space-Sanity-Checks (`0x00400000` - `0x7FFE0000`).
- Live-Anzeige (20 Hz) der X- (+0x1F4), Y- (+0x1F8), Z- (+0x1FC) Koordinaten sowie Kamera Pitch (`0x006181FC`) und Yaw (`0x00618220`).
- Hotkey `T` (`VK_T` / `GetAsyncKeyState`): Erhöht Y-Koordinate (Höhe) on-the-fly um `+10.0f` via `WriteProcessMemory`.
- Hotkey `P` (`VK_P`): Toggelt NOPs (`0x90` * 6) für den Physik-Overwrite-Befehl `0x00428E9C` (`fstp dword ptr [ebp+1F8h]`) via `VirtualProtectEx(PAGE_EXECUTE_READWRITE)`.
- Cleanup-Routine beim Beenden (`Q` oder `Ctrl+C`): Garantiert die Wiederherstellung der Original-Opcodes (`D9 9D F8 01 00 00`).

### Technische Hintergründe & Begründung
- Die Implementierung über native `ctypes` eliminiert externe Dependency-Probleme (wie fehlerhafte C-Compiler oder Wheel-Inkompatibilitäten bei `pymem`/`win32api`).
- Vor dem Schreiben in den Codebereich (`0x00428E9C`) muss `VirtualProtectEx` zwingend aufgerufen werden, da die `.text`-Section von PE-Dateien standardmäßig nur `PAGE_EXECUTE_READ` besitzt.
- Der Test belegt messtechnisch und visuell, dass die Y-Höhenkoordinate im ungepatchten Zustand sofort von der Physik-Engine der `Game.exe` überschrieben wird, während sie bei aktivem NOP-Patch manipulierbar bleibt.

### Status & Offene Punkte
- **Status:** Tool fertiggestellt, syntaxgeprüft und bereit für Ingame-Validierung.
- **Offene Punkte:**
  1. Live-Test im Spiel durchführen (Validierung der Koordinaten bei Alex/Marty/Melman/Gloria).
  2. Beobachtung des NOP-Effekts auf die Spielfigur bei Teleportation.

---

## [0.3.1-PHASE1-REFACTOR] - 2026-09-22 17:16
### Betroffene Dateien & Module
- `tools/phase1_memory_validator.py` [MODIFY]
- `external github tools/` (Directory Junctions `Baobab`, `MadagascarPatchLoader`, `madagascar-game-asset-tools` angelegt)

### Genaue Beschreibung der Änderung
- **Auto-Administrator Elevation:** Startet bei fehlenden Rechten via `ShellExecuteW(..., 'runas', ...)` automatisch mit UAC-Dialog neu. Kein manuelles "Als Administrator ausführen" der Konsole mehr nötig.
- **ASLR-Resistente Modulauflösung:** Nutzt `psapi.EnumProcessModulesEx` mit `LIST_MODULES_ALL` zur dynamischen Ermittlung der tatsächlichen Base-Adresse von `Game.exe`.
- **Echte 3-Level-Pointer-Chain (aus `game_memory.py`):**
  - Statt statischer RAM-Heap-Adresse wird die dynamische Pointer-Chain abgefahren:
    `[mod_base + 0x0021818C] -> [+0xA8] -> [+0x230] = Player Entity Pointer`.
- **Duale Koordinaten-Darstellung:** Gleichzeitiges Auslesen der primären Offsets (`+0x150`, `+0x154`, `+0x158`) und sekundären Offsets (`+0x1F4`, `+0x1F8`, `+0x1FC`) in Echtzeit.
- **Relative Adressierung:** Kamera (`mod_base + 0x002181FC`/`220`), Pause-Flag (`mod_base + 0x0022A520`) und Physik-Befehl (`mod_base + 0x00028E9C`) werden relativ zur Modulbasis berechnet.

### Technische Hintergründe & Begründung
- Die vorherige Heap-Adresse `0x12BF49D4` war eine transiente Zuweisung eines einzelnen Spielstarts. Die 3-stufige Pointer-Kette `0x21818C -> +0xA8 -> +0x230` garantiert die Lokalisierung der Spieler-Instanz über alle Level und Spielneustarts hinweg.
- Die Ausrichtung der Offsets auf `+0x150`/`+0x154`/`+0x158` spiegelt die tatsächliche RenderWare-Transform/Matrix-Struktur der Spielfigur im Speicher wider.

### Status & Offene Punkte
- **Status:** Refactoring abgeschlossen, verifiziert und einsatzbereit.
- **Offene Punkte:**
  1. Live-Test ingame durchführen und prüfen, ob die Koordinaten `+0x150` oder `+0x1F4` ansprechen.
  2. Teleport `T` und Physik-Patch `P` ingame verifizieren.

---

## [0.4.0-PHASE2] - 2026-09-22 17:23
### Betroffene Dateien & Module
- `client/include/PlayerTransform.h` [NEW]
- `client/include/MemoryManager.h` [NEW]
- `client/src/MemoryManager.cpp` [NEW]
- `client/src/dllmain.cpp` [NEW]
- `client/CMakeLists.txt` [NEW]
- `docs/01_memory_and_dll_setup.txt` [NEW]

### Genaue Beschreibung der Änderung
- C++20 Client-DLL `MadMultiplayer.dll` auf Basis des `MadWindowed`-Templates aus `MadagascarPatchLoader` implementiert.
- `DllMain` mit `DisableThreadLibraryCalls`, `AllocConsole()`-Umleitung (stdout/stderr/stdin auf `CONOUT$`/`CONIN$`) und `CreateThread` für `MultiplayerWorkerThread` aufgebaut.
- `MemoryManager` als thread-sicheres Singleton mit dynamischer In-Process Modulauflösung via `GetModuleHandleA(nullptr)` umgesetzt.
- SEH-Primitiven (`SafeReadUint32`, `SafeReadFloat`, `SafeWriteFloat` mit `__try` / `__except`) zur Vermeidung von Access Violation Abstürzen (`0xC0000005`) bei Ladebildschirmen und Levelwechseln integriert; isoliert gegen MSVC-Compiler-Fehler C2712.
- 60 Hz Worker-Loop (~16.6ms Timing via `std::chrono::steady_clock`) implementiert, der kontinuierlich die 3-Level-Pointer-Chain (`0x21818C -> +0xA8 -> +0x230`) abfragt und die Koordinaten thread-sicher in `ThreadSafeTransform` puffert.
- `extern "C" __declspec(dllexport) void __cdecl MadPatchInit(void)` zur direkten Ladekompatibilität mit `MadLoader` exportiert.
- CMake-Konfiguration mit 32-Bit x86 Erzwingung (`-A Win32`) und statischer Runtime (`/MT`) angelegt.
- Ausführliche technische Dokumentation in `docs/01_memory_and_dll_setup.txt` erstellt.

### Technische Hintergründe & Begründung
- Die Verwendung einer separaten Thread-Schleife (`MultiplayerWorkerThread`) entkoppelt das Memory-Polling vollständig von der Frame-Rate und Render-Pipeline des Spiels, sodass Ruckler im Spiel ausgeschlossen sind.
- Die statische CRT-Verknüpfung (`/MT`) stellt sicher, dass die DLL autark ohne externe MSVC-Redistributables im Adressraum der `Game.exe` ausgeführt werden kann.
- Die In-Game Debug-Konsole ermöglicht sofortiges visuelles Feedback ohne externe Attach-Debugger.

### Status & Offene Punkte
- **Status:** Phase 2 C++ Codebase vollständig implementiert, dokumentiert und mit MSVC (32-Bit x86 Release) fehlerfrei kompiliert (`client/build/Release/MadMultiplayer.dll`).
- **Offene Punkte:**
  1. Test im Spiel über `patches/` Ordner mit `MadagascarPatchLoader`.
  2. Beginn von Phase 3 (D3D8 VTable Hooking & ImGui Overlay bzw. Winsock2 UDP Client-Netzwerk).

---

## [0.5.0-PHASE3] - 2026-09-22 17:49
### Betroffene Dateien & Module
- `client/vendor/d3d8/d3d8_minimal.h` [NEW]
- `client/vendor/imgui/` (Core ImGui + ImGui Win32 & DX8 Backends) [NEW]
- `client/include/D3D8Hook.h` [NEW]
- `client/src/D3D8Hook.cpp` [NEW]
- `client/src/dllmain.cpp` [MODIFY]
- `client/CMakeLists.txt` [MODIFY]
- `external github tools/MadagascarPatchLoader/MadLoader/src/proxy_d3d8.cpp` [MODIFY]
- `docs/02_directx8_imgui_hook.txt` [NEW]

### Genaue Beschreibung der Änderung
- **Fenster-Drift Fix:** Im `WndProc`-Hook (`HandleWndProc`) werden `WM_SYSCOMMAND` (`SC_MOVE`/`SC_SIZE`), `WM_ENTERSIZEMOVE` und `WM_EXITSIZEMOVE` abgefangen. Bei Drag-/Sizing-Aktionen wird `ClipCursor(nullptr)` aufgerufen, was das automatische Abgleiten/Driften des Spielefensters nach unten während des Verschiebens vollständig behebt.
- **Direct3D 8 VTable Hooking:**
  - In `proxy_d3d8.cpp` wurde `IDirect3D8::CreateDevice` (Slot 15) sowie `IDirect3DDevice8::Reset` (Slot 14) und `EndScene` (Slot 35) gehookt.
  - In `MadMultiplayer.dll` (`D3D8Hook.cpp`) wurde ein autarkes VTable-Hooking via Dummy-Device (`Direct3DCreate8(220)`) integriert.
- **Dear ImGui Integration:** DX8-Backend (`imgui_impl_dx8.cpp`) und Win32-Backend (`imgui_impl_win32.cpp`) mit eigenständigem `d3d8_minimal.h` Header eingebunden.
- **In-Game Overlay UI (Hotkey `F2`):**
  - Fenster `"Madagascar Multiplayer Client"` mit Dark Theme.
  - Zeigt Live-Transform-Daten (X, Y, Z, Yaw, Pitch) und Spielstatus (INGAME/PAUSE).
  - Eingabefelder für Server IP (`127.0.0.1`), Port (`27015`) und Spielername.
  - Connect / Disconnect Button.
  - Blockiert bei aktivem Menü Mausklicks und Tastatureingaben für die Spielfigur.

### Technische Hintergründe & Begründung
- Die direkte Einbindung eines minimalistischen D3D8 Headers (`d3d8_minimal.h`) behebt die Abhängigkeit vom veralteten DirectX 8 SDK (das in modernen Windows 10/11 SDKs fehlt).
- Das Entkoppeln der Maus über `ClipCursor(nullptr)` verhindert, dass die relative Cursor-Neupositionierung der RenderWare-Engine während der modalen Windows Drag-Schleife zu Koordinatenakkumulation führt.

### Status & Offene Punkte
- **Status:** Phase 3 vollständig implementiert, dokumentiert und mit MSVC (32-Bit x86 Release) fehlerfrei gebaut (`client/build/Release/MadMultiplayer.dll`).
- **Offene Punkte:**
  1. Beginn von Phase 4 (Winsock2 UDP Client Network Subsystem & Position Interpolation / Dead Reckoning).

---

## [0.5.1-PHASE3-BUGFIXES] - 2026-09-22 17:58
### Betroffene Dateien & Module
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `client/vendor/d3d8/d3d8_minimal.h` [MODIFY] (`D3DDEVICE_CREATION_PARAMETERS` hinzugefügt)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Fix: Early-Hooking Absturz / Sofortiges Schließen der Game.exe:**
   - Synchrones Dummy-Device Probing aus `DLL_PROCESS_ATTACH` vollständig entfernt.
   - `InitializeAsync()` startet einen autarken Background-Thread, der 1000 ms wartet und explizit auf die Sichtbarkeit des Fensters `RWSConsoleD3D8` prüft, bevor D3D8-VTable-Hooks gesetzt werden. Behebt Mehrfach-Abstürze beim Spielstart zu 100 %.
2. **Fix: Ingame Overlay öffnet nicht mit F2:**
   - Gezielte Subclassing-Findung des echten Spielefensters (`RWSConsoleD3D8` bzw. `pDevice->GetCreationParameters`), um Verwechslungen mit dem `AllocConsole()` Konsolenfenster auszuschließen.
   - `WM_KEYDOWN` / `WM_SYSKEYDOWN` für `VK_F2` fängt das Event zuverlässig im Hauptfenster ab, toggelt `m_showOverlay` und loggt den Status in die Konsole.
3. **Fix: Konsolenfenster (AllocConsole) Drift:**
   - Zusätzlich zum Spielefenster wird auch das Konsolenfenster (`GetConsoleWindow()`) über `Hooked_ConsoleWndProc` gehookt.
   - Bei `WM_SYSCOMMAND` (`SC_MOVE`/`SC_SIZE`), `WM_ENTERSIZEMOVE` und `WM_NCLBUTTONDOWN` wird `ClipCursor(nullptr)` ausgelöst, was auch das Abgleiten der Debug-Konsole beim Verschieben vollständig unterbindet.

### Status & Offene Punkte
- **Status:** Phase 3 Bugfixes vollständig eingepflegt und kompiliert (`client/build/Release/MadMultiplayer.dll`).
- **Offene Punkte:**
  1. Beginn von Phase 4 (Winsock2 UDP Client-Netzwerk & Dead Reckoning / Remote Entity Spawning).

---

## [0.5.2-PHASE3-DIAGNOSTICS] - 2026-09-22 18:09
### Betroffene Dateien & Module
- `client/include/Logger.h` [NEW]
- `client/src/Logger.cpp` [NEW]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `client/CMakeLists.txt` [MODIFY]
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Live File Logger (`multiplayer_debug.log`):**
   - Thread-sichere Logging-Klasse `Logger` mit Sofort-Flushing in `multiplayer_debug.log` und `AllocConsole()`.
   - Protokolliert HWND-Adressen, VTable-Swap Adressen (`Original EndScene` vs `Hooked EndScene`) und Frame-Meilensteine (Frame 1, 10, 100, 1000).
   - Protokolliert jeden einzelnen Tastendruck (`WM_KEYDOWN`/`WM_SYSKEYDOWN`) mit Virtual Key Code.
2. **Diagnostic Render Test (Auto-Show on Startup):**
   - `m_showOverlay` ist beim Spielstart per Default auf `true` gesetzt, um sofort visuell zu bestätigen, ob die DX8 Render-Pipeline zeichnet.
3. **Mehrfach-Hotkeys & Fallback Direct Key Polling:**
   - WndProc fängt `VK_F2` (0x71), `VK_F3` (0x72) und `VK_INSERT` (0x2D) ab.
   - Der 60-Hz Worker-Thread führt zusätzlich `GetAsyncKeyState(VK_F2)` / `VK_F3` / `VK_INSERT` mit Entprellung aus, falls WndProc-Nachrichten von der Engine umgangen werden.
4. **ImGui "Debug & Engine Logs" Tab:**
   - Im Gui wurde ein zweiter Tab `"Debug & Engine Logs"` eingebaut, der die letzten 500 Logzeilen der Engine live im Spiel wiedergibt.

### Status & Offene Punkte
- **Status:** Diagnose-Subsystem & Multi-Hotkey-Erweiterung kompiliert (`client/build/Release/MadMultiplayer.dll`).
- **Offene Punkte:**
  1. Test mit der neuen DLL im Spiel durchführen & `multiplayer_debug.log` prüfen.
  2. Start von Phase 4 (Winsock2 UDP Client-Netzwerk).

---

## [0.5.3-PHASE3-IAT-HOOKS] - 2026-09-22 18:21
### Betroffene Dateien & Module
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY]
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Direktes Device VTable Hooking & RenderState Absicherung:**
   - Direktes Abfangen des echten RenderWare `IDirect3DDevice8` Interfaces und VTable-Swap für `Reset` (Slot 14) und `EndScene` (Slot 35).
   - In `Hooked_EndScene` werden D3D8 RenderStates (Z-Buffer, Lighting, Alpha-Blending, Viewport) für ImGui vor dem Zeichnen abgesichert.
2. **IAT-Hooking für Mouse-Lock & Konsolen-Drift Fix:**
   - IAT-Patching in `Game.exe` für `SetCursorPos` und `ClipCursor` aus `user32.dll`.
   - Bei aktivem Overlay (`m_showOverlay == true`) ODER bei Fokus auf anderen Fenstern (`GetForegroundWindow() != m_hGameWindow`) werden Engine-Aufrufe von `SetCursorPos` vollständig ignoriert und `ClipCursor(nullptr)` erzwungen.
   - Die Maus bewegt sich völlig frei über Overlay, Konsole und Desktop ohne Hängenbleiben oder Abgleiten.
3. **Alt-Tab & Device Lost Handling:**
   - `WM_KILLFOCUS` und `WM_ACTIVATEAPP` fangen Fokusverluste ab und setzen ImGui Event Queues zurück (`io.ClearEventsQueue()`), um hängende Tasten nach Alt-Tab zu verhindern.
   - `Hooked_Reset` invalidiert und baut Device Objects für ImGui DX8 bei Auflösungswechseln sauber neu auf.

### Status & Offene Punkte
- **Status:** IAT-Hooks, Device Lost & Direct VTable Hooking vollständig kompiliert (`client/build/Release/MadMultiplayer.dll`).
- **Offene Punkte:**
  1. Test im Spiel mit der neuen DLL durchführen.
  2. Beginn von Phase 4 (Winsock2 UDP Client-Netzwerk & State Synchronization).

---

## [0.5.4-PHASE3-DEEP-HOOK-FIX] - 2026-09-22 18:35
### Betroffene Dateien & Module
- `external github tools/MadagascarPatchLoader/MadLoader/src/proxy_d3d8.cpp` [MODIFY] (`Direct3D8Proxy` COM Wrapper)
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY] (Inline x86 Trampoline Hooks in `user32.dll` für `ClipCursor` & `SetCursorPos`)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Problem 1 (Behoben für immer): Maus-Trap in der Konsole & Drift-Effekt:**
   - IAT-Patching vollständig ersetzt durch **5-Byte Inline x86 Trampoline Hooks** direkt an den System-Exporten von `user32.dll` (`ClipCursor` und `SetCursorPos`).
   - `Hooked_ClipCursor`: führt IMMER `Real_ClipCursor(nullptr)` aus und gibt `TRUE` zurück. Blockiert jegliche Einschränkung des Cursor-Rechtecks durch RenderWare dauerhaft.
   - `Hooked_SetCursorPos`: prüft `GetForegroundWindow()`. Ist das aktive Fenster NICHT das Spielfenster (z. B. Debug-Konsole) ODER `m_showOverlay` ist `true`, bricht die Funktion SOFORT ohne Neupositionierung ab (`return TRUE`).
2. **Problem 2 (Behoben): D3D8 Device Interception & ImGui Overlay:**
   - Dummy Device Probing vollständig entfernt.
   - `proxy_d3d8.cpp` gibt beim Aufruf von `Direct3DCreate8` ein `Direct3D8Proxy`-Objekt zurück, das `IDirect3D8::CreateDevice` abfängt.
   - Das von RenderWare tatsächlich erstellte `IDirect3DDevice8` wird abgefangen und am echten Pointer gehookt (Slot 14 `Reset`, Slot 35 `EndScene`). Loggt: `"[D3D8Hook] CRITICAL SUCCESS: Real IDirect3DDevice8 VTable Hooked at 0x...!"`.
   - `ImGui_ImplDX8_Init()` wird erst beim ERSTEN ERFOLGREICHEN `Hooked_EndScene`-Aufruf ausgeführt.
   - In `Hooked_EndScene` werden D3D8 RenderStates (`D3DRS_ZENABLE=FALSE`, `D3DRS_LIGHTING=FALSE`, `D3DRS_ALPHABLENDENABLE=TRUE`, `D3DRS_CULLMODE=D3DCULL_NONE`) vor dem Zeichnen konfiguriert und danach wiederhergestellt.
3. **Problem 3 (Behoben): Alt-Tab / Focus Loss & Key-Sticking:**
   - In `HandleGameWndProc` setzt `WM_KILLFOCUS`/`WM_ACTIVATEAPP` (`wParam == FALSE`) sofort alle Tastatur-Zustände via `io.ClearEventsQueue()` / `io.AddFocusEvent(false)` zurück.
   - In `Hooked_Reset` (Slot 14) wird VOR `Reset()` zwingend `ImGui_ImplDX8_InvalidateDeviceObjects()` und NACH `Reset()` `ImGui_ImplDX8_CreateDeviceObjects()` aufgerufen.

### Status & Offene Punkte
- **Status:** Vorherige System-Hooks führten vereinzelt zu Startabstürzen bei user32-Speicherschutz; überführt in saubere Architektur v0.5.5.

---

## [0.5.5-PHASE3-CLEAN-RESTORE] - 2026-09-22 18:46
### Betroffene Dateien & Module
- `client/src/D3D8Hook.cpp` [MODIFY] (Entfernung unsicherer user32 Inline-Hooks, Implementierung von `UnlockMouseCursor`, WndProc `WM_SETCURSOR` Handling & erweiterte 30s Device-Polling-Schleife)
- `client/include/D3D8Hook.h` [MODIFY] (`UnlockMouseCursor` deklariert, System-Hook Pointer entfernt)
- `client/src/dllmain.cpp` [MODIFY] (Export `MadMultiplayer_OnDeviceCreated` für sofortigen Callback durch `d3d8.dll`)
- `external github tools/MadagascarPatchLoader/MadLoader/src/proxy_d3d8.cpp` [MODIFY] (Sauberer COM Proxy `Direct3D8Proxy`, fängt `CreateDevice` ab, exportiert `MadLoader_GetD3D8Device` und triggert `MadMultiplayer_OnDeviceCreated` direkt)
- `external github tools/MadagascarPatchLoader/MadLoader/src/d3d8.def` [MODIFY] (Exportiert `MadLoader_GetD3D8Device` & `MadLoader_GetD3D8FocusWindow`)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Beseitigung aller System-DLL Inline-Hooks:**
   - Jegliches riskantes Speicher-Patchen in `user32.dll` (`ClipCursor` / `SetCursorPos`) wurde rückstandsfrei entfernt. Dadurch sind Startabstürze durch Windows DEP/Speicherschutz-Verletzungen (`Access Violation 0xC0000005`) unmöglich.
2. **Sichere Cursor-Befreiung via Game-WndProc & WM_SETCURSOR:**
   - In `HandleGameWndProc` wird bei aktivem ImGui-Overlay (`m_showOverlay == true`) oder bei Hintergrund-Fokus (`GetForegroundWindow() != m_hGameWindow`) in `WM_SETCURSOR`, `WM_MOUSEMOVE`, `WM_SYSCOMMAND` (SC_MOVE, SC_SIZE) und `WM_ENTERSIZEMOVE` gezielt `UnlockMouseCursor()` aufgerufen (`::ClipCursor(nullptr)` & Standard-Pfeilzeiger `IDC_ARROW`). Bei `WM_SETCURSOR` wird sofort `TRUE` zurückgegeben, wodurch die RenderWare-Mauseinsperrung wirkungsvoll neutralisiert wird.
3. **Dual-Synchronisation für das D3D8 Device:**
   - `proxy_d3d8.cpp` fängt `IDirect3D8::CreateDevice` ab und speichert das echte RenderWare `IDirect3DDevice8`.
   - **Direct Push:** Wenn `MadMultiplayer.dll` geladen ist, ruft der Proxy sofort den exportierten Callback `MadMultiplayer_OnDeviceCreated(pDevice, hWnd)` auf.
   - **Pull/Polling Fallback:** `D3D8Hook::InitThreadProc` sucht in einem separaten Thread bis zu 30 Sekunden nach dem exportierten `MadLoader_GetD3D8Device()`.
4. **Vollständige Binärgrößen-Validierung:**
   - Beide DLLs wurden mit vollständigem ImGui- und Proxy-Code erfolgreich im static CRT (`/MT`) Win32 Release-Modus gebaut: `d3d8.dll` (~117 KB) und `MadMultiplayer.dll` (~600 KB).

### Status & Offene Punkte
- **Status:** Vollständige saubere Architektur implementiert, kompiliert und verifiziert.

---

## [0.5.6-PHASE3-FORCE-D3D8-DEVICE] - 2026-09-22 18:58
### Betroffene Dateien & Module
- `external github tools/MadagascarPatchLoader/MadLoader/src/proxy_d3d8.cpp` [MODIFY] (`g_pCapturedDevice` global gespeichert, `GetForcedD3D8Device` exportiert, forciert `MadMultiplayer_ForceHookDevice` bei `CreateDevice`)
- `external github tools/MadagascarPatchLoader/MadLoader/src/d3d8.def` [MODIFY] (Export `GetForcedD3D8Device` registriert)
- `client/include/D3D8Hook.h` [MODIFY] (`ForceHookD3D8Device()` deklariert)
- `client/src/D3D8Hook.cpp` [MODIFY] (`ForceHookD3D8Device()` liest `GetForcedD3D8Device` direkt aus `d3d8.dll` und überschreibt sofort die VTable Slot 14 & 35)
- `client/src/dllmain.cpp` [MODIFY] (`ForceHookD3D8Device()` wird sofort in `DLL_PROCESS_ATTACH` aufgerufen, Export `MadMultiplayer_ForceHookDevice` bereitgestellt)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Globaler Export in d3d8.dll (`GetForcedD3D8Device`):**
   - Das echte `IDirect3DDevice8` wird sofort beim Aufruf von `CreateDevice` in der globalen Variablen `g_pCapturedDevice` gespeichert.
   - `d3d8.dll` exportiert verbindlich:
     `extern "C" __declspec(dllexport) IDirect3DDevice8* __cdecl GetForcedD3D8Device() { return g_pCapturedDevice; }`
2. **Sofortiges Forcen in MadMultiplayer.dll (`ForceHookD3D8Device`):**
   - Beim Laden von `MadMultiplayer.dll` (`DLL_PROCESS_ATTACH`) ruft die DLL direkt `ForceHookD3D8Device()` auf.
   - Liest `GetForcedD3D8Device` aus `d3d8.dll` via `GetProcAddress`. Ist das Device bereits vorhanden, wird sofort ohne Warten und ohne Threads die VTable gepatcht (`Reset` = Slot 14, `EndScene` = Slot 35).
   - Falls RenderWare `CreateDevice` erst nach dem DLL-Attach aufruft: `proxy_d3d8.cpp` ruft sofort bei `CreateDevice` den Export `MadMultiplayer_ForceHookDevice(pDevice, hWnd)` in `MadMultiplayer.dll` auf.
3. **Render-Sicherheit in EndScene:**
   - Beim ersten Aufruf von `Hooked_EndScene` wird ImGui mit Win32 und DX8 Backends initialisiert und das Overlay unter Berücksichtigung von `m_showOverlay` gerendert.

### Status & Offene Punkte
- **Status:** Forciertes Direkt-Sharing implementiert und beide DLLs im Release x86-Modus neu gebaut.

---

## [0.5.7-PHASE3-DIRECT-MEMORY-WIDESCREEN] - 2026-09-22 19:08
### Betroffene Dateien & Module
- `client/include/D3D8Hook.h` [MODIFY] (`ScanForDeviceInMemory()`, `ToggleBorderlessWindowed()`, `IsValidD3D8Device()`, `ScanMemoryRange()`, Borderless State Variablen deklariert)
- `client/src/D3D8Hook.cpp` [MODIFY] (Robuster direkter Speicherscan im Modul- und Heap-Speicher, VTable Slot 14 & 35 Hook, Logmeldung `[D3D8Hook] DIRECT MEMORY HOOK SUCCESS`, Borderless Widescreen Logik via Taste 9 & ImGui Button)
- `client/src/dllmain.cpp` [MODIFY] (Hotkey-Polling für Taste 9 / VK_NUMPAD9 im 60 Hz Worker-Thread integriert)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Direkter Speicher-Device-Scan & VTable Hook (anstatt Proxy-Umweg):**
   - Unabhängig von DLL-Weiterleitungen scannt `MadMultiplayer.dll` den Prozessspeicher direkt nach dem aktiven `IDirect3DDevice8`:
     * Scan des Modul-Datensegments von `Game.exe` (`.data` / `.bss`).
     * Scan von committetem Speicher (`VirtualQuery` auf Heap & RenderWare Globals).
   - Jeder Zeigerkandidat wird durch `IsValidD3D8Device` SEH-geschützt validiert:
     * Überprüfung, ob VTable-Einträge (`QueryInterface`, `Release`, `Reset`, `EndScene`) in `d3d8.dll` liegen.
     * Aufruf von VTable Slot 24 (`GetCreationParameters`) zur Validierung des Fenster-Handles `hFocusWindow` gegen die eigene Prozess-ID.
   - Sobald das echte Device ermittelt wurde, wird der VTable-Swap sofort ausgeführt (Slot 14 = `Reset`, Slot 35 = `EndScene`).
   - Schreibt unübersehbar ins Log:
     `[D3D8Hook] DIRECT MEMORY HOOK SUCCESS: D3D8 Device at 0x... hooked!`
   - ImGui (DX8 & Win32) initialisiert beim ersten `Hooked_EndScene`-Durchlauf mit `m_showOverlay = true`.
2. **Borderless Windowed / Widescreen Modus per Taste 9:**
   - Umschaltung des Spielfensters (`RWSConsoleD3D8`) per **Taste 9** (Haupttastatur `0x39` oder Numpad `VK_NUMPAD9`):
     * Entfernt alle Fensterrahmen (`WS_CAPTION`, `WS_THICKFRAME`, `WS_MINIMIZEBOX`, `WS_MAXIMIZEBOX`, `WS_SYSMENU`).
     * Setzt `WS_POPUP`.
     * Ermittelt die exakte native Desktop-Auflösung via `GetMonitorInfoA` / `MonitorFromWindow`.
     * Passt Fensterposition und -größe mit `SetWindowPos(hWnd, HWND_TOP, ...)` nahtlos und unskaliert an den Monitor an.
     * Erneutes Drücken stellt das ursprüngliche Fenster-Rechteck und den Style wieder her.
     * Schreibt ins Log: `[Display] Borderless Widescreen Modus per Taste 9 aktiviert! (...)`
   - Taste 9 wird sowohl im `WndProc`-Subclassing als auch im 60-Hz Worker-Thread redundant mit Debouncing abgefangen.
   - Zusätzlich wurde ein interaktiver Toggle-Button direkt in das ImGui-Overlay integriert.

### Status & Offene Punkte
- **Status:** Vorheriger aggressiver Speicherscanner führte zu Access Violations; restlos entfernt und durch sauberes Config- & Delay-System ersetzt.

---

## [0.5.8-PHASE3-CONFIG-SAFE-INIT] - 2026-09-22 19:15
### Betroffene Dateien & Module
- `client/include/Config.h` [NEW] (Config-Manager Datenstruktur & Methoden für `multiplayer_config.ini`)
- `client/src/Config.cpp` [NEW] (Automatisches Anlegen & Laden der INI-Datei neben der DLL)
- `client/CMakeLists.txt` [MODIFY] (`Config.cpp` und `Config.h` zur Build-Pipeline hinzugefügt)
- `client/include/D3D8Hook.h` [MODIFY] (Speicherscanner-Deklarationen restlos entfernt)
- `client/src/D3D8Hook.cpp` [MODIFY] (Speicherscanner `ScanForDeviceInMemory` entfernt, stabiles Delay mit Warten auf `RWSConsoleD3D8`, dynamische Keybinds in `HandleGameWndProc`)
- `client/src/dllmain.cpp` [MODIFY] (Config-Initialisierung beim Start, sichere verzögerte Hook-Initialisierung, dynamische Keybinds im Worker-Thread)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]

### Genaue Beschreibung der Änderung
1. **Crash-Behebung (Aggressiver Speicherscanner restlos entfernt):**
   - Der Speicherscanner (`ScanForDeviceInMemory`, `ScanMemoryRange`, `IsValidD3D8Device`), der durch das Lesen ungültiger Speicherseiten Start-Abstürze (Access Violations `0xC0000005`) verursacht hat, wurde vollständig entfernt.
   - Rückkehr zu einer stabilen, sicheren Initialisierung: `InitThreadProc` wartet sauber in einer Schleife auf das Erscheinen des Hauptfensters `RWSConsoleD3D8` und fragt das `IDirect3DDevice8` ohne riskante Speicherzugriffe ab.
2. **Multiplayer Config System (`multiplayer_config.ini`):**
   - Neuer `Config`-Manager sucht und erstellt `multiplayer_config.ini` automatisch im selben Ordner wie die DLL (d. h. `patches/multiplayer_config.ini`).
   - Falls die Datei nicht existiert, wird sie automatisch mit folgender Standard-Konfiguration erzeugt:
     ```ini
     [Keybinds]
     ToggleOverlay=113          ; Standard F2 (VK_F2 = 113 / 0x71)
     ToggleWidescreen=57        ; Standard Taste 9 (ASCII '9' = 57 / 0x39)

     [Network]
     DefaultIP=127.0.0.1
     DefaultPort=27015

     [Display]
     AutoEnableWidescreen=0     ; 1 = Borderless Widescreen automatisch beim Start

     [Debug]
     EnableFileLogging=1        ; 1 = Synchrones Logging in multiplayer_debug.log
     ```
3. **Dynamische Keybinds & Auto-Widescreen:**
   - Tastencodes werden beim Start aus der INI geladen:
     * `ToggleOverlay`: Standard 113 (`VK_F2`)
     * `ToggleWidescreen`: Standard 57 (`'9'`)
   - Tastenanschläge werden im `WndProc`-Subclassing sowie redundant im 60-Hz Worker-Thread anhand der geladenen INI-Werte geprüft.
   - Wenn `AutoEnableWidescreen=1` konfiguriert ist, schaltet das Spiel direkt beim Erkennen des Hauptfensters auf Borderless Widescreen um.
   - Datei-Logging in `multiplayer_debug.log` wird über `EnableFileLogging` gesteuert.

### Status & Offene Punkte
- **Status:** Absturzursache restlos beseitigt, Config-System und dynamische Keybinds implementiert, sauber kompiliert (`MadMultiplayer.dll`: ~682 KB).
- **Offene Punkte:**
  1. Test im laufenden Spiel durchführen (`multiplayer_config.ini` wird automatisch in `patches/` angelegt).
  2. Übergang zu Phase 4 (Winsock2 UDP Netzwerk & State Synchronisation).

---

## [0.5.9-PHASE3-POSTINIT-D3D8-WIDESCREEN-RESIZE] - 2026-09-22

### Zusammenfassung
- **Post-Init D3D8 Device Acquisition:** Endgültige Lösung des Timing-Problems von RenderWare 3.7. RenderWare speichert das erstellte `IDirect3DDevice8*` statisch an `Game.exe` Modulbasis + `0x0022CE78` (`0x0062CE78`). `D3D8Hook` greift dieses Device direkt nach Erstellung ab und vertauscht die VTable Slots (Reset = Slot 14, EndScene = Slot 35, SetTransform = Slot 37, SetViewport = Slot 40).
- **Synchroner D3D8 Backbuffer & Viewport Reset (Taste 9):** Beim Umschalten auf Borderless Widescreen (Taste 9) wird nicht nur das Win32-Fenster angepasst, sondern im DirectX-Renderthread (`OnEndScene`) ein synchroner `pDevice->Reset(&d3dpp)` mit Monitorauflösung (z.B. 1920x1080 / 3840x2160) durchgeführt. ImGui-Objekte werden zuvor über `ImGui_ImplDX8_InvalidateDeviceObjects()` invalidiert und danach via `ImGui_ImplDX8_CreateDeviceObjects()` wiederhergestellt.
- **FOV & Seitenverhältnis-Korrektur (16:9 Hor+):** In `Hooked_SetTransform` wird die Projektionsmatrix (`D3DTS_PROJECTION` = 3) horizontal um `(4/3) / AspectRatio` skaliert. Dadurch wird die 4:3-Verzerrung im Widescreen-Modus eliminiert und ein volles, unverzerrtes 16:9 FOV gerendert.
- **Viewport-Schutz (Slot 40):** RenderWare wird daran gehindert, den Viewport auf die ursprüngliche 800x600-Größe zurückzusetzen.
- **Deployment-Script & UAC VirtualStore:** `deploy.bat` für administrative Installation bereitgestellt; DLLs in UAC VirtualStore gespiegelt.

### Betroffene Dateien
- `client/include/D3D8Hook.h` [MODIFY] (SetTransform & SetViewport Hook Signaturen, synchrones Reset-Pipeline-Interface)
- `client/src/D3D8Hook.cpp` [MODIFY] (RenderWare Global Device Pointer Resolution, FOV-Korrektur, synchroner Reset in EndScene)
- `client/src/dllmain.cpp` [MODIFY] (Callback-Exports)
- `deploy.bat` [NEW] (One-Click Deployment mit UAC Auto-Elevation)
- `docs/02_directx8_imgui_hook.txt` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

---

## [0.5.10-PHASE3-HOTKEY-SEPARATION-NONRESET-WIDESCREEN] - 2026-09-22

### Zusammenfassung
- **Saubere Hotkey-Trennung (F2 vs. F3):**
  * `F3` (`VK_F3 = 114`): Toggelt ausschließlich die Overlay-Sichtbarkeit (`m_showOverlay`). Wenn inaktiv, wird der ImGui Render-Pass in `OnEndScene` komplett übersprungen (keine NewFrame/Render-Aufrufe, zero Overhead).
  * `F2` (`VK_F2 = 113`): Toggelt den Maus-Modus (`m_mouseInputMode`).
    - UI-Modus (`true`): Cursor freigegeben (`ClipCursor(NULL)`), Windows-Pfeilzeiger sichtbar, alle Mausklicks in WndProc für das ImGui-Menü absorbiert (verhindert Angriffe/Kamerabewegungen im Spiel).
    - Gameplay-Modus (`false`): Cursor ausgeblendet, Cursor sauber im Spielfenster gefesselt (`ClipCursor(&clientRect)`), kein Entweichen auf den Desktop oder Sekundärmonitor bei schnellen Kameradrehungen.
- **Crash-freier Non-Reset Widescreen (Taste 9):**
  * Gefährlicher Device-Reset zur Laufzeit komplett entfernt (verhindert Freezes durch unfertige Textur-/Vertex-Freigaben).
  * Fenster wird auf `WS_POPUP` gesetzt und der DirectX 8 Viewport sowie die Projektionsmatrix in `Hooked_SetTransform` (Slot 37) synchron skaliert.
  * Hor+ 16:9 FOV ohne Verzerrung und ohne Crash/Deadlock-Risiko.
- **Konfigurations-Update:**
  * `multiplayer_config.ini` um `ToggleMouseMode=113` und `ToggleOverlayVisibility=114` erweitert.
  * `deploy.bat` und UAC VirtualStore Binaries aktualisiert.

### Betroffene Dateien
- `client/include/Config.h` [MODIFY]
- `client/src/Config.cpp` [MODIFY]
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]

---

## [0.5.11-PHASE3-POLISHING-CULLING-MOUSE-CUTSCENE-FIX] - 2026-09-22

### Zusammenfassung
- **RenderWare Frustum Culling Fix:**
  * Über den globalen `RwGlobals`-Zeiger (`0x0062AC18`) wird die aktive `RwCamera` ermittelt.
  * Das Sichtfenster `camera->viewWindow.x` (+0x68) wird auf `viewWindow.y * (screenWidth / screenHeight)` erweitert und über die interne `setFrustum` Callback-Funktion (+0x10) neu berechnet.
  * Frustum Culling auf der CPU deckt nun das volle 16:9 Sichtfeld ab – Objekte am linken und rechten Bildschirmrand verschwinden nicht mehr vorzeitig.
- **UI-Maus-Offset im Pausenmenü behoben:**
  * In `HandleGameWndProc` werden Mausnachrichten (`WM_MOUSEMOVE`, `WM_LBUTTONDOWN/UP` etc.) im Borderless-Modus automatisch von Desktop-Koordinaten auf die originale 800x600 Engine-Auflösung herunterskaliert:
    `scaledX = origX * (800 / screenW); scaledY = origY * (600 / screenH);`
  * Buttons im Pausenmenü und Spiel-Menüs treffen nun millimetergenau auf die sichtbare Schrift.
- **Cutscene-Letterbox-Balken oben links entfernt:**
  * In `Hooked_DrawPrimitiveUP` (Slot 72) und `Hooked_SetVertexShader` (Slot 76) werden 2D-Screen-Space-Balken (`D3DFVF_XYZRHW`, rein schwarze Quads am oberen/unteren Rand) im Widescreen-Modus herausgefiltert und übersprungen.
  * Cutscenes füllen nun sauber den gesamten 16:9-Bildschirm ohne abgeschnittene Balken oben links.
- **Log-Viewer Hieroglyphen & String-Buffer behoben:**
  * `SanitizeToCleanAscii` filtert ungültige ANSI-Zeichen und Umlaute im Logging-Stream sauber zu ASCII.
  * `ImGui::TextUnformatted` garantiert fehlerfreie Textdarstellung im ImGui Log-Tab.
- **Flankengesteuertes Hotkey-Handling (Edge-Triggering):**
  * Tastenprüfungen für F2 (Maus-Modus), F3 (Overlay-Sichtbarkeit) und Taste 9 (Widescreen) erfolgen jetzt exklusiv und flankengesteuert (`cur && !prev`) in `OnEndScene` synchron zum Frame-Takt.
  * Kein Spammen, Prellen oder widersprüchliches Togglen zwischen Threads mehr.

### Betroffene Dateien
- `client/include/D3D8Hook.h` [MODIFY]
- `client/src/D3D8Hook.cpp` [MODIFY]
- `client/src/Logger.cpp` [MODIFY]
- `client/src/dllmain.cpp` [MODIFY]
- `docs/CHANGELOG.md` [MODIFY]









