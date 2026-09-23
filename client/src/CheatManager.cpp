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
    // 1. AUTHORITATIVE STATIC COIN POINTER CHAIN
    //
    // Exact 3-Level Pointer Chain:
    // Game.exe + 0x00229628 -> Level 1 (+0x0C) -> Level 2 (+0x1C) -> Target (+0x534)
    //
    // Authoritative wallet variable used by game logic and Souvenir Shop.
    // 100% data-driven, ZERO opcode hooks, no cutscene freezes or softlocks.
    // -------------------------------------------------------------------------
    uintptr_t CheatManager::GetCoinAddress() {
        uintptr_t hGameModule = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (!hGameModule) return 0;

        // Dereference Level 0 -> Level 1 (Game.exe + 0x00229628)
        uintptr_t pBase = hGameModule + 0x00229628;
        if (IsBadReadPtr(reinterpret_cast<void*>(pBase), sizeof(uintptr_t))) return 0;
        uintptr_t pLevel1 = *reinterpret_cast<uintptr_t*>(pBase);
        if (!pLevel1 || IsBadReadPtr(reinterpret_cast<void*>(pLevel1 + 0x0C), sizeof(uintptr_t))) return 0;

        // Dereference Level 1 -> Level 2 (+0x0C)
        uintptr_t pLevel2 = *reinterpret_cast<uintptr_t*>(pLevel1 + 0x0C);
        if (!pLevel2 || IsBadReadPtr(reinterpret_cast<void*>(pLevel2 + 0x1C), sizeof(uintptr_t))) return 0;

        // Dereference Level 2 -> Level 3 (+0x1C)
        uintptr_t pLevel3 = *reinterpret_cast<uintptr_t*>(pLevel2 + 0x1C);
        if (!pLevel3 || IsBadReadPtr(reinterpret_cast<void*>(pLevel3 + 0x534), sizeof(int32_t))) return 0;

        // Final Target Address (+0x534)
        uintptr_t finalCoinAddr = pLevel3 + 0x534;
        return finalCoinAddr;
    }

    bool CheatManager::SetCoins(int amount) {
        uintptr_t targetAddr = GetCoinAddress();
        if (!targetAddr || IsBadWritePtr(reinterpret_cast<void*>(targetAddr), sizeof(int32_t))) {
            MAD_LOG("[CheatManager] SetCoins failed: Invalid pointer target.");
            return false;
        }

        *reinterpret_cast<int32_t*>(targetAddr) = amount;

        // Synchronisiere auch den MemoryManager-Spiegel falls verfuegbar
        MemoryManager::Get().WriteCoins(amount);

        MAD_LOG("[CheatManager] Authoritative wallet balance set to %d at 0x%08X", amount, (unsigned int)targetAddr);
        return true;
    }

    // -------------------------------------------------------------------------
    // 2. COORDINATE-LOCK FLIGHT & FREEZING (100% DATENGESTUETZT, KEINE CRASHES!)
    // -------------------------------------------------------------------------
    void CheatManager::SetFlightEnabled(bool enabled) {
        m_flightEnabled = enabled;
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

        // Freeze Coins at Target Value
        if (m_bFreezeCoins) {
            uintptr_t coinAddr = GetCoinAddress();
            if (coinAddr && !IsBadWritePtr(reinterpret_cast<void*>(coinAddr), sizeof(int32_t))) {
                *reinterpret_cast<int32_t*>(coinAddr) = m_nTargetCoins;
            }
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
    void CheatManager::RenderMenu() {
        PlayerTransform pt{};
        bool playerReady = MemoryManager::Get().ReadLocalPlayer(pt) && pt.isValid;

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
            ImGui::TextDisabled("Controls:\n  • W / S: Forward / Backward along Yaw\n  • A / D: Strafe Left / Right\n  • Space: Ascend (+Y)\n  • Left Shift / C: Descend (-Y)\n  • In-Air Freezing: When no vertical key is pressed, altitude is frozen in-place!");
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
        // SECTION 3: INVENTORY & COINS (AUTHORITATIVE POINTER CHAIN)
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("🪙  Inventory & Coins", ImGuiTreeNodeFlags_DefaultOpen)) {
            uintptr_t targetAddr = GetCoinAddress();
            int32_t currentVal = 0;
            bool isValid = (targetAddr != 0);
            if (isValid) {
                currentVal = *reinterpret_cast<int32_t*>(targetAddr);
            }

            ImGui::Text("Authoritative Wallet Pointer: ");
            ImGui::SameLine();
            if (isValid) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "0x%08X [CONNECTED]", (unsigned int)targetAddr);
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Current Authoritative Coins: %d (at 0x%08X)", currentVal, (unsigned int)targetAddr);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Coins Pointer: Waiting for player instance...");
            }

            ImGui::Separator();

            // Action Controls
            if (ImGui::Button("+50 Coins", ImVec2(120, 26))) {
                SetCoins(isValid ? (currentVal + 50) : 50);
            }
            ImGui::SameLine();
            if (ImGui::Button("Set 100 Coins (Level Max)", ImVec2(185, 26))) {
                SetCoins(100);
            }
            ImGui::SameLine();
            if (ImGui::Button("Set 999 Coins (Shop Wallet)", ImVec2(195, 26))) {
                SetCoins(999);
            }

            ImGui::Spacing();
            ImGui::Checkbox("Freeze Coins at Value", &m_bFreezeCoins);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt("##TargetCoins", &m_nTargetCoins, 1, 100);

            ImGui::Spacing();

            // Framed Notice Text Box
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.6f, 0.9f, 0.6f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
            ImGui::BeginChild("CoinNoticeBox", ImVec2(0, 75), true);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "ℹ  Wichtiger Hinweis zur Münz-Synchronisation:");
            ImGui::TextWrapped("Hinweis: Der HUD-Zähler im laufenden Level aktualisiert sich erst beim Einsammeln "
                               "einer weiteren Münze, beim Levelwechsel oder direkt im Souvenir-Shop. "
                               "Die Münzen sind im Spiel und im Shop sofort voll verfügbar und abrechenbar!");
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
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
            ImGui::SameLine();
            if (ImGui::Button("Max Tokens (100)", ImVec2(160, 24))) {
                ApplyMaxPawTokens();
            }
        }
    }

} // namespace MadMultiplayer
