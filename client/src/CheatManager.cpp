#include "../include/CheatManager.h"
#include "../include/MemoryManager.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include "../vendor/imgui/imgui.h"
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstdlib>

namespace MadMultiplayer {

    // -------------------------------------------------------------------------
    // REVERSE ENGINEERING: COIN UPDATE HOOK AT 0x0043BE37 (7 BYTES)
    //
    // 0043BE30 - C7 46 18 00000000 - mov [esi+18], 00000000  (7 bytes)
    // 0043BE37 - 8B 44 24 08       - mov eax, [esp+08]       (4 bytes: new coin param)
    // 0043BE3B - 89 46 1C          - mov [esi+1C], eax       (3 bytes: store coins)
    // 0043BE3E - 5E                - pop esi                 (1 byte)
    // 0043BE3F - C2 0800           - ret 0008                (3 bytes)
    //
    // Hook span: 0x0043BE37 .. 0x0043BE3E (7 bytes: 8B 44 24 08 89 46 1C)
    // Replaced with: 0xE9 <rel32> 0x90 0x90 (5-byte JMP + 2 NOPs)
    // Jump-back: 0x0043BE3E (pop esi; ret 8)
    // -------------------------------------------------------------------------
    static uintptr_t g_pInventoryBase = 0;
    static bool      g_bCoinCheatTriggered = false;
    static uintptr_t g_coinJumpBackAddr = 0;
    static bool      g_coinHookInstalled = false;
    static uint8_t   g_origCoinBytes[7] = { 0x8B, 0x44, 0x24, 0x08, 0x89, 0x46, 0x1C };

    __declspec(naked) static void Hook_CoinWrite() {
        __asm {
            // ESI enthaelt den Zeiger auf die Spieler-Inventarstruktur
            mov g_pInventoryBase, esi

            // Wurde der Set 999 Coins Cheat getriggert?
            cmp g_bCoinCheatTriggered, 1
            jne normal_coin_write

            // Cheat aktiv: Wert 999 in Argument und Register schreiben
            mov g_bCoinCheatTriggered, 0
            mov dword ptr [esp + 8], 999
            mov eax, 999
            jmp perform_write

        normal_coin_write:
            // Originale Instruktion: mov eax, [esp+08]
            mov eax, [esp + 8]

        perform_write:
            // Originale Instruktion: mov [esi+1C], eax
            mov [esi + 0x1C], eax

            // Sauber zurueckspringen zu 0x0043BE3E (wo pop esi und ret 0008 liegen)
            jmp dword ptr [g_coinJumpBackAddr]
        }
    }

    static const MapPreset g_mapPresets[] = {
        { "Central Park Zoo (Ursprung / Gehege)",   0.0f,   10.0f,    0.0f, 0.0f },
        { "Zoo - Panorama Aussicht (Sky View)",     0.0f,  120.0f,    0.0f, 0.0f },
        { "Zoo - Hauptausgang / Tor",              55.0f,   12.0f,  -85.0f, 1.57f },
        { "New York Strassen (Fluchtbeginn)",     120.0f,   15.0f,   45.0f, 0.0f },
        { "Grand Central Station (Bahnhofshalle)", -75.0f,   5.0f,  210.0f, 3.14f },
        { "Madagascar Strand (Ankunft)",          210.0f,   25.0f, -140.0f, 0.78f },
        { "Dschungel Kronendach (Baumkrone)",     150.0f,   85.0f,  -60.0f, 0.0f }
    };
    static const int g_numMapPresets = sizeof(g_mapPresets) / sizeof(g_mapPresets[0]);

    CheatManager& CheatManager::Instance() {
        static CheatManager instance;
        return instance;
    }

    CheatManager::~CheatManager() {
        Shutdown();
    }

