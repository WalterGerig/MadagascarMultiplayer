#include "../include/CheatManager.h"
#include "../include/MemoryManager.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include "../vendor/imgui/imgui.h"
#include <cmath>
#include <cstdio>
#include <ctime>

namespace MadMultiplayer {

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

    void CheatManager::Initialize() {
        if (m_initialized) return;

        // Config-Standards laden
        const auto& cfg = Config::Instance().Get();
        m_flightHotkey = cfg.keyToggleFlight;
        m_flightSpeed = cfg.defaultFlightSpeed;
        m_moveSpeedMultiplier = cfg.defaultMoveSpeedMultiplier;
        m_godModeEnabled = (cfg.enableGodModeDefault != 0);
        m_infiniteJumpEnabled = (cfg.enableInfiniteJumpDefault != 0);

        // Waypoint-Slots initialisieren
        for (size_t i = 0; i < m_waypoints.size(); ++i) {
            m_waypoints[i].isValid = false;
            snprintf(m_waypoints[i].label, sizeof(m_waypoints[i].label), "Slot %zu (Leer)", i + 1);
            m_waypoints[i].timestamp[0] = '\0';
        }

        m_initialized = true;
        MAD_LOG("[CheatManager] Cheat Engine & Sandbox erfolgreich initialisiert.");
    }

    void CheatManager::SetFlightEnabled(bool enabled) {
        m_flightEnabled = enabled;
        auto& mem = MemoryManager::Instance();
        if (enabled) {
            // Physik-Opcode patchen (Schwerkraft-Ueberschreibung deaktivieren)
            mem.SetPhysicsPatch(true);
            MAD_LOG("[CheatManager] Noclip / Flugmodus AKTIVIERT (Schwerkraft-Patch aktiv).");
        } else {
            // Physik-Opcode wiederherstellen
            mem.SetPhysicsPatch(false);
            MAD_LOG("[CheatManager] Noclip / Flugmodus DEAKTIVIERT.");
        }
    }

    void CheatManager::TeleportTo(float x, float y, float z) {
        auto& mem = MemoryManager::Instance();
        if (mem.WriteLocalPosition(x, y, z)) {
            MAD_LOG("[CheatManager] Teleport erfolgreich zu (X=%.2f, Y=%.2f, Z=%.2f)", x, y, z);
        } else {
            MAD_LOG("[CheatManager] Teleport fehlgeschlagen: Spieler-Entity ungueltig!");
        }
    }

