#include "../include/CheatManager.h"
#include "../include/MemoryManager.h"
#include "../include/D3D8Hook.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include "../vendor/imgui/imgui.h"
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstdlib>

namespace MadMultiplayer {

    // -------------------------------------------------------------------------
    // LEVEL PRESETS
    // -------------------------------------------------------------------------

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
        m_fFlightSpeed = 15.0f;
        m_flightEnabled = false;
        m_bFlightActive = false;
        m_dwManualCoinAddress = 0;
        m_manualCoinAddressHex[0] = '\0';
        m_bFreezeManualCoin = false;
        m_moveSpeedMultiplier = cfg.defaultMoveSpeedMultiplier;
        m_godModeEnabled = (cfg.enableGodModeDefault != 0);
        m_infiniteJumpEnabled = (cfg.enableInfiniteJumpDefault != 0);

        // Waypoint-Slots initialisieren
        for (size_t i = 0; i < m_waypoints.size(); ++i) {
            m_waypoints[i].isValid = false;
            snprintf(m_waypoints[i].label, sizeof(m_waypoints[i].label), "Slot %zu: (Leer)", i + 1);
            m_waypoints[i].timestamp[0] = '\0';
        }

        m_initialized = true;
        MAD_LOG("[CheatManager] Cheat Engine initialisiert (Flight Hotkey: F4, Speed: %.1f).", m_flightSpeed);
    }

    void CheatManager::Shutdown() {
        if (!m_initialized) return;
        SetFlightEnabled(false);
        m_initialized = false;
        MAD_LOG("[CheatManager] Cheat Subsystem beendet.");
    }

    // -------------------------------------------------------------------------
    // 1. DUAL-POINTER & MANUAL FAILSAFE COIN SYSTEM
    //
    // Pointer Chain 1: Direct Level/Shop Wallet ([Game.exe + 0x211620] + 0x53C)
    // Pointer Chain 2: Session/Stats (Game.exe + 0x229628 -> 0xC -> 0x1C -> 0x534)
    // Manual Override: Direct memory address input for instant failsafe writing
    // -------------------------------------------------------------------------
    static inline bool IsValidRam(uintptr_t addr) {
        return (addr >= 0x00010000 && addr <= 0x7FFE0000 && (addr % 4 == 0));
    }

    uintptr_t CheatManager::GetWalletCoinAddress()
    {
        __try {
            uintptr_t hGame = (uintptr_t)GetModuleHandleA(NULL);
            if (!hGame) return 0;

            // Pointer Chain 1: Direct Level/Shop Wallet ([Game.exe + 0x211620] + 0x53C)
            uintptr_t pBase = *(uintptr_t*)(hGame + 0x00211620);
            if (IsValidRam(pBase)) {
                uintptr_t walletAddr = pBase + 0x53C;
                if (IsValidRam(walletAddr)) return walletAddr;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    uintptr_t CheatManager::GetStatsCoinAddress()
    {
        __try {
            uintptr_t hGame = (uintptr_t)GetModuleHandleA(NULL);
            if (!hGame) return 0;

            // Pointer Chain 2: Session/Stats (Game.exe + 0x229628 -> 0xC -> 0x1C -> 0x534)
            uintptr_t p1 = *(uintptr_t*)(hGame + 0x00229628);
            if (!IsValidRam(p1)) return 0;
            uintptr_t p2 = *(uintptr_t*)(p1 + 0x0C);
            if (!IsValidRam(p2)) return 0;
            uintptr_t p3 = *(uintptr_t*)(p2 + 0x1C);
            if (!IsValidRam(p3)) return 0;
            uintptr_t statsAddr = p3 + 0x534;
            if (IsValidRam(statsAddr)) return statsAddr;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return 0;
    }

    uintptr_t CheatManager::GetCoinAddress()
    {
        if (m_dwManualCoinAddress != 0 && IsValidRam(m_dwManualCoinAddress)) return m_dwManualCoinAddress;
        uintptr_t wallet = GetWalletCoinAddress();
        if (wallet != 0) return wallet;
        return GetStatsCoinAddress();
    }

    void CheatManager::SetManualCoinAddress(uintptr_t addr)
    {
        m_dwManualCoinAddress = addr;
        snprintf(m_manualCoinAddressHex, sizeof(m_manualCoinAddressHex), "%08X", (unsigned int)addr);
    }

    bool CheatManager::SetCoins(int amount)
    {
        bool bSuccess = false;

        // 1. Write to manual override address if provided by user
        if (m_dwManualCoinAddress != 0) {
            __try {
                if (IsValidRam(m_dwManualCoinAddress)) {
                    *(int32_t*)m_dwManualCoinAddress = amount;
                    bSuccess = true;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // 2. Write to active wallet address
        uintptr_t wallet = GetWalletCoinAddress();
        if (wallet != 0) {
            __try {
                *(int32_t*)wallet = amount;
                bSuccess = true;
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // 3. Write to stats address
        uintptr_t stats = GetStatsCoinAddress();
        if (stats != 0) {
            __try {
                *(int32_t*)stats = amount;
                bSuccess = true;
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        return bSuccess;
    }

    // -------------------------------------------------------------------------
    // 2. COORDINATE-LOCK FLIGHT & FREEZING (100% DATENGESTUETZT, KEINE CRASHES!)
    // -------------------------------------------------------------------------
    void CheatManager::SetFlightEnabled(bool enabled) {
        m_flightEnabled = enabled;
        m_bFlightActive = enabled;
        auto& mem = MemoryManager::Get();
        if (enabled) {
            PlayerTransform cur{};
            if (mem.ReadLocalPlayer(cur) && cur.isValid) {
                m_vFlyPos.x = cur.x;
                m_vFlyPos.y = cur.y;
                m_vFlyPos.z = cur.z;
            }
            MAD_LOG("[CheatManager] Coordinate-Lock Flight AKTIVIERT (Pos locked @ %.2f, %.2f, %.2f).",
                    m_vFlyPos.x, m_vFlyPos.y, m_vFlyPos.z);
        } else {
            MAD_LOG("[CheatManager] Coordinate-Lock Flight DEAKTIVIERT.");
        }
    }

    void CheatManager::ProcessFlightMovement(float deltaTime, const PlayerTransform& cur) {
        if (!m_flightEnabled) return;

        // Wenn UI-Modus aktiv ist oder ImGui Tastatureingaben abfaengt -> Position halten & in der Luft freezen
        if (!g_bUIModeActive && !(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
            float moveDist = m_flightSpeed * deltaTime;

            // Horizontale Bewegung relativ zur Kamera-Blickrichtung (Yaw)
            float radYaw = cur.yaw;
            float fwdX = -sinf(radYaw);
            float fwdZ = cosf(radYaw);
            float rightX = cosf(radYaw);
            float rightZ = sinf(radYaw);

            // W / S: Vorwaerts / Rueckwaerts
            if (GetAsyncKeyState('W') & 0x8000) {
                m_vFlyPos.x += fwdX * moveDist;
                m_vFlyPos.z += fwdZ * moveDist;
            }
            if (GetAsyncKeyState('S') & 0x8000) {
                m_vFlyPos.x -= fwdX * moveDist;
                m_vFlyPos.z -= fwdZ * moveDist;
            }

            // A / D: Seitwaerts schweben (Strafe)
            if (GetAsyncKeyState('D') & 0x8000) {
                m_vFlyPos.x += rightX * moveDist;
                m_vFlyPos.z += rightZ * moveDist;
            }
            if (GetAsyncKeyState('A') & 0x8000) {
                m_vFlyPos.x -= rightX * moveDist;
                m_vFlyPos.z -= rightZ * moveDist;
            }

            // Space: Vertikal aufsteigen
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                m_vFlyPos.y += m_flightSpeed * deltaTime;
            }
            // Left Shift oder C: Vertikal absinken
            if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetAsyncKeyState('C') & 0x8000)) {
                m_vFlyPos.y -= m_flightSpeed * deltaTime;
            }
            // IN-AIR FREEZING: Wenn weder Space noch Shift/C gedrueckt sind, bleibt m_vFlyPos.y strikt unveraendert!
        }

        // Direct Memory Override: Position jeden Tick ueberschreiben (KEINE Velocity-Writes!)
        MemoryManager::Get().SetPlayerPosition(m_vFlyPos.x, m_vFlyPos.y, m_vFlyPos.z);
    }

    // -------------------------------------------------------------------------
    // 3. TELEPORTATION & WAYPOINTS (SAFE HEIGHT OFFSET + CRASH-FREE)
    // -------------------------------------------------------------------------
    void CheatManager::TeleportTo(float x, float y, float z) {
        auto& mem = MemoryManager::Get();
        PlayerTransform pt{};
        if (!mem.ReadLocalPlayer(pt) || !pt.isValid) {
            MAD_LOG("[CheatManager] Teleport abgebrochen: Spieler-Entity ungueltig!");
            return;
        }

        float safeY = y + 1.0f; // +1.0f Hoehen-Offset gegen Einsinken in Boden-Kollision
        m_vFlyPos.x = x;
        m_vFlyPos.y = safeY;
        m_vFlyPos.z = z;

        if (mem.SetPlayerPosition(x, safeY, z)) {
            m_targetPos[0] = x;
            m_targetPos[1] = safeY;
            m_targetPos[2] = z;
            MAD_LOG("[CheatManager] Teleport erfolgreich zu (X=%.2f, Y=%.2f, Z=%.2f)", x, safeY, z);
        }
    }

    void CheatManager::SaveWaypoint(size_t slotIdx) {
        if (slotIdx >= m_waypoints.size()) return;
        PlayerTransform pt{};
        if (MemoryManager::Get().ReadLocalPlayer(pt) && pt.isValid) {
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
        if (MemoryManager::Get().WriteMangoAmmo(m_customMangoAmmo)) {
            MAD_LOG("[CheatManager] Mangos/Munition aufgefuellt auf %d", m_customMangoAmmo);
        }
    }

    void CheatManager::ApplyMaxPawTokens() {
        MemoryManager::Get().WritePawTokens(m_customTokens);
        MAD_LOG("[CheatManager] Pfoten/Tiki-Tokens gesetzt auf %d", m_customTokens);
    }

    void CheatManager::ApplyFullHealth() {
        if (MemoryManager::Get().WriteHealth(m_customHealthValue)) {
            MAD_LOG("[CheatManager] Gesundheit auf %d gesetzt.", m_customHealthValue);
        }
    }

    void CheatManager::ProcessPlayerModifiers(float deltaTime, const PlayerTransform& cur) {
        auto& mem = MemoryManager::Get();

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

        // Keine Modifier-Tasten abfragen, wenn ImGui Keyboard fokussiert hat oder UI offen ist
        if (g_bUIModeActive || (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
            return;
        }

        // 2. Unendlicher Sprung / Super Jump (Flankengesteuert auf Space)
        bool curJumpKey = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        if (m_infiniteJumpEnabled && curJumpKey && !m_prevJumpKey && !m_flightEnabled) {
            float lift = 4.5f * m_superJumpMultiplier;
            mem.SetPlayerPosition(cur.x, cur.y + lift, cur.z);
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
                mem.SetPlayerPosition(cur.x + extraX, cur.y, cur.z + extraZ);
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

        m_flightSpeed = m_fFlightSpeed;

        // Freeze Coins at Target Value
        if (m_bFreezeCoins) {
            SetCoins(m_nTargetCoins);
        }

        // Freeze Manual Address if requested
        if (m_bFreezeManualCoin && m_dwManualCoinAddress != 0) {
            __try {
                if (IsValidRam(m_dwManualCoinAddress)) {
                    *(int32_t*)m_dwManualCoinAddress = 999;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // Hotkey: F4 fuer Coordinate-Lock Flight
        bool curF4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
        if (curF4 && !m_prevFlightHotkey) {
            if (!g_bUIModeActive && !(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
                ToggleFlight();
            }
        }
        m_prevFlightHotkey = curF4;

        // Alternativer Hotkey: N
        bool curN = (GetAsyncKeyState('N') & 0x8000) != 0;
        if (curN && !m_prevNKey) {
            if (!g_bUIModeActive && !(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
                ToggleFlight();
            }
        }
        m_prevNKey = curN;

        PlayerTransform pt{};
        if (MemoryManager::Get().ReadLocalPlayer(pt) && pt.isValid) {
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
    void CheatManager::RenderMenu()
    {
        ImGui::BeginChild("CheatScrollBox", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        // Section 1: Movement & Teleport
        if (ImGui::CollapsingHeader("Movement & Teleport", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (MemoryManager::Get().IsPlayerValid()) {
                Vector3 pos = MemoryManager::Get().GetPlayerPosition();
                ImGui::Text("Live Position: X = %.2f | Y = %.2f | Z = %.2f", pos.x, pos.y, pos.z);
                if (ImGui::Checkbox("Enable Coordinate-Lock Flight (F4)", &m_bFlightActive)) {
                    SetFlightEnabled(m_bFlightActive);
                }
                if (ImGui::SliderFloat("Flight Speed", &m_fFlightSpeed, 1.0f, 100.0f)) {
                    m_flightSpeed = m_fFlightSpeed;
                }

                // Teleport widgets
                ImGui::Separator();
                ImGui::InputFloat3("Target Coordinates (X, Y, Z)", m_targetPos, "%.2f");

                if (ImGui::Button("Teleport to Target [Teleport Now]", ImVec2(240, 26))) {
                    TeleportTo(m_targetPos[0], m_targetPos[1], m_targetPos[2]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Current Pos (+1.0m Safe Height)")) {
                    m_targetPos[0] = pos.x;
                    m_targetPos[1] = pos.y + 1.0f;
                    m_targetPos[2] = pos.z;
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
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "[Status: No Player Spawned - Main Menu / Lobby / Loading]");
                ImGui::BeginDisabled();
                ImGui::Button("Teleport to Target [Disabled]");
                ImGui::EndDisabled();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Section 2: Inventory & Coins
        if (ImGui::CollapsingHeader("Inventory & Coins", ImGuiTreeNodeFlags_DefaultOpen))
        {
            uintptr_t walletAddr = GetWalletCoinAddress();
            uintptr_t statsAddr = GetStatsCoinAddress();

            // Display live values of both resolved addresses:
            // Wallet (0x...): %d
            // Stats (0x...): %d
            if (walletAddr != 0) {
                int32_t walletCoins = 0;
                __try {
                    walletCoins = *(int32_t*)walletAddr;
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Wallet (0x%08X): %d", (unsigned int)walletAddr, walletCoins);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Wallet (0x%08X): [Nicht lesbar]", (unsigned int)walletAddr);
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Wallet: [Nicht verfuegbar im Hauptmenue / Lobby]");
            }

            if (statsAddr != 0) {
                int32_t statsCoins = 0;
                __try {
                    statsCoins = *(int32_t*)statsAddr;
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Stats (0x%08X): %d", (unsigned int)statsAddr, statsCoins);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Stats (0x%08X): [Nicht lesbar]", (unsigned int)statsAddr);
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Stats: [Warte auf aktive Spiel-Session]");
            }

            ImGui::Spacing();

            // Quick Buttons: [ Set 100 Coins ] & [ Set 999 Coins ] (calls SetCoins)
            if (ImGui::Button("[ Set 100 Coins ]", ImVec2(180, 26))) {
                SetCoins(100);
            }
            ImGui::SameLine();
            if (ImGui::Button("[ Set 999 Coins ]", ImVec2(180, 26))) {
                SetCoins(999);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Manual Override Box
            ImGui::Text("Manuelle Cheat Engine Adresse (Failsafe Direct Overwrite):");
            if (ImGui::InputText("Manuelle Cheat Engine Adresse (Hex)", m_manualCoinAddressHex, sizeof(m_manualCoinAddressHex), ImGuiInputTextFlags_CharsHexadecimal)) {
                m_dwManualCoinAddress = (uintptr_t)strtoul(m_manualCoinAddressHex, nullptr, 16);
            }

            if (m_dwManualCoinAddress != 0) {
                if (IsValidRam(m_dwManualCoinAddress)) {
                    int32_t manualVal = 0;
                    __try {
                        manualVal = *(int32_t*)m_dwManualCoinAddress;
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Manuelle Adresse: 0x%08X | Wert: %d", (unsigned int)m_dwManualCoinAddress, manualVal);
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Manuelle Adresse: 0x%08X [Nicht lesbar]", (unsigned int)m_dwManualCoinAddress);
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Ungueltige RAM-Adresse (0x00010000 - 0x7FFE0000, 4-Byte aligned)");
                }
            }

            if (ImGui::Button("[ 999 auf diese Adresse schreiben ]", ImVec2(280, 26))) {
                if (m_dwManualCoinAddress != 0 && IsValidRam(m_dwManualCoinAddress)) {
                    __try {
                        *(int32_t*)m_dwManualCoinAddress = 999;
                    } __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Diese Adresse einfrieren", &m_bFreezeManualCoin);

            ImGui::Spacing();
            ImGui::TextWrapped("Hinweis: Der HUD-Zaehler im laufenden Level aktualisiert sich erst beim Einsammeln "
                               "einer weiteren Muenze, beim Levelwechsel oder direkt im Souvenir-Shop. "
                               "Die Muenzen sind im Spiel und im Shop sofort voll verfuegbar!");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Section 3: God Mode & Player Modifiers
        if (ImGui::CollapsingHeader("God Mode & Modifiers"))
        {
            if (MemoryManager::Get().IsPlayerValid()) {
                ImGui::Checkbox("God Mode (Invulnerability / Lock HP)", &m_godModeEnabled);
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
                ImGui::SameLine();
                if (ImGui::Button("Max Tokens (100)", ImVec2(160, 24))) {
                    ApplyMaxPawTokens();
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "[Status: No Player Spawned - Main Menu / Lobby / Loading]");
            }
        }

        ImGui::EndChild();
    }

} // namespace MadMultiplayer