    void CheatManager::Initialize() {
        if (m_initialized) return;

        // Config-Standards laden
        const auto& cfg = Config::Instance().Get();
        m_flightHotkey = VK_F4; // Hotkey F4
        m_flightSpeed = 15.0f;  // Standard: 15.0f (Bereich 1.0f - 100.0f)
        m_moveSpeedMultiplier = cfg.defaultMoveSpeedMultiplier;
        m_godModeEnabled = (cfg.enableGodModeDefault != 0);
        m_infiniteJumpEnabled = (cfg.enableInfiniteJumpDefault != 0);

        // Waypoint-Slots initialisieren
        for (size_t i = 0; i < m_waypoints.size(); ++i) {
            m_waypoints[i].isValid = false;
            snprintf(m_waypoints[i].label, sizeof(m_waypoints[i].label), "Slot %zu: (Leer)", i + 1);
            m_waypoints[i].timestamp[0] = '\0';
        }

        // 7-Byte Mid-Function Hook bei 0x0043BE37 installieren
        InstallCoinHook();

        m_initialized = true;
        MAD_LOG("[CheatManager] Cheat Engine initialisiert (Flight Hotkey: F4, Speed: %.1f).", m_flightSpeed);
    }

    void CheatManager::Shutdown() {
        if (!m_initialized) return;
        SetFlightEnabled(false);
        UninstallCoinHook();
        m_initialized = false;
        MAD_LOG("[CheatManager] Cheat Subsystem beendet.");
    }