    void CheatManager::SaveWaypoint(size_t slotIdx) {
        if (slotIdx >= m_waypoints.size()) return;

        PlayerTransform pt{};
        if (MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid) {
            auto& slot = m_waypoints[slotIdx];
            slot.isValid = true;
            slot.x = pt.x;
            slot.y = pt.y;
            slot.z = pt.z;
            slot.yaw = pt.yaw;
            slot.pitch = pt.pitch;

            time_t now = time(nullptr);
            tm tmBuf{};
            localtime_s(&tmBuf, &now);
            snprintf(slot.timestamp, sizeof(slot.timestamp), "%02d:%02d:%02d",
                     tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
            snprintf(slot.label, sizeof(slot.label), "Slot %zu: (%.1f, %.1f, %.1f) um %s",
                     slotIdx + 1, slot.x, slot.y, slot.z, slot.timestamp);

            MAD_LOG("[CheatManager] Checkpoint in Slot %zu gespeichert: %s", slotIdx + 1, slot.label);
        }
    }

    void CheatManager::LoadWaypoint(size_t slotIdx) {
        if (slotIdx >= m_waypoints.size()) return;

        const auto& slot = m_waypoints[slotIdx];
        if (slot.isValid) {
            TeleportTo(slot.x, slot.y, slot.z);
        }
    }

    void CheatManager::ApplyMaxCoins() {
        if (MemoryManager::Instance().WriteCoins(m_customCoins)) {
            MAD_LOG("[CheatManager] Muenzen gesetzt auf %d", m_customCoins);
        }
    }

    void CheatManager::ApplyRefillMangoes() {
        if (MemoryManager::Instance().WriteMangoAmmo(m_customMangoAmmo)) {
            MAD_LOG("[CheatManager] Mangos/Fruechtemunition gesetzt auf %d", m_customMangoAmmo);
        }
    }

    void CheatManager::ApplyMaxPawTokens() {
        if (MemoryManager::Instance().WritePawTokens(m_customTokens)) {
            MAD_LOG("[CheatManager] Pfoten/Tiki-Muenzen gesetzt auf %d", m_customTokens);
        }
    }

    void CheatManager::ApplyFullHealth() {
        if (MemoryManager::Instance().WriteHealth(m_customHealthValue)) {
            MAD_LOG("[CheatManager] Gesundheit auf %d gesetzt.", m_customHealthValue);
        }
    }

    void CheatManager::ProcessFlightMovement(float deltaTime, const PlayerTransform& cur) {
        if (!m_flightEnabled) return;

        // Keine Bewegung wenn der Spieler gerade Text in ImGui tippt
        if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }

        float speed = 35.0f * m_flightSpeed * deltaTime;
        float dx = 0.0f;
        float dy = 0.0f;
        float dz = 0.0f;

        // Blickrichtung aus Yaw
        float radYaw = cur.yaw;
        float fwdX = -sinf(radYaw);
        float fwdZ = cosf(radYaw);
        float rightX = cosf(radYaw);
        float rightZ = sinf(radYaw);

        // Tastenabfragen (W / S / A / D / Space / Shift)
        if (GetAsyncKeyState('W') & 0x8000) {
            dx += fwdX * speed;
            dz += fwdZ * speed;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            dx -= fwdX * speed;
            dz -= fwdZ * speed;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            dx += rightX * speed;
            dz += rightZ * speed;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            dx -= rightX * speed;
            dz -= rightZ * speed;
        }

        // Vertikale Bewegung (Steigen / Sinken)
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            dy += speed;
        }
        if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_LCONTROL) & 0x8000)) {
            dy -= speed;
        }

        if (fabs(dx) > 0.0001f || fabs(dy) > 0.0001f || fabs(dz) > 0.0001f) {
            float newX = cur.x + dx;
            float newY = cur.y + dy;
            float newZ = cur.z + dz;
            MemoryManager::Instance().WriteLocalPosition(newX, newY, newZ);
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

        // Hotkey: Noclip Toggle (Standard: N)
        bool curFlightKey = (GetAsyncKeyState(m_flightHotkey) & 0x8000) != 0;
        if (curFlightKey && !m_prevFlightHotkey) {
            if (!(ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)) {
                ToggleFlight();
            }
        }
        m_prevFlightHotkey = curFlightKey;

        PlayerTransform pt{};
        if (MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid) {
            // Live Koordinaten in manuelle Eingabefelder kopieren, falls diese noch unberuehrt sind
            static bool initialSync = false;
            if (!initialSync) {
                m_manualX = pt.x;
                m_manualY = pt.y;
                m_manualZ = pt.z;
                initialSync = true;
            }

            ProcessFlightMovement(deltaTime, pt);
            ProcessPlayerModifiers(deltaTime, pt);
        }
    }

    void CheatManager::RenderMenu() {
        PlayerTransform pt{};
        bool playerReady = MemoryManager::Instance().ReadLocalPlayer(pt) && pt.isValid;

        // ---------------------------------------------------------------------
        // 1. FLIGHT / NOCLIP MODUS
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("✈  Flight / Noclip Modus (Flugmodus)", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool flight = m_flightEnabled;
            if (ImGui::Checkbox("Noclip / Flugmodus aktivieren [Taste N]", &flight)) {
                SetFlightEnabled(flight);
            }
            ImGui::SameLine();
            if (m_flightEnabled) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[AKTIV - Schwerkraft deaktiviert]");
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[Inaktiv]");
            }

            ImGui::SliderFloat("Flug-Geschwindigkeit", &m_flightSpeed, 0.1f, 10.0f, "%.1fx");
            ImGui::TextDisabled("Steuerung im Flugmodus:\n  • W / S: Vorwaerts / Rueckwaerts\n  • A / D: Seitlich schweben (Strafe)\n  • Leertaste: Aufsteigen (Hoehe +)\n  • Shift / Strg: Absinken (Hoehe -)");
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // 2. TELEPORTATION & CHECKPOINTS
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("📍  Teleportation & Checkpoints", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (playerReady) {
                ImGui::Text("Aktuelle Position: X=%.2f | Y=%.2f | Z=%.2f | Yaw=%.2f rad", pt.x, pt.y, pt.z, pt.yaw);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Spieler nicht aktiv (Menue / Laedt...)");
            }

            ImGui::Separator();
            ImGui::Text("Koordinaten direkt eingeben:");
            ImGui::InputFloat("Ziel-X", &m_manualX, 1.0f, 10.0f, "%.2f");
            ImGui::InputFloat("Ziel-Y (Hoehe)", &m_manualY, 1.0f, 5.0f, "%.2f");
            ImGui::InputFloat("Ziel-Z", &m_manualZ, 1.0f, 10.0f, "%.2f");

            if (ImGui::Button("Ausgewaehlte Koordinaten anspringen [Teleport]", ImVec2(320, 26))) {
                TeleportTo(m_manualX, m_manualY, m_manualZ);
            }
            ImGui::SameLine();
            if (ImGui::Button("Aktuelle Pos uebernehmen")) {
                if (playerReady) {
                    m_manualX = pt.x;
                    m_manualY = pt.y;
                    m_manualZ = pt.z;
                }
            }

            ImGui::Spacing();
            ImGui::Text("Karten-Voreinstellungen (Level Presets):");
            const char* presetNames[g_numMapPresets];
            for (int i = 0; i < g_numMapPresets; ++i) presetNames[i] = g_mapPresets[i].name;

            ImGui::Combo("Preset Ort", &m_selectedPreset, presetNames, g_numMapPresets);
            if (ImGui::Button("Zum Level-Preset teleportieren", ImVec2(240, 24))) {
                const auto& pr = g_mapPresets[m_selectedPreset];
                TeleportTo(pr.x, pr.y, pr.z);
            }

            ImGui::Spacing();
            ImGui::Text("Benutzerdefinierte Checkpoint-Slots:");
            for (size_t i = 0; i < m_waypoints.size(); ++i) {
                ImGui::PushID((int)i);
                ImGui::Text("%s", m_waypoints[i].label);
                ImGui::SameLine(280);
                if (ImGui::Button("Speichern")) {
                    SaveWaypoint(i);
                }
                ImGui::SameLine();
                if (ImGui::Button("Laden")) {
                    LoadWaypoint(i);
                }
                ImGui::PopID();
            }
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // 3. GOD MODE & SPIELERMODIFIKATOREN
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("🛡  God Mode & Modifikatoren", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("God Mode (Unverwundbarkeit / HP sperren)", &m_godModeEnabled);
            ImGui::SameLine();
            if (m_godModeEnabled) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[AKTIV]");
            }

            ImGui::SliderInt("HP Zielwert", &m_customHealthValue, 1, 200);
            if (ImGui::Button("Volle Gesundheit sofort herstellen (100 HP)", ImVec2(280, 24))) {
                ApplyFullHealth();
            }

            ImGui::Separator();
            ImGui::Checkbox("Unendlicher Sprung (Air-Jump mit Leertaste)", &m_infiniteJumpEnabled);
            ImGui::SliderFloat("Super-Sprunghoehe Multiplikator", &m_superJumpMultiplier, 1.0f, 5.0f, "%.1fx");
            ImGui::SliderFloat("Lauf-Geschwindigkeit Multiplikator", &m_moveSpeedMultiplier, 1.0f, 5.0f, "%.1fx");
        }

        ImGui::Spacing();

        // ---------------------------------------------------------------------
        // 4. COLLECTIBLES & STATS SPAWNER
        // ---------------------------------------------------------------------
        if (ImGui::CollapsingHeader("🪙  Collectibles & Stats Spawner", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputInt("Muenzen Anzahl", &m_customCoins);
            if (ImGui::Button("Max Muenzen setzen (999)", ImVec2(220, 24))) {
                m_customCoins = 999;
                ApplyMaxCoins();
            }

            ImGui::Spacing();
            ImGui::InputInt("Mangos / Fruchtmunition", &m_customMangoAmmo);
            if (ImGui::Button("Munition auffuellen (99)", ImVec2(220, 24))) {
                m_customMangoAmmo = 99;
                ApplyRefillMangoes();
            }

            ImGui::Spacing();
            ImGui::InputInt("Pfoten / Tiki-Tokens", &m_customTokens);
            if (ImGui::Button("Max Tiki-Tokens setzen (100)", ImVec2(220, 24))) {
                m_customTokens = 100;
                ApplyMaxPawTokens();
            }
        }
    }

} // namespace MadMultiplayer
