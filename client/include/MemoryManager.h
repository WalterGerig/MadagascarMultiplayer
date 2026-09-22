#pragma once
#include <windows.h>
#include <cstdint>
#include "PlayerTransform.h"

namespace MadMultiplayer {

    class MemoryManager {
    public:
        // Offsets relativ zur Modulbasis von Game.exe
        static constexpr uintptr_t OFFSET_BASE_PTR       = 0x0021818C;
        static constexpr uintptr_t OFFSET_LEVEL_2        = 0x000000A8;
        static constexpr uintptr_t OFFSET_LEVEL_3        = 0x00000230;

        // Koordinaten-Offsets in der Spieler-Entity
        static constexpr uintptr_t OFF_POS_X_PRIMARY     = 0x150;
        static constexpr uintptr_t OFF_POS_Y_PRIMARY     = 0x154;
        static constexpr uintptr_t OFF_POS_Z_PRIMARY     = 0x158;

        static constexpr uintptr_t OFF_POS_X_SECONDARY   = 0x1F4;
        static constexpr uintptr_t OFF_POS_Y_SECONDARY   = 0x1F8;
        static constexpr uintptr_t OFF_POS_Z_SECONDARY   = 0x1FC;

        // Stats & Collectibles Offsets in der Spieler-Entity
        static constexpr uintptr_t OFF_HEALTH            = 0x184; // 32-bit Integer (Health)
        static constexpr uintptr_t OFF_MAX_HEALTH        = 0x188; // 32-bit Integer (Max Health)
        static constexpr uintptr_t OFF_COINS             = 0x18C; // 32-bit Integer (Coins/Money)
        static constexpr uintptr_t OFF_MANGO_AMMO        = 0x190; // 32-bit Integer (Fruit/Mango Ammo)
        static constexpr uintptr_t OFF_PAW_TOKENS        = 0x194; // 32-bit Integer (Paw/Tiki Tokens)

        // Globale Engine-Adressen
        static constexpr uintptr_t OFFSET_PAUSED         = 0x0022A520;
        static constexpr uintptr_t OFFSET_CAM_PITCH      = 0x002181FC;
        static constexpr uintptr_t OFFSET_CAM_YAW        = 0x00218220;

        static MemoryManager& Instance();

        bool Initialize();
        void Shutdown();

        // Liest den aktuellen Spielerzustand thread-sicher und SEH-abgesichert aus
        bool ReadLocalPlayer(PlayerTransform& outTransform);

        // Schreibt neue Koordinaten in die Spieler-Entity
        bool WriteLocalPosition(float x, float y, float z);
        bool ZeroVelocities();

        // Health & Stats Manipulation
        bool ReadHealth(int32_t& outHealth);
        bool WriteHealth(int32_t health);
        bool ReadCoins(int32_t& outCoins);
        bool WriteCoins(int32_t coins);
        bool ReadMangoAmmo(int32_t& outAmmo);
        bool WriteMangoAmmo(int32_t ammo);
        bool ReadPawTokens(int32_t& outTokens);
        bool WritePawTokens(int32_t tokens);

        // Generische SEH-geschützte Speicherzugriffe
        bool SafeReadBytes(uintptr_t address, void* buffer, size_t size);
        bool SafeWriteBytes(uintptr_t address, const void* buffer, size_t size);
        static bool IsValidUserPointer(uintptr_t ptr);

        uintptr_t GetModuleBase() const { return m_moduleBase; }
        uintptr_t GetPlayerEntity() const { return m_playerEntity; }
        bool EnsurePlayerEntity();

    private:
        MemoryManager() = default;
        ~MemoryManager();

        uintptr_t m_moduleBase{ 0 };
        uintptr_t m_playerEntity{ 0 };
        bool      m_initialized{ false };

        // Interne SEH-sichere Pointer-Auflösung
        bool ResolvePlayerEntityInternal(uintptr_t& outEntity);
    };

} // namespace MadMultiplayer
