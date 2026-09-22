#pragma once
#include <mutex>
#include <cstdint>

namespace MadMultiplayer {

    struct PlayerTransform {
        float x{ 0.0f };
        float y{ 0.0f };
        float z{ 0.0f };
        float yaw{ 0.0f };
        float pitch{ 0.0f };
        bool  isValid{ false };
        bool  isPaused{ false };
        uint32_t timestampMs{ 0 };
    };

    class ThreadSafeTransform {
    public:
        void Set(const PlayerTransform& transform) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_transform = transform;
        }

        PlayerTransform Get() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_transform;
        }

    private:
        mutable std::mutex m_mutex;
        PlayerTransform m_transform{};
    };

} // namespace MadMultiplayer
