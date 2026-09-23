#pragma once
#include <windows.h>
#include <string>

namespace MadMultiplayer {

    struct ConfigData {
        int         keyToggleMouseMode{ 113 };       // Standard F2 (VK_F2 = 113 / 0x71) -> Maus freigeben / im Spiel fangen
        int         keyToggleOverlay{ 114 };         // Standard F3 (VK_F3 = 114 / 0x72) -> Overlay ein- / ausblenden
        int         keyToggleWidescreen{ 57 };       // Standard Taste 9 (ASCII '9' = 57 / 0x39) -> 16:9 Borderless
        std::string defaultIP{ "127.0.0.1" };
        int         defaultPort{ 27015 };
        int         autoEnableWidescreen{ 0 };
        int         enableFileLogging{ 1 };

        // Cheats & Sandbox
        int         keyToggleFlight{ 78 };           // Taste N (VK_N = 78 / 0x4E)
        float       defaultFlightSpeed{ 1.0f };
        int         enableGodModeDefault{ 0 };
        int         enableInfiniteJumpDefault{ 0 };
        float       defaultMoveSpeedMultiplier{ 1.0f };
    };

    class Config {
    public:
        static Config& Instance();

        void Initialize(HMODULE hDllModule);
        const ConfigData& Get() const { return m_data; }

        void Reload();
        void Save();

        const std::string& GetIniPath() const { return m_iniPath; }

    private:
        Config() = default;
        ~Config() = default;

        ConfigData  m_data;
        std::string m_iniPath;
    };

} // namespace MadMultiplayer
