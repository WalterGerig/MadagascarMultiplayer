#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <windows.h>

namespace MadMultiplayer {

    struct LogMessage {
        std::string timestamp;
        std::string text;
        DWORD threadId;
    };

    class Logger {
    public:
        static Logger& Instance();

        void Initialize();
        void Init() { Initialize(); }
        void Shutdown();

        void Log(const char* fmt, ...);
        
        std::vector<LogMessage> GetLogs() const;
        void ClearLogs();

    private:
        Logger() = default;
        ~Logger();

        std::string GetCurrentTimestamp() const;

        mutable std::mutex m_mutex;
        std::vector<LogMessage> m_logs;
        HANDLE m_hFile{ INVALID_HANDLE_VALUE };
        bool m_initialized{ false };
    };

} // namespace MadMultiplayer

// Helper Macro
#define MAD_LOG(fmt, ...) MadMultiplayer::Logger::Instance().Log(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  MAD_LOG("[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  MAD_LOG("[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) MAD_LOG("[ERROR] " fmt, ##__VA_ARGS__)
