#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "PlayerTransform.h"

namespace MadMultiplayer {

    struct Vector3 {
        float x{ 0.0f };
        float y{ 0.0f };
        float z{ 0.0f };
    };

    struct WaypointSlot {
        bool    isValid{ false };
        Vector3 pos{ 0.0f, 0.0f, 0.0f };
        float   yaw{ 0.0f };
        float   pitch{ 0.0f };
        char    label[64]{ "Leerer Slot" };
        char    timestamp[32]{ "" };
    };

    struct MapPreset {
        const char* name;
        float x, y, z, yaw;
    };

    class CheatManager {
    public:
        static CheatManager& Instance();

        void Initialize();
        void Shutdown();
        void Update(float deltaTime);
        void RenderMenu();

        // 1. Coordinate-Lock Flight / Noclip (100% Data-Driven, No NOPs, No Crash)
        bool IsFlightEnabled() const { return m_flightEnabled; }
        void SetFlightEnabled(bool enabled);
        void ToggleFlight() { SetFlightEnabled(!m_flightEnabled); }

        float GetFlightSpeed() const { return m_flightSpeed; }
        void  SetFlightSpeed(float speed) { m_flightSpeed = speed; m_fFlightSpeed = speed; }

        int  GetFlightHotkey() const { return m_flightHotkey; }
        void SetFlightHotkey(int vkKey) { m_flightHotkey = vkKey; }

        const Vector3& GetFlyPos() const { return m_vFlyPos; }
        const Vector3& GetFlyTarget() const { return m_vFlyPos; }

        // 2. Teleportation & Waypoints
        void SaveWaypoint(size_t slotIdx);
        void LoadWaypoint(size_t slotIdx);
        void TeleportTo(float x, float y, float z);

        // 3. Authoritative Static Coin Pointer Chain (Game.exe + 0x00229628 -> 0x0C -> 0x1C -> 0x534)
        uintptr_t GetCoinAddress();
        bool      SetCoins(int amount);
        bool      IsCoinFreezeEnabled() const { return m_bFreezeCoins; }
        void      SetCoinFreezeEnabled(bool enabled) { m_bFreezeCoins = enabled; }
        int       GetTargetCoins() const { return m_nTargetCoins; }
        void      SetTargetCoins(int target) { m_nTargetCoins = target; }

        // 4. God Mode & Modifiers
        bool IsGodModeEnabled() const { return m_godModeEnabled; }
        void SetGodModeEnabled(bool enabled) { m_godModeEnabled = enabled; }

        bool IsInfiniteJumpEnabled() const { return m_infiniteJumpEnabled; }
        void SetInfiniteJumpEnabled(bool enabled) { m_infiniteJumpEnabled = enabled; }

        float GetSuperJumpMultiplier() const { return m_superJumpMultiplier; }
        void  SetSuperJumpMultiplier(float mult) { m_superJumpMultiplier = mult; }

        float GetMovementSpeedMultiplier() const { return m_moveSpeedMultiplier; }
        void  SetMovementSpeedMultiplier(float mult) { m_moveSpeedMultiplier = mult; }

        // 5. Classic Spawner Aktionen
        void ApplyRefillMangoes();
        void ApplyMaxPawTokens();
        void ApplyFullHealth();

    private:
        CheatManager() = default;
        ~CheatManager();

        bool m_initialized{ false };

        // Flight / Noclip (Coordinate-Lock & Altitude Freeze)
        bool    m_flightEnabled{ false };
        float   m_flightSpeed{ 15.0f }; // Standard: 15.0f
        float   m_fFlightSpeed{ 15.0f };
        int     m_flightHotkey{ VK_F4 }; // Hotkey F4
        bool    m_prevFlightHotkey{ false };
        bool    m_prevNKey{ false };
        Vector3 m_vFlyPos{ 0.0f, 0.0f, 0.0f };

        // Teleportation & Waypoints
        float m_targetPos[3]{ 0.0f, 10.0f, 0.0f };
        int   m_selectedPreset{ 0 };
        std::array<WaypointSlot, 3> m_waypoints{};

        // Authoritative Coin Pointer & Freeze
        bool m_bFreezeCoins{ false };
        int  m_nTargetCoins{ 999 };

        // God Mode & Player Modifiers
        bool  m_godModeEnabled{ false };
        int   m_customHealthValue{ 100 };
        bool  m_infiniteJumpEnabled{ false };
        float m_superJumpMultiplier{ 1.5f };
        bool  m_prevJumpKey{ false };
        float m_moveSpeedMultiplier{ 1.0f };

        // Classic Collectibles
        int m_customMangoAmmo{ 99 };
        int m_customTokens{ 100 };

        void ProcessFlightMovement(float deltaTime, const PlayerTransform& cur);
        void ProcessPlayerModifiers(float deltaTime, const PlayerTransform& cur);
    };

} // namespace MadMultiplayer
