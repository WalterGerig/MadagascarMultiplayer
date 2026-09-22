#include "../include/Config.h"
#include <fstream>

namespace MadMultiplayer {

    Config& Config::Instance() {
        static Config instance;
        return instance;
    }

    void Config::Initialize(HMODULE hDllModule) {
        char path[MAX_PATH] = {};
        if (hDllModule) {
            GetModuleFileNameA(hDllModule, path, MAX_PATH);
            char* slash = nullptr;
            for (char* p = path; *p; ++p) {
                if (*p == '\\' || *p == '/') slash = p;
            }
            if (slash) *(slash + 1) = '\0';
        }

        m_iniPath = std::string(path) + "multiplayer_config.ini";

        // Prüfen ob Datei existiert, falls nicht -> mit Standardwerten anlegen
        DWORD attr = GetFileAttributesA(m_iniPath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            Save();
        } else {
            Reload();
        }
    }

    void Config::Reload() {
        if (m_iniPath.empty()) return;

        const char* ini = m_iniPath.c_str();

        m_data.keyToggleMouseMode  = GetPrivateProfileIntA("Keybinds", "ToggleMouseMode", 113, ini);
        // Abwärtskompatibilität: prüfe ToggleOverlayVisibility, falls nicht vorhanden ToggleOverlay
        m_data.keyToggleOverlay    = GetPrivateProfileIntA("Keybinds", "ToggleOverlayVisibility", 
                                       GetPrivateProfileIntA("Keybinds", "ToggleOverlay", 114, ini), ini);
        m_data.keyToggleWidescreen = GetPrivateProfileIntA("Keybinds", "ToggleWidescreen", 57, ini);

        char ipBuf[128] = "127.0.0.1";
        GetPrivateProfileStringA("Network", "DefaultIP", "127.0.0.1", ipBuf, sizeof(ipBuf), ini);
        m_data.defaultIP = ipBuf;

        m_data.defaultPort         = GetPrivateProfileIntA("Network", "DefaultPort", 27015, ini);
        m_data.autoEnableWidescreen= GetPrivateProfileIntA("Display", "AutoEnableWidescreen", 0, ini);
        m_data.enableFileLogging   = GetPrivateProfileIntA("Debug", "EnableFileLogging", 1, ini);

        // Cheats & Sandbox
        m_data.keyToggleFlight            = GetPrivateProfileIntA("Cheats", "KeyToggleFlight", 78, ini);
        m_data.enableGodModeDefault       = GetPrivateProfileIntA("Cheats", "EnableGodModeDefault", 0, ini);
        m_data.enableInfiniteJumpDefault  = GetPrivateProfileIntA("Cheats", "EnableInfiniteJumpDefault", 0, ini);

        char floatBuf[64] = "1.0";
        GetPrivateProfileStringA("Cheats", "DefaultFlightSpeed", "1.0", floatBuf, sizeof(floatBuf), ini);
        m_data.defaultFlightSpeed = (float)atof(floatBuf);
        if (m_data.defaultFlightSpeed <= 0.05f) m_data.defaultFlightSpeed = 1.0f;

        GetPrivateProfileStringA("Cheats", "DefaultMoveSpeedMultiplier", "1.0", floatBuf, sizeof(floatBuf), ini);
        m_data.defaultMoveSpeedMultiplier = (float)atof(floatBuf);
        if (m_data.defaultMoveSpeedMultiplier <= 0.05f) m_data.defaultMoveSpeedMultiplier = 1.0f;
    }

    void Config::Save() {
        if (m_iniPath.empty()) return;

        // Schreibe eine saubere INI-Datei mit Kommentaren
        std::ofstream file(m_iniPath, std::ios::trunc);
        if (file.is_open()) {
            file << "[Keybinds]\n";
            file << "ToggleMouseMode=" << m_data.keyToggleMouseMode << "         ; F2 (VK_F2 = 113) -> Maus freigeben / im Spiel fangen\n";
            file << "ToggleOverlayVisibility=" << m_data.keyToggleOverlay << "   ; F3 (VK_F3 = 114) -> Overlay ein- / ausblenden\n";
            file << "ToggleWidescreen=" << m_data.keyToggleWidescreen << "        ; Taste 9 (ASCII '9' = 57) -> 16:9 Borderless\n\n";

            file << "[Network]\n";
            file << "DefaultIP=" << m_data.defaultIP << "\n";
            file << "DefaultPort=" << m_data.defaultPort << "\n\n";

            file << "[Display]\n";
            file << "AutoEnableWidescreen=" << m_data.autoEnableWidescreen << "     ; 1 = Borderless Widescreen automatisch beim Start\n\n";

            file << "[Debug]\n";
            file << "EnableFileLogging=" << m_data.enableFileLogging << "        ; 1 = Synchrones Logging in multiplayer_debug.log\n\n";

            file << "[Cheats]\n";
            file << "KeyToggleFlight=" << m_data.keyToggleFlight << "           ; N (VK_N = 78 / 0x4E) -> Noclip / Flugmodus umschalten\n";
            file << "DefaultFlightSpeed=" << m_data.defaultFlightSpeed << "       ; Standard-Fluggeschwindigkeit\n";
            file << "EnableGodModeDefault=" << m_data.enableGodModeDefault << "     ; 1 = God Mode beim Start aktiv\n";
            file << "EnableInfiniteJumpDefault=" << m_data.enableInfiniteJumpDefault << " ; 1 = Unendlicher Sprung beim Start aktiv\n";
            file << "DefaultMoveSpeedMultiplier=" << m_data.defaultMoveSpeedMultiplier << " ; Standard-Laufgeschwindigkeitsmultiplikator\n";
            file.close();
        }
    }

} // namespace MadMultiplayer
