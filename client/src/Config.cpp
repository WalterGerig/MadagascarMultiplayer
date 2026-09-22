#include "Config.h"
#include <cstdio>
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
            file << "EnableFileLogging=" << m_data.enableFileLogging << "        ; 1 = Synchrones Logging in multiplayer_debug.log\n";
            file.close();
        }
    }

} // namespace MadMultiplayer