    void CheatManager::InstallCoinHook() {
        if (g_coinHookInstalled) return;

        uintptr_t modBase = MemoryManager::Instance().GetModuleBase();
        if (!modBase) return;

        uintptr_t targetAddr = modBase + 0x0003BE37;
        g_coinJumpBackAddr = modBase + 0x0003BE3E;

        uint8_t curBytes[7] = {};
        if (MemoryManager::Instance().SafeReadBytes(targetAddr, curBytes, 7)) {
            // Pruefe ob die Bytes mit 8B 44 24 08 uebereinstimmen
            if (curBytes[0] == 0x8B && curBytes[1] == 0x44 && curBytes[2] == 0x24 && curBytes[3] == 0x08) {
                DWORD oldProtect = 0;
                if (VirtualProtect(reinterpret_cast<void*>(targetAddr), 7, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    uint8_t patch[7] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90 };
                    uintptr_t relOffset = reinterpret_cast<uintptr_t>(&Hook_CoinWrite) - (targetAddr + 5);
                    std::memcpy(&patch[1], &relOffset, 4);
                    std::memcpy(reinterpret_cast<void*>(targetAddr), patch, 7);
                    VirtualProtect(reinterpret_cast<void*>(targetAddr), 7, oldProtect, &oldProtect);
                    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(targetAddr), 7);
                    g_coinHookInstalled = true;
                    MAD_LOG("[CheatManager] 7-Byte Coin Hook erfolgreich installiert @ 0x%08X -> JumpBack: 0x%08X",
                            (unsigned int)targetAddr, (unsigned int)g_coinJumpBackAddr);
                }
            } else {
                MAD_LOG("[CheatManager] Coin Opcode @ 0x%08X stimmte nicht ueberein (%02X %02X %02X %02X)",
                        (unsigned int)targetAddr, curBytes[0], curBytes[1], curBytes[2], curBytes[3]);
            }
        }
    }

    void CheatManager::UninstallCoinHook() {
        if (!g_coinHookInstalled) return;

        uintptr_t modBase = MemoryManager::Instance().GetModuleBase();
        if (!modBase) return;

        uintptr_t targetAddr = modBase + 0x0003BE37;
        DWORD oldProtect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(targetAddr), 7, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::memcpy(reinterpret_cast<void*>(targetAddr), g_origCoinBytes, 7);
            VirtualProtect(reinterpret_cast<void*>(targetAddr), 7, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(targetAddr), 7);
            g_coinHookInstalled = false;
            MAD_LOG("[CheatManager] Coin Hook deinstalliert @ 0x%08X", (unsigned int)targetAddr);
        }
    }

    void CheatManager::TriggerSet999Coins() {
        g_bCoinCheatTriggered = true;
        uintptr_t invBase = GetEffectiveInventoryAddress();
        if (invBase && MemoryManager::IsValidUserPointer(invBase)) {
            WriteInventoryCoins(999);
        }
        MemoryManager::Instance().WriteCoins(999);
        MAD_LOG("[CheatManager] Set 999 Coins getriggert.");
    }

    uintptr_t CheatManager::GetEffectiveInventoryAddress() const {
        if (g_pInventoryBase != 0) {
            return g_pInventoryBase;
        }
        if (m_manualInventoryStr[0] != '\0') {
            uintptr_t parsed = static_cast<uintptr_t>(strtoul(m_manualInventoryStr, nullptr, 16));
            if (MemoryManager::IsValidUserPointer(parsed)) {
                return parsed;
            }
        }
        return m_manualInventoryAddr;
    }

    bool CheatManager::ReadInventoryCoins(uint32_t& outCoins) {
        uintptr_t base = GetEffectiveInventoryAddress();
        if (!base) return false;
        int32_t val = 0;
        if (MemoryManager::Instance().SafeReadBytes(base + 0x1C, &val, sizeof(val))) {
            outCoins = static_cast<uint32_t>(val);
            return true;
        }
        return false;
    }

    bool CheatManager::WriteInventoryCoins(uint32_t coins) {
        uintptr_t base = GetEffectiveInventoryAddress();
        if (!base) return false;
        return MemoryManager::Instance().SafeWriteBytes(base + 0x1C, &coins, sizeof(coins));
    }

    bool CheatManager::AddInventoryCoins(int32_t delta) {
        uint32_t current = 0;
        if (!ReadInventoryCoins(current)) current = 0;
        int32_t next = static_cast<int32_t>(current) + delta;
        if (next < 0) next = 0;
        return WriteInventoryCoins(static_cast<uint32_t>(next));
    }

    bool CheatManager::ReadSecondaryToken(uint32_t& outToken) {
        uintptr_t base = GetEffectiveInventoryAddress();
        if (!base) return false;
        int32_t val = 0;
        if (MemoryManager::Instance().SafeReadBytes(base + 0x18, &val, sizeof(val))) {
            outToken = static_cast<uint32_t>(val);
            return true;
        }
        return false;
    }

    bool CheatManager::WriteSecondaryToken(uint32_t token) {
        uintptr_t base = GetEffectiveInventoryAddress();
        if (!base) return false;
        return MemoryManager::Instance().SafeWriteBytes(base + 0x18, &token, sizeof(token));
    }

    // -------------------------------------------------------------------------
    // 2. COORDINATE-LOCK FLIGHT & FREEZING (100% DATENGESTUETZT, KEINE NOPS!)
    // -------------------------------------------------------------------------
    void CheatManager::SetFlightEnabled(bool enabled) {
        m_flightEnabled = enabled;
        auto& mem = MemoryManager::Instance();
        if (enabled) {
            PlayerTransform cur{};
            if (mem.ReadLocalPlayer(cur) && cur.isValid) {
                m_vFlyTarget.x = cur.x;
                m_vFlyTarget.y = cur.y;
                m_vFlyTarget.z = cur.z;
            }
            mem.ZeroVelocities();
            MAD_LOG("[CheatManager] Coordinate-Lock Flight AKTIVIERT (Pos locked @ %.2f, %.2f, %.2f).",
                    m_vFlyTarget.x, m_vFlyTarget.y, m_vFlyTarget.z);
        } else {
            mem.ZeroVelocities();
            MAD_LOG("[CheatManager] Coordinate-Lock Flight DEAKTIVIERT.");
        }
    }

    void CheatManager::ProcessFlightMovement(float deltaTime, const PlayerTransform& cur) {
        if (!m_flightEnabled) return;

        // Wenn der Spieler gerade in einem ImGui-Textfeld tippt -> Position halten & freezen
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) {
            MemoryManager::Instance().WriteLocalPosition(m_vFlyTarget.x, m_vFlyTarget.y, m_vFlyTarget.z);
            MemoryManager::Instance().ZeroVelocities();
            return;
        }

        float moveDist = m_flightSpeed * deltaTime;

        // Horizontale Bewegung relativ zur Kamera-Blickrichtung (Yaw)
        float radYaw = cur.yaw;
        float fwdX = -sinf(radYaw);
        float fwdZ = cosf(radYaw);
        float rightX = cosf(radYaw);
        float rightZ = sinf(radYaw);

        // W / S: Vorwaerts / Rueckwaerts
        if (GetAsyncKeyState('W') & 0x8000) {
            m_vFlyTarget.x += fwdX * moveDist;
            m_vFlyTarget.z += fwdZ * moveDist;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            m_vFlyTarget.x -= fwdX * moveDist;
            m_vFlyTarget.z -= fwdZ * moveDist;
        }

        // A / D: Seitwaerts schweben (Strafe)
        if (GetAsyncKeyState('D') & 0x8000) {
            m_vFlyTarget.x += rightX * moveDist;
            m_vFlyTarget.z += rightZ * moveDist;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            m_vFlyTarget.x -= rightX * moveDist;
            m_vFlyTarget.z -= rightZ * moveDist;
        }

        // Space: Vertikal aufsteigen
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            m_vFlyTarget.y += m_flightSpeed * deltaTime;
        }
        // Left Shift oder C: Vertikal absinken
        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetAsyncKeyState('C') & 0x8000)) {
            m_vFlyTarget.y -= m_flightSpeed * deltaTime;
        }
        // IN-AIR FREEZING: Wenn weder Space noch Shift/C gedrueckt sind, bleibt m_vFlyTarget.y strikt unveraendert!

        // Direct Memory Override: Position jeden Tick ueberschreiben und Geschwindigkeiten nullen
        MemoryManager::Instance().WriteLocalPosition(m_vFlyTarget.x, m_vFlyTarget.y, m_vFlyTarget.z);
        MemoryManager::Instance().ZeroVelocities();
    }

    // -------------------------------------------------------------------------
    // 3. TELEPORTATION & WAYPOINTS (SAFE HEIGHT OFFSET + NULL CHECKS)
    // -------------------------------------------------------------------------
    void CheatManager::TeleportTo(float x, float y, float z) {
        auto& mem = MemoryManager::Instance();
        PlayerTransform pt{};
        if (!mem.ReadLocalPlayer(pt) || !pt.isValid) {
            MAD_LOG("[CheatManager] Teleport abgebrochen: Spieler-Entity ungueltig!");
            return;
        }

        float safeY = y + 1.0f; // +1.0f Hoehen-Offset gegen Einsinken in Boden-Kollision
        m_vFlyTarget.x = x;
        m_vFlyTarget.y = safeY;
        m_vFlyTarget.z = z;

        if (mem.WriteLocalPosition(x, safeY, z)) {
            mem.ZeroVelocities();
            m_targetPos[0] = x;
            m_targetPos[1] = safeY;
            m_targetPos[2] = z;
            MAD_LOG("[CheatManager] Teleport erfolgreich zu (X=%.2f, Y=%.2f, Z=%.2f)", x, safeY, z);
        }
    }

    void CheatManager::SaveWaypoint(size_t slotIdx) {
        if (slotIdx >= m_waypoints.size()) return;
        PlayerTransform pt{};
        if (MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid) {
            m_waypoints[slotIdx].isValid = true;
            m_waypoints[slotIdx].pos = { pt.x, pt.y, pt.z };
            m_waypoints[slotIdx].yaw = pt.yaw;
            m_waypoints[slotIdx].pitch = pt.pitch;

            time_t now = time(nullptr);
            struct tm tm_buf;
            localtime_s(&tm_buf, &now);
            strftime(m_waypoints[slotIdx].timestamp, sizeof(m_waypoints[slotIdx].timestamp), "%H:%M:%S", &tm_buf);

            snprintf(m_waypoints[slotIdx].label, sizeof(m_waypoints[slotIdx].label),
                     "Slot %zu: (%.1f, %.1f, %.1f) [%s]",
                     slotIdx + 1, pt.x, pt.y, pt.z, m_waypoints[slotIdx].timestamp);

            MAD_LOG("[CheatManager] Checkpoint Slot %zu gespeichert: %s", slotIdx + 1, m_waypoints[slotIdx].label);
        }
    }

    void CheatManager::LoadWaypoint(size_t slotIdx) {
        if (slotIdx >= m_waypoints.size()) return;
        if (!m_waypoints[slotIdx].isValid) {
            MAD_LOG("[CheatManager] Checkpoint Slot %zu ist leer!", slotIdx + 1);
            return;
        }
        const auto& wp = m_waypoints[slotIdx];
        TeleportTo(wp.pos.x, wp.pos.y, wp.pos.z);
        MAD_LOG("[CheatManager] Checkpoint Slot %zu geladen: %s", slotIdx + 1, wp.label);
    }

    // -------------------------------------------------------------------------
    // 4. STATS & MODIFIERS
    // -------------------------------------------------------------------------
    void CheatManager::ApplyRefillMangoes() {
        if (MemoryManager::Instance().WriteMangoAmmo(m_customMangoAmmo)) {
            MAD_LOG("[CheatManager] Mangos/Munition aufgefuellt auf %d", m_customMangoAmmo);
        }
    }

    void CheatManager::ApplyMaxPawTokens() {
        WriteSecondaryToken(100);
        MemoryManager::Instance().WritePawTokens(m_customTokens);
        MAD_LOG("[CheatManager] Pfoten/Tiki-Tokens gesetzt auf %d", m_customTokens);
    }

    void CheatManager::ApplyFullHealth() {
        if (MemoryManager::Instance().WriteHealth(m_customHealthValue)) {
            MAD_LOG("[CheatManager] Gesundheit auf %d gesetzt.", m_customHealthValue);
        }
    }

    void CheatManager::ProcessPlayerModifiers(float deltaTime, const PlayerTransform& cur) {
        auto& mem = MemoryManager::Instance();

        // 1. God Mode kontinuierlich anwenden
        if (m_godModeEnabled) {
            int32_t hp = 0;
            if (mem.ReadHealth(hp)) {
                if (hp < m_customHealthValue) {
                    mem.WriteHealth(m_customHealthValue);
                }
            } else {
                mem.WriteHealth(m_customHealthValue);
            }
        }

        // Keine Modifier-Tasten abfragen, wenn ImGui Keyboard fokussiert hat
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }

        // 2. Unendlicher Sprung / Super Jump (Flankengesteuert auf Space)
        bool curJumpKey = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        if (m_infiniteJumpEnabled && curJumpKey && !m_prevJumpKey && !m_flightEnabled) {
            float lift = 4.5f * m_superJumpMultiplier;
            mem.WriteLocalPosition(cur.x, cur.y + lift, cur.z);
        }
        m_prevJumpKey = curJumpKey;

        // 3. Laufgeschwindigkeits-Multiplikator
        if (m_moveSpeedMultiplier > 1.01f && !m_flightEnabled) {
            bool moving = (GetAsyncKeyState('W') & 0x8000) ||
                          (GetAsyncKeyState('S') & 0x8000) ||
                          (GetAsyncKeyState('A') & 0x8000) ||
                          (GetAsyncKeyState('D') & 0x8000);
            if (moving) {
                float extraSpeed = (m_moveSpeedMultiplier - 1.0f) * 12.0f * deltaTime;
                float radYaw = cur.yaw;
                float extraX = -sinf(radYaw) * extraSpeed;
                float extraZ = cosf(radYaw) * extraSpeed;
                mem.WriteLocalPosition(cur.x + extraX, cur.y, cur.z + extraZ);
            }
        }
    }

    void CheatManager::Update(float deltaTime) {
        if (!m_initialized) {
            Initialize();
        }

        // Begrenze extreme DeltaTimes bei Ladebildschirmen
        if (deltaTime <= 0.0f || deltaTime > 0.2f) {
            deltaTime = 0.0166f;
        }

        // Hotkey: F4 fuer Coordinate-Lock Flight
        bool curF4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
        if (curF4 && !m_prevFlightHotkey) {
            if (!(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
                ToggleFlight();
            }
        }
        m_prevFlightHotkey = curF4;

        // Alternativer Hotkey: N
        bool curN = (GetAsyncKeyState('N') & 0x8000) != 0;
        if (curN && !m_prevNKey) {
            if (!(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
                ToggleFlight();
            }
        }
        m_prevNKey = curN;

        PlayerTransform pt{};
        if (MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid) {
            static bool initialSync = false;
            if (!initialSync) {
                m_targetPos[0] = pt.x;
                m_targetPos[1] = pt.y + 1.0f;
                m_targetPos[2] = pt.z;
                initialSync = true;
            }

            ProcessFlightMovement(deltaTime, pt);
            ProcessPlayerModifiers(deltaTime, pt);
        }
    }

    // -------------------------------------------------------------------------
    // 5. IMGUI UI LAYOUT ([ Cheats & Sandbox ])
    // -------------------------------------------------------------------------
    void CheatManager::RenderMenu() {
        PlayerTransform pt{};
        bool playerReady = MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid;

        // ---------------------------------------------------------------------
        // SECTION 1: MOVEMENT & FLIGHT
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("✈  Movement & Coordinate-Lock Flight", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool flight = m_flightEnabled;
            if (ImGui::Checkbox("Enable Coordinate-Lock Flight (HotKey: F4)", &flight)) {
                SetFlightEnabled(flight);
            }
            ImGui::SameLine();
            if (m_flightEnabled) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[ACTIVE - Position Locked & Gravity Free]");
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[Inactive - Press F4 or N]");
            }

            ImGui::SliderFloat("Flight Speed", &m_flightSpeed, 1.0f, 100.0f, "%.1f");
            ImGui::TextDisabled("Controls:\n  • W / S: Forward / Backward along Yaw\n  • A / D: Strafe Left / Right\n  • Space: Ascend (+Y)\n  • Left Shift / C: Descend (-Y)\n  • In-Air Freezing: When no vertical key is pressed, altitude is frozen!");
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // SECTION 2: TELEPORT & COORDINATES
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("📍  Teleport & Coordinates", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (playerReady) {
                ImGui::Text("Live Position: X = %.2f | Y = %.2f (Height) | Z = %.2f | Yaw = %.2f rad", pt.x, pt.y, pt.z, pt.yaw);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Player entity not active (Menu or Loading)");
            }

            ImGui::Separator();
            ImGui::InputFloat3("Target Coordinates (X, Y, Z)", m_targetPos, "%.2f");

            if (ImGui::Button("Teleport to Target [Teleport Now]", ImVec2(240, 26))) {
                TeleportTo(m_targetPos[0], m_targetPos[1], m_targetPos[2]);
            }
            ImGui::SameLine();
            if (ImGui::Button("Current Pos (+1.0m Safe Height)")) {
                if (playerReady) {
                    m_targetPos[0] = pt.x;
                    m_targetPos[1] = pt.y + 1.0f;
                    m_targetPos[2] = pt.z;
                }
            }

            ImGui::Spacing();
            ImGui::Text("Checkpoint Slots (Save / Load Waypoints):");
            for (size_t i = 0; i < m_waypoints.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::Text("%s", m_waypoints[i].label);
                ImGui::SameLine(320);
                char saveLabel[32];
                snprintf(saveLabel, sizeof(saveLabel), "Save Slot %zu", i + 1);
                if (ImGui::Button(saveLabel)) {
                    SaveWaypoint(i);
                }
                ImGui::SameLine();
                char loadLabel[32];
                snprintf(loadLabel, sizeof(loadLabel), "Load Slot %zu", i + 1);
                if (ImGui::Button(loadLabel)) {
                    LoadWaypoint(i);
                }
                ImGui::PopID();
            }

            ImGui::Spacing();
            ImGui::Text("Level & Map Presets:");
            const char* presetNames[g_numMapPresets];
            for (int i = 0; i < g_numMapPresets; ++i) presetNames[i] = g_mapPresets[i].name;

            ImGui::Combo("Preset Area", &m_selectedPreset, presetNames, g_numMapPresets);
            if (ImGui::Button("Teleport to Preset Area", ImVec2(220, 24))) {
                const auto& pr = g_mapPresets[m_selectedPreset];
                TeleportTo(pr.x, pr.y, pr.z);
            }
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // SECTION 3: INVENTORY & COINS CHEAT SYSTEM (HOOK @ 0x0043BE37)
        // -------------------------------------------------------------------------
        if (ImGui::CollapsingHeader("🪙  Inventory & Coins Cheat System", ImGuiTreeNodeFlags_DefaultOpen)) {
            uintptr_t effectiveAddr = GetEffectiveInventoryAddress();

            ImGui::Text("Hook Status: ");
            ImGui::SameLine();
            if (g_pInventoryBase != 0) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Captured via Hook @ 0x0043BE37 (Base: 0x%08X)", (unsigned int)g_pInventoryBase);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Waiting for capture (Collect 1 coin in-game or enter manual address)");
            }

            ImGui::Text("Effective Inventory Base: 0x%08X", (unsigned int)effectiveAddr);
            ImGui::InputText("Inventory / Coin Base (Hex Fallback)", m_manualInventoryStr, sizeof(m_manualInventoryStr));
            ImGui::SameLine();
            if (ImGui::Button("Set Manual")) {
                m_manualInventoryAddr = static_cast<uintptr_t>(strtoul(m_manualInventoryStr, nullptr, 16));
            }

            ImGui::Separator();

            uint32_t currentCoins = 0;
            bool coinsReadOk = ReadInventoryCoins(currentCoins);
            if (coinsReadOk) {
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Current Coins (+0x1C): %u", currentCoins);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Current Coins (+0x1C): [Waiting for Base Pointer]");
            }

            if (ImGui::Button("Set 999 Coins", ImVec2(160, 26))) {
                TriggerSet999Coins();
            }
            ImGui::SameLine();
            if (ImGui::Button("+100 Coins", ImVec2(140, 26))) {
                AddInventoryCoins(100);
            }

            ImGui::Spacing();
            uint32_t currentTokens = 0;
            bool tokensReadOk = ReadSecondaryToken(currentTokens);
            if (tokensReadOk) {
                ImGui::Text("Secondary Tokens (+0x18): %u", currentTokens);
            } else {
                ImGui::TextDisabled("Secondary Tokens (+0x18): [Cannot Read]");
            }

            if (ImGui::Button("Max Secondary Tokens (100)", ImVec2(220, 24))) {
                WriteSecondaryToken(100);
                MemoryManager::Instance().WritePawTokens(100);
            }

            ImGui::Spacing();
            ImGui::Text("Address / Offset Probe Field:");
            ImGui::InputInt("Probe Offset", &m_probeOffset, 1, 4);
            if (effectiveAddr) {
                int32_t probeRead = 0;
                if (MemoryManager::Instance().SafeReadBytes(effectiveAddr + m_probeOffset, &probeRead, sizeof(probeRead))) {
                    ImGui::Text("Value @ 0x%08X + 0x%X: %d (0x%08X)",
                                (unsigned int)effectiveAddr, m_probeOffset, probeRead, (unsigned int)probeRead);
                }
            }
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // SECTION 4: GOD MODE & PLAYER MODIFIERS
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("🛡  God Mode & Player Modifiers")) {
            ImGui::Checkbox("God Mode (Invulnerability / Lock HP)", &m_godModeEnabled);
            ImGui::SameLine();
            if (m_godModeEnabled) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[ACTIVE]");
            }

            ImGui::SliderInt("Target HP", &m_customHealthValue, 1, 200);
            if (ImGui::Button("Restore Full Health (100 HP)", ImVec2(240, 24))) {
                ApplyFullHealth();
            }

            ImGui::Separator();
            ImGui::Checkbox("Infinite Jump (Mid-Air Jump on Space)", &m_infiniteJumpEnabled);
            ImGui::SliderFloat("Super Jump Multiplier", &m_superJumpMultiplier, 1.0f, 5.0f, "%.1fx");
            ImGui::SliderFloat("Movement Speed Multiplier", &m_moveSpeedMultiplier, 1.0f, 5.0f, "%.1fx");

            ImGui::Spacing();
            if (ImGui::Button("Refill Mango Ammo (99)", ImVec2(200, 24))) {
                ApplyRefillMangoes();
            }
        }
    }

} // namespace MadMultiplayer
