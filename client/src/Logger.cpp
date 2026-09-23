#include "Logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>

namespace MadMultiplayer {

    namespace {
        // Bereinigt ANSI/Windows-1252 Umlaute und Sonderzeichen zu sauberem ASCII fuer ImGui
        std::string SanitizeToCleanAscii(const char* input) {
            if (!input) return "";
            std::string result;
            result.reserve(strlen(input) + 16);
            for (const unsigned char* p = reinterpret_cast<const unsigned char*>(input); *p; ++p) {
                unsigned char c = *p;
                if (c == 0xE4 || c == 0xC4) { result += "ae"; }      // ä, Ä
                else if (c == 0xF6 || c == 0xD6) { result += "oe"; } // ö, Ö
                else if (c == 0xFC || c == 0xDC) { result += "ue"; } // ü, Ü
                else if (c == 0xDF) { result += "ss"; }              // ß
                else if (c >= 32 && c <= 126) { result += (char)c; } // Druckbares ASCII
                else if (c == '\t' || c == '\n' || c == '\r') { result += (char)c; }
                else { result += ' '; }
            }
            return result;
        }
    } // namespace

    Logger& Logger::Instance() {
        static Logger instance;
        return instance;
    }

    Logger::~Logger() {
        Shutdown();
    }

    void Logger::Initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_initialized) return;

        char path[MAX_PATH] = { 0 };
        HMODULE hMod = nullptr;
        // 1. Prioritaet: Ordner von MadMultiplayer.dll (normalerweise patches\)
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(&Logger::Instance), &hMod) && hMod) {
            GetModuleFileNameA(hMod, path, MAX_PATH);
            char* slash = strrchr(path, '\\');
            if (slash) *(slash + 1) = '\0';
            strcat_s(path, sizeof(path), "multiplayer_debug.log");
            m_hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }

        // 2. Prioritaet: Ordner der Game.exe
        if (m_hFile == INVALID_HANDLE_VALUE) {
            GetModuleFileNameA(nullptr, path, MAX_PATH);
            char* slash = strrchr(path, '\\');
            if (slash) *(slash + 1) = '\0';
            strcat_s(path, sizeof(path), "multiplayer_debug.log");
            m_hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }

        // 3. Prioritaet: VirtualStore Ordner fuer Non-Admin UAC Umgebungen
        if (m_hFile == INVALID_HANDLE_VALUE) {
            char localAppData[MAX_PATH] = { 0 };
            if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
                sprintf_s(path, sizeof(path), "%s\\VirtualStore\\Program Files (x86)\\Activision\\Madagascar\\Game\\patches\\multiplayer_debug.log", localAppData);
                m_hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            }
        }

        // 4. Prioritaet: %TEMP% Fallback
        if (m_hFile == INVALID_HANDLE_VALUE) {
            char tempDir[MAX_PATH] = { 0 };
            if (GetTempPathA(MAX_PATH, tempDir) > 0) {
                sprintf_s(path, sizeof(path), "%smultiplayer_debug.log", tempDir);
                m_hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            }
        }

        m_initialized = (m_hFile != INVALID_HANDLE_VALUE);

        if (m_initialized) {
            char header[512];
            sprintf_s(header, sizeof(header),
                      "==================================================================\r\n"
                      " MADAGASCAR MULTIPLAYER DEBUG LOG (SESSION INITIALIZED)\r\n"
                      " File: %s\r\n"
                      "==================================================================\r\n", path);
            DWORD written = 0;
            WriteFile(m_hFile, header, (DWORD)strlen(header), &written, nullptr);
            FlushFileBuffers(m_hFile);
        }
    }

    void Logger::Shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        m_initialized = false;
    }

    std::string Logger::GetCurrentTimestamp() const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char buf[32];
        sprintf_s(buf, "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        return std::string(buf);
    }

    void Logger::Log(const char* fmt, ...) {
        char body[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(body, sizeof(body), fmt, ap);
        va_end(ap);
        body[sizeof(body) - 1] = '\0';

        std::string cleanBody = SanitizeToCleanAscii(body);
        std::string ts = GetCurrentTimestamp();
        DWORD tid = GetCurrentThreadId();

        char formatted[1200];
        if (!cleanBody.empty() && cleanBody[0] == '[') {
            sprintf_s(formatted, "[%s] [TID:%04lu] %s", ts.c_str(), tid, cleanBody.c_str());
        } else {
            sprintf_s(formatted, "[%s] [TID:%04lu] [SYS] %s", ts.c_str(), tid, cleanBody.c_str());
        }

        // 1. In Konsole schreiben
        printf("%s\n", formatted);
        fflush(stdout);

        // 2. In Datei schreiben & Ringpuffer threadsicher aktualisieren
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_initialized) {
            Initialize();
        }

        if (m_hFile != INVALID_HANDLE_VALUE) {
            char line[1250];
            sprintf_s(line, "%s\r\n", formatted);
            DWORD written = 0;
            WriteFile(m_hFile, line, (DWORD)strlen(line), &written, nullptr);
            FlushFileBuffers(m_hFile);
        }

        // 3. Im Ringpuffer für ImGui UI speichern (max 500 Einträge)
        m_logs.push_back({ ts, cleanBody, tid });
        if (m_logs.size() > 500) {
            m_logs.erase(m_logs.begin());
        }
    }

    std::vector<LogMessage> Logger::GetLogs() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_logs;
    }

    void Logger::ClearLogs() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.clear();
    }

} // namespace MadMultiplayer
