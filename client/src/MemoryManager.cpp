#include "../include/MemoryManager.h"
#include <cstdio>
#include <cstring>

namespace MadMultiplayer {

    // Isolierte SEH-Primitiven zur Vermeidung von MSVC C2712 (C++ unwinding Konflikte)
    static bool SafeReadUint32(uintptr_t address, uint32_t& outValue) {
        __try {
            outValue = *reinterpret_cast<const volatile uint32_t*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeReadFloat(uintptr_t address, float& outValue) {
        __try {
            outValue = *reinterpret_cast<const volatile float*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeWriteFloat(uintptr_t address, float value) {
        __try {
            *reinterpret_cast<volatile float*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeReadInt32(uintptr_t address, int32_t& outValue) {
        __try {
            outValue = *reinterpret_cast<const volatile int32_t*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeWriteInt32(uintptr_t address, int32_t value) {
        __try {
            *reinterpret_cast<volatile int32_t*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeReadBytesRaw(uintptr_t address, void* buffer, size_t size) {
        __try {
            std::memcpy(buffer, reinterpret_cast<const void*>(address), size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool SafeWriteBytesRaw(uintptr_t address, const void* buffer, size_t size) {
        __try {
            std::memcpy(reinterpret_cast<void*>(address), buffer, size);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    MemoryManager& MemoryManager::Instance() {
        static MemoryManager instance;
        return instance;
    }

    MemoryManager::~MemoryManager() {
        Shutdown();
    }

    bool MemoryManager::IsValidUserPointer(uintptr_t ptr) {
        // Gültiger x86 User-Space Bereich (0x00400000 bis 0x7FFE0000)
        return (ptr >= 0x00400000 && ptr <= 0x7FFE0000);
    }

    bool MemoryManager::Initialize() {
        if (m_initialized) return true;

        m_moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (!m_moduleBase) {
            printf("[MadMultiplayer::Memory] FEHLER: Konnte GetModuleHandle(NULL) nicht auflösen!\n");
            return false;
        }

        printf("[MadMultiplayer::Memory] Initialisiert. Modul-Basisadresse: 0x%08X\n", (unsigned int)m_moduleBase);

        // Original-Bytes der Physik-Instruktion sichern
        uintptr_t physAddr = m_moduleBase + OFFSET_PHYSICS_OPCODE;
        __try {
            std::memcpy(m_origPhysicsBytes, reinterpret_cast<const void*>(physAddr), sizeof(m_origPhysicsBytes));
            printf("[MadMultiplayer::Memory] Physik-Instruktion @ 0x%08X gesichert (%02X %02X %02X %02X %02X %02X)\n",
                   (unsigned int)physAddr,
                   m_origPhysicsBytes[0], m_origPhysicsBytes[1], m_origPhysicsBytes[2],
                   m_origPhysicsBytes[3], m_origPhysicsBytes[4], m_origPhysicsBytes[5]);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            printf("[MadMultiplayer::Memory] WARNUNG: Konnte Physik-Instruktion nicht lesen (SEH Exception)!\n");
        }

        m_initialized = true;
        return true;
    }

    void MemoryManager::Shutdown() {
        if (!m_initialized) return;

        // Physik-Patch zurücksetzen falls aktiv
        if (m_physicsPatched) {
            SetPhysicsPatch(false);
        }

        m_initialized = false;
        m_playerEntity = 0;
        printf("[MadMultiplayer::Memory] Subsystem heruntergefahren.\n");
    }

    bool MemoryManager::ResolvePlayerEntityInternal(uintptr_t& outEntity) {
        if (!m_moduleBase) return false;

        uint32_t lvl1 = 0;
        if (!SafeReadUint32(m_moduleBase + OFFSET_BASE_PTR, lvl1) || !IsValidUserPointer(lvl1)) {
            return false;
        }

        uint32_t lvl2 = 0;
        if (!SafeReadUint32(lvl1 + OFFSET_LEVEL_2, lvl2) || !IsValidUserPointer(lvl2)) {
            return false;
        }

        uint32_t player = 0;
        if (!SafeReadUint32(lvl2 + OFFSET_LEVEL_3, player) || !IsValidUserPointer(player)) {
            return false;
        }

        outEntity = player;
        return true;
    }

    bool MemoryManager::ReadLocalPlayer(PlayerTransform& outTransform) {
        if (!m_initialized) {
            outTransform.isValid = false;
            return false;
        }

        uintptr_t entity = 0;
        if (!ResolvePlayerEntityInternal(entity)) {
            m_playerEntity = 0;
            outTransform.isValid = false;
            return false;
        }

        m_playerEntity = entity;

        // Koordinaten auslesen (Primary Offset +0x150, Fallback/Secondary +0x1F4)
        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!SafeReadFloat(entity + OFF_POS_X_PRIMARY, x) ||
            !SafeReadFloat(entity + OFF_POS_Y_PRIMARY, y) ||
            !SafeReadFloat(entity + OFF_POS_Z_PRIMARY, z)) {
            outTransform.isValid = false;
            return false;
        }

        // Kamera Pitch & Yaw auslesen
        float pitch = 0.0f, yaw = 0.0f;
        SafeReadFloat(m_moduleBase + OFFSET_CAM_PITCH, pitch);
        SafeReadFloat(m_moduleBase + OFFSET_CAM_YAW, yaw);

        // Pause Flag
        uint32_t pauseVal = 0;
        SafeReadUint32(m_moduleBase + OFFSET_PAUSED, pauseVal);

        outTransform.x = x;
        outTransform.y = y;
        outTransform.z = z;
        outTransform.pitch = pitch;
        outTransform.yaw = yaw;
        outTransform.isPaused = (pauseVal != 0);
        outTransform.isValid = true;
        outTransform.timestampMs = GetTickCount();

        return true;
    }

    bool MemoryManager::WriteLocalPosition(float x, float y, float z) {
        if (!m_playerEntity || !IsValidUserPointer(m_playerEntity)) {
            return false;
        }

        bool success = true;
        // In primäre und sekundäre Offsets schreiben für vollständige Konsistenz
        success &= SafeWriteFloat(m_playerEntity + OFF_POS_X_PRIMARY, x);
        success &= SafeWriteFloat(m_playerEntity + OFF_POS_Y_PRIMARY, y);
        success &= SafeWriteFloat(m_playerEntity + OFF_POS_Z_PRIMARY, z);

        SafeWriteFloat(m_playerEntity + OFF_POS_X_SECONDARY, x);
        SafeWriteFloat(m_playerEntity + OFF_POS_Y_SECONDARY, y);
        SafeWriteFloat(m_playerEntity + OFF_POS_Z_SECONDARY, z);

        return success;
    }

    bool MemoryManager::EnsurePlayerEntity() {
        if (!m_initialized) return false;
        uintptr_t entity = 0;
        if (ResolvePlayerEntityInternal(entity) && IsValidUserPointer(entity)) {
            m_playerEntity = entity;
            return true;
        }
        m_playerEntity = 0;
        return false;
    }

    bool MemoryManager::ReadHealth(int32_t& outHealth) {
        if (!EnsurePlayerEntity()) return false;
        return SafeReadInt32(m_playerEntity + OFF_HEALTH, outHealth);
    }

    bool MemoryManager::WriteHealth(int32_t health) {
        if (!EnsurePlayerEntity()) return false;
        bool ok = SafeWriteInt32(m_playerEntity + OFF_HEALTH, health);
        SafeWriteInt32(m_playerEntity + OFF_MAX_HEALTH, (health > 100) ? health : 100);
        return ok;
    }

    bool MemoryManager::ReadCoins(int32_t& outCoins) {
        if (!EnsurePlayerEntity()) return false;
        return SafeReadInt32(m_playerEntity + OFF_COINS, outCoins);
    }

    bool MemoryManager::WriteCoins(int32_t coins) {
        if (!EnsurePlayerEntity()) return false;
        return SafeWriteInt32(m_playerEntity + OFF_COINS, coins);
    }

    bool MemoryManager::ReadMangoAmmo(int32_t& outAmmo) {
        if (!EnsurePlayerEntity()) return false;
        return SafeReadInt32(m_playerEntity + OFF_MANGO_AMMO, outAmmo);
    }

    bool MemoryManager::WriteMangoAmmo(int32_t ammo) {
        if (!EnsurePlayerEntity()) return false;
        return SafeWriteInt32(m_playerEntity + OFF_MANGO_AMMO, ammo);
    }

    bool MemoryManager::ReadPawTokens(int32_t& outTokens) {
        if (!EnsurePlayerEntity()) return false;
        return SafeReadInt32(m_playerEntity + OFF_PAW_TOKENS, outTokens);
    }

    bool MemoryManager::WritePawTokens(int32_t tokens) {
        if (!EnsurePlayerEntity()) return false;
        return SafeWriteInt32(m_playerEntity + OFF_PAW_TOKENS, tokens);
    }

    bool MemoryManager::SafeReadBytes(uintptr_t address, void* buffer, size_t size) {
        if (!address || !buffer || !IsValidUserPointer(address)) return false;
        return SafeReadBytesRaw(address, buffer, size);
    }

    bool MemoryManager::SafeWriteBytes(uintptr_t address, const void* buffer, size_t size) {
        if (!address || !buffer || !IsValidUserPointer(address)) return false;
        return SafeWriteBytesRaw(address, buffer, size);
    }

    bool MemoryManager::SetPhysicsPatch(bool enable) {
        if (!m_initialized) return false;

        uintptr_t physAddr = m_moduleBase + OFFSET_PHYSICS_OPCODE;
        DWORD oldProtect = 0;

        if (!VirtualProtect(reinterpret_cast<void*>(physAddr), sizeof(m_origPhysicsBytes), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            printf("[MadMultiplayer::Memory] VirtualProtect fehlgeschlagen! Error: %lu\n", GetLastError());
            return false;
        }

        if (enable) {
            uint8_t nops[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
            std::memcpy(reinterpret_cast<void*>(physAddr), nops, sizeof(nops));
            m_physicsPatched = true;
            printf("[MadMultiplayer::Memory] Physik-Overwrite @ 0x%08X mit NOPs aktiviert.\n", (unsigned int)physAddr);
        } else {
            std::memcpy(reinterpret_cast<void*>(physAddr), m_origPhysicsBytes, sizeof(m_origPhysicsBytes));
            m_physicsPatched = false;
            printf("[MadMultiplayer::Memory] Physik-Overwrite @ 0x%08X wiederhergestellt.\n", (unsigned int)physAddr);
        }

        VirtualProtect(reinterpret_cast<void*>(physAddr), sizeof(m_origPhysicsBytes), oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(physAddr), sizeof(m_origPhysicsBytes));
        return true;
    }

} // namespace MadMultiplayer
