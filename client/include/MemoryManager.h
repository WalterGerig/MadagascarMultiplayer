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

        // Globale Engine-Adressen
        static constexpr uintptr_t OFFSET_PAUSED         = 0x0022A520;
        static constexpr uintptr_t OFFSET_CAM_PITCH      = 0x002181FC;
        static constexpr uintptr_t OFFSET_CAM_YAW        = 0x00218220;
        static constexpr uintptr_t OFFSET_PHYSICS_OPCODE = 0x00028E9C; // fstp dword ptr [ebp+1F8h] (6 Bytes)

        static MemoryManager& Instance();

        bool Initialize();
        void Shutdown();

        // Liest den aktuellen Spielerzustand thread-sicher und SEH-abgesichert aus
        bool ReadLocalPlayer(PlayerTransform& outTransform);

        // Schreibt neue Koordinaten in die Spieler-Entity
        bool WriteLocalPosition(float x, float y, float z);

        // NOP-Patch für Physik-Overwrite (0x00428E9C) aktivieren / deaktivieren
        bool SetPhysicsPatch(bool enable);
        bool IsPhysicsPatched() const { return m_physicsPatched; }

        uintptr_t GetModuleBase() const { return m_moduleBase; }
        uintptr_t GetPlayerEntity() const { return m_playerEntity; }

    private:
        MemoryManager() = default;
        ~MemoryManager();

        uintptr_t m_moduleBase{ 0 };
        uintptr_t m_playerEntity{ 0 };
        bool      m_initialized{ false };
        bool      m_physicsPatched{ false };
        uint8_t   m_origPhysicsBytes[6]{ 0xD9, 0x9D, 0xF8, 0x01, 0x00, 0x00 };

        // Interne SEH-sichere Pointer-Auflösung
        bool ResolvePlayerEntityInternal(uintptr_t& outEntity);
        static bool IsValidUserPointer(uintptr_t ptr);
    };

} // namespace MadMultiplayer
